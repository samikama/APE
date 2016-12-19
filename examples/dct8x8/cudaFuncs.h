#ifndef FUNCLIST_H
#define FUNCLIST_H
#include "BmpUtil.h"
namespace dct8x8ModuleKernels{
  float WrapperCUDA1(byte *ImgSrc, byte *ImgDst, int Stride, ROI Size);
  float WrapperCUDA2(byte *ImgSrc, byte *ImgDst, int Stride, ROI Size);
}

#endif
