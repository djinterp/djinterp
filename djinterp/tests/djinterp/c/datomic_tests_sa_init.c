#include ".\datomic_tests_sa.h"


/*
d_tests_sa_atomic_init_int
  Tests atomic initialization for int type.
  Tests the following:
  - d_atomic_init_int sets the correct value
  - d_atomic_load_int reads the initialized value
*/
bool
d_tests_sa_atomic_init_int
(
    struct d_test_counter* _counter
)
{
    bool         result;
    d_atomic_int val;

    result = true;

    d_atomic_init_int(&val, 42);

    result = d_assert_standalone(
        d_atomic_load_int(&val) == 42,
        "init_int",
        "Init int should set value to 42",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_init_uint
  Tests atomic initialization for unsigned int type.
  Tests the following:
  - d_atomic_init_uint sets the correct value
*/
bool
d_tests_sa_atomic_init_uint
(
    struct d_test_counter* _counter
)
{
    bool          result;
    d_atomic_uint val;

    result = true;

    d_atomic_init_uint(&val, 99);

    result = d_assert_standalone(
        d_atomic_load_uint(&val) == 99,
        "init_uint",
        "Init uint should set value to 99",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_init_long
  Tests atomic initialization for long type.
  Tests the following:
  - d_atomic_init_long sets a negative value correctly
*/
bool
d_tests_sa_atomic_init_long
(
    struct d_test_counter* _counter
)
{
    bool          result;
    d_atomic_long val;

    result = true;

    d_atomic_init_long(&val, -123);

    result = d_assert_standalone(
        d_atomic_load_long(&val) == -123,
        "init_long",
        "Init long should set value to -123",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_init_ulong
  Tests atomic initialization for unsigned long type.
  Tests the following:
  - d_atomic_init_ulong sets the correct value
*/
bool
d_tests_sa_atomic_init_ulong
(
    struct d_test_counter* _counter
)
{
    bool           result;
    d_atomic_ulong val;

    result = true;

    d_atomic_init_ulong(&val, 456UL);

    result = d_assert_standalone(
        d_atomic_load_ulong(&val) == 456UL,
        "init_ulong",
        "Init ulong should set value to 456",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_init_llong
  Tests atomic initialization for long long type.
  Tests the following:
  - d_atomic_init_llong sets a large negative value correctly
*/
bool
d_tests_sa_atomic_init_llong
(
    struct d_test_counter* _counter
)
{
    bool           result;
    d_atomic_llong val;

    result = true;

    d_atomic_init_llong(&val, -9876543210LL);

    result = d_assert_standalone(
        d_atomic_load_llong(&val) == -9876543210LL,
        "init_llong",
        "Init llong should set value to -9876543210",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_init_ullong
  Tests atomic initialization for unsigned long long type.
  Tests the following:
  - d_atomic_init_ullong sets a large value correctly
*/
bool
d_tests_sa_atomic_init_ullong
(
    struct d_test_counter* _counter
)
{
    bool            result;
    d_atomic_ullong val;

    result = true;

    d_atomic_init_ullong(&val, 9876543210ULL);

    result = d_assert_standalone(
        d_atomic_load_ullong(&val) == 9876543210ULL,
        "init_ullong",
        "Init ullong should set value to 9876543210",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_init_ptr
  Tests atomic initialization for pointer type.
  Tests the following:
  - d_atomic_init_ptr sets a non-NULL pointer
  - d_atomic_init_ptr handles NULL correctly
*/
bool
d_tests_sa_atomic_init_ptr
(
    struct d_test_counter* _counter
)
{
    bool         result;
    d_atomic_ptr val;
    int          dummy;

    result = true;
    dummy  = 0;

    d_atomic_init_ptr(&val, &dummy);

    result = d_assert_standalone(
        d_atomic_load_ptr(&val) == &dummy,
        "init_ptr",
        "Init ptr should set pointer value",
        _counter) && result;

    d_atomic_init_ptr(&val, NULL);

    result = d_assert_standalone(
        d_atomic_load_ptr(&val) == NULL,
        "init_ptr_null",
        "Init ptr should handle NULL",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_init_size
  Tests atomic initialization for size_t type.
  Tests the following:
  - d_atomic_init_size sets the correct value
*/
bool
d_tests_sa_atomic_init_size
(
    struct d_test_counter* _counter
)
{
    bool            result;
    d_atomic_size_t val;

    result = true;

    d_atomic_init_size(&val, 12345);

    result = d_assert_standalone(
        d_atomic_load_size(&val) == 12345,
        "init_size",
        "Init size should set value to 12345",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_init_all
  Runs all atomic initialization tests.
  Tests the following:
  - int, uint, long, ulong, llong, ullong, ptr, size initialization
*/
bool
d_tests_sa_atomic_init_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Atomic Initialization\n");
    printf("  --------------------------------\n");

    result = d_tests_sa_atomic_init_int(_counter) && result;
    result = d_tests_sa_atomic_init_uint(_counter) && result;
    result = d_tests_sa_atomic_init_long(_counter) && result;
    result = d_tests_sa_atomic_init_ulong(_counter) && result;
    result = d_tests_sa_atomic_init_llong(_counter) && result;
    result = d_tests_sa_atomic_init_ullong(_counter) && result;
    result = d_tests_sa_atomic_init_ptr(_counter) && result;
    result = d_tests_sa_atomic_init_size(_counter) && result;

    return result;
}
