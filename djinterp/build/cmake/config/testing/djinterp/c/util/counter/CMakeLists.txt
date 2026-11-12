# =============================================================================
# djinterp [test]  --  build/cmake/config/testing/djinterp/c/util/counter/CMakeLists.txt
#
#   Test leaf for c/util/counter -- counter.h, counter_atomic.h,
# counter_mutex.h and counter_event.h -- in the C spec form: the runner in
# this directory, the section translation units globbed from the tests tree,
# the four sources under test, the event core's tier-0 sources the bindings
# fire through, datomic and dmutex for the atomic and guarded counters and
# their threads, and DTest's C face, which djinterp_c_runtime does not yet
# carry.
#
# author(s): Samuel 'teer' Neal-Blim                            date: 2026.11.09
# =============================================================================

set(_tests_dir "${DJINTERP_TESTS_ROOT}/djinterp/c/util/counter")

file(GLOB _sections CONFIGURE_DEPENDS "${_tests_dir}/counter_tests_*.c")

djinterp_add_test_executable(c_counter_tests
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/counter_tests_runner.c"
        ${_sections}
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/counter/counter.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/counter/counter_atomic.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/counter/counter_mutex.c"
        "${DJINTERP_SRC_ROOT}/djinterp/c/util/counter/counter_event.c"
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
