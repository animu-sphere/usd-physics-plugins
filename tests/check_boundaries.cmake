function(assert_sources_exclude root label)
  file(GLOB_RECURSE _sources
    "${root}/include/*.h"
    "${root}/include/*.hpp"
    "${root}/src/*.h"
    "${root}/src/*.hpp"
    "${root}/src/*.cpp"
    "${root}/tests/*.h"
    "${root}/tests/*.hpp"
    "${root}/tests/*.cpp")
  list(APPEND _sources "${root}/CMakeLists.txt")
  foreach(_source IN LISTS _sources)
    file(READ "${_source}" _content)
    foreach(_pattern IN LISTS ARGN)
      if(_content MATCHES "${_pattern}")
        message(FATAL_ERROR
          "${label} boundary violation: ${_source} matches '${_pattern}'")
      endif()
    endforeach()
  endforeach()
endfunction()

assert_sources_exclude(
  "${PROJECT_ROOT}/libs/physicsCore"
  "physicsCore"
  "[#]include[ \\t]*[<\"]Jolt/"
  "[#]include[ \\t]*[<\"]pxr/"
  "usd_stage_runner"
  "usd_mmd"
  "usd_vrm")

assert_sources_exclude(
  "${PROJECT_ROOT}/backends/physicsJolt"
  "physicsJolt"
  "[#]include[ \\t]*[<\"]pxr/"
  "usd_stage_runner"
  "usd_mmd"
  "usd_vrm")

file(GLOB_RECURSE _public_jolt_headers
  "${PROJECT_ROOT}/backends/physicsJolt/include/*.h"
  "${PROJECT_ROOT}/backends/physicsJolt/include/*.hpp")
foreach(_header IN LISTS _public_jolt_headers)
  file(READ "${_header}" _content)
  if(_content MATCHES "[#]include[ \\t]*[<\"]Jolt/")
    message(FATAL_ERROR "physicsJolt public header leaks Jolt: ${_header}")
  endif()
endforeach()

message(STATUS "physicsCore and physicsJolt source boundaries are clean")
