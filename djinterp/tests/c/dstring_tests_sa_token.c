#include ".\dstring_tests_sa.h"


/******************************************************************************
 * SECTION 13: TOKENIZATION FUNCTIONS
 *****************************************************************************/

/*
d_tests_sa_dstring_tokenize
  Tests d_string_tokenize function (strtok-style tokenization).
  Tests the following:
  - tokenizing string with single delimiter
  - tokenizing string with multiple delimiters
  - consecutive delimiters
  - no delimiters found (entire string is token)
  - string that is all delimiters
  - empty string handling
  - NULL string handling
  - NULL delimiters handling
*/
struct d_test_object*
d_tests_sa_dstring_tokenize
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    char*                 token;
    char*                 saveptr;
    size_t                idx;

    group = d_test_object_new_interior("d_string_tokenize", 8);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: tokenizing string with single delimiter
    str = d_string_new_from_cstr("hello world test");

    if (str)
    {
        saveptr = NULL;
        token   = d_string_tokenize(str, " ", &saveptr);
        group->elements[idx++] = D_ASSERT_TRUE(
            "tokenize_first_token",
            (token != NULL) && (strcmp(token, "hello") == 0),
            "first token should be 'hello'");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "tokenize_first_token",
            false,
            "failed to allocate test string");
    }

    // test: tokenizing string with multiple delimiters
    str = d_string_new_from_cstr("a,b;c:d");

    if (str)
    {
        saveptr = NULL;
        token   = d_string_tokenize(str, ",;:", &saveptr);
        group->elements[idx++] = D_ASSERT_TRUE(
            "tokenize_multiple_delims",
            (token != NULL) && (strcmp(token, "a") == 0),
            "first token should be 'a'");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "tokenize_multiple_delims",
            false,
            "failed to allocate test string");
    }

    // test: consecutive delimiters (should skip empty tokens)
    str = d_string_new_from_cstr("hello,,world");

    if (str)
    {
        saveptr = NULL;
        token   = d_string_tokenize(str, ",", &saveptr);

        if (token && strcmp(token, "hello") == 0)
        {
            token = d_string_tokenize(NULL, ",", &saveptr);
        }

        group->elements[idx++] = D_ASSERT_TRUE(
            "tokenize_consecutive_delims",
            (token != NULL) && (strcmp(token, "world") == 0),
            "should skip empty tokens");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "tokenize_consecutive_delims",
            false,
            "failed to allocate test string");
    }

    // test: no delimiters found (entire string is token)
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        saveptr = NULL;
        token   = d_string_tokenize(str, ",", &saveptr);
        group->elements[idx++] = D_ASSERT_TRUE(
            "tokenize_no_delims",
            (token != NULL) && (strcmp(token, "hello") == 0),
            "entire string should be single token");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "tokenize_no_delims",
            false,
            "failed to allocate test string");
    }

    // test: string that is all delimiters
    str = d_string_new_from_cstr(",,,");

    if (str)
    {
        saveptr = NULL;
        token   = d_string_tokenize(str, ",", &saveptr);
        group->elements[idx++] = D_ASSERT_NULL(
            "tokenize_all_delims",
            token,
            "should return NULL for all-delimiter string");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "tokenize_all_delims",
            false,
            "failed to allocate test string");
    }

    // test: empty string handling
    str = d_string_new_from_cstr("");

    if (str)
    {
        saveptr = NULL;
        token   = d_string_tokenize(str, ",", &saveptr);
        group->elements[idx++] = D_ASSERT_NULL(
            "tokenize_empty_string",
            token,
            "should return NULL for empty string");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "tokenize_empty_string",
            false,
            "failed to allocate test string");
    }

    // test: NULL string handling
    saveptr = NULL;
    token   = d_string_tokenize(NULL, ",", &saveptr);
    group->elements[idx++] = D_ASSERT_NULL(
        "tokenize_null_string",
        token,
        "should return NULL for NULL string");

    // test: NULL delimiters handling
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        saveptr = NULL;
        token   = d_string_tokenize(str, NULL, &saveptr);
        group->elements[idx++] = D_ASSERT_NULL(
            "tokenize_null_delims",
            token,
            "should return NULL for NULL delimiters");

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "tokenize_null_delims",
            false,
            "failed to allocate test string");
    }

    return group;
}

/*
d_tests_sa_dstring_split
  Tests d_string_split function (returns count, outputs array via pointer).
  Signature: size_t d_string_split(const d_string* _str, const char* _delim, d_string*** _tokens)
  Tests the following:
  - splitting string by single character delimiter
  - splitting string by multi-character delimiter
  - splitting with no delimiter found
  - splitting empty string
  - count return value
  - NULL string handling
  - NULL delimiter handling
*/
struct d_test_object*
d_tests_sa_dstring_split
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    struct d_string**     parts;
    size_t                count;
    size_t                idx;

    group = d_test_object_new_interior("d_string_split", 7);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: splitting string by single character delimiter
    str = d_string_new_from_cstr("hello,world,test");

    if (str)
    {
        parts = NULL;
        count = d_string_split(str, ",", &parts);
        group->elements[idx++] = D_ASSERT_TRUE(
            "split_single_char_delim",
            (parts != NULL) && (count == 3) &&
            d_string_equals_cstr(parts[0], "hello") &&
            d_string_equals_cstr(parts[1], "world") &&
            d_string_equals_cstr(parts[2], "test"),
            "should split into 3 parts");

        if (parts)
        {
            d_string_split_free(parts, count);
        }

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "split_single_char_delim",
            false,
            "failed to allocate test string");
    }

    // test: splitting string by multi-character delimiter
    str = d_string_new_from_cstr("hello::world::test");

    if (str)
    {
        parts = NULL;
        count = d_string_split(str, "::", &parts);
        group->elements[idx++] = D_ASSERT_TRUE(
            "split_multi_char_delim",
            (parts != NULL) && (count == 3) &&
            d_string_equals_cstr(parts[0], "hello") &&
            d_string_equals_cstr(parts[1], "world") &&
            d_string_equals_cstr(parts[2], "test"),
            "should split into 3 parts");

        if (parts)
        {
            d_string_split_free(parts, count);
        }

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "split_multi_char_delim",
            false,
            "failed to allocate test string");
    }

    // test: splitting with no delimiter found
    str = d_string_new_from_cstr("hello world");

    if (str)
    {
        parts = NULL;
        count = d_string_split(str, ",", &parts);
        group->elements[idx++] = D_ASSERT_TRUE(
            "split_no_delim_found",
            (parts != NULL) && (count == 1) &&
            d_string_equals_cstr(parts[0], "hello world"),
            "should return single part containing entire string");

        if (parts)
        {
            d_string_split_free(parts, count);
        }

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "split_no_delim_found",
            false,
            "failed to allocate test string");
    }

    // test: splitting empty string
    str = d_string_new_from_cstr("");

    if (str)
    {
        parts = NULL;
        count = d_string_split(str, ",", &parts);
        group->elements[idx++] = D_ASSERT_TRUE(
            "split_empty_string",
            (parts != NULL) && (count == 1) && d_string_is_empty(parts[0]),
            "should return single empty part");

        if (parts)
        {
            d_string_split_free(parts, count);
        }

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "split_empty_string",
            false,
            "failed to allocate test string");
    }

    // test: count return value
    str = d_string_new_from_cstr("a,b,c,d,e");

    if (str)
    {
        parts = NULL;
        count = d_string_split(str, ",", &parts);
        group->elements[idx++] = D_ASSERT_EQUAL(
            "split_count_return",
            count, 5,
            "should return count of 5");

        if (parts)
        {
            d_string_split_free(parts, count);
        }

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "split_count_return",
            false,
            "failed to allocate test string");
    }

    // test: NULL string handling
    parts = NULL;
    count = d_string_split(NULL, ",", &parts);
    group->elements[idx++] = D_ASSERT_TRUE(
        "split_null_string",
        (parts == NULL) && (count == 0),
        "should return 0 and NULL for NULL string");

    // test: NULL delimiter handling
    str   = d_string_new_from_cstr("hello");
    parts = NULL;
    count = d_string_split(str, NULL, &parts);
    group->elements[idx++] = D_ASSERT_TRUE(
        "split_null_delim",
        (parts == NULL) && (count == 0),
        "should return 0 and NULL for NULL delimiter");

    d_string_free(str);

    return group;
}

/*
d_tests_sa_dstring_split_free
  Tests d_string_split_free function.
  Tests the following:
  - freeing valid split result with multiple parts
  - freeing valid split result with single part
  - NULL array handling with non-zero count
  - NULL array handling with zero count
*/
struct d_test_object*
d_tests_sa_dstring_split_free
(
    void
)
{
    struct d_test_object* group;
    struct d_string*      str;
    struct d_string**     parts;
    size_t                count;
    size_t                idx;

    group = d_test_object_new_interior("d_string_split_free", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    // test: freeing valid split result with multiple parts
    str = d_string_new_from_cstr("a,b,c");

    if (str)
    {
        parts = NULL;
        count = d_string_split(str, ",", &parts);

        if (parts != NULL)
        {
            d_string_split_free(parts, count);
            group->elements[idx++] = D_ASSERT_TRUE(
                "split_free_valid_multiple",
                true,
                "should free multiple parts without crash");
        }
        else
        {
            group->elements[idx++] = D_ASSERT_TRUE(
                "split_free_valid_multiple",
                false,
                "split returned NULL");
        }

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "split_free_valid_multiple",
            false,
            "failed to allocate test string");
    }

    // test: freeing valid split result with single part (no delimiter found)
    str = d_string_new_from_cstr("hello");

    if (str)
    {
        parts = NULL;
        count = d_string_split(str, ",", &parts);

        if (parts != NULL)
        {
            d_string_split_free(parts, count);
            group->elements[idx++] = D_ASSERT_TRUE(
                "split_free_valid_single",
                true,
                "should free single part without crash");
        }
        else
        {
            group->elements[idx++] = D_ASSERT_TRUE(
                "split_free_valid_single",
                false,
                "split returned NULL");
        }

        d_string_free(str);
    }
    else
    {
        group->elements[idx++] = D_ASSERT_TRUE(
            "split_free_valid_single",
            false,
            "failed to allocate test string");
    }

    // test: NULL array handling with non-zero count
    d_string_split_free(NULL, 5);
    group->elements[idx++] = D_ASSERT_TRUE(
        "split_free_null_array",
        true,
        "should handle NULL array without crash");

    // test: NULL array handling with zero count
    d_string_split_free(NULL, 0);
    group->elements[idx++] = D_ASSERT_TRUE(
        "split_free_null_zero_count",
        true,
        "should handle NULL array with zero count without crash");

    return group;
}


/******************************************************************************
 * TOKEN ALL - AGGREGATE RUNNER
 *****************************************************************************/

/*
d_tests_sa_dstring_token_all
  Runs all tokenization function tests for dstring module.
  Tests the following:
  - tokenize function (strtok-style)
  - split function (returns array)
  - split_free function (cleanup)
*/
struct d_test_object*
d_tests_sa_dstring_token_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Tokenization Functions", 3);

    if (!group)
    {
        return NULL;
    }

    idx = 0;

    group->elements[idx++] = d_tests_sa_dstring_tokenize();
    group->elements[idx++] = d_tests_sa_dstring_split();
    group->elements[idx++] = d_tests_sa_dstring_split_free();

    return group;
}