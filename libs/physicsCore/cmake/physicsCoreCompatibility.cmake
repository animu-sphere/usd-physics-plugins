# Conservative compatibility identity for installed static packages. This is
# intentionally stricter than source-version compatibility, not a full ABI audit.
function(physics_core_toolchain_id output)
  set(_runtime "${CMAKE_MSVC_RUNTIME_LIBRARY}")
  if(MSVC AND NOT _runtime)
    set(_runtime "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
  endif()
  string(SHA256 _id
    "${CMAKE_CXX_COMPILER_ID}|${CMAKE_CXX_COMPILER_VERSION}|${CMAKE_SYSTEM_NAME}|${CMAKE_SYSTEM_PROCESSOR}|${CMAKE_SIZEOF_VOID_P}|${_runtime}")
  set(${output} "${_id}" PARENT_SCOPE)
endfunction()

function(physics_core_header_id directory output)
  file(GLOB_RECURSE _headers CONFIGURE_DEPENDS RELATIVE "${directory}"
    "${directory}/*.h" "${directory}/*.hpp" "${directory}/*.inl")
  list(SORT _headers)
  set(_identity "")
  foreach(_header IN LISTS _headers)
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${directory}/${_header}")
    file(READ "${directory}/${_header}" _contents)
    string(REPLACE "\r\n" "\n" _contents "${_contents}")
    string(SHA256 _digest "${_contents}")
    string(APPEND _identity "${_header}:${_digest}\n")
  endforeach()
  string(SHA256 _digest "${_identity}")
  set(${output} "${_digest}" PARENT_SCOPE)
endfunction()

function(physics_core_require_abi expected consumer)
  get_target_property(_actual physicsCore::physicsCore USDPHYSICS_ABI_ID)
  if(NOT "${_actual}" STREQUAL "${expected}")
    message(FATAL_ERROR
      "${consumer}: incompatible physicsCore descriptor/build identity; rebuild core, backend, bridge and consumer together")
  endif()
endfunction()
