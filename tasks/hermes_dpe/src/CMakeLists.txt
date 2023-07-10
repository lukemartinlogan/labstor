#------------------------------------------------------------------------------
# Build Small Message Task Library
#------------------------------------------------------------------------------
add_library(hermes_dpe SHARED
        hermes_dpe.cc)
add_dependencies(hermes_dpe labstor_runtime)
target_link_libraries(hermes_dpe labstor_runtime)

#------------------------------------------------------------------------------
# Install Small Message Task Library
#------------------------------------------------------------------------------
install(
        TARGETS
        hermes_dpe
        LIBRARY DESTINATION ${LABSTOR_INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${LABSTOR_INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${LABSTOR_INSTALL_BIN_DIR}
)

#------------------------------------------------------------------------------
# Coverage
#------------------------------------------------------------------------------
if(LABSTOR_ENABLE_COVERAGE)
    set_coverage_flags(hermes_dpe)
endif()
