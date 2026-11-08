/*******************************************************************************
* djinterp [test]                                       datomic_tests_exchange.c
*
*
* path:      /tests/djinterp/c/sync/datomic_tests_exchange.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.15
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "datomic_tests.h"


/*
d_tests_datomic_exchange_int
  Tests atomic exchange for int type.
  Tests the following:
  - d_atomic_exchange_int returns the old value
  - the atomic holds the new value after exchange
*/
bool d_tests_datomic_exchange_int(
    struct dj_test_suite* _suite
)
{
    bool         result;
    dj_atomic_int val;
    int          old;

    d_test_suite_test(_suite, "datomic_exchange_int");

    result = true;

    d_atomic_init_int(&val, 100);
    old = d_atomic_exchange_int(&val, 200);

    result = D_TEST_CHECK_MSG(_suite,
                              "exchange_int_old",
                              old == 100,
                              "Exchange should return old value 100") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "exchange_int_new",
                              d_atomic_load_int(&val) == 200,
                              "Exchange should set new value 200") && result;

    return result;
}

/*
d_tests_datomic_exchange_uint
  Tests atomic exchange for unsigned int type.
  Tests the following:
  - d_atomic_exchange_uint returns old and sets new value
*/
bool d_tests_datomic_exchange_uint(
    struct dj_test_suite* _suite
)
{
    bool          result;
    dj_atomic_uint val;
    unsigned int  old;

    d_test_suite_test(_suite, "datomic_exchange_uint");

    result = true;

    d_atomic_init_uint(&val, 50);
    old = d_atomic_exchange_uint(&val, 150);

    result = D_TEST_CHECK_MSG(_suite,
                              "exchange_uint",
                              old == 50 && d_atomic_load_uint(&val) == 150,
                              "Exchange uint should work correctly") && result;

    return result;
}

/*
d_tests_datomic_exchange_long
  Tests atomic exchange for long type.
  Tests the following:
  - d_atomic_exchange_long handles negative values correctly
*/
bool d_tests_datomic_exchange_long(
    struct dj_test_suite* _suite
)
{
    bool          result;
    dj_atomic_long val;
    long          old;

    d_test_suite_test(_suite, "datomic_exchange_long");

    result = true;

    d_atomic_init_long(&val, -50L);
    old = d_atomic_exchange_long(&val, 75L);

    result = D_TEST_CHECK_MSG(_suite,
                              "exchange_long",
                              old == -50L && d_atomic_load_long(&val) == 75L,
                              "Exchange long should work correctly") && result;

    return result;
}

/*
d_tests_datomic_exchange_ulong
  Tests atomic exchange for unsigned long type.
  Tests the following:
  - d_atomic_exchange_ulong returns old and sets new value
*/
bool d_tests_datomic_exchange_ulong(
    struct dj_test_suite* _suite
)
{
    bool           result;
    dj_atomic_ulong val;
    unsigned long  old;

    d_test_suite_test(_suite, "datomic_exchange_ulong");

    result = true;

    d_atomic_init_ulong(&val, 100UL);
    old = d_atomic_exchange_ulong(&val, 300UL);

    result = D_TEST_CHECK_MSG(_suite,
                              "exchange_ulong",
                              old == 100UL && d_atomic_load_ulong(&val) == 300UL,
                              "Exchange ulong should work correctly") && result;

    return result;
}

/*
d_tests_datomic_exchange_llong
  Tests atomic exchange for long long type.
  Tests the following:
  - d_atomic_exchange_llong returns old and sets new value
*/
bool d_tests_datomic_exchange_llong(
    struct dj_test_suite* _suite
)
{
    bool           result;
    dj_atomic_llong val;
    d_test_suite_test(_suite, "datomic_exchange_llong");

    long long      old;

    result = true;

    d_atomic_init_llong(&val, 1000LL);
    old = d_atomic_exchange_llong(&val, 2000LL);

    result = D_TEST_CHECK_MSG(_suite,
                              "exchange_llong",
                              old == 1000LL && d_atomic_load_llong(&val) == 2000LL,
                              "Exchange llong should work correctly") && result;

    return result;
}

/*
d_tests_datomic_exchange_ullong
  Tests atomic exchange for unsigned long long type.
  Tests the following:
  - d_atomic_exchange_ullong returns old and sets new value
*/
bool d_tests_datomic_exchange_ullong(
    struct dj_test_suite* _suite
)
{
    bool               result;
    dj_atomic_ullong     val;
    d_test_suite_test(_suite, "datomic_exchange_ullong");

    unsigned long long old;

    result = true;

    d_atomic_init_ullong(&val, 5000ULL);
    old = d_atomic_exchange_ullong(&val, 6000ULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "exchange_ullong",
                              old == 5000ULL && d_atomic_load_ullong(&val) == 6000ULL,
                              "Exchange ullong should work correctly") && result;

    return result;
}

/*
d_tests_datomic_exchange_ptr
  Tests atomic exchange for pointer type.
  Tests the following:
  - d_atomic_exchange_ptr returns old pointer and sets new one
*/
bool d_tests_datomic_exchange_ptr(
    struct dj_test_suite* _suite
)
{
    bool         result;
    dj_atomic_ptr val;
    int          dummy1;
    int          dummy2;
    void*        old;

    d_test_suite_test(_suite, "datomic_exchange_ptr");

    result = true;
    dummy1 = 1;
    dummy2 = 2;

    d_atomic_init_ptr(&val, &dummy1);
    old = d_atomic_exchange_ptr(&val, &dummy2);

    result = D_TEST_CHECK_MSG(_suite,
                              "exchange_ptr",
                              old == &dummy1 && d_atomic_load_ptr(&val) == &dummy2,
                              "Exchange ptr should work correctly") && result;

    return result;
}

/*
d_tests_datomic_exchange_size
  Tests atomic exchange for size_t type.
  Tests the following:
  - d_atomic_exchange_size returns old and sets new value
*/
bool d_tests_datomic_exchange_size(
    struct dj_test_suite* _suite
)
{
    bool            result;
    dj_atomic_size_t val;
    size_t          old;

    d_test_suite_test(_suite, "datomic_exchange_size");

    result = true;

    d_atomic_init_size(&val, 111);
    old = d_atomic_exchange_size(&val, 222);

    result = D_TEST_CHECK_MSG(_suite,
                              "exchange_size",
                              old == 111 && d_atomic_load_size(&val) == 222,
                              "Exchange size should work correctly") && result;

    return result;
}

/*
d_tests_datomic_exchange_explicit
  Tests atomic exchange with explicit memory orders.
  Tests the following:
  - d_atomic_exchange_int_explicit with acquire ordering
  - d_atomic_exchange_int_explicit with release ordering
  - d_atomic_exchange_int_explicit with seq_cst ordering
  - d_atomic_exchange_ptr_explicit with seq_cst ordering
*/
bool d_tests_datomic_exchange_explicit(
    struct dj_test_suite* _suite
)
{
    bool         result;
    dj_atomic_int val;
    dj_atomic_ptr pval;
    int          old;
    int          dummy1;
    int          dummy2;
    void*        old_ptr;

    d_test_suite_test(_suite, "datomic_exchange_explicit");

    result = true;
    dummy1 = 1;
    dummy2 = 2;

    d_atomic_init_int(&val, 10);

    old = d_atomic_exchange_int_explicit(&val, 20,
                                          D_MEMORY_ORDER_ACQUIRE);

    result = D_TEST_CHECK_MSG(_suite,
                              "exchange_explicit_acquire",
                              old == 10 && d_atomic_load_int(&val) == 20,
                              "Exchange with acquire order should work") && result;

    old = d_atomic_exchange_int_explicit(&val, 30,
                                          D_MEMORY_ORDER_RELEASE);

    result = D_TEST_CHECK_MSG(_suite,
                              "exchange_explicit_release",
                              old == 20 && d_atomic_load_int(&val) == 30,
                              "Exchange with release order should work") && result;

    old = d_atomic_exchange_int_explicit(&val, 40,
                                          D_MEMORY_ORDER_SEQ_CST);

    result = D_TEST_CHECK_MSG(_suite,
                              "exchange_explicit_seq_cst",
                              old == 30 && d_atomic_load_int(&val) == 40,
                              "Exchange with seq_cst order should work") && result;

    // test ptr explicit variant
    d_atomic_init_ptr(&pval, &dummy1);

    old_ptr = d_atomic_exchange_ptr_explicit(&pval, &dummy2,
                                              D_MEMORY_ORDER_SEQ_CST);

    result = D_TEST_CHECK_MSG(_suite,
                              "exchange_ptr_explicit_seq_cst",
                              old_ptr == &dummy1 && d_atomic_load_ptr(&pval) == &dummy2,
                              "Exchange ptr with seq_cst should work") && result;

    return result;
}

/*
d_tests_datomic_exchange_all
  Runs all atomic exchange tests.
  Tests the following:
  - all types: int, uint, long, ulong, llong, ullong, ptr, size
  - explicit memory order variants
*/
bool d_tests_datomic_exchange_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "datomic_exchange_all");

    result = true;

    result = d_tests_datomic_exchange_int(_suite) && result;
    result = d_tests_datomic_exchange_uint(_suite) && result;
    result = d_tests_datomic_exchange_long(_suite) && result;
    result = d_tests_datomic_exchange_ulong(_suite) && result;
    result = d_tests_datomic_exchange_llong(_suite) && result;
    result = d_tests_datomic_exchange_ullong(_suite) && result;
    result = d_tests_datomic_exchange_ptr(_suite) && result;
    result = d_tests_datomic_exchange_size(_suite) && result;
    result = d_tests_datomic_exchange_explicit(_suite) && result;

    return result;
}
