cmake_minimum_required(VERSION 3.10)
project(labstor)

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR}/include)

option(BUILD_SHARED_LIBS "Build shared libraries (.dll/.so) instead of static ones (.lib/.a)" ON)

add_library(labstor_client
        src/ipc_manager.cc
        src/singleton.cc)
target_link_libraries(labstor_client yaml-cpp pthread)

add_executable(labstor_runtime
        src/runtime/runtime.cc
        src/ipc_manager.cc
        src/singleton.cc)
target_link_libraries(labstor_runtime yaml-cpp pthread)

add_custom_target(lint COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/lint.sh ${CMAKE_SOURCE_DIR})

######MODULES & TESTS
enable_testing()
add_subdirectory(test)
