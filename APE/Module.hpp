#ifndef __APE_MODULE_HPP
#define __APE_MODULE_HPP
#include <iostream>
#include <vector>
#include <memory>
#include "tbb/concurrent_queue.h"
#include "APE/Work.hpp"
#include "APE/ConfigTree.hpp"

namespace APE{
  class Module{
  public:
    Module(){};
    virtual ~Module(){};
    virtual bool addWork(std::shared_ptr<BufferContainer> data)=0;
    virtual Work* getWorkToRun(){  
      APE::Work *w=0;
      m_toDoList->try_pop(w);
      return w;
    }
    virtual Work* getResult(){
      APE::Work *w=0;
      m_toDoList->try_pop(w);
      return w;
    }
    virtual void printStats(std::ostream &out)=0;
    virtual const std::vector<int> getProvidedAlgs()=0;
    virtual char* getCustomBuffer(){return 0;}
    virtual int getModuleId()=0;
    virtual bool configure(std::shared_ptr<APEConfig::ConfigTree>&)=0;
  protected:
    tbb::concurrent_queue<APE::Work*> *m_toDoList;
    tbb::concurrent_queue<APE::Work*> *m_completedList;
  };
}

#endif
