/*******************************************************************************
* djinterp [c][sync]                                        datomic_tests_load.c
*
*
* path:      /tests/djinterp/c/sync/datomic_tests_load.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.15
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "datomic_tests.h"


/*
d_tests_datomic_load_int
  Tests atomic load for int type.
  Tests the following:
  - d_atomic_load_int returns the initialized value
  - d_atomic_load_int returns updated value after store
*/
bool d_tests_datomic_load_int(
    struct dj_test_suite* _suite
)
{
    bool         result;
    dj_atomic_int val;

    d_test_suite_test(_suite, "datomic_load_int");

    result = true;

    d_atomic_init_int(&val, 100);

    result = D_TEST_CHECK_MSG(_suite,
                              "load_int",
                              d_atomic_load_int(&val) == 100,
                              "Load int should return 100") && result;

    d_atomic_store_int(&val, -50);

    result = D_TEST_CHECK_MSG(_suite,
                              "load_int_after_store",
                              d_atomic_load_int(&val) == -50,
                              "Load int should return -50 after store") && result;

    return result;
}

/*
d_tests_datomic_load_uint
  Tests atomic load for unsigned int type.
  Tests the following:
  - d_atomic_load_uint returns the initialized value
*/
bool d_tests_datomic_load_uint(
    struct dj_test_suite* _suite
)
{
    bool          result;
    dj_atomic_uint val;

    d_test_suite_test(_suite, "datomic_load_uint");

    result = true;

    d_atomic_init_uint(&val, 200);

    result = D_TEST_CHECK_MSG(_suite,
                              "load_uint",
                              d_atomic_load_uint(&val) == 200,
                              "Load uint should return 200") && result;

    return result;
}

/*
d_tests_datomic_load_long
  Tests atomic load for long type.
  Tests the following:
  - d_atomic_load_long returns the initialized value
*/
bool d_tests_datomic_load_long(
    struct dj_test_suite* _suite
)
{
    bool          result;
    dj_atomic_long val;

    d_test_suite_test(_suite, "datomic_load_long");

    result = true;

    d_atomic_init_long(&val, 300L);

    result = D_TEST_CHECK_MSG(_suite,
                              "load_long",
                              d_atomic_load_long(&val) == 300L,
                              "Load long should return 300") && result;

    return result;
}

/*
d_tests_datomic_load_ulong
  Tests atomic load for unsigned long type.
  Tests the following:
  - d_atomic_load_ulong returns the initialized value
*/
bool d_tests_datomic_load_ulong(
    struct dj_test_suite* _suite
)
{
    bool           result;
    dj_atomic_ulong val;

    d_test_suite_test(_suite, "datomic_load_ulong");

    result = true;

    d_atomic_init_ulong(&val, 400UL);

    result = D_TEST_CHECK_MSG(_suite,
                              "load_ulong",
                              d_atomic_load_ulong(&val) == 400UL,
                              "Load ulong should return 400") && result;

    return result;
}

/*
d_tests_datomic_load_llong
  Tests atomic load for long long type.
  Tests the following:
  - d_atomic_load_llong returns the initialized value
*/
bool d_tests_datomic_load_llong(
    struct dj_test_suite* _suite
)
{
    bool           result;
    dj_atomic_llong val;

    d_test_suite_test(_suite, "datomic_load_llong");

    result = true;

    d_atomic_init_llong(&val, 500LL);

    result = D_TEST_CHECK_MSG(_suite,
                              "load_llong",
                              d_atomic_load_llong(&val) == 500LL,
                              "Load llong should return 500") && result;

    return result;
}

/*
d_tests_datomic_load_ullong
  Tests atomic load for unsigned long long type.
  Tests the following:
  - d_atomic_load_ullong returns the initialized value
*/
bool d_tests_datomic_load_ullong(
    struct dj_test_suite* _suite
)
{
    bool            result;
    dj_atomic_ullong val;

    d_test_suite_test(_suite, "datomic_load_ullong");

    result = true;

    d_atomic_init_ullong(&val, 600ULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "load_ullong",
                              d_atomic_load_ullong(&val) == 600ULL,
                              "Load ullong should return 600") && result;

    return result;
}

/*
d_tests_datomic_load_ptr
  Tests atomic load for pointer type.
  Tests the following:
  - d_atomic_load_ptr returns the initialized pointer
*/
bool d_tests_datomic_load_ptr(
    struct dj_test_suite* _suite
)
{
    bool         result;
    dj_atomic_ptr val;
    int          dummy;

    d_test_suite_test(_suite, "datomic_load_ptr");

    result = true;
    dummy  = 42;

    d_atomic_init_ptr(&val, &dummy);

    result = D_TEST_CHECK_MSG(_suite,
                              "load_ptr",
                              d_atomic_load_ptr(&val) == &dummy,
                              "Load ptr should return pointer to dummy") && result;

    return result;
}

/*
d_tests_datomic_load_size
  Tests atomic load for size_t type.
  Tests the following:
  - d_atomic_load_size returns the initialized value
*/
bool d_tests_datomic_load_size(
    struct dj_test_suite* _suite
)
{
    bool            result;
    dj_atomic_size_t val;

    d_test_suite_test(_suite, "datomic_load_size");

    result = true;

    d_atomic_init_size(&val, 700);

    result = D_TEST_CHECK_MSG(_suite,
                              "load_size",
                              d_atomic_load_size(&val) == 700,
                              "Load size should return 700") && result;

    return result;
}

/*
d_tests_datomic_load_explicit
  Tests atomic load with explicit memory orders.
  Tests the following:
  - d_atomic_load_int_explicit with relaxed ordering
  - d_atomic_load_int_explicit with acquire ordering
  - d_atomic_load_int_explicit with seq_cst ordering
*/
bool d_tests_datomic_load_explicit(
    struct dj_test_suite* _suite
)
{
    bool         result;
    dj_atomic_int val;

    d_test_suite_test(_suite, "datomic_load_explicit");

    result = true;

    d_atomic_init_int(&val, 123);

    result = D_TEST_CHECK_MSG(_suite,
                              "load_explicit_relaxed",
                              d_atomic_load_int_explicit(&val, D_MEMORY_ORDER_RELAXED) == 123,
                              "Load with relaxed order should work") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "load_explicit_acquire",
                              d_atomic_load_int_explicit(&val, D_MEMORY_ORDER_ACQUIRE) == 123,
                              "Load with acquire order should work") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "load_explicit_seq_cst",
                              d_atomic_load_int_explicit(&val, D_MEMORY_ORDER_SEQ_CST) == 123,
                              "Load with seq_cst order should work") && result;

    return result;
}

/*
d_tests_datomic_load_all
  Runs all atomic load tests.
  Tests the following:
  - all types: int, uint, long, ulong, llong, ullong, ptr, size
  - explicit memory order variants
*/
bool d_tests_datomic_load_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "datomic_load_all");

    result = true;

    result = d_tests_datomic_load_int(_suite) && result;
    result = d_tests_datomic_load_uint(_suite) && result;
    result = d_tests_datomic_load_long(_suite) && result;
    result = d_tests_datomic_load_ulong(_suite) && result;
    result = d_tests_datomic_load_llong(_suite) && result;
    result = d_tests_datomic_load_ullong(_suite) && result;
    result = d_tests_datomic_load_ptr(_suite) && result;
    result = d_tests_datomic_load_size(_suite) && result;
    result = d_tests_datomic_load_explicit(_suite) && result;

    return result;
}
