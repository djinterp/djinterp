# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/util/timer/CMakeLists.txt
#
#   Test leaf for c/util/timer -- timer.h, timer_atomic.h, timer_mutex.h and
# timer_event.h -- in the C spec form: the runner in this directory, the
# section translation units globbed from the tests tree, the four sources
# under test, counter.c for the arithmetic the timer shares, dtime for the
# monotonic clock, the event core's tier-0 sources the bindings fire through,
# datomic and dmutex for the atomic and guarded timers and their threads,
# and DTest's C face, which djinterp_c_runtime does not yet carry.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.09
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/util/timer")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/timer_tests_*.c")

djinterp_add_test_executable(c_timer_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/timer_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/timer/timer.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/timer/timer_atomic.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/timer/timer_mutex.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/timer/timer_event.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/counter/counter.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/event/event_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/event/event_handler_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/event/event_table_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/event/event_registry_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/event/event_dispatcher_common.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/sync/datomic.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/sync/dmutex.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/dtime.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/string_fn.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/memory/dmemory.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/modules/test/test_suite.c"
    INCLUDES
        "${_tests_dir}")
