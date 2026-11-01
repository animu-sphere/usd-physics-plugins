# The physicsJolt sources with six-DOF on, as a library of its own. It needs the
# deterministic Jolt SDK, a different ABI set, so it never replaces physicsJolt or
# its pins. It installs the same CMake package: a consumer pins one of the two.
# See docs/architecture/DEPENDENCIES.md#3-jolt-physics.
cmake_minimum_required(VERSION 3.25)
project(physicsJoltSixDofLibrary LANGUAGES NONE)
enable_testing()
set(PHYSICSJOLT_REQUIRE_JOLT ON CACHE BOOL "Selected by the physicsJoltSixDof package")
set(PHYSICSJOLT_ENABLE_SIX_DOF ON CACHE BOOL "Selected by the physicsJoltSixDof package")
set(PHYSICSJOLT_BUILD_TESTS ON CACHE BOOL "Selected by the physicsJoltSixDof package")
add_subdirectory(../physicsJolt physicsJolt)
