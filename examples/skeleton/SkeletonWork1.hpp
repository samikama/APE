//------------- -*- C++ -*- ---------------------
#ifndef __APE_SKELETON_WORK1_HPP
#define __APE_SKELETON_WORK1_HPP
#include "APE/Work.hpp"
#include "SkeletonDataStructs.hpp"

class SkeletonWork1:public APE::Work{
public:
  /** Constructor.
   * A Work object is a wrapper for all kernels that needs to be executed on given data by ATHENA
   * For example, for ID Work it will be composed of 
   *   - Bytestream decoding
       - postprocessing 
       - clusterization 
       - spacepoint making
       - Track seed finding
       - Tracking 
       - Clone removal
   * \param [in] data from ATHENA to be processed. It might be a simpler struct modified by module
   * \param [in] scratchSpace to be used for intermediate output
   * \param [in] conditions pointer to multi-event data
   * \param [in] geometry pointer to global data
   * \param [out] returnQueue to return the scratch space.
   * \param [out] completedQueue reference to Modules completed queue.
   
   **/
  SkeletonWork1(std::shared_ptr<APE::BufferContainer> data,
		EventSpecificData *scratchSpace,
		std::shared_ptr<MultiEventData> conditions,
		GlobalData* geometry,
		tbb::concurrent_queue<EventSpecificData*> &returnQueue,
		tbb::concurrent_queue<APE::Work*> &completedQueue
		);

  /** Destructor.
   *  return scratch space to Module;
   **/
  ~SkeletonWork1();

  /** Method to return results.
   *  Work Item needs to return a BufferContainer containing the 
   *  payload and headers from initial request. 
   **/
  std::shared_ptr<APE::BufferContainer> getOutput() override;

  /** Method to execute the work.  
   *  This method is where kernel
   *  executions and logic are put in. APE executes this method.
   *  run() should execute all kernels that need to be applied 
   *  to the given data. If possible intermediate steps should be
   *  kept on GPU memory, which should be allocated by module 
   *  and returned back on destruction of the module. If possible 
   *  output data structure should be prepared in GPU with a kernel, 
   *  and preferably should be copied to CPU buffer container at the 
   *  end of the run().
   * \return true if there is a result for sending back to client, false otherwise.
   **/
  bool run() override;

  /** Monitoring method to keep statistics.
   *
   **/
  const std::vector<double>& getStats() override;
private:

  std::shared_ptr<APE::BufferContainer> m_data; 
  EventSpecificData *m_scratchSpace; /**< This is to keep work/scratch areas on GPU and CPU possibly a complex stucture with pointers to different areas. ID context structure can be an example.*//
  std::shared_ptr<MultiEventData> m_Conditions;/**< This is to keep the constants valid for event in question. Possibly a struct containing pointers to constants such as Magnetic field, Noise values in GPU memory */
  GlobalData* m_geom; /**< Constants that do not change, such as Geometrydata or hash/lookup tables*/
  tbb::concurrent_queue<EventSpecificData*> &m_returnQueue; /**< This is to return the event specific data passed to work as scratch space back to module to prevent re-allocations both on CPU and GPU for each work*/
  tbb::concurrent_queue<APE::Work*> &m_completedQueue; /**< Completed queue of the Module, once Work is finished, it adds itself to this queue so that results will be sent back to ATHENA*/
  std::vector<double> m_stats;
  
};
#endif
