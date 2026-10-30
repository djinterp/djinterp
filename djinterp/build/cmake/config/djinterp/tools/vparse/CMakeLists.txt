#*******************************************************************************
# vparse [vparse]                                                 CMakeLists.txt
#
# vparse, built as one of djinterp's tools: the library in parsegen/vparse
# with its suites, and its program `vparse`, the command line, with theirs.
# Its window, the workbench on uxoxo and Dear ImGui, is outside djinterp --
# its sources in inc/ui, src/ui and tests/ui, its build in build/cmake/config/ui/vparse,
# which builds this one within it and makes the module `vparse gui` loads
# (GRAMMAR.md, section 7) -- so nothing here needs uxoxo.
#   vparse lives in djinterp's tree. The library is inc/, src/ and
# tests/djinterp/cpp/modules/parsegen/vparse: the PEG machine, the generator,
# both frontends and the session. The tool is tools/vparse: its model's headers
# in inc/djinterp/tools/vparse, its sources in src/djinterp/tools/vparse with
# the command line under cli/, its suites in tests/djinterp/tools/vparse.
# This build makes all of it, from here, the way bitwisedom's build makes
# bitwisedom:
#
#     djinterp::c_base            the C root's dstdint.c and sink_common.c
#     djinterp::parse_foundation  the parse/parsegen C foundation, at C99
#     djinterp::arg_parser        the argument parser, CLI11 in C, at C99
#     djinterp::cli               the command-line kit over it, tools/cli,
#                                 with filesys, at C99: made by the kit's
#                                 own module, DjinterpCli.cmake, as
#                                 bitwisedom's build makes it
#     djinterp::vparse            the library, the session with it
#     vparse::workbench_core      runs, the store, the heat map: a grammar
#                                 read, judged by the session, built and run
#                                 with its trace. No uxoxo, no Dear ImGui.
#     vparse::cli                 the command line: its subcommands on the
#                                 kit, over the model
#     vparse                      the program `vparse`, whose `gui` command
#                                 loads the window's module where
#                                 build/cmake/config/ui/vparse made it
#
# and, under VPARSE_BUILD_TESTS, the suites of the model and the command
# line, the program run over an example, and djinterp's own suites for what
# the tool stands on -- the foundation, the argument parser and vparse -- at
# every level from their floors (tests/CMakeLists.txt).
#   djinterp's root is the one this file sits five directories below. The
# binaries go to <djinterp>/bin/tools/vparse/, with a platform suffix
# (cmake/OutputLayout.cmake).
#
# path:      /build/cmake/config/djinterp/tools/vparse/CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
#                                                            revised: 2026.10.26
#*******************************************************************************
cmake_minimum_required(VERSION 3.21)

project(vparse
    VERSION     0.3.0
    DESCRIPTION "An optimizing runtime parser generator, with two programs"
    LANGUAGES   C CXX)

# Only the top-level build sets global policy; as a subproject it stays quiet.
if(PROJECT_IS_TOP_LEVEL)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    enable_testing()

    # one runtime library across every target, or the link fails at the end
    if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
        set(CMAKE_MSVC_RUNTIME_LIBRARY
            "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif()
endif()


#-------------------------------------------------------------------------------
# options
#-------------------------------------------------------------------------------
option(VPARSE_BUILD_CLI          "Build vparse, the command line"         ON)
option(VPARSE_BUILD_TESTS        "Build the test suites"                  ON)
option(VPARSE_WARNINGS_AS_ERRORS "Treat warnings as errors"               OFF)

# VPARSE_BUILD_DJINTERP_TESTS
#   djinterp's own suites for the code this build takes from it -- the
# parse/parsegen foundation, the argument parser and vparse -- run from the
# leaves in djinterp's tree, at every language level. ON by default, so one
# ctest covers everything the tool stands on; needs VPARSE_BUILD_TESTS.
option(VPARSE_BUILD_DJINTERP_TESTS
       "Also build and run djinterp's foundation, arg_parser and vparse suites"
       ON)


#-------------------------------------------------------------------------------
# dependencies
#-------------------------------------------------------------------------------
include("${CMAKE_CURRENT_LIST_DIR}/cmake/ProjectRoots.cmake")   # the roots
include("${CMAKE_CURRENT_LIST_DIR}/cmake/OutputLayout.cmake")   # bin/tools
include("${CMAKE_CURRENT_LIST_DIR}/cmake/CompilerSetup.cmake")  # levels, flags
include("${CMAKE_CURRENT_LIST_DIR}/cmake/Djinterp.cmake")       # djinterp

# VPARSE_VERSION_TEXT
#   the build's version, which the window's build, building this one within
# it, gives its module: a module belongs to its program at one version.
set(VPARSE_VERSION_TEXT "${PROJECT_VERSION}" CACHE INTERNAL
    "vparse's version, as --version prints it")


#-------------------------------------------------------------------------------
# the model: what both programs stand on
#-------------------------------------------------------------------------------
# vparse_version(<target>)
#   Gives a program's units the build's version, which --version prints --
# and a module's, which its program holds to its own.
function(vparse_version _target)
    target_compile_definitions(${_target} PRIVATE
        VPARSE_VERSION="${VPARSE_VERSION_TEXT}")
endfunction()

if(VPARSE_BUILD_CLI)
    if(NOT TARGET djinterp::arg_parser)
        message(FATAL_ERROR
            "vparse's command line reads its arguments with djinterp's "
            "argument parser, and ${DJINTERP_ROOT} has none under "
            "src/djinterp/parse/cli/arg_parser/c/. Use a djinterp with "
            "parse/cli/arg_parser in it, or configure with "
            "-DVPARSE_BUILD_CLI=OFF to build vparse's library and its suites "
            "alone.")
    endif()

    # the engine: runs, the store, the heat map
    add_library(vparse_workbench_core STATIC
        "${VPARSE_SOURCE_DIR}/heat.cpp"
        "${VPARSE_SOURCE_DIR}/run.cpp"
        "${VPARSE_SOURCE_DIR}/store.cpp")
    add_library(vparse::workbench_core ALIAS vparse_workbench_core)

    # PUBLIC: the engine's headers show djinterp's vparse types and its ui's
    target_link_libraries(vparse_workbench_core PUBLIC djinterp::vparse)
    vparse_apply_compiler_defaults(vparse_workbench_core OWN
        CXX_STANDARD ${VPARSE_TOOL_CXX_STANDARD})
    vparse_output(vparse_workbench_core)
    set_target_properties(vparse_workbench_core PROPERTIES FOLDER "vparse")
endif()


#-------------------------------------------------------------------------------
# the command line
#-------------------------------------------------------------------------------
if(VPARSE_BUILD_CLI)
    # the kit the command line is made on: its module makes djinterp::cli
    # over the argument parser made above, which it does not make again
    set(_vparse_kit
        "${DJINTERP_ROOT}/build/cmake/config/djinterp/tools/cli/DjinterpCli.cmake")

    if(NOT EXISTS "${_vparse_kit}")
        message(FATAL_ERROR
            "vparse's command line is made on djinterp's command-line kit, "
            "and ${DJINTERP_ROOT} has none "
            "(build/cmake/djinterp/tools/cli/DjinterpCli.cmake is missing). "
            "Use a djinterp of 2026.10.22 or later, or configure with "
            "-DVPARSE_BUILD_CLI=OFF.")
    endif()

    include("${_vparse_kit}")

    # the subcommands, as a library the suite runs in-process
    add_library(vparse_cli STATIC
        "${VPARSE_SOURCE_DIR}/cli/cli.cpp"
        "${VPARSE_SOURCE_DIR}/cli/cli.hpp"
        "${VPARSE_SOURCE_DIR}/cli/commands.cpp"
        "${VPARSE_SOURCE_DIR}/cli/commands.hpp"
        "${VPARSE_SOURCE_DIR}/cli/common.cpp"
        "${VPARSE_SOURCE_DIR}/cli/common.hpp"
        "${VPARSE_SOURCE_DIR}/cli/measures.cpp"
        "${VPARSE_SOURCE_DIR}/cli/report.cpp"
        "${VPARSE_SOURCE_DIR}/cli/report.hpp")
    add_library(vparse::cli ALIAS vparse_cli)

    target_link_libraries(vparse_cli PUBLIC
        vparse::workbench_core
        djinterp::cli)
    vparse_apply_compiler_defaults(vparse_cli OWN
        CXX_STANDARD ${VPARSE_TOOL_CXX_STANDARD})
    vparse_version(vparse_cli)

    # the program
    add_executable(vparse
        "${VPARSE_SOURCE_DIR}/cli/main.cpp")

    target_link_libraries(vparse PRIVATE vparse::cli)
    vparse_apply_compiler_defaults(vparse OWN
        CXX_STANDARD ${VPARSE_TOOL_CXX_STANDARD})

    foreach(_target vparse_cli vparse)
        vparse_output(${_target})
        set_target_properties(${_target} PROPERTIES FOLDER "vparse")
    endforeach()
endif()


#-------------------------------------------------------------------------------
# tests
#-------------------------------------------------------------------------------
# here or within the window's build (build/cmake/config/ui/vparse), which enables testing
if(VPARSE_BUILD_TESTS)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/tests"
                     "${CMAKE_CURRENT_BINARY_DIR}/tests")
endif()


#-------------------------------------------------------------------------------
# what was configured
#-------------------------------------------------------------------------------
if(PROJECT_IS_TOP_LEVEL)
    message(STATUS "")
    message(STATUS "vparse ${PROJECT_VERSION}")
    message(STATUS "  compiler   : ${CMAKE_CXX_COMPILER_ID} "
                   "${CMAKE_CXX_COMPILER_VERSION}")
    message(STATUS "  levels     : C99, C++${VPARSE_CXX_STANDARD}; the tool "
                   "C++${VPARSE_TOOL_CXX_STANDARD}")
    message(STATUS "  djinterp   : ${DJINTERP_ROOT}")
    message(STATUS "  vparse     : ${VPARSE_BUILD_CLI} (the command line)")

    if(VPARSE_BUILD_CLI)
        message(STATUS "  binaries   : ${VPARSE_BIN_DIR}")
    endif()

    message(STATUS "  tests      : ${VPARSE_BUILD_TESTS}")
    message(STATUS "")
endif()
