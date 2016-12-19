#include "APE/ConfigIO.hpp"
#include <stdexcept>

std::map<std::string,APEConfig::ConfigIO*>& APEConfig::ConfigIO::getPluginMap(){
  static std::map<std::string,APEConfig::ConfigIO*>* m_plugins=new std::map<std::string,APEConfig::ConfigIO*>();
  return *m_plugins;
}

//std::map<std::string,APEConfig::ConfigIO*> APEConfig::ConfigIO::m_plugins;

std::shared_ptr<APEConfig::ConfigTree> APEConfig::ConfigIO::read(const std::string &fileName,const std::string &pname){
  auto p=getPluginMap().find(pname);
  if(p==getPluginMap().end()){
    throw std::invalid_argument(std::string("plugin ")+pname+" is not known");
  }
  return p->second->read_impl(fileName);
}

void APEConfig::ConfigIO::write(std::shared_ptr<APEConfig::ConfigTree> t, const std::string &fileName,const std::string &pname){
  auto p=getPluginMap().find(pname);
  if(p==getPluginMap().end()){
    throw std::invalid_argument(std::string("Plugin ")+pname+" is not known");
  }
  p->second->write_impl(t,fileName);
}

APEConfig::ConfigIO* APEConfig::ConfigIO::getInstance(const std::string & name){
  auto p=getPluginMap().find(name);
  if(p==getPluginMap().end())return 0;
  return p->second;
}
bool APEConfig::ConfigIO::setInstance(ConfigIO* ins,const std::string &name){
  auto ret=getPluginMap().insert(std::make_pair(name,ins));
  return ret.second;
  //m_instance = ins;
}
