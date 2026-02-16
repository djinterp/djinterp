#include ".\test_standalone_tests_sa.h"


/******************************************************************************
 * XII. UTILITY FUNCTION TESTS
 *****************************************************************************/

/*
d_tests_sa_standalone_get_elapsed_time
  Tests the d_test_sa_get_elapsed_time function.
  Tests the following:
  - Returns zero for identical start and end
  - Returns positive value when end > start
  - Returns value in seconds
*/
bool
d_tests_sa_standalone_get_elapsed_time
(
    struct d_test_counter* _counter
)
{
    bool    result;
    clock_t start;
    clock_t end;
    double  elapsed;

    result = true;

    // test 1: returns zero for identical start and end
    start   = 0;
    end     = 0;
    elapsed = d_test_sa_get_elapsed_time(start, end);

    result = d_assert_standalone(
        elapsed == 0.0,
        "elapsed_time_zero",
        "Identical start and end should return 0",
        _counter) && result;

    // test 2: returns positive value when end > start
    start   = 0;
    end     = CLOCKS_PER_SEC;
    elapsed = d_test_sa_get_elapsed_time(start, end);

    result = d_assert_standalone(
        elapsed > 0.0,
        "elapsed_time_positive",
        "end > start should return positive value",
        _counter) && result;

    // test 3: returns approximately 1 second for CLOCKS_PER_SEC difference
    result = d_assert_standalone(
        (elapsed >= 0.99) && (elapsed <= 1.01),
        "elapsed_time_one_second",
        "CLOCKS_PER_SEC ticks should be approximately 1 second",
        _counter) && result;

    // test 4: works with actual clock values
    {
        clock_t actual_start;
        clock_t actual_end;
        double  actual_elapsed;
        volatile int i;

        actual_start = clock();

        // busy wait to consume some time
        for (i = 0; i < 100000; i++)
        {
            // empty loop
        }

        actual_end     = clock();
        actual_elapsed = d_test_sa_get_elapsed_time(actual_start, actual_end);

        result = d_assert_standalone(
            actual_elapsed >= 0.0,
            "elapsed_time_actual_nonnegative",
            "Actual elapsed time should be non-negative",
            _counter) && result;
    }

    // test 5: handles fractional seconds
    start   = 0;
    end     = CLOCKS_PER_SEC / 2;
    elapsed = d_test_sa_get_elapsed_time(start, end);

    result = d_assert_standalone(
        (elapsed >= 0.49) && (elapsed <= 0.51),
        "elapsed_time_half_second",
        "Half CLOCKS_PER_SEC should be approximately 0.5 seconds",
        _counter) && result;

    // test 6: handles multiple seconds
    start   = 0;
    end     = CLOCKS_PER_SEC * 5;
    elapsed = d_test_sa_get_elapsed_time(start, end);

    result = d_assert_standalone(
        (elapsed >= 4.99) && (elapsed <= 5.01),
        "elapsed_time_five_seconds",
        "5 * CLOCKS_PER_SEC should be approximately 5 seconds",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_utility_all
  Aggregation function that runs all utility function tests.
*/
bool
d_tests_sa_standalone_utility_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Utility Functions\n");
    printf("  ----------------------------\n");

    result = d_tests_sa_standalone_get_elapsed_time(_counter) && result;

    return result;
}
