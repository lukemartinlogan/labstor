cmake_minimum_required(VERSION 3.10)
project(labstor)

#-----------------------------------------------------------------------------
# Define Options
#-----------------------------------------------------------------------------
option(BUILD_SHARED_LIBS "Build shared libraries (.dll/.so) instead of static ones (.lib/.a)" ON)
option(BUILD_MPI_TESTS "Build tests which depend on MPI" ON)
option(BUILD_OpenMP_TESTS "Build tests which depend on OpenMP" ON)
option(LABSTOR_ENABLE_COVERAGE "Check how well tests cover code" OFF)
option(LABSTOR_ENABLE_DOXYGEN "Check how well the code is documented" OFF)

#-----------------------------------------------------------------------------
# Compiler Optimization
#-----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("IN DEBUG MODE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
    add_compile_definitions(HERMES_LOG_VERBOSITY=10)
else()
    message("IN RELEASE MODE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O3")
    add_compile_definitions(HERMES_LOG_VERBOSITY=1)
endif()

#-----------------------------------------------------------------------------
# Find Packages
#-----------------------------------------------------------------------------
# HermesShm
find_package(HermesShm CONFIG REQUIRED)
message(STATUS "found hermes_shm.h at ${HermesShm_INCLUDE_DIRS}")

# YAML-CPP
find_package(yaml-cpp REQUIRED)
message(STATUS "found yaml-cpp at ${yaml-cpp_DIR}")

# Catch2
find_package(Catch2 3.0.1 REQUIRED)
message(STATUS "found catch2.h at ${Catch2_CXX_INCLUDE_DIRS}")

# MPICH
if(BUILD_MPI_TESTS)
    find_package(MPI REQUIRED COMPONENTS C CXX)
    message(STATUS "found mpi.h at ${MPI_CXX_INCLUDE_DIRS}")
endif()

# OpenMP
if(BUILD_OpenMP_TESTS)
    find_package(OpenMP REQUIRED COMPONENTS C CXX)
    message(STATUS "found omp.h at ${OpenMP_CXX_INCLUDE_DIRS}")
endif()

# Cereal
find_package(cereal REQUIRED)
if(cereal)
    message(STATUS "found cereal")
endif()

# Pkg-Config
find_package(PkgConfig REQUIRED)
if(PkgConfig)
    message(STATUS "found pkg config")
endif()

# Zeromq
pkg_check_modules(ZMQ REQUIRED libzmq)
include_directories(${ZeroMQ_INCLUDE_DIRS})

#------------------------------------------------------------------------------
# Setup CMake Environment
#------------------------------------------------------------------------------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
        ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all Executables.")
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
        ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all Libraries")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
        ${PROJECT_BINARY_DIR}/bin CACHE PATH "Single Directory for all static libraries.")

#------------------------------------------------------------------------------
# Setup install and output Directories
#------------------------------------------------------------------------------
if(NOT LABSTOR_INSTALL_BIN_DIR)
    set(LABSTOR_INSTALL_BIN_DIR ${CMAKE_INSTALL_PREFIX}/bin)
endif()
if(NOT LABSTOR_INSTALL_LIB_DIR)
    set(LABSTOR_INSTALL_LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib)
endif()
if(NOT LABSTOR_INSTALL_INCLUDE_DIR)
    set(LABSTOR_INSTALL_INCLUDE_DIR ${CMAKE_INSTALL_PREFIX}/include)
endif()
if(NOT LABSTOR_INSTALL_DATA_DIR)
    set(LABSTOR_INSTALL_DATA_DIR ${CMAKE_INSTALL_PREFIX}/share)
endif()

#-----------------------------------------------------------------------------
# Build Labstor Main Packages
#-----------------------------------------------------------------------------
# Main includes
include_directories(${HermesShm_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/include)
# Task includes
include_directories(${CMAKE_SOURCE_DIR}/tasks)
include_directories(${CMAKE_SOURCE_DIR}/tasks/labstor_admin/include)
include_directories(${CMAKE_SOURCE_DIR}/tasks/small_message/include)
include_directories(${CMAKE_SOURCE_DIR}/tasks/small_message/include)
include_directories(${CMAKE_SOURCE_DIR}/tasks/hermes/include)
include_directories(${CMAKE_SOURCE_DIR}/tasks/bdev/include)
include_directories(${CMAKE_SOURCE_DIR}/tasks/ram_bdev/include)
include_directories(${CMAKE_SOURCE_DIR}/tasks/posix_bdev/include)
include_directories(${CMAKE_SOURCE_DIR}/tasks/hermes_mdm/include)
include_directories(${CMAKE_SOURCE_DIR}/tasks/hermes_stats/include)
include_directories(${CMAKE_SOURCE_DIR}/tasks/hermes_adapters/include)
# Test includes
include_directories(${CMAKE_SOURCE_DIR}/test/unit)

set(TEST_MAIN ${CMAKE_SOURCE_DIR}/test/unit)
add_subdirectory(src)
add_subdirectory(tasks)
add_subdirectory(benchmark)
add_custom_target(lint COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/lint.sh ${CMAKE_SOURCE_DIR})

#-----------------------------------------------------------------------------
# Build + Enable Testing
#-----------------------------------------------------------------------------
enable_testing()
add_subdirectory(test)

#-----------------------------------------------------------------------------
# Install LabStor Headers
#-----------------------------------------------------------------------------
install(DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX})

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/CMake/LabstorConfig.cmake
        ${PROJECT_BINARY_DIR}/CMakeFiles/LabstorConfig.cmake @ONLY
)

install(
        FILES
        ${PROJECT_BINARY_DIR}/CMakeFiles/HermesShmConfig.cmake
        DESTINATION
        ${CMAKE_INSTALL_PREFIX}/cmake
)