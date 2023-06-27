include_directories(${CMAKE_SOURCE_DIR})

#------------------------------------------------------------------------------
# External dependencies
#------------------------------------------------------------------------------
# None for now

#------------------------------------------------------------------------------
# Build Labstor Client Library
#------------------------------------------------------------------------------
include_directories(${HermesShm_INCLUDE_DIRS})
add_library(labstor_client SHARED
        ${CMAKE_CURRENT_SOURCE_DIR}/config_client.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/config_server.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/labstor_client.cc)
target_link_libraries(labstor_client
  ${HermesShm_LIBRARIES}
  yaml-cpp
  cereal::cereal
  -ldl -lrt -lc)

#------------------------------------------------------------------------------
# Build Labstor Runtime
#------------------------------------------------------------------------------
add_executable(labstor_runtime labstor_runtime.cc)
add_dependencies(labstor_runtime labstor_client)
target_link_libraries(labstor_runtime labstor_client -ldl -lrt -lc)

#------------------------------------------------------------------------------
# Build LabStor Finalization Function
#------------------------------------------------------------------------------
add_executable(finalize_labstor finalize_labstor.cc)
add_dependencies(finalize_labstor labstor_client)
target_link_libraries(finalize_labstor labstor_client)

#-----------------------------------------------------------------------------
# Add file(s) to CMake Install
#-----------------------------------------------------------------------------
install(
  FILES
    ${HERMES_HEADERS}
  DESTINATION
    ${HERMES_INSTALL_INCLUDE_DIR}
  COMPONENT
    headers
)

#-----------------------------------------------------------------------------
# Add Target(s) to CMake Install
#-----------------------------------------------------------------------------
install(
  TARGETS
    labstor_client
    labstor_runtime
    finalize_labstor
  LIBRARY DESTINATION ${LABSTOR_INSTALL_LIB_DIR}
  ARCHIVE DESTINATION ${LABSTOR_INSTALL_LIB_DIR}
  RUNTIME DESTINATION ${LABSTOR_INSTALL_BIN_DIR}
)

#------------------------------------------------------------------------------
# Coverage
#------------------------------------------------------------------------------
if(LABSTOR_ENABLE_COVERAGE)
  set_coverage_flags(labstor_client)
  set_coverage_flags(labstor_runtime)
  set_coverage_flags(finalize_labstor)
endif()