#include "APE/YamlConfig.hpp"
#include <iostream>
#include <fstream>
#include <stdexcept>
#include "config-yamlcpp.h"
#include "yaml-cpp/yaml.h"

namespace APEConfig{
  YamlConfig * cfg_yamlconfig=new APEConfig::YamlConfig();
}

// This is to support api change between yaml-cpp v0.3 and yaml-cpp v0.5

#ifdef HAVE_YAMLCPP03

std::shared_ptr<APEConfig::ConfigTree> APEConfig::YamlConfig::read_impl(const std::string &fileName){
  std::ifstream fin(fileName);
  if(!fin.good()){
    throw std::invalid_argument(std::string(" Can't open file ")+fileName+std::string(" for reading "));
  }
  auto ct=std::make_shared<APEConfig::ConfigTree>("APE");
  try{
    YAML::Parser parser(fin);
    YAML::Node doc;
    parser.GetNextDocument(doc);
    YAML::NodeType::value t=doc.Type();
    switch(t){
    case YAML::NodeType::Null:
      std::cout<<"Type is null "<<std::endl;
      break;
    case YAML::NodeType::Scalar:
      ct->addParameter(parseScalar("param",doc),false);
      break;
    case YAML::NodeType::Sequence:
      ct=parseSequence("APE",doc);
      break;
    case YAML::NodeType::Map:{
      ct=parseMap("APE",doc);
      break;
    }
    default:
      std::cout<<"Type is unknown "<<std::endl;
    }
    
  }catch(YAML::ParserException &ex){
    std::cerr<<" Caught a parser exception for Yaml "<<ex.what()<<std::endl;
    return 0;
  }catch(std::exception &ex){
    std::cerr<<" Caught an exception "<<ex.what()<<std::endl;
    return 0;
  }
  return ct;

}

std::shared_ptr<APEConfig::Parameter> APEConfig::YamlConfig::parseScalar(const std::string& key,const YAML::Node& n)const{
  std::string value;
  n>>value;
  return std::make_shared<APEConfig::Parameter>(key,value);
}

std::shared_ptr<APEConfig::ConfigTree> APEConfig::YamlConfig::parseSequence(const std::string& key, const YAML::Node& n)const{
  auto ct=std::make_shared<APEConfig::ConfigTree>(key);
  for(size_t t=0;t<n.size();t++){
    auto &it=n[t];
    auto typ=it.Type();
    switch(typ){
    case YAML::NodeType::Null:{
      std::cout<<"Type is null "<<std::endl;
      break;
    }
    case YAML::NodeType::Scalar:{
      ct->addParameter(parseScalar("param",it),false);
      break;
    }
    case YAML::NodeType::Sequence:{
      ct->addSubTree(parseSequence("SubTree",it),false);
      break;
    }
    case YAML::NodeType::Map:{
      std::string key;
      it.begin().first()>>key;
      ct->addSubTree(parseMap(key,it.begin().second()),false);
      break;
    }
    default:
      std::cout<<"Type is unknown "<<std::endl;
    }

  }
  return ct;
}

std::shared_ptr<APEConfig::ConfigTree> APEConfig::YamlConfig::parseMap(const std::string& key, const YAML::Node& n)const{
  auto ct=std::make_shared<APEConfig::ConfigTree>(key);
  for(auto it=n.begin();it!=n.end();++it){
    std::string key;
    it.first()>>key;
    auto typ=it.second().Type();
    switch(typ){
    case YAML::NodeType::Null:{
      std::cout<<"Type is null "<<std::endl;
      break;
    }
    case YAML::NodeType::Scalar:{
      ct->addParameter(parseScalar(key,it.second()),false);
      break;
    }
    case YAML::NodeType::Sequence:{
      ct->addSubTree(parseSequence(key,it.second()),false);
      break;
    }
    case YAML::NodeType::Map:{
      ct->addSubTree(parseMap(key,it.second()),false);
      break;
    }
    default:
      std::cout<<"Type is unknown "<<std::endl;
    }
  }
  return ct;
}


void APEConfig::YamlConfig::write_impl(std::shared_ptr<APEConfig::ConfigTree> t, const std::string &filename){

}
//
// YAMLCPP 0.5 implementation
//
#else

std::shared_ptr<APEConfig::ConfigTree> APEConfig::YamlConfig::read_impl(const std::string &fileName){
  std::ifstream fin(fileName);
  if(!fin.good()){
    throw std::invalid_argument(std::string(" Can't open file ")+fileName+std::string(" for reading "));
  }
  auto ct=std::make_shared<APEConfig::ConfigTree>("APE");
  try{
    YAML::Node node=YAML::Load(fin);
    switch(node.Type()){
    case YAML::NodeType::Null:
      std::cout<<"Type is null "<<std::endl;
      break;
    case YAML::NodeType::Scalar:
      ct->addParameter(parseScalar("param",node),false);
      break;
    case YAML::NodeType::Sequence:
      ct=parseSequence("APE",node);
      break;
    case YAML::NodeType::Map:{
      ct=parseMap("APE",node);
      break;
    }
    default:
      std::cout<<"Type is unknown "<<std::endl;
    }
    
  }catch(YAML::ParserException &ex){
    std::cerr<<" Caught a parser exception for Yaml "<<ex.what()<<std::endl;
    return 0;
  }catch(std::exception &ex){
    std::cerr<<" Caught an exception "<<ex.what()<<std::endl;
    return 0;
  }
  return ct;

}

std::shared_ptr<APEConfig::Parameter> APEConfig::YamlConfig::parseScalar(const std::string& key,const YAML::Node& n)const{
  std::string value;

  try{
    value=n.as<std::string>();
  }catch(YAML::ParserException &ex){
    std::cerr<<" Caught a parser exception for Yaml "<<ex.what()<<std::endl;
    return 0;
  }catch(std::exception &ex){
    std::cerr<<" Caught an exception "<<ex.what()<<std::endl;
    return 0;
  }
  return std::make_shared<APEConfig::Parameter>(key,value);
}

std::shared_ptr<APEConfig::ConfigTree> APEConfig::YamlConfig::parseSequence(const std::string& key, const YAML::Node& n)const{
  auto ct=std::make_shared<APEConfig::ConfigTree>(key);
  for(size_t t=0;t<n.size();t++){
    auto &it=n[t];
    auto typ=it.Type();
    switch(typ){
    case YAML::NodeType::Null:{
      std::cout<<"Type is null "<<std::endl;
      break;
    }
    case YAML::NodeType::Scalar:{
      ct->addParameter(parseScalar("param",it),false);
      break;
    }
    case YAML::NodeType::Sequence:{
      ct->addSubTree(parseSequence("SubTree",it),false);
      break;
    }
    case YAML::NodeType::Map:{
      std::string key;
      key=it.begin()->first.as<std::string>();
      ct->addSubTree(parseMap(key,it.begin()->second),false);
      break;
    }
    default:
      std::cout<<"Type is unknown "<<std::endl;
    }
  }
  return ct;
}

std::shared_ptr<APEConfig::ConfigTree> APEConfig::YamlConfig::parseMap(const std::string& key, const YAML::Node& n)const{
  auto ct=std::make_shared<APEConfig::ConfigTree>(key);
  for(auto it=n.begin();it!=n.end();++it){
    std::string key;
    key=it->first.as<std::string>();
    auto typ=it->second.Type();
    switch(typ){
    case YAML::NodeType::Null:{
      std::cout<<"Type is null "<<std::endl;
      break;
    }
    case YAML::NodeType::Scalar:{
      ct->addParameter(parseScalar(key,it->second),false);
      break;
    }
    case YAML::NodeType::Sequence:{
      ct->addSubTree(parseSequence(key,it->second),false);
      break;
    }
    case YAML::NodeType::Map:{
      ct->addSubTree(parseMap(key,it->second),false);
      break;
    }
    default:
      std::cout<<"Type is unknown "<<std::endl;
    }
  }
  return ct;
}


void APEConfig::YamlConfig::write_impl(std::shared_ptr<APEConfig::ConfigTree> t, const std::string &filename){

}

#endif
