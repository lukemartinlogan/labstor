#------------------------------------------------------------------------------
# Build Small Message Task Library
#------------------------------------------------------------------------------
add_library(posix_bdev SHARED
        posix_bdev.cc)
add_dependencies(posix_bdev ${Labstor_RUNTIME_DEPS})
target_link_libraries(posix_bdev ${Labstor_RUNTIME_LIBRARIES})

#------------------------------------------------------------------------------
# Install Small Message Task Library
#------------------------------------------------------------------------------
install(
        TARGETS
        posix_bdev
        LIBRARY DESTINATION ${LABSTOR_INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${LABSTOR_INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${LABSTOR_INSTALL_BIN_DIR}
)

#------------------------------------------------------------------------------
# Coverage
#------------------------------------------------------------------------------
if(LABSTOR_ENABLE_COVERAGE)
    set_coverage_flags(posix_bdev)
endif()
