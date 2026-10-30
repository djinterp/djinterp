#*******************************************************************************
# bitwisedom [ui]                                                  UiRoots.cmake
#
# The two roots the window is built from: djinterp's, which holds the model,
# the command line and this build, and uxoxo's, which holds the window's
# elements and the Dear ImGui under them, and lives outside djinterp.
#   DJINTERP_ROOT defaults to the tree this file is in. UXOXO_ROOT, unless
# it is given, is looked for two directories above djinterp's root -- where
# a directory of projects keeps djinterp and uxoxo side by side -- first as
# uxoxo/uxoxo-project, then as uxoxo: the first that holds
# inc/uxoxo/element.hpp. Each is checked here, and one missing or wrong is
# one message naming the variable to set.
#
# path:      /build/cmake/config/ui/bitwisedom/cmake/UiRoots.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
#                                                            revised: 2026.10.23
#*******************************************************************************
include_guard(GLOBAL)

# bitwisedom_ui_root(<variable> <description>)
#   A root's cache entry, its slashes CMake's whatever the shell gave it.
function(bitwisedom_ui_root _variable _description)
    file(TO_CMAKE_PATH "${${_variable}}" _normalized)
    set(${_variable} "${_normalized}" CACHE PATH "${_description}" FORCE)
endfunction()


#-------------------------------------------------------------------------------
# djinterp
#-------------------------------------------------------------------------------
get_filename_component(_ui_djinterp_root
                       "${CMAKE_CURRENT_LIST_DIR}/../../../../../.." ABSOLUTE)

set(DJINTERP_ROOT "${_ui_djinterp_root}" CACHE PATH
    "djinterp's repository root: the directory with inc/ and src/")
bitwisedom_ui_root(DJINTERP_ROOT
    "djinterp's repository root: the directory with inc/ and src/")

if(NOT EXISTS "${DJINTERP_ROOT}/inc/djinterp/cpp/djinterp.hpp")
    message(FATAL_ERROR
        "djinterp's repository root was not found at:\n"
        "    ${DJINTERP_ROOT}\n"
        "(no inc/djinterp/djinterp.hpp there). Set -DDJINTERP_ROOT=<path> "
        "to point at it.")
endif()


#-------------------------------------------------------------------------------
# uxoxo
#-------------------------------------------------------------------------------
# as given, or two directories above djinterp's root
if(NOT UXOXO_ROOT)
    get_filename_component(_ui_projects "${DJINTERP_ROOT}/../.." ABSOLUTE)
    set(_ui_tried "")

    foreach(_ui_candidate "${_ui_projects}/uxoxo/uxoxo-project"
                          "${_ui_projects}/uxoxo")
        list(APPEND _ui_tried "${_ui_candidate}")

        if(EXISTS "${_ui_candidate}/inc/uxoxo/element.hpp")
            set(UXOXO_ROOT "${_ui_candidate}")
            break()
        endif()
    endforeach()

    if(NOT UXOXO_ROOT)
        string(REPLACE ";" "\n    " _ui_tried "${_ui_tried}")

        message(FATAL_ERROR
            "uxoxo was not found two directories above djinterp's root; "
            "looked in:\n"
            "    ${_ui_tried}\n"
            "Set -DUXOXO_ROOT=<path> to uxoxo's project root, the directory "
            "with inc/uxoxo/element.hpp.")
    endif()
endif()

bitwisedom_ui_root(UXOXO_ROOT
    "uxoxo's project root: the directory with inc/uxoxo/")

if(NOT EXISTS "${UXOXO_ROOT}/inc/uxoxo/element.hpp")
    message(FATAL_ERROR
        "${UXOXO_ROOT}\n"
        "does not look like uxoxo's project root: expected to find "
        "'inc/uxoxo/element.hpp' inside it.\n"
        "Set -DUXOXO_ROOT=<path> to point at the right directory.")
endif()

message(STATUS "bitwisedom: uxoxo at ${UXOXO_ROOT}")
