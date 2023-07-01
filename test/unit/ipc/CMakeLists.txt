cmake_minimum_required(VERSION 3.10)
project(labstor)

set(CMAKE_CXX_STANDARD 17)

#------------------------------------------------------------------------------
# Build Tests
#------------------------------------------------------------------------------

add_executable(test_messages_exec
        ${TEST_MAIN}/main.cc
        test_init.cc
        test_ipc.cc
)

add_dependencies(test_messages_exec labstor_client)
target_link_libraries(test_messages_exec
        labstor_client Catch2::Catch2)

#------------------------------------------------------------------------------
# Test Cases
#------------------------------------------------------------------------------

# STRING TESTS
add_test(NAME test_ipc COMMAND
        ${CMAKE_BINARY_DIR}/bin/test_messages "TestIpc")

#------------------------------------------------------------------------------
# Install Targets
#------------------------------------------------------------------------------
install(TARGETS
        test_messages_exec
        EXPORT
        ${HERMES_EXPORTED_TARGETS}
        LIBRARY DESTINATION ${HERMES_INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${HERMES_INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${HERMES_INSTALL_BIN_DIR})

#-----------------------------------------------------------------------------
# Coverage
#-----------------------------------------------------------------------------
if(HERMES_ENABLE_COVERAGE)
    set_coverage_flags(test_messages)
endif()
