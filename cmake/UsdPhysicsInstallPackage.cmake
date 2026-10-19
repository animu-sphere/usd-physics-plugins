# Shared config/version/export boilerplate for the independent static packages.
# Targets, headers, dependencies and compatibility identities stay component-owned.
include_guard(GLOBAL)
include(CMakePackageConfigHelpers)

function(usdphysics_install_package package)
  set(_destination "${CMAKE_INSTALL_LIBDIR}/cmake/${package}")
  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${package}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${package}Config.cmake"
    INSTALL_DESTINATION "${_destination}"
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR)
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${package}ConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMinorVersion)
  install(EXPORT ${package}Targets
    FILE ${package}Targets.cmake
    NAMESPACE ${package}::
    DESTINATION "${_destination}")
  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${package}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${package}ConfigVersion.cmake"
    DESTINATION "${_destination}")
endfunction()
