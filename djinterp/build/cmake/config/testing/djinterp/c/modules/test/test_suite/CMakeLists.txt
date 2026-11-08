# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/modules/test/test_suite/CMakeLists.txt
#
#   Test leaf for DTest's C face, c/modules/test/test_suite.h, in the C spec
# form: the runner in this directory, the section translation units globbed
# from the tests tree, and the module's one source, which djinterp_c_runtime
# does not yet carry (a request to lane A).
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.05
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/test")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/test_suite_tests_*.c")

djinterp_add_test_executable(c_test_suite_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/test_suite_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
