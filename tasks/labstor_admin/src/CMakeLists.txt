#------------------------------------------------------------------------------
# Build Labstor Admin Task Library
#------------------------------------------------------------------------------
add_library(labstor_admin SHARED
        ${CMAKE_CURRENT_SOURCE_DIR}/labstor_admin.cc)
add_dependencies(labstor_admin labstor_runtime)
target_link_libraries(labstor_admin labstor_runtime)

#------------------------------------------------------------------------------
# Install Labstor Admin Task Library
#------------------------------------------------------------------------------
install(
        TARGETS
        labstor_admin
        LIBRARY DESTINATION ${LABSTOR_INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${LABSTOR_INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${LABSTOR_INSTALL_BIN_DIR}
)

#------------------------------------------------------------------------------
# Coverage
#------------------------------------------------------------------------------
if(LABSTOR_ENABLE_COVERAGE)
    set_coverage_flags(labstor_admin)
endif()
