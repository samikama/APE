#include "dct8x8Module.hpp"
#include "dctCudaWork1.hpp"
#include "dctCudaWork2.hpp"
#include <cuda_runtime_api.h>

extern "C" APE::Module* getModule(){
  return new dct8x8Module();
}

extern "C" void deleteModule(APE::Module* c){
  dct8x8Module* dc=reinterpret_cast<dct8x8Module*>(c);
  delete dc;
}

dct8x8Module::dct8x8Module(){
  m_toDoList=new tbb::concurrent_queue<APE::Work*>();
  m_completedList=new tbb::concurrent_queue<APE::Work*>();
  cudaSetDevice(0);
}

dct8x8Module::~dct8x8Module(){
  
}

bool dct8x8Module::addWork(std::shared_ptr<APE::BufferContainer> data){
  //std::cout<<"Adding work"<<std::endl;
  if(data->getAlgorithm()==1){
    auto work=new dctCudaWork1(data,m_completedList);
    m_toDoList->push(work);
    return true;
  }else{
    auto work=new dctCudaWork2(data,m_completedList);
    m_toDoList->push(work);
    return true;
  }
  return false;
}

APE::Work* dct8x8Module::getResult(){
  APE::Work* w=0;
  m_completedList->try_pop(w);
  if(w){
    return w;
  }
  return 0;
}
int dct8x8Module::getModuleId(){return 0xE0000000;}

void dct8x8Module::printStats(std::ostream &out){};

const std::vector<int> dct8x8Module::getProvidedAlgs(){
  return std::vector<int>();
}

bool dct8x8Module::configure(std::shared_ptr<APEConfig::ConfigTree> &t){
  if(t==0)return false;
  return true;
}
