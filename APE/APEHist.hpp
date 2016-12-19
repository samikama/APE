#ifndef __APETOOLS_HISTOGRAM_HPP
#define __APETOOLS_HISTOGRAM_HPP
#include <string>
#include <iostream>
#include <vector>
#include <mutex>
#include <atomic>

namespace APEUtils{
  class Locksmith{
  public:
    static Locksmith* getInstance(){
      if(m_instance==0){
	m_instance=new Locksmith();
      }
      return m_instance;
    };
    std::vector<std::mutex> getLocks();
    size_t getNLocks(){return m_nLocks;};
    size_t getLockMask(){return m_lockMask;}
    void setNLockBits(unsigned int n);
  private:
    static Locksmith* m_instance;
    size_t m_lockMask;
    unsigned int m_nLocks;
    Locksmith(){setNLockBits(4);}
    ~Locksmith(){}
  };

  class HistBase{
  public:
    HistBase(const std::string& title);
    virtual ~HistBase(){};
    virtual void fill(double x)=0;
    virtual void fill(double x,double y)=0;
    virtual void fill(double x,double y,double w)=0;
    void setXtitle(const std::string&t){setTitle(t,x_title);}
    void setYtitle(const std::string&t){setTitle(t,y_title);}
    void setZtitle(const std::string&t){setTitle(t,z_title);}
  protected:
    virtual void print(std::ostream &)const =0;
    std::size_t m_lockMask;
    std::string m_title;
    std::string x_title;
    std::string y_title;
    std::string z_title;
    std::vector<std::mutex> m_locks;
    std::vector<double> m_sumW;
    std::vector<double> m_sumW2;
    std::vector<double> m_sumWx;
    std::vector<double> m_sumWx2;
    std::vector<size_t> m_nEnt;
  private:
    void setTitle(const std::string& title,std::string& target);
    friend std::ostream & operator<<(std::ostream &, const APEUtils::HistBase &);
  };

  class HistBase2D:public HistBase{
  public:
    HistBase2D(const std::string& title):HistBase(title){}
    virtual ~HistBase2D(){};
    virtual void fill(double x, double y,double w)=0;
    virtual void fill(double x, double w)=0;
    virtual void fill(double x)=0;
  };
  
  template<typename T>
  class Hist1D : public HistBase{
  public:
    typedef T value_type;
    typedef Hist1D<T> hist_type;
    Hist1D(const std::string &title,unsigned int nbins, double xmin, double xmax);
    ~Hist1D();
    inline virtual void fill(double x) override final{fill(x,1.0);}
    inline virtual void fill(double x, double w) override final;
    inline virtual void fill(double x, double y, double w) override final{fill(x,w);}
  protected:
    virtual void print(std::ostream & )const override final;
    
  private:
    T* m_bins;
    unsigned int m_nbins;
    double m_xmin;
    double m_xmax;
    double m_ibinwidth;
    const std::string& typeStr()const;
  };

  template<typename T>
  class Hist2D:public HistBase2D{
  public:
    typedef T value_type;
    typedef Hist2D<T> hist_type;
    Hist2D(const std::string &title,unsigned int nxbins, double xmin, double xmax,
	   unsigned int nybins, double ymin, double ymax);
    ~Hist2D();
    inline virtual void fill(double x, double y) override final{fill(x,y,1.0);};
    inline virtual void fill(double x, double y,double w) override final;
    inline virtual void fill(double x)override final{};
  protected:
    virtual void print(std::ostream & )const override final;
  private:
    T* m_bins;
    unsigned int m_nbins,m_nxbins,m_nybins;
    double m_xmin,m_xmax;
    double m_ymin,m_ymax;
    double m_ixbinwidth,m_iybinwidth;
    const std::string& typeStr()const;
  };

  std::ostream & operator<<(std::ostream &out,const APEUtils::HistBase &b);

  template<typename T>
  void Hist1D<T>::fill(double x, double w){
    int nbin=(int)((x-m_xmin)*m_ibinwidth)+1;
    if(nbin<1){
      nbin=0;
    }else if(nbin>m_nbins+1){
      nbin=m_nbins+1;
    }
    std::size_t l=nbin & m_lockMask;
    if(l>m_locks.size()){
      std::cerr<<" l= "<<l
	       <<" x= "<<x
	       <<" nbin= "<<nbin
	       <<" m_lockMask= "<<m_lockMask
	       <<" m_xmin= "<<m_xmin
	       <<" m_xmax= "<<m_xmax
	       <<std::endl;
    }
    {
      std::unique_lock<std::mutex> binMtx(m_locks[l]);
      m_nEnt[l]+=1;
      m_sumW[l]+=w;
      m_sumW2[l]+=w*w;
      double wx=w*x;
      m_sumWx[l]+=wx;
      m_sumWx2[l]+=wx*x;
      m_bins[nbin]+=w;
    }
  }

  // template<typename T>
  // void Hist2D<T>::fill(double x, double w){
  //   int nbin=(int)((x-m_xmin)*m_ixbinwidth)+1;
  //   nbin+=m_nxbins+2;
  //   if(nbin<1){
  //     nbin=0;//set first row
  //   }else if(nbin>m_nbins){
  //     nbin=m_nbins;//first row
  //   }
  //   std::size_t l=nbin & m_lockMask;
  //   if(l>m_locks.size()){
  //     std::cerr<<" l= "<<l
  // 	       <<" x= "<<x
  // 	       <<" nbin= "<<nbin
  // 	       <<" m_lockMask= "<<m_lockMask
  // 	       <<" m_xmin= "<<m_xmin
  // 	       <<" m_xmax= "<<m_xmax
  // 	       <<std::endl;
  //   }
  //   std::unique_lock<std::mutex> binMtx(m_locks[l]);
  //   m_nEnt[l]+=1;
  //   m_sumW[l]+=w;
  //   m_sumW2[l]+=w*w;
  //   m_bins[nbin]+=w;
  // }
  template<typename T>
  void Hist2D<T>::fill(double x, double y,double w){
    int nxbin=(int)((x-m_xmin)*m_ixbinwidth)+1;
    int nybin=(int)((y-m_ymin)*m_iybinwidth)+1;
    int nbin=nybin*(m_nxbins+2)+nxbin;
    if(nbin<1){
      nbin=0;//set first row
    }else if(nbin>m_nbins){
      nbin=m_nbins;//first row
    }
    std::size_t l=nbin & m_lockMask;
    if(l>m_locks.size()){
      std::cerr<<" l= "<<l
	       <<" x= "<<x
	       <<" y= "<<y
	       <<" nxbin= "<<nxbin
	       <<" nybin= "<<nybin
	       <<" nbin= "<<nbin
	       <<" m_lockMask= "<<m_lockMask
	       <<" m_xmin= "<<m_xmin
	       <<" m_xmax= "<<m_xmax
	       <<std::endl;
    }
    {
      std::unique_lock<std::mutex> binMtx(m_locks[l]);
      m_nEnt[l]+=1;
      m_sumW[l]+=w;
      m_sumW2[l]+=w*w;
      double wx=w*x;
      m_sumWx[l]+=wx;
      m_sumWx2[l]+=wx*x;
      m_bins[nbin]+=w;
    }
  }
}


#endif
