//syslibs
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <dlfcn.h>
//STL
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <iostream>
#include <algorithm>
//#include <cctype>
//TBB
#include "tbb/task_scheduler_init.h"
#include "tbb/tick_count.h"
// APE includes
#include "APE/Server.hpp"
#include "APE/ConfigTree.hpp"
#include "APE/YamlConfig.hpp"

void printUsage(std::string myName){
  if(myName.rfind('/')!=std::string::npos)myName=myName.substr(myName.rfind('/')+1);
  std::cout<<"Usage: "
	   <<myName<<" [OPTIONS] "<<std::endl
	   <<"Offloading server for executing algorithms on coprocessors"<<std::endl<<std::endl
	   <<"      -m,--module libraryName          Module library(ies) to run"<<std::endl
	   <<"      -c --config configFile           Configuration file (default APEConfig.yaml)"<<std::endl
	   <<"      -h,--help                        display this message"<<std::endl
	   <<std::endl;
}

int main(int argc,char* argv[]){
  int c;
  std::string moduleName;
  std::string configFile("APEConfig.yaml");
  std::vector<std::string> moduleNames;
  tbb::task_scheduler_init init(tbb::task_scheduler_init::deferred);
  bool configGiven=false;
  while (1) {
    int option_index = 0;
    static struct option long_options[] = {
      {"help", 0, 0, 'h'},
      {"config", 1, 0, 'c'},
      {"module", 1, 0, 'm'},      
      {0, 0, 0, 0}
    };

    c = getopt_long(argc, argv, "hc:m:",
		    long_options, &option_index);
    if (c == -1)
      break;
    switch (c) {
    case 'h':
      printUsage(argv[0]);
      exit(EXIT_SUCCESS);
      break;
    case 'c':  {      
      configGiven=true;
      configFile=optarg;
      if(moduleNames.size()>0){
	std::cerr<<"You should either use -m or -c options"<<std::endl;
	exit(EXIT_FAILURE);
      }
      break;
    }
    case 'm':  {      
      moduleName=optarg;
      moduleNames.push_back(moduleName);
      if(configGiven){
	std::cerr<<"You should either use -m or -c options"<<std::endl;
	exit(EXIT_FAILURE);
      }
      break;
    }
    default:
      printf("unknown parameter! getopt returned character code 0%o ??\n", c);
    }
  }
  //if(moduleNames.size()==0)moduleNames.push_back(moduleName);
  // if(numThreads>0){
  //   init.initialize(numThreads);
  //   if(verbosity>0)std::cout<<"Initialized tbb pool with "<<numThreads<<" threads."<<std::endl;
  // }else{
  init.initialize(tbb::task_scheduler_init::automatic);
    //numThreads=tbb::task_scheduler_init::default_num_threads();
  // }

  size_t totalBytes=0;
  size_t numLoops=0;
  tbb::tick_count t0 = tbb::tick_count::now();
  std::shared_ptr<APE::Server> server;
  std::shared_ptr<APEConfig::ConfigTree> config;
  if(configGiven){
    std::size_t pos=configFile.find_last_of('.');
    if(pos==std::string::npos){
      std::cerr<<"Config file needs an extension such as .yaml .xml .ini"<<std::endl;
      exit(EXIT_FAILURE);
    }
    std::string extension=configFile.substr(pos+1);
    std::transform(extension.begin(),extension.end(),extension.begin(),::tolower);
    try{
      config=APEConfig::ConfigIO::read(configFile,extension);
    }catch(std::exception &ex){
      std::cerr<<"Parsing config file failed with "<<ex.what()<<std::endl;
      exit(EXIT_FAILURE);
    }
    //config->print(std::cout);
    server=std::make_shared<APE::Server>(config);
  }else{
    if(moduleNames.size()>0){
      server=std::make_shared<APE::Server>(moduleNames[0]);
      if(moduleNames.size()>1){
	for(size_t md=1;md<moduleNames.size();md++){
	  if(!server->addModule(moduleNames[md])){
	    std::cerr<<"Adding module "<<md<<" to APE server failed "<<std::endl;
	  }
	}
      }
    }else{
      
    }
  }
  if(!server){
    std::cerr<<"Server configuration failed. "<<std::endl;
    exit(EXIT_FAILURE);
  }
  int retVal=server->run();
  tbb::tick_count::interval_t interval = tbb::tick_count::now()-t0;
  std::cout<<"IPC took "<<interval.seconds()<<" seconds "<<std::endl;
  std::cout<<"numLoops="<<numLoops<<", totBytes="<<totalBytes<<" exiting!"<<std::endl;
  std::cout<<"Timing stats"<<std::endl;

  // calcStats(serialCode,parallelCode,prepIntervals,serial_enabled,parallel_enabled);
  return retVal;
}
