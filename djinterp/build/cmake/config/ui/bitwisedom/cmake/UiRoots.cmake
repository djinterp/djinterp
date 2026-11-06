#*******************************************************************************
# bitwisedom [ui]                                                  UiRoots.cmake
#
# The two roots the window is built from: djinterp's, which holds the model,
# the command line and this build, and uxoxo's, which holds the window's
# elements and the Dear ImGui under them, and lives outside djinterp.
#   DJINTERP_ROOT defaults to the tree this file is in. UXOXO_ROOT is where
# -DUXOXO_ROOT, or else the environment's UXOXO_ROOT, points; else the
# sibling checkout (ruling cli-taken 22): `uxoxo` beside djinterp's checkout
# -- the directory that holds djinterp's root -- or `uxoxo/uxoxo-project`
# there; then, for a directory of projects that keeps each project's
# checkouts in a folder of its own, the same two a level up --
# `frameworks/djinterp/release` beside `frameworks/uxoxo/uxoxo-project`, the
# owner's layout: the first that holds inc/uxoxo/element.hpp. It is found
# when the window is built, never when it runs: uxoxo is compiled into the
# window's module. And it is never a copy inside djinterp's tree, which
# knows nothing of uxoxo (ruling cli e). Each root is checked here, and one
# missing or wrong is one message naming the variable to set.
#
# path:      /build/cmake/config/ui/bitwisedom/cmake/UiRoots.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
#                                                            revised: 2026.11.04
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
# as given, by the cache or the environment; else the sibling checkout
if( (NOT UXOXO_ROOT) AND
    (DEFINED ENV{UXOXO_ROOT}) AND
    (NOT "$ENV{UXOXO_ROOT}" STREQUAL "") )
    set(UXOXO_ROOT "$ENV{UXOXO_ROOT}")
endif()

if(NOT UXOXO_ROOT)
    get_filename_component(_ui_checkouts "${DJINTERP_ROOT}/../.." ABSOLUTE)
    get_filename_component(_ui_projects "${_ui_checkouts}/.." ABSOLUTE)
    set(_ui_tried "")

    foreach(_ui_candidate "${_ui_checkouts}/uxoxo"
                          "${_ui_checkouts}/uxoxo/uxoxo-project"
                          "${_ui_projects}/uxoxo/uxoxo-project"
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
            "uxoxo was not found beside djinterp's checkout; looked in:\n"
            "    ${_ui_tried}\n"
            "Set -DUXOXO_ROOT=<path>, or UXOXO_ROOT in the environment, to "
            "uxoxo's project root, the directory with "
            "inc/uxoxo/element.hpp.")
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

# never a copy inside djinterp, which knows nothing of uxoxo (ruling cli e)
file(RELATIVE_PATH _ui_inside "${DJINTERP_ROOT}" "${UXOXO_ROOT}")

if( (NOT _ui_inside MATCHES "^\\.\\.(/|$)") AND
    (NOT IS_ABSOLUTE "${_ui_inside}") )
    message(FATAL_ERROR
        "${UXOXO_ROOT}\n"
        "is inside djinterp's tree, ${DJINTERP_ROOT}: djinterp keeps no copy "
        "of uxoxo (ruling cli e). Check uxoxo out beside djinterp's "
        "checkout, or set -DUXOXO_ROOT=<path> to a checkout outside it.")
endif()

message(STATUS "bitwisedom: uxoxo at ${UXOXO_ROOT}")
