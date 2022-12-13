cmake_minimum_required(VERSION 3.10)
project(labstor)

set(CMAKE_CXX_STANDARD 17)

##################Build LabStor main packages

add_library(labstor_data_structures
        thread/mutex.cc
        thread/rwlock.cc
        memory/page_allocator.cc
        memory/memory_manager.cc
        data_structure_singleton.cc)
target_link_libraries(labstor_data_structures
        yaml-cpp pthread -lrt -ldl)

add_library(labstor_client
        ipc_manager.cc
        singleton.cc)
add_dependencies(labstor_client
        labstor_data_structures)
target_link_libraries(labstor_client
        labstor_data_structures)
target_link_libraries(labstor_client yaml-cpp pthread -lrt -ldl)

add_executable(labstor_runtime
        runtime/runtime.cc
        ipc_manager.cc
        singleton.cc)
add_dependencies(labstor_runtime
        labstor_data_structures)
target_link_libraries(labstor_runtime
        labstor_data_structures)
target_link_libraries(labstor_runtime yaml-cpp pthread -lrt -ldl)

##################Install LabStor
install(
        TARGETS labstor_data_structures labstor_client
        DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(
        TARGETS labstor_runtime
        DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX})
