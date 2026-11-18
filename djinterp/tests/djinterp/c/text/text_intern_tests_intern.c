/*******************************************************************************
* djinterp [test]                                     text_intern_tests_intern.c
*
* The text_intern suite: every block, as the table is small.
*
*
* path:      /tests/djinterp/c/text/text_intern_tests_intern.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.17
*******************************************************************************/
#include "./text_intern_tests.h"  // corresponding header

// std
#include <stdio.h>   // snprintf
#include <string.h>  // strcmp, memcmp, strlen


/*
d_tests_text_intern_new
  A new table.
  Tests the following:
  - it has no names, and finds none
  - a symbol it does not have reads as "", of length 0
  - it frees empty, and frees holding names
*/
bool
d_tests_text_intern_new(void)
{
    struct dj_text_intern* table = d_text_intern_new();

    D_TEST_REQUIRE(table != NULL);
    D_TEST_EXPECT(d_text_intern_count(table) == 0u);
    D_TEST_EXPECT(d_text_intern_find(table, "a", 1u) == D_TEXT_INTERN_NONE);
    D_TEST_EXPECT(strcmp(d_text_intern_text(table, 0u), "") == 0);
    D_TEST_EXPECT(d_text_intern_length(table, 0u) == 0u);
    d_text_intern_free(table);

    table = d_text_intern_new();
    D_TEST_REQUIRE(table != NULL);
    D_TEST_EXPECT(d_text_intern_add(table, "name", 4u) == 0u);
    d_text_intern_free(table);

    return true;
}

/*
d_tests_text_intern_null
  NULL anywhere.
  Tests the following:
  - free, count, text and length take a NULL table
  - add and find with a NULL table or text give D_TEXT_INTERN_NONE, and add
    nothing
*/
bool
d_tests_text_intern_null(void)
{
    struct dj_text_intern* table = d_text_intern_new();

    D_TEST_REQUIRE(table != NULL);
    d_text_intern_free(NULL);
    D_TEST_EXPECT(d_text_intern_count(NULL) == 0u);
    D_TEST_EXPECT(strcmp(d_text_intern_text(NULL, 0u), "") == 0);
    D_TEST_EXPECT(d_text_intern_length(NULL, 0u) == 0u);
    D_TEST_EXPECT(d_text_intern_add(NULL, "a", 1u) == D_TEXT_INTERN_NONE);
    D_TEST_EXPECT(d_text_intern_find(NULL, "a", 1u) == D_TEXT_INTERN_NONE);
    D_TEST_EXPECT(d_text_intern_add(table, NULL, 0u) == D_TEXT_INTERN_NONE);
    D_TEST_EXPECT(d_text_intern_find(table, NULL, 0u) == D_TEXT_INTERN_NONE);
    D_TEST_EXPECT(d_text_intern_count(table) == 0u);
    d_text_intern_free(table);

    return true;
}

/*
d_tests_text_intern_dense
  Numbering.
  Tests the following:
  - names are numbered 0, 1, 2 in the order they are first added
  - adding a name again gives its own symbol, and adds nothing
*/
bool
d_tests_text_intern_dense(void)
{
    struct dj_text_intern* table = d_text_intern_new();

    D_TEST_REQUIRE(table != NULL);
    D_TEST_EXPECT(d_text_intern_add(table, "file", 4u) == 0u);
    D_TEST_EXPECT(d_text_intern_add(table, "line", 4u) == 1u);
    D_TEST_EXPECT(d_text_intern_add(table, "text", 4u) == 2u);
    D_TEST_EXPECT(d_text_intern_add(table, "line", 4u) == 1u);
    D_TEST_EXPECT(d_text_intern_add(table, "file", 4u) == 0u);
    D_TEST_EXPECT(d_text_intern_count(table) == 3u);
    d_text_intern_free(table);

    return true;
}

/*
d_tests_text_intern_counted
  A name is its length's bytes.
  Tests the following:
  - a name need not be terminated: the first three bytes of "abcdef" are
    "abc"
  - the empty name is a name, with a symbol of its own
  - a name and its prefix are two names
*/
bool
d_tests_text_intern_counted(void)
{
    struct dj_text_intern* table = d_text_intern_new();
    uint32_t               abc;
    uint32_t               empty;

    D_TEST_REQUIRE(table != NULL);
    abc = d_text_intern_add(table, "abcdef", 3u);
    D_TEST_EXPECT(abc == 0u);
    D_TEST_EXPECT(d_text_intern_add(table, "abc", 3u) == abc);
    D_TEST_EXPECT(d_text_intern_add(table, "abcdef", 6u) == 1u);
    D_TEST_EXPECT(d_text_intern_add(table, "ab", 2u) == 2u);

    empty = d_text_intern_add(table, "", 0u);
    D_TEST_EXPECT(empty == 3u);
    D_TEST_EXPECT(d_text_intern_add(table, "xyz", 0u) == empty);
    D_TEST_EXPECT(strcmp(d_text_intern_text(table, empty), "") == 0);
    D_TEST_EXPECT(d_text_intern_count(table) == 4u);
    d_text_intern_free(table);

    return true;
}

/*
d_tests_text_intern_find
  Finding without adding.
  Tests the following:
  - find gives an added name's symbol
  - a miss gives D_TEXT_INTERN_NONE and leaves the count as it was
*/
bool
d_tests_text_intern_find(void)
{
    struct dj_text_intern* table = d_text_intern_new();

    D_TEST_REQUIRE(table != NULL);
    (void)d_text_intern_add(table, "alpha", 5u);
    (void)d_text_intern_add(table, "beta", 4u);
    D_TEST_EXPECT(d_text_intern_find(table, "beta", 4u) == 1u);
    D_TEST_EXPECT(d_text_intern_find(table, "alpha", 5u) == 0u);
    D_TEST_EXPECT(d_text_intern_find(table, "gamma", 5u) ==
                  D_TEXT_INTERN_NONE);
    D_TEST_EXPECT(d_text_intern_find(table, "alph", 4u) ==
                  D_TEXT_INTERN_NONE);
    D_TEST_EXPECT(d_text_intern_count(table) == 2u);
    d_text_intern_free(table);

    return true;
}

/*
d_tests_text_intern_growth
  Many names.
  Tests the following:
  - ten thousand distinct names take symbols 0 to 9999, through every
    doubling of the buckets and the arena
  - afterwards each is found at its symbol and reads back as itself
*/
bool
d_tests_text_intern_growth(void)
{
    struct dj_text_intern* table = d_text_intern_new();
    char                   name[32];
    size_t                 at;
    bool                   ok = true;

    D_TEST_REQUIRE(table != NULL);

    for (at = 0u; at < 10000u; ++at)
    {
        const int length = snprintf(name, sizeof(name), "name_%lu",
                                    (unsigned long)at);

        if (d_text_intern_add(table, name, (size_t)length) != (uint32_t)at)
        {
            ok = false;
        }
    }

    D_TEST_EXPECT(ok);
    D_TEST_EXPECT(d_text_intern_count(table) == 10000u);

    for (at = 0u, ok = true; at < 10000u; ++at)
    {
        const int length = snprintf(name, sizeof(name), "name_%lu",
                                    (unsigned long)at);

        if ( (d_text_intern_find(table, name, (size_t)length) !=
              (uint32_t)at)                                          ||
             (strcmp(d_text_intern_text(table, (uint32_t)at), name) != 0) ||
             (d_text_intern_length(table, (uint32_t)at) != (size_t)length) )
        {
            ok = false;
        }
    }

    D_TEST_EXPECT(ok);
    d_text_intern_free(table);

    return true;
}

/*
d_tests_text_intern_text
  Reading a name back.
  Tests the following:
  - text gives the name, terminated, and length its length
  - the copy is the table's: the caller's buffer may change after the add
  - a symbol past the count reads as "", of length 0
*/
bool
d_tests_text_intern_text(void)
{
    struct dj_text_intern* table = d_text_intern_new();
    char                   buffer[8] = "keyword";
    uint32_t               symbol;

    D_TEST_REQUIRE(table != NULL);
    symbol = d_text_intern_add(table, buffer, 3u);
    D_TEST_REQUIRE(symbol == 0u);
    buffer[0] = 'X';
    D_TEST_EXPECT(strcmp(d_text_intern_text(table, symbol), "key") == 0);
    D_TEST_EXPECT(d_text_intern_length(table, symbol) == 3u);
    D_TEST_EXPECT(strcmp(d_text_intern_text(table, 1u), "") == 0);
    D_TEST_EXPECT(strcmp(d_text_intern_text(table, D_TEXT_INTERN_NONE),
                         "") == 0);
    D_TEST_EXPECT(d_text_intern_length(table, 1u) == 0u);
    d_text_intern_free(table);

    return true;
}

/*
d_tests_text_intern_embedded_nul
  A name holding a NUL.
  Tests the following:
  - "a\0b" and "a" are two names
  - length counts all three bytes, while text shows the part before the NUL
*/
bool
d_tests_text_intern_embedded_nul(void)
{
    struct dj_text_intern* table = d_text_intern_new();
    uint32_t               with;
    uint32_t               without;

    D_TEST_REQUIRE(table != NULL);
    with    = d_text_intern_add(table, "a\0b", 3u);
    without = d_text_intern_add(table, "a", 1u);
    D_TEST_EXPECT(with != without);
    D_TEST_EXPECT(d_text_intern_length(table, with) == 3u);
    D_TEST_EXPECT(memcmp(d_text_intern_text(table, with), "a\0b", 3u) == 0);
    D_TEST_EXPECT(strlen(d_text_intern_text(table, with)) == 1u);
    D_TEST_EXPECT(d_text_intern_find(table, "a\0b", 3u) == with);
    d_text_intern_free(table);

    return true;
}
