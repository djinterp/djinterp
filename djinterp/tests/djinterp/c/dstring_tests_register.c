/*******************************************************************************
* djinterp [test]                                       dstring_tests_register.c
*
* dstring.h's suite, block "register": the register's decisions on dstring.
*
*
* path:      /tests/djinterp/c/dstring_tests_register.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.05
*******************************************************************************/
#include "./dstring_tests.h"

// std
#include <stdio.h>   // sprintf
#include <string.h>  // strcmp, strlen
// re_std
#include "../../../inc/re_std/c/cstdint/dstdint.h"  // SIZE_MAX


/*
d_tests_dstring_tokenize_updates_size
  Decision 13: tokenizing writes NULs into the text, and the size follows.
  Tests the following:
  - the first call returns the first token and sets the size to its length
  - a later call returns the next token and leaves the size alone
  - size and strlen of the text agree after each call
*/
bool
d_tests_dstring_tokenize_updates_size(void)
{
    struct dj_string* s;
    char*             save;
    char*             token;

    s    = d_string_new_from_cstr("alpha,beta,gamma");
    save = NULL;
    D_TEST_REQUIRE(s != NULL);

    token = d_string_tokenize(s, ",", &save);
    D_TEST_REQUIRE( (token != NULL) && (strcmp(token, "alpha") == 0) );
    D_TEST_REQUIRE(d_string_size(s) == 5u);
    D_TEST_REQUIRE(d_string_size(s) == strlen(d_string_cstr(s)));

    token = d_string_tokenize(NULL, ",", &save);
    D_TEST_REQUIRE( (token != NULL) && (strcmp(token, "beta") == 0) );
    D_TEST_REQUIRE(d_string_size(s) == 5u);

    token = d_string_tokenize(NULL, ",", &save);
    D_TEST_REQUIRE( (token != NULL) && (strcmp(token, "gamma") == 0) );
    D_TEST_REQUIRE(d_string_tokenize(NULL, ",", &save) == NULL);

    d_string_free(s);

    return true;
}

/*
d_tests_dstring_split_tokens
  Decision 19: d_string_split, split into static inline helpers, splits as
it did.
  Tests the following:
  - runs of delimiters make no empty tokens
  - the source string is unchanged
  - an empty string gives one empty token
  - a NULL argument gives 0 and no array
*/
bool
d_tests_dstring_split_tokens(void)
{
    struct dj_string*  s;
    struct dj_string*  empty;
    struct dj_string** tokens;
    size_t             count;

    s = d_string_new_from_cstr(",a,,bb,ccc,");
    D_TEST_REQUIRE(s != NULL);

    count = d_string_split(s, ",", &tokens);
    D_TEST_REQUIRE(count == 3u);
    D_TEST_REQUIRE(tokens != NULL);
    D_TEST_REQUIRE(strcmp(d_string_cstr(tokens[0]), "a") == 0);
    D_TEST_REQUIRE(strcmp(d_string_cstr(tokens[1]), "bb") == 0);
    D_TEST_REQUIRE(strcmp(d_string_cstr(tokens[2]), "ccc") == 0);
    D_TEST_REQUIRE(strcmp(d_string_cstr(s), ",a,,bb,ccc,") == 0);
    d_string_split_free(tokens, count);

    empty = d_string_new();
    D_TEST_REQUIRE(empty != NULL);
    count = d_string_split(empty, ",", &tokens);
    D_TEST_REQUIRE(count == 1u);
    D_TEST_REQUIRE( (tokens != NULL) && (d_string_size(tokens[0]) == 0u) );
    d_string_split_free(tokens, count);

    tokens = NULL;
    D_TEST_REQUIRE(d_string_split(NULL, ",", &tokens) == 0u);
    D_TEST_REQUIRE(tokens == NULL);
    D_TEST_REQUIRE(d_string_split(s, NULL, &tokens) == 0u);
    D_TEST_REQUIRE(d_string_split(s, ",", NULL) == 0u);

    d_string_free(empty);
    d_string_free(s);

    return true;
}

/*
d_tests_dstring_split_growth
  Decision 19, and the upgrade audit's site at dstring.c:3444: the token
array starts at 8 and doubles, its sizes now taken through d_ckd_mul.
  Tests the following:
  - 1000 tokens, past seven doublings, all arrive in order
*/
bool
d_tests_dstring_split_growth(void)
{
    struct dj_string*  s;
    struct dj_string** tokens;
    char               text[1000 * 5 + 1];
    char               expect[8];
    size_t             count;
    size_t             i;
    size_t             at;

    at = 0;

    for (i = 0; i < 1000u; i++)
    {
        at += (size_t)sprintf(text + at, "%03lu;", (unsigned long)i);
    }

    s = d_string_new_from_cstr(text);
    D_TEST_REQUIRE(s != NULL);

    count = d_string_split(s, ";", &tokens);
    D_TEST_REQUIRE(count == 1000u);

    for (i = 0; i < count; i++)
    {
        sprintf(expect, "%03lu", (unsigned long)i);
        D_TEST_REQUIRE(strcmp(d_string_cstr(tokens[i]), expect) == 0);
    }

    d_string_split_free(tokens, count);
    d_string_free(s);

    return true;
}

/*
d_tests_dstring_split_only_delimiters
  Decision 24: no token, no array.
  Tests the following:
  - a string of delimiters only gives 0 and a NULL array
*/
bool
d_tests_dstring_split_only_delimiters(void)
{
    struct dj_string*  s;
    struct dj_string** tokens;

    s      = d_string_new_from_cstr(",;,;;");
    tokens = (struct dj_string**)&s;  // anything but NULL
    D_TEST_REQUIRE(s != NULL);

    D_TEST_REQUIRE(d_string_split(s, ",;", &tokens) == 0u);
    D_TEST_REQUIRE(tokens == NULL);

    d_string_free(s);

    return true;
}

/*
d_tests_dstring_npos_counts_clamp
  Decision 22: a SIZE_MAX count is "the rest", as std::string's npos is, and
cannot wrap.
  Tests the following:
  - substr from 2 takes the rest
  - replace and replace_cstr from 1 replace the rest
  - erase from 1 erases the rest
*/
bool
d_tests_dstring_npos_counts_clamp(void)
{
    struct dj_string* s;
    struct dj_string* sub;
    struct dj_string* r;

    s = d_string_new_from_cstr("abcdef");
    r = d_string_new_from_cstr("XY");
    D_TEST_REQUIRE( (s != NULL) && (r != NULL) );

    sub = d_string_substr(s, 2, SIZE_MAX);
    D_TEST_REQUIRE(sub != NULL);
    D_TEST_REQUIRE(strcmp(d_string_cstr(sub), "cdef") == 0);
    d_string_free(sub);

    D_TEST_REQUIRE(d_string_replace(s, 1, SIZE_MAX, r));
    D_TEST_REQUIRE(strcmp(d_string_cstr(s), "aXY") == 0);

    D_TEST_REQUIRE(d_string_replace_cstr(s, 2, SIZE_MAX, "zz"));
    D_TEST_REQUIRE(strcmp(d_string_cstr(s), "aXzz") == 0);

    D_TEST_REQUIRE(d_string_erase(s, 1, SIZE_MAX));
    D_TEST_REQUIRE(strcmp(d_string_cstr(s), "a") == 0);
    D_TEST_REQUIRE(d_string_size(s) == 1u);

    d_string_free(r);
    d_string_free(s);

    return true;
}

/*
d_tests_dstring_self_operands
  Decisions 23 and 121: a string may be its own operand; the copy is made
before the buffer can move.
  Tests the following:
  - append(s, s) doubles the text
  - insert(s, 1, s) puts the whole text inside itself
  - assign(s, s) leaves it as it was
  - replace(s, 0, 1, s) replaces a character with the whole text
*/
bool
d_tests_dstring_self_operands(void)
{
    struct dj_string* s;

    s = d_string_new_from_cstr("ab");
    D_TEST_REQUIRE(s != NULL);

    D_TEST_REQUIRE(d_string_append(s, s));
    D_TEST_REQUIRE(strcmp(d_string_cstr(s), "abab") == 0);

    D_TEST_REQUIRE(d_string_insert(s, 1, s));
    D_TEST_REQUIRE(strcmp(d_string_cstr(s), "aabab" "bab") == 0);

    D_TEST_REQUIRE(d_string_assign(s, s));
    D_TEST_REQUIRE(strcmp(d_string_cstr(s), "aababbab") == 0);
    D_TEST_REQUIRE(d_string_size(s) == 8u);

    D_TEST_REQUIRE(d_string_replace(s, 0, 1, s));
    D_TEST_REQUIRE(strcmp(d_string_cstr(s), "aababbab" "ababbab") == 0);

    d_string_free(s);

    return true;
}
