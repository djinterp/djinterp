# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/modules/interp/CMakeLists.txt
#
#    Test leaf for modules/interp/djin.h, in DTest's C face: the runner in this
# directory, the suite's units globbed from the tests tree, the source under
# test and what it reaches -- option_env and its registry, the option core
# under them -- and DTest's C face, which djinterp_c_runtime does not yet
# carry. The runtime carries sink_common.c, which the text writes through.
#    Run it with
#        ctest -R c_djin_tests
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.11
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/interp")
set(_option    "${DJINTERP_SRC_ROOT}/djinterp/c/option")
file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/djin_tests*.c")

djinterp_add_test_executable(c_djin_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/djin_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/interp/djin.c"
        "${_option}/option_common.c"
        "${_option}/option_env.c"
        "${_option}/option_env_registry.c"
        "${_option}/option_set.c"
        "${_option}/option_set_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
