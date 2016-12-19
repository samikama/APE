#ifndef __APECONFIG__CONFIGIO_HPP
#define __APECONFIG__CONFIGIO_HPP
#include <memory>

#include "APE/ConfigTree.hpp"
namespace APEConfig{
  class ConfigIO{
  public:
    static std::shared_ptr<APEConfig::ConfigTree> read(const std::string &fileName,const std::string& plugin="yaml");
    static void write(std::shared_ptr<APEConfig::ConfigTree> t, const std::string &filename,const std::string& plugin="yaml");
    static ConfigIO* getInstance(const std::string& pluginName="yaml");
  protected:
    virtual std::shared_ptr<APEConfig::ConfigTree> read_impl(const std::string &fileName)=0;
    virtual void write_impl(std::shared_ptr<APEConfig::ConfigTree> t, const std::string &filename)=0;
    bool setInstance(ConfigIO* ins,const std::string & pluginName);
  private:
    static std::map<std::string,ConfigIO*>& getPluginMap();
  };

}

#endif
