#include ".\functional_common_tests_sa.h"


/*
d_tests_sa_functional_is_null
  Tests d_functional_is_null for correctness.
  Tests the following:
  - NULL element pointer returns true (element itself is NULL)
  - pointer-to-NULL-pointer returns true (stored pointer is NULL)
  - pointer-to-non-NULL-pointer returns false
  - context parameter is ignored
  - various non-NULL stored pointer types return false
*/
bool
d_tests_sa_functional_is_null
(
    struct d_test_counter* _counter
)
{
    bool        all_passed;
    const void* stored_ptr;
    int         value;
    int         dummy_context;
    bool        result;

    // validate parameter
    if (!_counter)
    {
        return false;
    }

    all_passed = true;

    // --- test: NULL element pointer returns true ---
    result = d_functional_is_null(NULL, NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "is_null: NULL element pointer returns true",
        "expected true when element itself is NULL",
        _counter);

    // --- test: pointer to NULL stored pointer returns true ---
    stored_ptr = NULL;
    result     = d_functional_is_null(&stored_ptr, NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "is_null: stored NULL pointer returns true",
        "expected true when *element is NULL",
        _counter);

    // --- test: pointer to non-NULL stored pointer returns false ---
    value      = 42;
    stored_ptr = &value;
    result     = d_functional_is_null(&stored_ptr, NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "is_null: stored non-NULL pointer returns false",
        "expected false when *element points to valid data",
        _counter);

    // --- test: context parameter is ignored ---
    stored_ptr    = NULL;
    dummy_context = 99;
    result        = d_functional_is_null(&stored_ptr,
                                         &dummy_context);

    all_passed &= d_assert_standalone(
        result == true,
        "is_null: context is ignored (stored NULL)",
        "expected true; context should not affect result",
        _counter);

    // --- test: non-NULL stored with context still returns false ---
    stored_ptr = &value;
    result     = d_functional_is_null(&stored_ptr,
                                      &dummy_context);

    all_passed &= d_assert_standalone(
        result == false,
        "is_null: context is ignored (stored non-NULL)",
        "expected false; context should not affect result",
        _counter);

    return all_passed;
}


/*
d_tests_sa_functional_is_not_null
  Tests d_functional_is_not_null for correctness.
  Tests the following:
  - NULL element pointer returns false (element itself is NULL)
  - pointer-to-NULL-pointer returns false (stored pointer is NULL)
  - pointer-to-non-NULL-pointer returns true
  - context parameter is ignored
  - complementary behavior to d_functional_is_null
*/
bool
d_tests_sa_functional_is_not_null
(
    struct d_test_counter* _counter
)
{
    bool        all_passed;
    const void* stored_ptr;
    int         value;
    int         dummy_context;
    bool        result;

    // validate parameter
    if (!_counter)
    {
        return false;
    }

    all_passed = true;

    // --- test: NULL element pointer returns false ---
    result = d_functional_is_not_null(NULL, NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "is_not_null: NULL element pointer returns false",
        "expected false when element itself is NULL",
        _counter);

    // --- test: pointer to NULL stored pointer returns false ---
    stored_ptr = NULL;
    result     = d_functional_is_not_null(&stored_ptr, NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "is_not_null: stored NULL pointer returns false",
        "expected false when *element is NULL",
        _counter);

    // --- test: pointer to non-NULL stored pointer returns true ---
    value      = 42;
    stored_ptr = &value;
    result     = d_functional_is_not_null(&stored_ptr, NULL);

    all_passed &= d_assert_standalone(
        result == true,
        "is_not_null: stored non-NULL pointer returns true",
        "expected true when *element points to valid data",
        _counter);

    // --- test: context parameter is ignored ---
    stored_ptr    = &value;
    dummy_context = 99;
    result        = d_functional_is_not_null(&stored_ptr,
                                             &dummy_context);

    all_passed &= d_assert_standalone(
        result == true,
        "is_not_null: context is ignored (stored non-NULL)",
        "expected true; context should not affect result",
        _counter);

    // --- test: complementary to is_null (NULL stored) ---
    stored_ptr = NULL;
    result     = d_functional_is_not_null(&stored_ptr, NULL);

    all_passed &= d_assert_standalone(
        result == false,
        "is_not_null: complementary to is_null for NULL stored",
        "expected false; complement of is_null's true",
        _counter);

    // --- test: complementary to is_null (non-NULL stored) ---
    stored_ptr = &value;

    all_passed &= d_assert_standalone(
        d_functional_is_null(&stored_ptr, NULL) !=
            d_functional_is_not_null(&stored_ptr, NULL),
        "is_not_null: is_null and is_not_null are complementary",
        "expected opposite results for same non-NULL stored ptr",
        _counter);

    // --- test: complementary to is_null (NULL element) ---
    all_passed &= d_assert_standalone(
        d_functional_is_null(NULL, NULL) !=
            d_functional_is_not_null(NULL, NULL),
        "is_not_null: complementary for NULL element",
        "expected opposite results for NULL element",
        _counter);

    return all_passed;
}
