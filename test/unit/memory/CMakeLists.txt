cmake_minimum_required(VERSION 3.10)
project(labstor)

set(CMAKE_CXX_STANDARD 17)

add_executable(test_reserve_exec reserve.cc)
add_dependencies(test_reserve_exec labstor_client)
target_link_libraries(test_reserve_exec labstor_client)

add_executable(test_memory_slots_exec memory_slots.cc)
add_dependencies(test_memory_slots_exec labstor_client)
target_link_libraries(test_memory_slots_exec labstor_client MPI::MPI_CXX)


add_test(NAME test_memory_slots COMMAND
        mpirun -n 2 ${CMAKE_CURRENT_BINARY_DIR}/test_memory_slots_exec)

add_test(NAME test_reserve COMMAND
        ${CMAKE_CURRENT_BINARY_DIR}/test_reserve_exec)