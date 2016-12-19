#ifndef __APE_WORK_HPP
#define __APE_WORK_HPP
#include <vector>
#include <memory>
#include "APE/BufferContainer.hpp"

namespace APE{
  class Work{
  public:
    Work(){};
    virtual ~Work(){};
    virtual std::shared_ptr<BufferContainer> getOutput()=0;
    virtual bool run()=0;
    virtual const std::vector<double>& getStats()=0;
  protected:
    std::shared_ptr<BufferContainer> m_buffer;
    //bool m_ownBuffer;
  };
}

#endif
