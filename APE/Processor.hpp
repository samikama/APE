#ifndef __APE_PROCESSOR_HPP
#define __APE_PROCESSOR_HPP
#include "APE/Work.hpp"
#include "tbb/concurrent_queue.h"
#include "tbb/task.h"

namespace APE{
  class Processor:public tbb::task{
  public:
    Processor(tbb::concurrent_queue<Work*> *input,
	      tbb::concurrent_queue<Work*> *output):in(input),out(output){}
    virtual tbb::task* execute()=0;
  protected:
    tbb::concurrent_queue<Work*> *in;
    tbb::concurrent_queue<Work*> *out;
  };
}

#endif
