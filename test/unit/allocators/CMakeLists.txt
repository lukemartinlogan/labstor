cmake_minimum_required(VERSION 3.10)
project(labstor)

set(CMAKE_CXX_STANDARD 17)

add_executable(test_allocator_exec
        main_thread.cc
        allocator.cc)
add_dependencies(test_allocator_exec labstor_client)
target_link_libraries(test_allocator_exec
        labstor_client Catch2::Catch2 MPI::MPI_CXX)

add_test(NAME test_page_allocator COMMAND
        ${CMAKE_CURRENT_BINARY_DIR}/test_allocator_exec "PageAllocator")