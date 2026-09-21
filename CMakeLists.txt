cmake_minimum_required(VERSION 3.25)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION"
  _usd_physics_plugins_version LIMIT_COUNT 1)
string(STRIP "${_usd_physics_plugins_version}" _usd_physics_plugins_version)

project(UsdPhysicsPlugins
  VERSION ${_usd_physics_plugins_version}
  DESCRIPTION "Backend-neutral physics runtime components for OpenUSD applications"
  LANGUAGES CXX)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_property(_usd_physics_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT _usd_physics_multi_config AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(USDPHYSICS_BUILD_TESTS "Build usd-physics-plugins tests" ${PROJECT_IS_TOP_LEVEL})
if(USDPHYSICS_BUILD_TESTS)
  enable_testing()
endif()

add_subdirectory(libs/physicsCore)
add_subdirectory(backends/physicsJolt)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/UsdPhysicsBoundaryChecks.cmake")
usdphysics_assert_target_dependencies(
  TARGET physicsCore
  ALLOWED_PUBLIC)
usdphysics_assert_target_dependencies(
  TARGET physicsJolt
  ALLOWED_PUBLIC physicsCore::physicsCore
  ALLOWED_PRIVATE physicsCore::physicsCore)

if(USDPHYSICS_BUILD_TESTS)
  add_subdirectory(tests)
endif()
