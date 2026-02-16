#include ".\datomic_tests_sa.h"


/*
d_tests_sa_atomic_null_load
  Tests null-safety of atomic load operations.
  Tests the following:
  - d_atomic_load_int(NULL) returns zero
  - d_atomic_load_uint(NULL) returns zero
  - d_atomic_load_llong(NULL) returns zero
  - d_atomic_load_size(NULL) returns zero
  - d_atomic_load_int_explicit(NULL, ...) returns zero
*/
bool
d_tests_sa_atomic_null_load
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(
        d_atomic_load_int(NULL) == 0,
        "null_load_int",
        "Load int from NULL should return 0",
        _counter) && result;

    result = d_assert_standalone(
        d_atomic_load_uint(NULL) == 0U,
        "null_load_uint",
        "Load uint from NULL should return 0",
        _counter) && result;

    result = d_assert_standalone(
        d_atomic_load_llong(NULL) == 0LL,
        "null_load_llong",
        "Load llong from NULL should return 0",
        _counter) && result;

    result = d_assert_standalone(
        d_atomic_load_size(NULL) == (size_t)0,
        "null_load_size",
        "Load size from NULL should return 0",
        _counter) && result;

    result = d_assert_standalone(
        d_atomic_load_int_explicit(NULL,
                                    D_MEMORY_ORDER_SEQ_CST) == 0,
        "null_load_explicit",
        "Load explicit from NULL should return 0",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_null_store
  Tests null-safety of atomic store operations.
  Tests the following:
  - d_atomic_store_int(NULL, ...) does not crash
  - d_atomic_store_llong(NULL, ...) does not crash
  - d_atomic_store_int_explicit(NULL, ...) does not crash
*/
bool
d_tests_sa_atomic_null_store
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // these should silently return without crashing
    d_atomic_store_int(NULL, 42);
    d_atomic_store_llong(NULL, 42LL);
    d_atomic_store_int_explicit(NULL, 42,
                                 D_MEMORY_ORDER_SEQ_CST);

    result = d_assert_standalone(
        true,
        "null_store_no_crash",
        "Store to NULL should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_null_exchange
  Tests null-safety of atomic exchange operations.
  Tests the following:
  - d_atomic_exchange_int(NULL, ...) returns zero
  - d_atomic_exchange_llong(NULL, ...) returns zero
  - d_atomic_exchange_int_explicit(NULL, ...) returns zero
*/
bool
d_tests_sa_atomic_null_exchange
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(
        d_atomic_exchange_int(NULL, 42) == 0,
        "null_exchange_int",
        "Exchange int on NULL should return 0",
        _counter) && result;

    result = d_assert_standalone(
        d_atomic_exchange_llong(NULL, 42LL) == 0LL,
        "null_exchange_llong",
        "Exchange llong on NULL should return 0",
        _counter) && result;

    result = d_assert_standalone(
        d_atomic_exchange_int_explicit(NULL, 42,
                                        D_MEMORY_ORDER_SEQ_CST) == 0,
        "null_exchange_explicit",
        "Exchange explicit on NULL should return 0",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_null_cas
  Tests null-safety of atomic compare-and-exchange operations.
  Tests the following:
  - d_atomic_compare_exchange_strong_int(NULL, ...) returns false
  - CAS with NULL expected pointer returns false
  - CAS with both NULL obj and expected returns false
  - d_atomic_compare_exchange_strong_int_explicit(NULL, ...) false
*/
bool
d_tests_sa_atomic_null_cas
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

    // null object
    result = d_assert_standalone(
        !d_atomic_compare_exchange_strong_int(NULL,
                                               &expected,
                                               200),
        "null_cas_obj",
        "CAS with NULL obj should return false",
        _counter) && result;

    // null expected pointer
    result = d_assert_standalone(
        !d_atomic_compare_exchange_strong_int(&val, NULL, 200),
        "null_cas_expected",
        "CAS with NULL expected should return false",
        _counter) && result;

    // both null
    result = d_assert_standalone(
        !d_atomic_compare_exchange_strong_int(NULL, NULL, 200),
        "null_cas_both",
        "CAS with both NULL should return false",
        _counter) && result;

    // explicit variant
    result = d_assert_standalone(
        !d_atomic_compare_exchange_strong_int_explicit(
            NULL, &expected, 200,
            D_MEMORY_ORDER_SEQ_CST,
            D_MEMORY_ORDER_SEQ_CST),
        "null_cas_explicit",
        "CAS explicit with NULL obj should return false",
        _counter) && result;

    // verify val was not modified by any null-guarded CAS
    result = d_assert_standalone(
        d_atomic_load_int(&val) == 100,
        "null_cas_no_side_effect",
        "CAS with NULL expected should not modify value",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_null_fetch
  Tests null-safety of atomic fetch-and-modify operations.
  Tests the following:
  - d_atomic_fetch_add_int(NULL, ...) returns zero
  - d_atomic_fetch_sub_int(NULL, ...) returns zero
  - d_atomic_fetch_or_int(NULL, ...) returns zero
  - d_atomic_fetch_xor_int(NULL, ...) returns zero
  - d_atomic_fetch_and_int(NULL, ...) returns zero
  - d_atomic_fetch_add_int_explicit(NULL, ...) returns zero
*/
bool
d_tests_sa_atomic_null_fetch
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(
        d_atomic_fetch_add_int(NULL, 1) == 0,
        "null_fetch_add",
        "Fetch-add on NULL should return 0",
        _counter) && result;

    result = d_assert_standalone(
        d_atomic_fetch_sub_int(NULL, 1) == 0,
        "null_fetch_sub",
        "Fetch-sub on NULL should return 0",
        _counter) && result;

    result = d_assert_standalone(
        d_atomic_fetch_or_int(NULL, 0xFF) == 0,
        "null_fetch_or",
        "Fetch-or on NULL should return 0",
        _counter) && result;

    result = d_assert_standalone(
        d_atomic_fetch_xor_int(NULL, 0xFF) == 0,
        "null_fetch_xor",
        "Fetch-xor on NULL should return 0",
        _counter) && result;

    result = d_assert_standalone(
        d_atomic_fetch_and_int(NULL, 0xFF) == 0,
        "null_fetch_and",
        "Fetch-and on NULL should return 0",
        _counter) && result;

    result = d_assert_standalone(
        d_atomic_fetch_add_int_explicit(NULL, 1,
                                         D_MEMORY_ORDER_SEQ_CST) == 0,
        "null_fetch_add_explicit",
        "Fetch-add explicit on NULL should return 0",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_null_flag
  Tests null-safety of atomic flag operations.
  Tests the following:
  - d_atomic_flag_test_and_set(NULL) returns false
  - d_atomic_flag_clear(NULL) does not crash
  - d_atomic_flag_test_and_set_explicit(NULL, ...) returns false
  - d_atomic_flag_clear_explicit(NULL, ...) does not crash
*/
bool
d_tests_sa_atomic_null_flag
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_assert_standalone(
        !d_atomic_flag_test_and_set(NULL),
        "null_flag_test_and_set",
        "Flag test_and_set on NULL should return false",
        _counter) && result;

    // should not crash
    d_atomic_flag_clear(NULL);

    result = d_assert_standalone(
        true,
        "null_flag_clear",
        "Flag clear on NULL should not crash",
        _counter) && result;

    result = d_assert_standalone(
        !d_atomic_flag_test_and_set_explicit(
            NULL, D_MEMORY_ORDER_SEQ_CST),
        "null_flag_test_and_set_explicit",
        "Flag test_and_set explicit on NULL should return false",
        _counter) && result;

    // should not crash
    d_atomic_flag_clear_explicit(NULL, D_MEMORY_ORDER_SEQ_CST);

    result = d_assert_standalone(
        true,
        "null_flag_clear_explicit",
        "Flag clear explicit on NULL should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_null_ptr
  Tests null-safety of atomic pointer operations.
  Tests the following:
  - d_atomic_load_ptr(NULL) returns NULL
  - d_atomic_store_ptr(NULL, ...) does not crash
  - d_atomic_exchange_ptr(NULL, ...) returns NULL
  - d_atomic_compare_exchange_strong_ptr(NULL, ...) returns false
  - d_atomic_load_ptr_explicit(NULL, ...) returns NULL
  - d_atomic_exchange_ptr_explicit(NULL, ...) returns NULL
*/
bool
d_tests_sa_atomic_null_ptr
(
    struct d_test_counter* _counter
)
{
    bool  result;
    int   dummy;
    void* expected;

    result   = true;
    dummy    = 42;
    expected = &dummy;

    result = d_assert_standalone(
        d_atomic_load_ptr(NULL) == NULL,
        "null_ptr_load",
        "Load ptr from NULL should return NULL",
        _counter) && result;

    // should not crash
    d_atomic_store_ptr(NULL, &dummy);

    result = d_assert_standalone(
        true,
        "null_ptr_store",
        "Store ptr to NULL should not crash",
        _counter) && result;

    result = d_assert_standalone(
        d_atomic_exchange_ptr(NULL, &dummy) == NULL,
        "null_ptr_exchange",
        "Exchange ptr on NULL should return NULL",
        _counter) && result;

    result = d_assert_standalone(
        !d_atomic_compare_exchange_strong_ptr(NULL,
                                               &expected,
                                               &dummy),
        "null_ptr_cas",
        "CAS ptr on NULL should return false",
        _counter) && result;

    result = d_assert_standalone(
        d_atomic_load_ptr_explicit(
            NULL, D_MEMORY_ORDER_SEQ_CST) == NULL,
        "null_ptr_load_explicit",
        "Load ptr explicit from NULL should return NULL",
        _counter) && result;

    result = d_assert_standalone(
        d_atomic_exchange_ptr_explicit(
            NULL, &dummy, D_MEMORY_ORDER_SEQ_CST) == NULL,
        "null_ptr_exchange_explicit",
        "Exchange ptr explicit on NULL should return NULL",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_null_all
  Runs all null-safety tests.
  Tests the following:
  - null load, store, exchange, CAS, fetch, flag, and pointer ops
*/
bool
d_tests_sa_atomic_null_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Null-Safety Guards\n");
    printf("  -----------------------------\n");

    result = d_tests_sa_atomic_null_load(_counter) && result;
    result = d_tests_sa_atomic_null_store(_counter) && result;
    result = d_tests_sa_atomic_null_exchange(_counter) && result;
    result = d_tests_sa_atomic_null_cas(_counter) && result;
    result = d_tests_sa_atomic_null_fetch(_counter) && result;
    result = d_tests_sa_atomic_null_flag(_counter) && result;
    result = d_tests_sa_atomic_null_ptr(_counter) && result;

    return result;
}
