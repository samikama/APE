// -------------------- -*- C++ -*- --------------------
#ifndef __APE_BUFFER_BASE_H
#define __APE_BUFFER_BASE_H
#include <cstddef>
namespace APE{
  class BufferBase{
  public:
    virtual size_t getBufferSize()const =0;
    virtual void* getBuffer()const =0;
    BufferBase(){};
    ~BufferBase(){};
  protected:
    virtual void* getRawBuffer()const=0;
    virtual size_t getRawBufferSize()const=0;
    BufferBase(const BufferBase&){};
    BufferBase& operator=(const BufferBase&){return *this;}
  };
}
#endif
