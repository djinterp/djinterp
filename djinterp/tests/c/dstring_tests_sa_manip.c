/******************************************************************************
* djinterp [test]                                        dstring_tests_sa_manip.c
*
*   Tests for string manipulation (strrev, strlwr, strupr), search operations
* (strcasestr, strchrnul), and tokenization (strtok_r).
*
*
* path:      \src\test\dstring_tests_sa_manip.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.30
******************************************************************************/
#include ".\dstring_tests_sa.h"


/******************************************************************************
 * STRING TOKENIZATION TESTS
 *****************************************************************************/

/*
d_tests_dstring_strtok_r
  Tests d_strtok_r for thread-safe tokenization.
  Tests the following:
  - tokenizes string correctly
  - handles multiple delimiters
  - returns NULL when no more tokens
  - handles NULL string on continuation
  - handles empty tokens
  - preserves state between calls
  - handles single character delimiters
*/
struct d_test_object*
d_tests_dstring_strtok_r
(
    void
)
{
    struct d_test_object* group;
    char                  str1[] = "one,two,three,four";
    char                  str2[] = "  hello   world  ";
    char                  str3[] = "a::b::c::";
    char                  str4[] = "single";
    char*                 token;
    char*                 saveptr1;
    char*                 saveptr2;
    bool                  test_basic_tokenize;
    bool                  test_multiple_delims;
    bool                  test_null_continuation;
    bool                  test_empty_tokens;
    bool                  test_state_preserved;
    bool                  test_single_token;
    bool                  test_no_more_tokens;
    size_t                idx;

    // test 1: basic tokenization
    saveptr1 = NULL;
    token = d_strtok_r(str1, ",", &saveptr1);
    test_basic_tokenize = (token != NULL) && (strcmp(token, "one") == 0);
    token = d_strtok_r(NULL, ",", &saveptr1);
    test_basic_tokenize = test_basic_tokenize && 
                         (token != NULL) && 
                         (strcmp(token, "two") == 0);

    // test 2: multiple delimiters (spaces)
    saveptr2 = NULL;
    token = d_strtok_r(str2, " ", &saveptr2);
    test_multiple_delims = (token != NULL) && (strcmp(token, "hello") == 0);
    token = d_strtok_r(NULL, " ", &saveptr2);
    test_multiple_delims = test_multiple_delims && 
                          (token != NULL) && 
                          (strcmp(token, "world") == 0);

    // test 3: NULL string on continuation
    saveptr1 = NULL;
    token = d_strtok_r(str1, ",", &saveptr1);
    token = d_strtok_r(NULL, ",", &saveptr1);  // continue with NULL
    test_null_continuation = (token != NULL);

    // test 4: empty tokens (consecutive delimiters)
    saveptr1 = NULL;
    token = d_strtok_r(str3, ":", &saveptr1);
    test_empty_tokens = (token != NULL) && (strcmp(token, "a") == 0);
    token = d_strtok_r(NULL, ":", &saveptr1);
    test_empty_tokens = test_empty_tokens && 
                       (token != NULL) && 
                       (strcmp(token, "b") == 0);

    // test 5: state preserved between calls
    char str5[] = "1-2-3";
    char str6[] = "a-b-c";
    char* save5 = NULL;
    char* save6 = NULL;
    
    char* tok5 = d_strtok_r(str5, "-", &save5);
    char* tok6 = d_strtok_r(str6, "-", &save6);
    tok5 = d_strtok_r(NULL, "-", &save5);
    tok6 = d_strtok_r(NULL, "-", &save6);
    
    test_state_preserved = (tok5 != NULL) && (strcmp(tok5, "2") == 0) &&
                          (tok6 != NULL) && (strcmp(tok6, "b") == 0);

    // test 6: single token
    saveptr1 = NULL;
    token = d_strtok_r(str4, ",", &saveptr1);
    test_single_token = (token != NULL) && (strcmp(token, "single") == 0);
    token = d_strtok_r(NULL, ",", &saveptr1);
    test_single_token = test_single_token && (token == NULL);

    // test 7: no more tokens returns NULL
    char str7[] = "last";
    saveptr1 = NULL;
    token = d_strtok_r(str7, ",", &saveptr1);
    token = d_strtok_r(NULL, ",", &saveptr1);
    test_no_more_tokens = (token == NULL);

    // build result tree
    group = d_test_object_new_interior("d_strtok_r", 7);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("basic_tokenize",
                                           test_basic_tokenize,
                                           "tokenizes string correctly");
    group->elements[idx++] = D_ASSERT_TRUE("multiple_delims",
                                           test_multiple_delims,
                                           "handles multiple delimiters");
    group->elements[idx++] = D_ASSERT_TRUE("null_continuation",
                                           test_null_continuation,
                                           "handles NULL string on continuation");
    group->elements[idx++] = D_ASSERT_TRUE("empty_tokens",
                                           test_empty_tokens,
                                           "handles empty tokens");
    group->elements[idx++] = D_ASSERT_TRUE("state_preserved",
                                           test_state_preserved,
                                           "preserves state between calls");
    group->elements[idx++] = D_ASSERT_TRUE("single_token",
                                           test_single_token,
                                           "handles single token");
    group->elements[idx++] = D_ASSERT_TRUE("no_more_tokens",
                                           test_no_more_tokens,
                                           "returns NULL when no more tokens");

    return group;
}


/*
d_tests_dstring_tokenization_all
  Runs all tokenization tests.
  Tests the following:
  - d_strtok_r
*/
struct d_test_object*
d_tests_dstring_tokenization_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("String Tokenization", 1);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dstring_strtok_r();

    return group;
}


/******************************************************************************
 * STRING LENGTH TESTS
 *****************************************************************************/

/*
d_tests_dstring_strnlen
  Tests d_strnlen for length with limit.
  Tests the following:
  - returns actual length when less than max
  - returns max when string is longer
  - handles zero max length
  - handles NULL string
  - handles empty string
  - handles exact length match
*/
struct d_test_object*
d_tests_dstring_strnlen
(
    void
)
{
    struct d_test_object* group;
    size_t                result;
    bool                  test_less_than_max;
    bool                  test_greater_than_max;
    bool                  test_zero_max;
    bool                  test_null_string;
    bool                  test_empty_string;
    bool                  test_exact_match;
    size_t                idx;

    // test 1: actual length less than max
    result = d_strnlen("Hello", 10);
    test_less_than_max = (result == 5);

    // test 2: string longer than max
    result = d_strnlen(D_TEST_DSTRING_LONG_STR, 10);
    test_greater_than_max = (result == 10);

    // test 3: zero max length
    result = d_strnlen("Something", 0);
    test_zero_max = (result == 0);

    // test 4: NULL string
    result = d_strnlen(NULL, 100);
    test_null_string = (result == 0);

    // test 5: empty string
    result = d_strnlen("", 100);
    test_empty_string = (result == 0);

    // test 6: exact length match
    result = d_strnlen("12345", 5);
    test_exact_match = (result == 5);

    // build result tree
    group = d_test_object_new_interior("d_strnlen", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("less_than_max",
                                           test_less_than_max,
                                           "returns actual length when less than max");
    group->elements[idx++] = D_ASSERT_TRUE("greater_than_max",
                                           test_greater_than_max,
                                           "returns max when string is longer");
    group->elements[idx++] = D_ASSERT_TRUE("zero_max",
                                           test_zero_max,
                                           "handles zero max length");
    group->elements[idx++] = D_ASSERT_TRUE("null_string",
                                           test_null_string,
                                           "handles NULL string");
    group->elements[idx++] = D_ASSERT_TRUE("empty_string",
                                           test_empty_string,
                                           "handles empty string");
    group->elements[idx++] = D_ASSERT_TRUE("exact_match",
                                           test_exact_match,
                                           "handles exact length match");

    return group;
}


/*
d_tests_dstring_length_all
  Runs all string length tests.
  Tests the following:
  - d_strnlen
*/
struct d_test_object*
d_tests_dstring_length_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("String Length", 1);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dstring_strnlen();

    return group;
}


/******************************************************************************
 * STRING SEARCH TESTS
 *****************************************************************************/

/*
d_tests_dstring_strcasestr
  Tests d_strcasestr for case-insensitive substring search.
  Tests the following:
  - finds substring with same case
  - finds substring with different case
  - returns NULL when not found
  - handles NULL inputs
  - finds at beginning
  - finds at end
  - handles empty needle
*/
struct d_test_object*
d_tests_dstring_strcasestr
(
    void
)
{
    struct d_test_object* group;
    char*                 result;
    bool                  test_same_case;
    bool                  test_diff_case;
    bool                  test_not_found;
    bool                  test_null_inputs;
    bool                  test_at_beginning;
    bool                  test_at_end;
    bool                  test_empty_needle;
    size_t                idx;

    // test 1: same case
    result = d_strcasestr("Hello World", "World");
    test_same_case = (result != NULL) && (strcmp(result, "World") == 0);

    // test 2: different case
    result = d_strcasestr("Hello World", "WORLD");
    test_diff_case = (result != NULL) && (strcmp(result, "World") == 0);

    // test 3: not found
    result = d_strcasestr("Hello World", "xyz");
    test_not_found = (result == NULL);

    // test 4: NULL inputs
    result = d_strcasestr(NULL, "test");
    char* result2 = d_strcasestr("test", NULL);
    test_null_inputs = (result == NULL) && (result2 == NULL);

    // test 5: at beginning
    result = d_strcasestr("Testing 123", "test");
    test_at_beginning = (result != NULL) && 
                       (result == (char*)("Testing 123"));

    // test 6: at end
    result = d_strcasestr("Start to End", "END");
    test_at_end = (result != NULL) && (strcmp(result, "End") == 0);

    // test 7: empty needle
    result = d_strcasestr("Something", "");
    test_empty_needle = (result != NULL);

    // build result tree
    group = d_test_object_new_interior("d_strcasestr", 7);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("same_case",
                                           test_same_case,
                                           "finds substring with same case");
    group->elements[idx++] = D_ASSERT_TRUE("diff_case",
                                           test_diff_case,
                                           "finds substring with different case");
    group->elements[idx++] = D_ASSERT_TRUE("not_found",
                                           test_not_found,
                                           "returns NULL when not found");
    group->elements[idx++] = D_ASSERT_TRUE("null_inputs",
                                           test_null_inputs,
                                           "handles NULL inputs");
    group->elements[idx++] = D_ASSERT_TRUE("at_beginning",
                                           test_at_beginning,
                                           "finds at beginning");
    group->elements[idx++] = D_ASSERT_TRUE("at_end",
                                           test_at_end,
                                           "finds at end");
    group->elements[idx++] = D_ASSERT_TRUE("empty_needle",
                                           test_empty_needle,
                                           "handles empty needle");

    return group;
}


/*
d_tests_dstring_strchrnul
  Tests d_strchrnul for character search returning end pointer.
  Tests the following:
  - finds character when present
  - returns end pointer when not found
  - finds first occurrence
  - handles NULL string
  - finds null terminator
  - handles empty string
*/
struct d_test_object*
d_tests_dstring_strchrnul
(
    void
)
{
    struct d_test_object* group;
    char                  test_str[] = "Hello World";
    char*                 result;
    bool                  test_found;
    bool                  test_not_found;
    bool                  test_first_occurrence;
    bool                  test_null_string;
    bool                  test_find_null;
    bool                  test_empty_string;
    size_t                idx;

    // test 1: character found
    result = d_strchrnul(test_str, 'o');
    test_found = (result != NULL) && (*result == 'o') && 
                (result == &test_str[4]);

    // test 2: character not found returns end
    result = d_strchrnul(test_str, 'x');
    test_not_found = (result != NULL) && (*result == '\0') && 
                    (result == &test_str[strlen(test_str)]);

    // test 3: finds first occurrence
    result = d_strchrnul("multiple o's here", 'o');
    test_first_occurrence = (result != NULL) && 
                           (result == strchr("multiple o's here", 'o'));

    // test 4: NULL string
    result = d_strchrnul(NULL, 'a');
    test_null_string = (result == NULL);

    // test 5: find null terminator
    result = d_strchrnul(test_str, '\0');
    test_find_null = (result != NULL) && (*result == '\0');

    // test 6: empty string
    result = d_strchrnul("", 'a');
    test_empty_string = (result != NULL) && (*result == '\0');

    // build result tree
    group = d_test_object_new_interior("d_strchrnul", 6);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("found",
                                           test_found,
                                           "finds character when present");
    group->elements[idx++] = D_ASSERT_TRUE("not_found",
                                           test_not_found,
                                           "returns end pointer when not found");
    group->elements[idx++] = D_ASSERT_TRUE("first_occurrence",
                                           test_first_occurrence,
                                           "finds first occurrence");
    group->elements[idx++] = D_ASSERT_TRUE("null_string",
                                           test_null_string,
                                           "handles NULL string");
    group->elements[idx++] = D_ASSERT_TRUE("find_null",
                                           test_find_null,
                                           "finds null terminator");
    group->elements[idx++] = D_ASSERT_TRUE("empty_string",
                                           test_empty_string,
                                           "handles empty string");

    return group;
}


/*
d_tests_dstring_search_all
  Runs all string search tests.
  Tests the following:
  - d_strcasestr
  - d_strchrnul
*/
struct d_test_object*
d_tests_dstring_search_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("String Search", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dstring_strcasestr();
    group->elements[idx++] = d_tests_dstring_strchrnul();

    return group;
}


/******************************************************************************
 * CASE CONVERSION TESTS
 *****************************************************************************/

/*
d_tests_dstring_strlwr
  Tests d_strlwr for lowercase conversion.
  Tests the following:
  - converts uppercase to lowercase
  - preserves lowercase letters
  - preserves non-alphabetic characters
  - handles NULL input
  - handles empty string
  - returns original pointer
  - modifies in place
*/
struct d_test_object*
d_tests_dstring_strlwr
(
    void
)
{
    struct d_test_object* group;
    char                  str1[] = "HELLO WORLD";
    char                  str2[] = "MiXeD CaSe 123!";
    char                  str3[] = "already lowercase";
    char                  str4[] = "123!@#$%^";
    char                  str5[] = "";
    char*                 result;
    bool                  test_uppercase_convert;
    bool                  test_mixed_case;
    bool                  test_preserve_lower;
    bool                  test_preserve_nonalpha;
    bool                  test_null_input;
    bool                  test_empty_string;
    bool                  test_returns_pointer;
    size_t                idx;

    // test 1: converts uppercase
    result = d_strlwr(str1);
    test_uppercase_convert = (strcmp(str1, "hello world") == 0);

    // test 2: mixed case
    result = d_strlwr(str2);
    test_mixed_case = (strcmp(str2, "mixed case 123!") == 0);

    // test 3: preserves lowercase
    result = d_strlwr(str3);
    test_preserve_lower = (strcmp(str3, "already lowercase") == 0);

    // test 4: preserves non-alphabetic
    result = d_strlwr(str4);
    test_preserve_nonalpha = (strcmp(str4, "123!@#$%^") == 0);

    // test 5: NULL input
    result = d_strlwr(NULL);
    test_null_input = (result == NULL);

    // test 6: empty string
    result = d_strlwr(str5);
    test_empty_string = (result == str5) && (str5[0] == '\0');

    // test 7: returns original pointer
    char str6[] = "TEST";
    result = d_strlwr(str6);
    test_returns_pointer = (result == str6);

    // build result tree
    group = d_test_object_new_interior("d_strlwr", 7);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("uppercase_convert",
                                           test_uppercase_convert,
                                           "converts uppercase to lowercase");
    group->elements[idx++] = D_ASSERT_TRUE("mixed_case",
                                           test_mixed_case,
                                           "handles mixed case");
    group->elements[idx++] = D_ASSERT_TRUE("preserve_lower",
                                           test_preserve_lower,
                                           "preserves lowercase letters");
    group->elements[idx++] = D_ASSERT_TRUE("preserve_nonalpha",
                                           test_preserve_nonalpha,
                                           "preserves non-alphabetic characters");
    group->elements[idx++] = D_ASSERT_TRUE("null_input",
                                           test_null_input,
                                           "handles NULL input");
    group->elements[idx++] = D_ASSERT_TRUE("empty_string",
                                           test_empty_string,
                                           "handles empty string");
    group->elements[idx++] = D_ASSERT_TRUE("returns_pointer",
                                           test_returns_pointer,
                                           "returns original pointer");

    return group;
}


/*
d_tests_dstring_strupr
  Tests d_strupr for uppercase conversion.
  Tests the following:
  - converts lowercase to uppercase
  - preserves uppercase letters
  - preserves non-alphabetic characters
  - handles NULL input
  - handles empty string
  - returns original pointer
  - modifies in place
*/
struct d_test_object*
d_tests_dstring_strupr
(
    void
)
{
    struct d_test_object* group;
    char                  str1[] = "hello world";
    char                  str2[] = "MiXeD CaSe 123!";
    char                  str3[] = "ALREADY UPPERCASE";
    char                  str4[] = "123!@#$%^";
    char                  str5[] = "";
    char*                 result;
    bool                  test_lowercase_convert;
    bool                  test_mixed_case;
    bool                  test_preserve_upper;
    bool                  test_preserve_nonalpha;
    bool                  test_null_input;
    bool                  test_empty_string;
    bool                  test_returns_pointer;
    size_t                idx;

    // test 1: converts lowercase
    result = d_strupr(str1);
    test_lowercase_convert = (strcmp(str1, "HELLO WORLD") == 0);

    // test 2: mixed case
    result = d_strupr(str2);
    test_mixed_case = (strcmp(str2, "MIXED CASE 123!") == 0);

    // test 3: preserves uppercase
    result = d_strupr(str3);
    test_preserve_upper = (strcmp(str3, "ALREADY UPPERCASE") == 0);

    // test 4: preserves non-alphabetic
    result = d_strupr(str4);
    test_preserve_nonalpha = (strcmp(str4, "123!@#$%^") == 0);

    // test 5: NULL input
    result = d_strupr(NULL);
    test_null_input = (result == NULL);

    // test 6: empty string
    result = d_strupr(str5);
    test_empty_string = (result == str5) && (str5[0] == '\0');

    // test 7: returns original pointer
    char str6[] = "test";
    result = d_strupr(str6);
    test_returns_pointer = (result == str6);

    // build result tree
    group = d_test_object_new_interior("d_strupr", 7);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("lowercase_convert",
                                           test_lowercase_convert,
                                           "converts lowercase to uppercase");
    group->elements[idx++] = D_ASSERT_TRUE("mixed_case",
                                           test_mixed_case,
                                           "handles mixed case");
    group->elements[idx++] = D_ASSERT_TRUE("preserve_upper",
                                           test_preserve_upper,
                                           "preserves uppercase letters");
    group->elements[idx++] = D_ASSERT_TRUE("preserve_nonalpha",
                                           test_preserve_nonalpha,
                                           "preserves non-alphabetic characters");
    group->elements[idx++] = D_ASSERT_TRUE("null_input",
                                           test_null_input,
                                           "handles NULL input");
    group->elements[idx++] = D_ASSERT_TRUE("empty_string",
                                           test_empty_string,
                                           "handles empty string");
    group->elements[idx++] = D_ASSERT_TRUE("returns_pointer",
                                           test_returns_pointer,
                                           "returns original pointer");

    return group;
}


/*
d_tests_dstring_case_conversion_all
  Runs all case conversion tests.
  Tests the following:
  - d_strlwr
  - d_strupr
*/
struct d_test_object*
d_tests_dstring_case_conversion_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("Case Conversion", 2);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dstring_strlwr();
    group->elements[idx++] = d_tests_dstring_strupr();

    return group;
}


/******************************************************************************
 * STRING MANIPULATION TESTS
 *****************************************************************************/

/*
d_tests_dstring_strrev
  Tests d_strrev for string reversal.
  Tests the following:
  - reverses normal string
  - handles single character
  - handles empty string
  - handles NULL input
  - returns original pointer
  - handles palindrome
  - modifies in place
*/
struct d_test_object*
d_tests_dstring_strrev
(
    void
)
{
    struct d_test_object* group;
    char                  str1[] = "Hello";
    char                  str2[] = "A";
    char                  str3[] = "";
    char                  str4[] = "racecar";
    char                  str5[] = "12345";
    char*                 result;
    bool                  test_normal_reverse;
    bool                  test_single_char;
    bool                  test_empty_string;
    bool                  test_null_input;
    bool                  test_returns_pointer;
    bool                  test_palindrome;
    bool                  test_in_place;
    size_t                idx;

    // test 1: normal reverse
    result = d_strrev(str1);
    test_normal_reverse = (strcmp(str1, "olleH") == 0);

    // test 2: single character
    result = d_strrev(str2);
    test_single_char = (strcmp(str2, "A") == 0);

    // test 3: empty string
    result = d_strrev(str3);
    test_empty_string = (result == str3) && (str3[0] == '\0');

    // test 4: NULL input
    result = d_strrev(NULL);
    test_null_input = (result == NULL);

    // test 5: returns original pointer
    result = d_strrev(str5);
    test_returns_pointer = (result == str5);

    // test 6: palindrome stays same
    result = d_strrev(str4);
    test_palindrome = (strcmp(str4, "racecar") == 0);

    // test 7: modifies in place
    char str6[] = "test";
    char* original = str6;
    result = d_strrev(str6);
    test_in_place = (result == original) && (strcmp(str6, "tset") == 0);

    // build result tree
    group = d_test_object_new_interior("d_strrev", 7);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = D_ASSERT_TRUE("normal_reverse",
                                           test_normal_reverse,
                                           "reverses normal string");
    group->elements[idx++] = D_ASSERT_TRUE("single_char",
                                           test_single_char,
                                           "handles single character");
    group->elements[idx++] = D_ASSERT_TRUE("empty_string",
                                           test_empty_string,
                                           "handles empty string");
    group->elements[idx++] = D_ASSERT_TRUE("null_input",
                                           test_null_input,
                                           "handles NULL input");
    group->elements[idx++] = D_ASSERT_TRUE("returns_pointer",
                                           test_returns_pointer,
                                           "returns original pointer");
    group->elements[idx++] = D_ASSERT_TRUE("palindrome",
                                           test_palindrome,
                                           "handles palindrome");
    group->elements[idx++] = D_ASSERT_TRUE("in_place",
                                           test_in_place,
                                           "modifies in place");

    return group;
}


/*
d_tests_dstring_manipulation_all
  Runs all string manipulation tests.
  Tests the following:
  - d_strrev
*/
struct d_test_object*
d_tests_dstring_manipulation_all
(
    void
)
{
    struct d_test_object* group;
    size_t                idx;

    group = d_test_object_new_interior("String Manipulation", 1);

    if (!group)
    {
        return NULL;
    }

    idx = 0;
    group->elements[idx++] = d_tests_dstring_strrev();

    return group;
}


