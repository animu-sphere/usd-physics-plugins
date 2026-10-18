if(NOT TEST_CONFIG)
  set(TEST_CONFIG Release)
endif()

set(_prefix "${PROJECT_BUILD_DIR}/installed-consumer-prefix")
set(_build "${PROJECT_BUILD_DIR}/installed-consumer-build")
set(_mismatch_build "${PROJECT_BUILD_DIR}/incompatible-jolt-consumer-build")
set(_expected_backend_available OFF)
if(TEST_JOLT_DIR)
  set(_expected_backend_available ON)
endif()
file(REMOVE_RECURSE "${_prefix}" "${_build}" "${_mismatch_build}")

execute_process(
  COMMAND "${CMAKE_COMMAND}" --install "${PROJECT_BUILD_DIR}"
    --prefix "${_prefix}" --config "${TEST_CONFIG}"
  RESULT_VARIABLE _install_result)
if(_install_result)
  message(FATAL_ERROR "install step failed: ${_install_result}")
endif()

file(GLOB_RECURSE _installed_metadata "${_prefix}/*.cmake")
file(TO_CMAKE_PATH "${PROJECT_SOURCE_ROOT}/libs/physicsCore" _core_source)
file(TO_CMAKE_PATH "${PROJECT_SOURCE_ROOT}/backends/physicsJolt" _jolt_source)
file(TO_CMAKE_PATH "${PROJECT_BUILD_DIR}/libs/physicsCore" _core_build)
file(TO_CMAKE_PATH "${PROJECT_BUILD_DIR}/backends/physicsJolt" _jolt_build)
foreach(_metadata IN LISTS _installed_metadata)
  file(READ "${_metadata}" _content)
  string(REPLACE "\\" "/" _content "${_content}")
  foreach(_forbidden IN ITEMS
      "${_core_source}" "${_jolt_source}" "${_core_build}" "${_jolt_build}"
      "${PROJECT_SOURCE_ROOT}/libs/physicsUsd" "${PROJECT_BUILD_DIR}/libs/physicsUsd")
    string(FIND "${_content}" "${_forbidden}" _position)
    if(NOT _position EQUAL -1)
      message(FATAL_ERROR
        "installed metadata leaks '${_forbidden}': ${_metadata}")
    endif()
  endforeach()
endforeach()

set(_configure_args
  -S "${CONSUMER_SOURCE_DIR}"
  -B "${_build}"
  -G "${TEST_GENERATOR}"
  "-DCMAKE_BUILD_TYPE=${TEST_CONFIG}"
  "-DEXPECT_PHYSICSJOLT_BACKEND_AVAILABLE=${_expected_backend_available}"
  "-DCMAKE_PREFIX_PATH=${_prefix}")
if(TEST_GENERATOR_PLATFORM)
  list(APPEND _configure_args -A "${TEST_GENERATOR_PLATFORM}")
endif()
if(TEST_USD)
  list(APPEND _configure_args "-DTEST_USD=ON" "-Dpxr_DIR=${TEST_PXR_DIR}"
    "-DCMAKE_PREFIX_PATH=${_prefix}\;${TEST_PXR_DIR}")
endif()
if(TEST_MAKE_PROGRAM)
  list(APPEND _configure_args "-DCMAKE_MAKE_PROGRAM=${TEST_MAKE_PROGRAM}")
endif()
if(TEST_CXX_COMPILER)
  list(APPEND _configure_args "-DCMAKE_CXX_COMPILER=${TEST_CXX_COMPILER}")
endif()
if(TEST_RC_COMPILER)
  list(APPEND _configure_args "-DCMAKE_RC_COMPILER=${TEST_RC_COMPILER}")
endif()
if(TEST_MT)
  list(APPEND _configure_args "-DCMAKE_MT=${TEST_MT}")
endif()
if(TEST_TOOLCHAIN_FILE)
  list(APPEND _configure_args "-DCMAKE_TOOLCHAIN_FILE=${TEST_TOOLCHAIN_FILE}")
endif()
if(TEST_JOLT_DIR)
  list(APPEND _configure_args "-DJolt_DIR=${TEST_JOLT_DIR}")

  set(_mismatch_args ${_configure_args})
  list(FIND _mismatch_args "${_build}" _mismatch_build_index)
  if(_mismatch_build_index EQUAL -1)
    message(FATAL_ERROR "could not prepare incompatible Jolt consumer check")
  endif()
  list(REMOVE_AT _mismatch_args ${_mismatch_build_index})
  list(INSERT _mismatch_args ${_mismatch_build_index} "${_mismatch_build}")
  list(APPEND _mismatch_args "-DTEST_FAKE_INCOMPATIBLE_JOLT=ON")
  execute_process(
    COMMAND "${CMAKE_COMMAND}" ${_mismatch_args}
    RESULT_VARIABLE _mismatch_result
    OUTPUT_VARIABLE _mismatch_output
    ERROR_VARIABLE _mismatch_error)
  if(NOT _mismatch_result OR
     NOT "${_mismatch_output}\n${_mismatch_error}" MATCHES
       "incompatible ABI compile definitions")
    message(FATAL_ERROR
      "installed package did not reject an incompatible Jolt target")
  endif()
endif()
execute_process(
  COMMAND "${CMAKE_COMMAND}" ${_configure_args}
  RESULT_VARIABLE _configure_result)
if(_configure_result)
  message(FATAL_ERROR "installed consumer configure failed: ${_configure_result}")
endif()

set(_negative_cases CORE MISSING_CORE_METADATA TOOLCHAIN)
if(TEST_USD)
  list(APPEND _negative_cases CORE_USD)
endif()
foreach(_case IN LISTS _negative_cases)
  set(_negative_args ${_configure_args})
  list(FIND _negative_args "${_build}" _negative_build_index)
  list(REMOVE_AT _negative_args ${_negative_build_index})
  list(INSERT _negative_args ${_negative_build_index} "${_build}-reject-${_case}")
  list(APPEND _negative_args "--fresh")
  if(_case STREQUAL "MISSING_CORE_METADATA")
    list(APPEND _negative_args "-DTEST_FAKE_MISSING_CORE_METADATA=ON")
  elseif(_case STREQUAL "CORE_USD")
    list(APPEND _negative_args "-DTEST_FAKE_INCOMPATIBLE_CORE=ON" "-DTEST_ONLY_USD=ON")
  else()
    list(APPEND _negative_args "-DTEST_FAKE_INCOMPATIBLE_${_case}=ON")
  endif()
  execute_process(COMMAND "${CMAKE_COMMAND}" ${_negative_args}
    RESULT_VARIABLE _negative_result
    OUTPUT_VARIABLE _negative_output ERROR_VARIABLE _negative_error)
  if(_case STREQUAL "TOOLCHAIN")
    set(_expected_error "incompatible compiler/platform/MSVC runtime")
  else()
    set(_expected_error "incompatible physicsCore descriptor/build identity")
  endif()
  if(NOT _negative_result OR NOT "${_negative_output}\n${_negative_error}" MATCHES "${_expected_error}")
    message(FATAL_ERROR "installed package did not reject ${_case}: ${_negative_output}\n${_negative_error}")
  endif()
endforeach()

# The prefix belongs to this test. Restore the header before either asserting
# the rejection or building the positive consumer.
set(_descriptor "${_prefix}/include/usd_physics/core/descriptors.h")
file(READ "${_descriptor}" _original_descriptor)
file(APPEND "${_descriptor}" "\n// mismatched installed header test\n")
set(_header_args ${_configure_args})
list(FIND _header_args "${_build}" _header_build_index)
list(REMOVE_AT _header_args ${_header_build_index})
list(INSERT _header_args ${_header_build_index} "${_build}-reject-HEADERS")
execute_process(COMMAND "${CMAKE_COMMAND}" ${_header_args} --fresh
  RESULT_VARIABLE _header_result OUTPUT_VARIABLE _header_output ERROR_VARIABLE _header_error)
file(WRITE "${_descriptor}" "${_original_descriptor}")
if(NOT _header_result OR NOT "${_header_output}\n${_header_error}" MATCHES
    "installed headers do not match")
  message(FATAL_ERROR "installed package did not reject mismatched headers")
endif()

execute_process(
  COMMAND "${CMAKE_COMMAND}" --build "${_build}" --config "${TEST_CONFIG}"
  RESULT_VARIABLE _build_result)
if(_build_result)
  message(FATAL_ERROR "installed consumer build failed: ${_build_result}")
endif()

execute_process(
  COMMAND "${CMAKE_CTEST_COMMAND}" --test-dir "${_build}"
    -C "${TEST_CONFIG}" --output-on-failure
  RESULT_VARIABLE _test_result)
if(_test_result)
  message(FATAL_ERROR "installed consumer test failed: ${_test_result}")
endif()
