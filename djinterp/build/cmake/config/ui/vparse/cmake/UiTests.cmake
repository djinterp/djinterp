#*******************************************************************************
# vparse [ui]                                                      UiTests.cmake
#
# The window's tests, which build/cmake/config/ui/vparse's CMakeLists includes
# under VPARSE_UI_BUILD_TESTS, beside vparse's own, which djinterp's vparse
# build adds within it:
#
#     vparse_workbench_tests        the window's model and face, as values:
#                                   the opening run, events, the face
#     vparse_workbench_imgui_tests  the face drawn on uxoxo's Dear ImGui
#                                   platform in frames with no window, where
#                                   Dear ImGui has its null backend
#     vparse_gui_help               `vparse gui --help`: the module found
#                                   beside the command line, loaded and run
#     vparse_gui_version            `vparse gui --version -v`: the module's
#                                   build, its window and its Dear ImGui
#     vparse_gui_unread             `vparse gui` given a grammar it cannot
#                                   read: refused before any window, as the
#                                   commands refuse one
#
# Each suite must print exactly its count of sections, so a section that
# silently stopped being compiled fails rather than reading as a smaller
# pass; each run of the command line must end with its status and say its
# words, both, which the kit's d_cli_add_test holds it to. None needs a
# display.
#
# path:      /build/cmake/config/ui/vparse/cmake/UiTests.cmake
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.26
#                                                            revised: 2026.10.26
#*******************************************************************************
include_guard(GLOBAL)


#-------------------------------------------------------------------------------
# the window's suites
#-------------------------------------------------------------------------------
# vparse_ui_suite(<target> <sections> <source> <library>...)
#   One of the window's suites, from its source, at the window's level and
# linked to what it tests, run in the build's binary directory.
function(vparse_ui_suite _target _sections _source)
    add_executable(${_target} "${VPARSE_UI_TESTS_DIR}/${_source}")
    target_link_libraries(${_target} PRIVATE ${ARGN})
    vparse_apply_compiler_defaults(${_target} OWN
        CXX_STANDARD ${UXOXO_CXX_STANDARD})
    set_target_properties(${_target} PROPERTIES FOLDER "vparse/ui/tests")

    add_test(NAME ${_target} COMMAND ${_target}
             WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")
    set_tests_properties(${_target} PROPERTIES
        PASS_REGULAR_EXPRESSION "passed: ${_sections}   failed: 0"
        FAIL_REGULAR_EXPRESSION "\\[FAIL\\]")
endfunction()

vparse_ui_suite(vparse_workbench_tests 3 workbench_tests.cpp
                vparse::workbench_face)

# Dear ImGui's null backend runs frames with no screen; it is newer than the
# release uxoxo's platform itself needs, so a Dear ImGui without it builds
# the module and not this suite. The backend is Dear ImGui's code, compiled
# as Dear ImGui is: its level and the character set, and no warning set of
# ours.
set(_vparse_ui_null "${VPARSE_IMGUI_ROOT}/backends/imgui_impl_null.cpp")

if(EXISTS "${_vparse_ui_null}")
    add_library(vparse_imgui_null STATIC "${_vparse_ui_null}")
    target_link_libraries(vparse_imgui_null PUBLIC uxoxo::imgui)
    vparse_apply_dependency_defaults(vparse_imgui_null)
    set_target_properties(vparse_imgui_null PROPERTIES
        FOLDER "vparse/ui/tests")

    vparse_ui_suite(vparse_workbench_imgui_tests 3
                    workbench_imgui_tests.cpp
                    vparse::workbench_imgui
                    vparse_imgui_null)
else()
    message(STATUS "vparse: the workbench's ImGui suite left out -- the "
                   "Dear ImGui at ${VPARSE_IMGUI_ROOT} has no "
                   "backends/imgui_impl_null.cpp")
endif()


#-------------------------------------------------------------------------------
# the module, run by the command line
#-------------------------------------------------------------------------------
# found beside the command line, loaded and run, with no window opened: its
# help, its version, and a grammar it refuses as the commands refuse one
d_cli_add_test(vparse_gui_help
    COMMAND vparse gui --help
    STATUS  0
    MATCHES "Usage: vparse gui \\[OPTION\\]\\.\\.\\. \\[GRAMMAR\\] \\[INPUT\\]")

d_cli_add_test(vparse_gui_version
    COMMAND vparse gui --version -v
    STATUS  0
    MATCHES "\nwindow: [^\n]+\nimgui: ")

d_cli_add_test(vparse_gui_unread
    COMMAND vparse gui no_such_grammar.txt
    STATUS  2
    MATCHES "vparse: error: cannot read 'no_such_grammar.txt': ")
