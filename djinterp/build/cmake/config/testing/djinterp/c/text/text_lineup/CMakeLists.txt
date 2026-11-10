# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/text/
#                      text_lineup/CMakeLists.txt
#
#   Test leaf for text_lineup.h's suite, in DTest's C face (the module was
# restored and ported on 2026.11.09): the runner in this directory, the
# suite's translation units globbed from the tests tree, the unit under test,
# the text buffer it appends to and what that builds on, and DTest's C face.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.09
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/text")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/text_lineup_tests*.c")

djinterp_add_test_executable(c_text_lineup_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/text_lineup_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/text/text_lineup.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/container/buffer/text_buffer.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/container/buffer/buffer_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/functional/filter.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/dstring.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/string_fn.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/dio.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
