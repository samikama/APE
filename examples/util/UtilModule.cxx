#include "UtilModule.hpp"
#include "cuda_runtime.h"

/**
   Factory Function
*/
extern "C" APE::Module* getModule(){
  return new UtilModule();
}

/**
   deleter Function
*/
extern "C" void deleteModule(APE::Module* c){
  UtilModule* dc=dynamic_cast<UtilModule*>(c);
  delete dc;
}

/** Constructor

 */

UtilModule::UtilModule(){
  m_toDoList=new tbb::concurrent_queue<APE::Work*>();
  m_completedList=new tbb::concurrent_queue<APE::Work*>();
};

UtilModule::~UtilModule(){
  delete m_toDoList;
  delete m_completedList;
  for(auto i :m_devices){
    cudaSetDevice(i);
    cudaDeviceReset();
  }
}

bool UtilModule::addWork(std::shared_ptr<APE::BufferContainer> data){
  return false;
}

APE::Work* UtilModule::getResult(){
  APE::Work *w=0;
  m_completedList->try_pop(w);
  return w;
}

int UtilModule::getModuleId(){return 0xE3000000;}

bool  UtilModule::configure(std::shared_ptr<APEConfig::ConfigTree>& ct){
  if(!ct)return false;

  int maxDevice;
  cudaGetDeviceCount(&maxDevice);
  for(auto p:ct->getSubtree("AllowedGPUs")->findParameters("param")){
    int device=p->getValue<int>();
    if(device>=0 && device<maxDevice){    
      m_devices.push_back(device);
    }
  }
  return true;
}

void UtilModule::printStats(std::ostream &out){

}

const std::vector<int> UtilModule::getProvidedAlgs(){
  return std::vector<int>{0};
}
