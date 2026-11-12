# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/modules/test/test_timer/CMakeLists.txt
#
#   Test leaf for DTest's C timer, c/modules/test/test_timer.h and its tie-in
# test_timer_event.h, in the C spec form: the runner in this directory, the
# section translation units globbed from the tests tree, the two sources
# under test, the timer family's timer and event binding (and the counter's
# bounds arithmetic, which the timer uses), the event core's tier-0 sources,
# dtime.c for the monotonic clock, and DTest's C face, which
# djinterp_c_runtime does not yet carry.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.11
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/modules/test")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/test_timer_tests_*.c")

djinterp_add_test_executable(c_test_timer_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/test_timer_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_timer.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_timer_event.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/timer/timer.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/timer/timer_event.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/counter/counter.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/event/event_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/event/event_handler_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/event/event_table_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/event/event_registry_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/event/event_dispatcher_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/dtime.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/string_fn.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
