/*******************************************************************************
* djinterp [cpp][test]                                 test_suite_tests_text.cpp
*
* test_suite.h from C++, section 3: text checks on C++'s strings, and a text
* check made under C++'s run_module, where no C suite is running.
*
*
* path:      /tests/djinterp/cpp/modules/test/test_suite_tests_text.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.15
*                                                            revised: 2026.11.15
*******************************************************************************/
#include "./test_suite_tests.hpp"  // corresponding header

// std
#include <string>  // std::string


NS_DJINTERP
NS_TESTING


/*
suite_inner_text
  An inner test that compares a std::string's bytes, CRLF and all, with an LF
text, then with one that differs.
*/
static bool
suite_inner_text()
{
    const std::string written("frame   2  shell\r\n");

    D_TEST_REQUIRE_TEXT(d_test_text_buffer(written.data(),
                                           written.size()),
                        d_test_text_cstr("frame   2  shell\n"));
    D_TEST_REQUIRE_TEXT(d_test_text_buffer(written.data(),
                                           written.size()),
                        d_test_text_cstr("frame  2  shell\n"));

    return true;
}

/*
tests_suite_text_in_cpp
  D_TEST_REQUIRE_TEXT on a std::string's bytes.
  Tests the following:
  - the first check holds, a CR before the LF ignored; the second fails and
    its note is lane G's, word for word
*/
bool
tests_suite_text_in_cpp()
{
    const struct dj_test_case_spec row = D_TEST_CASE(suite_inner_text,
                                                     "two text checks");

    struct dj_test_summary summary = {};
    int                    code    = 0;
    std::string            report;

    D_TEST_REQUIRE(suite_run_row(row,
                                 summary,
                                 code,
                                 report));
    D_TEST_REQUIRE(summary.checks == 2);
    D_TEST_REQUIRE(summary.checks_failed == 1);
    D_TEST_REQUIRE(report.find(": line 1 differs, at column 8\n"
                               "                 got:  "
                               "\"frame   2  shell\"\n"
                               "                 want: "
                               "\"frame  2  shell\"\n") != std::string::npos);

    return true;
}

/*
tests_suite_text_no_suite
  Under run_module no C suite is running, so a text check with a NULL suite
answers by its return alone, and writes a failure to stderr.
  Tests the following:
  - the same texts: true
  - texts that differ, and a file that is not there: false (two lines on
    stderr say so, named as expected failures)
*/
bool
tests_suite_text_no_suite()
{
    D_TEST_REQUIRE(d_test_suite_check_text(NULL,
                                           "same",
                                           d_test_text_cstr("a\r\nb\r\n"),
                                           d_test_text_cstr("a\nb\n")));
    D_TEST_REQUIRE(!d_test_suite_check_text(NULL,
                                            "an expected failure",
                                            d_test_text_cstr("a\n"),
                                            d_test_text_cstr("b\n")));
    D_TEST_REQUIRE(!d_test_suite_check_text(NULL,
                                            "an expected failure",
                                            d_test_text_cstr("a\n"),
                                            d_test_text_file(
                                                "test_suite_not_there")));

    return true;
}


NS_END  // testing
NS_END  // djinterp
