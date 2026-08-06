cmake_minimum_required(VERSION 3.16)
project(mybem CXX)

option(ENABLE_FAST "Build with optimizations for speed" ON)
option(UNSAFE_MATH "Build with -funsafe-math-optimizations" ON)
option(ENABLE_PARALLEL "Build using OpenMP parallelization" ON)
option(ENABLE_NATIVE "Build with -march=native" ON)
option(EIGEN_FROM_SYSTEM "Use the system-provided Eigen" ON)
set(EIGEN_ALTERNATIVE "" CACHE STRING "Path to an alternative Eigen include dir")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif ()

find_package(GSL REQUIRED)
if (ENABLE_PARALLEL)
    # AppleClang ships no OpenMP runtime, so FindOpenMP fails unless it is
    # pointed at brew's libomp. On gcc/clang elsewhere this block is a no-op.
    if (APPLE AND NOT OpenMP_CXX_FLAGS)
        execute_process(COMMAND brew --prefix libomp
                OUTPUT_VARIABLE LIBOMP OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
        if (EXISTS "${LIBOMP}/lib/libomp.dylib")
            set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp -I${LIBOMP}/include")
            set(OpenMP_CXX_LIB_NAMES omp)
            set(OpenMP_omp_LIBRARY "${LIBOMP}/lib/libomp.dylib")
        endif ()
    endif ()
    find_package(OpenMP)
    if (NOT OpenMP_CXX_FOUND)
        message(FATAL_ERROR
                "OpenMP not found (apt: libgomp, brew: install libomp, "
                "MetaCentrum: module add gcc). Pass -DENABLE_PARALLEL=OFF to "
                "build serial.")
    endif ()
endif ()

# No version floor: Eigen's config matches same-major-version, so asking for
# 3.3.4 rejects Eigen 5. Any Eigen providing Eigen/Core is fine here. Eigen 5
# only exports the Eigen3::Eigen target, it leaves EIGEN3_INCLUDE_DIR empty.
if (NOT EIGEN_ALTERNATIVE STREQUAL "")
    set(EIGEN_INCLUDE_DIR ${EIGEN_ALTERNATIVE})
elseif (EIGEN_FROM_SYSTEM)
    find_package(Eigen3 QUIET)
    if (NOT Eigen3_FOUND)
        message(FATAL_ERROR
                "Eigen not found. Install it (apt: libeigen3-dev, brew: eigen, "
                "MetaCentrum: module add eigen) or pass -DEIGEN_ALTERNATIVE=<path>.")
    endif ()
    message(STATUS "Using system Eigen ${Eigen3_VERSION}")
endif ()

add_library(mybem_sim
        src/model.cpp
        src/registry.cpp
        src/io/yaml.cpp
        src/io/csv.cpp
        src/io/drone.cpp
        src/tune/cma.cpp
        src/models/propeller_model.cpp
        src/models/simple.cpp
        src/models/bem.cpp
        src/models/body_drag.cpp
        src/models/motor_reaction.cpp
        src/bem/gsl_helper.cpp
        src/bem/integrands.cpp
        src/bem/coning.cpp
        src/bem/longitudinal_flapping.cpp
        src/bem/lateral_flapping.cpp
)

target_include_directories(mybem_sim PUBLIC include ${EIGEN_INCLUDE_DIR})
target_link_libraries(mybem_sim PUBLIC GSL::gsl)
if (TARGET Eigen3::Eigen)
    target_link_libraries(mybem_sim PUBLIC Eigen3::Eigen)
endif ()
target_compile_options(mybem_sim PRIVATE -Wall)

if (ENABLE_FAST)
    target_compile_options(mybem_sim PRIVATE -O3)
endif ()
if (UNSAFE_MATH)
    target_compile_options(mybem_sim PRIVATE -funsafe-math-optimizations)
endif ()
if (ENABLE_NATIVE)
    target_compile_options(mybem_sim PRIVATE -march=native)
endif ()
if (ENABLE_PARALLEL AND OpenMP_CXX_FOUND)
    target_link_libraries(mybem_sim PUBLIC OpenMP::OpenMP_CXX)
endif ()

add_executable(mybem-apply src/apps/apply.cpp)
target_link_libraries(mybem-apply mybem_sim)

# OpenMP flags reach this target through mybem_sim's PUBLIC link.
add_executable(mybem-tune src/apps/tune.cpp)
target_link_libraries(mybem-tune mybem_sim)
