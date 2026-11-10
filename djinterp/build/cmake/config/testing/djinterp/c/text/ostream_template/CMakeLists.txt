# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/text/
#                      ostream_template/CMakeLists.txt
#
#   Test leaf for the template engine's suite, in DTest's C face: the runner in
# this directory, the suite's translation units globbed from the tests tree,
# the engine, and DTest's C face.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.09
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/text")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/ostream_template_tests*.c")

djinterp_add_test_executable(c_ostream_template_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/ostream_template_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/text/ostream_template.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
