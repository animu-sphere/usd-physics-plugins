cmake_minimum_required(VERSION 3.25)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../VERSION")
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/../../VERSION"
    _physics_jolt_version LIMIT_COUNT 1)
  string(STRIP "${_physics_jolt_version}" _physics_jolt_version)
else()
  set(_physics_jolt_version "0.1.0")
endif()

project(physicsJolt
  VERSION ${_physics_jolt_version}
  DESCRIPTION "Jolt backend package scaffold for physicsCore"
  LANGUAGES CXX)

if(NOT DEFINED PHYSICSJOLT_BUILD_TESTS)
  if(DEFINED USDPHYSICS_BUILD_TESTS)
    set(_physics_jolt_tests_default "${USDPHYSICS_BUILD_TESTS}")
  else()
    set(_physics_jolt_tests_default "${PROJECT_IS_TOP_LEVEL}")
  endif()
  option(PHYSICSJOLT_BUILD_TESTS "Build physicsJolt tests"
    ${_physics_jolt_tests_default})
endif()

if(NOT TARGET physicsCore::physicsCore)
  find_package(physicsCore CONFIG REQUIRED)
endif()

add_library(physicsJolt STATIC src/availability.cpp)
add_library(physicsJolt::physicsJolt ALIAS physicsJolt)
set_target_properties(physicsJolt PROPERTIES
  EXPORT_NAME physicsJolt
  POSITION_INDEPENDENT_CODE ON)
target_compile_features(physicsJolt PUBLIC cxx_std_17)
target_include_directories(physicsJolt PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(physicsJolt PUBLIC physicsCore::physicsCore)

include(GNUInstallDirs)
install(TARGETS physicsJolt
  EXPORT physicsJoltTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/physicsJoltConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/physicsJoltConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/physicsJolt")
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/physicsJoltConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMinorVersion)
install(EXPORT physicsJoltTargets
  FILE physicsJoltTargets.cmake
  NAMESPACE physicsJolt::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/physicsJolt")
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/physicsJoltConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/physicsJoltConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/physicsJolt")

if(PHYSICSJOLT_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
