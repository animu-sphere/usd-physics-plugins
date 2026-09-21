function(usdphysics_assert_target_dependencies)
  cmake_parse_arguments(ARG "" "TARGET" "ALLOWED_PUBLIC" ${ARGN})
  if(NOT ARG_TARGET OR NOT TARGET ${ARG_TARGET})
    message(FATAL_ERROR "usdphysics_assert_target_dependencies requires an existing TARGET")
  endif()

  get_target_property(_links ${ARG_TARGET} INTERFACE_LINK_LIBRARIES)
  if(NOT _links)
    set(_links "")
  endif()
  foreach(_link IN LISTS _links)
    if(NOT _link IN_LIST ARG_ALLOWED_PUBLIC)
      message(FATAL_ERROR
        "${ARG_TARGET} publishes forbidden dependency '${_link}'; allowed: ${ARG_ALLOWED_PUBLIC}")
    endif()
  endforeach()
endfunction()

