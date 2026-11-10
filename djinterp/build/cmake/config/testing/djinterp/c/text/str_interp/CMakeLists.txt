# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/text/
#                      str_interp/CMakeLists.txt
#
#   Test leaf for str_interp.h's suite, in DTest's C face (converted from the
# standalone tier on 2026.11.09): the runner in this directory, the suite's
# translation units globbed from the tests tree, the unit under test, and
# DTest's C face.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.09
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/text")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/str_interp_tests*.c")

djinterp_add_test_executable(c_str_interp_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/str_interp_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/text/str_interp.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/string_fn.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
