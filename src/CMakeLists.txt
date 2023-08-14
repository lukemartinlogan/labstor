#------------------------------------------------------------------------------
# External dependencies
#------------------------------------------------------------------------------
# None for now

#------------------------------------------------------------------------------
# Build Labstor Client Library
#------------------------------------------------------------------------------
add_library(labstor_client SHARED
        ${CMAKE_CURRENT_SOURCE_DIR}/config_client.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/config_server.cc
        ${CMAKE_CURRENT_SOURCE_DIR}/labstor_client.cc)
target_link_libraries(labstor_client
  ${HermesShm_LIBRARIES}
  yaml-cpp
  cereal::cereal
  thallium
  -ldl -lrt -lc -pthread)

#------------------------------------------------------------------------------
# Build Labstor Runtime Library
#------------------------------------------------------------------------------
add_library(labstor_runtime
        rpc.cc
        worker.cc
        labstor_runtime.cc)
add_dependencies(labstor_runtime labstor_client)
target_link_libraries(labstor_runtime labstor_client thallium -ldl -lrt -lc)

#------------------------------------------------------------------------------
# Build Labstor Runtime Start Function
#------------------------------------------------------------------------------
add_executable(labstor_start_runtime
        labstor_start_runtime.cc)
add_dependencies(labstor_start_runtime labstor_runtime)
target_link_libraries(labstor_start_runtime labstor_runtime)

#------------------------------------------------------------------------------
# Build LabStor Runtime Stop Function
#------------------------------------------------------------------------------
add_executable(labstor_stop_runtime labstor_stop_runtime.cc)
add_dependencies(labstor_stop_runtime labstor_client)
target_link_libraries(labstor_stop_runtime labstor_client)

#-----------------------------------------------------------------------------
# Add file(s) to CMake Install
#-----------------------------------------------------------------------------
install(
  FILES
    ${LABSTOR_HEADERS}
  DESTINATION
    ${LABSTOR_INSTALL_INCLUDE_DIR}
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
    labstor_start_runtime
    labstor_stop_runtime
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
  set_coverage_flags(labstor_start_runtime)
  set_coverage_flags(labstor_stop_runtime)
endif()