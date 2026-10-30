#*******************************************************************************
# bitwisedom [bitwisedom]                                         CMakeLists.txt
#
# bitwisedom: a bitfield manager on djinterp -- the model and the command
# line. The window is not built here: it is uxoxo's, and lives outside the
# djinterp tree, in build/cmake/config/ui/bitwisedom, whose build includes this one.
#   The build is vparse's shape: the repository's root is a cache variable
# (cmake/ProjectRoots.cmake), djinterp is consumed as headers, its C/C++
# lexer and its command-line kit, compiled as C (cmake/Djinterp.cmake), and
# this project's own sources are held to the style guide's whole baseline
# (cmake/CompilerSetup.cmake). Each configure marks the sources that changed
# since the last one to compile again, whatever their times say
# (cmake/Freshness.cmake). Targets:
#
#     bitwisedom::core   the model -- sheets, resolution, operations, the C/C++
#                       generator and its self-tests, the C environment and
#                       importer, the settings and the edits, and the file
#                       format -- with no window in it
#     bitwisedom::cli    the command line's commands, on djinterp's kit
#     bitwisedom         the command line (GRAMMAR.md, section 10); `gui`
#                       loads the window's module, built in build/cmake/config/ui
#     bitwisedom-gen     the generator's command line, as build scripts have
#                       always called it
#
# and, under BITWISEDOM_BUILD_TESTS, the model's suite at every level from the
# floor, the generated headers compiled at every C and C++ level the compiler
# can select, and the command line's transcripts.
#
# path:      /build/cmake/config/djinterp/tools/bitwisedom/CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.04
#                                                            revised: 2026.10.24
#*******************************************************************************
cmake_minimum_required(VERSION 3.21)

project(bitwisedom
    VERSION     0.1.0
    DESCRIPTION "A bitfield manager: symbols mapped to values, as C/C++ macros"
    LANGUAGES   C CXX)

option(BITWISEDOM_BUILD_TESTS        "Build the tests"                  ON)
option(BITWISEDOM_WARNINGS_AS_ERRORS "Treat warnings as errors"         OFF)

# the version every part is built at -- the command line's, and the one the
# window's module must match -- for build/cmake/config/ui, which includes this build
set(BITWISEDOM_VERSION "${PROJECT_VERSION}" CACHE INTERNAL
    "bitwisedom's version, as the command line and its modules are built")

include("${CMAKE_CURRENT_LIST_DIR}/cmake/ProjectRoots.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/OutputLayout.cmake")

set(BITWISEDOM_SOURCE_DIR
    "${BITWISEDOM_ROOT}/src/djinterp/tools/bitwisedom")
set(BITWISEDOM_TESTS_DIR
    "${BITWISEDOM_ROOT}/tests/djinterp/tools/bitwisedom")
set(BITWISEDOM_EXAMPLES_DIR
    "${BITWISEDOM_ROOT}/res/djinterp/tools/bitwisedom/examples")

# the sources that changed since this build tree last configured -- an
# overlay unzipped over the trees keeps its archive's times, which can be
# older than the last build's objects -- marked to compile again
include("${CMAKE_CURRENT_LIST_DIR}/cmake/Freshness.cmake")
bitwisedom_freshen("${CMAKE_BINARY_DIR}/bitwisedom_sources.sha1"
                   "${BITWISEDOM_SOURCE_DIR}"
                   "${BITWISEDOM_ROOT}/inc/djinterp/tools/bitwisedom"
                   "${BITWISEDOM_TESTS_DIR}"
                   "${BITWISEDOM_ROOT}/inc/djinterp/cpp/modules/parse/cli"
                   "${BITWISEDOM_ROOT}/src/djinterp/c/modules/parse/cli"
                   "${BITWISEDOM_ROOT}/inc/djinterp/tools/cli"
                   "${BITWISEDOM_ROOT}/src/djinterp/tools/cli")

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
    "${BITWISEDOM_SOURCE_DIR}/edit.cpp"
    "${BITWISEDOM_SOURCE_DIR}/examples.cpp"
    "${BITWISEDOM_SOURCE_DIR}/exchange.cpp"
    "${BITWISEDOM_SOURCE_DIR}/expr.cpp"
    "${BITWISEDOM_SOURCE_DIR}/history.cpp"
    "${BITWISEDOM_SOURCE_DIR}/json.cpp"
    "${BITWISEDOM_SOURCE_DIR}/ops.cpp"
    "${BITWISEDOM_SOURCE_DIR}/persist.cpp"
    "${BITWISEDOM_SOURCE_DIR}/rename.cpp"
    "${BITWISEDOM_SOURCE_DIR}/resolve.cpp"
    "${BITWISEDOM_SOURCE_DIR}/selftest.cpp"
    "${BITWISEDOM_SOURCE_DIR}/settings.cpp"
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
    target_compile_definitions(${_name} PUBLIC
        "BITWISEDOM_VERSION=\"${BITWISEDOM_VERSION}\"")
    bitwisedom_apply_compiler_defaults(${_name} OWN CXX_STANDARD ${_level})
endfunction()

bitwisedom_add_core(bitwisedom_core ${BITWISEDOM_CXX_STANDARD})
add_library(bitwisedom::core ALIAS bitwisedom_core)


#-------------------------------------------------------------------------------
# the command line
#-------------------------------------------------------------------------------
# the commands, on djinterp's kit; then the two programs over them
add_library(bitwisedom_cli STATIC
    "${BITWISEDOM_SOURCE_DIR}/cli/cli_common.cpp"
    "${BITWISEDOM_SOURCE_DIR}/cli/cli_edit.cpp"
    "${BITWISEDOM_SOURCE_DIR}/cli/cli_main.cpp"
    "${BITWISEDOM_SOURCE_DIR}/cli/cli_read.cpp"
    "${BITWISEDOM_SOURCE_DIR}/cli/cli_rows.cpp")
add_library(bitwisedom::cli ALIAS bitwisedom_cli)
bitwisedom_output(bitwisedom_cli)
target_link_libraries(bitwisedom_cli PUBLIC bitwisedom::core djinterp::cli)
bitwisedom_apply_compiler_defaults(bitwisedom_cli OWN)

add_executable(bitwisedom
    "${BITWISEDOM_SOURCE_DIR}/tools/bitwisedom_main.cpp")
bitwisedom_output(bitwisedom)
target_link_libraries(bitwisedom PRIVATE bitwisedom::cli)
bitwisedom_apply_compiler_defaults(bitwisedom OWN)

add_executable(bitwisedom-gen
    "${BITWISEDOM_SOURCE_DIR}/tools/bitwisedom_gen.cpp")
bitwisedom_output(bitwisedom-gen)
target_link_libraries(bitwisedom-gen PRIVATE bitwisedom::cli)
bitwisedom_apply_compiler_defaults(bitwisedom-gen OWN)


#-------------------------------------------------------------------------------
# tests
#-------------------------------------------------------------------------------
if(BITWISEDOM_BUILD_TESTS)
    add_subdirectory("${BITWISEDOM_TESTS_DIR}"
                     "${CMAKE_BINARY_DIR}/tests")
endif()
