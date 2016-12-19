#include "APE/APEHist.hpp"
#include <cstring>
#include <cstdlib>
#include <algorithm>

namespace APEUtils{
  Locksmith* Locksmith::m_instance=0;
  void Locksmith::setNLockBits(unsigned int n){
    if(n>0){
      if(n>31){//max 32 locks
	m_lockMask=~0-1;
	m_nLocks=~0;
      }else{
	m_lockMask=(1<<n)-1;
	m_nLocks=m_lockMask+1;
      }
      std::cerr<<"Setting m_lockMask="<<m_lockMask<<" m_nLocks="<<m_nLocks<<std::endl;

    };
  };
  std::vector<std::mutex> Locksmith::getLocks(){
    return std::vector<std::mutex>(m_nLocks);
  }
  
  HistBase::HistBase(const std::string& title):m_title(title){
    m_lockMask=APEUtils::Locksmith::getInstance()->getLockMask();
    m_locks=APEUtils::Locksmith::getInstance()->getLocks();
    size_t t=APEUtils::Locksmith::getInstance()->getNLocks();
    setTitle(title,m_title);
    m_sumW.resize(t,0.);
    m_sumW2.resize(t,0.);
    m_sumWx.resize(t,0.);
    m_sumWx2.resize(t,0.);
    m_nEnt.resize(t,0.);
  }

  void HistBase::setTitle(const std::string& title,std::string& target){
    target=title;
    if((title.find('|')!=std::string::npos) || (title.find(';')!=std::string::npos)){
      std::cerr<<"Illegal character at histogram label "<<title<<std::endl;
    }

    std::replace(target.begin(),target.end(),'|',' ');
    std::replace(target.begin(),target.end(),';',' ');
  }
    
  template<typename T>
  Hist1D<T>::Hist1D(const std::string &title,
		    unsigned int nbins, 
		    double xmin, double xmax):m_bins(0),m_nbins(nbins),m_xmin(xmin),m_xmax(xmax),m_ibinwidth(1.0),HistBase(title){
    if(nbins>0){
      m_bins=new T[nbins+2];
      ::memset(m_bins,0,sizeof(T)*(nbins+2));
    }
    if(xmin>xmax){
      m_xmin=xmax;
      m_xmax=xmin;
    }
    if(m_xmin==m_xmax){
      m_xmax+=1.0;
    }
    m_ibinwidth=((double)nbins)/(m_xmax-m_xmin);
  }

  template<typename T>
  Hist1D<T>::~Hist1D(){
    delete[] m_bins; 
  }

  template<typename T>
  void Hist1D<T>::print(std::ostream& out)const{
    out<<m_title;
    out<<" | "<<m_title<<" ; "<<x_title<<" ; "<<y_title<<" ; "<<z_title;
    out<<std::endl;
    out<<m_nbins<<" "<<m_xmin<<" "<<m_xmax<<" "
       <<std::accumulate(m_nEnt.begin(),m_nEnt.end(),0.)<<" "
       <<std::accumulate(m_sumW.begin(),m_sumW.end(),0.)<<" "
       <<std::accumulate(m_sumW2.begin(),m_sumW2.end(),0.)<<" "
       <<std::accumulate(m_sumWx.begin(),m_sumWx.end(),0.)<<" "
       <<std::accumulate(m_sumWx2.begin(),m_sumWx2.end(),0.)
       <<std::endl;
    for(size_t i=0;i<m_nbins+2;i++){
      out<<m_bins[i]<<" ";
    }
    out<<std::endl;
  }

  std::ostream & operator<<(std::ostream &out,const APEUtils::HistBase &b){
    b.print(out);
    return out;
  };

  template<typename T>
  Hist2D<T>::Hist2D(const std::string &title,
		    unsigned int nxbins, 
		    double xmin, double xmax,
		    unsigned int nybins, 
		    double ymin, double ymax
		    ):HistBase2D(title),m_bins(0),
		      m_nbins((nxbins+2)*(nybins+2)),m_nxbins(nxbins),m_nybins(nybins),
		      m_xmin(xmin),m_xmax(xmax),
		      m_ymin(ymin),m_ymax(ymax),
		      m_ixbinwidth(1.0),m_iybinwidth(1.0)
  {

    if(m_nbins>0){
      m_bins=new T[m_nbins];
      ::memset(m_bins,0,sizeof(T)*(m_nbins));
    }
    if(xmin>xmax){
      m_xmin=xmax;
      m_xmax=xmin;
    }
    if(m_xmin==m_xmax){
      m_xmax+=1.0;
    }
    if(ymin>ymax){
      m_ymin=ymax;
      m_ymax=ymin;
    }
    if(m_ymin==m_ymax){
      m_ymax+=1.0;
    }
    m_ixbinwidth=((double)m_nxbins)/(m_xmax-m_xmin);
    m_iybinwidth=((double)m_nybins)/(m_ymax-m_ymin);
  }

  template<typename T>
  Hist2D<T>::~Hist2D(){
    delete[] m_bins; 
  }
  

  template<typename T>
  void Hist2D<T>::print(std::ostream& out)const{
    out<<m_title;
    out<<" | "<<m_title<<" ; "<<x_title<<" ; "<<y_title<<" ; "<<z_title;
    out<<std::endl;
    out<<m_nxbins<<" "<<m_xmin<<" "<<m_xmax<<" "
       <<m_nybins<<" "<<m_ymin<<" "<<m_ymax<<" "
       <<std::accumulate(m_nEnt.begin(),m_nEnt.end(),0)<<" "
       <<std::accumulate(m_sumW.begin(),m_sumW.end(),0.)<<" "
       <<std::accumulate(m_sumW2.begin(),m_sumW2.end(),0.)<<" "
       <<std::accumulate(m_sumWx.begin(),m_sumWx.end(),0.)<<" "
       <<std::accumulate(m_sumWx2.begin(),m_sumWx2.end(),0.)
       <<std::endl;
    for(size_t i=0;i<m_nbins;i++){
      out<<m_bins[i]<<" ";
    }
    out<<std::endl;
  }


}// end APEUtils

template class APEUtils::Hist1D<char>;
template class APEUtils::Hist1D<unsigned char>;
template class APEUtils::Hist1D<short>;
template class APEUtils::Hist1D<unsigned short>;
template class APEUtils::Hist1D<int>;
template class APEUtils::Hist1D<unsigned int>;
template class APEUtils::Hist1D<long>;
template class APEUtils::Hist1D<unsigned long>;
template class APEUtils::Hist1D<long long>;
template class APEUtils::Hist1D<unsigned long long>;
template class APEUtils::Hist1D<float>;
template class APEUtils::Hist1D<double>;
template class APEUtils::Hist1D<long double>;

template class APEUtils::Hist2D<char>;
template class APEUtils::Hist2D<unsigned char>;
template class APEUtils::Hist2D<short>;
template class APEUtils::Hist2D<unsigned short>;
template class APEUtils::Hist2D<int>;
template class APEUtils::Hist2D<unsigned int>;
template class APEUtils::Hist2D<long>;
template class APEUtils::Hist2D<unsigned long>;
template class APEUtils::Hist2D<long long>;
template class APEUtils::Hist2D<unsigned long long>;
template class APEUtils::Hist2D<float>;
template class APEUtils::Hist2D<double>;
template class APEUtils::Hist2D<long double>;
