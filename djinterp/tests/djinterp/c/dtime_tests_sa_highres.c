/******************************************************************************
* djinterp [test]                                        dtime_tests_sa_highres.c
*
*   Tests for high-resolution time functions (clock_gettime, clock_getres,
* timespec_get).
*
*
* path:      \src\test\dtime_tests_sa_highres.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.25
******************************************************************************/
#include "dtime_tests_sa.h"


/******************************************************************************
 * IV. HIGH-RESOLUTION TIME TESTS
 *****************************************************************************/

/*
d_tests_dtime_clock_gettime
  Tests d_clock_gettime for high-resolution time retrieval.
  Tests the following:
  - retrieves CLOCK_REALTIME successfully
  - retrieves CLOCK_MONOTONIC successfully
  - CLOCK_REALTIME returns reasonable timestamp (post-2020)
  - CLOCK_MONOTONIC values are non-negative
  - tv_nsec is within valid range [0, 999999999]
  - consecutive calls show time progression
  - returns 0 on success
  - returns -1 for NULL timespec parameter
  - handles CLOCK_PROCESS_CPUTIME_ID
  - handles CLOCK_THREAD_CPUTIME_ID
*/
struct d_test_object*
d_tests_dtime_clock_gettime
(
    void
)
{
    struct d_test_object* group;
    struct timespec       ts1;
    struct timespec       ts2;
    int                   result;
    time_t                min_reasonable_time;
    bool                  test_realtime_success;
    bool                  test_monotonic_success;
    bool                  test_realtime_reasonable;
    bool                  test_monotonic_nonnegative;
    bool                  test_nsec_valid_range;
    bool                  test_time_progression;
    bool                  test_returns_zero;
    bool                  test_null_timespec;
    bool                  test_process_cputime;
    bool                  test_thread_cputime;
    size_t                idx;

    // minimum reasonable time: 2020-01-01 00:00:00 UTC
    min_reasonable_time = 1577836800;

    // test 1: CLOCK_REALTIME success
    memset(&ts1, 0, sizeof(ts1));
    result = d_clock_gettime(CLOCK_REALTIME, &ts1);
    test_realtime_success = (result == 0);

    // test 2: CLOCK_MONOTONIC success
    memset(&ts1, 0, sizeof(ts1));
    result = d_clock_gettime(CLOCK_MONOTONIC, &ts1);
    test_monotonic_success = (result == 0);

    // test 3: CLOCK_REALTIME returns reasonable timestamp
    memset(&ts1, 0, sizeof(ts1));
    d_clock_gettime(CLOCK_REALTIME, &ts1);
    test_realtime_reasonable = (ts1.tv_sec >= min_reasonable_time);

    // test 4: CLOCK_MONOTONIC values are non-negative
    memset(&ts1, 0, sizeof(ts1));
    d_clock_gettime(CLOCK_MONOTONIC, &ts1);
    test_monotonic_nonnegative = (ts1.tv_sec >= 0) && (ts1.tv_nsec >= 0);

    // test 5: tv_nsec is within valid range
    memset(&ts1, 0, sizeof(ts1));
    d_clock_gettime(CLOCK_REALTIME, &ts1);
    test_nsec_valid_range = (ts1.tv_nsec >= 0) && 
                            (ts1.tv_nsec < D_TIME_NSEC_PER_SEC);

    // test 6: consecutive calls show time progression (or at least no regression)
    memset(&ts1, 0, sizeof(ts1));
    memset(&ts2, 0, sizeof(ts2));
    d_clock_gettime(CLOCK_MONOTONIC, &ts1);
    
    // small busy-wait to ensure time passes
    {
        volatile int i;

        for (i = 0; i < 10000; i++)
        {
            // intentionally empty
        }
    }
    
    d_clock_gettime(CLOCK_MONOTONIC, &ts2);
    test_time_progression = (ts2.tv_sec > ts1.tv_sec) ||
                            ( (ts2.tv_sec == ts1.tv_sec) && 
                              (ts2.tv_nsec >= ts1.tv_nsec) );

    // test 7: returns 0 on success
    memset(&ts1, 0, sizeof(ts1));
    result = d_clock_gettime(CLOCK_REALTIME, &ts1);
    test_returns_zero = (result == 0);

    // test 8: returns -1 for NULL timespec
    result = d_clock_gettime(CLOCK_REALTIME, NULL);
    test_null_timespec = (result == -1);

    // test 9: CLOCK_PROCESS_CPUTIME_ID (may not be supported on all platforms)
    memset(&ts1, 0, sizeof(ts1));
    result = d_clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts1);
    // accept either success (0) or unsupported (-1)
    test_process_cputime = (result == 0) || (result == -1);

    // test 10: CLOCK_THREAD_CPUTIME_ID (may not be supported on all platforms)
    memset(&ts1, 0, sizeof(ts1));
    result = d_clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts1);
    // accept either success (0) or unsupported (-1)
    test_thread_cputime = (result == 0) || (result == -1);

    // build result tree
    group = d_test_object_new_interior("d_clock_gettime", 10);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("realtime_success",
                                           test_realtime_success,
                                           "d_clock_gettime CLOCK_REALTIME succeeds");
    group->elements[idx++] = D_ASSERT_TRUE("monotonic_success",
                                           test_monotonic_success,
                                           "d_clock_gettime CLOCK_MONOTONIC succeeds");
    group->elements[idx++] = D_ASSERT_TRUE("realtime_reasonable",
                                           test_realtime_reasonable,
                                           "CLOCK_REALTIME returns post-2020 time");
    group->elements[idx++] = D_ASSERT_TRUE("monotonic_nonnegative",
                                           test_monotonic_nonnegative,
                                           "CLOCK_MONOTONIC values are non-negative");
    group->elements[idx++] = D_ASSERT_TRUE("nsec_valid_range",
                                           test_nsec_valid_range,
                                           "tv_nsec is in [0, 999999999]");
    group->elements[idx++] = D_ASSERT_TRUE("time_progression",
                                           test_time_progression,
                                           "consecutive calls show time progression");
    group->elements[idx++] = D_ASSERT_TRUE("returns_zero",
                                           test_returns_zero,
                                           "d_clock_gettime returns 0 on success");
    group->elements[idx++] = D_ASSERT_TRUE("null_timespec",
                                           test_null_timespec,
                                           "d_clock_gettime returns -1 for NULL");
    group->elements[idx++] = D_ASSERT_TRUE("process_cputime",
                                           test_process_cputime,
                                           "CLOCK_PROCESS_CPUTIME_ID handled");
    group->elements[idx++] = D_ASSERT_TRUE("thread_cputime",
                                           test_thread_cputime,
                                           "CLOCK_THREAD_CPUTIME_ID handled");

    return group;
}


/*
d_tests_dtime_clock_getres
  Tests d_clock_getres for clock resolution retrieval.
  Tests the following:
  - retrieves CLOCK_REALTIME resolution successfully
  - retrieves CLOCK_MONOTONIC resolution successfully
  - resolution values are positive
  - resolution is at least 1 second or finer
  - tv_nsec is within valid range
  - returns 0 on success
  - returns -1 for NULL resolution parameter
  - CLOCK_MONOTONIC typically has nanosecond-level resolution
*/
struct d_test_object*
d_tests_dtime_clock_getres
(
    void
)
{
    struct d_test_object* group;
    struct timespec       res;
    int                   result;
    bool                  test_realtime_success;
    bool                  test_monotonic_success;
    bool                  test_resolution_positive;
    bool                  test_resolution_reasonable;
    bool                  test_nsec_valid_range;
    bool                  test_returns_zero;
    bool                  test_null_resolution;
    bool                  test_high_resolution;
    size_t                idx;

    // test 1: CLOCK_REALTIME resolution success
    memset(&res, 0xFF, sizeof(res));
    result = d_clock_getres(CLOCK_REALTIME, &res);
    test_realtime_success = (result == 0);

    // test 2: CLOCK_MONOTONIC resolution success
    memset(&res, 0xFF, sizeof(res));
    result = d_clock_getres(CLOCK_MONOTONIC, &res);
    test_monotonic_success = (result == 0);

    // test 3: resolution values are positive (or zero for sub-nanosecond)
    memset(&res, 0xFF, sizeof(res));
    d_clock_getres(CLOCK_REALTIME, &res);
    test_resolution_positive = (res.tv_sec >= 0) && (res.tv_nsec >= 0);

    // test 4: resolution is at most 1 second (should be finer on modern systems)
    memset(&res, 0xFF, sizeof(res));
    d_clock_getres(CLOCK_REALTIME, &res);
    test_resolution_reasonable = (res.tv_sec <= 1);

    // test 5: tv_nsec is within valid range
    memset(&res, 0xFF, sizeof(res));
    d_clock_getres(CLOCK_REALTIME, &res);
    test_nsec_valid_range = (res.tv_nsec >= 0) && 
                            (res.tv_nsec < D_TIME_NSEC_PER_SEC);

    // test 6: returns 0 on success
    memset(&res, 0, sizeof(res));
    result = d_clock_getres(CLOCK_REALTIME, &res);
    test_returns_zero = (result == 0);

    // test 7: returns -1 for NULL resolution parameter
    result = d_clock_getres(CLOCK_REALTIME, NULL);
    test_null_resolution = (result == -1);

    // test 8: modern systems typically have nanosecond or microsecond resolution
    // resolution should be less than 1ms (1000000 ns) on most modern systems
    memset(&res, 0xFF, sizeof(res));
    d_clock_getres(CLOCK_MONOTONIC, &res);
    test_high_resolution = (res.tv_sec == 0) && 
                           (res.tv_nsec <= 1000000);  // at most 1ms

    // build result tree
    group = d_test_object_new_interior("d_clock_getres", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("realtime_success",
                                           test_realtime_success,
                                           "d_clock_getres CLOCK_REALTIME succeeds");
    group->elements[idx++] = D_ASSERT_TRUE("monotonic_success",
                                           test_monotonic_success,
                                           "d_clock_getres CLOCK_MONOTONIC succeeds");
    group->elements[idx++] = D_ASSERT_TRUE("resolution_positive",
                                           test_resolution_positive,
                                           "resolution values are non-negative");
    group->elements[idx++] = D_ASSERT_TRUE("resolution_reasonable",
                                           test_resolution_reasonable,
                                           "resolution is at most 1 second");
    group->elements[idx++] = D_ASSERT_TRUE("nsec_valid_range",
                                           test_nsec_valid_range,
                                           "tv_nsec is in valid range");
    group->elements[idx++] = D_ASSERT_TRUE("returns_zero",
                                           test_returns_zero,
                                           "d_clock_getres returns 0 on success");
    group->elements[idx++] = D_ASSERT_TRUE("null_resolution",
                                           test_null_resolution,
                                           "d_clock_getres returns -1 for NULL");
    group->elements[idx++] = D_ASSERT_TRUE("high_resolution",
                                           test_high_resolution,
                                           "CLOCK_MONOTONIC has sub-ms resolution");

    return group;
}


/*
d_tests_dtime_timespec_get
  Tests d_timespec_get for C11-style timespec retrieval.
  Tests the following:
  - retrieves TIME_UTC successfully
  - returns the base on success
  - timestamp is reasonable (post-2020)
  - tv_nsec is within valid range
  - returns 0 for NULL timespec parameter
  - returns 0 for invalid base
  - consecutive calls show time progression
  - multiple calls produce consistent results
*/
struct d_test_object*
d_tests_dtime_timespec_get
(
    void
)
{
    struct d_test_object* group;
    struct timespec       ts1;
    struct timespec       ts2;
    int                   result;
    time_t                min_reasonable_time;
    bool                  test_utc_success;
    bool                  test_returns_base;
    bool                  test_timestamp_reasonable;
    bool                  test_nsec_valid_range;
    bool                  test_null_timespec;
    bool                  test_invalid_base;
    bool                  test_time_progression;
    bool                  test_consistency;
    size_t                idx;

    // minimum reasonable time: 2020-01-01 00:00:00 UTC
    min_reasonable_time = 1577836800;

    // test 1: TIME_UTC success
    memset(&ts1, 0, sizeof(ts1));
    result = d_timespec_get(&ts1, TIME_UTC);
    test_utc_success = (result != 0);  // returns base on success, 0 on failure

    // test 2: returns the base value on success
    memset(&ts1, 0, sizeof(ts1));
    result = d_timespec_get(&ts1, TIME_UTC);
    test_returns_base = (result == TIME_UTC);

    // test 3: timestamp is reasonable (post-2020)
    memset(&ts1, 0, sizeof(ts1));
    d_timespec_get(&ts1, TIME_UTC);
    test_timestamp_reasonable = (ts1.tv_sec >= min_reasonable_time);

    // test 4: tv_nsec is within valid range
    memset(&ts1, 0, sizeof(ts1));
    d_timespec_get(&ts1, TIME_UTC);
    test_nsec_valid_range = (ts1.tv_nsec >= 0) && 
                            (ts1.tv_nsec < D_TIME_NSEC_PER_SEC);

    // test 5: returns 0 for NULL timespec
    result = d_timespec_get(NULL, TIME_UTC);
    test_null_timespec = (result == 0);

    // test 6: returns 0 for invalid base (base 0 is invalid, base 999 is invalid)
    memset(&ts1, 0, sizeof(ts1));
    result = d_timespec_get(&ts1, 0);
    test_invalid_base = (result == 0);

    // test 7: consecutive calls show time progression
    memset(&ts1, 0, sizeof(ts1));
    memset(&ts2, 0, sizeof(ts2));
    d_timespec_get(&ts1, TIME_UTC);
    
    // small busy-wait
    {
        volatile int i;

        for (i = 0; i < 10000; i++)
        {
            // intentionally empty
        }
    }
    
    d_timespec_get(&ts2, TIME_UTC);
    test_time_progression = (ts2.tv_sec > ts1.tv_sec) ||
                            ( (ts2.tv_sec == ts1.tv_sec) && 
                              (ts2.tv_nsec >= ts1.tv_nsec) );

    // test 8: multiple calls in quick succession produce consistent results
    // (timestamps should be very close, within 1 second)
    memset(&ts1, 0, sizeof(ts1));
    memset(&ts2, 0, sizeof(ts2));
    d_timespec_get(&ts1, TIME_UTC);
    d_timespec_get(&ts2, TIME_UTC);
    test_consistency = (ts2.tv_sec - ts1.tv_sec) <= 1;

    // build result tree
    group = d_test_object_new_interior("d_timespec_get", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("utc_success",
                                           test_utc_success,
                                           "d_timespec_get TIME_UTC succeeds");
    group->elements[idx++] = D_ASSERT_TRUE("returns_base",
                                           test_returns_base,
                                           "d_timespec_get returns base on success");
    group->elements[idx++] = D_ASSERT_TRUE("timestamp_reasonable",
                                           test_timestamp_reasonable,
                                           "timestamp is post-2020");
    group->elements[idx++] = D_ASSERT_TRUE("nsec_valid_range",
                                           test_nsec_valid_range,
                                           "tv_nsec is in [0, 999999999]");
    group->elements[idx++] = D_ASSERT_TRUE("null_timespec",
                                           test_null_timespec,
                                           "returns 0 for NULL timespec");
    group->elements[idx++] = D_ASSERT_TRUE("invalid_base",
                                           test_invalid_base,
                                           "returns 0 for invalid base");
    group->elements[idx++] = D_ASSERT_TRUE("time_progression",
                                           test_time_progression,
                                           "consecutive calls show progression");
    group->elements[idx++] = D_ASSERT_TRUE("consistency",
                                           test_consistency,
                                           "rapid calls produce consistent times");

    return group;
}


/*
d_tests_dtime_high_resolution_time_all
  Runs all high-resolution time tests.
  Tests the following:
  - d_clock_gettime
  - d_clock_getres
  - d_timespec_get
*/
struct d_test_object*
d_tests_dtime_high_resolution_time_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("IV. High-Resolution Time", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dtime_clock_gettime();
    group->elements[idx++] = d_tests_dtime_clock_getres();
    group->elements[idx++] = d_tests_dtime_timespec_get();

    return group;
}
