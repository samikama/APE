// -------------------- -*- C++ -*- --------------------
#ifndef __APE_BUFFERCONTAINER_H
#define __APE_BUFFERCONTAINER_H
#include <cstddef>
#include <cstdint>
#include "APE/BufferBase.hpp"

namespace APE{
  struct APEHeaders{
    int token;// uid for service
    int module; // module to run
    int algorithm; // algorithm to run
    size_t payloadSize; // size of the data in payload
    unsigned char priority;
    unsigned char accelType;
    int reserved;// reserved for extra use
    uint32_t srcID;
  };
  class BufferAccessor;
  class BufferContainer:public APE::BufferBase{
  public:
    /**
     * Constructor. Allocates a user buffer with size buffSize 
     */
    BufferContainer(size_t buffSize);

    /** 
     * Constructor. Uses buffer passed in buff as internal buffer.
     * Buffer should be big enough to contain headers and user data.
     * buff is NOT released on destruction of object.
     */
    BufferContainer(void* buff,size_t buffSize);
    ~BufferContainer();
    /**
     * Return the size of user buffer
     */
    size_t getBufferSize() const;
    /**
     * Return pointer to user buffer
     */
    void * getBuffer() const;
    /**
     * Return the size of the payload. It can be smaller than the buffer size
     */
    size_t getPayloadSize()const;
    /**
     * Set payload size. This determines how much of 
     * the user buffer will be transferred.
     */
    bool setPayloadSize(size_t t);
    /**
     * Get target module id.
     */
    int getModule()const;
    /**
     * Get requested algorithm id
     */
    int getAlgorithm()const;
    /**
     * Set target module.
     */
    void setModule(int m);
    /**
     *  Set requested algorithm id.
     */ 
    void setAlgorithm(int a);
    /**
     * Get uid token set by the service. It is 
     * used for identification of request.
     */
    int getToken()const;
    /**
     * Copy header information from another BufferContainer.
     * Needed for setting the output BufferContainer headers. 
     */
    void setHeaderInfoFrom(const BufferContainer&);
    /**
     * Get total transfer size. It corresponds to headers + payload size.
     */
    size_t getTransferSize();
    /**
     * set SRC identifier. Shouldn't be necessary. 
     */ 
    void setSrcID(uint32_t srcId);
    /**
     * get SRC identifier. Shouldn't be necessary. 
     */ 
    uint32_t getSrcID();
  protected:
    friend class BufferAccessor;
    void setToken(int t);
    APEHeaders* getHeaders() const;
    void* getRawBuffer()const ;
    size_t getRawBufferSize()const;

  private:
    BufferContainer(const BufferContainer&)=delete;
    BufferContainer& operator=(const BufferContainer&)=delete;
    void* m_buffer;
    void* m_userBuffer;
    char* origBuffer;
    APEHeaders* header;
    size_t m_userBufferSize;
    size_t m_bufferSize;
  };
}


#endif
