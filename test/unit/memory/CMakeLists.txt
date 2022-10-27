cmake_minimum_required(VERSION 3.10)
project(labstor)

set(CMAKE_CXX_STANDARD 17)

add_executable(test_reserve_exec reserve.cc)
add_dependencies(test_reserve_exec labstor_client)
target_link_libraries(test_reserve_exec labstor_client)

add_test(NAME test_reserve COMMAND
        ${CMAKE_CURRENT_BINARY_DIR}/test_reserve_exec)