/*******************************************************************************
* djinterp [test]                                     string_fn_tests_register.c
*
* string_fn.h's suite, block "register": the register's decisions on
* string_fn.
*
*
* path:      /tests/djinterp/c/string_fn_tests_register.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.05
*******************************************************************************/
#include "./string_fn_tests.h"  // corresponding header

// std
#include <errno.h>   // EINVAL, ERANGE, ENOENT
#include <string.h>  // strcmp, strlen, memset


/*
d_tests_string_fn_strerror_r
  Decision 11: d_strerror_r delegates to the platform.
  Tests the following:
  - a known number is described, by more than the old three-entry table had
  - an unknown number still gets a description
  - a NULL buffer or a size of 0 is EINVAL
  - a buffer too small is ERANGE and is left untouched
*/
bool
d_tests_string_fn_strerror_r(void)
{
    char buffer[128];
    char tiny[2];

    memset(buffer, 0, sizeof(buffer));
    D_TEST_REQUIRE(d_strerror_r(ENOENT, buffer, sizeof(buffer)) == 0);
    D_TEST_REQUIRE(strlen(buffer) > 0u);
    D_TEST_REQUIRE(strcmp(buffer, "Unknown error") != 0);

    memset(buffer, 0, sizeof(buffer));
    D_TEST_REQUIRE(d_strerror_r(987654, buffer, sizeof(buffer)) == 0);
    D_TEST_REQUIRE(strlen(buffer) > 0u);

    D_TEST_REQUIRE(d_strerror_r(EINVAL, NULL, 16) == EINVAL);
    D_TEST_REQUIRE(d_strerror_r(EINVAL, buffer, 0) == EINVAL);

    tiny[0] = 'X';
    tiny[1] = 'Y';
    D_TEST_REQUIRE(d_strerror_r(ENOENT, tiny, sizeof(tiny)) == ERANGE);
    D_TEST_REQUIRE( (tiny[0] == 'X') && (tiny[1] == 'Y') );

    return true;
}

/*
d_tests_string_fn_ascii_only
  Decision 12: classes and case folding are ASCII.
  Tests the following:
  - ASCII letters, digits and spaces are classified
  - a byte above 0x7F is never a letter or a space
  - case folding leaves such a byte alone, and a case-insensitive compare
    does not fold Latin-1's É to é
*/
bool
d_tests_string_fn_ascii_only(void)
{
    char upper[] = "AB\xC9z";
    char lower[] = "ab\xC9z";

    D_TEST_REQUIRE(d_str_is_alpha("abcXYZ", 6));
    D_TEST_REQUIRE(d_str_is_alnum("a1B2", 4));
    D_TEST_REQUIRE(d_str_is_whitespace(" \t\n", 3));
    D_TEST_REQUIRE(!d_str_is_alpha("\xE9", 1));
    D_TEST_REQUIRE(!d_str_is_alnum("a\xE9", 2));
    D_TEST_REQUIRE(!d_str_is_whitespace("\xA0", 1));

    D_TEST_REQUIRE(d_strlwr(upper) == upper);
    D_TEST_REQUIRE(strcmp(upper, lower) == 0);
    D_TEST_REQUIRE(d_strcasecmp("\xC9", "\xE9") != 0);
    D_TEST_REQUIRE(d_strcasecmp("ABC", "abc") == 0);

    return true;
}

/*
d_tests_string_fn_casestr_embedded_nul
  Decision 30: d_strcasestr_index compares by length, so an embedded NUL is a
character like any other.
  Tests the following:
  - a needle that starts with NUL is found after one in the haystack
  - the search still folds case around it
  - a needle that is not there is D_STRING_NPOS
*/
bool
d_tests_string_fn_casestr_embedded_nul(void)
{
    static const char haystack[] = { 'a', 'b', '\0', 'C', 'd' };
    static const char needle[]   = { '\0', 'c' };
    static const char absent[]   = { '\0', 'x' };

    D_TEST_REQUIRE(d_strcasestr_index(haystack, sizeof(haystack),
                                      needle, sizeof(needle)) == 2);
    D_TEST_REQUIRE(d_strcasestr_index(haystack, sizeof(haystack),
                                      "BC", 2) == D_STRING_NPOS);
    D_TEST_REQUIRE(d_strcasestr_index(haystack, sizeof(haystack),
                                      absent, sizeof(absent)) ==
                   D_STRING_NPOS);
    D_TEST_REQUIRE(d_strcasestr_index(haystack, sizeof(haystack),
                                      "AB", 2) == 0);

    return true;
}
