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
  DESCRIPTION "Jolt Physics backend for physicsCore"
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

option(PHYSICSJOLT_REQUIRE_JOLT
  "Fail configuration when Jolt Physics is unavailable" OFF)
option(PHYSICSJOLT_ENABLE_SIX_DOF
  "Enable experimental six-DOF capability with a validated Jolt SDK" OFF)

if(NOT COMMAND physics_core_require_abi)
  message(FATAL_ERROR "physicsJolt requires rebuilt physicsCore package compatibility metadata")
endif()
get_target_property(PHYSICSJOLT_CORE_ABI_ID physicsCore::physicsCore USDPHYSICS_ABI_ID)

set(PHYSICSJOLT_BACKEND_TARGET "")
find_package(Jolt CONFIG QUIET)
if(TARGET Jolt::Jolt)
  set(PHYSICSJOLT_BACKEND_TARGET Jolt::Jolt)
elseif(TARGET Jolt)
  set(PHYSICSJOLT_BACKEND_TARGET Jolt)
endif()

if(PHYSICSJOLT_REQUIRE_JOLT AND NOT PHYSICSJOLT_BACKEND_TARGET)
  message(FATAL_ERROR
    "physicsJolt requires Jolt Physics 5.5.0, but no Jolt CMake target was found")
endif()
if(PHYSICSJOLT_ENABLE_SIX_DOF AND NOT PHYSICSJOLT_BACKEND_TARGET)
  message(FATAL_ERROR "Six-DOF execution requires an available Jolt SDK")
endif()

if(PHYSICSJOLT_BACKEND_TARGET)
  set(_physics_jolt_source src/jolt_world.cpp)
  set(_physics_jolt_backend_label "${PHYSICSJOLT_BACKEND_TARGET}")
  set(PHYSICSJOLT_BACKEND_AVAILABLE ON)
  set(PHYSICSJOLT_FIND_JOLT_DEPENDENCY "find_dependency(Jolt CONFIG)")
  get_target_property(PHYSICSJOLT_JOLT_INTERFACE_COMPILE_DEFINITIONS
    ${PHYSICSJOLT_BACKEND_TARGET} INTERFACE_COMPILE_DEFINITIONS)
  if(PHYSICSJOLT_JOLT_INTERFACE_COMPILE_DEFINITIONS MATCHES "-NOTFOUND$")
    set(PHYSICSJOLT_JOLT_INTERFACE_COMPILE_DEFINITIONS "")
  endif()
else()
  set(_physics_jolt_source src/availability.cpp)
  set(_physics_jolt_backend_label "unavailable")
  set(PHYSICSJOLT_BACKEND_AVAILABLE OFF)
  set(PHYSICSJOLT_FIND_JOLT_DEPENDENCY "")
  set(PHYSICSJOLT_JOLT_INTERFACE_COMPILE_DEFINITIONS "")
endif()

if(PHYSICSJOLT_ENABLE_SIX_DOF AND NOT
   "JPH_CROSS_PLATFORM_DETERMINISTIC" IN_LIST PHYSICSJOLT_JOLT_INTERFACE_COMPILE_DEFINITIONS)
  message(FATAL_ERROR "Six-DOF execution requires a deterministic Jolt SDK")
endif()

add_library(physicsJolt STATIC ${_physics_jolt_source})
add_library(physicsJolt::physicsJolt ALIAS physicsJolt)
set_target_properties(physicsJolt PROPERTIES
  EXPORT_NAME physicsJolt
  POSITION_INDEPENDENT_CODE ON)
target_compile_features(physicsJolt PUBLIC cxx_std_17)
target_compile_definitions(physicsJolt PRIVATE
  "$<$<BOOL:${PHYSICSJOLT_ENABLE_SIX_DOF}>:USDPHYSICS_JOLT_SIX_DOF>")
target_include_directories(physicsJolt PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(physicsJolt PUBLIC physicsCore::physicsCore)
if(PHYSICSJOLT_BACKEND_TARGET)
  target_link_libraries(physicsJolt PRIVATE ${PHYSICSJOLT_BACKEND_TARGET})
endif()
message(STATUS "physicsJolt: backend ${_physics_jolt_backend_label}")

include(GNUInstallDirs)
install(TARGETS physicsJolt
  EXPORT physicsJoltTargets
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/UsdPhysicsInstallPackage.cmake")
usdphysics_install_package(physicsJolt)

if(PHYSICSJOLT_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
