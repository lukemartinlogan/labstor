#------------------------------------------------------------------------------
# Build Small Message Task Library
#------------------------------------------------------------------------------
include_directories(myapp PRIVATE ${libfabric_INCLUDE_DIRS})
link_directories(${libfabric_LIBRARY_DIRS})
add_library(remote_queue SHARED
        remote_queue.cc)
add_dependencies(remote_queue labstor_runtime)
target_link_libraries(remote_queue
        labstor_runtime ${libfabric_LIBRARIES})

#------------------------------------------------------------------------------
# Install Small Message Task Library
#------------------------------------------------------------------------------
install(
        TARGETS
        remote_queue
        LIBRARY DESTINATION ${LABSTOR_INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${LABSTOR_INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${LABSTOR_INSTALL_BIN_DIR}
)

#------------------------------------------------------------------------------
# Coverage
#------------------------------------------------------------------------------
if(LABSTOR_ENABLE_COVERAGE)
    set_coverage_flags(remote_queue)
endif()
