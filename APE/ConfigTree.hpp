#ifndef __APE_CONFIG_TREE_HPP
#define __APE_CONFIG_TREE_HPP

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <exception>
#include <memory>
#include <sstream>

namespace APEConfig{
  class MissingKey: public std::exception{
  public:
    MissingKey(const std::string &m){
      m_msg=std::string("Key ")+m+" does not exist";
    }
    virtual const char* what() const throw(){
      return m_msg.c_str();
    }
  private:
    std::string m_msg;
  };
  class Parameter{
  public:
    Parameter(const std::string &name,const std::string& val);
    Parameter(const std::string &name);
    ~Parameter();
    const std::string getName()const{return m_name;}
    template<typename T=std::string> 
    T getValue()const;
    template<typename T> 
    void setValue(const T& val){
      std::stringstream oss;
      oss<<val;
      m_value=oss.str();
    }
    void print(std::ostream &, const std::string &indent="");

  private:
    std::string m_name;
    std::string m_value;
  };
  
  class ConfigTree{
  public:
    ConfigTree(const std::string &name);
    ~ConfigTree();

    template<typename T=std::string> 
    T getValue(const std::string &key)const;

    template<typename T>
    bool addParameter(const std::string&,
		      const T,bool overWrite=true);
    bool addParameter(std::shared_ptr<APEConfig::Parameter> p,
		      bool overWrite=true);
    std::shared_ptr<APEConfig::ConfigTree> const 
              getSubtree(const std::string &key) const ;

    std::vector<std::shared_ptr<APEConfig::Parameter> > 
              findParameters(const std::string &key) const ;

    std::vector<std::shared_ptr<APEConfig::ConfigTree> > 
              findSubtrees(const std::string &key) const ;

    bool addSubTree(std::shared_ptr<APEConfig::ConfigTree> p,
		    bool overWrite=true);
    std::vector<std::shared_ptr<APEConfig::Parameter> > getParams() const;
    std::vector<std::shared_ptr<APEConfig::ConfigTree> > getSubTrees() const;
    const std::string& getName()const {return m_myName;}
    void print(std::ostream &, const std::string &indent="");
  private:
    std::string m_myName;
    std::vector<std::shared_ptr<APEConfig::ConfigTree> > m_subTrees;
    std::vector<std::shared_ptr<APEConfig::Parameter> > m_params;

  };


}


#endif
