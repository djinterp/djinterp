#*******************************************************************************
# bitwisedom [cmake]                                          ProjectRoots.cmake
#
# The repository: djinterp, which contains bitwisedom.
#   DJINTERP_ROOT defaults to the containing repository, derived from this
# file's own location, and is validated here rather than at first use.
# uxoxo, which only the window needs, is build/cmake/config/ui's to find: the
# window lives outside the djinterp tree.
#
# path:      /build/cmake/config/djinterp/tools/bitwisedom/cmake/ProjectRoots.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
#                                                            revised: 2026.10.22
#*******************************************************************************
include_guard(GLOBAL)

get_filename_component(_bitwisedom_djinterp_root
    "${CMAKE_CURRENT_LIST_DIR}/../../../../../../.." ABSOLUTE)

set(DJINTERP_ROOT "${_bitwisedom_djinterp_root}"
    CACHE PATH "djinterp's repository root: the directory with inc/ and src/")

set(BITWISEDOM_ROOT "${_bitwisedom_djinterp_root}"
    CACHE PATH "Containing djinterp repository root")

# CMake wants forward slashes everywhere, including in a -D the shell handed
# us with backslashes.
foreach(_root DJINTERP_ROOT BITWISEDOM_ROOT)
    file(TO_CMAKE_PATH "${${_root}}" _normalized)
    set(${_root} "${_normalized}" CACHE PATH "" FORCE)
endforeach()

get_filename_component(BITWISEDOM_ROOT "${BITWISEDOM_ROOT}" ABSOLUTE)


# bitwisedom_require_root
#   Fails at configure time, naming the variable to set, when a root is
# missing or is not the directory it claims to be.
function(bitwisedom_require_root _variable _path _witness _what)
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

bitwisedom_require_root(DJINTERP_ROOT "${DJINTERP_ROOT}"
                       "inc/djinterp/cpp/djinterp.hpp" "djinterp's repository root")

