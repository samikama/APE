#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <iostream>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <dirent.h>
#include <getopt.h>
#include "yampl/SocketFactory.h"
#include "APE/BufferContainer.hpp"
#include "APE/BufferAccessor.hpp"
#include "APE/ServerDefines.hpp"
#include "metaData.h"
#include "BmpUtil.h"
#include "defines.h"

void printUsage(std::string myName){
  if(myName.rfind('/')!=std::string::npos)myName=myName.substr(myName.rfind('/')+1);
  std::cout<<"Usage: "
	   <<myName<<" <number files to send>"<<std::endl
	   <<"Send bmp data files in dataDir to APE and check SNR on returned values"<<std::endl
	   <<"If directory contains fewer files than requested, same"<<std::endl
	   <<" files will be send in a cyclic manner multiple times until n is reached."<<std::endl
	   <<"      -n,--numTransfers <number>   Number of requests to send. (default=10000)"<<std::endl
	   <<"      -d,--dataDir                 Where to look for input bmp files (default=\"data\")"<<std::endl
	   <<"      -h,--help                    print this text and exit."<<std::endl
	   <<std::endl;
}

double deltaTms(const long end[2],const long start[2]){
  return ((end[0]-start[0])*1000.+(end[1]-start[1])/1000000.);
}

void printTimings(std::ostream& ost,const std::vector<double>& sec){
  double avg=0.0;
  double sum=0.0;
  double stddev=0.0;
  if(sec.size()<2){
    ost<<"Zero or one transfer succeded. No statistics "<<std::endl;
    return;
  }
  for(size_t k=1;k<sec.size();k++){
    sum+=sec.at(k);
  }
  avg=sum/((double)sec.size()-1);
  for(size_t k=0;k<sec.size();k++){
    double diff=avg-sec[k];
    stddev+=diff*diff;
  }
  stddev=sqrt(stddev/(double)sec.size());
  char buff[500];
  int pos=0;
  for(size_t k=0;k<sec.size();k++){
    pos+=snprintf(buff+pos,500-pos,"%10.7f ",sec.at(k));
    if(pos>120){
      snprintf(buff+pos,500-pos," (ms)");
      //ost<<buff<<std::endl;
      pos=0;
    }
  }
  snprintf(buff+pos,500-pos," (ms)\n\nAverage=%10.7f  Stddev=%10.7f\n",avg,stddev);
  ost<<buff;
}

void readData(const std::vector<std::string> &input,
	      std::vector<APE::BufferContainer*> &storage,
	      size_t &maxSize){
  int ImgWidth, ImgHeight;
  ROI ImgSize;
  char nameBuff[1024];//PreLoadBmp requires non-const char*
  for(auto n: input){
    snprintf(nameBuff,1024,"%s",n.c_str());
    int res = PreLoadBmp(nameBuff, &ImgWidth, &ImgHeight);
    ImgSize.width = ImgWidth;
    ImgSize.height = ImgHeight;
    
    //CONSOLE INFORMATION: saying hello to user
    std::cout<<"Loading test image: "<<n<<" ..."<<std::endl;
    if (res){
      std::cout<<" Image file \""<<n<<"\" not found or invalid. Skipping!"<<std::endl;
      continue;
    }
    
    //check image dimensions are multiples of BLOCK_SIZE
    if (ImgWidth % BLOCK_SIZE != 0 || ImgHeight % BLOCK_SIZE != 0){
      std::cout<<"Error: Input image dimensions must be multiples of 8!"<<std::endl;
      continue;
    }

    std::cout<<"loading image with dimensions [ "<<ImgWidth<<" x "<<ImgHeight<<" ] "<<std::endl;
    
    //allocate image buffers
    int ImgStride=((int)ceil(ImgWidth/16.0f))*16;
    size_t dataSize=ImgStride*ImgHeight;
    auto BC=new APE::BufferContainer(dataSize+sizeof(dctCudaModule::IMGInfo));
    BC->setModule(0xE0000000);
    dctCudaModule::IMGInfo *imgInfo=(dctCudaModule::IMGInfo*)BC->getBuffer();
    byte *imgData=(byte*)((char*)(BC->getBuffer())+sizeof(dctCudaModule::IMGInfo));
    imgInfo->width=ImgWidth;
    imgInfo->height=ImgHeight;
    imgInfo->stride=ImgStride;
    imgInfo->dataSize=dataSize;
    if(BC->getPayloadSize()>maxSize)maxSize=BC->getPayloadSize();
    //load sample image
    LoadBmpAsGray(nameBuff, ImgStride, ImgSize, imgData);
    storage.push_back(BC);
  }
}

int main(int argc, char *argv[]){
  const std::string ping = "terminate";
  //  char pong[100], *pong_ptr = &pong[0];
  int c=0;
  int numFiles=10000;
  std::string maxNum("10000");
  std::string dataDir("data");
  std::string sockName("apeSock");
  bool numGiven=false;
  bool saveOutput=false;
  while (1) {
    int option_index = 0;
    static struct option long_options[] = {
      {"help", 0, 0, 'h'},
      {"numTransfers", 1, 0, 'n'},
      {"dataDir", 1, 0, 'd'},
      {"saveOutput", 0, 0, 's'},
      {"socket-name",1,0,'S'},
      {0, 0, 0, 0}
    };

    // c = getopt_long(argc, argv, "hpn:d:f:i:s:t:",
    // 		    long_options, &option_index);
    c = getopt_long(argc, argv, "hn:d:sS:",
		    long_options, &option_index);
    if (c == -1)
      break;
    switch (c) {
    case 'h':
      printUsage(argv[0]);
      exit(EXIT_SUCCESS);
      break;
    case 's':
      saveOutput=true;
      break;
    case 'd':
      dataDir=std::string(optarg);
      break;
    case 'n':{
      maxNum=std::string(optarg);
      numGiven=true;
      break;
    }
    case 'S':{
      if(strlen(optarg)>0){
	sockName=std::string(optarg);
	break;
      }else{
	printUsage(argv[0]);	
	return EXIT_FAILURE;
      }
    }
    default:
      printf("unknown parameter! getopt returned character code 0%o ??\n", c);
    }
  }
  if(optind<argc){
    maxNum=std::string(argv[optind]);
    numGiven=true;
  }
  if(numGiven){
    errno=0;
    numFiles=strtol(maxNum.c_str(),0,10);
    if(errno!=0){
      
    }
  }
  std::vector<APE::BufferContainer*> fileData;
  std::vector<std::string> fileNames;

  if(dataDir.length()){
    struct dirent *entry;
    DIR *dp;
    dp = opendir(dataDir.c_str());
    if (dp == NULL) {
        perror("dataDir does not exist or could not be read.");
        exit(1);
    }
    while ((entry = readdir(dp))){
      std::string n(entry->d_name);
      if(n.length()>4 && n.substr(n.size()-4)==".bmp"){
	fileNames.push_back(dataDir+"/"+n);
      }
    }
    closedir(dp);
    if(fileNames.size()==0){
      std::cerr<<"dataDir=\""<<dataDir<<"\" do not contain any .bmp files"<<std::endl;
      exit(5);
    }else{
      std::cout<<"Found "<<fileNames.size()<<" bmp files in "<<dataDir<<std::endl;
    }
  }
  fileData.reserve(fileNames.size());
  size_t maxDataSize=0;
  readData(fileNames,fileData,maxDataSize);
  if(fileData.size()==0){
    std::cerr<<"Couldn't load any valid bmp files. Please check dataDirectory"<<std::endl;
    exit(1);
  }else{
    std::cout<<"File loading finished starting communication"<<std::endl;
  }
  char sockBuff[1000];
  uid_t userUID=geteuid();
  snprintf(sockBuff,1000,"%s_%u",sockName.c_str(),userUID);
  yampl::Channel channelS(sockBuff,yampl::LOCAL_SHM);
  snprintf(sockBuff,1000,"%s_%u_upload",sockName.c_str(),userUID);
  yampl::Channel channelR(sockBuff,yampl::LOCAL_SHM);
  auto *factory = new yampl::SocketFactory();
  auto connBuff=new APE::BufferContainer(sizeof(int));
  connBuff->setModule(SERVER_MODULE);
  connBuff->setAlgorithm(ALG_OPENING_CONNECTION);
  *(int*)connBuff->getBuffer()=(int)getpid();
  auto *ssocket = factory->createClientSocket(channelS);
  auto *rsocket = factory->createClientSocket(channelR);
  auto *RecvBuff=new APE::BufferContainer(maxDataSize);
  ssocket->send(APE::BufferAccessor::getRawBuffer(*connBuff), connBuff->getTransferSize());
  rsocket->send(APE::BufferAccessor::getRawBuffer(*connBuff), connBuff->getTransferSize());
  void *rBuff=APE::BufferAccessor::getRawBuffer(*RecvBuff);
  std::vector<double> timings;
  timings.reserve(numFiles);
  int token=0;
  size_t numFoundFiles=fileData.size();
  size_t sentDataVolume=0;
  size_t receivedDataVolume=0;
  for(size_t t=0;t<numFiles;t++){
    auto input=fileData.at(t%numFoundFiles);
    input->setAlgorithm(( t & 0x1)+1);
    APE::BufferAccessor::setToken(*input,token);
    if((t&0x3ff)==0)std::cout<<"Sending work # "<<t<<std::endl;
    auto start=std::chrono::steady_clock::now();
    sentDataVolume+=input->getTransferSize();
    ssocket->send(APE::BufferAccessor::getRawBuffer(*input), input->getTransferSize());
    ssize_t transferred=rsocket->tryRecv(rBuff, RecvBuff->getTransferSize(),100);
    std::chrono::duration<double> dur=std::chrono::steady_clock::now()-start;
    timings.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(dur).count());
    token++;
    if(transferred>0){
      receivedDataVolume+=transferred;      
    }else{
      std::cout<<"Receiving data failed!"<<std::endl;
    }
  }
  ssocket->send(ping.c_str(),ping.length());
  printTimings(std::cout,timings);
}
