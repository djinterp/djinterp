#include ".\datomic_tests_sa.h"


/*
d_tests_sa_atomic_flag_test_and_set
  Tests atomic flag test-and-set operation.
  Tests the following:
  - initial test_and_set returns false (flag was clear)
  - second test_and_set returns true (flag was set)
  - flag remains set on subsequent calls
*/
bool
d_tests_sa_atomic_flag_test_and_set
(
    struct d_test_counter* _counter
)
{
    bool          result;
    d_atomic_flag flag;

    result = true;
    flag   = (d_atomic_flag)D_ATOMIC_FLAG_INIT;

    // initial test_and_set should return false
    result = d_assert_standalone(
        !d_atomic_flag_test_and_set(&flag),
        "flag_test_and_set_initial",
        "Initial flag should be clear (return false)",
        _counter) && result;

    // second test_and_set should return true
    result = d_assert_standalone(
        d_atomic_flag_test_and_set(&flag),
        "flag_test_and_set_set",
        "Flag should be set (return true)",
        _counter) && result;

    // flag remains set
    result = d_assert_standalone(
        d_atomic_flag_test_and_set(&flag),
        "flag_test_and_set_remains",
        "Flag should remain set",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_flag_clear
  Tests atomic flag clear operation.
  Tests the following:
  - flag can be cleared after being set
  - test_and_set returns false after clear
*/
bool
d_tests_sa_atomic_flag_clear
(
    struct d_test_counter* _counter
)
{
    bool          result;
    d_atomic_flag flag;

    result = true;
    flag   = (d_atomic_flag)D_ATOMIC_FLAG_INIT;

    // set the flag first
    d_atomic_flag_test_and_set(&flag);

    // clear the flag
    d_atomic_flag_clear(&flag);

    // test_and_set after clear should return false
    result = d_assert_standalone(
        !d_atomic_flag_test_and_set(&flag),
        "flag_clear_works",
        "Flag should be clear after clear()",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_flag_explicit
  Tests atomic flag with explicit memory orders.
  Tests the following:
  - test_and_set_explicit with seq_cst ordering
  - clear_explicit with seq_cst ordering
*/
bool
d_tests_sa_atomic_flag_explicit
(
    struct d_test_counter* _counter
)
{
    bool          result;
    d_atomic_flag flag;

    result = true;
    flag   = (d_atomic_flag)D_ATOMIC_FLAG_INIT;

    // test with explicit memory order
    result = d_assert_standalone(
        !d_atomic_flag_test_and_set_explicit(
            &flag, D_MEMORY_ORDER_SEQ_CST),
        "flag_explicit_test_and_set",
        "Explicit test_and_set should work",
        _counter) && result;

    d_atomic_flag_clear_explicit(&flag, D_MEMORY_ORDER_SEQ_CST);

    result = d_assert_standalone(
        !d_atomic_flag_test_and_set(&flag),
        "flag_explicit_clear",
        "Explicit clear should work",
        _counter) && result;

    return result;
}

/*
d_tests_sa_atomic_flag_all
  Runs all atomic flag tests.
  Tests the following:
  - test_and_set, clear, explicit
*/
bool
d_tests_sa_atomic_flag_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Atomic Flag Operations\n");
    printf("  ---------------------------------\n");

    result = d_tests_sa_atomic_flag_test_and_set(_counter) && result;
    result = d_tests_sa_atomic_flag_clear(_counter) && result;
    result = d_tests_sa_atomic_flag_explicit(_counter) && result;

    return result;
}