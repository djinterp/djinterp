# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/cpp/modules/test/test_suite/CMakeLists.txt
#
#   Test leaf for DTest's C face, c/modules/test/test_suite.h, as C++ suites
# use it: the runner in this directory, which hands the suite's spec to C++'s
# run_module, the section translation units globbed from the tests tree, and
# the C face's one source, which djinterp_c_runtime does not yet carry. The C
# face's own suite is c_test_suite_tests, in c/modules/test/test_suite.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.15
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/cpp/modules/test")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/test_suite_tests_*.cpp")
list(FILTER _sections EXCLUDE REGEX "_tests_(runner|main)\\.cpp$")

djinterp_add_test_executable(test_suite_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/test_suite_tests_runner.cpp"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
