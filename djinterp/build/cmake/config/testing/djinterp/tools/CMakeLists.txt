# =============================================================================
# testing/djinterp/tools/
#   Tools aggregate: recurses into every tool leaf beneath it (today the
# command-line kit's and dawk's), which no parent reached before this file.
# Each leaf makes its tests with add_executable and add_test of its own, so,
# like parse/cli/, this file only recurses, without a group target that
# would stand empty and warn.
# =============================================================================
file(GLOB _entries CONFIGURE_DEPENDS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
     "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(_entry ${_entries})
    if ( (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${_entry}") AND
         (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_entry}/CMakeLists.txt") )
        add_subdirectory("${_entry}")
    endif()
endforeach()
