cmake_minimum_required(VERSION 3.10)
project(labstor)

set(CMAKE_CXX_STANDARD 17)

add_executable(test_connect_exec test_connect.cc)
add_dependencies(test_connect_exec labstor_client)
target_link_libraries(test_connect_exec labstor_client)

add_test(NAME test_connect COMMAND
        bash ${CMAKE_CURRENT_SOURCE_DIR}/test_connect.sh
            ${CMAKE_BINARY_DIR}/labstor_runtime
            ${CMAKE_CURRENT_BINARY_DIR}/test_connect_exec
            ${CMAKE_SOURCE_DIR}/config/config.yaml)