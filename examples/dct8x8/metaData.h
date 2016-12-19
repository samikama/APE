#pragma once
#include <cstddef>

namespace dctCudaModule{
  struct IMGInfo {
    size_t dataSize;
    unsigned int width;
    unsigned int height;
    unsigned int stride;
  };
}
