/*******************************************************************************
* djinterp [cpp][test]                               test_suite_tests_checks.cpp
*
* test_suite.h from C++, section 1: D_TEST_REQUIRE and D_TEST_EXPECT with a
* template's comma, which the preprocessor splits unless the macro takes its
* arguments whole.
*
*
* path:      /tests/djinterp/cpp/modules/test/test_suite_tests_checks.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.15
*                                                            revised: 2026.11.15
*******************************************************************************/
#include "./test_suite_tests.hpp"  // corresponding header

// std
#include <string>       // std::string
#include <type_traits>  // std::is_same


NS_DJINTERP
NS_TESTING


/*
suite_inner_requires
  An inner test whose second of three requires, a template's comma in each,
fails; the third is never made.
*/
static bool
suite_inner_requires()
{
    D_TEST_REQUIRE(std::is_same<int, int>::value);
    D_TEST_REQUIRE(std::is_same<int, long>::value);
    D_TEST_REQUIRE(std::is_same<long, long>::value);

    return true;
}

/*
tests_suite_require_commas
  D_TEST_REQUIRE takes is_same<A, B> without parentheses.
  Tests the following:
  - two requires are made, the second failed, and the test stopped there
  - the failed one's text is the expression as written, comma and all
*/
bool
tests_suite_require_commas()
{
    const struct dj_test_case_spec row = D_TEST_CASE(suite_inner_requires,
                                                     "three requires");

    struct dj_test_summary summary = {};
    int                    code    = 0;
    std::string            report;

    D_TEST_REQUIRE(suite_run_row(row,
                                 summary,
                                 code,
                                 report));
    D_TEST_REQUIRE(summary.checks == 2);
    D_TEST_REQUIRE(summary.checks_failed == 1);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_FAILED);
    D_TEST_REQUIRE(report.find(": std::is_same<int, long>::value\n") !=
                   std::string::npos);

    return true;
}

// suite_expect_reached
//   variable: how far the expecting inner test went, and what it saw.
static int suite_expect_reached = 0;

/*
suite_inner_expects
  An inner test whose second of three expectations fails; it goes on to the
end, records what each gave, and returns true.
*/
static bool
suite_inner_expects()
{
    const bool first  = D_TEST_EXPECT(std::is_same<int, int>::value);
    const bool second = D_TEST_EXPECT(std::is_same<int, long>::value);
    const bool third  = D_TEST_EXPECT(std::is_same<long, long>::value);

    suite_expect_reached = ( (first) &&
                             (!second) &&
                             (third) ) ? 3 : -1;

    return true;
}

/*
tests_suite_expect_commas
  D_TEST_EXPECT takes is_same<A, B> too, and goes on after a failure.
  Tests the following:
  - all three are made, each giving its check's result
  - the failed one fails the test, though the test returned true
*/
bool
tests_suite_expect_commas()
{
    const struct dj_test_case_spec row = D_TEST_CASE(suite_inner_expects,
                                                     "three expectations");

    struct dj_test_summary summary = {};
    int                    code    = 0;
    std::string            report;

    suite_expect_reached = 0;
    D_TEST_REQUIRE(suite_run_row(row,
                                 summary,
                                 code,
                                 report));
    D_TEST_REQUIRE(suite_expect_reached == 3);
    D_TEST_REQUIRE(summary.checks == 3);
    D_TEST_REQUIRE(summary.checks_failed == 1);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_FAILED);

    return true;
}


NS_END  // testing
NS_END  // djinterp
