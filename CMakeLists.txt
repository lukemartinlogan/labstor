cmake_minimum_required(VERSION 3.10)
project(labstor)

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR}/include)

###################OPTIONS
option(BUILD_SHARED_LIBS "Build shared libraries (.dll/.so) instead of static ones (.lib/.a)" ON)
option(BUILD_MPI_TESTS "Build tests which depend on MPI" ON)
option(BUILD_OpenMP_TESTS "Build tests which depend on OpenMP" ON)
option(LABSTOR_RPC_THALLIUM "Enable compatability for mochi" ON)
option(LABSTOR_ENABLE_COVERAGE "Check how well tests cover code" ON)
option(LABSTOR_ENABLE_DOXYGEN "Check how well the code is documented" ON)

##################CMAKE MODULES
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

##################OPTIMIZATION
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "-O3")
endif()

##################REQUIRED EXTERNAL LIBRARIES

# YAML-CPP
find_package(yaml-cpp REQUIRED)
message(STATUS "found yaml-cpp at ${yaml-cpp_DIR}")

# Catch2
find_package(Catch2 3.0.1 REQUIRED)
message(STATUS "found catch2.h at ${Catch2_CXX_INCLUDE_DIRS}")

# MPICH
if(BUILD_MPI_TESTS)
    find_package(MPI REQUIRED COMPONENTS C CXX)
    message(STATUS "found mpi.h at ${MPI_CXX_INCLUDE_DIRS}")
endif()

# OpenMP
if(BUILD_OpenMP_TESTS)
    find_package(OpenMP REQUIRED COMPONENTS C CXX)
    message(STATUS "found omp.h at ${OpenMP_CXX_INCLUDE_DIRS}")
endif()

# thallium
if(LABSTOR_RPC_THALLIUM)
    find_package(thallium CONFIG REQUIRED)
    if(thallium_FOUND)
        message(STATUS "found thallium at ${thallium_DIR}")
    endif()
endif()

##################CODE COVERAGE
if(LABSTOR_ENABLE_COVERAGE)
    set(COVERAGE_FLAGS "-fprofile-arcs -ftest-coverage" CACHE STRING
        "Flags to the coverage program to perform coverage inspection")
    mark_as_advanced(COVERAGE_FLAGS)

    macro(set_coverage_flags target)
        set_target_properties(${target}
                PROPERTIES
                COMPILE_FLAGS ${COVERAGE_FLAGS}
                LINK_FLAGS ${COVERAGE_FLAGS}
                )
    endmacro()
endif()

##################DOCUMENTATION
if(LABSTOR_ENABLE_DOXYGEN)
    include(UseDoxygenDoc)

    add_doxygen_doc(
            BUILD_DIR
            ${CMAKE_CURRENT_BINARY_DIR}/_build
            DOXY_FILE
            ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile.in
            TARGET_NAME
            dox
            COMMENT
            "HTML documentation"
    )
endif()

##################Build LabStor main packages

add_subdirectory(src)

add_custom_target(lint COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/lint.sh ${CMAKE_SOURCE_DIR})
add_custom_target(preamble COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/preamble.py ${CMAKE_SOURCE_DIR})

##################MODULES & TESTS
enable_testing()
add_subdirectory(test)

##################Install LabStor
install(DIRECTORY include DESTINATION ${CMAKE_INSTALL_PREFIX})
