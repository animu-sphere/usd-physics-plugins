cmake_minimum_required(VERSION 3.25)
project(UsdPhysicsInstalledConsumer LANGUAGES CXX)

if(TEST_FAKE_INCOMPATIBLE_JOLT)
  add_library(Jolt::Jolt INTERFACE IMPORTED)
  set_property(TARGET Jolt::Jolt PROPERTY INTERFACE_COMPILE_DEFINITIONS
    USD_PHYSICS_TEST_INCOMPATIBLE_JOLT_ABI=1)
endif()

find_package(physicsCore CONFIG REQUIRED)
find_package(physicsJolt CONFIG REQUIRED)
if(NOT DEFINED physicsJolt_BACKEND_AVAILABLE)
  message(FATAL_ERROR "physicsJolt package must report backend availability")
endif()
if(NOT "${physicsJolt_BACKEND_AVAILABLE}" STREQUAL
       "${EXPECT_PHYSICSJOLT_BACKEND_AVAILABLE}")
  message(FATAL_ERROR
    "physicsJolt backend metadata does not match the installed package")
endif()

add_executable(usd_physics_installed_consumer main.cpp)
target_compile_features(usd_physics_installed_consumer PRIVATE cxx_std_17)
target_link_libraries(usd_physics_installed_consumer PRIVATE
  physicsCore::physicsCore
  physicsJolt::physicsJolt)

enable_testing()
add_test(NAME installed_consumer.runs COMMAND usd_physics_installed_consumer)
