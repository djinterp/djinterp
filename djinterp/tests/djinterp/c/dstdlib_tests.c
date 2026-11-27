/*******************************************************************************
* djinterp [c]                                                   dstdlib_tests.c
*
* dstdlib.h's suite: reading, setting and removing variables of the
* environment, with the same answers on every platform.
*
*
* path:      /tests/djinterp/c/dstdlib_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.26
*                                                            revised: 2026.11.26
*******************************************************************************/
#include "./dstdlib_tests.h"  // corresponding header


// D_TESTS_DSTDLIB_NAME
//   constant: the variable most tests set, and remove.
#define D_TESTS_DSTDLIB_NAME "DJINTERP_DSTDLIB_TEST"


//==============================================================================
// 1.  TESTS
//==============================================================================


// 1.1    Reading a variable
//------------------------------------------------------------------------------

/*
d_tests_dstdlib_copy_set
  A variable set to "value" is read back.
  Tests the following:
  - d_setenv answers 0
  - d_getenv_copy answers 0, with the value and its length, 5
*/
bool
d_tests_dstdlib_copy_set(void)
{
    char   value[16];
    size_t length;

    memset(value, 'X', sizeof(value));
    length = 99u;
    D_TEST_REQUIRE(d_setenv(D_TESTS_DSTDLIB_NAME, "value") == 0);
    D_TEST_REQUIRE(d_getenv_copy(D_TESTS_DSTDLIB_NAME, value,
                                 sizeof(value), &length) == 0);
    D_TEST_REQUIRE(strcmp(value, "value") == 0);
    D_TEST_REQUIRE(length == 5u);
    D_TEST_REQUIRE(d_unsetenv(D_TESTS_DSTDLIB_NAME) == 0);

    return true;
}

/*
d_tests_dstdlib_copy_bounds
  A value of five characters needs six bytes.
  Tests the following:
  - five bytes: ERANGE, the buffer the empty string, the length 5
  - six bytes: 0, the value
  - a NULL length is allowed
*/
bool
d_tests_dstdlib_copy_bounds(void)
{
    char   value[6];
    size_t length;

    D_TEST_REQUIRE(d_setenv(D_TESTS_DSTDLIB_NAME, "value") == 0);

    memset(value, 'X', sizeof(value));
    length = 0u;
    D_TEST_REQUIRE(d_getenv_copy(D_TESTS_DSTDLIB_NAME, value, 5u,
                                 &length) == ERANGE);
    D_TEST_REQUIRE(value[0] == '\0');
    D_TEST_REQUIRE(length == 5u);

    memset(value, 'X', sizeof(value));
    D_TEST_REQUIRE(d_getenv_copy(D_TESTS_DSTDLIB_NAME, value, 6u,
                                 NULL) == 0);
    D_TEST_REQUIRE(strcmp(value, "value") == 0);
    D_TEST_REQUIRE(d_unsetenv(D_TESTS_DSTDLIB_NAME) == 0);

    return true;
}

/*
d_tests_dstdlib_copy_query
  The question form: no buffer, size 0.
  Tests the following:
  - set: ERANGE, with its length
  - not set: ENOENT, with a length of 0
*/
bool
d_tests_dstdlib_copy_query(void)
{
    size_t length;

    D_TEST_REQUIRE(d_setenv(D_TESTS_DSTDLIB_NAME, "seven 7") == 0);
    length = 0u;
    D_TEST_REQUIRE(d_getenv_copy(D_TESTS_DSTDLIB_NAME, NULL, 0u,
                                 &length) == ERANGE);
    D_TEST_REQUIRE(length == 7u);

    D_TEST_REQUIRE(d_unsetenv(D_TESTS_DSTDLIB_NAME) == 0);
    length = 99u;
    D_TEST_REQUIRE(d_getenv_copy(D_TESTS_DSTDLIB_NAME, NULL, 0u,
                                 &length) == ENOENT);
    D_TEST_REQUIRE(length == 0u);

    return true;
}

/*
d_tests_dstdlib_copy_unset
  A variable removed is not set.
  Tests the following:
  - ENOENT, the buffer the empty string, the length 0
*/
bool
d_tests_dstdlib_copy_unset(void)
{
    char   value[8];
    size_t length;

    D_TEST_REQUIRE(d_setenv(D_TESTS_DSTDLIB_NAME, "gone") == 0);
    D_TEST_REQUIRE(d_unsetenv(D_TESTS_DSTDLIB_NAME) == 0);

    memset(value, 'X', sizeof(value));
    length = 99u;
    D_TEST_REQUIRE(d_getenv_copy(D_TESTS_DSTDLIB_NAME, value,
                                 sizeof(value), &length) == ENOENT);
    D_TEST_REQUIRE(value[0] == '\0');
    D_TEST_REQUIRE(length == 0u);

    return true;
}

/*
d_tests_dstdlib_dup
  The copy in storage the caller frees.
  Tests the following:
  - set: 0, the value, freed with free()
  - not set: ENOENT, and NULL
  - a NULL out-pointer: EINVAL
*/
bool
d_tests_dstdlib_dup(void)
{
    char* value;

    D_TEST_REQUIRE(d_setenv(D_TESTS_DSTDLIB_NAME, "a longer value") == 0);
    value = NULL;
    D_TEST_REQUIRE(d_getenv_dup(D_TESTS_DSTDLIB_NAME, &value) == 0);
    D_TEST_REQUIRE(value != NULL);
    D_TEST_REQUIRE(strcmp(value, "a longer value") == 0);
    free(value);

    D_TEST_REQUIRE(d_unsetenv(D_TESTS_DSTDLIB_NAME) == 0);
    value = (char*)"not touched";
    D_TEST_REQUIRE(d_getenv_dup(D_TESTS_DSTDLIB_NAME, &value) == ENOENT);
    D_TEST_REQUIRE(value == NULL);

    D_TEST_REQUIRE(d_getenv_dup(D_TESTS_DSTDLIB_NAME, NULL) == EINVAL);

    return true;
}

/*
d_tests_dstdlib_names
  What no variable can be named.
  Tests the following:
  - NULL, "" and "A=B" are EINVAL for each of the four
  - a NULL buffer with a size that is not 0 is EINVAL
*/
bool
d_tests_dstdlib_names(void)
{
    static const char* const names[] = { NULL, "", "A=B" };
    char                     value[4];
    char*                    copy;
    size_t                   i;

    for (i = 0u; i < (sizeof(names) / sizeof(names[0])); ++i)
    {
        copy = NULL;
        D_TEST_REQUIRE(d_getenv_copy(names[i], value, sizeof(value),
                                     NULL) == EINVAL);
        D_TEST_REQUIRE(d_getenv_dup(names[i], &copy) == EINVAL);
        D_TEST_REQUIRE(copy == NULL);
        D_TEST_REQUIRE(d_setenv(names[i], "x") == EINVAL);
        D_TEST_REQUIRE(d_unsetenv(names[i]) == EINVAL);
    }

    D_TEST_REQUIRE(d_getenv_copy(D_TESTS_DSTDLIB_NAME, NULL, 4u,
                                 NULL) == EINVAL);
    D_TEST_REQUIRE(d_setenv(D_TESTS_DSTDLIB_NAME, NULL) == EINVAL);

    return true;
}


// 1.2    Setting and removing a variable
//------------------------------------------------------------------------------

/*
d_tests_dstdlib_set_replace
  A second value replaces the first.
  Tests the following:
  - "one", then "two": "two" is read
*/
bool
d_tests_dstdlib_set_replace(void)
{
    char value[8];

    D_TEST_REQUIRE(d_setenv(D_TESTS_DSTDLIB_NAME, "one") == 0);
    D_TEST_REQUIRE(d_setenv(D_TESTS_DSTDLIB_NAME, "two") == 0);
    D_TEST_REQUIRE(d_getenv_copy(D_TESTS_DSTDLIB_NAME, value,
                                 sizeof(value), NULL) == 0);
    D_TEST_REQUIRE(strcmp(value, "two") == 0);
    D_TEST_REQUIRE(d_unsetenv(D_TESTS_DSTDLIB_NAME) == 0);

    return true;
}

/*
d_tests_dstdlib_set_empty
  A value of "" is no value: the variable goes.
  Tests the following:
  - d_setenv(name, "") answers 0
  - the variable is then not set: ENOENT
*/
bool
d_tests_dstdlib_set_empty(void)
{
    char value[8];

    D_TEST_REQUIRE(d_setenv(D_TESTS_DSTDLIB_NAME, "here") == 0);
    D_TEST_REQUIRE(d_setenv(D_TESTS_DSTDLIB_NAME, "") == 0);
    D_TEST_REQUIRE(d_getenv_copy(D_TESTS_DSTDLIB_NAME, value,
                                 sizeof(value), NULL) == ENOENT);

    return true;
}

/*
d_tests_dstdlib_unset_absent
  Removing what is not there.
  Tests the following:
  - d_unsetenv of a variable never set answers 0, twice
*/
bool
d_tests_dstdlib_unset_absent(void)
{
    D_TEST_REQUIRE(d_unsetenv("DJINTERP_DSTDLIB_NEVER_SET") == 0);
    D_TEST_REQUIRE(d_unsetenv("DJINTERP_DSTDLIB_NEVER_SET") == 0);

    return true;
}
