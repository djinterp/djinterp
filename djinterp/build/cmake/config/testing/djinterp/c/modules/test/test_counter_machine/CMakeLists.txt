# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/modules/test/test_counter_machine/CMakeLists.txt
#
#   Test leaf for section 4 of c/modules/test/test_counter.h, the counters'
# opt-in abstract machine, which compiles only with D_CFG_TEST_COUNTER_MACHINE:
# this leaf sets it. The runner in this directory, the section translation
# units globbed from the tests tree, test_counter.c and the counter it builds
# on, option_env.c and the option core it stands on, and DTest's C face,
# which djinterp_c_runtime does not yet carry.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.11
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/test")

file(GLOB _sections CONFIGURE_DEPENDS
     "${_tests_dir}/test_counter_machine_tests_*.c")

djinterp_add_test_executable(c_test_counter_machine_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/test_counter_machine_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_counter.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/counter/counter.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/option/option_env.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/option/option_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/option/option_set_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/string_fn.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    DEFINES
        D_CFG_TEST_COUNTER_MACHINE=1
    INCLUDES
        "${_tests_dir}")
