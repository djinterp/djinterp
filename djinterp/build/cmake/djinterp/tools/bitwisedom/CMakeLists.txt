#*******************************************************************************
# bitwisedom [bitwisedom]                                         CMakeLists.txt
#
# bitwisedom: a bitfield manager on djinterp, with a window on uxoxo and Dear
# ImGui.
#   The build is vparse's shape: the three projects' roots are cache
# variables (cmake/ProjectRoots.cmake), djinterp is consumed as headers and
# its C/C++ lexer, compiled as C (cmake/Djinterp.cmake), uxoxo builds itself
# and is added as a subproject
# (cmake/Uxoxo.cmake), and this project's own sources are held to the style
# guide's whole baseline (cmake/CompilerSetup.cmake). Targets:
#
#     bitwisedom::core   the model -- sheets, resolution, operations, the C/C++
#                       generator, the C environment and importer, and the
#                       file format -- with no GUI in it
#     bitwisedom-gen     the generator on the command line
#     bitwisedom         the window: Win32 + Direct3D 11 on Windows, GLFW +
#                       OpenGL 3 elsewhere (BITWISEDOM_HOST chooses)
#
# and, under BITWISEDOM_BUILD_TESTS, the model's suite at every level from the
# floor, the generated headers compiled at every C and C++ level the compiler
# can select, and the window driven headless through ImGui's null backend.
#
# path:      /build/cmake/djinterp/tools/bitwisedom/CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
#                                                            revised: 2026.10.11
#*******************************************************************************
cmake_minimum_required(VERSION 3.21)

project(bitwisedom
    VERSION     0.1.0
    DESCRIPTION "A bitfield manager: symbols mapped to values, as C/C++ macros"
    LANGUAGES   C CXX)

option(BITWISEDOM_BUILD_APP          "Build the window (needs uxoxo)"   ON)
option(BITWISEDOM_BUILD_TESTS        "Build the tests"                  ON)
option(BITWISEDOM_BUILD_UXOXO_TESTS  "Build uxoxo's own tests too"      OFF)
option(BITWISEDOM_WARNINGS_AS_ERRORS "Treat warnings as errors"         OFF)

set(BITWISEDOM_HOST "auto" CACHE STRING
    "The window's backend: auto (win32 on Windows, else glfw), win32, glfw")
set_property(CACHE BITWISEDOM_HOST PROPERTY STRINGS auto win32 glfw)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/ProjectRoots.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/OutputLayout.cmake")

set(BITWISEDOM_SOURCE_DIR
    "${BITWISEDOM_ROOT}/src/djinterp/tools/bitwisedom")
set(BITWISEDOM_TESTS_DIR
    "${BITWISEDOM_ROOT}/tests/djinterp/tools/bitwisedom")
set(BITWISEDOM_EXAMPLES_DIR
    "${BITWISEDOM_ROOT}/res/djinterp/tools/bitwisedom/examples")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/CompilerSetup.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/Djinterp.cmake")

if(BITWISEDOM_BUILD_TESTS)
    enable_testing()
endif()


#-------------------------------------------------------------------------------
# the model
#-------------------------------------------------------------------------------
set(BITWISEDOM_CORE_SOURCES
    "${BITWISEDOM_SOURCE_DIR}/bits.cpp"
    "${BITWISEDOM_SOURCE_DIR}/cenv.cpp"
    "${BITWISEDOM_SOURCE_DIR}/cimport.cpp"
    "${BITWISEDOM_SOURCE_DIR}/clex.cpp"
    "${BITWISEDOM_SOURCE_DIR}/codegen.cpp"
    "${BITWISEDOM_SOURCE_DIR}/examples.cpp"
    "${BITWISEDOM_SOURCE_DIR}/exchange.cpp"
    "${BITWISEDOM_SOURCE_DIR}/expr.cpp"
    "${BITWISEDOM_SOURCE_DIR}/history.cpp"
    "${BITWISEDOM_SOURCE_DIR}/json.cpp"
    "${BITWISEDOM_SOURCE_DIR}/ops.cpp"
    "${BITWISEDOM_SOURCE_DIR}/persist.cpp"
    "${BITWISEDOM_SOURCE_DIR}/rename.cpp"
    "${BITWISEDOM_SOURCE_DIR}/resolve.cpp"
    "${BITWISEDOM_SOURCE_DIR}/sheet.cpp")

# the large examples, as arrays of their bytes (cmake/EmbedExamples.cmake)
include("${CMAKE_CURRENT_LIST_DIR}/cmake/EmbedExamples.cmake")
bitwisedom_embed_examples("${BITWISEDOM_EXAMPLES_DIR}/large")

# bitwisedom_add_core(<name> <c++ level>)
#   The model, as a static library at one level.
function(bitwisedom_add_core _name _level)
    add_library(${_name} STATIC ${BITWISEDOM_CORE_SOURCES})
    bitwisedom_output(${_name})
    target_include_directories(${_name} PUBLIC "${BITWISEDOM_ROOT}/inc")
    target_include_directories(${_name} PRIVATE "${BITWISEDOM_GENERATED_DIR}")
    target_link_libraries(${_name} PUBLIC djinterp::headers djinterp::lex)
    bitwisedom_apply_compiler_defaults(${_name} OWN CXX_STANDARD ${_level})
endfunction()

bitwisedom_add_core(bitwisedom_core ${BITWISEDOM_CXX_STANDARD})
add_library(bitwisedom::core ALIAS bitwisedom_core)


#-------------------------------------------------------------------------------
# the generator
#-------------------------------------------------------------------------------
add_executable(bitwisedom-gen "${BITWISEDOM_SOURCE_DIR}/tools/bitwisedom_gen.cpp")
bitwisedom_output(bitwisedom-gen)
target_link_libraries(bitwisedom-gen PRIVATE bitwisedom::core)
bitwisedom_apply_compiler_defaults(bitwisedom-gen OWN)


#-------------------------------------------------------------------------------
# the window
#-------------------------------------------------------------------------------
set(BITWISEDOM_APP_SOURCES
    "${BITWISEDOM_SOURCE_DIR}/app/app.cpp"
    "${BITWISEDOM_SOURCE_DIR}/app/code.cpp"
    "${BITWISEDOM_SOURCE_DIR}/app/flow.cpp"
    "${BITWISEDOM_SOURCE_DIR}/app/grid.cpp"
    "${BITWISEDOM_SOURCE_DIR}/app/settings.cpp"
    "${BITWISEDOM_SOURCE_DIR}/app/theme.cpp"
    "${BITWISEDOM_SOURCE_DIR}/app/view.cpp"
    "${BITWISEDOM_SOURCE_DIR}/app/fonts/fonts.cpp")

if(BITWISEDOM_BUILD_APP)
    include("${CMAKE_CURRENT_LIST_DIR}/cmake/Uxoxo.cmake")

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

        add_executable(bitwisedom WIN32
            ${BITWISEDOM_APP_SOURCES}
            "${BITWISEDOM_SOURCE_DIR}/app/host_win32.cpp")
        # shell32: the files dropped on the window
        target_link_libraries(bitwisedom PRIVATE
            uxoxo::imgui_win32_dx11 shell32)
    elseif(_host STREQUAL "glfw")
        if(NOT TARGET uxoxo::imgui_glfw_opengl3)
            message(FATAL_ERROR
                "BITWISEDOM_HOST is glfw, but uxoxo found no GLFW. Install "
                "GLFW 3.3 or later (libglfw3-dev, brew install glfw, or vcpkg "
                "glfw3), or on Windows use BITWISEDOM_HOST=win32.")
        endif()

        find_package(OpenGL REQUIRED)

        add_executable(bitwisedom
            ${BITWISEDOM_APP_SOURCES}
            "${BITWISEDOM_SOURCE_DIR}/app/host_glfw.cpp")
        target_link_libraries(bitwisedom PRIVATE
            uxoxo::imgui_glfw_opengl3 OpenGL::GL)
    else()
        message(FATAL_ERROR
            "BITWISEDOM_HOST is '${BITWISEDOM_HOST}'; it must be auto, win32 "
            "or glfw")
    endif()

    bitwisedom_output(bitwisedom)
    target_link_libraries(bitwisedom PRIVATE bitwisedom::core uxoxo::uxoxo)
    target_include_directories(bitwisedom SYSTEM PRIVATE
        ${BITWISEDOM_UXOXO_INCLUDES})
    bitwisedom_apply_compiler_defaults(bitwisedom OWN)

    message(STATUS "bitwisedom: the window's host is ${_host}")
endif()


#-------------------------------------------------------------------------------
# tests
#-------------------------------------------------------------------------------
if(BITWISEDOM_BUILD_TESTS)
    add_subdirectory("${BITWISEDOM_TESTS_DIR}"
                     "${CMAKE_BINARY_DIR}/tests")
endif()
