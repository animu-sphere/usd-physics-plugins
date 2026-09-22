add_executable(physics_jolt_scaffold_test physics_jolt_scaffold_test.cpp)
target_link_libraries(physics_jolt_scaffold_test PRIVATE physicsJolt::physicsJolt)
add_test(NAME physicsJolt.scaffold COMMAND physics_jolt_scaffold_test)

if(PHYSICSJOLT_BACKEND_AVAILABLE)
  add_executable(physics_jolt_backend_test physics_jolt_backend_test.cpp)
  target_link_libraries(physics_jolt_backend_test PRIVATE physicsJolt::physicsJolt)
  add_test(NAME physicsJolt.backend COMMAND physics_jolt_backend_test)
endif()
