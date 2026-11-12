/*******************************************************************************
* djinterp [c][sync]                                       datomic_tests_fetch.c
*
*
* path:      /tests/djinterp/c/sync/datomic_tests_fetch.c
* link(s):   TBA
* author(s): TBA                                             created: 2026.10.15
*                                                            revised: 2026.11.06
*******************************************************************************/
// djinterp
#include "datomic_tests.h"


/*
d_tests_datomic_fetch_add_int
  Tests fetch-add for int type.
  Tests the following:
  - d_atomic_fetch_add_int returns old value
  - value is incremented after fetch-add
*/
bool d_tests_datomic_fetch_add_int(
    struct dj_test_suite* _suite
)
{
    bool         result;
    dj_atomic_int val;
    int          old;

    d_test_suite_test(_suite, "datomic_fetch_add_int");

    result = true;

    d_atomic_init_int(&val, 10);
    old = d_atomic_fetch_add_int(&val, 5);

    result = D_TEST_CHECK_MSG(_suite,
                              "fetch_add_int_old",
                              old == 10,
                              "Fetch-add should return old value 10") && result;

    result = D_TEST_CHECK_MSG(_suite,
                              "fetch_add_int_new",
                              d_atomic_load_int(&val) == 15,
                              "Fetch-add should result in 15") && result;

    return result;
}

/*
d_tests_datomic_fetch_add_uint
  Tests fetch-add for unsigned int type.
  Tests the following:
  - d_atomic_fetch_add_uint returns old and adds correctly
*/
bool d_tests_datomic_fetch_add_uint(
    struct dj_test_suite* _suite
)
{
    bool          result;
    dj_atomic_uint val;
    unsigned int  old;

    d_test_suite_test(_suite, "datomic_fetch_add_uint");

    result = true;

    d_atomic_init_uint(&val, 20);
    old = d_atomic_fetch_add_uint(&val, 10);

    result = D_TEST_CHECK_MSG(_suite,
                              "fetch_add_uint",
                              old == 20 && d_atomic_load_uint(&val) == 30,
                              "Fetch-add uint should work") && result;

    return result;
}

/*
d_tests_datomic_fetch_add_long
  Tests fetch-add for long type.
  Tests the following:
  - d_atomic_fetch_add_long returns old and adds correctly
*/
bool d_tests_datomic_fetch_add_long(
    struct dj_test_suite* _suite
)
{
    bool          result;
    dj_atomic_long val;
    long          old;

    d_test_suite_test(_suite, "datomic_fetch_add_long");

    result = true;

    d_atomic_init_long(&val, 100L);
    old = d_atomic_fetch_add_long(&val, 50L);

    result = D_TEST_CHECK_MSG(_suite,
                              "fetch_add_long",
                              old == 100L && d_atomic_load_long(&val) == 150L,
                              "Fetch-add long should work") && result;

    return result;
}

/*
d_tests_datomic_fetch_add_llong
  Tests fetch-add for long long type.
  Tests the following:
  - d_atomic_fetch_add_llong returns old and adds correctly
*/
bool d_tests_datomic_fetch_add_llong(
    struct dj_test_suite* _suite
)
{
    bool           result;
    dj_atomic_llong val;
    d_test_suite_test(_suite, "datomic_fetch_add_llong");

    long long      old;

    result = true;

    d_atomic_init_llong(&val, 1000LL);
    old = d_atomic_fetch_add_llong(&val, 500LL);

    result = D_TEST_CHECK_MSG(_suite,
                              "fetch_add_llong",
                              old == 1000LL && d_atomic_load_llong(&val) == 1500LL,
                              "Fetch-add llong should work") && result;

    return result;
}

/*
d_tests_datomic_fetch_add_size
  Tests fetch-add for size_t type.
  Tests the following:
  - d_atomic_fetch_add_size returns old and adds correctly
*/
bool d_tests_datomic_fetch_add_size(
    struct dj_test_suite* _suite
)
{
    bool            result;
    dj_atomic_size_t val;
    size_t          old;

    d_test_suite_test(_suite, "datomic_fetch_add_size");

    result = true;

    d_atomic_init_size(&val, 200);
    old = d_atomic_fetch_add_size(&val, 100);

    result = D_TEST_CHECK_MSG(_suite,
                              "fetch_add_size",
                              old == 200 && d_atomic_load_size(&val) == 300,
                              "Fetch-add size should work") && result;

    return result;
}

/*
d_tests_datomic_fetch_sub_int
  Tests fetch-sub for int type.
  Tests the following:
  - d_atomic_fetch_sub_int returns old and subtracts correctly
*/
bool d_tests_datomic_fetch_sub_int(
    struct dj_test_suite* _suite
)
{
    bool         result;
    dj_atomic_int val;
    int          old;

    d_test_suite_test(_suite, "datomic_fetch_sub_int");

    result = true;

    d_atomic_init_int(&val, 50);
    old = d_atomic_fetch_sub_int(&val, 20);

    result = D_TEST_CHECK_MSG(_suite,
                              "fetch_sub_int",
                              old == 50 && d_atomic_load_int(&val) == 30,
                              "Fetch-sub int should work") && result;

    return result;
}

/*
d_tests_datomic_fetch_sub_long
  Tests fetch-sub for long type.
  Tests the following:
  - d_atomic_fetch_sub_long returns old and subtracts correctly
*/
bool d_tests_datomic_fetch_sub_long(
    struct dj_test_suite* _suite
)
{
    bool          result;
    dj_atomic_long val;
    long          old;

    d_test_suite_test(_suite, "datomic_fetch_sub_long");

    result = true;

    d_atomic_init_long(&val, 300L);
    old = d_atomic_fetch_sub_long(&val, 100L);

    result = D_TEST_CHECK_MSG(_suite,
                              "fetch_sub_long",
                              old == 300L && d_atomic_load_long(&val) == 200L,
                              "Fetch-sub long should work") && result;

    return result;
}

/*
d_tests_datomic_fetch_sub_size
  Tests fetch-sub for size_t type.
  Tests the following:
  - d_atomic_fetch_sub_size returns old and subtracts correctly
*/
bool d_tests_datomic_fetch_sub_size(
    struct dj_test_suite* _suite
)
{
    bool            result;
    dj_atomic_size_t val;
    size_t          old;

    d_test_suite_test(_suite, "datomic_fetch_sub_size");

    result = true;

    d_atomic_init_size(&val, 500);
    old = d_atomic_fetch_sub_size(&val, 200);

    result = D_TEST_CHECK_MSG(_suite,
                              "fetch_sub_size",
                              old == 500 && d_atomic_load_size(&val) == 300,
                              "Fetch-sub size should work") && result;

    return result;
}

/*
d_tests_datomic_fetch_or_int
  Tests fetch-or for int type.
  Tests the following:
  - d_atomic_fetch_or_int returns old and ORs correctly
*/
bool d_tests_datomic_fetch_or_int(
    struct dj_test_suite* _suite
)
{
    bool         result;
    dj_atomic_int val;
    int          old;

    d_test_suite_test(_suite, "datomic_fetch_or_int");

    result = true;

    d_atomic_init_int(&val, 0x0F);
    old = d_atomic_fetch_or_int(&val, 0xF0);

    result = D_TEST_CHECK_MSG(_suite,
                              "fetch_or_int",
                              old == 0x0F && d_atomic_load_int(&val) == 0xFF,
                              "Fetch-or int should work") && result;

    return result;
}

/*
d_tests_datomic_fetch_xor_int
  Tests fetch-xor for int type.
  Tests the following:
  - d_atomic_fetch_xor_int returns old and XORs correctly
*/
bool d_tests_datomic_fetch_xor_int(
    struct dj_test_suite* _suite
)
{
    bool         result;
    dj_atomic_int val;
    int          old;

    d_test_suite_test(_suite, "datomic_fetch_xor_int");

    result = true;

    d_atomic_init_int(&val, 0xFF);
    old = d_atomic_fetch_xor_int(&val, 0x0F);

    result = D_TEST_CHECK_MSG(_suite,
                              "fetch_xor_int",
                              old == 0xFF && d_atomic_load_int(&val) == 0xF0,
                              "Fetch-xor int should work") && result;

    return result;
}

/*
d_tests_datomic_fetch_and_int
  Tests fetch-and for int type.
  Tests the following:
  - d_atomic_fetch_and_int returns old and ANDs correctly
*/
bool d_tests_datomic_fetch_and_int(
    struct dj_test_suite* _suite
)
{
    bool         result;
    dj_atomic_int val;
    int          old;

    d_test_suite_test(_suite, "datomic_fetch_and_int");

    result = true;

    d_atomic_init_int(&val, 0xFF);
    old = d_atomic_fetch_and_int(&val, 0x0F);

    result = D_TEST_CHECK_MSG(_suite,
                              "fetch_and_int",
                              old == 0xFF && d_atomic_load_int(&val) == 0x0F,
                              "Fetch-and int should work") && result;

    return result;
}

/*
d_tests_datomic_fetch_explicit
  Tests fetch operations with explicit memory orders.
  Tests the following:
  - fetch_add_int_explicit with acquire ordering
  - fetch_sub_int_explicit with release ordering
  - fetch_or_int_explicit with seq_cst ordering
  - fetch_and_int_explicit with relaxed ordering
  - fetch_xor_int_explicit with seq_cst ordering
*/
bool d_tests_datomic_fetch_explicit(
    struct dj_test_suite* _suite
)
{
    bool         result;
    dj_atomic_int val;
    int          old;

    d_test_suite_test(_suite, "datomic_fetch_explicit");

    result = true;

    // fetch_add explicit
    d_atomic_init_int(&val, 10);

    old = d_atomic_fetch_add_int_explicit(&val, 5,
                                           D_MEMORY_ORDER_ACQUIRE);

    result = D_TEST_CHECK_MSG(_suite,
                              "fetch_add_explicit_acquire",
                              old == 10 && d_atomic_load_int(&val) == 15,
                              "Fetch-add with acquire order should work") && result;

    // fetch_sub explicit
    old = d_atomic_fetch_sub_int_explicit(&val, 3,
                                           D_MEMORY_ORDER_RELEASE);

    result = D_TEST_CHECK_MSG(_suite,
                              "fetch_sub_explicit_release",
                              old == 15 && d_atomic_load_int(&val) == 12,
                              "Fetch-sub with release order should work") && result;

    // fetch_or explicit
    d_atomic_store_int(&val, 0x0F);

    old = d_atomic_fetch_or_int_explicit(&val, 0xF0,
                                          D_MEMORY_ORDER_SEQ_CST);

    result = D_TEST_CHECK_MSG(_suite,
                              "fetch_or_explicit_seq_cst",
                              old == 0x0F && d_atomic_load_int(&val) == 0xFF,
                              "Fetch-or with seq_cst order should work") && result;

    // fetch_and explicit
    old = d_atomic_fetch_and_int_explicit(&val, 0x0F,
                                           D_MEMORY_ORDER_RELAXED);

    result = D_TEST_CHECK_MSG(_suite,
                              "fetch_and_explicit_relaxed",
                              old == 0xFF && d_atomic_load_int(&val) == 0x0F,
                              "Fetch-and with relaxed order should work") && result;

    // fetch_xor explicit
    old = d_atomic_fetch_xor_int_explicit(&val, 0xFF,
                                           D_MEMORY_ORDER_SEQ_CST);

    result = D_TEST_CHECK_MSG(_suite,
                              "fetch_xor_explicit_seq_cst",
                              old == 0x0F && d_atomic_load_int(&val) == 0xF0,
                              "Fetch-xor with seq_cst order should work") && result;

    return result;
}

/*
d_tests_datomic_fetch_all
  Runs all atomic fetch-and-modify tests.
  Tests the following:
  - fetch_add for int, uint, long, llong, size
  - fetch_sub for int, long, size
  - fetch_or, fetch_xor, fetch_and for int
  - explicit memory order variants
*/
bool d_tests_datomic_fetch_all(
    struct dj_test_suite* _suite
)
{
    bool result;

    d_test_suite_block(_suite, "datomic_fetch_all");

    result = true;

    result = d_tests_datomic_fetch_add_int(_suite) && result;
    result = d_tests_datomic_fetch_add_uint(_suite) && result;
    result = d_tests_datomic_fetch_add_long(_suite) && result;
    result = d_tests_datomic_fetch_add_llong(_suite) && result;
    result = d_tests_datomic_fetch_add_size(_suite) && result;
    result = d_tests_datomic_fetch_sub_int(_suite) && result;
    result = d_tests_datomic_fetch_sub_long(_suite) && result;
    result = d_tests_datomic_fetch_sub_size(_suite) && result;
    result = d_tests_datomic_fetch_or_int(_suite) && result;
    result = d_tests_datomic_fetch_xor_int(_suite) && result;
    result = d_tests_datomic_fetch_and_int(_suite) && result;
    result = d_tests_datomic_fetch_explicit(_suite) && result;

    return result;
}
