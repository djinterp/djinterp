#include ".\string_fn_tests_sa.h"


/******************************************************************************
 * STRING DUPLICATION TESTS
 *****************************************************************************/

/*
d_tests_string_fn_strdup
  Tests d_strdup for string duplication.
  Tests the following:
  - duplicates normal string correctly
  - allocates correct amount of memory
  - returns NULL for NULL input
  - handles empty string
  - creates independent copy
  - handles strings with special characters
*/
struct d_test_object*
d_tests_string_fn_strdup
(
    void
)
{
    struct d_test_object* group;
    char*                 dup1;
    char*                 dup2;
    char*                 dup3;
    char*                 dup4;
    char                  original[] = "Modifiable";
    bool                  test_normal_dup;
    bool                  test_null_input;
    bool                  test_empty_string;
    bool                  test_independence;
    bool                  test_special_chars;
    bool                  test_memory_allocated;
    size_t                idx;

    // test 1: normal duplication
    dup1 = d_strdup(D_TEST_DSTRING_MEDIUM_STR);
    test_normal_dup = (dup1 != NULL) && 
                     (strcmp(dup1, D_TEST_DSTRING_MEDIUM_STR) == 0) &&
                     (dup1 != D_TEST_DSTRING_MEDIUM_STR);

    // test 2: NULL input
    dup2 = d_strdup(NULL);
    test_null_input = (dup2 == NULL);

    // test 3: empty string
    dup3 = d_strdup("");
    test_empty_string = (dup3 != NULL) && 
                       (dup3[0] == '\0') && 
                       (strlen(dup3) == 0);

    // test 4: independence (modify original after dup)
    dup4 = d_strdup(original);
    test_independence = false;
    
    if (dup4)
    {
        original[0] = 'X';
        test_independence = (dup4[0] == 'M') && 
                          (strcmp(dup4, "Modifiable") == 0);
    }

    // test 5: special characters
    char special[] = "Tab\there\nNewline\r\nCRLF";
    char* dup5 = d_strdup(special);
    test_special_chars = (dup5 != NULL) && 
                         (strcmp(dup5, special) == 0);

    // test 6: memory is actually allocated (different pointers)
    char* dup6 = d_strdup(D_TEST_DSTRING_SHORT_STR);
    test_memory_allocated = (dup6 != NULL) && 
                           (dup6 != D_TEST_DSTRING_SHORT_STR);

    // cleanup
    if (dup1) free(dup1);
    if (dup3) free(dup3);
    if (dup4) free(dup4);
    if (dup5) free(dup5);
    if (dup6) free(dup6);

    // build result tree
    group = d_test_object_new_interior("d_strdup", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("normal_dup",
                                           test_normal_dup,
                                           "duplicates normal string correctly");
    group->elements[idx++] = D_ASSERT_TRUE("null_input",
                                           test_null_input,
                                           "returns NULL for NULL input");
    group->elements[idx++] = D_ASSERT_TRUE("empty_string",
                                           test_empty_string,
                                           "handles empty string");
    group->elements[idx++] = D_ASSERT_TRUE("independence",
                                           test_independence,
                                           "creates independent copy");
    group->elements[idx++] = D_ASSERT_TRUE("special_chars",
                                           test_special_chars,
                                           "handles special characters");
    group->elements[idx++] = D_ASSERT_TRUE("memory_allocated",
                                           test_memory_allocated,
                                           "allocates separate memory");

    return group;
}


/*
d_tests_string_fn_strndup
  Tests d_strndup for counted string duplication.
  Tests the following:
  - duplicates n characters correctly
  - null-terminates result
  - handles n larger than string length
  - returns NULL for NULL input
  - handles zero count
  - truncates at specified length
*/
struct d_test_object*
d_tests_string_fn_strndup
(
    void
)
{
    struct d_test_object* group;
    char*                 dup1;
    char*                 dup2;
    char*                 dup3;
    char*                 dup4;
    char*                 dup5;
    bool                  test_partial_dup;
    bool                  test_null_termination;
    bool                  test_n_larger;
    bool                  test_null_input;
    bool                  test_zero_count;
    bool                  test_truncation;
    size_t                idx;

    // test 1: partial duplication (first 5 chars of "Hello, World!")
    dup1 = d_strndup(D_TEST_DSTRING_MEDIUM_STR, 5);
    test_partial_dup = (dup1 != NULL) && 
                      (strcmp(dup1, "Hello") == 0) && 
                      (strlen(dup1) == 5);

    // test 2: null termination
    dup2 = d_strndup("Testing123", 4);
    test_null_termination = (dup2 != NULL) && 
                           (dup2[4] == '\0') && 
                           (strcmp(dup2, "Test") == 0);

    // test 3: n larger than string length
    dup3 = d_strndup(D_TEST_DSTRING_SHORT_STR, 100);
    test_n_larger = (dup3 != NULL) && 
                   (strcmp(dup3, D_TEST_DSTRING_SHORT_STR) == 0) &&
                   (strlen(dup3) == strlen(D_TEST_DSTRING_SHORT_STR));

    // test 4: NULL input
    dup4 = d_strndup(NULL, 10);
    test_null_input = (dup4 == NULL);

    // test 5: zero count
    dup5 = d_strndup(D_TEST_DSTRING_SHORT_STR, 0);
    test_zero_count = (dup5 != NULL) && (dup5[0] == '\0');

    // test 6: proper truncation
    char* dup6 = d_strndup(D_TEST_DSTRING_LONG_STR, 20);
    test_truncation = (dup6 != NULL) && 
                     (strlen(dup6) == 20) &&
                     (strncmp(dup6, D_TEST_DSTRING_LONG_STR, 20) == 0);

    // cleanup
    if (dup1) free(dup1);
    if (dup2) free(dup2);
    if (dup3) free(dup3);
    if (dup5) free(dup5);
    if (dup6) free(dup6);

    // build result tree
    group = d_test_object_new_interior("d_strndup", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("partial_dup",
                                           test_partial_dup,
                                           "duplicates n characters correctly");
    group->elements[idx++] = D_ASSERT_TRUE("null_termination",
                                           test_null_termination,
                                           "null-terminates result");
    group->elements[idx++] = D_ASSERT_TRUE("n_larger",
                                           test_n_larger,
                                           "handles n larger than string length");
    group->elements[idx++] = D_ASSERT_TRUE("null_input",
                                           test_null_input,
                                           "returns NULL for NULL input");
    group->elements[idx++] = D_ASSERT_TRUE("zero_count",
                                           test_zero_count,
                                           "handles zero count");
    group->elements[idx++] = D_ASSERT_TRUE("truncation",
                                           test_truncation,
                                           "truncates at specified length");

    return group;
}


/*
d_tests_string_fn_duplication_all
  Runs all string duplication tests.
  Tests the following:
  - d_strdup
  - d_strndup
*/
struct d_test_object*
d_tests_string_fn_duplication_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("String Duplication", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_string_fn_strdup();
    group->elements[idx++] = d_tests_string_fn_strndup();

    return group;
}


/******************************************************************************
 * CASE-INSENSITIVE COMPARISON TESTS
 *****************************************************************************/

/*
d_tests_string_fn_strcasecmp
  Tests d_strcasecmp for case-insensitive comparison.
  Tests the following:
  - returns 0 for identical strings
  - returns 0 for case-different strings
  - returns negative for less-than
  - returns positive for greater-than
  - handles NULL inputs
  - handles empty strings
  - handles mixed alphanumeric
*/
struct d_test_object*
d_tests_string_fn_strcasecmp
(
    void
)
{
    struct d_test_object* group;
    int                   result;
    bool                  test_identical;
    bool                  test_case_diff;
    bool                  test_less_than;
    bool                  test_greater_than;
    bool                  test_null_handling;
    bool                  test_empty_strings;
    bool                  test_mixed_alnum;
    size_t                idx;

    // test 1: identical strings
    result = d_strcasecmp("hello", "hello");
    test_identical = (result == 0);

    // test 2: case-different but equal
    result = d_strcasecmp("HeLLo", "hEllO");
    test_case_diff = (result == 0);

    // test 3: less than
    result = d_strcasecmp("apple", "banana");
    test_less_than = (result < 0);

    // test 4: greater than
    result = d_strcasecmp("zebra", "aardvark");
    test_greater_than = (result > 0);

    // test 5: NULL handling
    result = d_strcasecmp(NULL, "test");
    int result2 = d_strcasecmp("test", NULL);
    int result3 = d_strcasecmp(NULL, NULL);
    test_null_handling = (result != 0 || result2 != 0 || result3 == 0);

    // test 6: empty strings
    result = d_strcasecmp("", "");
    int result4 = d_strcasecmp("something", "");
    int result5 = d_strcasecmp("", "something");
    test_empty_strings = (result == 0) && (result4 > 0) && (result5 < 0);

    // test 7: mixed alphanumeric
    result = d_strcasecmp("Test123", "TEST123");
    test_mixed_alnum = (result == 0);

    // build result tree
    group = d_test_object_new_interior("d_strcasecmp", 7);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("identical",
                                           test_identical,
                                           "returns 0 for identical strings");
    group->elements[idx++] = D_ASSERT_TRUE("case_diff",
                                           test_case_diff,
                                           "returns 0 for case-different strings");
    group->elements[idx++] = D_ASSERT_TRUE("less_than",
                                           test_less_than,
                                           "returns negative for less-than");
    group->elements[idx++] = D_ASSERT_TRUE("greater_than",
                                           test_greater_than,
                                           "returns positive for greater-than");
    group->elements[idx++] = D_ASSERT_TRUE("null_handling",
                                           test_null_handling,
                                           "handles NULL inputs");
    group->elements[idx++] = D_ASSERT_TRUE("empty_strings",
                                           test_empty_strings,
                                           "handles empty strings");
    group->elements[idx++] = D_ASSERT_TRUE("mixed_alnum",
                                           test_mixed_alnum,
                                           "handles mixed alphanumeric");

    return group;
}


/*
d_tests_string_fn_strncasecmp
  Tests d_strncasecmp for counted case-insensitive comparison.
  Tests the following:
  - compares n characters correctly
  - ignores differences beyond n
  - handles n larger than strings
  - returns 0 for zero count
  - handles NULL inputs
  - case-insensitive within n chars
*/
struct d_test_object*
d_tests_string_fn_strncasecmp
(
    void
)
{
    struct d_test_object* group;
    int                   result;
    bool                  test_n_chars;
    bool                  test_ignore_beyond_n;
    bool                  test_n_larger;
    bool                  test_zero_count;
    bool                  test_null_handling;
    bool                  test_case_within_n;
    size_t                idx;

    // test 1: compare first n characters
    result = d_strncasecmp("Hello World", "Hello There", 5);
    test_n_chars = (result == 0);

    // test 2: ignores differences beyond n
    result = d_strncasecmp("TestABC", "TestXYZ", 4);
    test_ignore_beyond_n = (result == 0);

    // test 3: n larger than strings
    result = d_strncasecmp("short", "SHORT", 100);
    test_n_larger = (result == 0);

    // test 4: zero count
    result = d_strncasecmp("different", "strings", 0);
    test_zero_count = (result == 0);

    // test 5: NULL handling
    result = d_strncasecmp(NULL, "test", 4);
    int result2 = d_strncasecmp("test", NULL, 4);
    test_null_handling = (result != 0 || result2 != 0);

    // test 6: case-insensitive within n
    result = d_strncasecmp("ABCdef", "abcDEF", 6);
    test_case_within_n = (result == 0);

    // build result tree
    group = d_test_object_new_interior("d_strncasecmp", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("n_chars",
                                           test_n_chars,
                                           "compares n characters correctly");
    group->elements[idx++] = D_ASSERT_TRUE("ignore_beyond_n",
                                           test_ignore_beyond_n,
                                           "ignores differences beyond n");
    group->elements[idx++] = D_ASSERT_TRUE("n_larger",
                                           test_n_larger,
                                           "handles n larger than strings");
    group->elements[idx++] = D_ASSERT_TRUE("zero_count",
                                           test_zero_count,
                                           "returns 0 for zero count");
    group->elements[idx++] = D_ASSERT_TRUE("null_handling",
                                           test_null_handling,
                                           "handles NULL inputs");
    group->elements[idx++] = D_ASSERT_TRUE("case_within_n",
                                           test_case_within_n,
                                           "case-insensitive within n chars");

    return group;
}


/*
d_tests_string_fn_case_comparison_all
  Runs all case-insensitive comparison tests.
  Tests the following:
  - d_strcasecmp
  - d_strncasecmp
*/
struct d_test_object*
d_tests_string_fn_case_comparison_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Case-Insensitive Comparison", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_string_fn_strcasecmp();
    group->elements[idx++] = d_tests_string_fn_strncasecmp();

    return group;
}


