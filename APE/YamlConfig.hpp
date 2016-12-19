#ifndef __APECONFIG__YAMLCONFIG_HPP
#define __APECONFIG__YAMLCONFIG_HPP
#include "APE/ConfigIO.hpp"
namespace YAML{
  class Node;
}
namespace APEConfig{
  class YamlConfig:public APEConfig::ConfigIO{
  public:
    YamlConfig(){
      setInstance(this,"yaml");
    };

    ~YamlConfig(){}
  protected:
    virtual std::shared_ptr<APEConfig::ConfigTree> read_impl(const std::string &fileName);
    virtual void write_impl(std::shared_ptr<APEConfig::ConfigTree> t, const std::string &filename);
  private:
    std::shared_ptr<APEConfig::Parameter> parseScalar(const std::string& key,const YAML::Node& n)const;
    std::shared_ptr<APEConfig::ConfigTree> parseSequence(const std::string& key, const YAML::Node& n)const;
    std::shared_ptr<APEConfig::ConfigTree> parseMap(const std::string& key, const YAML::Node& n)const;
  };
}

#endif
