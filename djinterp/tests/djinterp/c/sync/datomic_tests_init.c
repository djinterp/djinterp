/*******************************************************************************
* djinterp [c][sync]                                        datomic_tests_init.c
*
*
* path:      /tests/djinterp/c/sync/datomic_tests_init.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.15
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "datomic_tests.h"


/*
d_tests_datomic_init_int
  Tests atomic initialization for int type.
  Tests the following:
  - d_atomic_init_int sets the correct value
  - d_atomic_load_int reads the initialized value
*/
bool d_tests_datomic_init_int(
    struct dj_test_suite* _suite
)
{
    bool         result;
    dj_atomic_int val;

    d_test_suite_test(_suite, "datomic_init_int");

    result = true;

    d_atomic_init_int(&val, 42);

    result = D_TEST_CHECK_MSG(_suite,
                              "init_int",
                              d_atomic_load_int(&val) == 42,
                              "Init int should set value to 42") && result;

    return result;
}

/*
d_tests_datomic_init_uint
  Tests atomic initialization for unsigned int type.
  Tests the following:
  - d_atomic_init_uint sets the correct value
*/
bool d_tests_datomic_init_uint(
    struct dj_test_suite* _suite
)
{
    bool          result;
    dj_atomic_uint val;

    d_test_suite_test(_suite, "datomic_init_uint");

    result = true;

    d_atomic_init_uint(&val, 99);

    result = D_TEST_CHECK_MSG(_suite,
                              "init_uint",
                              d_atomic_load_uint(&val) == 99,
                              "Init uint should set value to 99") && result;

    return result;
}

/*
d_tests_datomic_init_long
  Tests atomic initialization for long type.
  Tests the following:
  - d_atomic_init_long sets a negative value correctly
*/
bool d_tests_datomic_init_long(
    struct dj_test_suite* _suite
)
{
    bool          result;
    dj_atomic_long val;

    d_test_suite_test(_suite, "datomic_init_long");

    result = true;

    d_atomic_init_long(&val, -123);

    result = D_TEST_CHECK_MSG(_suite,
                              "init_long",
                              d_atomic_load_long(&val) == -123,
                              "Init long should set value to -123") && result;

    return result;
}

/*
d_tests_datomic_init_ulong
  Tests atomic initialization for unsigned long type.
  Tests the following:
  - d_atomic_init_ulong sets the correct value
*/
bool d_tests_datomic_init_ulong(
    struct dj_test_suite* _suite
)
{
    bool           result;
    dj_atomic_ulong val;

    d_test_suite_test(_suite, "datomic_init_ulong");

    result = true;

    d_atomic_init_ulong(&val, 456UL);

    result = D_TEST_CHECK_MSG(_suite,
                              "init_ulong",
                              d_atomic_load_ulong(&val) == 456UL,
                              "Init ulong should set value to 456") && result;

    return result;
}

/*
d_tests_datomic_init_llong
  Tests atomic initialization for long long type.
  Tests the following:
  - d_atomic_init_llong sets a large negative value correctly
*/
bool d_tests_datomic_init_llong(
    struct dj_test_suite* _suite
)
{
    bool           result;
    dj_atomic_llong val;

    d_test_suite_test(_suite, "datomic_init_llong");

    result = true;

    d_atomic_init_llong(&val, -9876543210LL);

    result = D_TEST_CHECK_MSG(_suite,
                              "init_llong",
                              d_atomic_load_llong(&val) == -9876543210LL,
                              "Init llong should set value to -9876543210") && result;

    return result;
}

/*
d_tests_datomic_init_ullong
  Tests atomic initialization for unsigned long long type.
  Tests the following:
  - d_atomic_init_ullong sets a large value correctly
*/
bool d_tests_datomic_init_ullong(
    struct dj_test_suite* _suite
)
{
    bool            result;
    dj_atomic_ullong val;

    d_test_suite_test(_suite, "datomic_init_ullong");

    result = true;

    d_atomic_init_ullong(&val, 9876543210ULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "init_ullong",
                              d_atomic_load_ullong(&val) == 9876543210ULL,
                              "Init ullong should set value to 9876543210") && result;

    return result;
}

/*
d_tests_datomic_init_ptr
  Tests atomic initialization for pointer type.
  Tests the following:
  - d_atomic_init_ptr sets a non-NULL pointer
  - d_atomic_init_ptr handles NULL correctly
*/
bool d_tests_datomic_init_ptr(
    struct dj_test_suite* _suite
)
{
    bool         result;
    dj_atomic_ptr val;
    int          dummy;

    d_test_suite_test(_suite, "datomic_init_ptr");

    result = true;
    dummy  = 0;

    d_atomic_init_ptr(&val, &dummy);

    result = D_TEST_CHECK_MSG(_suite,
                              "init_ptr",
                              d_atomic_load_ptr(&val) == &dummy,
                              "Init ptr should set pointer value") && result;

    d_atomic_init_ptr(&val, NULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "init_ptr_null",
                              d_atomic_load_ptr(&val) == NULL,
                              "Init ptr should handle NULL") && result;

    return result;
}

/*
d_tests_datomic_init_size
  Tests atomic initialization for size_t type.
  Tests the following:
  - d_atomic_init_size sets the correct value
*/
bool d_tests_datomic_init_size(
    struct dj_test_suite* _suite
)
{
    bool            result;
    dj_atomic_size_t val;

    d_test_suite_test(_suite, "datomic_init_size");

    result = true;

    d_atomic_init_size(&val, 12345);

    result = D_TEST_CHECK_MSG(_suite,
                              "init_size",
                              d_atomic_load_size(&val) == 12345,
                              "Init size should set value to 12345") && result;

    return result;
}

/*
d_tests_datomic_init_all
  Runs all atomic initialization tests.
  Tests the following:
  - int, uint, long, ulong, llong, ullong, ptr, size initialization
*/
bool d_tests_datomic_init_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "datomic_init_all");

    result = true;

    result = d_tests_datomic_init_int(_suite) && result;
    result = d_tests_datomic_init_uint(_suite) && result;
    result = d_tests_datomic_init_long(_suite) && result;
    result = d_tests_datomic_init_ulong(_suite) && result;
    result = d_tests_datomic_init_llong(_suite) && result;
    result = d_tests_datomic_init_ullong(_suite) && result;
    result = d_tests_datomic_init_ptr(_suite) && result;
    result = d_tests_datomic_init_size(_suite) && result;

    return result;
}
