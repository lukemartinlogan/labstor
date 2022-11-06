cmake_minimum_required(VERSION 3.10)
project(labstor)

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR}/include)

option(BUILD_SHARED_LIBS "Build shared libraries (.dll/.so) instead of static ones (.lib/.a)" ON)
option(BUILD_MPI_TESTS "Build tests which depend on MPI" ON)
option(BUILD_OpenMP_TESTS "Build tests which depend on OpenMP" ON)

##################REQUIRED EXTERNAL LIBRARIES

#YAML-CPP
find_package(yaml-cpp REQUIRED)
if(yaml-cpp_FOUND)
    message(STATUS "found yaml-cpp at ${yaml-cpp_DIR}")
endif()

#MPICH
if(BUILD_MPI_TESTS)
    find_package(MPI REQUIRED COMPONENTS C CXX)
    message(STATUS "found mpi.h at ${MPI_CXX_INCLUDE_DIRS}")
endif()

#OpenMP
if(BUILD_OpenMP_TESTS)
    find_package(OpenMP REQUIRED COMPONENTS C CXX)
    message(STATUS "found omp.h at ${OpenMP_CXX_INCLUDE_DIRS}")
endif()

##################Build LabStor main packages

add_library(labstor_thread
        src/thread/mutex.cc)

add_library(labstor_data_structures
        src/memory/page_allocator.cc)
add_dependencies(labstor_data_structures
        labstor_thread)
target_link_libraries(labstor_data_structures
        labstor_thread)

add_library(labstor_client
        src/ipc_manager.cc
        src/singleton.cc
        src/memory/memory_manager.cc)
add_dependencies(labstor_client
        labstor_data_structures)
target_link_libraries(labstor_client
        labstor_data_structures)
target_link_libraries(labstor_client yaml-cpp pthread -lrt -ldl)

add_executable(labstor_runtime
        src/runtime/runtime.cc
        src/ipc_manager.cc
        src/singleton.cc
        src/memory/memory_manager.cc)
add_dependencies(labstor_runtime
        labstor_data_structures)
target_link_libraries(labstor_runtime
        labstor_data_structures)
target_link_libraries(labstor_runtime yaml-cpp pthread -lrt -ldl)

add_custom_target(lint COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/lint.sh ${CMAKE_SOURCE_DIR})

add_custom_target(preamble COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/preamble.py ${CMAKE_SOURCE_DIR})

##################MODULES & TESTS
enable_testing()
add_subdirectory(test)
