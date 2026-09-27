cmake_minimum_required(VERSION 3.25)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../VERSION")
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/../../VERSION" _physics_usd_version LIMIT_COUNT 1)
  string(STRIP "${_physics_usd_version}" _physics_usd_version)
else()
  set(_physics_usd_version "0.1.0")
endif()
project(physicsUsd VERSION ${_physics_usd_version} LANGUAGES CXX)

if(NOT TARGET physicsCore::physicsCore)
  find_package(physicsCore 0.1 CONFIG REQUIRED)
endif()
find_package(pxr CONFIG REQUIRED)
foreach(component IN ITEMS usd usdGeom usdPhysics)
  if(TARGET pxr::${component})
    list(APPEND _physics_usd_targets pxr::${component})
  elseif(TARGET ${component})
    list(APPEND _physics_usd_targets ${component})
  else()
    message(FATAL_ERROR "physicsUsd requires the OpenUSD ${component} target")
  endif()
endforeach()

add_library(physicsUsd STATIC src/box_scene.cpp)
add_library(physicsUsd::physicsUsd ALIAS physicsUsd)
set_target_properties(physicsUsd PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_compile_features(physicsUsd PUBLIC cxx_std_17)
target_include_directories(physicsUsd PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(physicsUsd PUBLIC physicsCore::physicsCore ${_physics_usd_targets})

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
install(TARGETS physicsUsd EXPORT physicsUsdTargets ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
configure_package_config_file(cmake/physicsUsdConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/physicsUsdConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/physicsUsd)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/physicsUsdConfigVersion.cmake"
  VERSION ${PROJECT_VERSION} COMPATIBILITY SameMinorVersion)
install(EXPORT physicsUsdTargets FILE physicsUsdTargets.cmake NAMESPACE physicsUsd::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/physicsUsd)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/physicsUsdConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/physicsUsdConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/physicsUsd)

set(_physics_usd_tests_default ${PROJECT_IS_TOP_LEVEL})
if(DEFINED USDPHYSICS_BUILD_TESTS)
  set(_physics_usd_tests_default ${USDPHYSICS_BUILD_TESTS})
endif()
option(PHYSICSUSD_BUILD_TESTS "Build standard USD declaration tests" ${_physics_usd_tests_default})
if(PHYSICSUSD_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
