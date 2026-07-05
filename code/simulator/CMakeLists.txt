cmake_minimum_required(VERSION 3.0.2)
project(simulator)

################################################################################
# Options
################################################################################

option(ENABLE_FAST "Build with optimizations for speed" ON)
option(UNSAFE_MATH "Build with -funsafe-math-optimizations: high impact on speed, but lowers floating point precision." ON)
option(ENABLE_PARALLEL "Build using openmp parallelization" ON)
option(EIGEN_FROM_SYSTEM "Use the system-provided Eigen" ON)
set(
        EIGEN_ALTERNATIVE "" CACHE STRING
        "Path to alternative Eigen, autodownload if blank"
)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)


################################################################################
# Finding Dependencies
################################################################################
message(STATUS "===============   Setup Dependencies   ================")

find_package(OpenMP)
find_package(GSL REQUIRED)

if (EIGEN_FROM_SYSTEM)
    find_package(Eigen3 3.3.4 QUIET)
    if (EIGEN3_FOUND)
        message(STATUS "Using system provided Eigen.")
    else ()
        message(STATUS "No sufficient Eigen version (3.3.4) found.")
        message(STATUS "Restoring to download Eigen sources.")
        include(cmake/eigen.cmake)
    endif ()
elseif (EIGEN_ALTERNATIVE STREQUAL "")
    include(cmake/eigen.cmake)
else ()
    set(EIGEN_INCLUDE_DIR ${EIGEN_ALTERNATIVE})
endif ()

# Check for ccache
if (NOT DEFINED CATKIN_DEVEL_PREFIX)
    find_program(CCACHE_PROGRAM ccache)
    if (CCACHE_PROGRAM)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    else ()
        message(STATUS "Build time could be improved with ccache!")
        message(STATUS "    sudo apt install ccache")
    endif ()
endif ()


################################################################################
# Setup Compilation
################################################################################
message(STATUS "===============   Setup Compilation   =================")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_definitions(-DEIGEN_STACK_ALLOCATION_LIMIT=1048576)
include_directories(${EIGEN_INCLUDE_DIR})

# Set default build type
if (NOT EXISTS ${CMAKE_BINARY_DIR}/CMakeCache.txt)
    if (NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
    endif ()
endif ()

# Add c++ flags
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -Wall -DNDEBUG -Wno-deprecated-declarations")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -Wall -g")
set(CMAKE_CXX_ARCH_FLAGS " -march=native")

# Optimized flags
if (ENABLE_FAST)
    message(STATUS "Enabling fast optimization flags!")
    set(CMAKE_CXX_FAST_FLAGS " -O3")
else ()
    set(CMAKE_CXX_FAST_FLAGS " -O0")
endif ()
if (UNSAFE_MATH)
    message(STATUS "Using -funsafe-math-optimizations")
    set(CMAKE_CXX_FAST_FLAGS "${CMAKE_CXX_FAST_FLAGS} -funsafe-math-optimizations")
endif ()


# Parallelization Flags
if (ENABLE_PARALLEL)
    message(STATUS "Enabling parallelization!")
    set(CMAKE_CXX_PAR_FLAGS " -fopenmp")
else ()
    set(CMAKE_CXX_PAR_FLAGS " -DEIGEN_DONT_PARALLELIZE")
endif ()

# Summarize Flags
set(CMAKE_CXX_FLAGS_RELEASE
        "${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_CXX_FAST_FLAGS} ${CMAKE_CXX_ARCH_FLAGS} ${CMAKE_CXX_PAR_FLAGS} ${CMAKE_CXX_BLAS_FLAGS}")
      string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "The activated CXX RELEASE configuration is:\n ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "The activated CXX DEBUG configuration is:\n ${CMAKE_CXX_FLAGS_DEBUG}")


################################################################################
# Specify Build Resources
################################################################################
message(STATUS "==================   Setup Build   ====================")

# Create file lists for sources, tests and benchmarks
file(GLOB_RECURSE SOURCES
        src/*.cpp
        )


################################################################################
# Setup Build
################################################################################

# Library and Executables
include_directories(include)

if (NOT SOURCES)
    set(LIBRARY_NAME)
else ()
    add_library(${PROJECT_NAME} ${SOURCES})
    target_link_libraries(${PROJECT_NAME} ${GSL_LIBRARIES})
    set(LIBRARY_NAME ${PROJECT_NAME})
endif ()


add_executable(bem-model src/simulator_node.cpp)
target_link_libraries(bem-model ${LIBRARY_NAME})

add_executable(identify src/identify.cpp)
target_link_libraries(identify ${LIBRARY_NAME})

message(STATUS "===========   Done!   ==============")
