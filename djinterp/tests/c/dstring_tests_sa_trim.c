#include ".\dstring_tests_sa.h"


/******************************************************************************
 * SECTION 12: TRIMMING FUNCTIONS
 *****************************************************************************/

/******************************************************************************
 * I. IN-PLACE TRIMMING TESTS
 *****************************************************************************/

/*
d_tests_sa_dstring_trim
  Tests d_string_trim function (in-place, both sides).
  Tests the following:
  - trimming whitespace from both sides
  - trimming only leading whitespace
  - trimming only trailing whitespace
  - string with no whitespace to trim
  - string that is all whitespace
  - empty string handling
  - NULL string handling
*/
struct d_test_object*
d_tests_sa_dstring_trim
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_trim", 7);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: trimming whitespace from both sides
    str = d_string_new_from_cstr("  hello world  ");

    if (str)
    {
        result = d_string_trim(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_both_sides",
            result && d_string_equals_cstr(str, "hello world"),
            "should trim whitespace from both sides");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_both_sides",
            false,
            "failed to allocate test string");
    }

    // test: trimming only leading whitespace
    str = d_string_new_from_cstr("   hello");

    if (str)
    {
        result = d_string_trim(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_leading_only",
            result && d_string_equals_cstr(str, "hello"),
            "should trim leading whitespace");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_leading_only",
            false,
            "failed to allocate test string");
    }

    // test: trimming only trailing whitespace
    str = d_string_new_from_cstr("hello   ");

    if (str)
    {
        result = d_string_trim(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_trailing_only",
            result && d_string_equals_cstr(str, "hello"),
            "should trim trailing whitespace");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_trailing_only",
            false,
            "failed to allocate test string");
    }

    // test: string with no whitespace to trim
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_trim(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_no_whitespace",
            result && d_string_equals_cstr(str, "hello"),
            "should remain unchanged");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_no_whitespace",
            false,
            "failed to allocate test string");
    }

    // test: string that is all whitespace
    str = d_string_new_from_cstr("     ");

    if (str)
    {
        result = d_string_trim(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_all_whitespace",
            result && d_string_is_empty(str),
            "should result in empty string");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_all_whitespace",
            false,
            "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result = d_string_trim(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_empty_string",
            result && d_string_is_empty(str),
            "should remain empty");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_empty_string",
            false,
            "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_trim(NULL);
    group->elements[idx++] = D_ASSERT_FALSE(
        "trim_null_string",
        result,
        "should return false for NULL string");

    return group;
}

/*
d_tests_sa_dstring_trim_left
  Tests d_string_trim_left function (in-place, left side only).
  Tests the following:
  - trimming leading whitespace
  - string with no leading whitespace
  - string with only trailing whitespace
  - string that is all whitespace
  - empty string handling
  - NULL string handling
*/
struct d_test_object*
d_tests_sa_dstring_trim_left
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_trim_left", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: trimming leading whitespace
    str = d_string_new_from_cstr("   hello world");

    if (str)
    {
        result = d_string_trim_left(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_left_leading",
            result && d_string_equals_cstr(str, "hello world"),
            "should trim leading whitespace");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_left_leading",
            false,
            "failed to allocate test string");
    }

    // test: string with no leading whitespace
    str = d_string_new_from_cstr("hello   ");

    if (str)
    {
        result = d_string_trim_left(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_left_no_leading",
            result && d_string_equals_cstr(str, "hello   "),
            "should remain unchanged");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_left_no_leading",
            false,
            "failed to allocate test string");
    }

    // test: string with both leading and trailing whitespace
    str = d_string_new_from_cstr("  hello  ");

    if (str)
    {
        result = d_string_trim_left(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_left_preserves_trailing",
            result && d_string_equals_cstr(str, "hello  "),
            "should only trim leading whitespace");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_left_preserves_trailing",
            false,
            "failed to allocate test string");
    }

    // test: string that is all whitespace
    str = d_string_new_from_cstr("     ");

    if (str)
    {
        result = d_string_trim_left(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_left_all_whitespace",
            result && d_string_is_empty(str),
            "should result in empty string");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_left_all_whitespace",
            false,
            "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result = d_string_trim_left(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_left_empty_string",
            result && d_string_is_empty(str),
            "should remain empty");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_left_empty_string",
            false,
            "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_trim_left(NULL);
    group->elements[idx++] = D_ASSERT_FALSE(
        "trim_left_null_string",
        result,
        "should return false for NULL string");

    return group;
}

/*
d_tests_sa_dstring_trim_right
  Tests d_string_trim_right function (in-place, right side only).
  Tests the following:
  - trimming trailing whitespace
  - string with no trailing whitespace
  - string with only leading whitespace
  - string that is all whitespace
  - empty string handling
  - NULL string handling
*/
struct d_test_object*
d_tests_sa_dstring_trim_right
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_trim_right", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: trimming trailing whitespace
    str = d_string_new_from_cstr("hello world   ");

    if (str)
    {
        result = d_string_trim_right(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_right_trailing",
            result && d_string_equals_cstr(str, "hello world"),
            "should trim trailing whitespace");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_right_trailing",
            false,
            "failed to allocate test string");
    }

    // test: string with no trailing whitespace
    str = d_string_new_from_cstr("   hello");

    if (str)
    {
        result = d_string_trim_right(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_right_no_trailing",
            result && d_string_equals_cstr(str, "   hello"),
            "should remain unchanged");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_right_no_trailing",
            false,
            "failed to allocate test string");
    }

    // test: string with both leading and trailing whitespace
    str = d_string_new_from_cstr("  hello  ");

    if (str)
    {
        result = d_string_trim_right(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_right_preserves_leading",
            result && d_string_equals_cstr(str, "  hello"),
            "should only trim trailing whitespace");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_right_preserves_leading",
            false,
            "failed to allocate test string");
    }

    // test: string that is all whitespace
    str = d_string_new_from_cstr("     ");

    if (str)
    {
        result = d_string_trim_right(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_right_all_whitespace",
            result && d_string_is_empty(str),
            "should result in empty string");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_right_all_whitespace",
            false,
            "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result = d_string_trim_right(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_right_empty_string",
            result && d_string_is_empty(str),
            "should remain empty");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_right_empty_string",
            false,
            "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_trim_right(NULL);
    group->elements[idx++] = D_ASSERT_FALSE(
        "trim_right_null_string",
        result,
        "should return false for NULL string");

    return group;
}

/*
d_tests_sa_dstring_trim_chars
  Tests d_string_trim_chars function (in-place, custom characters).
  Tests the following:
  - trimming custom characters from both sides
  - trimming single custom character
  - trimming multiple custom characters
  - no characters to trim
  - string that is all trim characters
  - empty string handling
  - NULL string handling
  - NULL chars handling
*/
struct d_test_object*
d_tests_sa_dstring_trim_chars
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    bool                  result;
    size_t                idx;

    group = d_test_object_new_interior("d_string_trim_chars", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: trimming custom characters from both sides
    str = d_string_new_from_cstr("---hello world---");

    if (str)
    {
        result = d_string_trim_chars(str, "-");
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_chars_both_sides",
            result && d_string_equals_cstr(str, "hello world"),
            "should trim dashes from both sides");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_chars_both_sides",
            false,
            "failed to allocate test string");
    }

    // test: trimming single custom character
    str = d_string_new_from_cstr("xxxhelloxxx");

    if (str)
    {
        result = d_string_trim_chars(str, "x");
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_chars_single_char",
            result && d_string_equals_cstr(str, "hello"),
            "should trim 'x' from both sides");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_chars_single_char",
            false,
            "failed to allocate test string");
    }

    // test: trimming multiple custom characters
    str = d_string_new_from_cstr(".-.-hello-.-.");

    if (str)
    {
        result = d_string_trim_chars(str, ".-");
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_chars_multiple_chars",
            result && d_string_equals_cstr(str, "hello"),
            "should trim '.' and '-' from both sides");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_chars_multiple_chars",
            false,
            "failed to allocate test string");
    }

    // test: no characters to trim
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result = d_string_trim_chars(str, "-");
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_chars_no_match",
            result && d_string_equals_cstr(str, "hello"),
            "should remain unchanged");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_chars_no_match",
            false,
            "failed to allocate test string");
    }

    // test: string that is all trim characters
    str = d_string_new_from_cstr("-----");

    if (str)
    {
        result = d_string_trim_chars(str, "-");
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_chars_all_trim",
            result && d_string_is_empty(str),
            "should result in empty string");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_chars_all_trim",
            false,
            "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result = d_string_trim_chars(str, "-");
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_chars_empty_string",
            result && d_string_is_empty(str),
            "should remain empty");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trim_chars_empty_string",
            false,
            "failed to allocate test string");
    }

    // test: NULL string handling
    result = d_string_trim_chars(NULL, "-");
    group->elements[idx++] = D_ASSERT_FALSE(
        "trim_chars_null_string",
        result,
        "should return false for NULL string");

    // test: NULL chars handling
    str    = d_string_new_from_cstr("hello");
    result = d_string_trim_chars(str, NULL);
    group->elements[idx++] = D_ASSERT_FALSE(
        "trim_chars_null_chars",
        result,
        "should return false for NULL chars");

    d_string_free(str);

    return group;
}


/******************************************************************************
 * II. NON-MODIFYING TRIMMING TESTS (RETURN NEW STRING)
 *****************************************************************************/

/*
d_tests_sa_dstring_trimmed
  Tests d_string_trimmed function (returns new string, both sides).
  Tests the following:
  - creating trimmed copy from string with whitespace
  - original string unchanged
  - string with no whitespace to trim
  - string that is all whitespace
  - empty string handling
  - NULL string handling
*/
struct d_test_object*
d_tests_sa_dstring_trimmed
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    struct d_string*      result_str;
    size_t                idx;

    group = d_test_object_new_interior("d_string_trimmed", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: creating trimmed copy from string with whitespace
    str = d_string_new_from_cstr("  hello world  ");

    if (str)
    {
        result_str = d_string_trimmed(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_both_sides",
            result_str && d_string_equals_cstr(result_str, "hello world"),
            "should create trimmed copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_both_sides",
            false,
            "failed to allocate test string");
    }

    // test: original string unchanged
    str = d_string_new_from_cstr("  hello  ");

    if (str)
    {
        result_str = d_string_trimmed(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_original_unchanged",
            d_string_equals_cstr(str, "  hello  "),
            "original should remain unchanged");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_original_unchanged",
            false,
            "failed to allocate test string");
    }

    // test: string with no whitespace to trim
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        result_str = d_string_trimmed(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_no_whitespace",
            result_str && d_string_equals_cstr(result_str, "hello"),
            "should create identical copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_no_whitespace",
            false,
            "failed to allocate test string");
    }

    // test: string that is all whitespace
    str = d_string_new_from_cstr("     ");

    if (str)
    {
        result_str = d_string_trimmed(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_all_whitespace",
            result_str && d_string_is_empty(result_str),
            "should create empty string");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_all_whitespace",
            false,
            "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result_str = d_string_trimmed(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_empty_string",
            result_str && d_string_is_empty(result_str),
            "should create empty copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_empty_string",
            false,
            "failed to allocate test string");
    }

    // test: NULL string handling
    result_str = d_string_trimmed(NULL);
    group->elements[idx++] = D_ASSERT_NULL(
        "trimmed_null_string",
        result_str,
        "should return NULL for NULL string");

    return group;
}

/*
d_tests_sa_dstring_trimmed_left
  Tests d_string_trimmed_left function (returns new string, left side only).
  Tests the following:
  - creating left-trimmed copy
  - original string unchanged
  - string with no leading whitespace
  - string that is all whitespace
  - empty string handling
  - NULL string handling
*/
struct d_test_object*
d_tests_sa_dstring_trimmed_left
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    struct d_string*      result_str;
    size_t                idx;

    group = d_test_object_new_interior("d_string_trimmed_left", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: creating left-trimmed copy
    str = d_string_new_from_cstr("   hello world  ");

    if (str)
    {
        result_str = d_string_trimmed_left(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_left_leading",
            result_str && d_string_equals_cstr(result_str, "hello world  "),
            "should create left-trimmed copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_left_leading",
            false,
            "failed to allocate test string");
    }

    // test: original string unchanged
    str = d_string_new_from_cstr("  hello");

    if (str)
    {
        result_str = d_string_trimmed_left(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_left_original_unchanged",
            d_string_equals_cstr(str, "  hello"),
            "original should remain unchanged");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_left_original_unchanged",
            false,
            "failed to allocate test string");
    }

    // test: string with no leading whitespace
    str = d_string_new_from_cstr("hello   ");

    if (str)
    {
        result_str = d_string_trimmed_left(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_left_no_leading",
            result_str && d_string_equals_cstr(result_str, "hello   "),
            "should create identical copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_left_no_leading",
            false,
            "failed to allocate test string");
    }

    // test: string that is all whitespace
    str = d_string_new_from_cstr("     ");

    if (str)
    {
        result_str = d_string_trimmed_left(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_left_all_whitespace",
            result_str && d_string_is_empty(result_str),
            "should create empty string");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_left_all_whitespace",
            false,
            "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result_str = d_string_trimmed_left(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_left_empty_string",
            result_str && d_string_is_empty(result_str),
            "should create empty copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_left_empty_string",
            false,
            "failed to allocate test string");
    }

    // test: NULL string handling
    result_str = d_string_trimmed_left(NULL);
    group->elements[idx++] = D_ASSERT_NULL(
        "trimmed_left_null_string",
        result_str,
        "should return NULL for NULL string");

    return group;
}

/*
d_tests_sa_dstring_trimmed_right
  Tests d_string_trimmed_right function (returns new string, right side only).
  Tests the following:
  - creating right-trimmed copy
  - original string unchanged
  - string with no trailing whitespace
  - string that is all whitespace
  - empty string handling
  - NULL string handling
*/
struct d_test_object*
d_tests_sa_dstring_trimmed_right
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    struct d_string*      result_str;
    size_t                idx;

    group = d_test_object_new_interior("d_string_trimmed_right", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: creating right-trimmed copy
    str = d_string_new_from_cstr("  hello world   ");

    if (str)
    {
        result_str = d_string_trimmed_right(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_right_trailing",
            result_str && d_string_equals_cstr(result_str, "  hello world"),
            "should create right-trimmed copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_right_trailing",
            false,
            "failed to allocate test string");
    }

    // test: original string unchanged
    str = d_string_new_from_cstr("hello  ");

    if (str)
    {
        result_str = d_string_trimmed_right(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_right_original_unchanged",
            d_string_equals_cstr(str, "hello  "),
            "original should remain unchanged");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_right_original_unchanged",
            false,
            "failed to allocate test string");
    }

    // test: string with no trailing whitespace
    str = d_string_new_from_cstr("   hello");

    if (str)
    {
        result_str = d_string_trimmed_right(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_right_no_trailing",
            result_str && d_string_equals_cstr(result_str, "   hello"),
            "should create identical copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_right_no_trailing",
            false,
            "failed to allocate test string");
    }

    // test: string that is all whitespace
    str = d_string_new_from_cstr("     ");

    if (str)
    {
        result_str = d_string_trimmed_right(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_right_all_whitespace",
            result_str && d_string_is_empty(result_str),
            "should create empty string");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_right_all_whitespace",
            false,
            "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        result_str = d_string_trimmed_right(str);
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_right_empty_string",
            result_str && d_string_is_empty(result_str),
            "should create empty copy");

        d_string_free(result_str);
        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "trimmed_right_empty_string",
            false,
            "failed to allocate test string");
    }

    // test: NULL string handling
    result_str = d_string_trimmed_right(NULL);
    group->elements[idx++] = D_ASSERT_NULL(
        "trimmed_right_null_string",
        result_str,
        "should return NULL for NULL string");

    return group;
}


/******************************************************************************
 * TRIM ALL - AGGREGATE RUNNER
 *****************************************************************************/

/*
d_tests_sa_dstring_trim_all
  Runs all trimming function tests for dstring module.
  Tests the following:
  - in-place trimming functions (trim, trim_left, trim_right, trim_chars)
  - non-modifying trimming functions (trimmed, trimmed_left, trimmed_right)
*/
struct d_test_object*
d_tests_sa_dstring_trim_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Trimming Functions", 7);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // in-place trimming tests
    group->elements[idx++] = d_tests_sa_dstring_trim();
    group->elements[idx++] = d_tests_sa_dstring_trim_left();
    group->elements[idx++] = d_tests_sa_dstring_trim_right();
    group->elements[idx++] = d_tests_sa_dstring_trim_chars();

    // non-modifying trimming tests
    group->elements[idx++] = d_tests_sa_dstring_trimmed();
    group->elements[idx++] = d_tests_sa_dstring_trimmed_left();
    group->elements[idx++] = d_tests_sa_dstring_trimmed_right();

    return group;
}
