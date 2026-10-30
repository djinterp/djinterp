#*******************************************************************************
# vparse [cmake]                                              ProjectRoots.cmake
#
# Where djinterp lives. vparse is djinterp's -- the library in
# parsegen/vparse, the tool in tools/vparse -- so djinterp's root is the one
# this file sits six directories below, and needs no setting. uxoxo, which
# only the window needs, is build/cmake/config/ui/vparse's to find: the window
# is outside djinterp, and this build names no uxoxo.
#   Paths are validated here rather than at first use: a wrong root should be
# one clear message at configure time, not a missing header three minutes
# into a build.
#
# path:      /build/cmake/config/djinterp/tools/vparse/cmake/ProjectRoots.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
#                                                            revised: 2026.10.26
#*******************************************************************************
include_guard(GLOBAL)

get_filename_component(_vparse_djinterp_root
    "${CMAKE_CURRENT_LIST_DIR}/../../../../../../.." ABSOLUTE)

set(DJINTERP_ROOT "${_vparse_djinterp_root}"
    CACHE PATH "djinterp's repository root: the directory with inc/ and src/")

# CMake wants forward slashes everywhere, including in a -D the shell handed
# us with backslashes.
file(TO_CMAKE_PATH "${DJINTERP_ROOT}" _vparse_normalized)
set(DJINTERP_ROOT "${_vparse_normalized}" CACHE PATH "" FORCE)

get_filename_component(DJINTERP_ROOT "${DJINTERP_ROOT}" ABSOLUTE)

# where the tool's sources and suites are, in djinterp's tree
set(VPARSE_SOURCE_DIR "${DJINTERP_ROOT}/src/djinterp/tools/vparse")
set(VPARSE_TESTS_DIR  "${DJINTERP_ROOT}/tests/djinterp/tools/vparse")


# vparse_require_root
#   Fails at configure time, naming the variable to set, when a root is
# missing or is not the directory it claims to be.
function(vparse_require_root _variable _path _witness _what)
    if(NOT IS_DIRECTORY "${_path}")
        message(FATAL_ERROR
            "${_what} was not found at:\n    ${_path}\n"
            "Set -D${_variable}=<path> to point at it.")
    endif()

    if(_witness AND NOT EXISTS "${_path}/${_witness}")
        message(FATAL_ERROR
            "${_path}\ndoes not look like ${_what}: expected to find "
            "'${_witness}' inside it.\nSet -D${_variable}=<path> to point at "
            "the right directory.")
    endif()
endfunction()

vparse_require_root(DJINTERP_ROOT "${DJINTERP_ROOT}"
                    "inc/djinterp/cpp/modules/parsegen/vparse/session.hpp"
                    "djinterp's repository root, with vparse in it")

