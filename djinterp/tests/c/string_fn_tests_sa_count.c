#include ".\string_fn_tests_sa.h"


/******************************************************************************
 * COUNTING TESTS
 *****************************************************************************/

/*
d_tests_string_fn_strcount_char
  Tests d_strcount_char for character counting.
  Tests the following:
  - counts single occurrence
  - counts multiple occurrences
  - returns 0 when not found
  - handles NULL string
  - handles empty string
  - counts at boundaries
*/
struct d_test_object*
d_tests_string_fn_strcount_char
(
    void
)
{
    struct d_test_object* group;
    size_t                result;
    bool                  test_single;
    bool                  test_multiple;
    bool                  test_not_found;
    bool                  test_null;
    bool                  test_empty;
    bool                  test_boundaries;
    size_t                idx;

    // test 1: single occurrence
    result = d_strcount_char("Hello", 5, 'H');
    test_single = (result == 1);

    // test 2: multiple occurrences
    result = d_strcount_char("aababcabc", 9, 'a');
    test_multiple = (result == 4);

    // test 3: not found
    result = d_strcount_char("Hello", 5, 'z');
    test_not_found = (result == 0);

    // test 4: NULL
    result = d_strcount_char(NULL, 5, 'a');
    test_null = (result == 0);

    // test 5: empty
    result = d_strcount_char("", 0, 'a');
    test_empty = (result == 0);

    // test 6: count at first and last positions
    result = d_strcount_char("abba", 4, 'a');
    test_boundaries = (result == 2);

    // build result tree
    group = d_test_object_new_interior("d_strcount_char", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("single",
                                           test_single,
                                           "counts single occurrence");
    group->elements[idx++] = D_ASSERT_TRUE("multiple",
                                           test_multiple,
                                           "counts multiple occurrences");
    group->elements[idx++] = D_ASSERT_TRUE("not_found",
                                           test_not_found,
                                           "returns 0 when not found");
    group->elements[idx++] = D_ASSERT_TRUE("null",
                                           test_null,
                                           "handles NULL string");
    group->elements[idx++] = D_ASSERT_TRUE("empty",
                                           test_empty,
                                           "handles empty string");
    group->elements[idx++] = D_ASSERT_TRUE("boundaries",
                                           test_boundaries,
                                           "counts at boundaries");

    return group;
}

/*
d_tests_string_fn_strcount_substr
  Tests d_strcount_substr for substring counting.
  Tests the following:
  - counts non-overlapping occurrences
  - returns 0 when not found
  - handles NULL inputs
  - handles empty substring
  - handles substring longer than string
  - counts adjacent occurrences
*/
struct d_test_object*
d_tests_string_fn_strcount_substr
(
    void
)
{
    struct d_test_object* group;
    size_t                result;
    bool                  test_non_overlapping;
    bool                  test_not_found;
    bool                  test_null;
    bool                  test_empty_substr;
    bool                  test_longer_substr;
    bool                  test_adjacent;
    size_t                idx;

    // test 1: non-overlapping
    result = d_strcount_substr("abcabcabc", 9, "abc");
    test_non_overlapping = (result == 3);

    // test 2: not found
    result = d_strcount_substr("Hello World", 11, "xyz");
    test_not_found = (result == 0);

    // test 3: NULL inputs
    size_t r1 = d_strcount_substr(NULL, 5, "abc");
    size_t r2 = d_strcount_substr("abc", 3, NULL);
    test_null = (r1 == 0) && (r2 == 0);

    // test 4: empty substring
    result = d_strcount_substr("Hello", 5, "");
    test_empty_substr = (result == 0);

    // test 5: substring longer than string
    result = d_strcount_substr("Hi", 2, "Hello");
    test_longer_substr = (result == 0);

    // test 6: adjacent occurrences
    result = d_strcount_substr("aaaa", 4, "aa");
    test_adjacent = (result == 2);

    // build result tree
    group = d_test_object_new_interior("d_strcount_substr", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("non_overlapping",
                                           test_non_overlapping,
                                           "counts non-overlapping occurrences");
    group->elements[idx++] = D_ASSERT_TRUE("not_found",
                                           test_not_found,
                                           "returns 0 when not found");
    group->elements[idx++] = D_ASSERT_TRUE("null",
                                           test_null,
                                           "handles NULL inputs");
    group->elements[idx++] = D_ASSERT_TRUE("empty_substr",
                                           test_empty_substr,
                                           "handles empty substring");
    group->elements[idx++] = D_ASSERT_TRUE("longer_substr",
                                           test_longer_substr,
                                           "handles longer substring");
    group->elements[idx++] = D_ASSERT_TRUE("adjacent",
                                           test_adjacent,
                                           "counts adjacent non-overlapping");

    return group;
}

/*
d_tests_string_fn_counting_all
  Runs all counting tests.
  Tests the following:
  - d_strcount_char
  - d_strcount_substr
*/
struct d_test_object*
d_tests_string_fn_counting_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Counting", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_string_fn_strcount_char();
    group->elements[idx++] = d_tests_string_fn_strcount_substr();

    return group;
}


/******************************************************************************
 * HASH TESTS
 *****************************************************************************/

/*
d_tests_string_fn_strhash
  Tests d_strhash for string hashing.
  Tests the following:
  - produces consistent hash for same input
  - different strings produce different hashes
  - handles NULL
  - handles empty string
  - hash changes with length
*/
struct d_test_object*
d_tests_string_fn_strhash
(
    void
)
{
    struct d_test_object* group;
    size_t                hash1;
    size_t                hash2;
    bool                  test_consistent;
    bool                  test_different;
    bool                  test_null;
    bool                  test_empty;
    bool                  test_length_sensitive;
    size_t                idx;

    // test 1: consistent
    hash1 = d_strhash("Hello", 5);
    hash2 = d_strhash("Hello", 5);
    test_consistent = (hash1 == hash2);

    // test 2: different strings => different hashes (very likely)
    hash1 = d_strhash("Hello", 5);
    hash2 = d_strhash("World", 5);
    test_different = (hash1 != hash2);

    // test 3: NULL
    hash1 = d_strhash(NULL, 5);
    test_null = (hash1 == 0);

    // test 4: empty
    hash1 = d_strhash("", 0);
    test_empty = (hash1 == 5381);  // djb2 initial value

    // test 5: same prefix but different length
    hash1 = d_strhash("Hello", 3);
    hash2 = d_strhash("Hello", 5);
    test_length_sensitive = (hash1 != hash2);

    // build result tree
    group = d_test_object_new_interior("d_strhash", 5);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("consistent",
                                           test_consistent,
                                           "produces consistent hash");
    group->elements[idx++] = D_ASSERT_TRUE("different",
                                           test_different,
                                           "different strings produce different hashes");
    group->elements[idx++] = D_ASSERT_TRUE("null",
                                           test_null,
                                           "handles NULL");
    group->elements[idx++] = D_ASSERT_TRUE("empty",
                                           test_empty,
                                           "handles empty string");
    group->elements[idx++] = D_ASSERT_TRUE("length_sensitive",
                                           test_length_sensitive,
                                           "hash changes with length");

    return group;
}

/*
d_tests_string_fn_hash_all
  Runs all hash tests.
  Tests the following:
  - d_strhash
*/
struct d_test_object*
d_tests_string_fn_hash_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Hash", 1);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_string_fn_strhash();

    return group;
}
