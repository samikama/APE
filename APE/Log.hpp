#ifndef __APE_LOG_HPP
#define __APE_LOG_HPP
#include <string>
#include <mutex>
#include <sstream>
#include <iostream>

// namespace APELog{
//   class endreq{
//   public:
//     endreq(){};
//     ~endreq(){};
//   };
//   class endl{
//   public:
//     endl(){};
//     ~endl(){};
//   };
// }
namespace APE{
  class Log;
  
  class AtomicLog{
    friend class APE::Log;
  private:
    AtomicLog(std::ostream* stream,
	      std::mutex* lock,
	      const std::string& hdr,bool active=true);
    typedef std::basic_ostream<char, std::char_traits<char> > CoutType;
    // this is the function signature of std::endl
    typedef CoutType& (*StandardEndLine)(CoutType&);  

    typedef APE::AtomicLog& (*AtomicLogManipulator)(APE::AtomicLog&);
    void flush(){
      if(m_active){
     	m_buff<<std::endl;
     	std::unique_lock<std::mutex> lock(*m_lock);
     	*m_stream<<m_buff.str();
     	m_active=false;
      }
    }
  public:
    template<typename T>
    inline APE::AtomicLog& operator<<(T const& val){
      if(m_active){
	m_buff<<val;
      }
      return *this;
    }
    
    // APE::AtomicLog& operator<<(APELog::endreq const &v){
    //   if(m_active){
    // 	m_buff<<std::endl;
    // 	std::unique_lock<std::mutex> lock(*m_lock);
    // 	*m_stream<<m_buff.str();
    // 	m_active=false;
    //   }
    //   return *this;
    // }
    // APE::AtomicLog& operator<<(APELog::endl const &v){
    //   if(m_active){
    // 	m_buff<<std::endl;
    //   }
    //   return *this;
    // }
    APE::AtomicLog& operator<<(AtomicLogManipulator manip){return manip(*this);}
    APE::AtomicLog& operator<<(StandardEndLine manip){manip(m_buff);return *this;}

    inline APE::AtomicLog&  operator=(APE::AtomicLog&& other);
    AtomicLog(const APE::AtomicLog &&);
    ~AtomicLog();
  private:
    std::ostream* m_stream;
    std::stringstream m_buff;
    std::mutex *m_lock;
    bool m_active;
  };

  class Log{
  public:
    enum Level{VERBOSE=0,DEBUG=10,INFO=20,WARNING=30,ERROR=40,FATAL=50,NONE=9999};
    Log(std::string name,
	APE::Log::Level level=APE::Log::WARNING,
	std::ostream& =std::cout);
    //Log():m_stream(std::cout){}
    ~Log();
    APE::Log::Level setLevel(APE::Log::Level l);
    static APE::AtomicLog& endl(AtomicLog& stream){
      stream.m_buff<<std::endl;
      return stream;
    }
    static APE::AtomicLog& endreq(AtomicLog& stream){
      stream.flush();
      return stream;
    }

    inline APE::AtomicLog  operator<<(APE::Log::Level const& val){
      if(m_currLevel<=val){
	std::string hdr;
	switch(val){
	case VERBOSE:
	  {
	    hdr=m_myName+std::string(" <VERBOSE>: ");
	    break;
	  }
	case DEBUG:
	  {
	    hdr=m_myName+std::string(" <DEBUG>: ");
	    break;
	  }
	case INFO:
	  {
	    hdr=m_myName+std::string(" <INFO>: ");
	    break;
	  }
	case WARNING:
	  {
	    hdr=m_myName+std::string(" <WARNING>: ");
	    break;
	  }
	case ERROR:
	  {
	    hdr=m_myName+std::string(" <ERROR>: ");
	    break;
	  }
	case FATAL:
	  {
	    hdr=m_myName+std::string(" <FATAL>: ");
	    break;
	  }
	case NONE:
	default:
	  break;
	}
	return APE::AtomicLog(&m_stream,m_lock,hdr);
      }
      return APE::AtomicLog(nullptr,nullptr,{},false);
    }
    APE::AtomicLog info();
  private:
    
    std::string m_myName;
    APE::Log::Level m_currLevel;
    std::string m_header;
    std::ostream &m_stream;
    std::mutex *m_lock;
  };
}//namespace  
#endif
