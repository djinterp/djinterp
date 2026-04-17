#include ".\datomic_tests_sa.h"


/*
d_tests_sa_atomic_fetch_add_int
  Tests fetch-add for int type.
  Tests the following:
  - d_atomic_fetch_add_int returns old value
  - value is incremented after fetch-add
*/
bool
d_tests_sa_atomic_fetch_add_int
(
    struct d_test_counter* _counter
)
{
    bool         result;
    d_atomic_int val;
    int          old;

    result = true;

    d_atomic_init_int(&val, 10);
    old = d_atomic_fetch_add_int(&val, 5);

    result = d_assert_standalone(
        old == 10,
        "fetch_add_int_old",
        "Fetch-add should return old value 10",
        _counter) && result;

    result = d_assert_standalone(
        d_atomic_load_int(&val) == 15,
        "fetch_add_int_new",
        "Fetch-add should result in 15",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_fetch_add_uint
  Tests fetch-add for unsigned int type.
  Tests the following:
  - d_atomic_fetch_add_uint returns old and adds correctly
*/
bool
d_tests_sa_atomic_fetch_add_uint
(
    struct d_test_counter* _counter
)
{
    bool          result;
    d_atomic_uint val;
    unsigned int  old;

    result = true;

    d_atomic_init_uint(&val, 20);
    old = d_atomic_fetch_add_uint(&val, 10);

    result = d_assert_standalone(
        old == 20 && d_atomic_load_uint(&val) == 30,
        "fetch_add_uint",
        "Fetch-add uint should work",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_fetch_add_long
  Tests fetch-add for long type.
  Tests the following:
  - d_atomic_fetch_add_long returns old and adds correctly
*/
bool
d_tests_sa_atomic_fetch_add_long
(
    struct d_test_counter* _counter
)
{
    bool          result;
    d_atomic_long val;
    long          old;

    result = true;

    d_atomic_init_long(&val, 100L);
    old = d_atomic_fetch_add_long(&val, 50L);

    result = d_assert_standalone(
        old == 100L && d_atomic_load_long(&val) == 150L,
        "fetch_add_long",
        "Fetch-add long should work",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_fetch_add_llong
  Tests fetch-add for long long type.
  Tests the following:
  - d_atomic_fetch_add_llong returns old and adds correctly
*/
bool
d_tests_sa_atomic_fetch_add_llong
(
    struct d_test_counter* _counter
)
{
    bool           result;
    d_atomic_llong val;
    long long      old;

    result = true;

    d_atomic_init_llong(&val, 1000LL);
    old = d_atomic_fetch_add_llong(&val, 500LL);

    result = d_assert_standalone(
        old == 1000LL && d_atomic_load_llong(&val) == 1500LL,
        "fetch_add_llong",
        "Fetch-add llong should work",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_fetch_add_size
  Tests fetch-add for size_t type.
  Tests the following:
  - d_atomic_fetch_add_size returns old and adds correctly
*/
bool
d_tests_sa_atomic_fetch_add_size
(
    struct d_test_counter* _counter
)
{
    bool            result;
    d_atomic_size_t val;
    size_t          old;

    result = true;

    d_atomic_init_size(&val, 200);
    old = d_atomic_fetch_add_size(&val, 100);

    result = d_assert_standalone(
        old == 200 && d_atomic_load_size(&val) == 300,
        "fetch_add_size",
        "Fetch-add size should work",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_fetch_sub_int
  Tests fetch-sub for int type.
  Tests the following:
  - d_atomic_fetch_sub_int returns old and subtracts correctly
*/
bool
d_tests_sa_atomic_fetch_sub_int
(
    struct d_test_counter* _counter
)
{
    bool         result;
    d_atomic_int val;
    int          old;

    result = true;

    d_atomic_init_int(&val, 50);
    old = d_atomic_fetch_sub_int(&val, 20);

    result = d_assert_standalone(
        old == 50 && d_atomic_load_int(&val) == 30,
        "fetch_sub_int",
        "Fetch-sub int should work",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_fetch_sub_long
  Tests fetch-sub for long type.
  Tests the following:
  - d_atomic_fetch_sub_long returns old and subtracts correctly
*/
bool
d_tests_sa_atomic_fetch_sub_long
(
    struct d_test_counter* _counter
)
{
    bool          result;
    d_atomic_long val;
    long          old;

    result = true;

    d_atomic_init_long(&val, 300L);
    old = d_atomic_fetch_sub_long(&val, 100L);

    result = d_assert_standalone(
        old == 300L && d_atomic_load_long(&val) == 200L,
        "fetch_sub_long",
        "Fetch-sub long should work",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_fetch_sub_size
  Tests fetch-sub for size_t type.
  Tests the following:
  - d_atomic_fetch_sub_size returns old and subtracts correctly
*/
bool
d_tests_sa_atomic_fetch_sub_size
(
    struct d_test_counter* _counter
)
{
    bool            result;
    d_atomic_size_t val;
    size_t          old;

    result = true;

    d_atomic_init_size(&val, 500);
    old = d_atomic_fetch_sub_size(&val, 200);

    result = d_assert_standalone(
        old == 500 && d_atomic_load_size(&val) == 300,
        "fetch_sub_size",
        "Fetch-sub size should work",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_fetch_or_int
  Tests fetch-or for int type.
  Tests the following:
  - d_atomic_fetch_or_int returns old and ORs correctly
*/
bool
d_tests_sa_atomic_fetch_or_int
(
    struct d_test_counter* _counter
)
{
    bool         result;
    d_atomic_int val;
    int          old;

    result = true;

    d_atomic_init_int(&val, 0x0F);
    old = d_atomic_fetch_or_int(&val, 0xF0);

    result = d_assert_standalone(
        old == 0x0F && d_atomic_load_int(&val) == 0xFF,
        "fetch_or_int",
        "Fetch-or int should work",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_fetch_xor_int
  Tests fetch-xor for int type.
  Tests the following:
  - d_atomic_fetch_xor_int returns old and XORs correctly
*/
bool
d_tests_sa_atomic_fetch_xor_int
(
    struct d_test_counter* _counter
)
{
    bool         result;
    d_atomic_int val;
    int          old;

    result = true;

    d_atomic_init_int(&val, 0xFF);
    old = d_atomic_fetch_xor_int(&val, 0x0F);

    result = d_assert_standalone(
        old == 0xFF && d_atomic_load_int(&val) == 0xF0,
        "fetch_xor_int",
        "Fetch-xor int should work",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_fetch_and_int
  Tests fetch-and for int type.
  Tests the following:
  - d_atomic_fetch_and_int returns old and ANDs correctly
*/
bool
d_tests_sa_atomic_fetch_and_int
(
    struct d_test_counter* _counter
)
{
    bool         result;
    d_atomic_int val;
    int          old;

    result = true;

    d_atomic_init_int(&val, 0xFF);
    old = d_atomic_fetch_and_int(&val, 0x0F);

    result = d_assert_standalone(
        old == 0xFF && d_atomic_load_int(&val) == 0x0F,
        "fetch_and_int",
        "Fetch-and int should work",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_fetch_explicit
  Tests fetch operations with explicit memory orders.
  Tests the following:
  - fetch_add_int_explicit with acquire ordering
  - fetch_sub_int_explicit with release ordering
  - fetch_or_int_explicit with seq_cst ordering
  - fetch_and_int_explicit with relaxed ordering
  - fetch_xor_int_explicit with seq_cst ordering
*/
bool
d_tests_sa_atomic_fetch_explicit
(
    struct d_test_counter* _counter
)
{
    bool         result;
    d_atomic_int val;
    int          old;

    result = true;

    // fetch_add explicit
    d_atomic_init_int(&val, 10);

    old = d_atomic_fetch_add_int_explicit(&val, 5,
                                           D_MEMORY_ORDER_ACQUIRE);

    result = d_assert_standalone(
        old == 10 && d_atomic_load_int(&val) == 15,
        "fetch_add_explicit_acquire",
        "Fetch-add with acquire order should work",
        _counter) && result;

    // fetch_sub explicit
    old = d_atomic_fetch_sub_int_explicit(&val, 3,
                                           D_MEMORY_ORDER_RELEASE);

    result = d_assert_standalone(
        old == 15 && d_atomic_load_int(&val) == 12,
        "fetch_sub_explicit_release",
        "Fetch-sub with release order should work",
        _counter) && result;

    // fetch_or explicit
    d_atomic_store_int(&val, 0x0F);

    old = d_atomic_fetch_or_int_explicit(&val, 0xF0,
                                          D_MEMORY_ORDER_SEQ_CST);

    result = d_assert_standalone(
        old == 0x0F && d_atomic_load_int(&val) == 0xFF,
        "fetch_or_explicit_seq_cst",
        "Fetch-or with seq_cst order should work",
        _counter) && result;

    // fetch_and explicit
    old = d_atomic_fetch_and_int_explicit(&val, 0x0F,
                                           D_MEMORY_ORDER_RELAXED);

    result = d_assert_standalone(
        old == 0xFF && d_atomic_load_int(&val) == 0x0F,
        "fetch_and_explicit_relaxed",
        "Fetch-and with relaxed order should work",
        _counter) && result;

    // fetch_xor explicit
    old = d_atomic_fetch_xor_int_explicit(&val, 0xFF,
                                           D_MEMORY_ORDER_SEQ_CST);

    result = d_assert_standalone(
        old == 0x0F && d_atomic_load_int(&val) == 0xF0,
        "fetch_xor_explicit_seq_cst",
        "Fetch-xor with seq_cst order should work",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_fetch_all
  Runs all atomic fetch-and-modify tests.
  Tests the following:
  - fetch_add for int, uint, long, llong, size
  - fetch_sub for int, long, size
  - fetch_or, fetch_xor, fetch_and for int
  - explicit memory order variants
*/
bool
d_tests_sa_atomic_fetch_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Atomic Fetch-and-Modify\n");
    printf("  ----------------------------------\n");

    result = d_tests_sa_atomic_fetch_add_int(_counter) && result;
    result = d_tests_sa_atomic_fetch_add_uint(_counter) && result;
    result = d_tests_sa_atomic_fetch_add_long(_counter) && result;
    result = d_tests_sa_atomic_fetch_add_llong(_counter) && result;
    result = d_tests_sa_atomic_fetch_add_size(_counter) && result;
    result = d_tests_sa_atomic_fetch_sub_int(_counter) && result;
    result = d_tests_sa_atomic_fetch_sub_long(_counter) && result;
    result = d_tests_sa_atomic_fetch_sub_size(_counter) && result;
    result = d_tests_sa_atomic_fetch_or_int(_counter) && result;
    result = d_tests_sa_atomic_fetch_xor_int(_counter) && result;
    result = d_tests_sa_atomic_fetch_and_int(_counter) && result;
    result = d_tests_sa_atomic_fetch_explicit(_counter) && result;

    return result;
}