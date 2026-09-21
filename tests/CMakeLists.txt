add_test(NAME workspace.boundaries
  COMMAND "${CMAKE_COMMAND}"
    "-DPROJECT_ROOT=${PROJECT_SOURCE_DIR}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/check_boundaries.cmake")

add_test(NAME workspace.docs
  COMMAND "${CMAKE_COMMAND}"
    "-DPROJECT_ROOT=${PROJECT_SOURCE_DIR}"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/check_docs.cmake")

add_test(NAME workspace.installed_consumer
  COMMAND "${CMAKE_COMMAND}"
    "-DPROJECT_BUILD_DIR=${PROJECT_BINARY_DIR}"
    "-DPROJECT_SOURCE_ROOT=${PROJECT_SOURCE_DIR}"
    "-DCONSUMER_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/installed_consumer"
    "-DTEST_GENERATOR=${CMAKE_GENERATOR}"
    "-DTEST_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
    "-DTEST_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}"
    "-DTEST_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DTEST_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
    "-DTEST_CONFIG=$<CONFIG>"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/run_installed_consumer.cmake")
set_tests_properties(workspace.installed_consumer PROPERTIES
  LABELS "installed-consumer")
