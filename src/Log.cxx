#include "APE/Log.hpp"
#include <algorithm>
APE::Log::Log(std::string name,
	      APE::Log::Level level,
	      std::ostream &os):m_myName(name),
				m_currLevel(level),
				m_stream(os){
  m_lock=new std::mutex;
  switch(level){
  case VERBOSE:
    {
      m_header=m_myName+std::string(" <VERBOSE>: ");
      break;
    }
  case DEBUG:
    {
      m_header=m_myName+std::string(" <DEBUG>: ");
      break;
    }
  case INFO:
    {
      m_header=m_myName+std::string(" <INFO>: ");
      break;
    }
  case WARNING:
    {
      m_header=m_myName+std::string(" <WARNING>: ");
      break;
    }
  case ERROR:
    {
      m_header=m_myName+std::string(" <ERROR>: ");
      break;
    }
  case FATAL:
    {
      m_header=m_myName+std::string(" <FATAL>: ");
      break;
    }
  case NONE:
  default:
    break;	
  }
				}

APE::Log::~Log(){m_stream.flush();};
APE::Log::Level APE::Log::setLevel(APE::Log::Level l){
  auto old=m_currLevel;
  m_currLevel=l;
  return old;
}


APE::AtomicLog APE::Log::info(){
  if(m_currLevel<=APE::Log::INFO){
    return APE::AtomicLog(&m_stream,
			  m_lock,
			  m_myName+std::string(" <INFO>: "),
			  true);
  }
  return APE::AtomicLog(nullptr,nullptr,{},false);
}

  
APE::AtomicLog::AtomicLog(std::ostream* stream,
			  std::mutex* lock,
			  const std::string& hdr,
			  bool act):m_stream(stream),
				    m_lock(lock),
				    m_active(act){
  if(m_active){
    m_buff<<hdr;
  }
}

APE::AtomicLog::~AtomicLog(){
  if(m_active){
    std::unique_lock<std::mutex> lock(*m_lock);
    *m_stream<<m_buff.str();
  }
}

APE::AtomicLog& APE::AtomicLog::operator=(APE::AtomicLog&& other){
  m_stream=std::move(other.m_stream);
  //m_buff=std::move(other.m_buff);
  m_buff.str(other.m_buff.str());
  m_lock=std::move(other.m_lock);
  m_active=other.m_active;
}

APE::AtomicLog::AtomicLog(const APE::AtomicLog&& o):
  m_stream(std::move(o.m_stream)),
  m_lock(std::move(o.m_lock)),
  m_active(o.m_active){  m_buff.str(o.m_buff.str());}
