#*******************************************************************************
# vparse [ui]                                                     CMakeLists.txt
#
# vparse's window: the workbench, the module `vparse gui` loads (GRAMMAR.md,
# section 7), made on uxoxo and Dear ImGui outside the djinterp tree -- its
# sources in inc/ui/vparse, src/ui/vparse and tests/ui/vparse.
#   A top-level build: it builds djinterp's vparse
# (build/cmake/djinterp/tools/vparse) -- the library, the model, the command
# line and, under VPARSE_UI_BUILD_TESTS, their suites -- into its own binary
# directory, then uxoxo, which cmake/UiRoots.cmake finds and cmake/Uxoxo.cmake
# checks and adds, and makes:
#
#     vparse::workbench_face   the window's model, its face as a uxoxo
#                              template, the update that applies an event,
#                              and the workbench's own elements
#     vparse::workbench_imgui  those elements on uxoxo's Dear ImGui platform,
#                              and the theme
#     vparse-gui               the module, with the command line in bin/,
#                              named as the command line looks for it --
#                              vparse-gui-linux64.so beside vparse-linux64:
#                              the window on Win32 + Direct3D 11 on Windows,
#                              GLFW + OpenGL 3 elsewhere, or none
#                              (VPARSE_UI_HOST chooses)
#
# and, under VPARSE_UI_BUILD_TESTS, the window's two suites -- its model and
# face as values, and the face drawn on Dear ImGui's null backend, where
# Dear ImGui has it -- and three runs of the command line that find, load and
# run the module as a user's would, with no window opened: its help, its
# version, and a grammar it cannot read.
#   The module is a shared object that links the model, the kit and uxoxo,
# so everything is compiled position-independent.
#
# path:      /build/cmake/config/ui/vparse/CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.26
#                                                            revised: 2026.10.26
#*******************************************************************************
cmake_minimum_required(VERSION 3.21)

project(vparse_ui
    DESCRIPTION "vparse's window, on uxoxo and Dear ImGui"
    LANGUAGES   C CXX)

option(VPARSE_UI_BUILD_TESTS
       "Build the window's tests, and vparse's with them"    ON)
option(VPARSE_UI_BUILD_UXOXO_TESTS "Build uxoxo's own tests too" OFF)

set(VPARSE_UI_HOST "auto" CACHE STRING
    "The window's host: auto, win32, glfw or none")
set_property(CACHE VPARSE_UI_HOST PROPERTY STRINGS auto win32 glfw none)

# the module links every static library it uses, uxoxo's and Dear ImGui's
# among them
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# one runtime library across every target, or the link fails at the end
if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
    set(CMAKE_MSVC_RUNTIME_LIBRARY
        "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

if(VPARSE_UI_BUILD_TESTS)
    enable_testing()
endif()

include("${CMAKE_CURRENT_LIST_DIR}/cmake/UiRoots.cmake")

# vparse, its command line and their suites, as djinterp builds them
set(VPARSE_BUILD_CLI   ON                       CACHE BOOL "" FORCE)
set(VPARSE_BUILD_TESTS ${VPARSE_UI_BUILD_TESTS} CACHE BOOL "" FORCE)

add_subdirectory("${DJINTERP_ROOT}/build/cmake/config/djinterp/tools/vparse"
                 "${CMAKE_BINARY_DIR}/vparse")

include("${CMAKE_CURRENT_LIST_DIR}/cmake/Uxoxo.cmake")

set(VPARSE_UI_SOURCE_DIR  "${DJINTERP_ROOT}/src/ui/vparse")
set(VPARSE_UI_INCLUDE_DIR "${DJINTERP_ROOT}/inc/ui/vparse")
set(VPARSE_UI_TESTS_DIR   "${DJINTERP_ROOT}/tests/ui/vparse")


#-------------------------------------------------------------------------------
# the face
#-------------------------------------------------------------------------------
# vparse_ui_use_uxoxo(<target>)
#   Links uxoxo, and reaches its headers and Dear ImGui's as system
# directories: a dependency's code, not judged by this project's warning
# set. uxoxo's build exports its own as ordinary ones.
function(vparse_ui_use_uxoxo _target)
    target_link_libraries(${_target} PUBLIC uxoxo::uxoxo)
    target_include_directories(${_target} SYSTEM PUBLIC
        "${UXOXO_ROOT}/inc"
        "${VPARSE_IMGUI_ROOT}"
        "${VPARSE_IMGUI_ROOT}/backends")
endfunction()

# the face: the model, its view and update, the workbench's elements
add_library(vparse_workbench_face STATIC
    "${VPARSE_UI_INCLUDE_DIR}/elements.hpp"
    "${VPARSE_UI_INCLUDE_DIR}/view.hpp"
    "${VPARSE_UI_SOURCE_DIR}/elements.cpp"
    "${VPARSE_UI_SOURCE_DIR}/view.cpp")
add_library(vparse::workbench_face ALIAS vparse_workbench_face)

target_link_libraries(vparse_workbench_face PUBLIC vparse::workbench_core)
vparse_ui_use_uxoxo(vparse_workbench_face)
vparse_apply_compiler_defaults(vparse_workbench_face OWN
    CXX_STANDARD ${UXOXO_CXX_STANDARD})

# the face on Dear ImGui: the elements' renderers, and the theme
add_library(vparse_workbench_imgui STATIC
    "${VPARSE_UI_SOURCE_DIR}/elements_imgui.cpp")
add_library(vparse::workbench_imgui ALIAS vparse_workbench_imgui)

target_link_libraries(vparse_workbench_imgui PUBLIC vparse::workbench_face)
vparse_ui_use_uxoxo(vparse_workbench_imgui)
vparse_apply_compiler_defaults(vparse_workbench_imgui OWN
    CXX_STANDARD ${UXOXO_CXX_STANDARD})

foreach(_library vparse_workbench_face
                 vparse_workbench_imgui)
    vparse_output(${_library})
    set_target_properties(${_library} PROPERTIES FOLDER "vparse/ui")
endforeach()


#-------------------------------------------------------------------------------
# the host
#-------------------------------------------------------------------------------
set(_host "${VPARSE_UI_HOST}")

if(_host STREQUAL "auto")
    if(TARGET uxoxo::imgui_win32_dx11)
        set(_host "win32")
    elseif(TARGET uxoxo::imgui_glfw_opengl3)
        set(_host "glfw")
    else()
        set(_host "none")
    endif()
endif()

if(_host STREQUAL "win32")
    if(NOT TARGET uxoxo::imgui_win32_dx11)
        message(FATAL_ERROR
            "VPARSE_UI_HOST is win32, which is Windows' alone. Choose glfw, "
            "none, or auto.")
    endif()

    set(_host_libraries uxoxo::imgui_win32_dx11)
elseif(_host STREQUAL "glfw")
    if(NOT TARGET uxoxo::imgui_glfw_opengl3)
        message(FATAL_ERROR
            "VPARSE_UI_HOST is glfw, but uxoxo found no GLFW. Install GLFW "
            "3.3 or later (libglfw3-dev, brew install glfw, or vcpkg glfw3), "
            "or choose another host.")
    endif()

    # the host itself calls OpenGL: to clear, and to read a frame back
    find_package(OpenGL REQUIRED)

    set(_host_libraries uxoxo::imgui_glfw_opengl3 OpenGL::GL)
elseif(_host STREQUAL "none")
    set(_host_libraries "")
else()
    message(FATAL_ERROR
        "VPARSE_UI_HOST is '${VPARSE_UI_HOST}'; it must be auto, win32, "
        "glfw or none.")
endif()


#-------------------------------------------------------------------------------
# the module
#-------------------------------------------------------------------------------
# the window, its host, and the run function `vparse gui` calls, which reads
# the command line through the kit, as the commands read theirs
d_cli_add_plugin(vparse-gui
    SOURCES "${VPARSE_UI_INCLUDE_DIR}/host.hpp"
            "${VPARSE_UI_SOURCE_DIR}/host_${_host}.cpp"
            "${VPARSE_UI_SOURCE_DIR}/plugin.cpp")
target_link_libraries(vparse-gui PRIVATE
    vparse::workbench_imgui
    ${_host_libraries})
vparse_apply_compiler_defaults(vparse-gui OWN
    CXX_STANDARD ${UXOXO_CXX_STANDARD})
vparse_version(vparse-gui)
vparse_output(vparse-gui)
set_target_properties(vparse-gui PROPERTIES FOLDER "vparse/ui")

# `vparse gui` loads the module the build made with it
add_dependencies(vparse vparse-gui)

message(STATUS "vparse: the window's host is ${_host}")


#-------------------------------------------------------------------------------
# tests
#-------------------------------------------------------------------------------
if(VPARSE_UI_BUILD_TESTS)
    include("${CMAKE_CURRENT_LIST_DIR}/cmake/UiTests.cmake")
endif()
