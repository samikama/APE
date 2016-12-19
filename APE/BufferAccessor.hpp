#ifndef __APE_BUFFER_ACCESSOR_HPP
#define __APE_BUFFER_ACCESSOR_HPP

#include "APE/BufferContainer.hpp"
namespace APE{
  class BufferAccessor{
  public:
    static void* getRawBuffer(const BufferContainer& bc){return bc.getRawBuffer();}
    static size_t getRawBufferSize(const BufferContainer& bc){return bc.getRawBufferSize();}
    static void setToken(BufferContainer& bc,int t){bc.setToken(t);}
  };
}
#endif
