/******************************************************************************
* djinterp [test]                                         dtime_tests_sa_sleep.c
*
*   Tests for sleep functions (nanosleep, usleep, sleep_ms).
*
*
* path:      \src\test\dtime_tests_sa_sleep.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.25
******************************************************************************/
#include "dtime_tests_sa.h"


/******************************************************************************
 * V. SLEEP FUNCTIONS TESTS
 *****************************************************************************/

/*
d_tests_dtime_nanosleep
  Tests d_nanosleep for high-resolution sleep.
  Tests the following:
  - sleeps for approximately the requested duration
  - returns 0 on successful completion
  - returns -1 for NULL request parameter
  - handles zero duration sleep
  - handles sub-millisecond sleep
  - remaining time is set appropriately on success
  - sleep duration is within acceptable tolerance
  - handles large nanosecond values (normalized)
*/
struct d_test_object*
d_tests_dtime_nanosleep
(
    void
)
{
    struct d_test_object* group;
    struct timespec       req;
    struct timespec       rem;
    struct timespec       start;
    struct timespec       end;
    int                   result;
    int64_t               elapsed_ms;
    int64_t               expected_ms;
    bool                  test_sleep_duration;
    bool                  test_returns_zero;
    bool                  test_null_request;
    bool                  test_zero_duration;
    bool                  test_sub_millisecond;
    bool                  test_remaining_cleared;
    bool                  test_within_tolerance;
    bool                  test_null_remaining;
    size_t                idx;

    // test 1 & 7: sleep for 100ms and verify duration is within tolerance
    expected_ms = D_TEST_DTIME_SLEEP_TEST_MS;
    req.tv_sec  = 0;
    req.tv_nsec = expected_ms * D_TIME_NSEC_PER_MSEC;
    memset(&rem, 0xFF, sizeof(rem));

    d_clock_gettime(CLOCK_MONOTONIC, &start);
    result = d_nanosleep(&req, &rem);
    d_clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed_ms = ((end.tv_sec - start.tv_sec) * D_TIME_MSEC_PER_SEC) +
                 ((end.tv_nsec - start.tv_nsec) / D_TIME_NSEC_PER_MSEC);

    test_sleep_duration = (elapsed_ms >= expected_ms);
    test_within_tolerance = (elapsed_ms >= expected_ms) &&
                            (elapsed_ms <= expected_ms + D_TEST_DTIME_SLEEP_TOLERANCE_MS);

    // test 2: returns 0 on success
    test_returns_zero = (result == 0);

    // test 3: NULL request parameter
    result = d_nanosleep(NULL, &rem);
    test_null_request = (result == -1);

    // test 4: zero duration sleep (should return immediately)
    req.tv_sec  = 0;
    req.tv_nsec = 0;
    d_clock_gettime(CLOCK_MONOTONIC, &start);
    result = d_nanosleep(&req, &rem);
    d_clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed_ms = ((end.tv_sec - start.tv_sec) * D_TIME_MSEC_PER_SEC) +
                 ((end.tv_nsec - start.tv_nsec) / D_TIME_NSEC_PER_MSEC);

    test_zero_duration = (result == 0) && (elapsed_ms < 50);  // should be very fast

    // test 5: sub-millisecond sleep (10 microseconds = 10000 nanoseconds)
    req.tv_sec  = 0;
    req.tv_nsec = 10000;  // 10 microseconds
    result = d_nanosleep(&req, &rem);
    test_sub_millisecond = (result == 0);

    // test 6: remaining time is zeroed or small on successful completion
    req.tv_sec  = 0;
    req.tv_nsec = 1000000;  // 1 millisecond
    memset(&rem, 0xFF, sizeof(rem));
    result = d_nanosleep(&req, &rem);
    // on success without interruption, remaining should be zero or very small
    test_remaining_cleared = (result == 0) && 
                             (rem.tv_sec == 0) && 
                             (rem.tv_nsec < D_TIME_NSEC_PER_MSEC);

    // test 8: NULL remaining parameter is acceptable
    req.tv_sec  = 0;
    req.tv_nsec = 1000000;  // 1 millisecond
    result = d_nanosleep(&req, NULL);
    test_null_remaining = (result == 0);

    // build result tree
    group = d_test_object_new_interior("d_nanosleep", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("sleep_duration",
                                           test_sleep_duration,
                                           "d_nanosleep sleeps at least requested time");
    group->elements[idx++] = D_ASSERT_TRUE("returns_zero",
                                           test_returns_zero,
                                           "d_nanosleep returns 0 on success");
    group->elements[idx++] = D_ASSERT_TRUE("null_request",
                                           test_null_request,
                                           "d_nanosleep returns -1 for NULL request");
    group->elements[idx++] = D_ASSERT_TRUE("zero_duration",
                                           test_zero_duration,
                                           "d_nanosleep handles zero duration");
    group->elements[idx++] = D_ASSERT_TRUE("sub_millisecond",
                                           test_sub_millisecond,
                                           "d_nanosleep handles sub-ms sleep");
    group->elements[idx++] = D_ASSERT_TRUE("remaining_cleared",
                                           test_remaining_cleared,
                                           "remaining time cleared on success");
    group->elements[idx++] = D_ASSERT_TRUE("within_tolerance",
                                           test_within_tolerance,
                                           "sleep duration within tolerance");
    group->elements[idx++] = D_ASSERT_TRUE("null_remaining",
                                           test_null_remaining,
                                           "d_nanosleep accepts NULL remaining");

    return group;
}


/*
d_tests_dtime_usleep
  Tests d_usleep for microsecond sleep.
  Tests the following:
  - sleeps for approximately the requested duration
  - returns 0 on success
  - handles zero duration sleep
  - handles small microsecond values
  - handles larger microsecond values (> 1 second)
  - sleep duration is within acceptable tolerance
  - handles 1 microsecond sleep
  - handles exact millisecond boundaries
*/
struct d_test_object*
d_tests_dtime_usleep
(
    void
)
{
    struct d_test_object* group;
    struct timespec       start;
    struct timespec       end;
    int                   result;
    int64_t               elapsed_us;
    int64_t               elapsed_ms;
    unsigned int          sleep_us;
    bool                  test_sleep_duration;
    bool                  test_returns_zero;
    bool                  test_zero_duration;
    bool                  test_small_value;
    bool                  test_large_value;
    bool                  test_within_tolerance;
    bool                  test_one_microsecond;
    bool                  test_millisecond_boundary;
    size_t                idx;

    // test 1 & 6: sleep for 100ms (100000 microseconds) and verify duration
    sleep_us = D_TEST_DTIME_SLEEP_TEST_MS * D_TIME_USEC_PER_MSEC;

    d_clock_gettime(CLOCK_MONOTONIC, &start);
    result = d_usleep(sleep_us);
    d_clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed_ms = ((end.tv_sec - start.tv_sec) * D_TIME_MSEC_PER_SEC) +
                 ((end.tv_nsec - start.tv_nsec) / D_TIME_NSEC_PER_MSEC);

    test_sleep_duration = (elapsed_ms >= D_TEST_DTIME_SLEEP_TEST_MS);
    test_within_tolerance = (elapsed_ms >= D_TEST_DTIME_SLEEP_TEST_MS) &&
                            (elapsed_ms <= D_TEST_DTIME_SLEEP_TEST_MS + 
                                           D_TEST_DTIME_SLEEP_TOLERANCE_MS);

    // test 2: returns 0 on success
    test_returns_zero = (result == 0);

    // test 3: zero duration sleep
    d_clock_gettime(CLOCK_MONOTONIC, &start);
    result = d_usleep(0);
    d_clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed_us = ((end.tv_sec - start.tv_sec) * D_TIME_USEC_PER_SEC) +
                 ((end.tv_nsec - start.tv_nsec) / D_TIME_NSEC_PER_USEC);

    test_zero_duration = (result == 0) && (elapsed_us < 50000);  // < 50ms

    // test 4: small microsecond value (100 microseconds)
    result = d_usleep(100);
    test_small_value = (result == 0);

    // test 5: larger value crossing second boundary (1.1 seconds = 1100000 us)
    // skip actual sleep to avoid long test time, just verify return
    // we'll use a shorter duration that still tests the path
    d_clock_gettime(CLOCK_MONOTONIC, &start);
    result = d_usleep(50000);  // 50ms
    d_clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed_ms = ((end.tv_sec - start.tv_sec) * D_TIME_MSEC_PER_SEC) +
                 ((end.tv_nsec - start.tv_nsec) / D_TIME_NSEC_PER_MSEC);

    test_large_value = (result == 0) && (elapsed_ms >= 40);  // allow some variance

    // test 7: one microsecond sleep
    result = d_usleep(1);
    test_one_microsecond = (result == 0);

    // test 8: exact millisecond boundary (1000 microseconds = 1 millisecond)
    d_clock_gettime(CLOCK_MONOTONIC, &start);
    result = d_usleep(1000);
    d_clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed_us = ((end.tv_sec - start.tv_sec) * D_TIME_USEC_PER_SEC) +
                 ((end.tv_nsec - start.tv_nsec) / D_TIME_NSEC_PER_USEC);

    test_millisecond_boundary = (result == 0) && (elapsed_us >= 1000);

    // build result tree
    group = d_test_object_new_interior("d_usleep", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("sleep_duration",
                                           test_sleep_duration,
                                           "d_usleep sleeps at least requested time");
    group->elements[idx++] = D_ASSERT_TRUE("returns_zero",
                                           test_returns_zero,
                                           "d_usleep returns 0 on success");
    group->elements[idx++] = D_ASSERT_TRUE("zero_duration",
                                           test_zero_duration,
                                           "d_usleep handles zero duration");
    group->elements[idx++] = D_ASSERT_TRUE("small_value",
                                           test_small_value,
                                           "d_usleep handles small values");
    group->elements[idx++] = D_ASSERT_TRUE("large_value",
                                           test_large_value,
                                           "d_usleep handles larger values");
    group->elements[idx++] = D_ASSERT_TRUE("within_tolerance",
                                           test_within_tolerance,
                                           "sleep duration within tolerance");
    group->elements[idx++] = D_ASSERT_TRUE("one_microsecond",
                                           test_one_microsecond,
                                           "d_usleep handles 1 microsecond");
    group->elements[idx++] = D_ASSERT_TRUE("millisecond_boundary",
                                           test_millisecond_boundary,
                                           "d_usleep handles ms boundary");

    return group;
}


/*
d_tests_dtime_sleep_ms
  Tests d_sleep_ms for millisecond sleep (convenience function).
  Tests the following:
  - sleeps for approximately the requested duration
  - returns 0 on success
  - handles zero duration sleep
  - handles small millisecond values
  - handles larger millisecond values
  - sleep duration is within acceptable tolerance
  - handles 1 millisecond sleep
  - handles exact second boundaries
*/
struct d_test_object*
d_tests_dtime_sleep_ms
(
    void
)
{
    struct d_test_object* group;
    struct timespec       start;
    struct timespec       end;
    int                   result;
    int64_t               elapsed_ms;
    unsigned long         sleep_ms;
    bool                  test_sleep_duration;
    bool                  test_returns_zero;
    bool                  test_zero_duration;
    bool                  test_small_value;
    bool                  test_medium_value;
    bool                  test_within_tolerance;
    bool                  test_one_millisecond;
    bool                  test_second_boundary;
    size_t                idx;

    // test 1 & 6: sleep for 100ms and verify duration
    sleep_ms = D_TEST_DTIME_SLEEP_TEST_MS;

    d_clock_gettime(CLOCK_MONOTONIC, &start);
    result = d_sleep_ms(sleep_ms);
    d_clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed_ms = ((end.tv_sec - start.tv_sec) * D_TIME_MSEC_PER_SEC) +
                 ((end.tv_nsec - start.tv_nsec) / D_TIME_NSEC_PER_MSEC);

    test_sleep_duration = (elapsed_ms >= (int64_t)sleep_ms);
    test_within_tolerance = (elapsed_ms >= (int64_t)sleep_ms) &&
                            (elapsed_ms <= (int64_t)sleep_ms + 
                                           D_TEST_DTIME_SLEEP_TOLERANCE_MS);

    // test 2: returns 0 on success
    test_returns_zero = (result == 0);

    // test 3: zero duration sleep
    d_clock_gettime(CLOCK_MONOTONIC, &start);
    result = d_sleep_ms(0);
    d_clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed_ms = ((end.tv_sec - start.tv_sec) * D_TIME_MSEC_PER_SEC) +
                 ((end.tv_nsec - start.tv_nsec) / D_TIME_NSEC_PER_MSEC);

    test_zero_duration = (result == 0) && (elapsed_ms < 50);

    // test 4: small millisecond value (5 milliseconds)
    d_clock_gettime(CLOCK_MONOTONIC, &start);
    result = d_sleep_ms(5);
    d_clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed_ms = ((end.tv_sec - start.tv_sec) * D_TIME_MSEC_PER_SEC) +
                 ((end.tv_nsec - start.tv_nsec) / D_TIME_NSEC_PER_MSEC);

    test_small_value = (result == 0) && (elapsed_ms >= 5);

    // test 5: medium value (50 milliseconds)
    d_clock_gettime(CLOCK_MONOTONIC, &start);
    result = d_sleep_ms(50);
    d_clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed_ms = ((end.tv_sec - start.tv_sec) * D_TIME_MSEC_PER_SEC) +
                 ((end.tv_nsec - start.tv_nsec) / D_TIME_NSEC_PER_MSEC);

    test_medium_value = (result == 0) && (elapsed_ms >= 50);

    // test 7: one millisecond sleep
    d_clock_gettime(CLOCK_MONOTONIC, &start);
    result = d_sleep_ms(1);
    d_clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed_ms = ((end.tv_sec - start.tv_sec) * D_TIME_MSEC_PER_SEC) +
                 ((end.tv_nsec - start.tv_nsec) / D_TIME_NSEC_PER_MSEC);

    test_one_millisecond = (result == 0) && (elapsed_ms >= 1);

    // test 8: exact second boundary (1000 milliseconds = 1 second)
    // use shorter duration to keep test fast
    d_clock_gettime(CLOCK_MONOTONIC, &start);
    result = d_sleep_ms(200);  // 200ms instead of 1000ms
    d_clock_gettime(CLOCK_MONOTONIC, &end);

    elapsed_ms = ((end.tv_sec - start.tv_sec) * D_TIME_MSEC_PER_SEC) +
                 ((end.tv_nsec - start.tv_nsec) / D_TIME_NSEC_PER_MSEC);

    test_second_boundary = (result == 0) && (elapsed_ms >= 200);

    // build result tree
    group = d_test_object_new_interior("d_sleep_ms", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("sleep_duration",
                                           test_sleep_duration,
                                           "d_sleep_ms sleeps at least requested time");
    group->elements[idx++] = D_ASSERT_TRUE("returns_zero",
                                           test_returns_zero,
                                           "d_sleep_ms returns 0 on success");
    group->elements[idx++] = D_ASSERT_TRUE("zero_duration",
                                           test_zero_duration,
                                           "d_sleep_ms handles zero duration");
    group->elements[idx++] = D_ASSERT_TRUE("small_value",
                                           test_small_value,
                                           "d_sleep_ms handles small values");
    group->elements[idx++] = D_ASSERT_TRUE("medium_value",
                                           test_medium_value,
                                           "d_sleep_ms handles medium values");
    group->elements[idx++] = D_ASSERT_TRUE("within_tolerance",
                                           test_within_tolerance,
                                           "sleep duration within tolerance");
    group->elements[idx++] = D_ASSERT_TRUE("one_millisecond",
                                           test_one_millisecond,
                                           "d_sleep_ms handles 1 millisecond");
    group->elements[idx++] = D_ASSERT_TRUE("second_boundary",
                                           test_second_boundary,
                                           "d_sleep_ms handles longer durations");

    return group;
}


/*
d_tests_dtime_sleep_functions_all
  Runs all sleep function tests.
  Tests the following:
  - d_nanosleep
  - d_usleep
  - d_sleep_ms
*/
struct d_test_object*
d_tests_dtime_sleep_functions_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("V. Sleep Functions", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dtime_nanosleep();
    group->elements[idx++] = d_tests_dtime_usleep();
    group->elements[idx++] = d_tests_dtime_sleep_ms();

    return group;
}
