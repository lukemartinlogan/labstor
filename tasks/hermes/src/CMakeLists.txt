#------------------------------------------------------------------------------
# Build Small Message Task Library
#------------------------------------------------------------------------------
add_library(hermes_config_manager SHARED
        config_manager.cc)
add_dependencies(hermes_config_manager ${Labstor_RUNTIME_DEPS})
target_link_libraries(hermes_config_manager ${Labstor_RUNTIME_LIBRARIES})

#------------------------------------------------------------------------------
# Install Small Message Task Library
#------------------------------------------------------------------------------
install(
        TARGETS
        hermes_config_manager
        LIBRARY DESTINATION ${LABSTOR_INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${LABSTOR_INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${LABSTOR_INSTALL_BIN_DIR}
)

#------------------------------------------------------------------------------
# Coverage
#------------------------------------------------------------------------------
if(LABSTOR_ENABLE_COVERAGE)
    set_coverage_flags(hermes_config_manager)
endif()
