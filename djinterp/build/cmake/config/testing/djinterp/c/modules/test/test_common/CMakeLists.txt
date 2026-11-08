# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/modules/test/test_common/CMakeLists.txt
#
#   Test leaf for c/modules/test/test_common.h, the DTest vocabulary, in the C
# spec form: the runner in this directory and the section translation units
# globbed from the tests tree. test_common.h is header-only, so the only source
# is DTest's C face, which djinterp_c_runtime does not yet carry (a request to
# lane A).
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.06
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/test")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/test_common_tests_*.c")

djinterp_add_test_executable(c_test_common_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/test_common_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
