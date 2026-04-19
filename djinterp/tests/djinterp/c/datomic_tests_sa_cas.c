#include ".\datomic_tests_sa.h"


/*
d_tests_sa_atomic_compare_exchange_strong_int
  Tests strong CAS for int type.
  Tests the following:
  - CAS succeeds when expected matches current value
  - CAS stores the desired value on success
  - CAS fails when expected does not match
  - expected is updated to current value on failure
*/
bool
d_tests_sa_atomic_compare_exchange_strong_int
(
    struct d_test_counter* _counter
)
{
    bool         result;
    d_atomic_int val;
    int          expected;

    result = true;

    d_atomic_init_int(&val, 100);
    expected = 100;

    result = d_assert_standalone(
        d_atomic_compare_exchange_strong_int(&val,
                                              &expected,
                                              200),
        "cas_strong_int_success",
        "CAS should succeed when expected matches",
        _counter) && result;

    result = d_assert_standalone(
        d_atomic_load_int(&val) == 200,
        "cas_strong_int_new_value",
        "CAS should set new value to 200",
        _counter) && result;

    expected = 100;

    result = d_assert_standalone(
        !d_atomic_compare_exchange_strong_int(&val,
                                               &expected,
                                               300),
        "cas_strong_int_fail",
        "CAS should fail when expected doesn't match",
        _counter) && result;

    result = d_assert_standalone(
        expected == 200,
        "cas_strong_int_expected_updated",
        "Expected should be updated to current value on failure",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_compare_exchange_weak_int
  Tests weak CAS for int type.
  Tests the following:
  - weak CAS eventually succeeds (may spuriously fail)
  - value is correct after successful weak CAS
*/
bool
d_tests_sa_atomic_compare_exchange_weak_int
(
    struct d_test_counter* _counter
)
{
    bool         result;
    d_atomic_int val;
    int          expected;
    bool         cas_result;
    int          attempts;

    result   = true;
    attempts = 0;

    d_atomic_init_int(&val, 50);

    // weak CAS may spuriously fail, so retry
    do
    {
        expected   = 50;
        cas_result = d_atomic_compare_exchange_weak_int(&val,
                                                         &expected,
                                                         75);
        attempts++;
    }
    while (!cas_result && attempts < 100);

    result = d_assert_standalone(
        cas_result && d_atomic_load_int(&val) == 75,
        "cas_weak_int_eventually_succeeds",
        "CAS weak should eventually succeed",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_compare_exchange_strong_uint
  Tests strong CAS for unsigned int type.
  Tests the following:
  - CAS succeeds for unsigned int
*/
bool
d_tests_sa_atomic_compare_exchange_strong_uint
(
    struct d_test_counter* _counter
)
{
    bool          result;
    d_atomic_uint val;
    unsigned int  expected;

    result = true;

    d_atomic_init_uint(&val, 10);
    expected = 10;

    result = d_assert_standalone(
        d_atomic_compare_exchange_strong_uint(&val,
                                               &expected,
                                               20),
        "cas_strong_uint",
        "CAS strong uint should work",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_compare_exchange_strong_long
  Tests strong CAS for long type.
  Tests the following:
  - CAS succeeds for long
*/
bool
d_tests_sa_atomic_compare_exchange_strong_long
(
    struct d_test_counter* _counter
)
{
    bool          result;
    d_atomic_long val;
    long          expected;

    result = true;

    d_atomic_init_long(&val, 30L);
    expected = 30L;

    result = d_assert_standalone(
        d_atomic_compare_exchange_strong_long(&val,
                                               &expected,
                                               40L),
        "cas_strong_long",
        "CAS strong long should work",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_compare_exchange_strong_llong
  Tests strong CAS for long long type.
  Tests the following:
  - CAS succeeds for long long
*/
bool
d_tests_sa_atomic_compare_exchange_strong_llong
(
    struct d_test_counter* _counter
)
{
    bool           result;
    d_atomic_llong val;
    long long      expected;

    result = true;

    d_atomic_init_llong(&val, 1234LL);
    expected = 1234LL;

    result = d_assert_standalone(
        d_atomic_compare_exchange_strong_llong(&val,
                                                &expected,
                                                5678LL),
        "cas_strong_llong",
        "CAS strong llong should work",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_compare_exchange_strong_ptr
  Tests strong CAS for pointer type.
  Tests the following:
  - CAS succeeds for pointer
*/
bool
d_tests_sa_atomic_compare_exchange_strong_ptr
(
    struct d_test_counter* _counter
)
{
    bool         result;
    d_atomic_ptr val;
    int          dummy1;
    int          dummy2;
    void*        expected;

    result = true;
    dummy1 = 1;
    dummy2 = 2;

    d_atomic_init_ptr(&val, &dummy1);
    expected = &dummy1;

    result = d_assert_standalone(
        d_atomic_compare_exchange_strong_ptr(&val,
                                              &expected,
                                              &dummy2),
        "cas_strong_ptr",
        "CAS strong ptr should work",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_compare_exchange_strong_size
  Tests strong CAS for size_t type.
  Tests the following:
  - CAS succeeds for size_t
*/
bool
d_tests_sa_atomic_compare_exchange_strong_size
(
    struct d_test_counter* _counter
)
{
    bool            result;
    d_atomic_size_t val;
    size_t          expected;

    result = true;

    d_atomic_init_size(&val, 999);
    expected = 999;

    result = d_assert_standalone(
        d_atomic_compare_exchange_strong_size(&val,
                                               &expected,
                                               888),
        "cas_strong_size",
        "CAS strong size should work",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_compare_exchange_explicit
  Tests CAS with explicit memory orders.
  Tests the following:
  - strong CAS with acq_rel/relaxed ordering succeeds
  - strong CAS with acq_rel/relaxed ordering fails and updates expected
  - weak CAS with explicit ordering eventually succeeds
  - ptr CAS with explicit ordering works
*/
bool
d_tests_sa_atomic_compare_exchange_explicit
(
    struct d_test_counter* _counter
)
{
    bool         result;
    d_atomic_int val;
    d_atomic_ptr pval;
    int          expected;
    int          dummy1;
    int          dummy2;
    void*        pexp;
    bool         cas_result;
    int          attempts;

    result = true;
    dummy1 = 1;
    dummy2 = 2;

    // strong explicit — success
    d_atomic_init_int(&val, 100);
    expected = 100;

    result = d_assert_standalone(
        d_atomic_compare_exchange_strong_int_explicit(
            &val, &expected, 200,
            D_MEMORY_ORDER_ACQ_REL,
            D_MEMORY_ORDER_RELAXED),
        "cas_strong_explicit_success",
        "CAS strong explicit should succeed",
        _counter) && result;

    result = d_assert_standalone(
        d_atomic_load_int(&val) == 200,
        "cas_strong_explicit_value",
        "CAS strong explicit should set new value",
        _counter) && result;

    // strong explicit — failure
    expected = 999;

    result = d_assert_standalone(
        !d_atomic_compare_exchange_strong_int_explicit(
            &val, &expected, 300,
            D_MEMORY_ORDER_ACQ_REL,
            D_MEMORY_ORDER_RELAXED),
        "cas_strong_explicit_fail",
        "CAS strong explicit should fail on mismatch",
        _counter) && result;

    result = d_assert_standalone(
        expected == 200,
        "cas_strong_explicit_expected_updated",
        "Expected should be updated on explicit CAS failure",
        _counter) && result;

    // weak explicit
    d_atomic_store_int(&val, 50);
    attempts = 0;

    do
    {
        expected   = 50;
        cas_result = d_atomic_compare_exchange_weak_int_explicit(
                         &val, &expected, 75,
                         D_MEMORY_ORDER_SEQ_CST,
                         D_MEMORY_ORDER_SEQ_CST);
        attempts++;
    }
    while (!cas_result && attempts < 100);

    result = d_assert_standalone(
        cas_result,
        "cas_weak_explicit_success",
        "CAS weak explicit should eventually succeed",
        _counter) && result;

    // ptr explicit
    d_atomic_init_ptr(&pval, &dummy1);
    pexp = &dummy1;

    result = d_assert_standalone(
        d_atomic_compare_exchange_strong_ptr_explicit(
            &pval, &pexp, &dummy2,
            D_MEMORY_ORDER_SEQ_CST,
            D_MEMORY_ORDER_SEQ_CST),
        "cas_ptr_explicit",
        "CAS ptr explicit should work",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_compare_exchange_all
  Runs all atomic compare-and-exchange tests.
  Tests the following:
  - strong CAS for int, uint, long, llong, ptr, size
  - weak CAS for int
  - explicit memory order variants
*/
bool
d_tests_sa_atomic_compare_exchange_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Atomic Compare-and-Exchange\n");
    printf("  --------------------------------------\n");

    result = d_tests_sa_atomic_compare_exchange_strong_int(_counter)
             && result;
    result = d_tests_sa_atomic_compare_exchange_weak_int(_counter)
             && result;
    result = d_tests_sa_atomic_compare_exchange_strong_uint(_counter)
             && result;
    result = d_tests_sa_atomic_compare_exchange_strong_long(_counter)
             && result;
    result = d_tests_sa_atomic_compare_exchange_strong_llong(_counter)
             && result;
    result = d_tests_sa_atomic_compare_exchange_strong_ptr(_counter)
             && result;
    result = d_tests_sa_atomic_compare_exchange_strong_size(_counter)
             && result;
    result = d_tests_sa_atomic_compare_exchange_explicit(_counter)
             && result;

    return result;
}