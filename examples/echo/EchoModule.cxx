#include "EchoModule.hpp"
#include "EchoWork.hpp"

extern "C" APE::Module* getModule(){
  return new EchoModule();
}

extern "C" void deleteModule(APE::Module* c){
  EchoModule* dc=dynamic_cast<EchoModule*>(c);
  delete dc;
}

EchoModule::EchoModule(){
  //m_toDoList=new tbb::concurrent_queue<APE::Work*>();
  m_toDoList=new tbb::concurrent_queue<APE::Work*>();
  m_completedList=new tbb::concurrent_queue<APE::Work*>();
  m_runStats=new APEUtils::Hist1D<long>("Echo module run() execution timing (usec)",1000,0.,2000.);
};

EchoModule::~EchoModule(){
  delete m_toDoList;
  delete m_completedList;
  delete m_runStats;
};

bool EchoModule::addWork(std::shared_ptr<APE::BufferContainer> data){
  std::shared_ptr<APE::BufferContainer> b(0);
  m_buffs.try_pop(b);
  if(!b){
    b.reset(new APE::BufferContainer(data->getBufferSize()));
  }
  auto work = new EchoWork(data,b,&m_buffs,m_completedList,m_runStats);
  m_toDoList->push(work);
  return true;
}


APE::Work* EchoModule::getResult(){
  APE::Work *w=0;
  m_completedList->try_pop(w);
  return w;
}

int EchoModule::getModuleId(){return 0xE1000000;}

void EchoModule::printStats(std::ostream &out){
  out<<*m_runStats<<std::endl;
}

const std::vector<int> EchoModule::getProvidedAlgs(){
  return std::vector<int>{0};
}

bool EchoModule::configure(std::shared_ptr<APEConfig::ConfigTree> &t){
  if(t==0)return false;
  return true;
}
