// - -*- C++ -*- - 
#ifndef __UTIL_MODULE_HPP__
#define __UTIL_MODULE_HPP__
#include "APE/Module.hpp"

class UtilModule:public APE::Module{
public:

  /** Constructor.
   **/
  UtilModule();

  /** Destructor.
   **/
  ~UtilModule();

  virtual bool addWork(std::shared_ptr<APE::BufferContainer> data) override ;
  virtual APE::Work* getResult() override ;
  virtual void printStats(std::ostream &out) override ;
  virtual const std::vector<int> getProvidedAlgs() override;
  virtual int getModuleId() override;
  virtual bool configure(std::shared_ptr<APEConfig::ConfigTree>&) override ;
private:
  std::vector<int> m_devices;
};

#endif
