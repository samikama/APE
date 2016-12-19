#include <unistd.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>
#include <vector>
#include <cstdio>
#include <getopt.h>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include "yampl/SocketFactory.h"
#include "yampl/Exceptions.h"
#include "APE/BufferContainer.hpp"
#include "APE/BufferAccessor.hpp"
#include <thread>
#include <chrono>
#include <tuple>
#include <algorithm>
#include <set>
#include <tbb/atomic.h>
#include <APE/ServerDefines.hpp>
#include <condition_variable>
#include <mutex>
#include "APE/ConfigTree.hpp"
#include "APE/YamlConfig.hpp"
#include "uuid/uuid.h"

tbb::atomic<int> onFlight=0;
tbb::atomic<bool> atEnd=false;
std::mutex recvMutex;
std::condition_variable recvCond;
void nullDealloc(void*,void*){}

void printUsage(std::string myName){
  if(myName.rfind('/')!=std::string::npos)myName=myName.substr(myName.rfind('/')+1);
  std::cout<<"An APE client to send pre-recoderd data to APE server"<<std::endl
	   <<"Usage: "
	   <<myName<<" "<<std::endl
	   <<"      -n,--numEvents   <nevents>   How many events will be sent in total (data files will be reused)"<<std::endl
	   <<"      -c,--config-name             name of the yaml Configuration (default=UtilConfig.yaml)"<<std::endl
	   <<"      -h,--help     This help"<<std::endl
	   <<std::endl;

}

class Event{
public:
  Event(uint64_t eventId):m_eventId(eventId),last(0){};
  ~Event(){};
  bool addRequest(int reqId,int module,int alg,std::shared_ptr<APE::BufferContainer> buff,bool hasResp=true){
    auto in=std::find_if(std::begin(m_buffers),std::end(m_buffers),[reqId](const std::tuple<int,std::shared_ptr<APE::BufferContainer>,bool>&a){return std::get<0>(a)==reqId;});
    if(in!=std::end(m_buffers))return false;
    m_buffers.emplace_back(std::make_tuple(reqId,buff,hasResp));
    std::sort(std::begin(m_buffers),std::end(m_buffers),[](const std::tuple<int,std::shared_ptr<APE::BufferContainer>,bool>&a, 
							   const std::tuple<int,std::shared_ptr<APE::BufferContainer>,bool>&b){
		return std::get<0>(a)<std::get<0>(b);});
    return true;
  }
  std::pair<std::shared_ptr<APE::BufferContainer>,bool> next(){
    if(last>=m_buffers.size()){
      return std::make_pair(std::shared_ptr<APE::BufferContainer>(0),false);
    };
    auto b=std::make_pair(std::get<1>(m_buffers.at(last)),std::get<2>(m_buffers.at(last)));
    last++;
    return b;
  }
  bool addResponse(int module,int alg){return false;}
  uint64_t getEventId(){return m_eventId;}
  void resetRequests(){last=0;}
private:
  std::vector<std::tuple<int,std::shared_ptr<APE::BufferContainer>,bool> > m_buffers;
  uint64_t m_eventId;
  int last;
};

bool name2fields(const std::string &name,uint64_t *evId, int *module,int *alg,int* req,int *needResponse){
  int currPos=0;
  char typ[50];
  uint64_t e;
  int m,a,r=-1,nr=0;
  if(name.substr(0,3)=="Req"){
    //std::cout<<name.substr(4).c_str()<<std::endl;
    sscanf(name.substr(4).c_str(),"%08ld-%d-%d-%d-%d.dat",&e,&m,&a,&r,&nr);
    *evId=e;
    *module=m;
    *alg=a;
    *req=r;
    *needResponse=nr;
  }else{
    sscanf(name.substr(5).c_str(),"%08ld-%d-%d.dat",&e,&m,&a);    
    *evId=e;
    *module=m;
    *alg=a;
    *req=-1;
    *needResponse=nr;
  }
  return true;
} 

struct InitFile{
  uint64_t ev;
  int mod;
  int alg;
};

std::vector<Event> loadFiles(const std::string &path,const std::set<int> &enabledModules,
			     std::vector<Event>& initEvts,std::vector<InitFile> &initFiles
			     ){
  std::vector<Event> events;
  std::map<uint64_t,Event*> mEvents;
  std::map<uint64_t,Event*> initEvents;
  std::map<uint64_t,Event*>::iterator it;
  if(path.empty())return events;
  DIR *dir;
  struct dirent *ent;
  struct stat sb;
  int fd;
  std::set<std::string> files;

  if ((dir = opendir (path.c_str())) != NULL) {
    while ((ent = readdir (dir)) != NULL) {
      std::string name(ent->d_name);
      if(name=="."||name=="..") continue;
// #ifdef _DIRENT_HAVE_D_TYPE  
//       if(ent->d_type!=DT_REG){
// 	continue;
//       }
// #else
      if(!lstat((path+"/"+name).c_str(),&sb)){
	if(!S_ISREG(sb.st_mode)){
	  continue;
	}
      }
      
//#endif
      if(name.substr(0,2)!="Re")continue;
      files.insert(name);
    }
    closedir(dir);
    char reqNameBuff[300];
    for(const auto & name:files){
      uint64_t evId=0;
      int module=0;
      int alg=0;
      int req=0;
      int needRes=0;
      name2fields(name,&evId,&module,&alg,&req,&needRes);
      //std::cout<<"Processing "<<name<<" evId="<<evId<<" mod="<<module<<" alg="<<alg<<" req="<<req<<" needRes="<<needRes<<std::endl;
      if(req==-1)continue;// ignore response files
      if(enabledModules.find(module)==enabledModules.end())continue;
      bool inInitList=false;
      for(auto& init:initFiles){
	if((init.ev==evId) && (init.mod==module) && init.alg==alg){
	  std::cout<<"Found init file Ev="<<evId<<" mod= "<<module<<" alg= "<<alg<<" "<<name<<std::endl;
	  inInitList=true;
	  break;
	}
      }
      if(inInitList){
	if((it=initEvents.find(evId))==initEvents.end()){
	  auto ins=initEvents.emplace(evId,new Event(evId));
	  it=ins.first;
	}
      }else{
	if((it=mEvents.find(evId))==mEvents.end()){
	  auto ins=mEvents.emplace(evId,new Event(evId));
	  it=ins.first;
	}
      }
      snprintf(reqNameBuff,300,"Resp-%08ld-%d-%d.dat",evId,module,alg);
      //bool hasResp=(files.find(reqNameBuff)!=files.end());

      bool hasResp=(needRes==1);
      if(inInitList&& hasResp)std::cout<<reqNameBuff<<std::endl;
      Event* ev=it->second;
      fd=open((path+std::string("/")+name).c_str(),O_RDONLY);
      if(fd==-1){
	std::cerr<<"Can not open file "<<name<<std::endl;
	exit(EXIT_FAILURE);
      }
      if(fstat(fd,&sb)==-1){
	std::cerr<<"Can not fstat() file "<<name<<std::endl;
	exit(EXIT_FAILURE);
      }
      size_t fsize=sb.st_size;
      auto bc=std::make_shared<APE::BufferContainer>(fsize);//some extra bytes but who cares!!!
      void* buff=APE::BufferAccessor::getRawBuffer(*bc);
      if(read(fd,buff,fsize)!=fsize){
	std::cerr<<"Reading file "<<name<<" failed."<<std::endl;
	exit(EXIT_FAILURE);
      }
      close(fd);
      ev->addRequest(req,module,alg,bc,hasResp);
    }
    events.reserve(mEvents.size());
    for(const auto& evs:mEvents){
      events.emplace_back(*(evs.second));
    }
    initEvts.reserve(initEvents.size());
    for(const auto& evs:initEvents){
      initEvts.emplace_back(*(evs.second));
    }
  } else {
    /* could not open directory */
    perror ("");
    std::cerr<<"Can't open "<<path<<std::endl;
    //return EXIT_FAILURE;
  }
  return events;
}


void doLoop(std::vector<Event>& events,unsigned int nevents,
	    yampl::ISocket* ssocket,
	    yampl::ISocket* rsocket){
  char * recvBuff=new char[1<<25];
  uint pos=0;
  std::pair<std::shared_ptr<APE::BufferContainer>,bool> b;
  int percentMark=(double)nevents/100.;
  if (percentMark<1){
    percentMark=1;
  }
  int percent=0;
  ssize_t bRecv=0;
  auto pid=getpid();
  for(uint i=0;i<nevents;i++){
    auto& e=events.at(pos);
    b=e.next();
    while(b.first){
      ssocket->send(APE::BufferAccessor::getRawBuffer(*(b.first)),b.first->getTransferSize());      
      if(b.second){
	ssize_t rv=rsocket->tryRecv(recvBuff,1<<25,-1l);
	if(rv<20){
	  std::cerr<<"Warning didn't receive any data. rv="<<rv<<std::endl;
	}
	bRecv+=rv;
	//std::cout<<"Received "<<rsocket->tryRecv(recvBuff,1<<25,-1l)<<" bytes"<<std::endl;
      }
      b=e.next();
    }
    pos++;
    if(pos==events.size())pos=0;
    if(i%percentMark==0){
      std::cout<<percent<<"% complete "<<bRecv<<" bytes received. pid="<<pid<<std::endl;
      percent++;
    }
  }
  delete[] recvBuff;
}

void doInit(std::vector<Event>& events,
	    yampl::ISocket* ssocket,
	    yampl::ISocket* rsocket){
  char * recvBuff=new char[1<<25];
  std::pair<std::shared_ptr<APE::BufferContainer>,bool> b;
  uint nevents=events.size();
  uint pos=0;
  for(uint i=0;i<nevents;i++){
    auto& e=events.at(pos);
    b=e.next();
    while(b.first){
      std::cout<<"Sending Evt "<<e.getEventId()
	       <<" Module "<<b.first->getModule()
	       <<" alg "<<b.first->getAlgorithm()
	       <<std::endl;
      ssocket->send(APE::BufferAccessor::getRawBuffer(*(b.first)),b.first->getTransferSize());      
      if(b.second){
	std::cout<<"waiting for response "<<e.getEventId()
		 <<" Module "<<b.first->getModule()
		 <<" alg "<<b.first->getAlgorithm()
		 <<std::endl;
	rsocket->tryRecv(recvBuff,1<<25,-1l);
      }
      b=e.next();
    }
    pos++;
    if(pos==events.size())pos=0;
  }
  delete[] recvBuff;
}

bool closeComm(yampl::ISocketFactory* &factory,
	       yampl::ISocket *&ssocket,
	       yampl::ISocket *&rsocket,
	       bool prefork){
  auto connRequest=new APE::BufferContainer(sizeof(int));
  int pid=(int)getpid();
  *(int*)(connRequest->getBuffer())=pid;
  connRequest->setModule(SERVER_MODULE);
  if(prefork){
    connRequest->setAlgorithm(ALG_CLOSING_FOR_FORKING);
  }else{
    connRequest->setAlgorithm(ALG_CLOSING_CONNECTION);
  }
  ssocket->send(APE::BufferAccessor::getRawBuffer(*connRequest), connRequest->getTransferSize());
  delete ssocket;
  ssocket=0;
  delete rsocket;
  rsocket=0;
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  delete factory;
  factory=0;
  delete connRequest;
  return true;

}

bool openComm(yampl::ISocketFactory* &factory,
	      yampl::ISocket *&ssocket,
	      yampl::ISocket *&rsocket,
	      std::shared_ptr<APEConfig::ConfigTree> SC,bool postFork){
  if(factory==0){
    factory=new yampl::SocketFactory();
  }
  std::string commType=SC->getValue<std::string>("CommType");
  std::string commChUp=SC->getValue<>("CommChannelUp");
  std::string commChDown=SC->getValue<>("CommChannelDown");
  bool useUID=SC->getValue<bool>("CommChannelUseUID");
  char sockBuff[1000];
  uid_t userUID=geteuid();

  auto cType=yampl::LOCAL_SHM;
  if(commType=="PIPE")cType=yampl::LOCAL_PIPE;
  if(commType=="ZMQ")cType=yampl::DISTRIBUTED;
  if(useUID){
    snprintf(sockBuff,1000,"%s_%u",commChDown.c_str(),userUID);
  }else{
    snprintf(sockBuff,1000,"%s",commChDown.c_str());      
  }
  yampl::Channel channelS(sockBuff,cType);
  if(useUID){
    snprintf(sockBuff,1000,"%s_%u",commChUp.c_str(),userUID);
  }else{
    snprintf(sockBuff,1000,"%s",commChUp.c_str());      
  }
  yampl::Channel channelR(sockBuff,cType);
  uuid_t uuid;
  uuid_clear(uuid);
  char uuidbuff[40];
  uuid_generate(uuid);
  uuid_unparse(uuid,uuidbuff);
  uuidbuff[36]='\0';
  std::string uuids(uuidbuff);
  std::cerr<<"UUID= "<<uuids<<std::flush<<std::endl;
  ssocket= factory->createClientSocket(channelS,yampl::COPY_DATA,nullDealloc,uuids);
  uuid_clear(uuid);
  uuid_generate(uuid);
  uuid_unparse(uuid,uuidbuff);
  rsocket= factory->createClientSocket(channelR,yampl::COPY_DATA,nullDealloc,std::string(uuidbuff));
  auto connRequest=new APE::BufferContainer(sizeof(int));
  int pid=(int)getpid();
  connRequest->setModule(SERVER_MODULE);
  if(postFork){
    connRequest->setAlgorithm(ALG_OPENING_AFTER_FORKING);
  }else{
    connRequest->setAlgorithm(ALG_OPENING_CONNECTION);

  }
  *(int*)(connRequest->getBuffer())=pid;
  ssocket->send(APE::BufferAccessor::getRawBuffer(*connRequest),connRequest->getTransferSize());
  rsocket->send(APE::BufferAccessor::getRawBuffer(*connRequest),connRequest->getTransferSize());
  return true;
}

int main(int argc,char *argv[]){
  std::string configName("UtilClient.yaml");
  std::string dataDir("clientData");
  unsigned int nEvents=0;
  std::set<int> enabledModules;//{ 0x10100000,51,71};
  char c;
  uint nforks=0;
  bool waitforChildren=false;
  std::vector<pid_t> children;
  while (1) {
    long val=0;
    int option_index = 0;
    static struct option long_options[] = {
      {"help", 0, 0, 'h'},
      {"numEvents", 1, 0, 'n'},
      {"numForks", 1, 0, 'f'},
      {"config-name",1,0,'c'},
      {0, 0, 0, 0}
    };
    c = getopt_long(argc, argv, "hn:c:f:",
		    long_options, &option_index);
    if (c == -1)
      break;
    switch (c) {
    case 'h':
      printUsage(argv[0]);
      exit(EXIT_SUCCESS);
      break;
    case 'n':{
      nEvents=atoi(optarg);
      break;
    }
    case 'f':{
      nforks=std::strtol(optarg,0,10);
      break;
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
  std::string serverRName;
  std::string serverSName;
  std::vector<InitFile> initFiles;
  if(config){
    //auto SC=config->getSubtree("Server");
    auto CC=config->getSubtree("Client");
    if(nEvents==0){
      nEvents=CC->getValue<unsigned int>("NumEvents");
    }
    for(auto p:CC->getSubtree("EnabledModules")->findParameters("param")){
      std::cout<<"Enabled module  "<<p->getValue<int>()<<std::endl;
      enabledModules.insert(p->getValue<int>());
    }
    dataDir=CC->getValue<>("DataDir");
    auto InC=CC->getSubtree("InitConfig");
    CC->print(std::cout);
    for(auto inF:InC->findSubtrees("InitFiles")){
      InitFile fs({inF->getValue<uint64_t>("Event"),inF->getValue<int>("Module"),inF->getValue<int>("Alg")});
      std::cout<<"Initialization file Event= "<<fs.ev
	       <<" module= "<<fs.mod<<" alg= "<<fs.alg<<std::endl;
      initFiles.push_back(fs);
    }
  }else{
    std::cerr<<"Couldn't find config file "<<configName<<std::endl;
    exit(EXIT_FAILURE);
  }
  children.reserve(nforks);
  std::cout<<"Starting loading files"<<std::endl;
  std::vector<Event> initializationEvents;
  auto events=loadFiles(dataDir,enabledModules,initializationEvents,initFiles);
  std::cout<<"Found "<<events.size()<<" Events."<<std::endl;
  std::cout<<"Initiating connection to server"<<std::endl;
  openComm(factory,ssocket,rsocket,config->getSubtree("Server"),false);
  std::cout<<"Sent Connection requests. Starting transfers"<<std::endl;
  std::cout<<"Doing initialization requests"<<std::endl;
  doInit(initializationEvents,ssocket,rsocket);
  bool iAmChildren=false;
  int myPos=0;
  if(nforks>0){
    std::cout<<"Forking "<< nforks <<" child clients"<<std::endl;
    waitforChildren=true;
    closeComm(factory,ssocket,rsocket,true);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));  
    for(uint i=0;i<nforks;i++){
      myPos++;
      pid_t t=fork();
      if(t==0){
	iAmChildren=true;
	break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(500));        
      children.push_back(t);
    }
    openComm(factory,ssocket,rsocket,config->getSubtree("Server"),true);
  }
  auto pid=getpid();
  auto tFirst=std::chrono::high_resolution_clock::now();
  std::cout<<"Starting loop for "<<nEvents<<std::endl;
  doLoop(events,nEvents,ssocket,rsocket);
  std::chrono::high_resolution_clock::time_point tEnd=std::chrono::high_resolution_clock::now();
  std::cout<<"DataTransfer finished closing connection "<<pid<<" pos="<<myPos<<std::endl;
  // connRequest->setModule(SERVER_MODULE);
  // connRequest->setAlgorithm(ALG_CLOSING_CONNECTION);
  // ssocket->send(APE::BufferAccessor::getRawBuffer(*connRequest), connRequest->getTransferSize());
  closeComm(factory,ssocket,rsocket,false);
  std::this_thread::sleep_for(std::chrono::milliseconds(10000));  
  std::cout<<"Connection closed"<<std::endl;
  // delete connRequest;
  // delete rsocket;
  // delete ssocket;
  std::cout<<"Total time ="<<std::chrono::duration_cast<std::chrono::milliseconds>(tEnd-tFirst).count()/1000.0<<" (s) "<<pid<<std::endl;
  if(waitforChildren&&!iAmChildren){
    auto it=children.begin();
    int status=0;
    int count=0;
    auto positions=children;
    while(children.size()){
      auto p=waitpid(-1,&status,WNOHANG);
      if(p==0){
	std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	count++;
	if(count%10==0){
	  std::cout<<"Sleeping.. "<<children.size()<<" children remains"<<pid<<std::endl;
	}
      }else{
	if(p>0){
	  int cpos=0;
	  for(size_t t=0;t<positions.size();t++){
	    if (positions[t]==p){
	      cpos=t;
	      break;
	    }
	  }
	  for(it=children.begin();it!=children.end();it++){
	    if(*it==p){
	      children.erase(it);
	      std::cout<<"child pid="<<p<<" at "<<cpos<<" exited"<<std::endl;
	      break;
	    }
	  }
	}
      }
    }
    std::chrono::high_resolution_clock::time_point tEnd=std::chrono::high_resolution_clock::now();
    std::cout<<"All children exited. Total run time="<< std::chrono::duration_cast<std::chrono::milliseconds>(tEnd-tFirst).count()/1000.0<<"(s) Sleeping for 5s "<<std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
  }
  delete factory;

  return EXIT_SUCCESS;
}
