# =============================================================================
# djinterp [test] -
# build/cmake/config/testing/djinterp/c/util/dregex/CMakeLists.txt
#
#   Test leaf for c/util/dregex, in DTest's C face, two CTest tests with
# their runners in this directory:
#     - c_dregex_tests: the engine's behaviour, leftmost-longest first, and
#       its capture groups, against dregex.c.
#     - c_dregex_sizes_tests: every allocation size that would wrap, white
#       box: its suite includes dregex.c, so the target links no copy of it.
# fuzz_dregex.c, a differential run against the system's <regex.h>, stays a
# program of its own.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.19
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/util/dregex")
set(_dtest     "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c")

djinterp_add_test_executable(c_dregex_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/dregex_tests_runner.c"
        "${_tests_dir}/dregex_tests.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/dregex.c"
        "${_dtest}"
    INCLUDES  "${_tests_dir}")

djinterp_add_test_executable(c_dregex_sizes_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/dregex_sizes_tests_runner.c"
        "${_tests_dir}/dregex_sizes_tests.c"
        "${_dtest}"
    INCLUDES  "${_tests_dir}")
