#include "APE/ConfigTree.hpp"
#include <sstream>
#include <algorithm>

APEConfig::Parameter::Parameter(const std::string &name,
				const std::string& val):m_name(name),m_value(val){
}
APEConfig::Parameter::Parameter(const std::string &name):m_name(name),m_value(""){};
APEConfig::Parameter::~Parameter(){}

namespace APEConfig{
  template<> 
  int Parameter::getValue<int>()const{
    return std::stoi(m_value);
  }


  template<> 
  float Parameter::getValue<float>()const{
    return std::stof(m_value);
  }

  template<> 
  double Parameter::getValue<double>()const{
    return std::stod(m_value);
  }

  template<> 
  long double Parameter::getValue<long double>()const{
    return std::stod(m_value);
  }

  template<>
  long Parameter::getValue<long>()const{
    return std::stol(m_value);
  }

  template<> 
  unsigned long Parameter::getValue<unsigned long>()const{
    return std::stoul(m_value);
  }

  template<> 
  unsigned int Parameter::getValue<unsigned int>()const{
    return (unsigned int)getValue<unsigned long>();
  }

  template<> 
  long long Parameter::getValue<long long>()const{
    return std::stoll(m_value);
  }

  template<> 
  unsigned long long Parameter::getValue<unsigned long long>()const{
    return std::stoull(m_value);
  }

  template<>
  std::string Parameter::getValue<std::string>()const{
    return m_value;
  }

  template<>
  bool Parameter::getValue<bool>()const{
    if(m_value=="t"||m_value=="1"||m_value=="True"||m_value=="true"||m_value=="TRUE") return true;
    return false;
  }

  void Parameter::print(std::ostream &out, const std::string &indent){
    out<<indent<<m_name<<" = "<<m_value<<std::endl;
  }
}

APEConfig::ConfigTree::ConfigTree(const std::string& name):m_myName(name){};
APEConfig::ConfigTree::~ConfigTree(){};

template<typename T> 
T APEConfig::ConfigTree::getValue(const std::string &key)const{
  auto it=std::find_if(m_params.begin(),m_params.end(),[&key](const std::shared_ptr<APEConfig::Parameter> &p){return key==p->getName();});
  if(it==m_params.end())throw APEConfig::MissingKey(key);
  return (*it)->getValue<T>();
}

template<typename T>
bool APEConfig::ConfigTree::addParameter(const std::string &key,
					 const T val,
					 bool overWrite){
  if(overWrite){
    auto it=std::find_if(m_params.begin(),m_params.end(),[&key](const std::shared_ptr<APEConfig::Parameter> p){return key==p->getName();});  
    if(it==m_params.end()){
      auto p=std::make_shared<APEConfig::Parameter>(key);
      p->setValue(val);
      m_params.push_back(p);
      return false;
    }else{
      (*it)->setValue(val);
      return true;
    }
  }else{
    auto p=std::make_shared<APEConfig::Parameter>(key);
    p->setValue(val);
    m_params.push_back(p);
  }
  return false;
}

bool APEConfig::ConfigTree::addParameter(std::shared_ptr<APEConfig::Parameter> param,bool overWrite){
  if(overWrite){
    const std::string pkey=param->getName();
    auto it=std::find_if(m_params.begin(),m_params.end(),[&pkey](const std::shared_ptr<APEConfig::Parameter> p){return pkey==p->getName();});  
    if(it==m_params.end()){
      m_params.push_back(param);
      return false;
    }else{
      *it=param;
      return true;
    }
  }else{
    m_params.push_back(param);
  }
  return false;
}

std::shared_ptr<APEConfig::ConfigTree> const APEConfig::ConfigTree::getSubtree(const std::string &key) const{
  auto it=std::find_if(m_subTrees.begin(),m_subTrees.end(),[&key](const std::shared_ptr<APEConfig::ConfigTree> &t){return key==t->getName();});
  if(it==m_subTrees.end())return 0;
  return *it;
}

std::vector<std::shared_ptr<APEConfig::Parameter> > APEConfig::ConfigTree::findParameters(const std::string &key)const{
  auto f= [&key]( const std::shared_ptr<APEConfig::Parameter> &p){return (key==p->getName());};
  size_t n=std::count_if(m_params.begin(),m_params.end(),f);
  if(n==0)return std::vector<std::shared_ptr<APEConfig::Parameter> >();
  std::vector<std::shared_ptr<APEConfig::Parameter>> found,missing;
  found.resize(n);missing.resize(m_params.size()-n);
  std::partition_copy(m_params.begin(),m_params.end(),found.begin(),missing.begin(),f);
  return found;
} 

std::vector<std::shared_ptr<APEConfig::ConfigTree> > APEConfig::ConfigTree::findSubtrees(const std::string &key)const{
  auto f= [&key](const std::shared_ptr<APEConfig::ConfigTree> &p){return (key==p->getName());};
  size_t n=std::count_if(m_subTrees.begin(),m_subTrees.end(),f);
  if(n==0)return std::vector<std::shared_ptr<APEConfig::ConfigTree> >();
  std::vector<std::shared_ptr<APEConfig::ConfigTree> > found,missing;
  found.resize(n);missing.resize(m_subTrees.size()-n);
  std::partition_copy(m_subTrees.begin(),m_subTrees.end(),found.begin(),missing.begin(),f);
  return found;
}
std::vector<std::shared_ptr<APEConfig::Parameter> > APEConfig::ConfigTree::getParams() const{
  return m_params;
}

std::vector<std::shared_ptr<APEConfig::ConfigTree> > APEConfig::ConfigTree::getSubTrees() const{
  return m_subTrees;}

bool APEConfig::ConfigTree::addSubTree(std::shared_ptr<APEConfig::ConfigTree> p,
				       bool overWrite){
  if(overWrite){
    const std::string pkey=p->getName();
    auto it=std::find_if(m_subTrees.begin(),m_subTrees.end(),[&pkey](const std::shared_ptr<APEConfig::ConfigTree> &p){return pkey==p->getName();});  
    if(it==m_subTrees.end()){
      m_subTrees.push_back(p);
      return false;
    }else{
      *it=p;
      return true;
    }
  }else{
    m_subTrees.push_back(p);
  }
  return false;
}

void APEConfig::ConfigTree::print(std::ostream &out, const std::string &indent){
  out<<indent<<m_myName<<" { "<<std::endl;
  std::string nindent=indent+"   ";
  for(size_t t=0;t<m_myName.size();t++)nindent+=" ";
  for(auto &st:m_subTrees){
    st->print(out,nindent);
  }
  for(auto &p:m_params)p->print(out,nindent);
  out<<indent<<"}"<<std::endl;
}

// Parameter instantiations
template bool APEConfig::Parameter::getValue<bool>()const;
template int APEConfig::Parameter::getValue<int>()const;
template unsigned int APEConfig::Parameter::getValue<unsigned int>()const;
template long APEConfig::Parameter::getValue<long>()const;
template unsigned long APEConfig::Parameter::getValue<unsigned long>()const;
template long long APEConfig::Parameter::getValue<long long>()const;
template unsigned long long APEConfig::Parameter::getValue<unsigned long long>()const;
template float  APEConfig::Parameter::getValue<float>()const;
template double  APEConfig::Parameter::getValue<double>()const;
template long double APEConfig::Parameter::getValue<long double>()const;
template std::string APEConfig::Parameter::getValue<std::string>()const;

// ConfigTree get value instantiations
template bool APEConfig::ConfigTree::getValue<bool>(const std::string&)const;
template int APEConfig::ConfigTree::getValue<int>(const std::string&)const;
template unsigned int APEConfig::ConfigTree::getValue<unsigned int>(const std::string&)const;
template long APEConfig::ConfigTree::getValue<long>(const std::string&)const;
template unsigned long APEConfig::ConfigTree::getValue<unsigned long>(const std::string&)const;
template long long APEConfig::ConfigTree::getValue<long long>(const std::string&)const;
template unsigned long long APEConfig::ConfigTree::getValue<unsigned long long>(const std::string&)const;
template float  APEConfig::ConfigTree::getValue<float>(const std::string&)const;
template double  APEConfig::ConfigTree::getValue<double>(const std::string&)const;
template long double APEConfig::ConfigTree::getValue<long double>(const std::string&)const;
template std::string APEConfig::ConfigTree::getValue<std::string>(const std::string&)const;

// ConfigTree addValue instantiations

template bool APEConfig::ConfigTree::addParameter<bool>(const std::string&, const bool,bool);
template bool APEConfig::ConfigTree::addParameter<int>(const std::string&,const int,bool);
template bool APEConfig::ConfigTree::addParameter<unsigned int>(const std::string&, const unsigned int,bool);
template bool APEConfig::ConfigTree::addParameter<long>(const std::string&,const long, bool);
template bool APEConfig::ConfigTree::addParameter<unsigned long>(const std::string&,unsigned long,bool);
template bool APEConfig::ConfigTree::addParameter<long long>(const std::string&,const long long,bool);
template bool APEConfig::ConfigTree::addParameter<unsigned long long>(const std::string&,const unsigned long long,bool);
template bool  APEConfig::ConfigTree::addParameter<float>(const std::string&,const float,bool);
template bool  APEConfig::ConfigTree::addParameter<double>(const std::string&,const double,bool);
template bool APEConfig::ConfigTree::addParameter<long double>(const std::string&,const long double,bool);
template bool APEConfig::ConfigTree::addParameter<std::string>(const std::string&,const std::string,bool);
