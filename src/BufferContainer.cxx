// -------------------- -*- C++ -*- --------------------
#include <cstddef>
#include "APE/BufferContainer.hpp"

APE::BufferContainer::BufferContainer(size_t buffSize){
  m_bufferSize=buffSize+sizeof(APEHeaders);//add size of headers
  m_userBufferSize=buffSize;
  origBuffer=new char[m_bufferSize];
  m_userBuffer=static_cast<void*> (origBuffer+sizeof(APEHeaders));
  m_buffer=static_cast<void*>(origBuffer);
  header=static_cast<APEHeaders*>(m_buffer);
  header->payloadSize=m_userBufferSize;
}

APE::BufferContainer::BufferContainer(void *buff,size_t buffSize){
  m_bufferSize=buffSize;//add size of headers
  m_userBufferSize=buffSize-sizeof(APEHeaders);
  m_userBuffer=(((char*)buff)+sizeof(APEHeaders));
  m_buffer=buff;
  header=static_cast<APEHeaders*>(m_buffer);
  origBuffer=0;
  //header->payloadSize=m_userBufferSize;
  
}

APE::BufferContainer::~BufferContainer(){delete[] origBuffer;}

size_t APE::BufferContainer::getBufferSize() const {return m_userBufferSize;}

void * APE::BufferContainer::getBuffer() const{
  return m_userBuffer;
}

size_t  APE::BufferContainer::getPayloadSize()const {return header->payloadSize;}

bool APE::BufferContainer::setPayloadSize(size_t t){
  if(t>m_userBufferSize){
    header->payloadSize=m_userBufferSize;
    return false;
  }
  header->payloadSize=t;
  return true;
}

int  APE::BufferContainer::getModule()const {return header->module;}
int  APE::BufferContainer::getAlgorithm()const {return header->algorithm;}
void  APE::BufferContainer::setModule(int m){header->module=m;}
void  APE::BufferContainer::setAlgorithm(int a){header->algorithm=a;}
int  APE::BufferContainer::getToken()const {return header->token;}
void  APE::BufferContainer::setToken(int t){header->token=t;}
APE::APEHeaders* APE::BufferContainer::getHeaders() const {return header;}
void*  APE::BufferContainer::getRawBuffer()const {return m_buffer;}
size_t  APE::BufferContainer::getRawBufferSize()const {return m_bufferSize;}
void APE::BufferContainer::setHeaderInfoFrom(const BufferContainer &rhs){
  header->token=rhs.header->token;
  header->module=rhs.header->module;
  header->algorithm=rhs.header->algorithm;
  header->payloadSize=rhs.header->payloadSize;
  header->priority=rhs.header->priority;
  header->accelType=rhs.header->accelType;
  header->reserved=rhs.header->reserved;
  header->srcID=rhs.header->srcID;
}

size_t APE::BufferContainer::getTransferSize(){return sizeof(APEHeaders)+header->payloadSize;}

void APE::BufferContainer::setSrcID(uint32_t id){header->srcID=id;}
uint32_t APE::BufferContainer::getSrcID(){return header->srcID;}
