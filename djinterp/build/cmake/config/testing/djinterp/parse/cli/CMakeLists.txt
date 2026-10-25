# =============================================================================
# testing/djinterp/parse/cli/
#   Command-line aggregate: recurses into the argument parser's leaf. That
# leaf makes its tests with add_executable and add_test of its own, not
# djinterp_add_test_executable, so there is nothing for a group target to
# roll up: this file only recurses, as djinterp_add_test_subtree would,
# without the group target that would stand empty and warn.
# =============================================================================
file(GLOB _entries CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
     "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(_entry ${_entries})
    if ( (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${_entry}") AND
         (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_entry}/CMakeLists.txt") )
        add_subdirectory("${_entry}")
    endif()
endforeach()
