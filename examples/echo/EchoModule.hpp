#include "APE/Module.hpp"
#include "APE/APEHist.hpp"

class EchoModule:public APE::Module{
public:
  EchoModule();
  ~EchoModule();
  virtual bool addWork(std::shared_ptr<APE::BufferContainer> data) override ;
  virtual APE::Work* getResult() override ;
  virtual void printStats(std::ostream &out) override ;
  virtual const std::vector<int> getProvidedAlgs() override;
  virtual int getModuleId() override;
  virtual bool configure(std::shared_ptr<APEConfig::ConfigTree>&) override ;
private:
  tbb::concurrent_queue<std::shared_ptr<APE::BufferContainer> > m_buffs;
  APEUtils::Hist1D<long> *m_runStats;
};
