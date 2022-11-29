cmake_minimum_required(VERSION 3.10)
project(labstor)

set(CMAKE_CXX_STANDARD 17)

add_executable(test_data_structure_exec
        main_mpi.cc
        string.cc
        vector.cc
        list.cc)

add_dependencies(test_data_structure_exec labstor_client)
target_link_libraries(test_data_structure_exec
        labstor_client Catch2::Catch2 MPI::MPI_CXX)

# 1p - 1 process, 1t - 1 thread
add_test(NAME test_vector_1p1t COMMAND
        ${CMAKE_CURRENT_BINARY_DIR}/test_data_structure_exec "VectorSimple")