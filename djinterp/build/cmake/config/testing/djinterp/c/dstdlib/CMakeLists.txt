# =============================================================================
# djinterp [test] - testing/djinterp/c/dstdlib/CMakeLists.txt
#
#   Test leaf for dstdlib.h's suite, in DTest's C face: the runner in this
# directory and the suite's one section. dstdlib.c is the runtime's, which
# every test links, and DTest's C face is djinterp_c_test.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.26
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c")

djinterp_add_test_executable(c_dstdlib_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/dstdlib_tests_runner.c"
        "${_tests_dir}/dstdlib_tests.c"
    INCLUDES
        "${_tests_dir}"
    LIBRARIES
        djinterp_c_test)
