add_executable(physics_usd_box_scene_test box_scene_test.cpp)
target_link_libraries(physics_usd_box_scene_test PRIVATE physicsUsd::physicsUsd)
add_test(NAME physicsUsd.box_scene COMMAND physics_usd_box_scene_test)
if(WIN32)
  set_tests_properties(physicsUsd.box_scene PROPERTIES ENVIRONMENT_MODIFICATION
    "PATH=path_list_prepend:${pxr_DIR}/bin;PATH=path_list_prepend:${pxr_DIR}/lib;PATH=path_list_prepend:${Python3_RUNTIME_LIBRARY_DIRS}")
endif()
