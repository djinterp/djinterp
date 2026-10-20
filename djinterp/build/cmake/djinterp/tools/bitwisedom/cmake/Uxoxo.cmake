#*******************************************************************************
# bitwisedom [cmake]                                                 Uxoxo.cmake
#
# uxoxo, and the Dear ImGui it carries, for the window.
#   uxoxo builds itself: its build/cmake/CMakeLists.txt defines uxoxo::uxoxo,
# uxoxo::imgui and the backends it can build here, and takes djinterp from
# the djinterp::headers target Djinterp.cmake defines. A uxoxo without that
# file predates the update this project needs -- the controls, the keys and
# targets, and the build of its own -- and is refused by name; so is one
# without the popup element and the renames in place of 2026.10.09, one
# without the window element and the tab bar's buttons of 2026.10.11, and
# one whose Dear ImGui is not the docking branch, which the window's dock
# spaces and dockable windows are made of (2026.10.18).
#   uxoxo's headers are added again as SYSTEM headers for this project's
# targets: they are a dependency's, and the style guide's whole baseline is
# this project's to meet, not theirs. Under cl its library is given
# /Zc:preprocessor, which its own build does not give it: it reads
# djinterp's headers, which this build reads with no other preprocessor.
#
# path:      /build/cmake/djinterp/tools/bitwisedom/cmake/Uxoxo.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
#                                                            revised: 2026.10.19
#*******************************************************************************
include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/Djinterp.cmake")

bitwisedom_require_root(UXOXO_ROOT "${UXOXO_ROOT}"
                       "inc/uxoxo/element.hpp" "uxoxo's project root")

if(NOT EXISTS "${UXOXO_ROOT}/build/cmake/CMakeLists.txt")
    message(FATAL_ERROR
        "uxoxo at ${UXOXO_ROOT} has no build/cmake/CMakeLists.txt.\n"
        "bitwisedom needs the updated uxoxo (2026.10.04 or later): its "
        "number, combo, colour, tree, menu, table and splitter elements, "
        "element keys and targets, and its own build.")
endif()

if(NOT EXISTS "${UXOXO_ROOT}/inc/uxoxo/element_tree_node.hpp")
    message(FATAL_ERROR
        "uxoxo at ${UXOXO_ROOT} predates the elements bitwisedom uses "
        "(element_tree_node.hpp is missing). Use the updated uxoxo.")
endif()

if(NOT EXISTS "${UXOXO_ROOT}/inc/uxoxo/element_popup.hpp")
    message(FATAL_ERROR
        "uxoxo at ${UXOXO_ROOT} predates the context menus and renames in "
        "place bitwisedom uses (element_popup.hpp is missing). Extract the "
        "uxoxo of 2026.10.11 or later over UXOXO_ROOT.")
endif()

if(NOT EXISTS "${UXOXO_ROOT}/inc/uxoxo/element_window.hpp")
    message(FATAL_ERROR
        "uxoxo at ${UXOXO_ROOT} predates the panel's windows and the tab "
        "bar's buttons bitwisedom uses (element_window.hpp is missing). "
        "Extract the uxoxo of 2026.10.11 or later over UXOXO_ROOT.")
endif()

# the docking branch of Dear ImGui: its imgui.h says so
set(_bitwisedom_imgui "${UXOXO_ROOT}/inc/imgui/imgui.h")
set(_bitwisedom_docking "")

if(EXISTS "${_bitwisedom_imgui}")
    file(STRINGS "${_bitwisedom_imgui}" _bitwisedom_docking
         REGEX "^#define[ \t]+IMGUI_HAS_DOCK")
endif()

if(NOT _bitwisedom_docking)
    message(FATAL_ERROR
        "uxoxo at ${UXOXO_ROOT} carries a Dear ImGui without docking "
        "(inc/imgui/imgui.h does not define IMGUI_HAS_DOCK). bitwisedom's "
        "windows dock, as Visual Studio's do: extract the uxoxo of "
        "2026.10.18, whose ImGui is the docking branch, over UXOXO_ROOT.")
endif()

# the tab rows that begin past a sheet's symbols column: uxoxo's ImGui of
# 2026.10.19 lets a dock node's tab bar begin inset. Without it, a sheet's
# tabs run across the whole width, as ImGui's own do
set(_bitwisedom_inset "")

if(EXISTS "${UXOXO_ROOT}/inc/imgui/imgui_internal.h")
    file(STRINGS "${UXOXO_ROOT}/inc/imgui/imgui_internal.h" _bitwisedom_inset
         REGEX "^#define[ \t]+IMGUI_DOCK_TAB_BAR_INSET")
endif()

if(NOT _bitwisedom_inset)
    message(WARNING
        "uxoxo at ${UXOXO_ROOT} carries a Dear ImGui whose dock tab bars "
        "cannot begin inset (inc/imgui/imgui_internal.h does not define "
        "IMGUI_DOCK_TAB_BAR_INSET): a sheet's tabs will run across its "
        "symbols column. Extract the uxoxo overlay of 2026.10.19 over "
        "UXOXO_ROOT for tabs over the bits alone.")
endif()

set(UXOXO_CXX_STANDARD ${BITWISEDOM_CXX_STANDARD} CACHE STRING
    "C++ level for uxoxo: 17, 20 or 23")
set(UXOXO_BUILD_TESTS ${BITWISEDOM_BUILD_UXOXO_TESTS} CACHE BOOL
    "Build uxoxo's own tests" FORCE)

add_subdirectory("${UXOXO_ROOT}/build/cmake"
                 "${CMAKE_BINARY_DIR}/_deps/uxoxo")

if(NOT TARGET uxoxo::uxoxo)
    message(FATAL_ERROR "uxoxo's build did not define uxoxo::uxoxo")
endif()

# uxoxo's library reads djinterp's headers, as this project's units do, and
# under cl takes the conforming preprocessor with them: uxoxo's own build
# gives its units /utf-8 alone
if( (MSVC) AND (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang") )
    get_target_property(_uxoxo_library uxoxo::uxoxo ALIASED_TARGET)
    target_compile_options(${_uxoxo_library} PRIVATE /Zc:preprocessor)
endif()

set(BITWISEDOM_UXOXO_INCLUDES
    "${UXOXO_ROOT}/inc"
    "${UXOXO_ROOT}/inc/imgui"
    "${UXOXO_ROOT}/inc/imgui/backends")
