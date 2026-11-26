# =============================================================================
# djinterp [test]  -  build/cmake/config/testing/djinterp/c/text/symbol/ascii/CMakeLists.txt
#
#   Test leaf for c/text/symbol/ascii.h, ASCII's classes and case with no
# locale, in the C spec form: the runner in this directory, the section
# translation unit from the tests tree, the source under test, and DTest's
# C face, from djinterp_c_test.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.23
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/text/symbol")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/ascii_tests_*.c")

djinterp_add_test_executable(c_ascii_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/ascii_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/text/symbol/ascii.c"
    LIBRARIES
        djinterp_c_test
    INCLUDES
        "${_tests_dir}")
