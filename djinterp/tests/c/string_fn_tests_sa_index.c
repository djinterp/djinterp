#include ".\string_fn_tests_sa.h"


/******************************************************************************
 * INDEX-RETURNING SEARCH TESTS
 *****************************************************************************/

/*
d_tests_string_fn_strchr_index
  Tests d_strchr_index for character index search.
  Tests the following:
  - finds first occurrence
  - returns D_STRING_NPOS when not found
  - handles NULL
  - handles empty string
  - finds at position 0
  - finds at last position
*/
struct d_test_object*
d_tests_string_fn_strchr_index
(
    void
)
{
    struct d_test_object* group;
    d_index               result;
    bool                  test_found;
    bool                  test_not_found;
    bool                  test_null;
    bool                  test_empty;
    bool                  test_at_zero;
    bool                  test_at_end;
    size_t                idx;

    // test 1: finds first occurrence
    result = d_strchr_index("Hello World", 11, 'o');
    test_found = (result == 4);

    // test 2: not found
    result = d_strchr_index("Hello", 5, 'z');
    test_not_found = (result == D_STRING_NPOS);

    // test 3: NULL
    result = d_strchr_index(NULL, 5, 'a');
    test_null = (result == D_STRING_NPOS);

    // test 4: empty
    result = d_strchr_index("", 0, 'a');
    test_empty = (result == D_STRING_NPOS);

    // test 5: at position 0
    result = d_strchr_index("Hello", 5, 'H');
    test_at_zero = (result == 0);

    // test 6: at last position
    result = d_strchr_index("Hello", 5, 'o');
    test_at_end = (result == 4);

    // build result tree
    group = d_test_object_new_interior("d_strchr_index", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("found",
                                           test_found,
                                           "finds first occurrence");
    group->elements[idx++] = D_ASSERT_TRUE("not_found",
                                           test_not_found,
                                           "returns D_STRING_NPOS when not found");
    group->elements[idx++] = D_ASSERT_TRUE("null",
                                           test_null,
                                           "handles NULL");
    group->elements[idx++] = D_ASSERT_TRUE("empty",
                                           test_empty,
                                           "handles empty string");
    group->elements[idx++] = D_ASSERT_TRUE("at_zero",
                                           test_at_zero,
                                           "finds at position 0");
    group->elements[idx++] = D_ASSERT_TRUE("at_end",
                                           test_at_end,
                                           "finds at last position");

    return group;
}

/*
d_tests_string_fn_strchr_index_from
  Tests d_strchr_index_from for character search from offset.
  Tests the following:
  - finds from given start position
  - skips occurrences before start
  - returns D_STRING_NPOS when start beyond length
  - returns D_STRING_NPOS when not found after start
*/
struct d_test_object*
d_tests_string_fn_strchr_index_from
(
    void
)
{
    struct d_test_object* group;
    d_index               result;
    bool                  test_from_start;
    bool                  test_skip_early;
    bool                  test_start_beyond;
    bool                  test_not_after_start;
    size_t                idx;

    // test 1: find from given start
    result = d_strchr_index_from("Hello World", 11, 'o', 0);
    test_from_start = (result == 4);

    // test 2: skip earlier occurrences
    result = d_strchr_index_from("Hello World", 11, 'o', 5);
    test_skip_early = (result == 7);

    // test 3: start beyond length
    result = d_strchr_index_from("Hello", 5, 'H', 10);
    test_start_beyond = (result == D_STRING_NPOS);

    // test 4: not found after start
    result = d_strchr_index_from("Hello", 5, 'H', 1);
    test_not_after_start = (result == D_STRING_NPOS);

    // build result tree
    group = d_test_object_new_interior("d_strchr_index_from", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("from_start",
                                           test_from_start,
                                           "finds from given start position");
    group->elements[idx++] = D_ASSERT_TRUE("skip_early",
                                           test_skip_early,
                                           "skips occurrences before start");
    group->elements[idx++] = D_ASSERT_TRUE("start_beyond",
                                           test_start_beyond,
                                           "returns D_STRING_NPOS when start beyond length");
    group->elements[idx++] = D_ASSERT_TRUE("not_after_start",
                                           test_not_after_start,
                                           "returns D_STRING_NPOS when not found after start");

    return group;
}

/*
d_tests_string_fn_strrchr_index
  Tests d_strrchr_index for reverse character index search.
  Tests the following:
  - finds last occurrence
  - returns D_STRING_NPOS when not found
  - handles single occurrence
  - handles NULL
  - handles empty string
*/
struct d_test_object*
d_tests_string_fn_strrchr_index
(
    void
)
{
    struct d_test_object* group;
    d_index               result;
    bool                  test_last;
    bool                  test_not_found;
    bool                  test_single;
    bool                  test_null;
    bool                  test_empty;
    size_t                idx;

    // test 1: last occurrence
    result = d_strrchr_index("Hello World", 11, 'o');
    test_last = (result == 7);

    // test 2: not found
    result = d_strrchr_index("Hello", 5, 'z');
    test_not_found = (result == D_STRING_NPOS);

    // test 3: single occurrence at position 0
    result = d_strrchr_index("Hello", 5, 'H');
    test_single = (result == 0);

    // test 4: NULL
    result = d_strrchr_index(NULL, 5, 'a');
    test_null = (result == D_STRING_NPOS);

    // test 5: empty
    result = d_strrchr_index("", 0, 'a');
    test_empty = (result == D_STRING_NPOS);

    // build result tree
    group = d_test_object_new_interior("d_strrchr_index", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("last",
                                           test_last,
                                           "finds last occurrence");
    group->elements[idx++] = D_ASSERT_TRUE("not_found",
                                           test_not_found,
                                           "returns D_STRING_NPOS when not found");
    group->elements[idx++] = D_ASSERT_TRUE("single",
                                           test_single,
                                           "handles single occurrence");
    group->elements[idx++] = D_ASSERT_TRUE("null",
                                           test_null,
                                           "handles NULL");
    group->elements[idx++] = D_ASSERT_TRUE("empty",
                                           test_empty,
                                           "handles empty string");

    return group;
}

/*
d_tests_string_fn_strstr_index
  Tests d_strstr_index for substring index search.
  Tests the following:
  - finds substring
  - returns D_STRING_NPOS when not found
  - finds empty substring at 0
  - handles NULL inputs
  - handles substring longer than string
  - finds at beginning
*/
struct d_test_object*
d_tests_string_fn_strstr_index
(
    void
)
{
    struct d_test_object* group;
    d_index               result;
    bool                  test_found;
    bool                  test_not_found;
    bool                  test_empty_substr;
    bool                  test_null;
    bool                  test_longer;
    bool                  test_at_begin;
    size_t                idx;

    // test 1: finds substring
    result = d_strstr_index("Hello World", 11, "World", 5);
    test_found = (result == 6);

    // test 2: not found
    result = d_strstr_index("Hello World", 11, "xyz", 3);
    test_not_found = (result == D_STRING_NPOS);

    // test 3: empty substring
    result = d_strstr_index("Hello", 5, "", 0);
    test_empty_substr = (result == 0);

    // test 4: NULL
    result = d_strstr_index(NULL, 5, "abc", 3);
    test_null = (result == D_STRING_NPOS);

    // test 5: substring longer than string
    result = d_strstr_index("Hi", 2, "Hello", 5);
    test_longer = (result == D_STRING_NPOS);

    // test 6: at beginning
    result = d_strstr_index("Hello World", 11, "Hello", 5);
    test_at_begin = (result == 0);

    // build result tree
    group = d_test_object_new_interior("d_strstr_index", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("found",
                                           test_found,
                                           "finds substring");
    group->elements[idx++] = D_ASSERT_TRUE("not_found",
                                           test_not_found,
                                           "returns D_STRING_NPOS when not found");
    group->elements[idx++] = D_ASSERT_TRUE("empty_substr",
                                           test_empty_substr,
                                           "finds empty substring at 0");
    group->elements[idx++] = D_ASSERT_TRUE("null",
                                           test_null,
                                           "handles NULL inputs");
    group->elements[idx++] = D_ASSERT_TRUE("longer",
                                           test_longer,
                                           "handles longer substring");
    group->elements[idx++] = D_ASSERT_TRUE("at_begin",
                                           test_at_begin,
                                           "finds at beginning");

    return group;
}

/*
d_tests_string_fn_strstr_index_from
  Tests d_strstr_index_from for substring search from offset.
  Tests the following:
  - finds from given start position
  - skips earlier occurrences
  - returns D_STRING_NPOS when start beyond length
  - returns D_STRING_NPOS when not found after start
*/
struct d_test_object*
d_tests_string_fn_strstr_index_from
(
    void
)
{
    struct d_test_object* group;
    d_index               result;
    bool                  test_from_start;
    bool                  test_skip_early;
    bool                  test_start_beyond;
    bool                  test_not_after_start;
    size_t                idx;

    // test 1: find from given start
    result = d_strstr_index_from("abcabcabc", 9, "abc", 3, 0);
    test_from_start = (result == 0);

    // test 2: skip earlier occurrences
    result = d_strstr_index_from("abcabcabc", 9, "abc", 3, 1);
    test_skip_early = (result == 3);

    // test 3: start beyond length
    result = d_strstr_index_from("Hello", 5, "lo", 2, 10);
    test_start_beyond = (result == D_STRING_NPOS);

    // test 4: not found after start
    result = d_strstr_index_from("Hello World", 11, "Hello", 5, 1);
    test_not_after_start = (result == D_STRING_NPOS);

    // build result tree
    group = d_test_object_new_interior("d_strstr_index_from", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("from_start",
                                           test_from_start,
                                           "finds from given start position");
    group->elements[idx++] = D_ASSERT_TRUE("skip_early",
                                           test_skip_early,
                                           "skips earlier occurrences");
    group->elements[idx++] = D_ASSERT_TRUE("start_beyond",
                                           test_start_beyond,
                                           "returns D_STRING_NPOS when start beyond length");
    group->elements[idx++] = D_ASSERT_TRUE("not_after_start",
                                           test_not_after_start,
                                           "returns D_STRING_NPOS when not found after start");

    return group;
}

/*
d_tests_string_fn_strrstr_index
  Tests d_strrstr_index for reverse substring index search.
  Tests the following:
  - finds last occurrence
  - returns D_STRING_NPOS when not found
  - handles single occurrence
  - handles NULL
  - handles empty substring
*/
struct d_test_object*
d_tests_string_fn_strrstr_index
(
    void
)
{
    struct d_test_object* group;
    d_index               result;
    bool                  test_last;
    bool                  test_not_found;
    bool                  test_single;
    bool                  test_null;
    bool                  test_empty;
    size_t                idx;

    // test 1: last occurrence
    result = d_strrstr_index("abcabcabc", 9, "abc", 3);
    test_last = (result == 6);

    // test 2: not found
    result = d_strrstr_index("Hello", 5, "xyz", 3);
    test_not_found = (result == D_STRING_NPOS);

    // test 3: single occurrence
    result = d_strrstr_index("Hello World", 11, "World", 5);
    test_single = (result == 6);

    // test 4: NULL
    result = d_strrstr_index(NULL, 5, "abc", 3);
    test_null = (result == D_STRING_NPOS);

    // test 5: empty substring returns string length
    result = d_strrstr_index("Hello", 5, "", 0);
    test_empty = (result == 5);

    // build result tree
    group = d_test_object_new_interior("d_strrstr_index", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("last",
                                           test_last,
                                           "finds last occurrence");
    group->elements[idx++] = D_ASSERT_TRUE("not_found",
                                           test_not_found,
                                           "returns D_STRING_NPOS when not found");
    group->elements[idx++] = D_ASSERT_TRUE("single",
                                           test_single,
                                           "handles single occurrence");
    group->elements[idx++] = D_ASSERT_TRUE("null",
                                           test_null,
                                           "handles NULL");
    group->elements[idx++] = D_ASSERT_TRUE("empty",
                                           test_empty,
                                           "handles empty substring");

    return group;
}

/*
d_tests_string_fn_strcasestr_index
  Tests d_strcasestr_index for case-insensitive substring index search.
  Tests the following:
  - finds case-insensitive match
  - returns D_STRING_NPOS when not found
  - handles same case
  - handles NULL
  - handles empty substring
*/
struct d_test_object*
d_tests_string_fn_strcasestr_index
(
    void
)
{
    struct d_test_object* group;
    d_index               result;
    bool                  test_case_insensitive;
    bool                  test_not_found;
    bool                  test_same_case;
    bool                  test_null;
    bool                  test_empty;
    size_t                idx;

    // test 1: case-insensitive match
    result = d_strcasestr_index("Hello World", 11, "WORLD", 5);
    test_case_insensitive = (result == 6);

    // test 2: not found
    result = d_strcasestr_index("Hello World", 11, "xyz", 3);
    test_not_found = (result == D_STRING_NPOS);

    // test 3: same case
    result = d_strcasestr_index("Hello World", 11, "World", 5);
    test_same_case = (result == 6);

    // test 4: NULL
    result = d_strcasestr_index(NULL, 5, "abc", 3);
    test_null = (result == D_STRING_NPOS);

    // test 5: empty substring
    result = d_strcasestr_index("Hello", 5, "", 0);
    test_empty = (result == 0);

    // build result tree
    group = d_test_object_new_interior("d_strcasestr_index", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("case_insensitive",
                                           test_case_insensitive,
                                           "finds case-insensitive match");
    group->elements[idx++] = D_ASSERT_TRUE("not_found",
                                           test_not_found,
                                           "returns D_STRING_NPOS when not found");
    group->elements[idx++] = D_ASSERT_TRUE("same_case",
                                           test_same_case,
                                           "handles same case");
    group->elements[idx++] = D_ASSERT_TRUE("null",
                                           test_null,
                                           "handles NULL");
    group->elements[idx++] = D_ASSERT_TRUE("empty",
                                           test_empty,
                                           "handles empty substring");

    return group;
}

/*
d_tests_string_fn_index_search_all
  Runs all index-returning search tests.
  Tests the following:
  - d_strchr_index
  - d_strchr_index_from
  - d_strrchr_index
  - d_strstr_index
  - d_strstr_index_from
  - d_strrstr_index
  - d_strcasestr_index
*/
struct d_test_object*
d_tests_string_fn_index_search_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Index-Returning Search", 7);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_string_fn_strchr_index();
    group->elements[idx++] = d_tests_string_fn_strchr_index_from();
    group->elements[idx++] = d_tests_string_fn_strrchr_index();
    group->elements[idx++] = d_tests_string_fn_strstr_index();
    group->elements[idx++] = d_tests_string_fn_strstr_index_from();
    group->elements[idx++] = d_tests_string_fn_strrstr_index();
    group->elements[idx++] = d_tests_string_fn_strcasestr_index();

    return group;
}


/******************************************************************************
 * IN-PLACE CHARACTER REPLACEMENT TESTS
 *****************************************************************************/

/*
d_tests_string_fn_strreplace_char
  Tests d_strreplace_char for in-place character replacement.
  Tests the following:
  - replaces all occurrences
  - returns correct count
  - handles no occurrences
  - handles NULL
  - handles empty string
  - replaces at boundaries
*/
struct d_test_object*
d_tests_string_fn_strreplace_char
(
    void
)
{
    struct d_test_object* group;
    char                  str1[] = "Hello World";
    char                  str2[] = "No match here";
    char                  str3[] = "aabaa";
    size_t                count;
    bool                  test_replaces_all;
    bool                  test_correct_count;
    bool                  test_no_match;
    bool                  test_null;
    bool                  test_empty;
    bool                  test_boundaries;
    size_t                idx;

    // test 1: replaces all occurrences
    count = d_strreplace_char(str1, 11, 'l', 'L');
    test_replaces_all = (strcmp(str1, "HeLLo WorLd") == 0);

    // test 2: correct count
    test_correct_count = (count == 3);

    // test 3: no match
    count = d_strreplace_char(str2, 13, 'z', 'Z');
    test_no_match = (count == 0) && 
                    (strcmp(str2, "No match here") == 0);

    // test 4: NULL
    count = d_strreplace_char(NULL, 5, 'a', 'b');
    test_null = (count == 0);

    // test 5: empty
    char empty[] = "";
    count = d_strreplace_char(empty, 0, 'a', 'b');
    test_empty = (count == 0);

    // test 6: at boundaries (first and last chars)
    count = d_strreplace_char(str3, 5, 'a', 'X');
    test_boundaries = (strcmp(str3, "XXbXX") == 0) && (count == 4);

    // build result tree
    group = d_test_object_new_interior("d_strreplace_char", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("replaces_all",
                                           test_replaces_all,
                                           "replaces all occurrences");
    group->elements[idx++] = D_ASSERT_TRUE("correct_count",
                                           test_correct_count,
                                           "returns correct count");
    group->elements[idx++] = D_ASSERT_TRUE("no_match",
                                           test_no_match,
                                           "handles no occurrences");
    group->elements[idx++] = D_ASSERT_TRUE("null",
                                           test_null,
                                           "handles NULL");
    group->elements[idx++] = D_ASSERT_TRUE("empty",
                                           test_empty,
                                           "handles empty string");
    group->elements[idx++] = D_ASSERT_TRUE("boundaries",
                                           test_boundaries,
                                           "replaces at boundaries");

    return group;
}

/*
d_tests_string_fn_replace_all
  Runs all replacement tests.
  Tests the following:
  - d_strreplace_char
*/
struct d_test_object*
d_tests_string_fn_replace_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("In-Place Replacement", 1);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_string_fn_strreplace_char();

    return group;
}
