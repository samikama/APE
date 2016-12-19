#include "APE/Server.hpp"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <iostream>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <algorithm>
#include <string>
#include "yampl/Exceptions.h"
#include "APE/BufferContainer.hpp"
#include "APE/BufferAccessor.hpp"
#include "APE/ServerDefines.hpp"
#include "APE/Module.hpp"
#include "APE/Work.hpp"
#include "APE/ConfigTree.hpp"
#include "APE/Log.hpp"
#include "APE/APEHist.hpp"

APE::Server::Server(std::shared_ptr<APEConfig::ConfigTree> config):m_channelName("apeSock"),m_myConfig(config){
  m_nClients=0;
  m_senderThread=0;
  m_enqueued=0;
  m_stopSender=true;
  m_maxTokens=0;
  //m_info.resize(5000);
  m_dumpQueue=false;
  m_senderSleeping=true;
  m_handledRequests=0;
  m_totalData=0;
  m_runnerSleeping=true;
  m_toRun=0;
  m_nSenders=1;
  m_nRunners=8;
  m_nReceivers=1;
  m_keepConnOpen=false;
  m_log=std::make_shared<APE::Log>("APE",APE::Log::INFO,std::cout);
  if(config)configure(config);
}

APE::Server::Server(const std::string& moduleLib,const std::string& channelName):m_channelName(channelName) {
  m_nClients=0;
  m_senderThread=0;
  m_enqueued=0;
  m_stopSender=true;
  m_maxTokens=0;
  m_info.resize(5000);
  m_dumpQueue=false;
  m_senderSleeping=true;
  m_handledRequests=0;
  m_totalData=0;
  m_moduleLibs.push_back(moduleLib);
  m_runnerSleeping=true;
  m_toRun=0;
  m_nSenders=1;
  m_nRunners=8;
  m_nReceivers=1;
  m_configured=false;
}

APE::Server::~Server(){
  
}

bool APE::Server::openSocket(const std::string &channelType, 
			     const std::string &commChannelUp,
			     const std::string &commChannelDown,
			     bool useUID){
  auto chanType=yampl::LOCAL_SHM;
  if(channelType=="PIPE"){
    chanType=yampl::LOCAL_PIPE;
  }else if(channelType=="ZMQ"){
    chanType=yampl::DISTRIBUTED;    
  }
  uid_t userUID=geteuid();
  if(commChannelUp.empty())return false;
  if(commChannelDown.empty())return false;
  try{
    char buff[1000];
    if(chanType!=yampl::DISTRIBUTED && useUID){
      snprintf(buff,1000,"%s_%u",commChannelDown.c_str(),userUID);
    }else{
      snprintf(buff,1000,"%s",commChannelDown.c_str());
    }
    yampl::Channel channelS(buff,chanType);
    *m_log<<APE::Log::VERBOSE<<"Using Receive/Download channel "<<buff<<std::endl;
    if(chanType!=yampl::DISTRIBUTED && useUID){
      snprintf(buff,1000,"%s_%u",commChannelUp.c_str(),userUID);
    }else{
      snprintf(buff,1000,"%s",commChannelUp.c_str());
    }
    *m_log<<APE::Log::VERBOSE<<"Using Send/Upload channel "<<buff<<std::endl;
    yampl::Channel channelU(buff,chanType);
    auto factory=new yampl::SocketFactory();
    m_socket.reset(factory->createServerSocket(channelS));
    m_sockUpload.reset(factory->createServerSocket(channelU));
    *m_log<<APE::Log::VERBOSE<<"Sockets are created "<<buff<<std::endl;    
    if (!m_socket){
      *m_log<<APE::Log::FATAL<<"Failed to open download channel"<<std::endl;      
    }
    if (!m_sockUpload){
      *m_log<<APE::Log::FATAL<<"Failed to open upload channel"<<std::endl;      
    }
  }catch(std::exception &ex){
    *m_log<<APE::Log::FATAL<<"Caught exception while trying to open server socket. Exception was \""<<ex.what()<<"\""<<std::endl;
    return false;
  }
  return true;
}

bool APE::Server::addModule(const std::string& modName){
  if(modName.empty()){
    return false;
  }
  if(std::find(m_moduleLibs.begin(),m_moduleLibs.end(),modName)!=m_moduleLibs.end()){
    return false;
  }
  m_moduleLibs.push_back(modName);
}

std::shared_ptr<APE::Module> APE::Server::loadModule(const std::string &libName){
  if(libName.empty()){
    return 0;
  }
  // m_moduleLibs.push_back(libName);
  typedef APE::Module* (*modFactory)(void);
  typedef void (*modGarbage)(APE::Module*);
  auto oldModule=m_libIdMap.find(libName);
  if(oldModule!=m_libIdMap.end()){
    std::cerr<<" Warning tried to load an already loaded module! Check configuration! Returning previously loaded module "<<std::endl; 
    return m_moduleMap[oldModule->second];
  }
  void *libHandle=dlopen(libName.c_str(),RTLD_LAZY|RTLD_LOCAL);
  if(!libHandle){
    const char* errmsg=dlerror();
    if(errmsg){
      std::cerr<<"Got error when opening library "<<libName<<" \""<<errmsg<<"\""<<std::endl;
    }else{
      std::cerr<<"Got UNKNOWN error when opening library "<<libName<<std::endl;
    }
    return 0;
  }
  m_libHandles[libName]=libHandle;
  modFactory fserial=(modFactory)dlsym(libHandle,"getModule");
  const char* dlsymError=dlerror();
  if(dlsymError){
    std::cerr<<"Can't get module factory function from library "<<libName<<std::endl;
    
    return 0;
  }
  modGarbage mgarb=(modGarbage)dlsym(libHandle,"deleteModule");
  dlsymError=dlerror();
  if(dlsymError){
    std::cerr<<"Can't get module deleter function from library "<<libName<<std::endl;
    return 0;
  }
  std::shared_ptr<APE::Module> modl(fserial(),mgarb);
  char buff[1000];
  snprintf(buff,1000,"Loaded module with id=0x%lx from %s",modl->getModuleId(),libName.c_str());
  *m_log<<APE::Log::INFO<<buff<<APE::Log::endl;
  m_moduleMap[modl->getModuleId()]=modl;
  m_libIdMap[libName]=modl->getModuleId();
  return modl;
}

bool APE::Server::loadModules(){
  typedef APE::Module* (*modFactory)(void);
  typedef void (*modGarbage)(APE::Module*);
  for(auto& module:m_moduleLibs){
    loadModule(module);
  }
  if(m_moduleMap.size()){
    return true;
  }
  return false;
}

std::shared_ptr<APE::BufferContainer> APE::Server::receiveData(){
  char *buff=0;
  std::string source("Sami");
  if(m_socket){
    //std::cout<<"Trying to receive"<<std::endl;
    auto buffC=std::shared_ptr<APE::BufferContainer>(0);
    ssize_t lenBytes=0;
    try{
      lenBytes=m_socket->tryRecv(buff,-1l,source);
    }catch(std::exception &ex){
      *m_log<<APE::Log::WARNING<<"Timeout "<<ex.what()<<APE::Log::endl;
      return buffC;
    }
    //std::cout<<"Received data from "<<source <<std::endl;
    //*m_log<<APE::Log::VERBOSE<<"Got data "<<lenBytes<<" from "<<source<<std::endl;
    auto it=m_clients.find(source);
    if(lenBytes==0){
      free(buff);
      return std::shared_ptr<APE::BufferContainer>(0);
    }
    buffC=std::make_shared<APE::BufferContainer>((void*)buff,lenBytes);

    if(it==m_clients.end()){
      //if(verbosity>0)
      *m_log<<APE::Log::INFO<<"New client connection from \""<<source<<"\""<<APE::Log::endl;
      it=m_clients.insert(std::make_pair(source,m_nClients)).first;
      if(m_nClients==0)m_tStart=std::chrono::high_resolution_clock::now();
      m_nClients++;
    }
    if(buffC->getModule()==SERVER_MODULE){
      if(buffC->getAlgorithm()==ALG_CLOSING_CONNECTION){
	//m_clients.erase(it);
	m_nClients--;
	m_sockMap[source]="";

	*m_log<<APE::Log::INFO<<"Closing Connection \""<<source<<"\" nclients="<<m_nClients<<" m_clientsSize="<<m_clients.size()<<APE::Log::endl;
	free(buff);
	auto it=m_recvMap.end();
	for(auto i=m_recvMap.begin();i!=m_recvMap.end();++i){
	  if (i->second==source){
	    it=i;
	    break;
	  }
	}
	if(it!=m_recvMap.end())m_recvMap.erase(it);
	return std::shared_ptr<APE::BufferContainer>(0);
      }else if(buffC->getAlgorithm()==ALG_OPENING_CONNECTION){
	int rpid=*(int*)buffC->getBuffer();//get pid
	auto itrec=m_recvMap.find(rpid);
	*m_log<<APE::Log::INFO<<"Opening receive connection  \""<<source<<"\""<<APE::Log::endl;
	if(itrec==m_recvMap.end()){
	  itrec=m_recvMap.insert(std::make_pair(rpid,source)).first;
	}
	char * buff2=0;
	std::string sendSource;
	try{
	  std::unique_lock<std::mutex> lock(m_sendMutex);
	  lenBytes=m_sockUpload->tryRecv(buff2,-1,sendSource);
	}catch(std::exception &ex){
	  *m_log<<APE::Log::INFO<<"Timeout in openning send connection"<<APE::Log::endl;
	}
	*m_log<<APE::Log::INFO<<"Opening send connection  \""<<sendSource<<"\""<<APE::Log::endl;
	auto buffC2=std::make_shared<APE::BufferContainer>((void*)buff2,lenBytes);
	int spid=*(int*)buffC2->getBuffer();
	auto itsend=m_sendMap.find(spid);
	if(itsend==m_sendMap.end()){
	  itsend=m_sendMap.insert(std::make_pair(spid,sendSource)).first;
	}
	if(spid==rpid){
	  m_sockMap.insert(std::make_pair(source,sendSource));
	}else{
	  auto rsit=m_sendMap.find(rpid);
	  if(rsit!=m_sendMap.end()){
	    m_sockMap.insert(std::make_pair(source,rsit->second));  
	  }
	  rsit=m_recvMap.find(spid);
	  if(rsit!=m_recvMap.end()){
	    m_sockMap.insert(std::make_pair(rsit->second,sendSource));  
	  }
	}
	free(buff);
	free(buff2);
	return std::shared_ptr<APE::BufferContainer>(0);
      }else if(buffC->getAlgorithm()==ALG_CLOSING_FOR_FORKING){
	m_keepConnOpen=true;
	m_nClients--;
	m_sockMap[source]="";
	*m_log<<APE::Log::INFO<<"Closing Connection from \""<<source
		 <<"\" for reopenning after fork nclients="
		 <<m_nClients<<" m_clientsSize="
		 <<m_clients.size()<<APE::Log::endl;
	free(buff);
	auto it=m_recvMap.end();
	for(auto i=m_recvMap.begin();i!=m_recvMap.end();++i){
	  if (i->second==source){
	    it=i;
	    break;
	  }
	}
	if(it!=m_recvMap.end())m_recvMap.erase(it);
	return std::shared_ptr<APE::BufferContainer>(0);

      }else if(buffC->getAlgorithm()==ALG_OPENING_AFTER_FORKING){
	m_keepConnOpen=false;
	int rpid=*(int*)buffC->getBuffer();
	auto itrec=m_recvMap.find(rpid);
	*m_log<<APE::Log::INFO<<"Opening receive connection from  \""<<source<<"\""<<APE::Log::endl;
	if(itrec==m_recvMap.end()){
	  itrec=m_recvMap.insert(std::make_pair(rpid,source)).first;
	}
	char * buff2=0;
	std::string sendSource;
	try{
	  std::unique_lock<std::mutex> lock(m_sendMutex);
	  lenBytes=m_sockUpload->tryRecv(buff2,-1,sendSource);
	}catch(std::exception &ex){
	  *m_log<<APE::Log::INFO<<"Timeout in openning send connection"<<APE::Log::endl;
	}
	*m_log<<APE::Log::INFO<<"Opening send connection  \""<<sendSource<<"\""<<APE::Log::endl;
	auto buffC2=std::make_shared<APE::BufferContainer>((void*)buff2,lenBytes);
	int spid=*(int*)buffC2->getBuffer();
	auto itsend=m_sendMap.find(spid);
	if(itsend==m_sendMap.end()){
	  itsend=m_sendMap.insert(std::make_pair(spid,sendSource)).first;
	}

	if(spid==rpid){
	  m_sockMap.insert(std::make_pair(source,sendSource));
	}else{
	  auto rsit=m_sendMap.find(rpid);
	  if(rsit!=m_sendMap.end()){
	    m_sockMap.insert(std::make_pair(source,rsit->second));  
	  }
	  rsit=m_recvMap.find(spid);
	  if(rsit!=m_recvMap.end()){
	    m_sockMap.insert(std::make_pair(rsit->second,sendSource));  
	  }
	}
	free(buff);
	free(buff2);
	return std::shared_ptr<APE::BufferContainer>(0);
      }else{
	char buff[36];
	snprintf(buff,36,"0X%8x",buffC->getAlgorithm());
	std::cerr<<"Received unknown request from Client "<<buff<<std::endl; 
      }
    }else{
      m_totalData+=lenBytes;
      int newToken=0;
      if(!m_tokenQueue.try_pop(newToken)){
	newToken=m_maxTokens;
	m_maxTokens++;
      }
      if(newToken<m_info.size()){
	auto& wi=m_info.at(newToken);
	wi.origToken=buffC->getToken();
	wi.inputBuff=buff;
	wi.client=&m_sockMap[source];
	APE::BufferAccessor::setToken(*buffC,newToken);
	// std::cout<<"New job from "<<source<<" assigned to token "<<newToken<<" was "
	// 	 <<wi.origToken<<" maxToken= "<<m_maxTokens<<std::endl;

      }else{
	std::cerr<<" Tokens exceeded available slots"<<std::endl;
	free(buff);
	return std::shared_ptr<APE::BufferContainer>(0);
      }
      return buffC;
    }
  }
}

void APE::Server::executeWorks(int id,std::shared_ptr<APE::Log> log){
  *log<<APE::Log::INFO<<"Runner thread "<<id<<" started "<<APE::Log::endl;
  char buff[16];
  snprintf(buff,100,"Thread %02d ",id);
  std::string thrName(buff);
  auto activeTime=dynamic_cast<APEUtils::Hist2D<float>*>(m_histMap["activeTime"]);
  auto sleepTime =dynamic_cast<APEUtils::Hist2D<float>*>(m_histMap["sleepTime"]);
  auto queueDepth=dynamic_cast<APEUtils::Hist2D<float>*>(m_histMap["queueDepth"]);
  // activeTime->setXtitle("time (ms)");
  // sleepTime->setXtitle("time (ms)");
  // queueDepth->setXtitle("Queue depth");
  // activeTime->setYtitle("Counts");
  // sleepTime->setYtitle("Counts");
  // queueDepth->setYtitle("Counts");
  // {
  //   std::unique_lock<std::mutex> lock(m_threadHistMtx);
  //   m_threadHists.push_back(activeTime);
  //   m_threadHists.push_back(sleepTime);
  //   m_threadHists.push_back(queueDepth);
  // }

  auto tStart=std::chrono::steady_clock::now();
  std::chrono::milliseconds tWorking(0);
  auto tlast=std::chrono::steady_clock::now();
  double tid(id);
  while(!m_stopRunner){
    m_runnerSleeping=false;
    if(m_toRun>0){
      for(auto &m:m_moduleMap){
	auto work=m.second->getWorkToRun();
	if(work){//spinlock
	  queueDepth->fill(m_toRun,tid);
	  auto tws=std::chrono::steady_clock::now();
	  sleepTime->fill(std::chrono::duration_cast<std::chrono::milliseconds>(tws-tlast).count(),tid);
	  if(work->run()){
	    m_enqueued++;
	    if(m_senderSleeping){
	      std::unique_lock<std::mutex> lock(m_sendMutex);
	      m_sendCond.notify_all();
	    }
	  }else{//release the token
	    auto request=work->getOutput();
	    auto token=request->getToken();
	    auto& wi=m_info.at(token);
	    free(wi.inputBuff);
	    wi.inputBuff=0;
	    m_tokenQueue.push(token);
	    delete work;
	    m_handledRequests++;
	  }
	  m_toRun--;
	  tlast=std::chrono::steady_clock::now();
	  auto tdelta=std::chrono::duration_cast<std::chrono::milliseconds>(tlast-tws);
	  tWorking+=tdelta;
	  activeTime->fill(tdelta.count(),tid);
	}
      }
    }else{
      std::unique_lock<std::mutex> lock(m_runnerMutex);
      m_runnerSleeping=true;
      m_runnerCond.wait_for(lock,std::chrono::milliseconds(200));
    }
  }
  auto tend=std::chrono::steady_clock::now();
  *log<<APE::Log::INFO<<"Runner thread "<<id<<" is stopping "<<m_toRun<<" works to run left"<<APE::Log::endl;
  auto tTotal=std::chrono::duration_cast<std::chrono::milliseconds>(tend-tStart);
  if(tTotal.count()>0){
    //std::this_thread::sleep_for(std::chrono::milliseconds(500*id));
    std::stringstream oss;
    oss<<"Runner thread "<<id<<" was running for "<<tTotal.count()/1000.
       <<" seconds and was active for "<< (tWorking.count()*100.)/((double)tTotal.count())
       <<"% of the time"<<std::endl;
    *log<<APE::Log::INFO<<oss.str();
  }
  
}

void APE::Server::sendResult(std::shared_ptr<APE::Log> log){
  *log<<APE::Log::INFO<<"Sender thread started "<<APE::Log::endl;
  auto tStart=std::chrono::steady_clock::now();
  std::chrono::milliseconds tWorking(0);
  while(!m_stopSender){
    m_senderSleeping=false;
    if(m_enqueued>0){
      for(auto &m:m_moduleMap){
	auto work=m.second->getResult();
	if(work){//spinlock
	  auto tws=std::chrono::steady_clock::now();
	  auto buffC=work->getOutput();
	  int token=buffC->getToken();
	  auto& wi=m_info.at(token);
	  APE::BufferAccessor::setToken(*buffC,wi.origToken);
	  // std::cout<<"Trying to send to "<<*wi.client<<" token="<<token<<" OrigToken="
	  // 	   <<wi.origToken<<" q="<<m_enqueued<<" r="<<m_toRun<<std::endl;
	  size_t trSize=buffC->getTransferSize();
	  m_totalData+=trSize;
	  try{
	    std::unique_lock<std::mutex> lock(m_sendMutex);
	    m_sockUpload->sendTo(*wi.client,APE::BufferAccessor::getRawBuffer(*buffC),
				 trSize,0);
	  }catch(yampl::UnroutableException &ex){
	    *log<<APE::Log::INFO<<"client "<<wi.client<<" disappeared token was "<<token<<APE::Log::endl;
	  }
	  free(wi.inputBuff);
	  wi.inputBuff=0;
	  m_tokenQueue.push(token);
	  m_enqueued--;
	  m_handledRequests++;
	  delete work;
	  tWorking+=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-tws);
	}
      }
    }else{
      //std::cout<<" Sender is sleeping "<<std::endl;      
      std::unique_lock<std::mutex> lock(m_sendMutex);
      m_senderSleeping=true;
      m_sendCond.wait_for(lock,std::chrono::milliseconds(200));
    }
  }
  auto tend=std::chrono::steady_clock::now();
  *log<<APE::Log::INFO<<"Sender thread stopping "<<m_enqueued<<" results left"<<APE::Log::endl;
  auto tTotal=std::chrono::duration_cast<std::chrono::milliseconds>(tend-tStart);
  if(tTotal.count()>0){
    std::stringstream oss;
    oss<<"Sender thread was running for "<<tTotal.count()/1000.
       <<" seconds and was active for "<< (tWorking.count()*100.)/((double)tTotal.count())
       <<"% of the time"<<std::endl;
    *log<<APE::Log::INFO<<oss.str();
  }
    
}

int APE::Server::run(){
  if(!m_configured){
    if(!loadModules()){
      std::cerr<<"Loading module failed. Exiting"<<std::endl;
      return EXIT_FAILURE;
    }
    if(!openSocket()){
      std::cerr<<"Openning server socket failed. Exiting!"<<std::endl;
      return EXIT_FAILURE;
    }
  }
  m_stopSender=false;
  m_logs.push_back(new std::string("Sender Log"));
  
  //m_senderThread=new std::thread(&APE::Server::sendResult,this,m_logs.back());
  m_senderThread=new std::thread(&APE::Server::sendResult,this,m_log);
  m_stopRunner=false;
  
  auto activeTime=new APEUtils::Hist2D<float>("Active Time",500,0.,5000.,m_nRunners,0.,m_nRunners);
  m_histMap["activeTime"]=activeTime;
  activeTime->setXtitle("time (ms)");
  activeTime->setYtitle("Thread ID");
  auto sleepTime=new APEUtils::Hist2D<float>("Sleep Time",1000,0.,10000.,m_nRunners,0.,m_nRunners);
  m_histMap["sleepTime"]=sleepTime;
  sleepTime->setXtitle("time (ms)");
  sleepTime->setYtitle("Thread ID");
  auto queueDepth=new APEUtils::Hist2D<float>("Queue Depth at execute",100,0.,100.,m_nRunners,0.,m_nRunners);
  m_histMap["queueDepth"]=queueDepth;
  queueDepth->setXtitle("Number of items in the queue");
  queueDepth->setYtitle("Thread ID");
  for(int i=0;i<m_nRunners;i++){
    m_logs.push_back(new std::string("Runner Log"));
    //m_runnerThreads.push_back(new std::thread(&APE::Server::executeWorks,this,i,m_logs.back()));  
    m_runnerThreads.push_back(new std::thread(&APE::Server::executeWorks,this,i,m_log));  
  }
  auto mmend=m_moduleMap.end();
  while(true){
    auto request=receiveData();
    if(request){
      auto mit=m_moduleMap.find(request->getModule());
      if(mit!=mmend){
	if(mit->second->addWork(request)){
	  m_toRun++;
	  if(m_runnerSleeping){
	    std::unique_lock<std::mutex> lock(m_runnerMutex);
	    m_runnerCond.notify_all();
	  }
	}else{//release work token
	  auto token=request->getToken();
	  auto& wi=m_info.at(token);
	  free(wi.inputBuff);
	  wi.inputBuff=0;
	  m_tokenQueue.push(token);
	}
      }else{
	std::cerr<<"Warning asked for module "
		 <<request->getModule()
		 <<" but no such module exists!"<<std::endl;
      }
    }else{
      if(m_nClients==0 && m_clients.size()>0 && !m_keepConnOpen ){// no more clients left
	*m_log<<APE::Log::INFO<<"No More clients left "<<m_enqueued<<" results to send"<<APE::Log::endl;
	m_dumpQueue=false;
	while(m_enqueued>0 && m_toRun>0){// wait until existing jobs complete (this should never happen)
	  std::this_thread::sleep_for(std::chrono::seconds(1));
	  if((m_enqueued%10)==0)*m_log<<APE::Log::INFO<<"Remaining "<<m_enqueued<<" results to send"<<APE::Log::endl;
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	{
	  m_stopRunner=true;
	  std::unique_lock<std::mutex> lockr(m_runnerMutex);
	  m_runnerCond.notify_all();
	  m_stopSender=true;
	  *m_log<<APE::Log::INFO<<"No More results to send"<<APE::Log::endl;	
	  std::unique_lock<std::mutex> lock(m_sendMutex);
	  m_sendCond.notify_all();
	}
	for(auto &it:m_runnerThreads){
	  it->detach();
	}
	m_senderThread->detach();
	*m_log<<APE::Log::INFO<<"Max queue was "<<m_maxTokens<<" items"<<APE::Log::endl;
	
	break;
      }else if(m_maxTokens>=m_info.size()){//Token Buffer Exceeded
	*m_log<<APE::Log::WARNING<<"APE Token Buffer is exceeded."<<APE::Log::endl;
      }
    }
  }
  auto tEnd=std::chrono::high_resolution_clock::now();
  auto diff=std::chrono::duration_cast<std::chrono::milliseconds>(tEnd-m_tStart).count();
  std::stringstream oss;

  *m_log<<APE::Log::INFO<<"Completed "<<m_handledRequests<<" requests."<<std::endl
	<<"Total time from first connection to last connection is "
	<<diff/1000.<<" seconds."<<std::endl
	<<"Avg time for completing a request is "<<diff/(double)m_handledRequests<<" (ms)"<<std::endl
	<<"Total data transferred "<<m_totalData/1024<<" kB. Avg data per request is "
	<<m_totalData/(m_handledRequests*1024)<<" kB."
	<<std::endl;
  *m_log<<APE::Log::INFO<<"Logs are"<<APE::Log::endl;
  std::this_thread::sleep_for(std::chrono::seconds(2));
  //*m_log<<APE::Log::INFO<<oss.str()<<APE::Log::endl;
  // for(auto &it:m_logs){
  //   std::cout<<*it<<std::endl;
  // }
  *m_log<<APE::Log::INFO<<"APE Histograms"<<APE::Log::endl;
  *m_log<<APE::Log::INFO<<"--- Module "<<SERVER_MODULE<<" "<<"APE"<<APE::Log::endl;
  for(auto &i:m_histMap){
    *m_log<<APE::Log::INFO<<*(i.second)<<std::endl;
  }
  *m_log<<APE::Log::INFO<<"-------- -- -- -- -- -- -- -- -- -- -------"<<APE::Log::endl;
  *m_log<<APE::Log::INFO<<"Module stats"<<APE::Log::endl;
  for(auto &i:m_moduleMap){
    oss.str("");
    *m_log<<APE::Log::INFO<<"--- Module "<<i.first<<" "<<m_modIdNameMap[i.first]<<APE::Log::endl;
    i.second->printStats(oss);
    *m_log<<APE::Log::INFO<<oss.str()<<std::endl;
    *m_log<<APE::Log::INFO<<"-------- -- -- -- -- -- -- -- -- -- -------"<<APE::Log::endl;
  }
  return EXIT_SUCCESS;
}

void APE::Server::printStats(std::ostream& out){

}

bool APE::Server::configure(std::shared_ptr<APEConfig::ConfigTree> &c){
  if(!c){
    throw std::runtime_error("Invalid configuration!");
  }
  std::map<std::string,APE::Log::Level> llmap={{"VERBOSE",APE::Log::VERBOSE},
					       {"DEBUG",APE::Log::DEBUG},
					       {"INFO",APE::Log::INFO},
					       {"WARNING",APE::Log::WARNING},
					       {"ERROR",APE::Log::ERROR},
					       {"FATAL",APE::Log::FATAL}};
  m_myConfig=c;
  auto SC=c->getSubtree("Server");
  try{
    std::string logLevel=SC->getValue<>("LogLevel");
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
  unsigned int numTokens=SC->getValue<unsigned int>("MaxWorkTokens");
  *m_log<<APE::Log::VERBOSE<<"Max Work Tokens= \""<<numTokens<<"\""<<std::endl;
  m_info.resize(numTokens);
  m_nReceivers=SC->getValue<unsigned int>("NumReceivers");
  *m_log<<APE::Log::VERBOSE<<"Num Receivers= \""<<m_nReceivers<<"\""<<std::endl;
  m_nSenders=SC->getValue<unsigned int>("NumSenders");
  *m_log<<APE::Log::VERBOSE<<"Num Senders= \""<<m_nSenders<<"\""<<std::endl;  
  m_nRunners=SC->getValue<unsigned int>("NumRunners");
  *m_log<<APE::Log::VERBOSE<<"Num Runners= \""<<m_nRunners<<"\""<<std::endl;
  bool hasModule=false;
  for(auto &ct : c->findSubtrees("Module")){
    hasModule=true;
    std::string modLib=ct->getValue<>("Library");
    *m_log<<APE::Log::VERBOSE<<"Loading library= \""<<modLib<<"\""<<std::endl;
    auto module=loadModule(modLib);
    if(module){
      *m_log<<APE::Log::VERBOSE<<"Module \""<<ct->getValue<>("Name")<<"\" loaded successfuly from "<<modLib<<std::endl;
      m_modIdNameMap[module->getModuleId()]=ct->getValue<>("Name");
      if(!module->configure(ct)){
	*m_log<<APE::Log::ERROR<<"Module "<<ct->getValue<>("Name")<<" from library "<<modLib<<" failed configuration"<<std::endl;
      }
    }
  }
  if(m_moduleMap.size()==0){
    if(hasModule){
      *m_log<<APE::Log::FATAL<<"Module configuration(s) failed. Check config file"<<std::endl;
      throw std::logic_error("Incorrect module Configuration(s)");
    }
    *m_log<<APE::Log::FATAL<<"Need at least 1 Module in configuration"<<std::endl;
    throw std::logic_error("Need at least one Module section in configuration");
  }
  bool rv=openSocket(commType,commChUp,commChDown,useUID);
  if(!rv)return false;
  m_configured=true;
  return true;
}
