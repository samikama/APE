// - -*- C++ -*- - 
#ifndef __SKELETON_MODULE_HPP__
#define __SKELETON_MODULE_HPP__
#include "APE/Module.hpp"
#include "SkeletonDataStructs.hpp"

/** There are several different types of data exist for module perspective
    they are defined by their lifetime
    - GlobalData is the data that is constant throught the\n 
                 lifetime of the APE server and same among different processes \n
                 Examples can be geometry data, hash tables etc\n
                 There is only 1 copy of this is needed
    - MultiEventData is the data that is constant through multiple events but may change eventually\n
                 during the lifetime of APE server. Examples of this type can be\n
		 Magnetic Field, Conditions data, Noise values etc.\
		 There should be multiple copies of this type in a module and updated as ATHENA\n
		 sends update requests. Modules have to assign appropriate data to matching work items on creation
    - EventSpecificData is the data that has a lifetime of an event and discarded after processing is done.\n
                Examples of this type of data is typically shipped with request and/or\n
		a scratch space needed by work for intermediate steps. An example could be the \n
		GPU memory used as outputs of Bytestream decoding, post processing and clusterization kernels\n
		as well as the CPU memory buffer to copy final GPU output to.\n
		There should be #of work items on flight copies of this data type.
*/

class SkeletonModule:public APE::Module{
public:

  /** Constructor.
   **/
  SkeletonModule();

  /** Destructor.
   **/
  ~SkeletonModule();

  /** Method to process the request from ATHENA.
   * This method takes an action, 
   * possibly creating a Work object, depending on the request from,
   * ATHENA passed in BufferContainer. 
   * 
   **/
  virtual bool addWork(std::shared_ptr<APE::BufferContainer> data) override ;

  /** Method to return finished Work items.
   * This method returns pointer to completed works from completed Queue.
   * APE framework takes the output from the Work item and 
   * return the results to ATHENA then destructs the WORK item.
   **/
  virtual APE::Work* getResult() override ;

  /** Method to dump monitoring information.
   * This method dumps the statiscs and 
   * monitoring output to given stream.
   **/
  virtual void printStats(std::ostream &out) override ;

  /** Method to return list of implemented algorithms.
   * This method should return a vector of algorithms 
   * for APE framwork to match
   **/
  virtual const std::vector<int> getProvidedAlgs() override;

  /** Method to return ModuleID.
   *  APE framework will hand in the requests 
   *  to matching modules using ModuleID
   *  ModuleIDs have to be unique and request should be 
   *  made with the same ID in ATHENA side.
   **/
  virtual int getModuleId() override;
  virtual bool configure(std::shared_ptr<APEConfig::ConfigTree>&) override ;
private:

  // Global data. Only one instance is necessary
  GlobalData *m_geometry;
  //list of LBs to Noise vaules. Integer is the LB number which given data is valid from
  std::vector<std::pair<int,std::shared_ptr<MultiEventData> > m_NoiseValues; 
  // Concurrent queue at which work items going to return the 
  // scratch space for Module to pass the information other work items
  tbb::concurrent_queue<EventSpecificData*> m_Work1ScratchStructures;
  tbb::concurrent_queue<EventSpecificDataWork2*> m_Work2ScratchStructures;
};

#endif
