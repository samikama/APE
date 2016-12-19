#include "APE/Module.hpp"

class dct8x8Module:public APE::Module{
public:
  dct8x8Module();
  ~dct8x8Module();
  virtual bool addWork(std::shared_ptr<APE::BufferContainer> data) override ;
  virtual APE::Work* getResult() override ;
  virtual void printStats(std::ostream &out) override ;
  virtual const std::vector<int> getProvidedAlgs() override;
  virtual int getModuleId() override;
  virtual bool configure(std::shared_ptr<APEConfig::ConfigTree>&) override;
private:
  
};
