/*******************************************************************************
* djinterp [test]                                              dio_tests_scanf.c
*
* dio.h's suite, block "scanf": the framework's own checked scanf engine,
* style #38 (kept by the owner's ruling of 2026.11.04, and tested here).
*   Every buffer's size is passed as a size_t, as the contract asks, and is
* the buffer's true size: the library is handed a width derived from it, and
* AddressSanitizer checks the write against the buffer.
*
*
* path:      /tests/djinterp/c/dio_tests_scanf.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.05
*******************************************************************************/
#include "./dio_tests.h"

// std
#include <stdio.h>   // EOF, tmpfile, fputs, rewind
#include <string.h>  // memset, strcmp, memcmp


/*
d_tests_dio_sscanf_s_correct
  The call the register reproduced as a crash, "%7s %d", and its kin.
  Tests the following:
  - both fields are assigned, and the count is 2
  - a suppressed conversion takes no target and no size
*/
bool
d_tests_dio_sscanf_s_correct(void)
{
    char word[8];
    int  number;

    memset(word, 'X', sizeof(word));
    number = 0;
    D_TEST_REQUIRE(d_sscanf_s("hello 42", "%7s %d",
                              word, (size_t)sizeof(word), &number) == 2);
    D_TEST_REQUIRE(strcmp(word, "hello") == 0);
    D_TEST_REQUIRE(number == 42);

    number = 0;
    D_TEST_REQUIRE(d_sscanf_s("skip 7", "%*s %d", &number) == 1);
    D_TEST_REQUIRE(number == 7);

    return true;
}

/*
d_tests_dio_sscanf_s_size_zero
  A buffer of size 0 cannot hold even the terminator.
  Tests the following:
  - %s into size 0 is a matching failure: 0 assigned
  - the buffer is not written at all
*/
bool
d_tests_dio_sscanf_s_size_zero(void)
{
    char word[4];

    memset(word, 'X', sizeof(word));
    D_TEST_REQUIRE(d_sscanf_s("x", "%s", word, (size_t)0) == 0);
    D_TEST_REQUIRE(word[0] == 'X');

    return true;
}

/*
d_tests_dio_sscanf_s_size_one
  A buffer of size 1: room for one %c, or for a %s's terminator alone.
  Tests the following:
  - %c into size 1 reads one character
  - %s into size 1 fails, leaving the empty string
*/
bool
d_tests_dio_sscanf_s_size_one(void)
{
    char c;
    char word[1];

    c = '?';
    D_TEST_REQUIRE(d_sscanf_s("x", "%c", &c, (size_t)1) == 1);
    D_TEST_REQUIRE(c == 'x');

    word[0] = 'X';
    D_TEST_REQUIRE(d_sscanf_s("x", "%s", word, (size_t)1) == 0);
    D_TEST_REQUIRE(word[0] == '\0');

    return true;
}

/*
d_tests_dio_sscanf_s_scanset
  A %[ set is a string conversion with its own characters.
  Tests the following:
  - %[a-z] reads the lower-case run and stops at the digit
  - %[^,] reads up to the comma, and the rest of the format goes on
*/
bool
d_tests_dio_sscanf_s_scanset(void)
{
    char letters[8];
    char head[8];
    int  tail;

    D_TEST_REQUIRE(d_sscanf_s("abc123", "%[a-z]",
                              letters, (size_t)sizeof(letters)) == 1);
    D_TEST_REQUIRE(strcmp(letters, "abc") == 0);

    tail = 0;
    D_TEST_REQUIRE(d_sscanf_s("left,9", "%[^,],%d",
                              head, (size_t)sizeof(head), &tail) == 2);
    D_TEST_REQUIRE(strcmp(head, "left") == 0);
    D_TEST_REQUIRE(tail == 9);

    return true;
}

/*
d_tests_dio_sscanf_s_too_small
  A field its buffer cannot hold is a matching failure, never a truncation.
  Tests the following:
  - "hello" into a %s of size 3 assigns nothing and leaves ""
  - a %3c into size 2 fails the same way
  - a field that fits exactly (size 6 for "hello") succeeds
*/
bool
d_tests_dio_sscanf_s_too_small(void)
{
    char word[6];
    char pair[2];

    memset(word, 'X', sizeof(word));
    D_TEST_REQUIRE(d_sscanf_s("hello", "%s", word, (size_t)3) == 0);
    D_TEST_REQUIRE(word[0] == '\0');

    D_TEST_REQUIRE(d_sscanf_s("abc", "%3c", pair, (size_t)sizeof(pair)) == 0);

    D_TEST_REQUIRE(d_sscanf_s("hello", "%s", word, (size_t)sizeof(word)) == 1);
    D_TEST_REQUIRE(strcmp(word, "hello") == 0);

    return true;
}

/*
d_tests_dio_sscanf_s_size_type
  The size is Annex K's rsize_t, a size_t, on every platform: Microsoft's own
sscanf_s takes an unsigned int there, and a 64-bit size whose low 32 bits are
small tells the two apart. A %c writes exactly its width, so the large size
can be given without lying about the buffer the library writes.
  Tests the following:
  - SIZE_MAX as a %3c's size reads three characters
  - on a 64-bit size_t, 2^32 + 1 holds a %3c: the engine read all 64 bits
*/
bool
d_tests_dio_sscanf_s_size_type(void)
{
    char   three[3];
    size_t wide;

    memset(three, 'X', sizeof(three));
    D_TEST_REQUIRE(d_sscanf_s("abcd", "%3c", three, (size_t)-1) == 1);
    D_TEST_REQUIRE(memcmp(three, "abc", 3) == 0);

    if (sizeof(size_t) > 4)
    {
        // built in two steps, so a 32-bit size_t never sees the shift
        wide = (size_t)65536;
        wide = (wide * (size_t)65536) + 1u;

        memset(three, 'X', sizeof(three));
        D_TEST_REQUIRE(d_sscanf_s("wxyz", "%3c", three, wide) == 1);
        D_TEST_REQUIRE(memcmp(three, "wxy", 3) == 0);
    }

    return true;
}

/*
d_tests_dio_sscanf_s_constraints
  Runtime-constraint violations return EOF before anything is read.
  Tests the following:
  - a NULL buffer, or a NULL format, is EOF
  - input that runs out before the first conversion is EOF too
*/
bool
d_tests_dio_sscanf_s_constraints(void)
{
    int number;

    D_TEST_REQUIRE(d_sscanf_s(NULL, "%d", &number) == EOF);
    D_TEST_REQUIRE(d_sscanf_s("1", NULL) == EOF);
    D_TEST_REQUIRE(d_sscanf_s("", "%d", &number) == EOF);

    return true;
}

/*
d_tests_dio_fscanf_s_stream
  The stream form reads once, so it caps a field's width at its buffer less
one and then checks for more.
  Tests the following:
  - "hi 5" from a stream assigns both fields
  - a field too long for its buffer fails
*/
bool
d_tests_dio_fscanf_s_stream(void)
{
    FILE* stream;
    char  word[4];
    int   number;

    stream = tmpfile();
    D_TEST_REQUIRE(stream != NULL);
    fputs("hi 5 toolong", stream);
    rewind(stream);

    number = 0;
    D_TEST_REQUIRE(d_fscanf_s(stream, "%s %d",
                              word, (size_t)sizeof(word), &number) == 2);
    D_TEST_REQUIRE(strcmp(word, "hi") == 0);
    D_TEST_REQUIRE(number == 5);

    D_TEST_REQUIRE(d_fscanf_s(stream, "%s",
                              word, (size_t)sizeof(word)) == 0);
    D_TEST_REQUIRE(word[0] == '\0');

    fclose(stream);

    return true;
}
