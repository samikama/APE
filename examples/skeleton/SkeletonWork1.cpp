#include "SkeletonWork1.hpp"

SkeletonWork1::SkeletonWork1(std::shared_ptr<APE::BufferContainer> data,
			     EventSpecificData *scratchSpace,
			     std::shared_ptr<MultiEventData> conditions,
			     GlobalData* geometry,
			     tbb::concurrent_queue<EventSpecificData*> &returnQueue,
			     tbb::concurrent_queue<APE::Work*> &completedQueue
			     ):m_data(data),m_scratchSpace(scratchSpace),
			       m_Conditions(conditions),m_geom(geometry),
			       m_returnQueue(returnQueue),m_completedQueue(completedQueue)
{
}
 
SkeletonWork1::~SkeletonWork1(){
    m_returnQueue.push(m_scratchSpace);
}
 
std::shared_ptr<APE::BufferContainer> SkeletonWork1::getOutput(){
  //prepare output buffer
  auto m_output=std::make_shared<APE::BufferContainer>();
  return m_output;
}

bool run() override{
  //execute kernel 1
  //execute kernel 2 
  //execute kernel 3
  //prepare output buffer
  //Execution is done. Add self to completed queue 
  m_completedQueue.push(this);
  return true;
}

const std::vector<double>& SkeletonWork1::getStats(){return m_stats;}
