#include ".\string_fn_tests_sa.h"


/******************************************************************************
 * PREFIX, SUFFIX, AND CONTAINMENT TESTS
 *****************************************************************************/

/*
d_tests_string_fn_strstartswith
  Tests d_strstartswith for prefix checking.
  Tests the following:
  - returns true for matching prefix
  - returns false for non-matching prefix
  - returns true for empty prefix
  - returns false when prefix longer than string
  - handles NULL inputs
  - returns true when prefix equals string
*/
struct d_test_object*
d_tests_string_fn_strstartswith
(
    void
)
{
    struct d_test_object* group;
    bool                  test_match;
    bool                  test_no_match;
    bool                  test_empty_prefix;
    bool                  test_prefix_longer;
    bool                  test_null;
    bool                  test_exact;
    size_t                idx;

    test_match        = d_strstartswith("Hello World", 11, "Hello", 5);
    test_no_match     = !d_strstartswith("Hello World", 11, "World", 5);
    test_empty_prefix = d_strstartswith("Hello", 5, "", 0);
    test_prefix_longer = !d_strstartswith("Hi", 2, "Hello", 5);
    test_null         = !d_strstartswith(NULL, 0, "Hi", 2) &&
                        !d_strstartswith("Hi", 2, NULL, 0);
    test_exact        = d_strstartswith("Hello", 5, "Hello", 5);

    // build result tree
    group = d_test_object_new_interior("d_strstartswith", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("match",
                                           test_match,
                                           "returns true for matching prefix");
    group->elements[idx++] = D_ASSERT_TRUE("no_match",
                                           test_no_match,
                                           "returns false for non-matching");
    group->elements[idx++] = D_ASSERT_TRUE("empty_prefix",
                                           test_empty_prefix,
                                           "returns true for empty prefix");
    group->elements[idx++] = D_ASSERT_TRUE("prefix_longer",
                                           test_prefix_longer,
                                           "returns false when prefix longer");
    group->elements[idx++] = D_ASSERT_TRUE("null",
                                           test_null,
                                           "handles NULL inputs");
    group->elements[idx++] = D_ASSERT_TRUE("exact",
                                           test_exact,
                                           "returns true when prefix equals string");

    return group;
}

/*
d_tests_string_fn_strendswith
  Tests d_strendswith for suffix checking.
  Tests the following:
  - returns true for matching suffix
  - returns false for non-matching suffix
  - returns true for empty suffix
  - returns false when suffix longer than string
  - handles NULL inputs
  - returns true when suffix equals string
*/
struct d_test_object*
d_tests_string_fn_strendswith
(
    void
)
{
    struct d_test_object* group;
    bool                  test_match;
    bool                  test_no_match;
    bool                  test_empty_suffix;
    bool                  test_suffix_longer;
    bool                  test_null;
    bool                  test_exact;
    size_t                idx;

    test_match         = d_strendswith("Hello World", 11, "World", 5);
    test_no_match      = !d_strendswith("Hello World", 11, "Hello", 5);
    test_empty_suffix  = d_strendswith("Hello", 5, "", 0);
    test_suffix_longer = !d_strendswith("Hi", 2, "Hello", 5);
    test_null          = !d_strendswith(NULL, 0, "Hi", 2) &&
                         !d_strendswith("Hi", 2, NULL, 0);
    test_exact         = d_strendswith("Hello", 5, "Hello", 5);

    // build result tree
    group = d_test_object_new_interior("d_strendswith", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("match",
                                           test_match,
                                           "returns true for matching suffix");
    group->elements[idx++] = D_ASSERT_TRUE("no_match",
                                           test_no_match,
                                           "returns false for non-matching");
    group->elements[idx++] = D_ASSERT_TRUE("empty_suffix",
                                           test_empty_suffix,
                                           "returns true for empty suffix");
    group->elements[idx++] = D_ASSERT_TRUE("suffix_longer",
                                           test_suffix_longer,
                                           "returns false when suffix longer");
    group->elements[idx++] = D_ASSERT_TRUE("null",
                                           test_null,
                                           "handles NULL inputs");
    group->elements[idx++] = D_ASSERT_TRUE("exact",
                                           test_exact,
                                           "returns true when suffix equals string");

    return group;
}

/*
d_tests_string_fn_strcontains
  Tests d_strcontains for substring containment.
  Tests the following:
  - finds substring in middle
  - finds substring at beginning
  - finds substring at end
  - returns false when not found
  - handles NULL inputs
  - handles empty substring
*/
struct d_test_object*
d_tests_string_fn_strcontains
(
    void
)
{
    struct d_test_object* group;
    bool                  test_middle;
    bool                  test_begin;
    bool                  test_end;
    bool                  test_not_found;
    bool                  test_null;
    bool                  test_empty;
    size_t                idx;

    test_middle    = d_strcontains("Hello World", 11, "lo Wo");
    test_begin     = d_strcontains("Hello World", 11, "Hello");
    test_end       = d_strcontains("Hello World", 11, "World");
    test_not_found = !d_strcontains("Hello World", 11, "xyz");
    test_null      = !d_strcontains(NULL, 0, "test") &&
                     !d_strcontains("test", 4, NULL);
    test_empty     = d_strcontains("Hello", 5, "");

    // build result tree
    group = d_test_object_new_interior("d_strcontains", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("middle",
                                           test_middle,
                                           "finds substring in middle");
    group->elements[idx++] = D_ASSERT_TRUE("begin",
                                           test_begin,
                                           "finds substring at beginning");
    group->elements[idx++] = D_ASSERT_TRUE("end",
                                           test_end,
                                           "finds substring at end");
    group->elements[idx++] = D_ASSERT_TRUE("not_found",
                                           test_not_found,
                                           "returns false when not found");
    group->elements[idx++] = D_ASSERT_TRUE("null",
                                           test_null,
                                           "handles NULL inputs");
    group->elements[idx++] = D_ASSERT_TRUE("empty",
                                           test_empty,
                                           "handles empty substring");

    return group;
}

/*
d_tests_string_fn_strcontains_char
  Tests d_strcontains_char for character containment.
  Tests the following:
  - finds character present
  - returns false when not found
  - handles NULL
  - handles empty string
*/
struct d_test_object*
d_tests_string_fn_strcontains_char
(
    void
)
{
    struct d_test_object* group;
    bool                  test_found;
    bool                  test_not_found;
    bool                  test_null;
    bool                  test_empty;
    size_t                idx;

    test_found     = d_strcontains_char("Hello", 5, 'l');
    test_not_found = !d_strcontains_char("Hello", 5, 'z');
    test_null      = !d_strcontains_char(NULL, 0, 'a');
    test_empty     = !d_strcontains_char("", 0, 'a');

    // build result tree
    group = d_test_object_new_interior("d_strcontains_char", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("found",
                                           test_found,
                                           "finds character present");
    group->elements[idx++] = D_ASSERT_TRUE("not_found",
                                           test_not_found,
                                           "returns false when not found");
    group->elements[idx++] = D_ASSERT_TRUE("null",
                                           test_null,
                                           "handles NULL");
    group->elements[idx++] = D_ASSERT_TRUE("empty",
                                           test_empty,
                                           "handles empty string");

    return group;
}

/*
d_tests_string_fn_prefix_suffix_all
  Runs all prefix, suffix, and containment tests.
  Tests the following:
  - d_strstartswith
  - d_strendswith
  - d_strcontains
  - d_strcontains_char
*/
struct d_test_object*
d_tests_string_fn_prefix_suffix_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Prefix, Suffix, Containment", 4);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_string_fn_strstartswith();
    group->elements[idx++] = d_tests_string_fn_strendswith();
    group->elements[idx++] = d_tests_string_fn_strcontains();
    group->elements[idx++] = d_tests_string_fn_strcontains_char();

    return group;
}
