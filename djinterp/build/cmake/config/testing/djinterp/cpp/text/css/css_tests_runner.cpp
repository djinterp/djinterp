/*******************************************************************************
* djinterp [cpp][text]                                      css_tests_runner.cpp
*
* The runner for css's suite: options, then the module.
*
*
* path:      /build/cmake/config/testing/djinterp/cpp/text/css/css_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/

// std
#include <cstdio>  // std::fprintf, std::printf
// djinterp
#include "css_tests.hpp"  // d_tests_css_all


/*
main
  The runner: the command line's options, the module, the report.
*/
int
main(
    int   _argc,
    char* _argv[]
)
{
    struct dj_test_suite_options options;
    struct dj_test_suite*        suite;
    int                          rc;

    d_test_suite_options_init(&options);

    rc = d_test_suite_options_parse(&options, _argc, _argv);

    if (rc != 0)
    {
        return (rc > 0) ? D_TEST_SUITE_EXIT_PASSED : D_TEST_SUITE_EXIT_ERROR;
    }

    suite = d_test_suite_new();

    if (!suite)
    {
        std::fprintf(stderr, "fatal: could not create the test suite\n");

        return D_TEST_SUITE_EXIT_ERROR;
    }

    d_test_suite_set_options(suite, &options);
    std::printf("== cpp/text CSS unit tests ==\n");

    d_tests_css_all(suite);

    rc = d_test_suite_run(suite, NULL);
    d_test_suite_free(suite);

    return rc;
}
