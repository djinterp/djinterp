#include "dtime_tests_sa.h"
#include <limits.h>


/******************************************************************************
 * X. TIMESPEC NORMALIZATION TESTS
 *****************************************************************************/

/*
d_tests_dtime_timespec_normalize
  Tests d_timespec_normalize for normalizing timespec values.
  Tests the following:
  - normalizes positive nanosecond overflow
  - normalizes negative nanoseconds
  - handles already normalized values (no change)
  - handles zero values
  - handles large nanosecond overflow (multiple seconds)
  - handles large negative nanoseconds
  - handles mixed positive seconds with negative nanoseconds
  - handles mixed negative seconds with positive nanoseconds
  - handles edge case at exactly 1 billion nanoseconds
  - result tv_nsec is always in [0, 999999999]
*/
struct d_test_object*
d_tests_dtime_timespec_normalize
(
    void
)
{
    struct d_test_object* group;
    struct timespec       ts;
    bool                  test_positive_overflow;
    bool                  test_negative_nsec;
    bool                  test_already_normalized;
    bool                  test_zero_values;
    bool                  test_large_overflow;
    bool                  test_large_negative;
    bool                  test_positive_sec_negative_nsec;
    bool                  test_negative_sec_positive_nsec;
    bool                  test_exact_billion;
    bool                  test_result_range;
    size_t                idx;

    // test 1: normalizes positive nanosecond overflow (1.5 billion ns -> 1s + 500M ns)
    ts.tv_sec  = 0;
    ts.tv_nsec = 1500000000L;  // 1.5 billion nanoseconds

    d_timespec_normalize(&ts);
    test_positive_overflow = (ts.tv_sec == 1) && (ts.tv_nsec == 500000000L);

    // test 2: normalizes negative nanoseconds (5s - 300M ns -> 4s + 700M ns)
    ts.tv_sec  = 5;
    ts.tv_nsec = -300000000L;

    d_timespec_normalize(&ts);
    test_negative_nsec = (ts.tv_sec == 4) && (ts.tv_nsec == 700000000L);

    // test 3: handles already normalized values (no change)
    ts.tv_sec  = 10;
    ts.tv_nsec = 123456789L;

    d_timespec_normalize(&ts);
    test_already_normalized = (ts.tv_sec == 10) && (ts.tv_nsec == 123456789L);

    // test 4: handles zero values
    ts.tv_sec  = 0;
    ts.tv_nsec = 0;

    d_timespec_normalize(&ts);
    test_zero_values = (ts.tv_sec == 0) && (ts.tv_nsec == 0);

    // test 5: handles large nanosecond overflow (5 billion ns -> 5s)
    // Note: On platforms where tv_nsec is a 32-bit long, values > ~2.1 billion
    // overflow. We test with a value that fits in 32-bit long on those platforms
    // but still exercises the overflow logic. 2.5 billion fits in int64_t but not
    // 32-bit long, so we conditionally test based on platform capabilities.
#if (LONG_MAX > 2500000000LL)
    ts.tv_sec  = 0;
    ts.tv_nsec = 5000000000LL;  // 5 billion nanoseconds
    d_timespec_normalize(&ts);
    test_large_overflow = (ts.tv_sec == 5) && (ts.tv_nsec == 0);
#else
    // On 32-bit long platforms, test with max representable positive value
    // that still causes overflow (2 billion ns = 2 seconds)
    ts.tv_sec  = 0;
    ts.tv_nsec = 2000000000L;  // 2 billion nanoseconds
    d_timespec_normalize(&ts);
    test_large_overflow = (ts.tv_sec == 2) && (ts.tv_nsec == 0);
#endif

    // test 6: handles large negative nanoseconds (0s - 2.5B ns -> -3s + 500M ns)
#if (LONG_MIN < -2500000000LL)
    ts.tv_sec  = 0;
    ts.tv_nsec = -2500000000LL;  // -2.5 billion nanoseconds
    d_timespec_normalize(&ts);
    // result should be -3 seconds + 500000000 nanoseconds = -2.5 seconds total
    test_large_negative = (ts.tv_sec == -3) && (ts.tv_nsec == 500000000L);
#else
    // On 32-bit long platforms, test with max representable negative value
    // that still causes underflow (-1.5 billion ns -> -2s + 500M ns)
    ts.tv_sec  = 0;
    ts.tv_nsec = -1500000000L;  // -1.5 billion nanoseconds
    d_timespec_normalize(&ts);
    test_large_negative = (ts.tv_sec == -2) && (ts.tv_nsec == 500000000L);
#endif

    // test 7: positive seconds with negative nanoseconds (10s - 1.2B ns -> 8s + 800M ns)
    ts.tv_sec  = 10;
    ts.tv_nsec = -1200000000L;  // -1.2 billion nanoseconds

    d_timespec_normalize(&ts);
    test_positive_sec_negative_nsec = (ts.tv_sec == 8) && (ts.tv_nsec == 800000000L);

    // test 8: negative seconds with positive nanoseconds (-5s + 300M ns)
    // This should remain as -5s + 300M ns OR be converted to -4s - 700M ns normalized
    // Standard normalization keeps tv_nsec >= 0, so -5s + 300M ns = -5s + 300M ns
    ts.tv_sec  = -5;
    ts.tv_nsec = 300000000L;

    d_timespec_normalize(&ts);
    // with proper normalization, tv_nsec should be in [0, 999999999]
    test_negative_sec_positive_nsec = (ts.tv_nsec >= 0) && 
                                      (ts.tv_nsec < D_TIME_NSEC_PER_SEC);

    // test 9: exact billion nanoseconds (0s + 1B ns -> 1s + 0ns)
    ts.tv_sec  = 0;
    ts.tv_nsec = D_TIME_NSEC_PER_SEC;  // exactly 1 billion

    d_timespec_normalize(&ts);
    test_exact_billion = (ts.tv_sec == 1) && (ts.tv_nsec == 0);

    // test 10: result tv_nsec is always in valid range after normalization
    {
        struct timespec test_cases[5];
        int             i;
        bool            all_valid;

        test_cases[0].tv_sec = 100;
        test_cases[0].tv_nsec = 3000000000LL;

        test_cases[1].tv_sec = -50;
        test_cases[1].tv_nsec = -500000000L;

        test_cases[2].tv_sec = 0;
        test_cases[2].tv_nsec = 999999999L;

        test_cases[3].tv_sec = 1;
        test_cases[3].tv_nsec = -1L;

        test_cases[4].tv_sec = -1;
        test_cases[4].tv_nsec = 1000000001L;

        all_valid = true;

        for (i = 0; i < 5; i++)
        {
            d_timespec_normalize(&test_cases[i]);

            if ( (test_cases[i].tv_nsec < 0) || 
                 (test_cases[i].tv_nsec >= D_TIME_NSEC_PER_SEC) )
            {
                all_valid = false;
            }
        }

        test_result_range = all_valid;
    }

    // build result tree
    group = d_test_object_new_interior("d_timespec_normalize", 10);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("positive_overflow",
                                           test_positive_overflow,
                                           "normalizes positive nsec overflow");
    group->elements[idx++] = D_ASSERT_TRUE("negative_nsec",
                                           test_negative_nsec,
                                           "normalizes negative nanoseconds");
    group->elements[idx++] = D_ASSERT_TRUE("already_normalized",
                                           test_already_normalized,
                                           "handles already normalized values");
    group->elements[idx++] = D_ASSERT_TRUE("zero_values",
                                           test_zero_values,
                                           "handles zero values");
    group->elements[idx++] = D_ASSERT_TRUE("large_overflow",
                                           test_large_overflow,
                                           "handles large nsec overflow");
    group->elements[idx++] = D_ASSERT_TRUE("large_negative",
                                           test_large_negative,
                                           "handles large negative nsec");
    group->elements[idx++] = D_ASSERT_TRUE("positive_sec_negative_nsec",
                                           test_positive_sec_negative_nsec,
                                           "handles +sec with -nsec");
    group->elements[idx++] = D_ASSERT_TRUE("negative_sec_positive_nsec",
                                           test_negative_sec_positive_nsec,
                                           "handles -sec with +nsec");
    group->elements[idx++] = D_ASSERT_TRUE("exact_billion",
                                           test_exact_billion,
                                           "handles exact billion nsec");
    group->elements[idx++] = D_ASSERT_TRUE("result_range",
                                           test_result_range,
                                           "result tv_nsec always in range");

    return group;
}


/*
d_tests_dtime_timespec_is_valid
  Tests d_timespec_is_valid for checking timespec validity.
  Tests the following:
  - returns true for valid normalized timespec
  - returns true for zero values
  - returns false for negative nanoseconds
  - returns false for nanoseconds >= 1 billion
  - returns true for maximum valid nanoseconds (999999999)
  - returns true for negative seconds with valid nanoseconds
  - returns false for nanoseconds exactly at 1 billion
  - handles edge cases correctly
  - returns false for NULL parameter
  - consistent results for same input
*/
struct d_test_object*
d_tests_dtime_timespec_is_valid
(
    void
)
{
    struct d_test_object* group;
    struct timespec       ts;
    int                   result;
    bool                  test_valid_normalized;
    bool                  test_zero_values;
    bool                  test_negative_nsec;
    bool                  test_nsec_too_large;
    bool                  test_max_valid_nsec;
    bool                  test_negative_sec_valid_nsec;
    bool                  test_exact_billion;
    bool                  test_edge_cases;
    bool                  test_null_param;
    bool                  test_consistency;
    size_t                idx;

    // test 1: returns true (non-zero) for valid normalized timespec
    ts.tv_sec  = 10;
    ts.tv_nsec = 500000000L;

    result = d_timespec_is_valid(&ts);
    test_valid_normalized = (result != 0);

    // test 2: returns true for zero values
    ts.tv_sec  = 0;
    ts.tv_nsec = 0;

    result = d_timespec_is_valid(&ts);
    test_zero_values = (result != 0);

    // test 3: returns false (zero) for negative nanoseconds
    ts.tv_sec  = 5;
    ts.tv_nsec = -100;

    result = d_timespec_is_valid(&ts);
    test_negative_nsec = (result == 0);

    // test 4: returns false for nanoseconds >= 1 billion
    ts.tv_sec  = 5;
    ts.tv_nsec = 1500000000L;

    result = d_timespec_is_valid(&ts);
    test_nsec_too_large = (result == 0);

    // test 5: returns true for maximum valid nanoseconds (999999999)
    ts.tv_sec  = 5;
    ts.tv_nsec = 999999999L;

    result = d_timespec_is_valid(&ts);
    test_max_valid_nsec = (result != 0);

    // test 6: returns true for negative seconds with valid nanoseconds
    ts.tv_sec  = -10;
    ts.tv_nsec = 500000000L;

    result = d_timespec_is_valid(&ts);
    test_negative_sec_valid_nsec = (result != 0);

    // test 7: returns false for nanoseconds exactly at 1 billion
    ts.tv_sec  = 0;
    ts.tv_nsec = D_TIME_NSEC_PER_SEC;  // exactly 1 billion

    result = d_timespec_is_valid(&ts);
    test_exact_billion = (result == 0);

    // test 8: handles various edge cases
    {
        struct timespec valid_cases[4];
        struct timespec invalid_cases[4];
        int             i;
        bool            all_valid_pass;
        bool            all_invalid_fail;

        // valid cases
        valid_cases[0].tv_sec = 0;
        valid_cases[0].tv_nsec = 0;

        valid_cases[1].tv_sec = 1000000;
        valid_cases[1].tv_nsec = 0;

        valid_cases[2].tv_sec = -1;
        valid_cases[2].tv_nsec = 999999999L;

        valid_cases[3].tv_sec = 0;
        valid_cases[3].tv_nsec = 1;

        // invalid cases
        invalid_cases[0].tv_sec = 0;
        invalid_cases[0].tv_nsec = -1;

        invalid_cases[1].tv_sec = 0;
        invalid_cases[1].tv_nsec = 1000000000L;

        invalid_cases[2].tv_sec = 5;
        invalid_cases[2].tv_nsec = 2000000000L;

        invalid_cases[3].tv_sec = -5;
        invalid_cases[3].tv_nsec = -500000000L;

        all_valid_pass = true;
        all_invalid_fail = true;

        for (i = 0; i < 4; i++)
        {
            if (d_timespec_is_valid(&valid_cases[i]) == 0)
            {
                all_valid_pass = false;
            }

            if (d_timespec_is_valid(&invalid_cases[i]) != 0)
            {
                all_invalid_fail = false;
            }
        }

        test_edge_cases = all_valid_pass && all_invalid_fail;
    }

    // test 9: returns false (zero) for NULL parameter
    result = d_timespec_is_valid(NULL);
    test_null_param = (result == 0);

    // test 10: consistent results for same input
    ts.tv_sec  = 123;
    ts.tv_nsec = 456789012L;

    {
        int r1;
        int r2;
        int r3;

        r1 = d_timespec_is_valid(&ts);
        r2 = d_timespec_is_valid(&ts);
        r3 = d_timespec_is_valid(&ts);

        test_consistency = (r1 == r2) && (r2 == r3) && (r1 != 0);
    }

    // build result tree
    group = d_test_object_new_interior("d_timespec_is_valid", 10);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("valid_normalized",
                                           test_valid_normalized,
                                           "returns true for valid timespec");
    group->elements[idx++] = D_ASSERT_TRUE("zero_values",
                                           test_zero_values,
                                           "returns true for zero values");
    group->elements[idx++] = D_ASSERT_TRUE("negative_nsec",
                                           test_negative_nsec,
                                           "returns false for negative nsec");
    group->elements[idx++] = D_ASSERT_TRUE("nsec_too_large",
                                           test_nsec_too_large,
                                           "returns false for nsec >= 1B");
    group->elements[idx++] = D_ASSERT_TRUE("max_valid_nsec",
                                           test_max_valid_nsec,
                                           "returns true for max valid nsec");
    group->elements[idx++] = D_ASSERT_TRUE("negative_sec_valid_nsec",
                                           test_negative_sec_valid_nsec,
                                           "handles negative sec with valid nsec");
    group->elements[idx++] = D_ASSERT_TRUE("exact_billion",
                                           test_exact_billion,
                                           "returns false for exact billion");
    group->elements[idx++] = D_ASSERT_TRUE("edge_cases",
                                           test_edge_cases,
                                           "handles edge cases correctly");
    group->elements[idx++] = D_ASSERT_TRUE("null_param",
                                           test_null_param,
                                           "returns false for NULL");
    group->elements[idx++] = D_ASSERT_TRUE("consistency",
                                           test_consistency,
                                           "produces consistent results");

    return group;
}


/*
d_tests_dtime_timespec_normalization_all
  Runs all timespec normalization tests.
  Tests the following:
  - d_timespec_normalize
  - d_timespec_is_valid
*/
struct d_test_object*
d_tests_dtime_timespec_normalization_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("X. Timespec Normalization", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dtime_timespec_normalize();
    group->elements[idx++] = d_tests_dtime_timespec_is_valid();

    return group;
}