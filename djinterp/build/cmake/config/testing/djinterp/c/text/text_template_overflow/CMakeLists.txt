# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/text/
#                      text_template_overflow/CMakeLists.txt
#
#   Test leaf for text_template.c's overflow suite, in DTest's C face: the
# runner in this directory and the suite's translation unit, which includes
# text_template.c whole (so it is not listed here), the units text_template.c
# needs, and DTest's C face. Run it under -fsanitize=address as well, as
# lane A is asked to in scripts/ci/check_ckd.sh.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.09
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/text")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/text_template_overflow_tests*.c")

djinterp_add_test_executable(c_text_template_overflow_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/text_template_overflow_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/text/str_interp.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/string_fn.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
