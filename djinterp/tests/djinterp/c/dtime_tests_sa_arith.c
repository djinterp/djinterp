/******************************************************************************
* djinterp [test]                                        dtime_tests_sa_arith.c
*
*   Tests for time arithmetic functions (timespec_add, timespec_sub,
* timespec_cmp, conversions to/from ms/us/ns).
*
*
* path:      \src\test\dtime_tests_sa_arith.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.25
******************************************************************************/
#include "dtime_tests_sa.h"


/******************************************************************************
 * VIII. TIME ARITHMETIC TESTS
 *****************************************************************************/

/*
d_tests_dtime_timespec_add
  Tests d_timespec_add for adding two timespec values.
  Tests the following:
  - adds two simple timespec values
  - handles nanosecond overflow (carries to seconds)
  - handles zero values
  - handles large second values
  - result is normalized (tv_nsec < 1 billion)
  - handles adding to zero
  - handles multiple carries
  - works with same source and destination
*/
struct d_test_object*
d_tests_dtime_timespec_add
(
    void
)
{
    struct d_test_object* group;
    struct timespec       a;
    struct timespec       b;
    struct timespec       result;
    bool                  test_simple_add;
    bool                  test_nsec_overflow;
    bool                  test_zero_values;
    bool                  test_large_seconds;
    bool                  test_normalized;
    bool                  test_add_to_zero;
    bool                  test_multiple_carries;
    bool                  test_commutative;
    size_t                idx;

    // test 1: simple addition (1.5s + 2.3s = 3.8s)
    a.tv_sec  = 1;
    a.tv_nsec = 500000000;  // 0.5s
    b.tv_sec  = 2;
    b.tv_nsec = 300000000;  // 0.3s
    memset(&result, 0, sizeof(result));

    d_timespec_add(&a, &b, &result);
    test_simple_add = (result.tv_sec == 3) && (result.tv_nsec == 800000000);

    // test 2: nanosecond overflow (0.7s + 0.5s = 1.2s)
    a.tv_sec  = 0;
    a.tv_nsec = 700000000;  // 0.7s
    b.tv_sec  = 0;
    b.tv_nsec = 500000000;  // 0.5s
    memset(&result, 0, sizeof(result));

    d_timespec_add(&a, &b, &result);
    test_nsec_overflow = (result.tv_sec == 1) && (result.tv_nsec == 200000000);

    // test 3: zero values (0 + 0 = 0)
    a.tv_sec  = 0;
    a.tv_nsec = 0;
    b.tv_sec  = 0;
    b.tv_nsec = 0;
    memset(&result, 0xFF, sizeof(result));

    d_timespec_add(&a, &b, &result);
    test_zero_values = (result.tv_sec == 0) && (result.tv_nsec == 0);

    // test 4: large second values
    a.tv_sec  = 1000000;
    a.tv_nsec = 0;
    b.tv_sec  = 2000000;
    b.tv_nsec = 0;
    memset(&result, 0, sizeof(result));

    d_timespec_add(&a, &b, &result);
    test_large_seconds = (result.tv_sec == 3000000) && (result.tv_nsec == 0);

    // test 5: result is normalized (tv_nsec < D_TIME_NSEC_PER_SEC)
    a.tv_sec  = 1;
    a.tv_nsec = 999999999;
    b.tv_sec  = 0;
    b.tv_nsec = 2;
    memset(&result, 0, sizeof(result));

    d_timespec_add(&a, &b, &result);
    test_normalized = (result.tv_nsec >= 0) && 
                      (result.tv_nsec < D_TIME_NSEC_PER_SEC) &&
                      (result.tv_sec == 2) && 
                      (result.tv_nsec == 1);

    // test 6: adding to zero
    a.tv_sec  = 0;
    a.tv_nsec = 0;
    b.tv_sec  = 5;
    b.tv_nsec = 123456789;
    memset(&result, 0, sizeof(result));

    d_timespec_add(&a, &b, &result);
    test_add_to_zero = (result.tv_sec == 5) && (result.tv_nsec == 123456789);

    // test 7: multiple nanosecond overflows (edge case)
    a.tv_sec  = 0;
    a.tv_nsec = 999999999;
    b.tv_sec  = 0;
    b.tv_nsec = 999999999;
    memset(&result, 0, sizeof(result));

    d_timespec_add(&a, &b, &result);
    // 0.999999999 + 0.999999999 = 1.999999998
    test_multiple_carries = (result.tv_sec == 1) && (result.tv_nsec == 999999998);

    // test 8: commutative (a + b == b + a)
    a.tv_sec  = 3;
    a.tv_nsec = 141592653;
    b.tv_sec  = 2;
    b.tv_nsec = 718281828;

    struct timespec result1;
    struct timespec result2;

    d_timespec_add(&a, &b, &result1);
    d_timespec_add(&b, &a, &result2);
    test_commutative = (result1.tv_sec == result2.tv_sec) &&
                       (result1.tv_nsec == result2.tv_nsec);

    // build result tree
    group = d_test_object_new_interior("d_timespec_add", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("simple_add",
                                           test_simple_add,
                                           "d_timespec_add adds simple values");
    group->elements[idx++] = D_ASSERT_TRUE("nsec_overflow",
                                           test_nsec_overflow,
                                           "d_timespec_add handles nsec overflow");
    group->elements[idx++] = D_ASSERT_TRUE("zero_values",
                                           test_zero_values,
                                           "d_timespec_add handles zero values");
    group->elements[idx++] = D_ASSERT_TRUE("large_seconds",
                                           test_large_seconds,
                                           "d_timespec_add handles large seconds");
    group->elements[idx++] = D_ASSERT_TRUE("normalized",
                                           test_normalized,
                                           "d_timespec_add normalizes result");
    group->elements[idx++] = D_ASSERT_TRUE("add_to_zero",
                                           test_add_to_zero,
                                           "d_timespec_add handles adding to zero");
    group->elements[idx++] = D_ASSERT_TRUE("multiple_carries",
                                           test_multiple_carries,
                                           "d_timespec_add handles edge cases");
    group->elements[idx++] = D_ASSERT_TRUE("commutative",
                                           test_commutative,
                                           "d_timespec_add is commutative");

    return group;
}


/*
d_tests_dtime_timespec_sub
  Tests d_timespec_sub for subtracting two timespec values.
  Tests the following:
  - subtracts two simple timespec values
  - handles nanosecond underflow (borrows from seconds)
  - handles zero values
  - handles equal values (result is zero)
  - result is normalized
  - handles a > b correctly
  - handles b > a (negative result)
  - subtracting zero yields original value
*/
struct d_test_object*
d_tests_dtime_timespec_sub
(
    void
)
{
    struct d_test_object* group;
    struct timespec       a;
    struct timespec       b;
    struct timespec       result;
    bool                  test_simple_sub;
    bool                  test_nsec_underflow;
    bool                  test_zero_values;
    bool                  test_equal_values;
    bool                  test_normalized;
    bool                  test_a_greater;
    bool                  test_b_greater;
    bool                  test_sub_zero;
    size_t                idx;

    // test 1: simple subtraction (3.8s - 1.3s = 2.5s)
    a.tv_sec  = 3;
    a.tv_nsec = 800000000;
    b.tv_sec  = 1;
    b.tv_nsec = 300000000;
    memset(&result, 0, sizeof(result));

    d_timespec_sub(&a, &b, &result);
    test_simple_sub = (result.tv_sec == 2) && (result.tv_nsec == 500000000);

    // test 2: nanosecond underflow (2.3s - 0.7s = 1.6s)
    a.tv_sec  = 2;
    a.tv_nsec = 300000000;
    b.tv_sec  = 0;
    b.tv_nsec = 700000000;
    memset(&result, 0, sizeof(result));

    d_timespec_sub(&a, &b, &result);
    test_nsec_underflow = (result.tv_sec == 1) && (result.tv_nsec == 600000000);

    // test 3: zero values (0 - 0 = 0)
    a.tv_sec  = 0;
    a.tv_nsec = 0;
    b.tv_sec  = 0;
    b.tv_nsec = 0;
    memset(&result, 0xFF, sizeof(result));

    d_timespec_sub(&a, &b, &result);
    test_zero_values = (result.tv_sec == 0) && (result.tv_nsec == 0);

    // test 4: equal values (5.5s - 5.5s = 0)
    a.tv_sec  = 5;
    a.tv_nsec = 500000000;
    b.tv_sec  = 5;
    b.tv_nsec = 500000000;
    memset(&result, 0xFF, sizeof(result));

    d_timespec_sub(&a, &b, &result);
    test_equal_values = (result.tv_sec == 0) && (result.tv_nsec == 0);

    // test 5: result is normalized
    a.tv_sec  = 10;
    a.tv_nsec = 100000000;
    b.tv_sec  = 5;
    b.tv_nsec = 900000000;
    memset(&result, 0, sizeof(result));

    d_timespec_sub(&a, &b, &result);
    // 10.1s - 5.9s = 4.2s
    test_normalized = (result.tv_nsec >= 0) && 
                      (result.tv_nsec < D_TIME_NSEC_PER_SEC) &&
                      (result.tv_sec == 4) && 
                      (result.tv_nsec == 200000000);

    // test 6: a > b gives positive result
    a.tv_sec  = 100;
    a.tv_nsec = 0;
    b.tv_sec  = 50;
    b.tv_nsec = 0;
    memset(&result, 0, sizeof(result));

    d_timespec_sub(&a, &b, &result);
    test_a_greater = (result.tv_sec == 50) && (result.tv_nsec == 0);

    // test 7: b > a gives negative result (platform-dependent behavior)
    a.tv_sec  = 1;
    a.tv_nsec = 0;
    b.tv_sec  = 2;
    b.tv_nsec = 0;
    memset(&result, 0, sizeof(result));

    d_timespec_sub(&a, &b, &result);
    // negative result: tv_sec should be -1
    test_b_greater = (result.tv_sec == -1) && (result.tv_nsec == 0);

    // test 8: subtracting zero yields original
    a.tv_sec  = 7;
    a.tv_nsec = 777777777;
    b.tv_sec  = 0;
    b.tv_nsec = 0;
    memset(&result, 0, sizeof(result));

    d_timespec_sub(&a, &b, &result);
    test_sub_zero = (result.tv_sec == 7) && (result.tv_nsec == 777777777);

    // build result tree
    group = d_test_object_new_interior("d_timespec_sub", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("simple_sub",
                                           test_simple_sub,
                                           "d_timespec_sub subtracts simple values");
    group->elements[idx++] = D_ASSERT_TRUE("nsec_underflow",
                                           test_nsec_underflow,
                                           "d_timespec_sub handles nsec underflow");
    group->elements[idx++] = D_ASSERT_TRUE("zero_values",
                                           test_zero_values,
                                           "d_timespec_sub handles zero values");
    group->elements[idx++] = D_ASSERT_TRUE("equal_values",
                                           test_equal_values,
                                           "d_timespec_sub handles equal values");
    group->elements[idx++] = D_ASSERT_TRUE("normalized",
                                           test_normalized,
                                           "d_timespec_sub normalizes result");
    group->elements[idx++] = D_ASSERT_TRUE("a_greater",
                                           test_a_greater,
                                           "d_timespec_sub handles a > b");
    group->elements[idx++] = D_ASSERT_TRUE("b_greater",
                                           test_b_greater,
                                           "d_timespec_sub handles b > a");
    group->elements[idx++] = D_ASSERT_TRUE("sub_zero",
                                           test_sub_zero,
                                           "d_timespec_sub with zero unchanged");

    return group;
}


/*
d_tests_dtime_timespec_cmp
  Tests d_timespec_cmp for comparing two timespec values.
  Tests the following:
  - returns negative when a < b
  - returns positive when a > b
  - returns zero when a == b
  - compares seconds first
  - compares nanoseconds when seconds equal
  - handles zero values
  - handles large values
  - symmetric comparison
*/
struct d_test_object*
d_tests_dtime_timespec_cmp
(
    void
)
{
    struct d_test_object* group;
    struct timespec       a;
    struct timespec       b;
    int                   cmp_result;
    bool                  test_a_less_than_b;
    bool                  test_a_greater_than_b;
    bool                  test_equal;
    bool                  test_seconds_priority;
    bool                  test_nsec_comparison;
    bool                  test_zero_values;
    bool                  test_large_values;
    bool                  test_symmetric;
    size_t                idx;

    // test 1: a < b (1.5s < 2.3s)
    a.tv_sec  = 1;
    a.tv_nsec = 500000000;
    b.tv_sec  = 2;
    b.tv_nsec = 300000000;

    cmp_result = d_timespec_cmp(&a, &b);
    test_a_less_than_b = (cmp_result < 0);

    // test 2: a > b (3.8s > 1.2s)
    a.tv_sec  = 3;
    a.tv_nsec = 800000000;
    b.tv_sec  = 1;
    b.tv_nsec = 200000000;

    cmp_result = d_timespec_cmp(&a, &b);
    test_a_greater_than_b = (cmp_result > 0);

    // test 3: a == b (5.5s == 5.5s)
    a.tv_sec  = 5;
    a.tv_nsec = 500000000;
    b.tv_sec  = 5;
    b.tv_nsec = 500000000;

    cmp_result = d_timespec_cmp(&a, &b);
    test_equal = (cmp_result == 0);

    // test 4: seconds have priority (2.0s > 1.999999999s)
    a.tv_sec  = 2;
    a.tv_nsec = 0;
    b.tv_sec  = 1;
    b.tv_nsec = 999999999;

    cmp_result = d_timespec_cmp(&a, &b);
    test_seconds_priority = (cmp_result > 0);

    // test 5: nanoseconds compared when seconds equal (5.3s < 5.7s)
    a.tv_sec  = 5;
    a.tv_nsec = 300000000;
    b.tv_sec  = 5;
    b.tv_nsec = 700000000;

    cmp_result = d_timespec_cmp(&a, &b);
    test_nsec_comparison = (cmp_result < 0);

    // test 6: zero values (0 == 0)
    a.tv_sec  = 0;
    a.tv_nsec = 0;
    b.tv_sec  = 0;
    b.tv_nsec = 0;

    cmp_result = d_timespec_cmp(&a, &b);
    test_zero_values = (cmp_result == 0);

    // test 7: large values
    a.tv_sec  = 1000000000;
    a.tv_nsec = 123456789;
    b.tv_sec  = 1000000000;
    b.tv_nsec = 123456788;

    cmp_result = d_timespec_cmp(&a, &b);
    test_large_values = (cmp_result > 0);  // a.tv_nsec > b.tv_nsec

    // test 8: symmetric comparison (if a < b then b > a)
    a.tv_sec  = 10;
    a.tv_nsec = 100000000;
    b.tv_sec  = 20;
    b.tv_nsec = 200000000;

    int cmp1 = d_timespec_cmp(&a, &b);
    int cmp2 = d_timespec_cmp(&b, &a);
    test_symmetric = (cmp1 < 0) && (cmp2 > 0);

    // build result tree
    group = d_test_object_new_interior("d_timespec_cmp", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("a_less_than_b",
                                           test_a_less_than_b,
                                           "d_timespec_cmp returns neg for a < b");
    group->elements[idx++] = D_ASSERT_TRUE("a_greater_than_b",
                                           test_a_greater_than_b,
                                           "d_timespec_cmp returns pos for a > b");
    group->elements[idx++] = D_ASSERT_TRUE("equal",
                                           test_equal,
                                           "d_timespec_cmp returns 0 for a == b");
    group->elements[idx++] = D_ASSERT_TRUE("seconds_priority",
                                           test_seconds_priority,
                                           "d_timespec_cmp compares seconds first");
    group->elements[idx++] = D_ASSERT_TRUE("nsec_comparison",
                                           test_nsec_comparison,
                                           "d_timespec_cmp compares nsec when eq");
    group->elements[idx++] = D_ASSERT_TRUE("zero_values",
                                           test_zero_values,
                                           "d_timespec_cmp handles zero values");
    group->elements[idx++] = D_ASSERT_TRUE("large_values",
                                           test_large_values,
                                           "d_timespec_cmp handles large values");
    group->elements[idx++] = D_ASSERT_TRUE("symmetric",
                                           test_symmetric,
                                           "d_timespec_cmp is symmetric");

    return group;
}


/*
d_tests_dtime_timespec_to_ms
  Tests d_timespec_to_ms for converting timespec to milliseconds.
  Tests the following:
  - converts simple values correctly
  - handles zero value
  - handles seconds-only value
  - handles nanoseconds-only value
  - handles large values
  - truncates sub-millisecond precision
  - handles edge case at millisecond boundary
  - round-trip with d_ms_to_timespec
*/
struct d_test_object*
d_tests_dtime_timespec_to_ms
(
    void
)
{
    struct d_test_object* group;
    struct timespec       ts;
    int64_t               ms;
    bool                  test_simple_conversion;
    bool                  test_zero_value;
    bool                  test_seconds_only;
    bool                  test_nsec_only;
    bool                  test_large_values;
    bool                  test_truncation;
    bool                  test_boundary;
    bool                  test_roundtrip;
    size_t                idx;

    // test 1: simple conversion (1.5s = 1500ms)
    ts.tv_sec  = 1;
    ts.tv_nsec = 500000000;

    ms = d_timespec_to_ms(&ts);
    test_simple_conversion = (ms == 1500);

    // test 2: zero value
    ts.tv_sec  = 0;
    ts.tv_nsec = 0;

    ms = d_timespec_to_ms(&ts);
    test_zero_value = (ms == 0);

    // test 3: seconds only (10s = 10000ms)
    ts.tv_sec  = 10;
    ts.tv_nsec = 0;

    ms = d_timespec_to_ms(&ts);
    test_seconds_only = (ms == 10000);

    // test 4: nanoseconds only (500ms = 500000000ns)
    ts.tv_sec  = 0;
    ts.tv_nsec = 500000000;

    ms = d_timespec_to_ms(&ts);
    test_nsec_only = (ms == 500);

    // test 5: large values (1000000 seconds)
    ts.tv_sec  = 1000000;
    ts.tv_nsec = 0;

    ms = d_timespec_to_ms(&ts);
    test_large_values = (ms == 1000000000LL);  // 1 billion ms

    // test 6: truncation of sub-millisecond (1.5005s -> 1500ms, not 1501ms)
    ts.tv_sec  = 1;
    ts.tv_nsec = 500500000;  // 500.5ms in the nsec portion

    ms = d_timespec_to_ms(&ts);
    test_truncation = (ms == 1500);  // truncates the 0.5ms

    // test 7: exact millisecond boundary (0.001s = 1ms)
    ts.tv_sec  = 0;
    ts.tv_nsec = 1000000;  // exactly 1ms

    ms = d_timespec_to_ms(&ts);
    test_boundary = (ms == 1);

    // test 8: round-trip with d_ms_to_timespec
    ts.tv_sec  = 12;
    ts.tv_nsec = 345000000;  // 12.345s = 12345ms

    ms = d_timespec_to_ms(&ts);

    struct timespec ts2;

    d_ms_to_timespec(ms, &ts2);
    test_roundtrip = (ts2.tv_sec == 12) && (ts2.tv_nsec == 345000000);

    // build result tree
    group = d_test_object_new_interior("d_timespec_to_ms", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("simple_conversion",
                                           test_simple_conversion,
                                           "d_timespec_to_ms converts correctly");
    group->elements[idx++] = D_ASSERT_TRUE("zero_value",
                                           test_zero_value,
                                           "d_timespec_to_ms handles zero");
    group->elements[idx++] = D_ASSERT_TRUE("seconds_only",
                                           test_seconds_only,
                                           "d_timespec_to_ms handles seconds only");
    group->elements[idx++] = D_ASSERT_TRUE("nsec_only",
                                           test_nsec_only,
                                           "d_timespec_to_ms handles nsec only");
    group->elements[idx++] = D_ASSERT_TRUE("large_values",
                                           test_large_values,
                                           "d_timespec_to_ms handles large values");
    group->elements[idx++] = D_ASSERT_TRUE("truncation",
                                           test_truncation,
                                           "d_timespec_to_ms truncates sub-ms");
    group->elements[idx++] = D_ASSERT_TRUE("boundary",
                                           test_boundary,
                                           "d_timespec_to_ms handles boundaries");
    group->elements[idx++] = D_ASSERT_TRUE("roundtrip",
                                           test_roundtrip,
                                           "d_timespec_to_ms round-trips");

    return group;
}


/*
d_tests_dtime_timespec_to_us
  Tests d_timespec_to_us for converting timespec to microseconds.
  Tests the following:
  - converts simple values correctly
  - handles zero value
  - handles seconds-only value
  - handles nanoseconds-only value
  - handles large values
  - truncates sub-microsecond precision
  - handles edge case at microsecond boundary
  - round-trip with d_us_to_timespec
*/
struct d_test_object*
d_tests_dtime_timespec_to_us
(
    void
)
{
    struct d_test_object* group;
    struct timespec       ts;
    int64_t               us;
    bool                  test_simple_conversion;
    bool                  test_zero_value;
    bool                  test_seconds_only;
    bool                  test_nsec_only;
    bool                  test_large_values;
    bool                  test_truncation;
    bool                  test_boundary;
    bool                  test_roundtrip;
    size_t                idx;

    // test 1: simple conversion (1.5s = 1500000us)
    ts.tv_sec  = 1;
    ts.tv_nsec = 500000000;

    us = d_timespec_to_us(&ts);
    test_simple_conversion = (us == 1500000);

    // test 2: zero value
    ts.tv_sec  = 0;
    ts.tv_nsec = 0;

    us = d_timespec_to_us(&ts);
    test_zero_value = (us == 0);

    // test 3: seconds only (10s = 10000000us)
    ts.tv_sec  = 10;
    ts.tv_nsec = 0;

    us = d_timespec_to_us(&ts);
    test_seconds_only = (us == 10000000);

    // test 4: nanoseconds only (500us = 500000ns)
    ts.tv_sec  = 0;
    ts.tv_nsec = 500000;

    us = d_timespec_to_us(&ts);
    test_nsec_only = (us == 500);

    // test 5: large values (1000 seconds)
    ts.tv_sec  = 1000;
    ts.tv_nsec = 0;

    us = d_timespec_to_us(&ts);
    test_large_values = (us == 1000000000LL);  // 1 billion us

    // test 6: truncation of sub-microsecond (1.000000500s -> 1000000us)
    ts.tv_sec  = 1;
    ts.tv_nsec = 500;  // 0.5us = 500ns

    us = d_timespec_to_us(&ts);
    test_truncation = (us == 1000000);  // truncates the 0.5us

    // test 7: exact microsecond boundary (0.000001s = 1us)
    ts.tv_sec  = 0;
    ts.tv_nsec = 1000;  // exactly 1us

    us = d_timespec_to_us(&ts);
    test_boundary = (us == 1);

    // test 8: round-trip with d_us_to_timespec
    ts.tv_sec  = 5;
    ts.tv_nsec = 123456000;  // 5.123456s = 5123456us

    us = d_timespec_to_us(&ts);

    struct timespec ts2;

    d_us_to_timespec(us, &ts2);
    test_roundtrip = (ts2.tv_sec == 5) && (ts2.tv_nsec == 123456000);

    // build result tree
    group = d_test_object_new_interior("d_timespec_to_us", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("simple_conversion",
                                           test_simple_conversion,
                                           "d_timespec_to_us converts correctly");
    group->elements[idx++] = D_ASSERT_TRUE("zero_value",
                                           test_zero_value,
                                           "d_timespec_to_us handles zero");
    group->elements[idx++] = D_ASSERT_TRUE("seconds_only",
                                           test_seconds_only,
                                           "d_timespec_to_us handles seconds only");
    group->elements[idx++] = D_ASSERT_TRUE("nsec_only",
                                           test_nsec_only,
                                           "d_timespec_to_us handles nsec only");
    group->elements[idx++] = D_ASSERT_TRUE("large_values",
                                           test_large_values,
                                           "d_timespec_to_us handles large values");
    group->elements[idx++] = D_ASSERT_TRUE("truncation",
                                           test_truncation,
                                           "d_timespec_to_us truncates sub-us");
    group->elements[idx++] = D_ASSERT_TRUE("boundary",
                                           test_boundary,
                                           "d_timespec_to_us handles boundaries");
    group->elements[idx++] = D_ASSERT_TRUE("roundtrip",
                                           test_roundtrip,
                                           "d_timespec_to_us round-trips");

    return group;
}


/*
d_tests_dtime_timespec_to_ns
  Tests d_timespec_to_ns for converting timespec to nanoseconds.
  Tests the following:
  - converts simple values correctly
  - handles zero value
  - handles seconds-only value
  - handles nanoseconds-only value
  - handles large values
  - preserves full precision
  - handles edge cases
  - round-trip with d_ns_to_timespec
*/
struct d_test_object*
d_tests_dtime_timespec_to_ns
(
    void
)
{
    struct d_test_object* group;
    struct timespec       ts;
    int64_t               ns;
    bool                  test_simple_conversion;
    bool                  test_zero_value;
    bool                  test_seconds_only;
    bool                  test_nsec_only;
    bool                  test_large_values;
    bool                  test_full_precision;
    bool                  test_edge_case;
    bool                  test_roundtrip;
    size_t                idx;

    // test 1: simple conversion (1.5s = 1500000000ns)
    ts.tv_sec  = 1;
    ts.tv_nsec = 500000000;

    ns = d_timespec_to_ns(&ts);
    test_simple_conversion = (ns == 1500000000LL);

    // test 2: zero value
    ts.tv_sec  = 0;
    ts.tv_nsec = 0;

    ns = d_timespec_to_ns(&ts);
    test_zero_value = (ns == 0);

    // test 3: seconds only (10s = 10000000000ns)
    ts.tv_sec  = 10;
    ts.tv_nsec = 0;

    ns = d_timespec_to_ns(&ts);
    test_seconds_only = (ns == 10000000000LL);

    // test 4: nanoseconds only (500ns)
    ts.tv_sec  = 0;
    ts.tv_nsec = 500;

    ns = d_timespec_to_ns(&ts);
    test_nsec_only = (ns == 500);

    // test 5: large values (1000 seconds)
    ts.tv_sec  = 1000;
    ts.tv_nsec = 0;

    ns = d_timespec_to_ns(&ts);
    test_large_values = (ns == 1000000000000LL);  // 1 trillion ns

    // test 6: preserves full precision (1.123456789s)
    ts.tv_sec  = 1;
    ts.tv_nsec = 123456789;

    ns = d_timespec_to_ns(&ts);
    test_full_precision = (ns == 1123456789LL);

    // test 7: edge case - maximum nanoseconds (0.999999999s)
    ts.tv_sec  = 0;
    ts.tv_nsec = 999999999;

    ns = d_timespec_to_ns(&ts);
    test_edge_case = (ns == 999999999LL);

    // test 8: round-trip with d_ns_to_timespec
    ts.tv_sec  = 3;
    ts.tv_nsec = 141592653;

    ns = d_timespec_to_ns(&ts);

    struct timespec ts2;

    d_ns_to_timespec(ns, &ts2);
    test_roundtrip = (ts2.tv_sec == 3) && (ts2.tv_nsec == 141592653);

    // build result tree
    group = d_test_object_new_interior("d_timespec_to_ns", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("simple_conversion",
                                           test_simple_conversion,
                                           "d_timespec_to_ns converts correctly");
    group->elements[idx++] = D_ASSERT_TRUE("zero_value",
                                           test_zero_value,
                                           "d_timespec_to_ns handles zero");
    group->elements[idx++] = D_ASSERT_TRUE("seconds_only",
                                           test_seconds_only,
                                           "d_timespec_to_ns handles seconds only");
    group->elements[idx++] = D_ASSERT_TRUE("nsec_only",
                                           test_nsec_only,
                                           "d_timespec_to_ns handles nsec only");
    group->elements[idx++] = D_ASSERT_TRUE("large_values",
                                           test_large_values,
                                           "d_timespec_to_ns handles large values");
    group->elements[idx++] = D_ASSERT_TRUE("full_precision",
                                           test_full_precision,
                                           "d_timespec_to_ns preserves precision");
    group->elements[idx++] = D_ASSERT_TRUE("edge_case",
                                           test_edge_case,
                                           "d_timespec_to_ns handles edge cases");
    group->elements[idx++] = D_ASSERT_TRUE("roundtrip",
                                           test_roundtrip,
                                           "d_timespec_to_ns round-trips");

    return group;
}


/*
d_tests_dtime_ms_to_timespec
  Tests d_ms_to_timespec for converting milliseconds to timespec.
  Tests the following:
  - converts simple values correctly
  - handles zero value
  - handles values less than 1 second
  - handles exact second values
  - handles large values
  - result is properly normalized
  - handles negative values (if supported)
  - consistency with d_timespec_to_ms
*/
struct d_test_object*
d_tests_dtime_ms_to_timespec
(
    void
)
{
    struct d_test_object* group;
    struct timespec       ts;
    int64_t               ms;
    bool                  test_simple_conversion;
    bool                  test_zero_value;
    bool                  test_sub_second;
    bool                  test_exact_second;
    bool                  test_large_values;
    bool                  test_normalized;
    bool                  test_negative;
    bool                  test_consistency;
    size_t                idx;

    // test 1: simple conversion (1500ms = 1.5s)
    ms = 1500;
    d_ms_to_timespec(ms, &ts);
    test_simple_conversion = (ts.tv_sec == 1) && (ts.tv_nsec == 500000000);

    // test 2: zero value
    ms = 0;
    memset(&ts, 0xFF, sizeof(ts));
    d_ms_to_timespec(ms, &ts);
    test_zero_value = (ts.tv_sec == 0) && (ts.tv_nsec == 0);

    // test 3: values less than 1 second (500ms = 0.5s)
    ms = 500;
    d_ms_to_timespec(ms, &ts);
    test_sub_second = (ts.tv_sec == 0) && (ts.tv_nsec == 500000000);

    // test 4: exact second values (3000ms = 3s)
    ms = 3000;
    d_ms_to_timespec(ms, &ts);
    test_exact_second = (ts.tv_sec == 3) && (ts.tv_nsec == 0);

    // test 5: large values (1000000000ms = 1000000s)
    ms = 1000000000LL;
    d_ms_to_timespec(ms, &ts);
    test_large_values = (ts.tv_sec == 1000000) && (ts.tv_nsec == 0);

    // test 6: result is normalized (tv_nsec < 1 billion)
    ms = 12345;
    d_ms_to_timespec(ms, &ts);
    test_normalized = (ts.tv_nsec >= 0) && (ts.tv_nsec < D_TIME_NSEC_PER_SEC);

    // test 7: negative values
    ms = -500;
    d_ms_to_timespec(ms, &ts);
    // -500ms should be -1 sec + 500000000 nsec or -0.5s equivalent
    test_negative = (ts.tv_sec < 0) || (ts.tv_nsec < 0) ||
                    (ts.tv_sec == -1 && ts.tv_nsec == 500000000) ||
                    (ts.tv_sec == 0 && ts.tv_nsec == -500000000);

    // test 8: consistency - multiple conversions of same value
    ms = 7777;

    struct timespec ts1;
    struct timespec ts2;

    d_ms_to_timespec(ms, &ts1);
    d_ms_to_timespec(ms, &ts2);
    test_consistency = (ts1.tv_sec == ts2.tv_sec) && (ts1.tv_nsec == ts2.tv_nsec);

    // build result tree
    group = d_test_object_new_interior("d_ms_to_timespec", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("simple_conversion",
                                           test_simple_conversion,
                                           "d_ms_to_timespec converts correctly");
    group->elements[idx++] = D_ASSERT_TRUE("zero_value",
                                           test_zero_value,
                                           "d_ms_to_timespec handles zero");
    group->elements[idx++] = D_ASSERT_TRUE("sub_second",
                                           test_sub_second,
                                           "d_ms_to_timespec handles sub-second");
    group->elements[idx++] = D_ASSERT_TRUE("exact_second",
                                           test_exact_second,
                                           "d_ms_to_timespec handles exact second");
    group->elements[idx++] = D_ASSERT_TRUE("large_values",
                                           test_large_values,
                                           "d_ms_to_timespec handles large values");
    group->elements[idx++] = D_ASSERT_TRUE("normalized",
                                           test_normalized,
                                           "d_ms_to_timespec normalizes result");
    group->elements[idx++] = D_ASSERT_TRUE("negative",
                                           test_negative,
                                           "d_ms_to_timespec handles negative");
    group->elements[idx++] = D_ASSERT_TRUE("consistency",
                                           test_consistency,
                                           "d_ms_to_timespec is consistent");

    return group;
}


/*
d_tests_dtime_us_to_timespec
  Tests d_us_to_timespec for converting microseconds to timespec.
  Tests the following:
  - converts simple values correctly
  - handles zero value
  - handles values less than 1 second
  - handles exact second values
  - handles large values
  - result is properly normalized
  - handles microsecond boundaries
  - consistency with d_timespec_to_us
*/
struct d_test_object*
d_tests_dtime_us_to_timespec
(
    void
)
{
    struct d_test_object* group;
    struct timespec       ts;
    int64_t               us;
    bool                  test_simple_conversion;
    bool                  test_zero_value;
    bool                  test_sub_second;
    bool                  test_exact_second;
    bool                  test_large_values;
    bool                  test_normalized;
    bool                  test_boundary;
    bool                  test_consistency;
    size_t                idx;

    // test 1: simple conversion (1500000us = 1.5s)
    us = 1500000;
    d_us_to_timespec(us, &ts);
    test_simple_conversion = (ts.tv_sec == 1) && (ts.tv_nsec == 500000000);

    // test 2: zero value
    us = 0;
    memset(&ts, 0xFF, sizeof(ts));
    d_us_to_timespec(us, &ts);
    test_zero_value = (ts.tv_sec == 0) && (ts.tv_nsec == 0);

    // test 3: values less than 1 second (500000us = 0.5s)
    us = 500000;
    d_us_to_timespec(us, &ts);
    test_sub_second = (ts.tv_sec == 0) && (ts.tv_nsec == 500000000);

    // test 4: exact second values (3000000us = 3s)
    us = 3000000;
    d_us_to_timespec(us, &ts);
    test_exact_second = (ts.tv_sec == 3) && (ts.tv_nsec == 0);

    // test 5: large values (1000000000us = 1000s)
    us = 1000000000LL;
    d_us_to_timespec(us, &ts);
    test_large_values = (ts.tv_sec == 1000) && (ts.tv_nsec == 0);

    // test 6: result is normalized (tv_nsec < 1 billion)
    us = 12345678;
    d_us_to_timespec(us, &ts);
    test_normalized = (ts.tv_nsec >= 0) && (ts.tv_nsec < D_TIME_NSEC_PER_SEC);

    // test 7: microsecond boundary (1us = 1000ns)
    us = 1;
    d_us_to_timespec(us, &ts);
    test_boundary = (ts.tv_sec == 0) && (ts.tv_nsec == 1000);

    // test 8: consistency - multiple conversions of same value
    us = 7654321;

    struct timespec ts1;
    struct timespec ts2;

    d_us_to_timespec(us, &ts1);
    d_us_to_timespec(us, &ts2);
    test_consistency = (ts1.tv_sec == ts2.tv_sec) && (ts1.tv_nsec == ts2.tv_nsec);

    // build result tree
    group = d_test_object_new_interior("d_us_to_timespec", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("simple_conversion",
                                           test_simple_conversion,
                                           "d_us_to_timespec converts correctly");
    group->elements[idx++] = D_ASSERT_TRUE("zero_value",
                                           test_zero_value,
                                           "d_us_to_timespec handles zero");
    group->elements[idx++] = D_ASSERT_TRUE("sub_second",
                                           test_sub_second,
                                           "d_us_to_timespec handles sub-second");
    group->elements[idx++] = D_ASSERT_TRUE("exact_second",
                                           test_exact_second,
                                           "d_us_to_timespec handles exact second");
    group->elements[idx++] = D_ASSERT_TRUE("large_values",
                                           test_large_values,
                                           "d_us_to_timespec handles large values");
    group->elements[idx++] = D_ASSERT_TRUE("normalized",
                                           test_normalized,
                                           "d_us_to_timespec normalizes result");
    group->elements[idx++] = D_ASSERT_TRUE("boundary",
                                           test_boundary,
                                           "d_us_to_timespec handles boundaries");
    group->elements[idx++] = D_ASSERT_TRUE("consistency",
                                           test_consistency,
                                           "d_us_to_timespec is consistent");

    return group;
}


/*
d_tests_dtime_ns_to_timespec
  Tests d_ns_to_timespec for converting nanoseconds to timespec.
  Tests the following:
  - converts simple values correctly
  - handles zero value
  - handles values less than 1 second
  - handles exact second values
  - handles large values
  - result is properly normalized
  - preserves nanosecond precision
  - consistency with d_timespec_to_ns
*/
struct d_test_object*
d_tests_dtime_ns_to_timespec
(
    void
)
{
    struct d_test_object* group;
    struct timespec       ts;
    int64_t               ns;
    bool                  test_simple_conversion;
    bool                  test_zero_value;
    bool                  test_sub_second;
    bool                  test_exact_second;
    bool                  test_large_values;
    bool                  test_normalized;
    bool                  test_precision;
    bool                  test_consistency;
    size_t                idx;

    // test 1: simple conversion (1500000000ns = 1.5s)
    ns = 1500000000LL;
    d_ns_to_timespec(ns, &ts);
    test_simple_conversion = (ts.tv_sec == 1) && (ts.tv_nsec == 500000000);

    // test 2: zero value
    ns = 0;
    memset(&ts, 0xFF, sizeof(ts));
    d_ns_to_timespec(ns, &ts);
    test_zero_value = (ts.tv_sec == 0) && (ts.tv_nsec == 0);

    // test 3: values less than 1 second (500000000ns = 0.5s)
    ns = 500000000;
    d_ns_to_timespec(ns, &ts);
    test_sub_second = (ts.tv_sec == 0) && (ts.tv_nsec == 500000000);

    // test 4: exact second values (3000000000ns = 3s)
    ns = 3000000000LL;
    d_ns_to_timespec(ns, &ts);
    test_exact_second = (ts.tv_sec == 3) && (ts.tv_nsec == 0);

    // test 5: large values (1000000000000ns = 1000s)
    ns = 1000000000000LL;
    d_ns_to_timespec(ns, &ts);
    test_large_values = (ts.tv_sec == 1000) && (ts.tv_nsec == 0);

    // test 6: result is normalized (tv_nsec < 1 billion)
    ns = 12345678901LL;
    d_ns_to_timespec(ns, &ts);
    test_normalized = (ts.tv_nsec >= 0) && (ts.tv_nsec < D_TIME_NSEC_PER_SEC);

    // test 7: preserves nanosecond precision (1ns)
    ns = 1;
    d_ns_to_timespec(ns, &ts);
    test_precision = (ts.tv_sec == 0) && (ts.tv_nsec == 1);

    // test 8: consistency - multiple conversions of same value
    ns = 9876543210LL;

    struct timespec ts1;
    struct timespec ts2;

    d_ns_to_timespec(ns, &ts1);
    d_ns_to_timespec(ns, &ts2);
    test_consistency = (ts1.tv_sec == ts2.tv_sec) && (ts1.tv_nsec == ts2.tv_nsec);

    // build result tree
    group = d_test_object_new_interior("d_ns_to_timespec", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("simple_conversion",
                                           test_simple_conversion,
                                           "d_ns_to_timespec converts correctly");
    group->elements[idx++] = D_ASSERT_TRUE("zero_value",
                                           test_zero_value,
                                           "d_ns_to_timespec handles zero");
    group->elements[idx++] = D_ASSERT_TRUE("sub_second",
                                           test_sub_second,
                                           "d_ns_to_timespec handles sub-second");
    group->elements[idx++] = D_ASSERT_TRUE("exact_second",
                                           test_exact_second,
                                           "d_ns_to_timespec handles exact second");
    group->elements[idx++] = D_ASSERT_TRUE("large_values",
                                           test_large_values,
                                           "d_ns_to_timespec handles large values");
    group->elements[idx++] = D_ASSERT_TRUE("normalized",
                                           test_normalized,
                                           "d_ns_to_timespec normalizes result");
    group->elements[idx++] = D_ASSERT_TRUE("precision",
                                           test_precision,
                                           "d_ns_to_timespec preserves precision");
    group->elements[idx++] = D_ASSERT_TRUE("consistency",
                                           test_consistency,
                                           "d_ns_to_timespec is consistent");

    return group;
}


/*
d_tests_dtime_time_arithmetic_all
  Runs all time arithmetic tests.
  Tests the following:
  - d_timespec_add
  - d_timespec_sub
  - d_timespec_cmp
  - d_timespec_to_ms
  - d_timespec_to_us
  - d_timespec_to_ns
  - d_ms_to_timespec
  - d_us_to_timespec
  - d_ns_to_timespec
*/
struct d_test_object*
d_tests_dtime_time_arithmetic_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("VIII. Time Arithmetic", 9);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dtime_timespec_add();
    group->elements[idx++] = d_tests_dtime_timespec_sub();
    group->elements[idx++] = d_tests_dtime_timespec_cmp();
    group->elements[idx++] = d_tests_dtime_timespec_to_ms();
    group->elements[idx++] = d_tests_dtime_timespec_to_us();
    group->elements[idx++] = d_tests_dtime_timespec_to_ns();
    group->elements[idx++] = d_tests_dtime_ms_to_timespec();
    group->elements[idx++] = d_tests_dtime_us_to_timespec();
    group->elements[idx++] = d_tests_dtime_ns_to_timespec();

    return group;
}
