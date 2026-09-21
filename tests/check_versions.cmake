file(READ "${PROJECT_ROOT}/VERSION" _version)
string(STRIP "${_version}" _version)

file(READ "${PROJECT_ROOT}/openstrata.toml" _workspace_manifest)
string(REGEX MATCH "version[ \t]*=[ \t]*\"([^\"]+)\""
  _workspace_version_match "${_workspace_manifest}")
if(NOT CMAKE_MATCH_1 STREQUAL _version)
  message(FATAL_ERROR
    "openstrata.toml version '${CMAKE_MATCH_1}' does not match VERSION '${_version}'")
endif()

foreach(_manifest IN ITEMS
    "${PROJECT_ROOT}/libs/physicsCore/openstrata.library.yaml"
    "${PROJECT_ROOT}/backends/physicsJolt/openstrata.library.yaml")
  file(READ "${_manifest}" _content)
  string(REGEX MATCH "version:[ \t]*\"?([^\" \t\r\n]+)"
    _library_version_match "${_content}")
  if(NOT CMAKE_MATCH_1 STREQUAL _version)
    message(FATAL_ERROR
      "${_manifest} version '${CMAKE_MATCH_1}' does not match VERSION '${_version}'")
  endif()
endforeach()

message(STATUS "Workspace and library versions match VERSION ${_version}")
