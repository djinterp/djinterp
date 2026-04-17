/******************************************************************************
* djinterp [test]                                     dtime_tests_sa_monotonic.c
*
*   Tests for monotonic time utility functions (monotonic_time_ms,
* monotonic_time_us, monotonic_time_ns).
*
*
* path:      \src\test\dtime_tests_sa_monotonic.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.25
******************************************************************************/
#include "dtime_tests_sa.h"


/******************************************************************************
 * IX. MONOTONIC TIME UTILITIES TESTS
 *****************************************************************************/

/*
d_tests_dtime_monotonic_time_ms
  Tests d_monotonic_time_ms for getting monotonic time in milliseconds.
  Tests the following:
  - returns non-negative value
  - consecutive calls show non-decreasing values
  - value increases after sleep
  - returns reasonable magnitude
  - multiple rapid calls are consistent
  - value is stable within short timeframe
  - tracks elapsed time accurately
  - handles system load gracefully
*/
struct d_test_object*
d_tests_dtime_monotonic_time_ms
(
    void
)
{
    struct d_test_object* group;
    int64_t               t1;
    int64_t               t2;
    int64_t               t3;
    int64_t               elapsed;
    int64_t               expected_min;
    bool                  test_non_negative;
    bool                  test_non_decreasing;
    bool                  test_increases_after_sleep;
    bool                  test_reasonable_magnitude;
    bool                  test_rapid_calls;
    bool                  test_stability;
    bool                  test_elapsed_accuracy;
    bool                  test_consistency;
    size_t                idx;

    // test 1: returns non-negative value
    t1 = d_monotonic_time_ms();
    test_non_negative = (t1 >= 0);

    // test 2: consecutive calls show non-decreasing values
    t1 = d_monotonic_time_ms();
    t2 = d_monotonic_time_ms();
    t3 = d_monotonic_time_ms();
    test_non_decreasing = (t2 >= t1) && (t3 >= t2);

    // test 3: value increases after sleep (50ms sleep)
    t1 = d_monotonic_time_ms();
    d_sleep_ms(50);
    t2 = d_monotonic_time_ms();
    test_increases_after_sleep = (t2 > t1);

    // test 4: returns reasonable magnitude (not astronomically large)
    // monotonic time since boot should be less than 10 years in ms
    // 10 years ~ 315,360,000,000 ms
    t1 = d_monotonic_time_ms();
    test_reasonable_magnitude = (t1 >= 0) && (t1 < 315360000000LL);

    // test 5: multiple rapid calls produce close values
    t1 = d_monotonic_time_ms();
    t2 = d_monotonic_time_ms();
    t3 = d_monotonic_time_ms();
    // all three should be within 10ms of each other
    test_rapid_calls = ((t3 - t1) < 10);

    // test 6: value is stable within short timeframe (no wild jumps)
    {
        int64_t values[5];
        int     i;
        bool    stable;

        stable = true;

        for (i = 0; i < 5; i++)
        {
            values[i] = d_monotonic_time_ms();
        }

        for (i = 1; i < 5; i++)
        {
            // no single step should jump more than 5ms without sleep
            if ((values[i] - values[i - 1]) > 5)
            {
                stable = false;
            }
        }

        test_stability = stable;
    }

    // test 7: tracks elapsed time accurately (100ms sleep)
    expected_min = D_TEST_DTIME_SLEEP_TEST_MS;
    t1 = d_monotonic_time_ms();
    d_sleep_ms((unsigned long)expected_min);
    t2 = d_monotonic_time_ms();
    elapsed = t2 - t1;
    // elapsed should be at least expected_min and no more than expected + tolerance
    test_elapsed_accuracy = (elapsed >= expected_min) &&
                            (elapsed <= expected_min + D_TEST_DTIME_SLEEP_TOLERANCE_MS);

    // test 8: consistency across multiple measurements
    {
        int64_t start;
        int64_t end;
        int64_t total_elapsed;
        int     i;

        start = d_monotonic_time_ms();

        for (i = 0; i < 5; i++)
        {
            d_sleep_ms(10);
        }

        end = d_monotonic_time_ms();
        total_elapsed = end - start;
        // 5 * 10ms = 50ms minimum
        test_consistency = (total_elapsed >= 50) && (total_elapsed <= 100);
    }

    // build result tree
    group = d_test_object_new_interior("d_monotonic_time_ms", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("non_negative",
                                           test_non_negative,
                                           "d_monotonic_time_ms returns >= 0");
    group->elements[idx++] = D_ASSERT_TRUE("non_decreasing",
                                           test_non_decreasing,
                                           "d_monotonic_time_ms is non-decreasing");
    group->elements[idx++] = D_ASSERT_TRUE("increases_after_sleep",
                                           test_increases_after_sleep,
                                           "value increases after sleep");
    group->elements[idx++] = D_ASSERT_TRUE("reasonable_magnitude",
                                           test_reasonable_magnitude,
                                           "value has reasonable magnitude");
    group->elements[idx++] = D_ASSERT_TRUE("rapid_calls",
                                           test_rapid_calls,
                                           "rapid calls produce close values");
    group->elements[idx++] = D_ASSERT_TRUE("stability",
                                           test_stability,
                                           "values are stable (no jumps)");
    group->elements[idx++] = D_ASSERT_TRUE("elapsed_accuracy",
                                           test_elapsed_accuracy,
                                           "tracks elapsed time accurately");
    group->elements[idx++] = D_ASSERT_TRUE("consistency",
                                           test_consistency,
                                           "consistent across measurements");

    return group;
}


/*
d_tests_dtime_monotonic_time_us
  Tests d_monotonic_time_us for getting monotonic time in microseconds.
  Tests the following:
  - returns non-negative value
  - consecutive calls show non-decreasing values
  - value increases after sleep
  - has finer resolution than milliseconds
  - multiple rapid calls show microsecond differences
  - tracks elapsed time accurately
  - relationship with d_monotonic_time_ms is correct
  - handles rapid successive calls
*/
struct d_test_object*
d_tests_dtime_monotonic_time_us
(
    void
)
{
    struct d_test_object* group;
    int64_t               t1;
    int64_t               t2;
    int64_t               t3;
    int64_t               t_ms;
    int64_t               t_us;
    int64_t               elapsed;
    int64_t               expected_min_us;
    bool                  test_non_negative;
    bool                  test_non_decreasing;
    bool                  test_increases_after_sleep;
    bool                  test_finer_resolution;
    bool                  test_microsecond_differences;
    bool                  test_elapsed_accuracy;
    bool                  test_ms_relationship;
    bool                  test_rapid_calls;
    size_t                idx;

    // test 1: returns non-negative value
    t1 = d_monotonic_time_us();
    test_non_negative = (t1 >= 0);

    // test 2: consecutive calls show non-decreasing values
    t1 = d_monotonic_time_us();
    t2 = d_monotonic_time_us();
    t3 = d_monotonic_time_us();
    test_non_decreasing = (t2 >= t1) && (t3 >= t2);

    // test 3: value increases after sleep (10ms sleep = 10000us minimum)
    t1 = d_monotonic_time_us();
    d_sleep_ms(10);
    t2 = d_monotonic_time_us();
    test_increases_after_sleep = (t2 > t1) && ((t2 - t1) >= 10000);

    // test 4: has finer resolution than milliseconds
    // multiple calls should show differences even without sleep
    {
        int     i;
        int     differences_found;

        differences_found = 0;

        for (i = 0; i < 100; i++)
        {
            t1 = d_monotonic_time_us();

            // small busy-wait
            {
                volatile int j;

                for (j = 0; j < 100; j++)
                {
                    // intentionally empty
                }
            }

            t2 = d_monotonic_time_us();

            if (t2 > t1)
            {
                differences_found++;
            }
        }

        // should find some differences (not all zero)
        test_finer_resolution = (differences_found > 0);
    }

    // test 5: can detect microsecond-level differences
    {
        int64_t min_diff;
        int64_t max_diff;
        int64_t diff;
        int     i;

        min_diff = INT64_MAX;
        max_diff = 0;

        for (i = 0; i < 50; i++)
        {
            t1 = d_monotonic_time_us();
            t2 = d_monotonic_time_us();
            diff = t2 - t1;

            if ( (diff >= 0) && 
                 (diff < min_diff) )
            {
                min_diff = diff;
            }

            if (diff > max_diff)
            {
                max_diff = diff;
            }
        }

        // minimum difference should be small (< 1000us = 1ms)
        test_microsecond_differences = (min_diff < 1000);
    }

    // test 6: tracks elapsed time accurately (50ms sleep)
    expected_min_us = 50 * D_TIME_USEC_PER_MSEC;  // 50000 us
    t1 = d_monotonic_time_us();
    d_sleep_ms(50);
    t2 = d_monotonic_time_us();
    elapsed = t2 - t1;
    // allow tolerance of 50ms (50000us)
    test_elapsed_accuracy = (elapsed >= expected_min_us) &&
                            (elapsed <= expected_min_us + 
                                        (D_TEST_DTIME_SLEEP_TOLERANCE_MS * 
                                         D_TIME_USEC_PER_MSEC));

    // test 7: relationship with d_monotonic_time_ms is approximately 1000:1
    t_ms = d_monotonic_time_ms();
    t_us = d_monotonic_time_us();
    // us should be approximately ms * 1000 (within reasonable margin)
    // allow 100ms margin for timing differences
    test_ms_relationship = (t_us >= (t_ms - 100) * 1000) &&
                           (t_us <= (t_ms + 100) * 1000);

    // test 8: handles rapid successive calls without issues
    {
        int64_t values[10];
        int     i;
        bool    all_valid;

        all_valid = true;

        for (i = 0; i < 10; i++)
        {
            values[i] = d_monotonic_time_us();
        }

        for (i = 1; i < 10; i++)
        {
            if (values[i] < values[i - 1])
            {
                all_valid = false;
            }
        }

        test_rapid_calls = all_valid;
    }

    // build result tree
    group = d_test_object_new_interior("d_monotonic_time_us", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("non_negative",
                                           test_non_negative,
                                           "d_monotonic_time_us returns >= 0");
    group->elements[idx++] = D_ASSERT_TRUE("non_decreasing",
                                           test_non_decreasing,
                                           "d_monotonic_time_us is non-decreasing");
    group->elements[idx++] = D_ASSERT_TRUE("increases_after_sleep",
                                           test_increases_after_sleep,
                                           "value increases after sleep");
    group->elements[idx++] = D_ASSERT_TRUE("finer_resolution",
                                           test_finer_resolution,
                                           "has finer resolution than ms");
    group->elements[idx++] = D_ASSERT_TRUE("microsecond_differences",
                                           test_microsecond_differences,
                                           "detects microsecond differences");
    group->elements[idx++] = D_ASSERT_TRUE("elapsed_accuracy",
                                           test_elapsed_accuracy,
                                           "tracks elapsed time accurately");
    group->elements[idx++] = D_ASSERT_TRUE("ms_relationship",
                                           test_ms_relationship,
                                           "correct relationship with ms");
    group->elements[idx++] = D_ASSERT_TRUE("rapid_calls",
                                           test_rapid_calls,
                                           "handles rapid calls correctly");

    return group;
}


/*
d_tests_dtime_monotonic_time_ns
  Tests d_monotonic_time_ns for getting monotonic time in nanoseconds.
  Tests the following:
  - returns non-negative value
  - consecutive calls show non-decreasing values
  - value increases after sleep
  - has finest resolution available
  - multiple rapid calls show nanosecond differences
  - tracks elapsed time accurately
  - relationship with d_monotonic_time_us is correct
  - handles potential overflow gracefully
*/
struct d_test_object*
d_tests_dtime_monotonic_time_ns
(
    void
)
{
    struct d_test_object* group;
    int64_t               t1;
    int64_t               t2;
    int64_t               t3;
    int64_t               t_us;
    int64_t               t_ns;
    int64_t               elapsed;
    int64_t               expected_min_ns;
    bool                  test_non_negative;
    bool                  test_non_decreasing;
    bool                  test_increases_after_sleep;
    bool                  test_finest_resolution;
    bool                  test_nanosecond_differences;
    bool                  test_elapsed_accuracy;
    bool                  test_us_relationship;
    bool                  test_large_values;
    size_t                idx;

    // test 1: returns non-negative value
    t1 = d_monotonic_time_ns();
    test_non_negative = (t1 >= 0);

    // test 2: consecutive calls show non-decreasing values
    t1 = d_monotonic_time_ns();
    t2 = d_monotonic_time_ns();
    t3 = d_monotonic_time_ns();
    test_non_decreasing = (t2 >= t1) && (t3 >= t2);

    // test 3: value increases after sleep (10ms sleep = 10000000ns minimum)
    t1 = d_monotonic_time_ns();
    d_sleep_ms(10);
    t2 = d_monotonic_time_ns();
    test_increases_after_sleep = (t2 > t1) && ((t2 - t1) >= 10000000);

    // test 4: has finest resolution available
    // consecutive calls should show small but measurable differences
    {
        int     i;
        int     differences_found;
        int64_t total_diff;

        differences_found = 0;
        total_diff = 0;

        for (i = 0; i < 100; i++)
        {
            t1 = d_monotonic_time_ns();
            t2 = d_monotonic_time_ns();

            if (t2 > t1)
            {
                differences_found++;
                total_diff += (t2 - t1);
            }
        }

        // should find many differences at nanosecond resolution
        test_finest_resolution = (differences_found > 10);
    }

    // test 5: can detect nanosecond-level differences
    {
        int64_t min_diff;
        int64_t diff;
        int     i;

        min_diff = INT64_MAX;

        for (i = 0; i < 100; i++)
        {
            t1 = d_monotonic_time_ns();
            t2 = d_monotonic_time_ns();
            diff = t2 - t1;

            if ( (diff > 0) && 
                 (diff < min_diff) )
            {
                min_diff = diff;
            }
        }

        // minimum difference should be small (< 1000000ns = 1ms)
        // on most systems this will be much smaller
        test_nanosecond_differences = (min_diff < 1000000);
    }

    // test 6: tracks elapsed time accurately (20ms sleep)
    expected_min_ns = 20 * D_TIME_NSEC_PER_MSEC;  // 20000000 ns
    t1 = d_monotonic_time_ns();
    d_sleep_ms(20);
    t2 = d_monotonic_time_ns();
    elapsed = t2 - t1;
    // allow tolerance
    test_elapsed_accuracy = (elapsed >= expected_min_ns) &&
                            (elapsed <= expected_min_ns + 
                                        (D_TEST_DTIME_SLEEP_TOLERANCE_MS * 
                                         D_TIME_NSEC_PER_MSEC));

    // test 7: relationship with d_monotonic_time_us is approximately 1000:1
    t_us = d_monotonic_time_us();
    t_ns = d_monotonic_time_ns();
    // ns should be approximately us * 1000 (within reasonable margin)
    // allow 10ms margin for timing differences
    test_us_relationship = (t_ns >= (t_us - 10000) * 1000) &&
                           (t_ns <= (t_us + 10000) * 1000);

    // test 8: handles large values gracefully
    // nanosecond values can get very large; verify no overflow issues
    {
        int64_t values[5];
        int     i;
        bool    all_positive;
        bool    all_increasing;

        all_positive = true;
        all_increasing = true;

        for (i = 0; i < 5; i++)
        {
            values[i] = d_monotonic_time_ns();

            if (values[i] < 0)
            {
                all_positive = false;
            }

            if ( (i > 0) && 
                 (values[i] < values[i - 1]) )
            {
                all_increasing = false;
            }

            d_sleep_ms(1);
        }

        test_large_values = all_positive && all_increasing;
    }

    // build result tree
    group = d_test_object_new_interior("d_monotonic_time_ns", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("non_negative",
                                           test_non_negative,
                                           "d_monotonic_time_ns returns >= 0");
    group->elements[idx++] = D_ASSERT_TRUE("non_decreasing",
                                           test_non_decreasing,
                                           "d_monotonic_time_ns is non-decreasing");
    group->elements[idx++] = D_ASSERT_TRUE("increases_after_sleep",
                                           test_increases_after_sleep,
                                           "value increases after sleep");
    group->elements[idx++] = D_ASSERT_TRUE("finest_resolution",
                                           test_finest_resolution,
                                           "has finest resolution available");
    group->elements[idx++] = D_ASSERT_TRUE("nanosecond_differences",
                                           test_nanosecond_differences,
                                           "detects nanosecond differences");
    group->elements[idx++] = D_ASSERT_TRUE("elapsed_accuracy",
                                           test_elapsed_accuracy,
                                           "tracks elapsed time accurately");
    group->elements[idx++] = D_ASSERT_TRUE("us_relationship",
                                           test_us_relationship,
                                           "correct relationship with us");
    group->elements[idx++] = D_ASSERT_TRUE("large_values",
                                           test_large_values,
                                           "handles large values correctly");

    return group;
}


/*
d_tests_dtime_monotonic_time_all
  Runs all monotonic time utility tests.
  Tests the following:
  - d_monotonic_time_ms
  - d_monotonic_time_us
  - d_monotonic_time_ns
*/
struct d_test_object*
d_tests_dtime_monotonic_time_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("IX. Monotonic Time Utilities", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dtime_monotonic_time_ms();
    group->elements[idx++] = d_tests_dtime_monotonic_time_us();
    group->elements[idx++] = d_tests_dtime_monotonic_time_ns();

    return group;
}
