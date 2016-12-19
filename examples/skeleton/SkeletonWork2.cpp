// - -*- C++ -*- - 
#include "SkeletonWork2.hpp"

SkeletonWork2::SkeletonWork2(std::shared_ptr<APE::BufferContainer> data,
			     EventSpecificDataWork2 *scratchSpace,
			     std::shared_ptr<MultiEventData> conditions,
			     GlobalData* geometry,
			     tbb::concurrent_queue<EventSpecificDataWork2*> &returnQueue,
			     tbb::concurrent_queue<APE::Work*> &completedQueue
			     ):m_data(data),m_scratchSpace(scratchSpace),
			       m_Conditions(conditions),m_geom(geometry),
			       m_returnQueue(returnQueue),m_completedQueue(completedQueue)
{
}
 
SkeletonWork2::~SkeletonWork2(){
    m_returnQueue.push(m_scratchSpace);
}
 
std::shared_ptr<APE::BufferContainer> SkeletonWork2::getOutput(){
  //prepare output buffer
  auto m_output=std::make_shared<APE::BufferContainer>();
  return m_output;
}

bool run() override{
  //execute kernel A
  //execute kernel B 
  //execute kernel C
  //prepare output buffer
  //Execution is done. Add self to completed queue 
  m_completedQueue.push(this);
  return true;
}

const std::vector<double>& SkeletonWork2::getStats(){return m_stats;}
