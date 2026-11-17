/*******************************************************************************
* djinterp [cpp][test]                                      test_suite_tests.hpp
*
* DTest's C face, test_suite.h, as C++ suites use it: its check macros in C++
* test functions, and D_TEST_CASE writing rows of C++'s tables as well as C's.
*   The C face's own suite (tests/djinterp/c/modules/test) holds its behaviour;
* this one holds what only C++ can show: a template's comma inside a check, a
* row of C++'s test_spec, and a text check made under C++'s run_module, where
* no C suite is running. Each check of a C suite's counts runs that suite as
* an inner one, reporting to a temporary file, as the C face's suite does.
*   The suite's own rows are written with D_TEST_CASE, and its runner hands
* them to run_module, so the suite runs only if those rows are right.
*
*
* path:      /tests/djinterp/cpp/modules/test/test_suite_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.15
*                                                            revised: 2026.11.15
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FIXTURES
    --------
    1.  Inner suites
2.  TESTS
    -----
    1.  Checks
    2.  Rows
    3.  Text
3.  SPEC
    ----
    1.  The provider
*/

#ifndef DJINTERP_TESTS_CPP_MODULES_TEST_TEST_SUITE_TESTS_HPP
#define DJINTERP_TESTS_CPP_MODULES_TEST_TEST_SUITE_TESTS_HPP 1

// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::FILE, std::tmpfile, std::rewind, std::fread
#include <string>   // std::string
// djinterp
#include "djinterp/cpp/djinterp.hpp"                     // framework root
#include "djinterp/c/modules/test/test_suite.h"          // under test
#ifdef DTEST_SPEC_MODE
    #include "djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec
#endif  // DTEST_SPEC_MODE


NS_DJINTERP
NS_TESTING


//==============================================================================
// 1.  FIXTURES
//==============================================================================
#ifndef DTEST_SPEC_MODE


// 1.1    Inner suites
//------------------------------------------------------------------------------
// suite_run_row
//   function: runs one C row as a one-block module in an inner suite that
// reports to a temporary file, and gives back its counts, exit code and
// report. Returns false when the inner suite could not be made.
inline
bool
suite_run_row(
    const struct dj_test_case_spec& _row,
    struct dj_test_summary&         _summary,
    int&                            _code,
    std::string&                    _report
)
{
    const struct dj_test_block_spec  block  = { "inner_block",
                                                "the inner block",
                                                &_row,
                                                1 };
    const struct dj_test_module_spec module = { "inner",
                                                "the inner module",
                                                &block,
                                                1 };
    struct dj_test_suite_options     options = {};
    std::FILE* const                 stream  = std::tmpfile();
    struct dj_test_suite* const      suite   = d_test_suite_new();

    _report.clear();

    // both made, or neither kept
    if ( (!stream) ||
         (!suite) )
    {
        // the stream, when it was made
        if (stream)
        {
            std::fclose(stream);
        }

        d_test_suite_free(suite);

        return false;
    }

    d_test_suite_options_init(&options);
    options.table  = false;
    options.stream = stream;
    d_test_suite_set_options(suite,
                             &options);
    d_test_suite_add_module(suite,
                            &module);
    _code = d_test_suite_run(suite,
                             &_summary);
    d_test_suite_free(suite);
    std::rewind(stream);

    char        buffer[512] = {0};
    std::size_t got         = 0;

    // the report, read back whole
    while ( (got = std::fread(buffer,
                              1,
                              sizeof(buffer),
                              stream)) != 0 )
    {
        _report.append(buffer,
                       got);
    }

    std::fclose(stream);

    return true;
}


#endif  // !DTEST_SPEC_MODE


//==============================================================================
// 2.  TESTS
//==============================================================================


// 2.1    Checks
//------------------------------------------------------------------------------
bool tests_suite_require_commas();

bool tests_suite_expect_commas();

// 2.2    Rows
//------------------------------------------------------------------------------
bool tests_suite_case_c_row();

bool tests_suite_case_cpp_row();

// 2.3    Text
//------------------------------------------------------------------------------
bool tests_suite_text_in_cpp();

bool tests_suite_text_no_suite();


//==============================================================================
// 3.  SPEC
//==============================================================================
#ifdef DTEST_SPEC_MODE


// 3.1    The provider
//------------------------------------------------------------------------------
// suite_spec
//   function: the suite's spec, every row written by D_TEST_CASE.
inline
::djinterp::test::module_spec
suite_spec()
{
    return ::djinterp::test::module_spec{
        "test_suite",
        "test_suite.h, DTest's C face, from C++: its check macros and rows",
        {
            ::djinterp::test::block_spec{
                "checks",
                "D_TEST_REQUIRE and D_TEST_EXPECT with a template's comma",
                {
                    D_TEST_CASE(tests_suite_require_commas,
                                "REQUIRE takes is_same<A, B>, unwrapped"),
                    D_TEST_CASE(tests_suite_expect_commas,
                                "EXPECT takes it too, and goes on")
                }
            },
            ::djinterp::test::block_spec{
                "rows",
                "D_TEST_CASE, in a C table and in a C++ one",
                {
                    D_TEST_CASE(tests_suite_case_c_row,
                                "a dj_test_case_spec row, from C++"),
                    D_TEST_CASE(tests_suite_case_cpp_row,
                                "a test_spec row, named after its function")
                }
            },
            ::djinterp::test::block_spec{
                "text",
                "text checks from C++",
                {
                    D_TEST_CASE(tests_suite_text_in_cpp,
                                "REQUIRE_TEXT on std::string's bytes"),
                    D_TEST_CASE(tests_suite_text_no_suite,
                                "under run_module, a text check answers alone")
                }
            }
        }
    };
}


#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTS_CPP_MODULES_TEST_TEST_SUITE_TESTS_HPP
