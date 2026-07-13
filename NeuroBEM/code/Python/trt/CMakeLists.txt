cmake_minimum_required(VERSION 3.1)
project(test)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

find_package(CUDA REQUIRED)
message("CUDA Libs: ${CUDA_LIBRARIES}")
message("CUDA Headers: ${CUDA_INCLUDE_DIRS}")

# TensorRT
set(TENSORRT_ROOT "$ENV{TRT_ROOT}/lib")
find_library(NVINFER NAMES nvinfer HINTS ${TENSORRT_ROOT})
find_library(NVPARSERS NAMES nvparsers HINTS ${TENSORRT_ROOT})
find_library(NVINFERPLUGIN NAMES nvinfer_plugin HINTS ${TENSORRT_ROOT})
if (NVINFER AND NVPARSERS AND NVINFERPLUGIN)
    message("TensorRT is available!")
    message("NVINFER: ${NVINFER}")
    message("NVPARSERS: ${NVPARSERS}")
    message("NVINFERPLUGIN: ${NVINFERPLUGIN}")
    set(TRT_AVAIL ON)
else ()
    message("TensorRT is NOT Available")
    set(TRT_AVAIL OFF)
endif ()

# CuDNN
option(CUDNN_AVAIL "CUDNN available" OFF)
find_library(CUDNN_LIBRARY
        NAMES libcudnn.so${__cudnn_ver_suffix} libcudnn${__cudnn_ver_suffix}.dylib ${__cudnn_lib_win_name}
        PATHS $ENV{LD_LIBRARY_PATH} ${__libpath_cudart} ${CUDNN_ROOT_DIR} ${PC_CUDNN_LIBRARY_DIRS} ${CMAKE_INSTALL_PREFIX}
        PATH_SUFFIXES lib lib64 bin
        DOC "CUDNN library.")
if (CUDNN_LIBRARY)
    message("CUDNN is available!")
    message("CUDNN_LIBRARY: ${CUDNN_LIBRARY}")
    set(CUDNN_AVAIL ON)
else ()
    message("CUDNN is NOT Available")
    set(CUDNN_AVAIL OFF)
endif ()



include_directories(${CMAKE_SOURCE_DIR})

set(CUDA_NVCC_FLAGS "--std=c++11")
set(CMAKE_CXX_FLAGS "--std=c++11 -Wno-deprecated-declarations")
set(CMAKE_CXX_STANDARD 11)

set(TRT_ROOT "$ENV{TRT_ROOT}")
add_executable(test test.cpp)
target_include_directories(test PUBLIC ${TRT_ROOT}/include
        ${CUDA_INCLUDE_DIRS})
target_link_libraries(test
        ${NVINFER}
        ${NVPARSERS}
        ${NVINFERPLUGIN}
        ${CUDA_LIBRARIES}
        ${CUDA_CUBLAS_LIBRARIES}
        ${CUDA_curand_LIBRARY}
        ${CUDNN_LIBRARY}
        )
