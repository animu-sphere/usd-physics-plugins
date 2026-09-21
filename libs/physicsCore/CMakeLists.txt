cmake_minimum_required(VERSION 3.25)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../VERSION")
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/../../VERSION"
    _physics_core_version LIMIT_COUNT 1)
  string(STRIP "${_physics_core_version}" _physics_core_version)
else()
  set(_physics_core_version "0.1.0")
endif()

project(physicsCore
  VERSION ${_physics_core_version}
  DESCRIPTION "Backend-neutral physics contracts"
  LANGUAGES CXX)

if(NOT DEFINED PHYSICSCORE_BUILD_TESTS)
  if(DEFINED USDPHYSICS_BUILD_TESTS)
    set(_physics_core_tests_default "${USDPHYSICS_BUILD_TESTS}")
  else()
    set(_physics_core_tests_default "${PROJECT_IS_TOP_LEVEL}")
  endif()
  option(PHYSICSCORE_BUILD_TESTS "Build physicsCore tests"
    ${_physics_core_tests_default})
endif()

add_library(physicsCore STATIC src/version.cpp)
add_library(physicsCore::physicsCore ALIAS physicsCore)
set_target_properties(physicsCore PROPERTIES
  EXPORT_NAME physicsCore
  POSITION_INDEPENDENT_CODE ON)
target_compile_features(physicsCore PUBLIC cxx_std_17)
target_compile_definitions(physicsCore PRIVATE
  USDPHYSICS_CORE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  USDPHYSICS_CORE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
  USDPHYSICS_CORE_VERSION_PATCH=${PROJECT_VERSION_PATCH})
target_include_directories(physicsCore PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>")

include(GNUInstallDirs)
install(TARGETS physicsCore
  EXPORT physicsCoreTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/physicsCoreConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/physicsCoreConfig.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/physicsCore")
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/physicsCoreConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMinorVersion)
install(EXPORT physicsCoreTargets
  FILE physicsCoreTargets.cmake
  NAMESPACE physicsCore::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/physicsCore")
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/physicsCoreConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/physicsCoreConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/physicsCore")

if(PHYSICSCORE_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
