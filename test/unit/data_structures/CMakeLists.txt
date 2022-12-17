cmake_minimum_required(VERSION 3.10)
project(labstor)

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR}/test/unit)
add_subdirectory(allocators)
add_subdirectory(backend)
add_subdirectory(containers)
add_subdirectory(lock)