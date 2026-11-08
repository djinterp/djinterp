/*******************************************************************************
* djinterp [test]                                          datomic_tests_store.c
*
*
* path:      /tests/djinterp/c/sync/datomic_tests_store.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.15
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "datomic_tests.h"


/*
d_tests_datomic_store_int
  Tests atomic store for int type.
  Tests the following:
  - d_atomic_store_int sets the correct value
*/
bool d_tests_datomic_store_int(
    struct dj_test_suite* _suite
)
{
    bool         result;
    dj_atomic_int val;

    d_test_suite_test(_suite, "datomic_store_int");

    result = true;

    d_atomic_init_int(&val, 0);
    d_atomic_store_int(&val, 999);

    result = D_TEST_CHECK_MSG(_suite,
                              "store_int",
                              d_atomic_load_int(&val) == 999,
                              "Store int should set value to 999") && result;

    return result;
}

/*
d_tests_datomic_store_uint
  Tests atomic store for unsigned int type.
  Tests the following:
  - d_atomic_store_uint sets the correct value
*/
bool d_tests_datomic_store_uint(
    struct dj_test_suite* _suite
)
{
    bool          result;
    dj_atomic_uint val;

    d_test_suite_test(_suite, "datomic_store_uint");

    result = true;

    d_atomic_init_uint(&val, 0);
    d_atomic_store_uint(&val, 888);

    result = D_TEST_CHECK_MSG(_suite,
                              "store_uint",
                              d_atomic_load_uint(&val) == 888,
                              "Store uint should set value to 888") && result;

    return result;
}

/*
d_tests_datomic_store_long
  Tests atomic store for long type.
  Tests the following:
  - d_atomic_store_long sets the correct value
*/
bool d_tests_datomic_store_long(
    struct dj_test_suite* _suite
)
{
    bool          result;
    dj_atomic_long val;

    d_test_suite_test(_suite, "datomic_store_long");

    result = true;

    d_atomic_init_long(&val, 0);
    d_atomic_store_long(&val, 777L);

    result = D_TEST_CHECK_MSG(_suite,
                              "store_long",
                              d_atomic_load_long(&val) == 777L,
                              "Store long should set value to 777") && result;

    return result;
}

/*
d_tests_datomic_store_ulong
  Tests atomic store for unsigned long type.
  Tests the following:
  - d_atomic_store_ulong sets the correct value
*/
bool d_tests_datomic_store_ulong(
    struct dj_test_suite* _suite
)
{
    bool           result;
    dj_atomic_ulong val;

    d_test_suite_test(_suite, "datomic_store_ulong");

    result = true;

    d_atomic_init_ulong(&val, 0);
    d_atomic_store_ulong(&val, 666UL);

    result = D_TEST_CHECK_MSG(_suite,
                              "store_ulong",
                              d_atomic_load_ulong(&val) == 666UL,
                              "Store ulong should set value to 666") && result;

    return result;
}

/*
d_tests_datomic_store_llong
  Tests atomic store for long long type.
  Tests the following:
  - d_atomic_store_llong sets the correct value
*/
bool d_tests_datomic_store_llong(
    struct dj_test_suite* _suite
)
{
    bool           result;
    dj_atomic_llong val;

    d_test_suite_test(_suite, "datomic_store_llong");

    result = true;

    d_atomic_init_llong(&val, 0);
    d_atomic_store_llong(&val, 555LL);

    result = D_TEST_CHECK_MSG(_suite,
                              "store_llong",
                              d_atomic_load_llong(&val) == 555LL,
                              "Store llong should set value to 555") && result;

    return result;
}

/*
d_tests_datomic_store_ullong
  Tests atomic store for unsigned long long type.
  Tests the following:
  - d_atomic_store_ullong sets the correct value
*/
bool d_tests_datomic_store_ullong(
    struct dj_test_suite* _suite
)
{
    bool            result;
    dj_atomic_ullong val;

    d_test_suite_test(_suite, "datomic_store_ullong");

    result = true;

    d_atomic_init_ullong(&val, 0);
    d_atomic_store_ullong(&val, 444ULL);

    result = D_TEST_CHECK_MSG(_suite,
                              "store_ullong",
                              d_atomic_load_ullong(&val) == 444ULL,
                              "Store ullong should set value to 444") && result;

    return result;
}

/*
d_tests_datomic_store_ptr
  Tests atomic store for pointer type.
  Tests the following:
  - d_atomic_store_ptr sets a non-NULL pointer
*/
bool d_tests_datomic_store_ptr(
    struct dj_test_suite* _suite
)
{
    bool         result;
    dj_atomic_ptr val;
    int          dummy;

    d_test_suite_test(_suite, "datomic_store_ptr");

    result = true;
    dummy  = 0;

    d_atomic_init_ptr(&val, NULL);
    d_atomic_store_ptr(&val, &dummy);

    result = D_TEST_CHECK_MSG(_suite,
                              "store_ptr",
                              d_atomic_load_ptr(&val) == &dummy,
                              "Store ptr should set pointer") && result;

    return result;
}

/*
d_tests_datomic_store_size
  Tests atomic store for size_t type.
  Tests the following:
  - d_atomic_store_size sets the correct value
*/
bool d_tests_datomic_store_size(
    struct dj_test_suite* _suite
)
{
    bool            result;
    dj_atomic_size_t val;

    d_test_suite_test(_suite, "datomic_store_size");

    result = true;

    d_atomic_init_size(&val, 0);
    d_atomic_store_size(&val, 333);

    result = D_TEST_CHECK_MSG(_suite,
                              "store_size",
                              d_atomic_load_size(&val) == 333,
                              "Store size should set value to 333") && result;

    return result;
}

/*
d_tests_datomic_store_explicit
  Tests atomic store with explicit memory orders.
  Tests the following:
  - d_atomic_store_int_explicit with relaxed ordering
  - d_atomic_store_int_explicit with release ordering
*/
bool d_tests_datomic_store_explicit(
    struct dj_test_suite* _suite
)
{
    bool         result;
    dj_atomic_int val;

    d_test_suite_test(_suite, "datomic_store_explicit");

    result = true;

    d_atomic_init_int(&val, 0);

    d_atomic_store_int_explicit(&val, 111,
                                 D_MEMORY_ORDER_RELAXED);

    result = D_TEST_CHECK_MSG(_suite,
                              "store_explicit_relaxed",
                              d_atomic_load_int(&val) == 111,
                              "Store with relaxed order should work") && result;

    d_atomic_store_int_explicit(&val, 222,
                                 D_MEMORY_ORDER_RELEASE);

    result = D_TEST_CHECK_MSG(_suite,
                              "store_explicit_release",
                              d_atomic_load_int(&val) == 222,
                              "Store with release order should work") && result;

    return result;
}

/*
d_tests_datomic_store_all
  Runs all atomic store tests.
  Tests the following:
  - all types: int, uint, long, ulong, llong, ullong, ptr, size
  - explicit memory order variants
*/
bool d_tests_datomic_store_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "datomic_store_all");

    result = true;

    result = d_tests_datomic_store_int(_suite) && result;
    result = d_tests_datomic_store_uint(_suite) && result;
    result = d_tests_datomic_store_long(_suite) && result;
    result = d_tests_datomic_store_ulong(_suite) && result;
    result = d_tests_datomic_store_llong(_suite) && result;
    result = d_tests_datomic_store_ullong(_suite) && result;
    result = d_tests_datomic_store_ptr(_suite) && result;
    result = d_tests_datomic_store_size(_suite) && result;
    result = d_tests_datomic_store_explicit(_suite) && result;

    return result;
}
