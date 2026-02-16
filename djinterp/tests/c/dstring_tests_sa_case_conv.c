#include ".\dstring_tests_sa.h"


/******************************************************************************
 * SECTION 10: CASE CONVERSION FUNCTIONS
 *****************************************************************************/

/*
d_tests_sa_dstring_to_lower
  Tests d_string_to_lower function (in-place conversion).
  Tests the following:
  - converting uppercase string to lowercase
  - converting mixed case string to lowercase
  - converting already lowercase string
  - string with numbers and symbols unchanged
  - empty string handling
  - NULL string handling
*/
struct d_test_object*
d_tests_sa_dstring_to_lower
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_to_lower", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: converting uppercase string to lowercase
    str = d_string_new_from_cstr("HELLO WORLD");

    if (str)
    {
        result = d_string_to_lower(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "to_lower_uppercase",
            result && d_string_equals_cstr(str, "hello world"),
            "should convert 'HELLO WORLD' to 'hello world'");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "to_lower_uppercase",
            false,
            "failed to allocate test string");
    }

    // test: converting mixed case string to lowercase
    str = d_string_new_from_cstr("HeLLo WoRLd");

    if (str)
    {
        result = d_string_to_lower(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "to_lower_mixed_case",
            result && d_string_equals_cstr(str, "hello world"),
            "should convert mixed case to lowercase");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "to_lower_mixed_case",
            false,
            "failed to allocate test string");
    }

    // test: converting already lowercase string
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_to_lower(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "to_lower_already_lower",
            result && d_string_equals_cstr(str, "hello world"),
            "should remain unchanged");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "to_lower_already_lower",
            false,
            "failed to allocate test string");
    }

    // test: string with numbers and symbols unchanged
    str = d_string_new_from_cstr("ABC123!@#XYZ");

    if (str)
    {
        result = d_string_to_lower(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "to_lower_with_numbers_symbols",
            result && d_string_equals_cstr(str, "abc123!@#xyz"),
            "should only convert letters, preserve numbers/symbols");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "to_lower_with_numbers_symbols",
            false,
            "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result = d_string_to_lower(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "to_lower_empty_string",
            result && d_string_is_empty(str),
            "should handle empty string");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "to_lower_empty_string",
            false,
            "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_to_lower(NULL);
    group->elements[idx++] = D_ASSERT_FALSE(
        "to_lower_null_string",
        result,
        "should return false for NULL string");

    return group;
}

/*
d_tests_sa_dstring_to_upper
  Tests d_string_to_upper function (in-place conversion).
  Tests the following:
  - converting lowercase string to uppercase
  - converting mixed case string to uppercase
  - converting already uppercase string
  - string with numbers and symbols unchanged
  - empty string handling
  - NULL string handling
*/
struct d_test_object*
d_tests_sa_dstring_to_upper
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_to_upper", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: converting lowercase string to uppercase
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result = d_string_to_upper(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "to_upper_lowercase",
            result && d_string_equals_cstr(str, "HELLO WORLD"),
            "should convert 'hello world' to 'HELLO WORLD'");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "to_upper_lowercase",
            false,
            "failed to allocate test string");
    }

    // test: converting mixed case string to uppercase
    str = d_string_new_from_cstr("HeLLo WoRLd");

    if (str)
    {
        result = d_string_to_upper(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "to_upper_mixed_case",
            result && d_string_equals_cstr(str, "HELLO WORLD"),
            "should convert mixed case to uppercase");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "to_upper_mixed_case",
            false,
            "failed to allocate test string");
    }

    // test: converting already uppercase string
    str = d_string_new_from_cstr("HELLO WORLD");

    if (str)
    {
        result = d_string_to_upper(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "to_upper_already_upper",
            result && d_string_equals_cstr(str, "HELLO WORLD"),
            "should remain unchanged");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "to_upper_already_upper",
            false,
            "failed to allocate test string");
    }

    // test: string with numbers and symbols unchanged
    str = d_string_new_from_cstr("abc123!@#xyz");

    if (str)
    {
        result = d_string_to_upper(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "to_upper_with_numbers_symbols",
            result && d_string_equals_cstr(str, "ABC123!@#XYZ"),
            "should only convert letters, preserve numbers/symbols");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "to_upper_with_numbers_symbols",
            false,
            "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result = d_string_to_upper(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "to_upper_empty_string",
            result && d_string_is_empty(str),
            "should handle empty string");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "to_upper_empty_string",
            false,
            "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_to_upper(NULL);
    group->elements[idx++] = D_ASSERT_FALSE(
        "to_upper_null_string",
        result,
        "should return false for NULL string");

    return group;
}

/*
d_tests_sa_dstring_lower
  Tests d_string_lower function (returns new string).
  Tests the following:
  - creating lowercase copy from uppercase string
  - creating lowercase copy from mixed case string
  - creating lowercase copy from already lowercase string
  - original string unchanged
  - empty string handling
  - NULL string handling
*/
struct d_test_object*
d_tests_sa_dstring_lower
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    struct d_string*      result_str;
    size_t                idx;

    group = d_test_object_new_interior("d_string_lower", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: creating lowercase copy from uppercase string
    str = d_string_new_from_cstr("HELLO WORLD");

    if (str)
    {
        result_str = d_string_lower(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "lower_from_uppercase",
            result_str && d_string_equals_cstr(result_str, "hello world"),
            "should create lowercase copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "lower_from_uppercase",
            false,
            "failed to allocate test string");
    }

    // test: creating lowercase copy from mixed case string
    str = d_string_new_from_cstr("HeLLo WoRLd");

    if (str)
    {
        result_str = d_string_lower(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "lower_from_mixed_case",
            result_str && d_string_equals_cstr(result_str, "hello world"),
            "should create lowercase copy from mixed case");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "lower_from_mixed_case",
            false,
            "failed to allocate test string");
    }

    // test: creating lowercase copy from already lowercase string
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result_str = d_string_lower(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "lower_from_lowercase",
            result_str && d_string_equals_cstr(result_str, "hello world"),
            "should create identical copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "lower_from_lowercase",
            false,
            "failed to allocate test string");
    }

    // test: original string unchanged
    str = d_string_new_from_cstr("HELLO");

    if (str)
    {
        result_str = d_string_lower(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "lower_original_unchanged",
            d_string_equals_cstr(str, "HELLO"),
            "original should remain unchanged");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "lower_original_unchanged",
            false,
            "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result_str = d_string_lower(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "lower_empty_string",
            result_str && d_string_is_empty(result_str),
            "should create empty copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "lower_empty_string",
            false,
            "failed to allocate test string");
    }

    // test: NULL string handling
    result_str = d_string_lower(NULL);
    group->elements[idx++] = D_ASSERT_NULL(
        "lower_null_string",
        result_str,
        "should return NULL for NULL string");

    return group;
}

/*
d_tests_sa_dstring_upper
  Tests d_string_upper function (returns new string).
  Tests the following:
  - creating uppercase copy from lowercase string
  - creating uppercase copy from mixed case string
  - creating uppercase copy from already uppercase string
  - original string unchanged
  - empty string handling
  - NULL string handling
*/
struct d_test_object*
d_tests_sa_dstring_upper
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    struct d_string*      result_str;
    size_t                idx;

    group = d_test_object_new_interior("d_string_upper", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: creating uppercase copy from lowercase string
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        result_str = d_string_upper(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "upper_from_lowercase",
            result_str && d_string_equals_cstr(result_str, "HELLO WORLD"),
            "should create uppercase copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "upper_from_lowercase",
            false,
            "failed to allocate test string");
    }

    // test: creating uppercase copy from mixed case string
    str = d_string_new_from_cstr("HeLLo WoRLd");

    if (str)
    {
        result_str = d_string_upper(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "upper_from_mixed_case",
            result_str && d_string_equals_cstr(result_str, "HELLO WORLD"),
            "should create uppercase copy from mixed case");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "upper_from_mixed_case",
            false,
            "failed to allocate test string");
    }

    // test: creating uppercase copy from already uppercase string
    str = d_string_new_from_cstr("HELLO WORLD");

    if (str)
    {
        result_str = d_string_upper(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "upper_from_uppercase",
            result_str && d_string_equals_cstr(result_str, "HELLO WORLD"),
            "should create identical copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "upper_from_uppercase",
            false,
            "failed to allocate test string");
    }

    // test: original string unchanged
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result_str = d_string_upper(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "upper_original_unchanged",
            d_string_equals_cstr(str, "hello"),
            "original should remain unchanged");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "upper_original_unchanged",
            false,
            "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result_str = d_string_upper(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "upper_empty_string",
            result_str && d_string_is_empty(result_str),
            "should create empty copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "upper_empty_string",
            false,
            "failed to allocate test string");
    }

    // test: NULL string handling
    result_str = d_string_upper(NULL);
    group->elements[idx++] = D_ASSERT_NULL(
        "upper_null_string",
        result_str,
        "should return NULL for NULL string");

    return group;
}


/******************************************************************************
 * CASE CONVERSION ALL - AGGREGATE RUNNER
 *****************************************************************************/

/*
d_tests_sa_dstring_case_all
  Runs all case conversion function tests for dstring module.
  Tests the following:
  - in-place conversion functions (to_lower, to_upper)
  - non-modifying functions (lower, upper)
*/
struct d_test_object*
d_tests_sa_dstring_case_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Case Conversion Functions", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // in-place conversion tests
    group->elements[idx++] = d_tests_sa_dstring_to_lower();
    group->elements[idx++] = d_tests_sa_dstring_to_upper();

    // non-modifying conversion tests
    group->elements[idx++] = d_tests_sa_dstring_lower();
    group->elements[idx++] = d_tests_sa_dstring_upper();

    return group;
}
