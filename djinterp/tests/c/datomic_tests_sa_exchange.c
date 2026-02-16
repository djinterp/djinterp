#include ".\datomic_tests_sa.h"


/*
d_tests_sa_atomic_exchange_int
  Tests atomic exchange for int type.
  Tests the following:
  - d_atomic_exchange_int returns the old value
  - the atomic holds the new value after exchange
*/
bool
d_tests_sa_atomic_exchange_int
(
    struct d_test_counter* _counter
)
{
    bool         result;
    d_atomic_int val;
    int          old;

    result = true;

    d_atomic_init_int(&val, 100);
    old = d_atomic_exchange_int(&val, 200);

    result = d_assert_standalone(
        old == 100,
        "exchange_int_old",
        "Exchange should return old value 100",
        _counter) && result;

    result = d_assert_standalone(
        d_atomic_load_int(&val) == 200,
        "exchange_int_new",
        "Exchange should set new value 200",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_exchange_uint
  Tests atomic exchange for unsigned int type.
  Tests the following:
  - d_atomic_exchange_uint returns old and sets new value
*/
bool
d_tests_sa_atomic_exchange_uint
(
    struct d_test_counter* _counter
)
{
    bool          result;
    d_atomic_uint val;
    unsigned int  old;

    result = true;

    d_atomic_init_uint(&val, 50);
    old = d_atomic_exchange_uint(&val, 150);

    result = d_assert_standalone(
        old == 50 && d_atomic_load_uint(&val) == 150,
        "exchange_uint",
        "Exchange uint should work correctly",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_exchange_long
  Tests atomic exchange for long type.
  Tests the following:
  - d_atomic_exchange_long handles negative values correctly
*/
bool
d_tests_sa_atomic_exchange_long
(
    struct d_test_counter* _counter
)
{
    bool          result;
    d_atomic_long val;
    long          old;

    result = true;

    d_atomic_init_long(&val, -50L);
    old = d_atomic_exchange_long(&val, 75L);

    result = d_assert_standalone(
        old == -50L && d_atomic_load_long(&val) == 75L,
        "exchange_long",
        "Exchange long should work correctly",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_exchange_ulong
  Tests atomic exchange for unsigned long type.
  Tests the following:
  - d_atomic_exchange_ulong returns old and sets new value
*/
bool
d_tests_sa_atomic_exchange_ulong
(
    struct d_test_counter* _counter
)
{
    bool           result;
    d_atomic_ulong val;
    unsigned long  old;

    result = true;

    d_atomic_init_ulong(&val, 100UL);
    old = d_atomic_exchange_ulong(&val, 300UL);

    result = d_assert_standalone(
        old == 100UL && d_atomic_load_ulong(&val) == 300UL,
        "exchange_ulong",
        "Exchange ulong should work correctly",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_exchange_llong
  Tests atomic exchange for long long type.
  Tests the following:
  - d_atomic_exchange_llong returns old and sets new value
*/
bool
d_tests_sa_atomic_exchange_llong
(
    struct d_test_counter* _counter
)
{
    bool           result;
    d_atomic_llong val;
    long long      old;

    result = true;

    d_atomic_init_llong(&val, 1000LL);
    old = d_atomic_exchange_llong(&val, 2000LL);

    result = d_assert_standalone(
        old == 1000LL && d_atomic_load_llong(&val) == 2000LL,
        "exchange_llong",
        "Exchange llong should work correctly",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_exchange_ullong
  Tests atomic exchange for unsigned long long type.
  Tests the following:
  - d_atomic_exchange_ullong returns old and sets new value
*/
bool
d_tests_sa_atomic_exchange_ullong
(
    struct d_test_counter* _counter
)
{
    bool               result;
    d_atomic_ullong     val;
    unsigned long long old;

    result = true;

    d_atomic_init_ullong(&val, 5000ULL);
    old = d_atomic_exchange_ullong(&val, 6000ULL);

    result = d_assert_standalone(
        old == 5000ULL && d_atomic_load_ullong(&val) == 6000ULL,
        "exchange_ullong",
        "Exchange ullong should work correctly",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_exchange_ptr
  Tests atomic exchange for pointer type.
  Tests the following:
  - d_atomic_exchange_ptr returns old pointer and sets new one
*/
bool
d_tests_sa_atomic_exchange_ptr
(
    struct d_test_counter* _counter
)
{
    bool         result;
    d_atomic_ptr val;
    int          dummy1;
    int          dummy2;
    void*        old;

    result = true;
    dummy1 = 1;
    dummy2 = 2;

    d_atomic_init_ptr(&val, &dummy1);
    old = d_atomic_exchange_ptr(&val, &dummy2);

    result = d_assert_standalone(
        old == &dummy1 && d_atomic_load_ptr(&val) == &dummy2,
        "exchange_ptr",
        "Exchange ptr should work correctly",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_exchange_size
  Tests atomic exchange for size_t type.
  Tests the following:
  - d_atomic_exchange_size returns old and sets new value
*/
bool
d_tests_sa_atomic_exchange_size
(
    struct d_test_counter* _counter
)
{
    bool            result;
    d_atomic_size_t val;
    size_t          old;

    result = true;

    d_atomic_init_size(&val, 111);
    old = d_atomic_exchange_size(&val, 222);

    result = d_assert_standalone(
        old == 111 && d_atomic_load_size(&val) == 222,
        "exchange_size",
        "Exchange size should work correctly",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_exchange_explicit
  Tests atomic exchange with explicit memory orders.
  Tests the following:
  - d_atomic_exchange_int_explicit with acquire ordering
  - d_atomic_exchange_int_explicit with release ordering
  - d_atomic_exchange_int_explicit with seq_cst ordering
  - d_atomic_exchange_ptr_explicit with seq_cst ordering
*/
bool
d_tests_sa_atomic_exchange_explicit
(
    struct d_test_counter* _counter
)
{
    bool         result;
    d_atomic_int val;
    d_atomic_ptr pval;
    int          old;
    int          dummy1;
    int          dummy2;
    void*        old_ptr;

    result = true;
    dummy1 = 1;
    dummy2 = 2;

    d_atomic_init_int(&val, 10);

    old = d_atomic_exchange_int_explicit(&val, 20,
                                          D_MEMORY_ORDER_ACQUIRE);

    result = d_assert_standalone(
        old == 10 && d_atomic_load_int(&val) == 20,
        "exchange_explicit_acquire",
        "Exchange with acquire order should work",
        _counter) && result;

    old = d_atomic_exchange_int_explicit(&val, 30,
                                          D_MEMORY_ORDER_RELEASE);

    result = d_assert_standalone(
        old == 20 && d_atomic_load_int(&val) == 30,
        "exchange_explicit_release",
        "Exchange with release order should work",
        _counter) && result;

    old = d_atomic_exchange_int_explicit(&val, 40,
                                          D_MEMORY_ORDER_SEQ_CST);

    result = d_assert_standalone(
        old == 30 && d_atomic_load_int(&val) == 40,
        "exchange_explicit_seq_cst",
        "Exchange with seq_cst order should work",
        _counter) && result;

    // test ptr explicit variant
    d_atomic_init_ptr(&pval, &dummy1);

    old_ptr = d_atomic_exchange_ptr_explicit(&pval, &dummy2,
                                              D_MEMORY_ORDER_SEQ_CST);

    result = d_assert_standalone(
        old_ptr == &dummy1 && d_atomic_load_ptr(&pval) == &dummy2,
        "exchange_ptr_explicit_seq_cst",
        "Exchange ptr with seq_cst should work",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_exchange_all
  Runs all atomic exchange tests.
  Tests the following:
  - all types: int, uint, long, ulong, llong, ullong, ptr, size
  - explicit memory order variants
*/
bool
d_tests_sa_atomic_exchange_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Atomic Exchange Operations\n");
    printf("  -------------------------------------\n");

    result = d_tests_sa_atomic_exchange_int(_counter) && result;
    result = d_tests_sa_atomic_exchange_uint(_counter) && result;
    result = d_tests_sa_atomic_exchange_long(_counter) && result;
    result = d_tests_sa_atomic_exchange_ulong(_counter) && result;
    result = d_tests_sa_atomic_exchange_llong(_counter) && result;
    result = d_tests_sa_atomic_exchange_ullong(_counter) && result;
    result = d_tests_sa_atomic_exchange_ptr(_counter) && result;
    result = d_tests_sa_atomic_exchange_size(_counter) && result;
    result = d_tests_sa_atomic_exchange_explicit(_counter) && result;

    return result;
}
