#include <unistd.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <vector>
#include <getopt.h>
#include <cmath>
#include <cstdio>
#include "yampl/SocketFactory.h"
#include "yampl/Exceptions.h"
#include "APE/BufferContainer.hpp"
#include "APE/BufferAccessor.hpp"
#include <thread>
#include <chrono>
#include <tbb/atomic.h>
#include <APE/ServerDefines.hpp>
#include <condition_variable>
#include <mutex>
#include <algorithm>
#include "APE/ConfigTree.hpp"
#include "APE/YamlConfig.hpp"
#include "APE/Log.hpp"
#include "uuid/uuid.h"

tbb::atomic<int> onFlight=0;
tbb::atomic<bool> atEnd=false;
std::mutex recvMutex;
std::condition_variable recvCond;
std::shared_ptr<APE::Log> m_log;

void nullDealloc(void*,void*){}

void printUsage(std::string myName){
  if(myName.rfind('/')!=std::string::npos)myName=myName.substr(myName.rfind('/')+1);
  std::cout<<"An APE client to send dummy data with varying sizes to APE server"<<std::endl
	   <<"Usage: "
	   <<myName<<" "<<std::endl
	   <<"      -u,--upper-limit <size>      Maximum buffer size to send (default=1<<24)"<<std::endl
	   <<"      -l,--lower-limit <size>      Minimum buffer size to send (default=4)"<<std::endl
	   <<"      -s,--num-steps   <steps>     Number of data size steps to send between limits (default=11)"<<std::endl
	   <<"      -n,--num-trial   <trials>    How many times each step will be sent to server (default=1000)"<<std::endl
	   <<"      -L,--linear-steps            Whether to use linear or logarithmic step sizes (default=log steps)"<<std::endl
	   <<"      -c,--config-name              name of the yaml Configuration (default=EchoConfig.yaml)"<<std::endl
	   <<"      -h,--help     This help"<<std::endl
	   <<std::endl;

}

void doLoop(APE::BufferContainer *b,int nLoops,yampl::ISocket* ssocket){
  int onFlightLimit=4<<20/b->getTransferSize();
  for(int i=0;i<nLoops;i++){
    ssocket->send(APE::BufferAccessor::getRawBuffer(*b),b->getTransferSize());
    if((i & 0x1f)==0x1f)std::cout<<".";
    if((i & 0x3ff)==0x3ff)std::cout<<std::endl;
    onFlight++;
    while(onFlight>onFlightLimit){
      std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
  }
}

void receiveResult(yampl::ISocket* rsocket,size_t maxBuffSize,
		   std::chrono::high_resolution_clock::time_point *retTime){
  APE::BufferContainer *RecvBuff=new APE::BufferContainer(maxBuffSize);
  size_t s=RecvBuff->getTransferSize();
  void *rBuff=APE::BufferAccessor::getRawBuffer(*RecvBuff);
  while( !atEnd ){
    while( onFlight>0){
      ssize_t l=rsocket->tryRecv(rBuff,s,-1l);
      if(l>10)onFlight--;
    }
    *retTime=std::chrono::high_resolution_clock::now();
  }
  delete RecvBuff;
  std::unique_lock<std::mutex> lock(recvMutex);
  recvCond.notify_all();
}

int main(int argc,char *argv[]){
  size_t maxBuff=1<<24;
  size_t minBuff=4;
  unsigned int nSteps=11;
  unsigned int nTrials=1000;
  bool logSteps=true;
  int c;
  m_log=std::make_shared<APE::Log>("APE",APE::Log::INFO);
  std::string sockName("apeSock");
  std::string configName("EchoConfig.yaml");
  while (1) {
    long val=0;
    int option_index = 0;
    static struct option long_options[] = {
      {"help", 0, 0, 'h'},
      {"upper-limit", 1, 0, 'u'},
      {"lower-limit", 1, 0, 'l'},
      {"num-steps", 1, 0, 's'},
      {"num-trials", 1, 0, 'n'},
      {"linear-steps", 0, 0, 'L'},
      {"socket-name",1,0,'S'},
      {"config-name",1,0,'c'},
      {0, 0, 0, 0}
    };

    c = getopt_long(argc, argv, "hu:s:n:l:LS:c:",
		    long_options, &option_index);
    if (c == -1)
      break;
    switch (c) {
    case 'h':
      printUsage(argv[0]);
      exit(EXIT_SUCCESS);
      break;
    case 'u':
      errno=0;
      val=strtol(optarg,0,10);
      if(errno==0 && val>0)maxBuff=val;
      break;
    case 'l':
      errno=0;
      val=strtol(optarg,0,10);
      if(errno==0 && val>0)minBuff=val;
      break;
    case 's':
      nSteps=atoi(optarg);
      if(nSteps<1)nSteps=1;
      break;
    case 'n':{
      nTrials=atoi(optarg);
      break;
    }
    case 'L':
      logSteps=false;
    case 'S':
      if(strlen(optarg)>0){
	sockName=std::string(optarg);
	break;
      }else{
	printUsage(argv[0]);	
	return EXIT_FAILURE;
      }
    case 'c':
      if(strlen(optarg)>0){
	configName=std::string(optarg);
	break;
      }else{
	printUsage(argv[0]);	
	return EXIT_FAILURE;
      }
    default:
      printf("unknown parameter! getopt returned character code 0%o ??\n", c);
      return EXIT_FAILURE;
    }
  }
  if(maxBuff<minBuff)std::swap(maxBuff,minBuff);
  if(minBuff<1)minBuff=1;
  std::shared_ptr<APEConfig::ConfigTree> config;
  try{
    config=APEConfig::ConfigIO::read(configName,"yaml");
  }catch(std::exception &ex){
    // std::cerr<<"Parsing config file failed with "<<ex.what()<<std::endl;
    // exit(EXIT_FAILURE);
    // config is missing
  }
  yampl::ISocketFactory *factory = new yampl::SocketFactory();
  yampl::ISocket *ssocket =0;
  yampl::ISocket *rsocket =0;
  char sockBuff[1000];
  uid_t userUID=geteuid();
  if(config){
    std::map<std::string,APE::Log::Level> llmap={{"VERBOSE",APE::Log::VERBOSE},
						 {"DEBUG",APE::Log::DEBUG},
						 {"INFO",APE::Log::INFO},
						 {"WARNING",APE::Log::WARNING},
						 {"ERROR",APE::Log::ERROR},
						 {"FATAL",APE::Log::FATAL}};
    auto SC=config->getSubtree("Server");
    auto CC=config->getSubtree("Client");
    try{
      std::string logLevel=CC->getValue<>("LogLevel");
      std::transform(std::begin(logLevel),std::end(logLevel),std::begin(logLevel),::toupper);
      if (llmap.find(logLevel)==llmap.end()){
	*m_log<<APE::Log::ERROR<<"Loglevel is not defined properly \""<<logLevel<<"\""<<std::endl;
      }else{
	m_log=std::make_shared<APE::Log>("APE",llmap[logLevel]);
      }
    }catch(std::exception& ex){
      
    }
    std::string commType=SC->getValue<std::string>("CommType");
    *m_log<<APE::Log::VERBOSE<<"Comm Type= \""<<commType<<"\""<<std::endl;
    std::string commChUp=SC->getValue<>("CommChannelUp");
    *m_log<<APE::Log::VERBOSE<<"Comm Channel upload/send= \""<<commChUp<<"\""<<std::endl;
    std::string commChDown=SC->getValue<>("CommChannelDown");
    *m_log<<APE::Log::VERBOSE<<"Comm Channel download/receive= \""<<commChDown<<"\""<<std::endl;
    bool useUID=SC->getValue<bool>("CommChannelUseUID");
    *m_log<<APE::Log::VERBOSE<<"Comm Channel Use UID= \""<<useUID<<"\""<<std::endl;
    auto cType=yampl::LOCAL_SHM;
    if(commType=="PIPE")cType=yampl::LOCAL_PIPE;
    if(commType=="ZMQ")cType=yampl::DISTRIBUTED;
    if(useUID){
      snprintf(sockBuff,1000,"%s_%u",commChDown.c_str(),userUID);
    }else{
      snprintf(sockBuff,1000,"%s",commChDown.c_str());      
    }
    *m_log<<APE::Log::VERBOSE<<"Comm Send/Upload channel \""<<sockBuff<<"\""<<std::endl;
    //std::cout<<"Using Send/Upload channel "<<sockBuff<<std::endl;
    yampl::Channel channelS(sockBuff,cType);
    if(useUID){
      snprintf(sockBuff,1000,"%s_%u",commChUp.c_str(),userUID);
    }else{
      snprintf(sockBuff,1000,"%s",commChUp.c_str());      
    }
    std::cout<<"Using Receive/Download channel "<<sockBuff<<std::endl;
    yampl::Channel channelR(sockBuff,cType);
    *m_log<<APE::Log::VERBOSE<<"Comm Receive/Download channel \""<<sockBuff<<"\""<<std::endl;
    uuid_t uuid;
    uuid_clear(uuid);
    char uuidbuff[40];
    uuid_generate(uuid);
    uuid_unparse(uuid,uuidbuff);
    uuidbuff[36]='\0';
    std::string uuids(uuidbuff);
    std::cerr<<"UUID= "<<uuids<<std::flush;
    //ssocket= factory->createClientSocket(channelS);
    ssocket= factory->createClientSocket(channelS,yampl::COPY_DATA,nullDealloc,std::string(uuids));
    uuid_generate(uuid);
    uuid_unparse(uuid,uuidbuff);
    rsocket= factory->createClientSocket(channelR,yampl::COPY_DATA,nullDealloc,std::string(uuidbuff));
    //rsocket= factory->createClientSocket(channelR);

    maxBuff=CC->getValue<unsigned int>("UpperLimit");
    *m_log<<APE::Log::VERBOSE<<"Upper Limit \""<<maxBuff<<"\""<<std::endl;
    minBuff=CC->getValue<unsigned int>("LowerLimit");
    *m_log<<APE::Log::VERBOSE<<"Lower Limit \""<<minBuff<<"\""<<std::endl;    
    nSteps=CC->getValue<unsigned int>("NumSteps");
    *m_log<<APE::Log::VERBOSE<<"Number of steps \""<<nSteps<<"\""<<std::endl;    
    nTrials=CC->getValue<unsigned int>("NumTrials");
    *m_log<<APE::Log::VERBOSE<<"Number of trials at each step \""<<nTrials<<"\""<<std::endl;    
    logSteps=!CC->getValue<bool>("LinearSteps");
    *m_log<<APE::Log::VERBOSE<<"Use Logarithmic stepping \""<<logSteps<<"\""<<std::endl;    
  }else{
    snprintf(sockBuff,1000,"%s_%u",sockName.c_str(),userUID);
    yampl::Channel channelS(sockBuff,yampl::LOCAL_SHM);
    snprintf(sockBuff,1000,"%s_upload_%u",sockName.c_str(),userUID);
    yampl::Channel channelR(sockBuff,yampl::LOCAL_SHM);
    ssocket= factory->createClientSocket(channelS);
    rsocket= factory->createClientSocket(channelR);
  }
  auto connRequest=new APE::BufferContainer(sizeof(int));
  int pid=(int)getpid();
  connRequest->setModule(SERVER_MODULE);
  connRequest->setAlgorithm(ALG_OPENING_CONNECTION);
  *(int*)(connRequest->getBuffer())=pid;
  auto buff=new APE::BufferContainer(maxBuff);
  std::vector<size_t> steps;
  steps.reserve(nSteps+1);
  steps.push_back(minBuff);
  if(minBuff!=maxBuff){
    if(logSteps){
      double min=log(minBuff);
      double max=log(maxBuff);
      double delta=(max-min)/(double)nSteps;
      for(int i=1;i<nSteps;i++){
	steps.push_back(exp(min+i*delta));
      }
    }else{
      double delta=(maxBuff-minBuff)/(double)nSteps;
      for(int i=1;i<nSteps;i++){
	steps.push_back(minBuff+i*delta);
      }
    }
    steps.push_back(maxBuff);
  }
  {
    auto m=*m_log<<APE::Log::INFO;
      m<<"Running for data sizes ";
      for(auto i:steps)m<<i<<" ";
      m<<std::endl;
  }
  std::vector<double> timings;
  timings.reserve(steps.size());
  *m_log<<APE::Log::INFO<<"Initiating connection to server"<<std::endl;
  auto tFirst=std::chrono::high_resolution_clock::now();
  ssocket->send(APE::BufferAccessor::getRawBuffer(*connRequest),connRequest->getTransferSize());
  *m_log<<APE::Log::VERBOSE<<"Sent connection request send socket "<<std::endl;
  rsocket->send(APE::BufferAccessor::getRawBuffer(*connRequest),connRequest->getTransferSize());
  *m_log<<APE::Log::VERBOSE<<"Sent connection request recv socket "<<std::endl;
  *m_log<<APE::Log::INFO<<"Sent Connection requests. Starting transfers"<<std::endl;
  buff->setModule(0xE1000000);
  std::chrono::high_resolution_clock::time_point tEnd;
  std::thread thr(receiveResult,rsocket,maxBuff,&tEnd);
  for(auto i : steps){
    buff->setPayloadSize(i);
    auto ts=std::chrono::high_resolution_clock::now();
    doLoop(buff,nTrials,ssocket);
    while(onFlight>0){
      std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
    auto te=std::chrono::high_resolution_clock::now();
    *m_log<<APE::Log::INFO<<i<<" bytes completed"<<std::endl;
    timings.push_back(std::chrono::duration_cast<std::chrono::microseconds>(te-ts).count()/1000.);
  }
  atEnd=true;
  {
    std::unique_lock<std::mutex> lock(recvMutex);  
    while(!recvCond.wait_for(lock,std::chrono::milliseconds(10),[](){return onFlight==0;})){
      if(onFlight%10==0)*m_log<<APE::Log::INFO<<"Remaining results="<<onFlight<<" to receive"<<std::endl;
    }
  }
  *m_log<<APE::Log::INFO<<"DataTransfer finished closing connection (onFlight="<<onFlight<<")"<<std::endl;
  connRequest->setModule(SERVER_MODULE);
  connRequest->setAlgorithm(ALG_CLOSING_CONNECTION);
  ssocket->send(APE::BufferAccessor::getRawBuffer(*connRequest), connRequest->getTransferSize());
  *m_log<<APE::Log::INFO<<"Connection closed"<<std::endl;
  delete connRequest;
  delete rsocket;
  delete ssocket;
  delete factory;
  delete buff;
  thr.detach();
  *m_log<<APE::Log::INFO<<"Total time ="<<std::chrono::duration_cast<std::chrono::milliseconds>(tEnd-tFirst).count()/1000.0<<" (s)"<<std::endl;
  *m_log<<APE::Log::INFO<<"Timing statistics for steps"<<std::endl;
  char stbuff[1000];
  snprintf(stbuff,1000,"%20s, %15s (ms), %15s (ms)","Data size","Total time","Avg time");
  *m_log<<APE::Log::INFO<<stbuff<<std::endl;
  size_t totalSize=0;
  for(size_t t=0;t<steps.size();t++){
    totalSize+=steps.at(t)*nTrials;
    snprintf(stbuff,1000,"%20ld, %15.3f (ms), %15.3f (ms)",steps.at(t),timings.at(t),timings.at(t)/nTrials);
    *m_log<<APE::Log::INFO<<stbuff<<std::endl;
  }
  *m_log<<APE::Log::INFO<<"Total bytes transferred= "<<totalSize<<std::endl;
  return EXIT_SUCCESS;
}
