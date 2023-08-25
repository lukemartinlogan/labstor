#------------------------------------------------------------------------------
# Build Small Message Task Library
#------------------------------------------------------------------------------
add_library(hermes_adapters SHARED
        hermes_adapters.cc)
add_dependencies(hermes_adapters ${Labstor_RUNTIME_DEPS})
target_link_libraries(hermes_adapters ${Labstor_RUNTIME_LIBRARIES})

#------------------------------------------------------------------------------
# Install Small Message Task Library
#------------------------------------------------------------------------------
install(
        TARGETS
        hermes_adapters
        LIBRARY DESTINATION ${LABSTOR_INSTALL_LIB_DIR}
        ARCHIVE DESTINATION ${LABSTOR_INSTALL_LIB_DIR}
        RUNTIME DESTINATION ${LABSTOR_INSTALL_BIN_DIR}
)

#------------------------------------------------------------------------------
# Coverage
#------------------------------------------------------------------------------
if(LABSTOR_ENABLE_COVERAGE)
    set_coverage_flags(hermes_adapters)
endif()
