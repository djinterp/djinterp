/*******************************************************************************
* djinterp [cpp][test]                                 test_suite_tests_rows.cpp
*
* test_suite.h from C++, section 2: D_TEST_CASE, which writes a row of C's
* dj_test_case_spec and of C++'s test_spec alike, since the two have the same
* fields in the same order.
*
*
* path:      /tests/djinterp/cpp/modules/test/test_suite_tests_rows.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.15
*                                                            revised: 2026.11.15
*******************************************************************************/
#include "./test_suite_tests.hpp"  // corresponding header

// std
#include <cstring>  // std::strcmp
// djinterp
#include "djinterp/cpp/modules/test/test_defaults.hpp"  // test_spec


NS_DJINTERP
NS_TESTING


/*
suite_inner_passes
  An inner test that passes.
*/
static bool
suite_inner_passes()
{
    return true;
}

/*
tests_suite_case_c_row
  D_TEST_CASE writes a C row from C++.
  Tests the following:
  - its name is the function's, its descriptor as given, its function the
    one named
  - the row runs: one test, passed
*/
bool
tests_suite_case_c_row()
{
    const struct dj_test_case_spec row = D_TEST_CASE(suite_inner_passes,
                                                     "passes");

    D_TEST_REQUIRE(std::strcmp(row.name,
                               "suite_inner_passes") == 0);
    D_TEST_REQUIRE(std::strcmp(row.descriptor,
                               "passes") == 0);
    D_TEST_REQUIRE(row.fn == &suite_inner_passes);

    struct dj_test_summary summary = {};
    int                    code    = 0;
    std::string            report;

    D_TEST_REQUIRE(suite_run_row(row,
                                 summary,
                                 code,
                                 report));
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_PASSED);
    D_TEST_REQUIRE(report.find("[PASS] suite_inner_passes\n") !=
                   std::string::npos);

    return true;
}

/*
tests_suite_case_cpp_row
  D_TEST_CASE writes a row of C++'s test_spec, alone and in a table.
  Tests the following:
  - its three fields, as for the C row
  - a block_spec's rows, written by it, keep their order
*/
bool
tests_suite_case_cpp_row()
{
    const ::djinterp::test::test_spec  row   =
        D_TEST_CASE(suite_inner_passes,
                    "passes");
    const ::djinterp::test::block_spec block =
        { "b",
          "two rows",
          { D_TEST_CASE(tests_suite_case_c_row,
                        "first"),
            D_TEST_CASE(suite_inner_passes,
                        "second") } };

    D_TEST_REQUIRE(std::strcmp(row.name,
                               "suite_inner_passes") == 0);
    D_TEST_REQUIRE(std::strcmp(row.descriptor,
                               "passes") == 0);
    D_TEST_REQUIRE(row.fn == &suite_inner_passes);
    D_TEST_REQUIRE(block.tests.size() == 2);
    D_TEST_REQUIRE(std::strcmp(block.tests[0].name,
                               "tests_suite_case_c_row") == 0);
    D_TEST_REQUIRE(block.tests[1].fn == &suite_inner_passes);

    return true;
}


NS_END  // testing
NS_END  // djinterp
