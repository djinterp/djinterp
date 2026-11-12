/*******************************************************************************
* djinterp [c]                                         djinterp_tests_register.c
*
* djinterp.h's suite, block "register": the register's decisions on the root.
*
*
* path:      /tests/djinterp/c/djinterp_tests_register.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.05
*******************************************************************************/
#include "./djinterp_tests.h"  // corresponding header

// std
#include <limits.h>  // SSIZE_MAX, through djinterp.h


/*
d_tests_djinterp_index_macros
  Decision 21: D_IS_VALID_INDEX, D_IS_VALID_INDEX_N and D_INDEX_IN_BOUNDS are
d_index_is_valid.
  Tests the following:
  - for a count of 3: 0, 2, -1 and -3 are valid; 3 and -4 are not
  - a count of 0 has no valid index of either sign
  - each macro agrees with d_index_is_valid
*/
bool
d_tests_djinterp_index_macros(void)
{
    static const dj_index indices[] = { 0, 1, 2, 3, -1, -3, -4 };
    size_t                i;

    D_TEST_REQUIRE(D_IS_VALID_INDEX(0, 3));
    D_TEST_REQUIRE(D_IS_VALID_INDEX(2, 3));
    D_TEST_REQUIRE(D_IS_VALID_INDEX(-1, 3));
    D_TEST_REQUIRE(D_IS_VALID_INDEX(-3, 3));
    D_TEST_REQUIRE(!D_IS_VALID_INDEX(3, 3));
    D_TEST_REQUIRE(!D_IS_VALID_INDEX(-4, 3));
    D_TEST_REQUIRE(!D_IS_VALID_INDEX(0, 0));
    D_TEST_REQUIRE(!D_IS_VALID_INDEX(-1, 0));

    for (i = 0; i < sizeof(indices) / sizeof(indices[0]); i++)
    {
        D_TEST_REQUIRE(D_IS_VALID_INDEX(indices[i], 3) ==
                       d_index_is_valid(indices[i], 3));
        D_TEST_REQUIRE(D_IS_VALID_INDEX_N(indices[i], 3) ==
                       d_index_is_valid(indices[i], 3));
        D_TEST_REQUIRE(D_INDEX_IN_BOUNDS(indices[i], 3) ==
                       d_index_is_valid(indices[i], 3));
    }

    return true;
}

/*
d_tests_djinterp_index_most_negative
  Decision 21: the old macros negated the index, which is undefined for the
most negative value; under UBSan this test would have stopped there.
  Tests the following:
  - the most negative dj_index is invalid for any count that fits
  - so is the most positive, for a count below it
  - each argument is evaluated once
*/
bool
d_tests_djinterp_index_most_negative(void)
{
    dj_index lowest;
    dj_index index;
    size_t   count;

    lowest = (dj_index)(-SSIZE_MAX - 1);

    D_TEST_REQUIRE(!D_IS_VALID_INDEX(lowest, 3));
    D_TEST_REQUIRE(!D_IS_VALID_INDEX_N(lowest, 3));
    D_TEST_REQUIRE(!D_IS_VALID_INDEX(lowest, (size_t)SSIZE_MAX));
    D_TEST_REQUIRE(!D_IS_VALID_INDEX((dj_index)SSIZE_MAX, 3));

    index = 1;
    count = 3;
    D_TEST_REQUIRE(D_IS_VALID_INDEX(index++, count++));
    D_TEST_REQUIRE( (index == 2) && (count == 4u) );

    return true;
}
