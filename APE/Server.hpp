#ifndef __APE__SERVER__H
#define __APE__SERVER__H
#include <string>
#include <map>
#include <memory>
#include <yampl/SocketFactory.h>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <tbb/atomic.h>
#include <tbb/concurrent_queue.h>
#include <vector>
#include <chrono>
#include <unordered_map>

namespace APEConfig{
  class ConfigTree;
}

namespace APEUtils{
  class HistBase;
}

namespace APE{
  class Module;
  class Work;
  class BufferContainer;
  class Log;
  
  class Server{
  public:
    Server(std::shared_ptr<APEConfig::ConfigTree> config);
    Server(const std::string& moduleLib="libSerial.so",
	   const std::string& channelName="apeSock");
    ~Server();
    bool openSocket(const std::string &cType="SHM",
		    const std::string &chUp="apeSock_upload",
		    const std::string &chDown="apeSock",
		    bool useUID=true);
    bool addModule(const std::string &moduleName);
    std::shared_ptr<APE::Module> loadModule(const std::string& libName);
    bool loadModules();
    std::shared_ptr<APE::BufferContainer> receiveData();
    void sendResult(std::shared_ptr<APE::Log> log);
    void executeWorks(int id,std::shared_ptr<APE::Log> log);
    int run();
    void printStats(std::ostream& out);
    bool configure(std::shared_ptr<APEConfig::ConfigTree> &c);
  private:
    struct WorkInfo{
      const std::string *client;
      int origToken;
      char* inputBuff;
    };

    std::unordered_map<int,std::shared_ptr<APE::Module> > m_moduleMap;
    std::shared_ptr<APE::BufferContainer> m_input;
    std::unordered_map<std::string,int> m_clients;
    std::unordered_map<int,std::string> m_sendMap,m_recvMap;
    std::unordered_map<std::string,std::string> m_sockMap;
    int m_nClients;
    std::shared_ptr<yampl::ISocket> m_socket,m_sockUpload;
    std::vector<std::string> m_moduleLibs;
    std::string m_channelName;
    std::thread *m_senderThread;
    std::vector<std::thread *> m_runnerThreads;
    std::vector<std::string*> m_logs;
    std::vector<APEUtils::HistBase*> m_threadHists;
    std::unordered_map<std::string,APEUtils::HistBase*> m_histMap;
    std::mutex m_sendMutex,m_runnerMutex,m_threadHistMtx;
    std::condition_variable m_sendCond,m_runnerCond;
    tbb::atomic<int> m_enqueued,m_toRun;
    tbb::atomic<bool> m_stopSender,m_dumpQueue,m_senderSleeping,m_runnerSleeping,m_stopRunner;
    tbb::atomic<size_t> m_totalData;
    std::shared_ptr<APE::Log> m_log;
    std::unordered_map<std::string,void *> m_libHandles;
    std::unordered_map<std::string,int> m_libIdMap;
    std::unordered_map<int,std::string> m_modIdNameMap;
    tbb::concurrent_queue<int> m_tokenQueue;
    std::vector<APE::Server::WorkInfo> m_info;
    std::chrono::system_clock::time_point m_tStart;
    int m_maxTokens;
    unsigned int m_nRunners,m_nSenders,m_nReceivers;
    size_t m_handledRequests;
    std::shared_ptr<APEConfig::ConfigTree> m_myConfig;
    bool m_configured;
    bool m_keepConnOpen;
  };
}
#endif
