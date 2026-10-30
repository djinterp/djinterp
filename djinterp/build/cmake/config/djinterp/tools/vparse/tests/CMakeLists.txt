#*******************************************************************************
# vparse [tests]                                                  CMakeLists.txt
#
# One `ctest` for what this build produces and what it stands on: the tool's
# suites and its program, and djinterp's own suites for the code the tool
# runs on.
#   THE TOOL'S SUITES are built where the tool is, at its levels; their sources
# are in tests/djinterp/tools/vparse. The model's links the model alone, the
# command line's the command line, and neither needs uxoxo, Dear ImGui or a
# display. The program is then run as a test of its own that needs no
# display either: vparse over the example grammar. The window's suites are
# build/cmake/config/ui/vparse's, with the window.
#   DJINTERP'S SUITES are not rebuilt here. Its tree carries test leaves
# written in plain CMake, which stand alone or are added from another
# project; they are added as they are, and bring their own levels, their own
# counts and their own library: the parse/parsegen foundation's, the argument
# parser's, and vparse's. The leaves build their libraries a second time, on
# purpose: the suites need the testing configuration, D_TESTING=1 changes the
# layout of struct d_parse_machine, and a library and everything linking it
# must agree on that. The libraries the programs link are the production
# ones (cmake/Djinterp.cmake); the two never meet.
#
# path:      /build/cmake/config/djinterp/tools/vparse/tests/CMakeLists.txt
# link(s):   TBA
# author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.28
#                                                            revised: 2026.10.26
#*******************************************************************************


# vparse_suite(<target> <sections> <source> <library> <level>)
#   Builds one of the tool's suites from its source, at the C++ level of the
# library it tests and linked to it, and runs it in the build tree's test
# directory, where it writes its files. The suite must print exactly its
# count of sections, so a section that silently stopped being compiled fails
# rather than reading as a smaller pass.
function(vparse_suite _target _sections _source _library _level)
    add_executable(${_target} "${VPARSE_TESTS_DIR}/${_source}")
    target_link_libraries(${_target} PRIVATE ${_library})

    # a suite reaches a program's headers as app/... and cli/..., as
    # bitwisedom's do
    target_include_directories(${_target} PRIVATE "${VPARSE_SOURCE_DIR}")
    vparse_apply_compiler_defaults(${_target} OWN CXX_STANDARD ${_level})

    set_target_properties(${_target} PROPERTIES FOLDER "vparse/tests")

    add_test(NAME ${_target} COMMAND ${_target}
             WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
    set_tests_properties(${_target} PROPERTIES
        PASS_REGULAR_EXPRESSION "passed: ${_sections}   failed: 0"
        FAIL_REGULAR_EXPRESSION "\\[FAIL\\]")
endfunction()


#-------------------------------------------------------------------------------
# the model, and the command line: their suites, and the program
#-------------------------------------------------------------------------------
if(TARGET vparse::workbench_core)
    vparse_suite(vparse_workbench_core_tests 8 workbench_core_tests.cpp
                 vparse::workbench_core ${VPARSE_TOOL_CXX_STANDARD})
endif()

if(TARGET vparse::cli)
    vparse_suite(vparse_cli_tests 17 cli_tests.cpp
                 vparse::cli ${VPARSE_TOOL_CXX_STANDARD})

    # the README whose JSON tables the suite holds every document to
    target_compile_definitions(vparse_cli_tests PRIVATE
        VPARSE_README="${DJINTERP_ROOT}/docs/djinterp/tools/vparse/README.md")

    # The program over the window's example, the grammar written here: it
    # matches, so the program's status alone is the test.
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/vparse_example.txt"
         "Sum  = Term More* ;\n"
         "More = '+' Term | '-' Term ;\n"
         "Term = [0-9]+ ;\n")

    add_test(NAME vparse_runs
             COMMAND vparse run vparse_example.txt 12+3-45 --text
             WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
endif()


#-------------------------------------------------------------------------------
# djinterp's suites, from its own leaves
#-------------------------------------------------------------------------------
if(NOT VPARSE_BUILD_DJINTERP_TESTS)
    return()
endif()

set(_dj_testing "${DJINTERP_ROOT}/build/cmake/config/testing/djinterp")
set(_dj_leaf    "${_dj_testing}/c/modules/parsegen/foundation")
set(_dj_args    "${_dj_testing}/c/modules/parse/cli/arg_parser")

# The leaves this build adds are the ones that came with the suites'
# down-port: each stands on plain CMake, and the foundation's carries the
# counts file both it and its script read.
if( (NOT EXISTS "${_dj_leaf}/expected_sections.txt")  OR
    (NOT EXISTS "${_dj_leaf}/CMakeLists.txt")         OR
    (NOT EXISTS "${DJINTERP_ROOT}/build/cmake/config/testing/djinterp/cpp/modules/parsegen/vparse/CMakeLists.txt") )
    message(FATAL_ERROR
        "djinterp's test leaves were not found under:\n    ${_dj_leaf}\n"
        "They came with the parse/parsegen suites. Restore them, or "
        "configure with -DVPARSE_BUILD_DJINTERP_TESTS=OFF to build the "
        "tool's suites alone.")
endif()

# The leaves keep a compiler's defaults but for the character set. Under a
# cl-style driver this build reads djinterp's headers one way everywhere
# (cmake/CompilerSetup.cmake), so the leaves' targets are given the same
# flags, as directory options: nothing above this line is affected, since a
# target takes them when it is created.
if(MSVC)
    add_compile_options(${VPARSE_CL_READS})

    foreach(_flag IN LISTS VPARSE_CL_READS_CXX)
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:${_flag}>)
    endforeach()

    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

set(CMAKE_FOLDER "djinterp/tests")

add_subdirectory("${DJINTERP_ROOT}/build/cmake/config/testing/djinterp/c/modules/parsegen/foundation"
                 "${CMAKE_BINARY_DIR}/_tests/djinterp/foundation")
add_subdirectory("${DJINTERP_ROOT}/build/cmake/config/testing/djinterp/cpp/modules/parsegen/vparse"
                 "${CMAKE_BINARY_DIR}/_tests/djinterp/vparse")

# The argument parser's leaf, where the programs are built: they read their
# command lines with it. It is being written, so a tree may hold the parser
# and not yet its leaf, and then the parser goes untested here, and says so.
if( (TARGET djinterp::arg_parser) AND
    (NOT EXISTS "${_dj_args}/CMakeLists.txt") )
    message(STATUS "vparse tests: djinterp's argument parser has no test "
                   "leaf at ${_dj_args}; its suites are left out")
elseif(TARGET djinterp::arg_parser)
    add_subdirectory("${_dj_args}"
                     "${CMAKE_BINARY_DIR}/_tests/djinterp/arg_parser")
endif()
