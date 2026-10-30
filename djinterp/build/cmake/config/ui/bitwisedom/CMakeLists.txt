#*******************************************************************************
# bitwisedom [ui]                                                 CMakeLists.txt
#
# bitwisedom's window: the module `bitwisedom gui` loads (GRAMMAR.md, section
# 7), made on uxoxo and Dear ImGui outside the djinterp tree -- its sources
# in inc/ui, src/ui and tests/ui.
#   A top-level build: it builds djinterp's bitwisedom
# (build/cmake/config/djinterp/tools/bitwisedom) -- the model, the command line
# and their tests -- into its own binary directory, then uxoxo, which
# cmake/UiRoots.cmake finds and cmake/Uxoxo.cmake checks and adds, and
# makes:
#
#     bitwisedom-gui   the module, with the command line in bin/, named as
#                     the command line looks for it -- bitwisedom-gui-
#                     linux64.so beside bitwisedom-linux64: the window on
#                     Win32 + Direct3D 11 on Windows, GLFW + OpenGL 3
#                     elsewhere (BITWISEDOM_HOST chooses)
#
# and, under BITWISEDOM_BUILD_TESTS, ui_smoke -- the window headless, through
# ImGui's null backend -- and three runs of the command line that find, load
# and run the module as a user's would, with no window opened: its help, its
# version, and an example it does not know, each held to its status and its
# words.
#   The module is a shared object that links the model, the kit and uxoxo,
# so everything is compiled position-independent.
#
# path:      /build/cmake/config/ui/bitwisedom/CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.23
#                                                            revised: 2026.10.26
#*******************************************************************************
cmake_minimum_required(VERSION 3.21)

project(bitwisedom_ui
    DESCRIPTION "bitwisedom's window, on uxoxo and Dear ImGui"
    LANGUAGES   C CXX)

option(BITWISEDOM_BUILD_TESTS        "Build the tests"                  ON)
option(BITWISEDOM_BUILD_UXOXO_TESTS  "Build uxoxo's own tests too"      OFF)

set(BITWISEDOM_HOST "auto" CACHE STRING
    "The window's host: auto (win32 on Windows, else glfw), win32, glfw")
set_property(CACHE BITWISEDOM_HOST PROPERTY STRINGS auto win32 glfw)

# the module links every static library it uses, uxoxo's and Dear ImGui's
# among them
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(BITWISEDOM_BUILD_TESTS)
    enable_testing()
endif()

include("${CMAKE_CURRENT_LIST_DIR}/cmake/UiRoots.cmake")

# the model, the command line and their tests, as djinterp builds them
add_subdirectory("${DJINTERP_ROOT}/build/cmake/config/djinterp/tools/bitwisedom"
                 "${CMAKE_BINARY_DIR}/bitwisedom")

include("${CMAKE_CURRENT_LIST_DIR}/cmake/Uxoxo.cmake")

set(BITWISEDOM_UI_SOURCE_DIR "${DJINTERP_ROOT}/src/ui/bitwisedom")
set(BITWISEDOM_UI_TESTS_DIR  "${DJINTERP_ROOT}/tests/ui/bitwisedom")

# the window's sources that changed since this build tree last configured,
# and uxoxo's with the Dear ImGui in it, marked to compile again
# (Freshness.cmake, beside djinterp's bitwisedom build)
bitwisedom_freshen("${CMAKE_BINARY_DIR}/bitwisedom_ui_sources.sha1"
                   "${BITWISEDOM_UI_SOURCE_DIR}"
                   "${DJINTERP_ROOT}/inc/ui/bitwisedom"
                   "${BITWISEDOM_UI_TESTS_DIR}"
                   "${UXOXO_ROOT}/inc"
                   "${UXOXO_ROOT}/src")


#-------------------------------------------------------------------------------
# the window
#-------------------------------------------------------------------------------
set(BITWISEDOM_UI_SOURCES
    "${BITWISEDOM_UI_SOURCE_DIR}/app.cpp"
    "${BITWISEDOM_UI_SOURCE_DIR}/code.cpp"
    "${BITWISEDOM_UI_SOURCE_DIR}/controls.cpp"
    "${BITWISEDOM_UI_SOURCE_DIR}/flow.cpp"
    "${BITWISEDOM_UI_SOURCE_DIR}/grid.cpp"
    "${BITWISEDOM_UI_SOURCE_DIR}/settings.cpp"
    "${BITWISEDOM_UI_SOURCE_DIR}/shell.cpp"
    "${BITWISEDOM_UI_SOURCE_DIR}/theme.cpp"
    "${BITWISEDOM_UI_SOURCE_DIR}/view.cpp"
    "${BITWISEDOM_UI_SOURCE_DIR}/fonts/fonts.cpp")

set(_host "${BITWISEDOM_HOST}")

if(_host STREQUAL "auto")
    if(WIN32)
        set(_host "win32")
    else()
        set(_host "glfw")
    endif()
endif()

if(_host STREQUAL "win32")
    if(NOT TARGET uxoxo::imgui_win32_dx11)
        message(FATAL_ERROR "BITWISEDOM_HOST is win32, but uxoxo has no "
                            "Win32 + Direct3D 11 backend off Windows")
    endif()

    set(_host_libraries uxoxo::imgui_win32_dx11 shell32 comdlg32)
elseif(_host STREQUAL "glfw")
    if(NOT TARGET uxoxo::imgui_glfw_opengl3)
        message(FATAL_ERROR
            "BITWISEDOM_HOST is glfw, but uxoxo found no GLFW. Install "
            "GLFW 3.3 or later (libglfw3-dev, brew install glfw, or vcpkg "
            "glfw3), or on Windows use BITWISEDOM_HOST=win32.")
    endif()

    find_package(OpenGL REQUIRED)

    set(_host_libraries uxoxo::imgui_glfw_opengl3 OpenGL::GL)

    if(WIN32)
        list(APPEND _host_libraries comdlg32)
    endif()
else()
    message(FATAL_ERROR
        "BITWISEDOM_HOST is '${BITWISEDOM_HOST}'; it must be auto, win32 "
        "or glfw")
endif()

# the module: the window, its host, and the run function `bitwisedom gui`
# calls, which reads the command line through the kit and bitwisedom's own
# rules for a SHEET
d_cli_add_plugin(bitwisedom-gui
    SOURCES ${BITWISEDOM_UI_SOURCES}
            "${BITWISEDOM_UI_SOURCE_DIR}/plugin.cpp"
            "${BITWISEDOM_UI_SOURCE_DIR}/host_${_host}.cpp")
bitwisedom_output(bitwisedom-gui)
target_link_libraries(bitwisedom-gui PRIVATE
    bitwisedom::cli uxoxo::uxoxo ${_host_libraries})
target_include_directories(bitwisedom-gui SYSTEM PRIVATE
    ${BITWISEDOM_UXOXO_INCLUDES})
bitwisedom_apply_compiler_defaults(bitwisedom-gui OWN)

# `bitwisedom gui` loads the module the build made with it
add_dependencies(bitwisedom bitwisedom-gui)

message(STATUS "bitwisedom: the window's host is ${_host}")


#-------------------------------------------------------------------------------
# tests
#-------------------------------------------------------------------------------
if(BITWISEDOM_BUILD_TESTS)
    # the window, headless: it compiles its own ImGui and its own copy of
    # uxoxo's platform, with every ID clash recorded
    # (IMGUI_DEBUG_HIGHLIGHT_ALL_ID_CONFLICTS): without that, ImGui notices a
    # clash only under the mouse. The define never reaches the module
    set(_imgui "${UXOXO_ROOT}/inc/imgui")

    add_executable(bitwisedom_ui_smoke
        "${BITWISEDOM_UI_TESTS_DIR}/ui_smoke.cpp"
        ${BITWISEDOM_UI_SOURCES}
        "${UXOXO_ROOT}/src/uxoxo/platform/imgui_platform.cpp"
        "${_imgui}/imgui.cpp"
        "${_imgui}/imgui_draw.cpp"
        "${_imgui}/imgui_tables.cpp"
        "${_imgui}/imgui_widgets.cpp"
        "${_imgui}/imgui_demo.cpp"
        "${_imgui}/misc/cpp/imgui_stdlib.cpp"
        "${_imgui}/backends/imgui_impl_null.cpp")
    bitwisedom_output(bitwisedom_ui_smoke)
    target_include_directories(bitwisedom_ui_smoke SYSTEM PRIVATE
        ${BITWISEDOM_UXOXO_INCLUDES})
    target_compile_definitions(bitwisedom_ui_smoke PRIVATE
        IMGUI_DEBUG_HIGHLIGHT_ALL_ID_CONFLICTS)
    target_link_libraries(bitwisedom_ui_smoke PRIVATE bitwisedom::core)

    if(WIN32)
        target_link_libraries(bitwisedom_ui_smoke PRIVATE comdlg32)
    endif()

    bitwisedom_apply_dependency_defaults(bitwisedom_ui_smoke)

    add_test(NAME ui_smoke
             COMMAND bitwisedom_ui_smoke
             WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    set_tests_properties(ui_smoke PROPERTIES
        PASS_REGULAR_EXPRESSION "passed: 46   failed: 0")

    # the module found beside the command line, loaded and run, with no
    # window opened: its help; its version; and a SHEET it refuses as the
    # commands refuse one -- each held to its status and its words by the
    # kit's d_cli_add_test
    d_cli_add_test(gui_help
        COMMAND bitwisedom gui --help
        STATUS  0
        MATCHES
            "Usage: bitwisedom gui \\[OPTION\\]\\.\\.\\. \\[SHEET\\]\\.\\.\\.")

    d_cli_add_test(gui_version
        COMMAND bitwisedom gui --version -v
        STATUS  0
        MATCHES "\nwindow: [^\n]+\nimgui: ")

    d_cli_add_test(gui_unknown_example
        COMMAND bitwisedom gui example:nonesuch
        STATUS  2
        MATCHES "bitwisedom: error: 'nonesuch' is not a built-in example")
endif()
