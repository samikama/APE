#include <cuda.h>
#include "BmpUtil.h"
#include <cuda_runtime.h>
#include <helper_cuda.h>       // helper functions for CUDA timing and initialization
#include <helper_functions.h>  // helper functions for timing, string parsing

#define BENCHMARK_SIZE  10

texture<float, 2, cudaReadModeElementType> TexSrc;

#include "dct8x8_kernel1.cuh"
#include "dct8x8_kernel_quantization.cuh"
#include "dct8x8_kernel2.cuh"


namespace dct8x8ModuleKernels{

  float WrapperCUDA1(byte *ImgSrc, byte *ImgDst, int Stride, ROI Size){
    cudaChannelFormatDesc floattex = cudaCreateChannelDesc<float>();

    //allocate device memory
    cudaArray *Src;
    float *Dst;
    size_t DstStride;
    //printf("Allocating memory\n");
    checkCudaErrors(cudaMallocArray(&Src, &floattex, Size.width, Size.height));
    checkCudaErrors(cudaMallocPitch((void **)(&Dst), &DstStride, Size.width * sizeof(float), Size.height));
    DstStride /= sizeof(float);
    //printf("Converting img format\n");
    //convert source image to float representation
    int ImgSrcFStride;
    float *ImgSrcF = MallocPlaneFloat(Size.width, Size.height, &ImgSrcFStride);
    CopyByte2Float(ImgSrc, Stride, ImgSrcF, ImgSrcFStride, Size);
    AddFloatPlane(-128.0f, ImgSrcF, ImgSrcFStride, Size);

    //copy from host memory to device
    //printf("Copy to 2d array\n");
    checkCudaErrors(cudaMemcpy2DToArray(Src, 0, 0,
                                        ImgSrcF, ImgSrcFStride * sizeof(float),
                                        Size.width * sizeof(float), Size.height,
                                        cudaMemcpyHostToDevice));

    //setup execution parameters
    dim3 threads(BLOCK_SIZE, BLOCK_SIZE);
    dim3 grid(Size.width / BLOCK_SIZE, Size.height / BLOCK_SIZE);

    //create and start CUDA timer
    StopWatchInterface *timerCUDA = 0;
    sdkCreateTimer(&timerCUDA);
    sdkResetTimer(&timerCUDA);
    //printf("Bind texture\n");
    //execute DCT kernel and benchmark
    checkCudaErrors(cudaBindTextureToArray(TexSrc, Src));
    //printf("Run kernel\n");
    for (int i=0; i<BENCHMARK_SIZE; i++)
      {
        sdkStartTimer(&timerCUDA);
        CUDAkernel1DCT<<< grid, threads >>>(Dst, (int) DstStride, 0, 0);
        checkCudaErrors(cudaDeviceSynchronize());
        sdkStopTimer(&timerCUDA);
      }
    //printf("Unbind kernel\n");
    checkCudaErrors(cudaUnbindTexture(TexSrc));
    getLastCudaError("Kernel execution failed");

    // finalize CUDA timer
    float TimerCUDASpan = sdkGetAverageTimerValue(&timerCUDA);
    sdkDeleteTimer(&timerCUDA);
    //printf("Run Quantization\n");
    // execute Quantization kernel
    CUDAkernelQuantizationFloat<<< grid, threads >>>(Dst, (int) DstStride);
    getLastCudaError("Kernel execution failed");
    //printf("Copy Quantization\n");
    //copy quantized coefficients from host memory to device array
    checkCudaErrors(cudaMemcpy2DToArray(Src, 0, 0,
                                        Dst, DstStride *sizeof(float),
                                        Size.width *sizeof(float), Size.height,
                                        cudaMemcpyDeviceToDevice));

    // execute IDCT kernel
    //printf("Bind Quantization\n");
    checkCudaErrors(cudaBindTextureToArray(TexSrc, Src));
    //printf("Run IDCT\n");
    CUDAkernel1IDCT<<< grid, threads >>>(Dst, (int) DstStride, 0, 0);
    checkCudaErrors(cudaUnbindTexture(TexSrc));
    getLastCudaError("Kernel execution failed");
    //printf("Copy IDCT back\n");
    //copy quantized image block to host
    checkCudaErrors(cudaMemcpy2D(ImgSrcF, ImgSrcFStride *sizeof(float),
                                 Dst, DstStride *sizeof(float),
                                 Size.width *sizeof(float), Size.height,
                                 cudaMemcpyDeviceToHost));
    //printf("Convert to byte\n");
    //convert image back to byte representation
    AddFloatPlane(128.0f, ImgSrcF, ImgSrcFStride, Size);
    CopyFloat2Byte(ImgSrcF, ImgSrcFStride, ImgDst, Stride, Size);

    //clean up memory
    //printf("Cleanup\n");
    checkCudaErrors(cudaFreeArray(Src));
    checkCudaErrors(cudaFree(Dst));
    FreePlane(ImgSrcF);

    //return time taken by the operation
    return TimerCUDASpan;
  };

  float WrapperCUDA2(byte *ImgSrc, byte *ImgDst, int Stride, ROI Size){
    int StrideF;
    float *ImgF1 = MallocPlaneFloat(Size.width, Size.height, &StrideF);

    //convert source image to float representation
    CopyByte2Float(ImgSrc, Stride, ImgF1, StrideF, Size);
    AddFloatPlane(-128.0f, ImgF1, StrideF, Size);

    //allocate device memory
    float *src, *dst;
    size_t DeviceStride;
    checkCudaErrors(cudaMallocPitch((void **)&src, &DeviceStride, Size.width * sizeof(float), Size.height));
    checkCudaErrors(cudaMallocPitch((void **)&dst, &DeviceStride, Size.width * sizeof(float), Size.height));
    DeviceStride /= sizeof(float);

    //copy from host memory to device
    checkCudaErrors(cudaMemcpy2D(src, DeviceStride * sizeof(float),
                                 ImgF1, StrideF * sizeof(float),
                                 Size.width * sizeof(float), Size.height,
                                 cudaMemcpyHostToDevice));

    //create and start CUDA timer
    StopWatchInterface *timerCUDA = 0;
    sdkCreateTimer(&timerCUDA);

    //setup execution parameters
    dim3 GridFullWarps(Size.width / KER2_BLOCK_WIDTH, Size.height / KER2_BLOCK_HEIGHT, 1);
    dim3 ThreadsFullWarps(8, KER2_BLOCK_WIDTH/8, KER2_BLOCK_HEIGHT/8);

    //perform block-wise DCT processing and benchmarking
    const int numIterations = 100;

    for (int i = -1; i < numIterations; i++)
      {
        if (i == 0)
	  {
            checkCudaErrors(cudaDeviceSynchronize());
            sdkResetTimer(&timerCUDA);
            sdkStartTimer(&timerCUDA);
	  }

        CUDAkernel2DCT<<<GridFullWarps, ThreadsFullWarps>>>(dst, src, (int)DeviceStride);
        getLastCudaError("Kernel execution failed");
      }

    checkCudaErrors(cudaDeviceSynchronize());
    sdkStopTimer(&timerCUDA);

    //finalize timing of CUDA Kernels
    float avgTime = (float)sdkGetTimerValue(&timerCUDA) / (float)numIterations;
    sdkDeleteTimer(&timerCUDA);
    //printf("%f MPix/s //%f ms\n", (1E-6 * (float)Size.width * (float)Size.height) / (1E-3 * avgTime), avgTime);

    //setup execution parameters for quantization
    dim3 ThreadsSmallBlocks(BLOCK_SIZE, BLOCK_SIZE);
    dim3 GridSmallBlocks(Size.width / BLOCK_SIZE, Size.height / BLOCK_SIZE);

    // execute Quantization kernel
    CUDAkernelQuantizationFloat<<< GridSmallBlocks, ThreadsSmallBlocks >>>(dst, (int) DeviceStride);
    getLastCudaError("Kernel execution failed");

    //perform block-wise IDCT processing
    CUDAkernel2IDCT<<<GridFullWarps, ThreadsFullWarps >>>(src, dst, (int)DeviceStride);
    checkCudaErrors(cudaDeviceSynchronize());
    getLastCudaError("Kernel execution failed");

    //copy quantized image block to host
    checkCudaErrors(cudaMemcpy2D(ImgF1, StrideF *sizeof(float),
                                 src, DeviceStride *sizeof(float),
                                 Size.width *sizeof(float), Size.height,
                                 cudaMemcpyDeviceToHost));

    //convert image back to byte representation
    AddFloatPlane(128.0f, ImgF1, StrideF, Size);
    CopyFloat2Byte(ImgF1, StrideF, ImgDst, Stride, Size);

    //clean up memory
    checkCudaErrors(cudaFree(dst));
    checkCudaErrors(cudaFree(src));
    FreePlane(ImgF1);

    //return time taken by the operation
    return avgTime;
  };

}
