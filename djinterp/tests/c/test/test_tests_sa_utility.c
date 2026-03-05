#include "./test_tests_sa.h"


/*
d_tests_sa_test_type_flag_to_string
  Tests d_test_type_flag_to_string for all known enum values.
  Tests the following:
  - D_TEST_TYPE_ASSERT maps to "ASSERTION"
  - D_TEST_TYPE_TEST_FN maps to "TEST_FN"
  - D_TEST_TYPE_TEST maps to "TEST"
  - D_TEST_TYPE_TEST_BLOCK maps to "TEST_BLOCK"
  - D_TEST_TYPE_MODULE maps to "MODULE"
  - D_TEST_TYPE_UNKNOWN maps to "UNKNOWN"
  - all returned strings are non-NULL
*/
bool
d_tests_sa_test_type_flag_to_string
(
    struct d_test_counter* _counter
)
{
    const char* str;
    bool        result;

    result = true;

    // test ASSERT
    str = d_test_type_flag_to_string(D_TEST_TYPE_ASSERT);

    result = d_assert_standalone(str != NULL,
                                 "d_test_type_flag_to_string",
                                 "ASSERT returns non-NULL string",
                                 _counter) && result;

    result = d_assert_standalone(
                 d_strequals(str, d_strnlen(str, 64),
                             "ASSERTION", 9),
                 "d_test_type_flag_to_string",
                 "D_TEST_TYPE_ASSERT maps to \"ASSERTION\"",
                 _counter) && result;

    // test TEST_FN
    str = d_test_type_flag_to_string(D_TEST_TYPE_TEST_FN);

    result = d_assert_standalone(
                 d_strequals(str, d_strnlen(str, 64),
                             "TEST_FN", 7),
                 "d_test_type_flag_to_string",
                 "D_TEST_TYPE_TEST_FN maps to \"TEST_FN\"",
                 _counter) && result;

    // test TEST
    str = d_test_type_flag_to_string(D_TEST_TYPE_TEST);

    result = d_assert_standalone(
                 d_strequals(str, d_strnlen(str, 64),
                             "TEST", 4),
                 "d_test_type_flag_to_string",
                 "D_TEST_TYPE_TEST maps to \"TEST\"",
                 _counter) && result;

    // test TEST_BLOCK
    str = d_test_type_flag_to_string(D_TEST_TYPE_TEST_BLOCK);

    result = d_assert_standalone(
                 d_strequals(str, d_strnlen(str, 64),
                             "TEST_BLOCK", 10),
                 "d_test_type_flag_to_string",
                 "D_TEST_TYPE_TEST_BLOCK maps to \"TEST_BLOCK\"",
                 _counter) && result;

    // test MODULE
    str = d_test_type_flag_to_string(D_TEST_TYPE_MODULE);

    result = d_assert_standalone(
                 d_strequals(str, d_strnlen(str, 64),
                             "MODULE", 6),
                 "d_test_type_flag_to_string",
                 "D_TEST_TYPE_MODULE maps to \"MODULE\"",
                 _counter) && result;

    // test UNKNOWN
    str = d_test_type_flag_to_string(D_TEST_TYPE_UNKNOWN);

    result = d_assert_standalone(
                 d_strequals(str, d_strnlen(str, 64),
                             "UNKNOWN", 7),
                 "d_test_type_flag_to_string",
                 "D_TEST_TYPE_UNKNOWN maps to \"UNKNOWN\"",
                 _counter) && result;

    return result;
}


/*
d_tests_sa_test_type_flag_unknown
  Tests d_test_type_flag_to_string with out-of-range value.
  Tests the following:
  - invalid/unknown enum value falls through to "UNKNOWN"
  - returned string is non-NULL
*/
bool
d_tests_sa_test_type_flag_unknown
(
    struct d_test_counter* _counter
)
{
    const char* str;
    bool        result;

    result = true;

    // test with a value outside the enum range
    str = d_test_type_flag_to_string((enum DTestTypeFlag)999);

    result = d_assert_standalone(str != NULL,
                                 "d_test_type_flag_unknown",
                                 "out-of-range value returns non-NULL",
                                 _counter) && result;

    result = d_assert_standalone(
                 d_strequals(str, d_strnlen(str, 64),
                             "UNKNOWN", 7),
                 "d_test_type_flag_unknown",
                 "out-of-range value maps to \"UNKNOWN\"",
                 _counter) && result;

    // test with negative-like cast
    str = d_test_type_flag_to_string((enum DTestTypeFlag)-1);

    result = d_assert_standalone(
                 d_strequals(str, d_strnlen(str, 64),
                             "UNKNOWN", 7),
                 "d_test_type_flag_unknown",
                 "negative cast value maps to \"UNKNOWN\"",
                 _counter) && result;

    return result;
}


/*
d_tests_sa_test_print_null
  Tests d_test_print with NULL test (should not crash).
  Tests the following:
  - d_test_print(NULL, NULL, 0) does not crash
  - d_test_print(NULL, "prefix", 6) does not crash
*/
bool
d_tests_sa_test_print_null
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // these should not crash -- we verify by reaching the assertion
    d_test_print(NULL, NULL, 0);
    d_test_print(NULL, "  ", 2);

    result = d_assert_standalone(true,
                                 "d_test_print_null",
                                 "d_test_print(NULL, ...) does not crash",
                                 _counter) && result;

    return result;
}


/*
d_tests_sa_test_print_valid
  Tests d_test_print with a valid test.
  Tests the following:
  - d_test_print with valid test does not crash
  - d_test_print with NULL prefix does not crash
  - d_test_print with prefix string does not crash
*/
bool
d_tests_sa_test_print_valid
(
    struct d_test_counter* _counter
)
{
    struct d_test* test;
    bool           result;

    result = true;

    test = d_test_new(NULL, 0);

    if (!test)
    {
        return d_assert_standalone(false,
                                   "d_test_print_valid",
                                   "setup: d_test_new failed",
                                   _counter);
    }

    // call with various prefix configurations -- verify no crash
    d_test_print(test, NULL, 0);
    d_test_print(test, "", 0);
    d_test_print(test, "  ", 2);
    d_test_print(test, "    ", 4);

    result = d_assert_standalone(true,
                                 "d_test_print_valid",
                                 "d_test_print with valid test does not crash",
                                 _counter) && result;

    d_test_free(test);

    return result;
}


/*
d_tests_sa_test_utility_all
  Aggregation function for all utility tests.
*/
bool
d_tests_sa_test_utility_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    result = d_tests_sa_test_type_flag_to_string(_counter) && result;
    result = d_tests_sa_test_type_flag_unknown(_counter) && result;
    result = d_tests_sa_test_print_null(_counter) && result;
    result = d_tests_sa_test_print_valid(_counter) && result;

    return result;
}
