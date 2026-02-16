#include ".\test_standalone_tests_sa.h"


/******************************************************************************
 * II. CONSTANT MACRO TESTS
 *****************************************************************************/

/*
d_tests_sa_standalone_object_constants
  Tests the D_TEST_OBJECT_LEAF and D_TEST_OBJECT_INTERIOR constants.
  Tests the following:
  - D_TEST_OBJECT_LEAF is defined and equals true
  - D_TEST_OBJECT_INTERIOR is defined and equals false
  - Constants are distinct
*/
bool
d_tests_sa_standalone_object_constants
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: D_TEST_OBJECT_LEAF is defined
    #ifdef D_TEST_OBJECT_LEAF
        result = d_assert_standalone(
            true,
            "object_leaf_defined",
            "D_TEST_OBJECT_LEAF should be defined",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "object_leaf_defined",
            "D_TEST_OBJECT_LEAF should be defined",
            _counter) && result;
    #endif

    // test 2: D_TEST_OBJECT_LEAF equals true
    result = d_assert_standalone(
        D_TEST_OBJECT_LEAF == true,
        "object_leaf_is_true",
        "D_TEST_OBJECT_LEAF should equal true",
        _counter) && result;

    // test 3: D_TEST_OBJECT_INTERIOR is defined
    #ifdef D_TEST_OBJECT_INTERIOR
        result = d_assert_standalone(
            true,
            "object_interior_defined",
            "D_TEST_OBJECT_INTERIOR should be defined",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "object_interior_defined",
            "D_TEST_OBJECT_INTERIOR should be defined",
            _counter) && result;
    #endif

    // test 4: D_TEST_OBJECT_INTERIOR equals false
    result = d_assert_standalone(
        D_TEST_OBJECT_INTERIOR == false,
        "object_interior_is_false",
        "D_TEST_OBJECT_INTERIOR should equal false",
        _counter) && result;

    // test 5: constants are distinct
    result = d_assert_standalone(
        D_TEST_OBJECT_LEAF != D_TEST_OBJECT_INTERIOR,
        "object_constants_distinct",
        "LEAF and INTERIOR should be different",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_line_width
  Tests the D_TEST_SA_LINE_WIDTH constant.
  Tests the following:
  - Macro is defined
  - Value is 80 (standard terminal width)
*/
bool
d_tests_sa_standalone_line_width
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: D_TEST_SA_LINE_WIDTH is defined
    #ifdef D_TEST_SA_LINE_WIDTH
        result = d_assert_standalone(
            true,
            "line_width_defined",
            "D_TEST_SA_LINE_WIDTH should be defined",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "line_width_defined",
            "D_TEST_SA_LINE_WIDTH should be defined",
            _counter) && result;
    #endif

    // test 2: value is 80
    result = d_assert_standalone(
        D_TEST_SA_LINE_WIDTH == 80,
        "line_width_value",
        "D_TEST_SA_LINE_WIDTH should be 80",
        _counter) && result;

    // test 3: value is positive
    result = d_assert_standalone(
        D_TEST_SA_LINE_WIDTH > 0,
        "line_width_positive",
        "D_TEST_SA_LINE_WIDTH should be positive",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_separators
  Tests the separator string constants.
  Tests the following:
  - D_TEST_SA_SEPARATOR_DOUBLE is defined and has correct length
  - D_TEST_SA_SEPARATOR_SINGLE is defined and has correct length
  - Separators are distinct
  - Separators contain expected characters
*/
bool
d_tests_sa_standalone_separators
(
    struct d_test_counter* _counter
)
{
    bool        result;
    const char* double_sep;
    const char* single_sep;
    size_t      double_len;
    size_t      single_len;

    result = true;

    // test 1: D_TEST_SA_SEPARATOR_DOUBLE is defined
    #ifdef D_TEST_SA_SEPARATOR_DOUBLE
        result = d_assert_standalone(
            true,
            "separator_double_defined",
            "D_TEST_SA_SEPARATOR_DOUBLE should be defined",
            _counter) && result;
        double_sep = D_TEST_SA_SEPARATOR_DOUBLE;
    #else
        result = d_assert_standalone(
            false,
            "separator_double_defined",
            "D_TEST_SA_SEPARATOR_DOUBLE should be defined",
            _counter) && result;
        double_sep = NULL;
    #endif

    // test 2: D_TEST_SA_SEPARATOR_SINGLE is defined
    #ifdef D_TEST_SA_SEPARATOR_SINGLE
        result = d_assert_standalone(
            true,
            "separator_single_defined",
            "D_TEST_SA_SEPARATOR_SINGLE should be defined",
            _counter) && result;
        single_sep = D_TEST_SA_SEPARATOR_SINGLE;
    #else
        result = d_assert_standalone(
            false,
            "separator_single_defined",
            "D_TEST_SA_SEPARATOR_SINGLE should be defined",
            _counter) && result;
        single_sep = NULL;
    #endif

    // test 3: double separator has correct length (80 chars)
    if (double_sep)
    {
        double_len = d_strnlen(double_sep, 100);

        result = d_assert_standalone(
            double_len == D_TEST_SA_LINE_WIDTH,
            "separator_double_length",
            "SEPARATOR_DOUBLE should be LINE_WIDTH characters",
            _counter) && result;

        // test 4: double separator contains '=' characters
        result = d_assert_standalone(
            double_sep[0] == '=',
            "separator_double_char",
            "SEPARATOR_DOUBLE should contain '=' characters",
            _counter) && result;
    }

    // test 5: single separator has correct length (80 chars)
    if (single_sep)
    {
        single_len = d_strnlen(single_sep, 100);

        result = d_assert_standalone(
            single_len == D_TEST_SA_LINE_WIDTH,
            "separator_single_length",
            "SEPARATOR_SINGLE should be LINE_WIDTH characters",
            _counter) && result;

        // test 6: single separator contains '-' characters
        result = d_assert_standalone(
            single_sep[0] == '-',
            "separator_single_char",
            "SEPARATOR_SINGLE should contain '-' characters",
            _counter) && result;
    }

    // test 7: separators are distinct
    if (double_sep && single_sep)
    {
        result = d_assert_standalone(
            double_sep[0] != single_sep[0],
            "separators_distinct",
            "Double and single separators should be different",
            _counter) && result;
    }

    return result;
}


/*
d_tests_sa_standalone_max_modules
  Tests the D_TEST_SA_MAX_MODULES constant.
  Tests the following:
  - Macro is defined
  - Value is 64
  - Value is reasonable for array sizing
*/
bool
d_tests_sa_standalone_max_modules
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: D_TEST_SA_MAX_MODULES is defined
    #ifdef D_TEST_SA_MAX_MODULES
        result = d_assert_standalone(
            true,
            "max_modules_defined",
            "D_TEST_SA_MAX_MODULES should be defined",
            _counter) && result;
    #else
        result = d_assert_standalone(
            false,
            "max_modules_defined",
            "D_TEST_SA_MAX_MODULES should be defined",
            _counter) && result;
    #endif

    // test 2: value is 64
    result = d_assert_standalone(
        D_TEST_SA_MAX_MODULES == 64,
        "max_modules_value",
        "D_TEST_SA_MAX_MODULES should be 64",
        _counter) && result;

    // test 3: value is positive
    result = d_assert_standalone(
        D_TEST_SA_MAX_MODULES > 0,
        "max_modules_positive",
        "D_TEST_SA_MAX_MODULES should be positive",
        _counter) && result;

    // test 4: value is reasonable (not too large)
    result = d_assert_standalone(
        D_TEST_SA_MAX_MODULES <= 1024,
        "max_modules_reasonable",
        "D_TEST_SA_MAX_MODULES should be <= 1024",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_constant_all
  Aggregation function that runs all constant macro tests.
*/
bool
d_tests_sa_standalone_constant_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Constant Macros\n");
    printf("  --------------------------\n");

    result = d_tests_sa_standalone_object_constants(_counter) && result;
    result = d_tests_sa_standalone_line_width(_counter) && result;
    result = d_tests_sa_standalone_separators(_counter) && result;
    result = d_tests_sa_standalone_max_modules(_counter) && result;

    return result;
}
