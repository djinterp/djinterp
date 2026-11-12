/*******************************************************************************
* djinterp [c][test]                              test_common_tests_operations.c
*
* test_common.h's suite, block "operations": the five operations on a status.
*   The values past INT32_MAX are the regression for the argument width: while
* the operations took int32_t, 0x100000001 reached them as 1 and was a valid
* "failed".
*
*
* path:      /tests/djinterp/c/modules/test/test_common_tests_operations.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.06
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "./test_common_tests.h"  // corresponding header


// D_TESTS_TEST_COMMON_WIDE
//   constant: a value whose low 32 bits are 1, which a 32-bit argument would
// have read as `failed`.
#define D_TESTS_TEST_COMMON_WIDE  INT64_C(0x100000001)

/*
d_tests_test_common_is_valid
  d_test_status_is_valid accepts exactly the five statuses.
  Tests the following:
  - 0 to 4 are valid
  - -1, 5 and 257 are not, nor are the extremes of int32_t and int64_t
  - a value past INT32_MAX whose low bits name a status is not
*/
bool
d_tests_test_common_is_valid(void)
{
    // each of the five is valid
    for (int64_t status = 0; status < D_TEST_STATUS_COUNT; status++)
    {
        D_TEST_REQUIRE(d_test_status_is_valid(status) == 1);
    }

    D_TEST_REQUIRE(d_test_status_is_valid(-1) == 0);
    D_TEST_REQUIRE(d_test_status_is_valid(D_TEST_STATUS_COUNT) == 0);
    D_TEST_REQUIRE(d_test_status_is_valid(257) == 0);
    D_TEST_REQUIRE(d_test_status_is_valid(INT32_MAX) == 0);
    D_TEST_REQUIRE(d_test_status_is_valid(INT32_MIN) == 0);
    D_TEST_REQUIRE(d_test_status_is_valid(INT64_MAX) == 0);
    D_TEST_REQUIRE(d_test_status_is_valid(INT64_MIN) == 0);
    D_TEST_REQUIRE(d_test_status_is_valid(D_TESTS_TEST_COMMON_WIDE) == 0);

    return true;
}

/*
d_tests_test_common_or_error
  d_test_status_or_error is the coercion rule test_event and test_object
share: a status passes through, anything else is stored as error.
  Tests the following:
  - each status comes back unchanged
  - -1, 5, 257 and a value past INT32_MAX come back as error
*/
bool
d_tests_test_common_or_error(void)
{
    // each of the five comes back as itself
    for (int64_t status = 0; status < D_TEST_STATUS_COUNT; status++)
    {
        D_TEST_REQUIRE((int64_t)d_test_status_or_error(status) == status);
    }

    D_TEST_REQUIRE(d_test_status_or_error(-1) == D_TEST_STATUS_ERROR);
    D_TEST_REQUIRE(d_test_status_or_error(D_TEST_STATUS_COUNT) ==
                   D_TEST_STATUS_ERROR);
    D_TEST_REQUIRE(d_test_status_or_error(257) == D_TEST_STATUS_ERROR);
    D_TEST_REQUIRE(d_test_status_or_error(D_TESTS_TEST_COMMON_WIDE) ==
                   D_TEST_STATUS_ERROR);

    return true;
}

/*
d_tests_test_common_is_terminal
  d_test_status_is_terminal: an outcome rather than a state on the way to one.
  Tests the following:
  - passed, failed, skipped and error are terminal; pending is not
  - an invalid value is not
*/
bool
d_tests_test_common_is_terminal(void)
{
    D_TEST_REQUIRE(d_test_status_is_terminal(D_TEST_STATUS_PASSED) == 1);
    D_TEST_REQUIRE(d_test_status_is_terminal(D_TEST_STATUS_FAILED) == 1);
    D_TEST_REQUIRE(d_test_status_is_terminal(D_TEST_STATUS_SKIPPED) == 1);
    D_TEST_REQUIRE(d_test_status_is_terminal(D_TEST_STATUS_ERROR) == 1);
    D_TEST_REQUIRE(d_test_status_is_terminal(D_TEST_STATUS_PENDING) == 0);

    D_TEST_REQUIRE(d_test_status_is_terminal(-1) == 0);
    D_TEST_REQUIRE(d_test_status_is_terminal(D_TEST_STATUS_COUNT) == 0);
    D_TEST_REQUIRE(d_test_status_is_terminal(D_TESTS_TEST_COMMON_WIDE) == 0);

    return true;
}

/*
d_tests_test_common_name
  d_test_status_name gives the spelling a parity record carries.
  Tests the following:
  - each status has its own name
  - -1, 5 and a value past INT32_MAX are "unknown"
*/
bool
d_tests_test_common_name(void)
{
    D_TEST_REQUIRE(d_test_suite_str_equal(d_test_status_name(D_TEST_STATUS_PASSED),
                                          "passed"));
    D_TEST_REQUIRE(d_test_suite_str_equal(d_test_status_name(D_TEST_STATUS_FAILED),
                                          "failed"));
    D_TEST_REQUIRE(d_test_suite_str_equal(d_test_status_name(D_TEST_STATUS_SKIPPED),
                                          "skipped"));
    D_TEST_REQUIRE(d_test_suite_str_equal(d_test_status_name(D_TEST_STATUS_PENDING),
                                          "pending"));
    D_TEST_REQUIRE(d_test_suite_str_equal(d_test_status_name(D_TEST_STATUS_ERROR),
                                          "error"));

    D_TEST_REQUIRE(d_test_suite_str_equal(d_test_status_name(-1),
                                          "unknown"));
    D_TEST_REQUIRE(d_test_suite_str_equal(d_test_status_name(D_TEST_STATUS_COUNT),
                                          "unknown"));
    D_TEST_REQUIRE(d_test_suite_str_equal(d_test_status_name(D_TESTS_TEST_COMMON_WIDE),
                                          "unknown"));

    return true;
}

/*
d_tests_test_common_worse_of
  d_test_status_worse_of is where a parent's status comes from, so its order
is a decision: error > failed > skipped > pending > passed.
  Tests the following:
  - for every pair of statuses, the result is the one ranked higher
  - the result does not depend on the order of the arguments
  - an invalid argument on either side gives error
*/
bool
d_tests_test_common_worse_of(void)
{
    // severity rank by status value, as the header's comment orders them
    static const int rank[D_TEST_STATUS_COUNT] = { 0, 3, 2, 1, 4 };

    int64_t want = 0;

    // every pair of statuses, against the rank table
    for (int64_t a = 0; a < D_TEST_STATUS_COUNT; a++)
    {
        // the worse of a and each b
        for (int64_t b = 0; b < D_TEST_STATUS_COUNT; b++)
        {
            want = (rank[a] >= rank[b]) ? a : b;

            D_TEST_REQUIRE((int64_t)d_test_status_worse_of(a,
                                                           b) == want);
            D_TEST_REQUIRE(d_test_status_worse_of(a,
                                                  b) ==
                           d_test_status_worse_of(b,
                                                  a));
        }

        D_TEST_REQUIRE(d_test_status_worse_of(a,
                                              -1) == D_TEST_STATUS_ERROR);
        D_TEST_REQUIRE(d_test_status_worse_of(D_TEST_STATUS_COUNT,
                                              a) ==
                       D_TEST_STATUS_ERROR);
        D_TEST_REQUIRE(d_test_status_worse_of(a,
                                              D_TESTS_TEST_COMMON_WIDE) ==
                       D_TEST_STATUS_ERROR);
    }

    return true;
}
