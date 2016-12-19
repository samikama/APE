#include "SkeletonModule.hpp"
#include "SkeletonWork1.hpp"
#include "SkeletonWork2.hpp"
/**
   Factory Function
*/
extern "C" APE::Module* getModule(){
  return new SkeletonModule();
}

/**
   deleter Function
*/
extern "C" void deleteModule(APE::Module* c){
  SkeletonModule* dc=dynamic_cast<SkeletonModule*>(c);
  delete dc;
}

/** Constructor

 */

SkeletonModule::SkeletonModule(){
  // Inherited from APE::Module
  m_toDoList=new tbb::concurrent_queue<APE::Work*>();
  // Inherited from APE::Module
  // It is a concurrent storage for 
  m_completedList=new tbb::concurrent_queue<APE::Work*>();
  //Construct a some EventSpecificData on GPUs
};

SkeletonModule::~SkeletonModule(){
  delete m_toDoList;
  delete m_completedList;
};

void SkeletonModule::addWork(std::shared_ptr<APE::BufferContainer> data){
  auto alg=data->getAlgorithm();
  if(alg==ALGORITHM1){
    EventSpecificData* s=0;
    m_Work1ScratchStructures->try_pop(s);
    if(s==0){
      s=new EventSpecificData;//allocate a new one
    }
    //get LB number from data to find conditions
    int LBNumber=*((int*)data->getBuffer()); //buffer should be more complex and contain some timelike(such as LB) information 
    std::shared_ptr<MultiEventData> m;
    for(size_t t=m_NoiseValues.size()-1;t>=0;t--){
      if(m_NoiseValues.at(t).first<LBNumber){//pick the most recent valid conditions data
	m=m_NoiseValues.at(t).second;
	break;
      }
    }
    auto work = new SkeletonWork1(data,//data to process
				  s,//scratch/output area to work
				  m,//MultiEvent data
				  m_geometry,//Global data
				  m_Work1ScratchStructures,//Container to return scratch space once work is completed
				  m_completedList);//Container to return the work once exectuion completed to ship off to the ATHENA

    //Add work to todo list for APE framework to process.
    m_toDoList->push(work);
    return true;
  }else if(alg==ALGORITHM2){
    
    EventSpecificDataWork2* s=0;
    m_Work2ScratchStructures->try_pop(s);
    if(s==0){
      s=new EventSpecificDataWork2;//allocate a new one
    }
    //get LB number from data to find conditions
    int LBNumber=*((int*)data->getBuffer()); //buffer should be more complex and contain some timelike(such as LB) information 
    std::shared_ptr<MultiEventData> m;
    for(size_t t=m_NoiseValues.size()-1;t>=0;t--){
      if(m_NoiseValues.at(t).first<LBNumber){//pick the most recent valid conditions data
	m=m_NoiseValues.at(t).second;
	break;
      }
    }
    auto work2 = new SkeletonWork2(data,//data to process
				  s,//scratch/output area to work
				  m,//MultiEvent data
				  m_geometry,//Global data
				  m_Work2ScratchStructures,//Container to return scratch space once work is completed
				  m_completedList);//Container to return the work once exectuion completed to ship off to the ATHENA

    //Add work to todo list for APE framework to process.
    m_toDoList->push(work);
    return true;
  }else if(alg==UPDATECONDITIONS){
    auto newconds=((CONDITIONSDATA*)data->getBuffer());
    int LB=newconds->LB;
    //upload new conditions data to gpu
    // This can be done in a method or by a 
    // Class
    //UploadNewConditions2GPU()
    m_NoiseValues.push_back(std::make_pair(LB,newconds->REALCONDITIONS));
    return false;
  }
  return false;
}

APE::Work* SkeletonModule::getResult(){
  APE::Work *w=0;
  m_completedList->try_pop(w);
  return w;
}

int SkeletonModule::getModuleId(){return 0xE2000000;}

bool  SkeletonModule::configure(std::shared_ptr<APEConfig::ConfigTree>& ct){
  if(!ct)return false;
  return true;
}

void SkeletonModule::printStats(std::ostream &out){

}

const std::vector<int> SkeletonModule::getProvidedAlgs(){
  return std::vector<int>{0};
}
