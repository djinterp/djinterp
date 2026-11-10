# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/text/
#                      text_template/CMakeLists.txt
#
#   Test leaf for text_template.h's suite, in DTest's C face (converted from
# the standalone tier on 2026.11.09): the runner in this directory, the
# suite's translation units globbed from the tests tree, the unit under test
# and the one it builds on, and DTest's C face. text_template_overflow_tests.c
# is a suite of its own (its leaf is text_template_overflow/), and the glob
# below does not reach it: it matches text_template_tests*.c only.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.09
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/text")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/text_template_tests*.c")

djinterp_add_test_executable(c_text_template_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/text_template_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/text/text_template.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/text/str_interp.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/string_fn.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
