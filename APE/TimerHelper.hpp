#ifndef __APE_TIMERHELPER_HPP
#define __APE_TIMERHELPER_HPP
#include <chrono>
#include "APE/APEHist.hpp"

namespace APEUtils{

  template <typename> struct TimerTraits;
  template <typename T> struct TimerTraits<APEUtils::Hist1D<T>>{
    typedef APEUtils::Hist1D<T> HistType;
    HistType* h;
  };
  template <typename T> struct TimerTraits<APEUtils::Hist2D<T>>{
    typedef APEUtils::Hist2D<T> HistType;
    HistType* h;
  };
  
  template <typename U,typename T=std::chrono::milliseconds >  
  class TimerHelper{
  public:
    TimerHelper( U *h,double y=1.0,double w=1.0):m_hist(h),m_y(y),m_w(w),m_tstart(std::chrono::steady_clock::now()){}
    ~TimerHelper(){m_hist->fill(std::chrono::duration_cast<T>(std::chrono::steady_clock::now()-m_tstart).count(),m_y,m_w);}
  private:
    U* m_hist;
    double m_y,m_w;
    std::chrono::steady_clock::time_point m_tstart;
  };
  // template <typename K,typename T=std::chrono::milliseconds>
  // class TimerHelper1D<APEutils::Hist1D<K>>{
  // public:
  //   TimerHelper1D(APEUtils::Hist1D<K> *h,double w)m_h(h),m_w(w),m_tstart(std::chrono::steady_clock::now()){}
  //   ~TimerHelper1D(){
  //     m_h->fill(std::chrono::duration_cast<T>(std::chrono::steady_clock::now()-m_tstart).count(),m_y,m_w);
  //   }
  // private:
  //   APEUtils::Hist1D<K> *m_h;
  //   double m_w;
  //   std::chrono::steady_clock::time_point m_tstart;
  // };
  // template <typename U,typename T=std::chrono::milliseconds >
  // class TimerHelper{
  // public:
  //   TimerHelper(U* histo,double y,double w=1.);
  //   ~TimerHelper();
  // private:
  //   U* m_hist;
  //   double m_y,m_w;
  //   std::chrono::steady_clock::time_point m_start;
  // };
  
  // template <typename T>
  // template <typename K>
  // class TimerHelper<APEUtils::Hist1D<K>,T>{
  // public:
  //   TimerHelper(APEUtils::Hist1D<K>* histo,double y=1.0, double w=1.0):m_hist(histo),m_y(y),m_w(w){
  //     m_start=std::chrono::steady_clock::now();
  //   }
  //   ~TimerHelper(){
  //     if(m_hist){
  // 	m_hist->fill(std::chrono::duration_cast<T>(std::chrono::steady_clock::now()-m_start).count(),m_y);
  //     }
  //   }
  // private:
  //   APEUtils::Hist1D<K>* m_hist;
  //   double m_y,m_w;
  //   std::chrono::steady_clock::time_point m_start;
  // };

  // template <typename T>
  // template <typename K>
  // class TimerHelper<APEUtils::Hist2D<K>,T>{
  // public:
  //   TimerHelper(APEUtils::Hist2D<K>* histo,double y=1.0, double w=1.0):m_hist(histo),m_y(y),m_w(w){
  //     m_start=std::chrono::steady_clock::now();
  //   }
  //   ~TimerHelper(){
  //     if(m_hist){
  // 	m_hist->fill(std::chrono::duration_cast<T>(std::chrono::steady_clock::now()-m_start).count(),m_y,m_w);
  //     }
  //   }
  // private:
  //   APEUtils::Hist2D<K>* m_hist;
  //   double m_y,m_w;
  //   std::chrono::steady_clock::time_point m_start;
  // };

}


#endif
