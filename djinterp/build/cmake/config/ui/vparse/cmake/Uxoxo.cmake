#*******************************************************************************
# vparse [ui]                                                        Uxoxo.cmake
#
# uxoxo, and the Dear ImGui it carries, for the window.
#   uxoxo builds itself: its build/cmake/CMakeLists.txt defines uxoxo::uxoxo,
# uxoxo::imgui and the backends it can build here, and takes djinterp from
# the djinterp::headers target djinterp's vparse build defines, which this
# build adds first. A uxoxo without that file predates the update the window
# needs, and is refused by name; so is one that lacks a header the window
# includes, or the Dear ImGui under inc/imgui.
#   uxoxo's library is given the reading djinterp's headers need
# (vparse_apply_reading, from djinterp's vparse build). UiRoots.cmake has
# found and checked UXOXO_ROOT; this file is included after djinterp's
# vparse build.
#
# path:      /build/cmake/config/ui/vparse/cmake/Uxoxo.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
#                                                            revised: 2026.10.26
#*******************************************************************************
include_guard(GLOBAL)

if(NOT TARGET djinterp::headers)
    message(FATAL_ERROR
        "Uxoxo.cmake is included after djinterp's vparse build, whose "
        "djinterp::headers uxoxo builds with")
endif()

vparse_require_root(UXOXO_ROOT "${UXOXO_ROOT}"
                    "inc/uxoxo/element.hpp" "uxoxo's project root")

if(NOT EXISTS "${UXOXO_ROOT}/build/cmake/CMakeLists.txt")
    message(FATAL_ERROR
        "uxoxo at ${UXOXO_ROOT} has no build/cmake/CMakeLists.txt.\n"
        "The workbench needs uxoxo of 2026.10.04 or later, which builds "
        "itself. Extract a current uxoxo over UXOXO_ROOT; vparse's command "
        "line and its suites build without it, from "
        "build/cmake/djinterp/tools/vparse.")
endif()

# what the workbench includes of uxoxo
set(_vparse_uxoxo_needs
    "inc/uxoxo/closing.hpp"
    "inc/uxoxo/element.hpp"
    "inc/uxoxo/element_button.hpp"
    "inc/uxoxo/element_column.hpp"
    "inc/uxoxo/element_label.hpp"
    "inc/uxoxo/element_panel.hpp"
    "inc/uxoxo/element_row.hpp"
    "inc/uxoxo/element_selectable.hpp"
    "inc/uxoxo/element_separator.hpp"
    "inc/uxoxo/element_tabs.hpp"
    "inc/uxoxo/element_text_field.hpp"
    "inc/uxoxo/element_toggle.hpp"
    "inc/uxoxo/live.hpp"
    "inc/uxoxo/platform/imgui_platform.hpp"
    "inc/uxoxo/realize.hpp")
set(_vparse_uxoxo_missing "")

foreach(_file IN LISTS _vparse_uxoxo_needs)
    if(NOT EXISTS "${UXOXO_ROOT}/${_file}")
        list(APPEND _vparse_uxoxo_missing "${_file}")
    endif()
endforeach()

if(_vparse_uxoxo_missing)
    string(REPLACE ";" "\n    " _vparse_uxoxo_missing
           "${_vparse_uxoxo_missing}")

    message(FATAL_ERROR
        "uxoxo at ${UXOXO_ROOT} lacks what the workbench uses:\n"
        "    ${_vparse_uxoxo_missing}\n"
        "Extract a current uxoxo over UXOXO_ROOT.")
endif()

# uxoxo carries its Dear ImGui, and its build expects it there
if(NOT EXISTS "${UXOXO_ROOT}/inc/imgui/imgui.cpp")
    message(FATAL_ERROR
        "uxoxo at ${UXOXO_ROOT} has no Dear ImGui under inc/imgui, where "
        "uxoxo carries it and its build looks for it. Put Dear ImGui 1.92 or "
        "later there, laid out as its repository is (imgui.cpp, backends/, "
        "misc/cpp/).")
endif()

set(UXOXO_BUILD_TESTS ${VPARSE_UI_BUILD_UXOXO_TESTS} CACHE BOOL
    "Build uxoxo's own tests" FORCE)

add_subdirectory("${UXOXO_ROOT}/build/cmake"
                 "${CMAKE_BINARY_DIR}/_deps/uxoxo")

if(NOT TARGET uxoxo::uxoxo)
    message(FATAL_ERROR "uxoxo's build did not define uxoxo::uxoxo")
endif()

# one way of reading djinterp: uxoxo's units include its headers too
get_target_property(_vparse_uxoxo_library uxoxo::uxoxo ALIASED_TARGET)
vparse_apply_reading(${_vparse_uxoxo_library} PRIVATE)

# where Dear ImGui is, for the suite that needs its null backend
set(VPARSE_IMGUI_ROOT "${UXOXO_ROOT}/inc/imgui")

message(STATUS "uxoxo: from ${UXOXO_ROOT}, through its own build")
