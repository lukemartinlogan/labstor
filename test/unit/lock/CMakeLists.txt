cmake_minimum_required(VERSION 3.10)
project(labstor)

set(CMAKE_CXX_STANDARD 17)

add_executable(test_mutex_exec
        main_thread.cc
        lock.cc)
add_dependencies(test_mutex_exec labstor_data_structures)
target_link_libraries(test_mutex_exec
        labstor_data_structures Catch2::Catch2 MPI::MPI_CXX OpenMP::OpenMP_CXX)

add_test(NAME test_mutex COMMAND
        ${CMAKE_CURRENT_BINARY_DIR}/test_mutex_exec "Mutex")

add_test(NAME test_rw_lock COMMAND
        ${CMAKE_CURRENT_BINARY_DIR}/test_mutex_exec "RwLock")

