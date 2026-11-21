# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/text/
#                      ostream_template_refuse/CMakeLists.txt
#
#   Test leaf for the template engine's refusing UTF-8 knob, in DTest's C face:
# the runner in this directory, the suite's translation unit, and the engine
# compiled with D_CFG_OSTREAM_TEMPLATE_UTF8_REFUSE (its value, 1, since the
# knob must be an integer literal). The runtime has no copy of the engine, so
# this one is the only one linked.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.13
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/text")

djinterp_add_test_executable(c_ostream_template_refuse_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/ostream_template_refuse_tests_runner.c"
        "${_tests_dir}/ostream_template_refuse_tests.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/text/ostream_template.c"
    DEFINES
        D_CFG_OSTREAM_TEMPLATE_UTF8=1
    LIBRARIES
        djinterp_c_test
    INCLUDES
        "${_tests_dir}")
