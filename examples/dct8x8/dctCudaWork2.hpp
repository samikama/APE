#include "APE/Work.hpp"
#include "tbb/concurrent_queue.h"

class dctCudaWork2:public APE::Work{
public:
  dctCudaWork2(std::shared_ptr<APE::BufferContainer> data,
	       tbb::concurrent_queue<APE::Work*> *compQueue);
  ~dctCudaWork2();
  std::shared_ptr<APE::BufferContainer> getOutput() override;
  bool run() override;
  const std::vector<double>& getStats();
private:
  std::shared_ptr<APE::BufferContainer> m_data;
  std::shared_ptr<APE::BufferContainer> m_output;
  tbb::concurrent_queue<APE::Work*> *m_compQueue;
  std::vector<double> m_stats;
};
