# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/text/text_intern/CMakeLists.txt
#
#   Test leaf for c/text/text_intern.h, names stood for by small integers, in
# the C spec form: the runner in this directory, the section translation units
# globbed from the tests tree, the source under test, and DTest's C face, from
# djinterp_c_test.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.17
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/text")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/text_intern_tests_*.c")

djinterp_add_test_executable(c_text_intern_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/text_intern_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/text/text_intern.c"
    LIBRARIES
        djinterp_c_test
    INCLUDES
        "${_tests_dir}")
