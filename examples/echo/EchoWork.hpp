#ifndef __APE_ECHO_WORK_HPP
#define __APE_ECHO_WORK_HPP
#include "APE/Work.hpp"
#include "APE/APEHist.hpp"
#include "APE/TimerHelper.hpp"
#include "tbb/concurrent_queue.h"

class EchoWork:public APE::Work{
public:
  EchoWork(std::shared_ptr<APE::BufferContainer> data,
	   std::shared_ptr<APE::BufferContainer> outBuff,
	   tbb::concurrent_queue<std::shared_ptr<APE::BufferContainer> > *rq,
	   tbb::concurrent_queue<APE::Work*> *q,
	   APEUtils::Hist1D<long>* runStats):m_outBuff(outBuff),m_rq(rq),m_q(q),
					  m_statsHisto(runStats){
    m_data=data;
  }
  ~EchoWork(){m_rq->push(m_outBuff);}
  std::shared_ptr<APE::BufferContainer> getOutput() override {
    return m_outBuff;
  }
  bool run() override{ 
    APEUtils::TimerHelper<APEUtils::Hist1D<long>,std::chrono::microseconds> tstats(m_statsHisto,1.0);
    if(m_outBuff->getBufferSize()<m_data->getBufferSize()){
      m_outBuff.reset(new APE::BufferContainer(m_data->getBufferSize()));
    }
    m_outBuff->setHeaderInfoFrom(*m_data);
    m_q->push(this);
    return true;
  }
  const std::vector<double>& getStats() override {return m_stats;}
private:
  std::shared_ptr<APE::BufferContainer> m_data;
  std::vector<double> m_stats;
  std::shared_ptr<APE::BufferContainer> m_outBuff;
  tbb::concurrent_queue< std::shared_ptr<APE::BufferContainer> > *m_rq;
  tbb::concurrent_queue<APE::Work*> *m_q;
  APEUtils::Hist1D<long>* m_statsHisto;
};
#endif
