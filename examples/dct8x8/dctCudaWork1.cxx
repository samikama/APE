#include "dctCudaWork1.hpp"
#include "cudaFuncs.h"// this is done to be able to have a separate cuda compilation
#include "metaData.h"
#include <iostream>

dctCudaWork1::dctCudaWork1(std::shared_ptr<APE::BufferContainer> data,  tbb::concurrent_queue<APE::Work*> *compQueue):m_data(data),m_compQueue(compQueue){

}

dctCudaWork1::~dctCudaWork1(){}

std::shared_ptr<APE::BufferContainer> dctCudaWork1::getOutput(){
  m_output->setHeaderInfoFrom(*m_data);
  return m_output;
}

bool dctCudaWork1::run(){
  //prepare data
  char* buff=(char*)m_data->getBuffer();
  //deserialization, it can be more advanced deserialization 
  dctCudaModule::IMGInfo *inf=(dctCudaModule::IMGInfo*)buff;
  byte* inputImg=(byte*)(buff+sizeof(dctCudaModule::IMGInfo));
  //allocate output image
  // This is done in buffer directly to prevent doing a memcopy later
  m_output=std::make_shared<APE::BufferContainer>(m_data->getBufferSize());// we have same output size
  ROI dimensions;
  dimensions.width=inf->width;
  dimensions.height=inf->height;
  char* outBuff=(char*)m_output->getBuffer();
  dctCudaModule::IMGInfo *outInfo=(dctCudaModule::IMGInfo*)buff;
  byte* outputImg=(byte*)(outBuff+sizeof(dctCudaModule::IMGInfo));
  //std::cout<<"Calling cuda kernel"<<std::endl;
  m_stats.push_back(dct8x8ModuleKernels::WrapperCUDA1(inputImg,outputImg,inf->stride,dimensions));//store time information in stats
  m_compQueue->push(this);
  return true;
}

const std::vector<double>& dctCudaWork1::getStats(){
  return m_stats;
}

