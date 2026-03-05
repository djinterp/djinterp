/******************************************************************************
* djinterp [test]                              test_session_tests_sa_config.c
*
*   Unit tests for test_session.h configuration functions.
*
* path:      \test\test\test_session_tests_sa_config.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/

#include "./test_session_tests_sa.h"


//=============================================================================
// CONFIGURATION TESTS
//=============================================================================

/*
d_tests_sa_test_session_set_get_option
  Tests setting and getting session options.
  Tests the following:
  - set_option with a valid option returns true
  - get_option retrieves the set value
  - get_option for an unset option returns NULL
*/
bool
d_tests_sa_test_session_set_get_option
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;
    static bool            abort_val = true;
    bool                   set_ok;
    void*                  got;
    void*                  missing;

    printf("  --- Testing set_option / get_option ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();

    if (!session)
    {
        printf("  %s session allocation failed\n", D_TEST_SYMBOL_FAIL);
        _test_info->tests_total++;

        return false;
    }

    // set an option
    set_ok = d_test_session_set_option(session,
                                        D_TEST_SESSION_OPT_ABORT_ON_FAILURE,
                                        &abort_val);

    if (!d_assert_standalone(set_ok,
                             "set_option returned true",
                             "set_option should return true",
                             _test_info))
    {
        all_passed = false;
    }

    // get the same option
    got = d_test_session_get_option(session,
                                    D_TEST_SESSION_OPT_ABORT_ON_FAILURE);

    if (!d_assert_standalone(got == &abort_val,
                             "get_option returns set value",
                             "get_option should return set value",
                             _test_info))
    {
        all_passed = false;
    }

    // get an option that was never set
    missing = d_test_session_get_option(session,
                                         D_TEST_SESSION_OPT_SHUFFLE);

    if (!d_assert_standalone(missing == NULL,
                             "get_option for unset returns NULL",
                             "get_option for unset should return NULL",
                             _test_info))
    {
        all_passed = false;
    }

    d_test_session_free(session);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s set_option / get_option unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s set_option / get_option unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_session_option_null
  Tests option functions with NULL session.
  Tests the following:
  - set_option on NULL returns false
  - get_option on NULL returns NULL
*/
bool
d_tests_sa_test_session_option_null
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    static bool val = true;

    printf("  --- Testing options (NULL) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    if (!d_assert_standalone(
            !d_test_session_set_option(NULL,
                D_TEST_SESSION_OPT_ABORT_ON_FAILURE, &val),
            "set_option on NULL returns false",
            "set_option on NULL should return false",
            _test_info))
    {
        all_passed = false;
    }

    if (!d_assert_standalone(
            d_test_session_get_option(NULL,
                D_TEST_SESSION_OPT_ABORT_ON_FAILURE) == NULL,
            "get_option on NULL returns NULL",
            "get_option on NULL should return NULL",
            _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s options (NULL) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s options (NULL) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_session_set_output_format
  Tests setting the output format.
  Tests the following:
  - set_output_format returns true for valid session
  - the format is stored correctly
  - VERBOSE format sets verbosity to VERBOSE
  - SILENT format sets verbosity to SILENT
*/
bool
d_tests_sa_test_session_set_output_format
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;

    printf("  --- Testing set_output_format ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();

    if (!session)
    {
        _test_info->tests_total++;

        return false;
    }

    // set to VERBOSE
    if (!d_assert_standalone(
            d_test_session_set_output_format(session,
                                              D_TEST_OUTPUT_VERBOSE),
            "set_output_format(VERBOSE) returned true",
            "set_output_format(VERBOSE) should return true",
            _test_info))
    {
        all_passed = false;
    }

    if (!d_assert_standalone(
            session->output.format == D_TEST_OUTPUT_VERBOSE,
            "format is VERBOSE",
            "format should be VERBOSE",
            _test_info))
    {
        all_passed = false;
    }

    if (!d_assert_standalone(
            session->output.verbosity == D_TEST_VERBOSITY_VERBOSE,
            "verbosity set to VERBOSE",
            "verbosity should be set to VERBOSE",
            _test_info))
    {
        all_passed = false;
    }

    // set to SILENT
    d_test_session_set_output_format(session, D_TEST_OUTPUT_SILENT);

    if (!d_assert_standalone(
            session->output.verbosity == D_TEST_VERBOSITY_SILENT,
            "verbosity set to SILENT",
            "verbosity should be set to SILENT",
            _test_info))
    {
        all_passed = false;
    }

    // NULL session
    if (!d_assert_standalone(
            !d_test_session_set_output_format(NULL, D_TEST_OUTPUT_CONSOLE),
            "set_output_format(NULL) returns false",
            "set_output_format(NULL) should return false",
            _test_info))
    {
        all_passed = false;
    }

    d_test_session_free(session);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s set_output_format unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s set_output_format unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_session_set_verbosity
  Tests setting verbosity directly.
  Tests the following:
  - set_verbosity returns true for valid session
  - verbosity is stored correctly
  - set_verbosity on NULL returns false
*/
bool
d_tests_sa_test_session_set_verbosity
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;

    printf("  --- Testing set_verbosity ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();

    if (session)
    {
        if (!d_assert_standalone(
                d_test_session_set_verbosity(session,
                                              D_TEST_VERBOSITY_DEBUG),
                "set_verbosity returned true",
                "set_verbosity should return true",
                _test_info))
        {
            all_passed = false;
        }

        if (!d_assert_standalone(
                session->output.verbosity == D_TEST_VERBOSITY_DEBUG,
                "verbosity is DEBUG",
                "verbosity should be DEBUG",
                _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }

    if (!d_assert_standalone(
            !d_test_session_set_verbosity(NULL, D_TEST_VERBOSITY_SILENT),
            "set_verbosity(NULL) returns false",
            "set_verbosity(NULL) should return false",
            _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s set_verbosity unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s set_verbosity unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_session_enable_color
  Tests enabling/disabling color output.
  Tests the following:
  - enable_color(false) sets use_color to false
  - enable_color(true) sets use_color to true
  - enable_color on NULL returns false
*/
bool
d_tests_sa_test_session_enable_color
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;

    printf("  --- Testing enable_color ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();

    if (session)
    {
        d_test_session_enable_color(session, false);

        if (!d_assert_standalone(!session->output.use_color,
                                 "color disabled",
                                 "color should be disabled",
                                 _test_info))
        {
            all_passed = false;
        }

        d_test_session_enable_color(session, true);

        if (!d_assert_standalone(session->output.use_color,
                                 "color re-enabled",
                                 "color should be re-enabled",
                                 _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }

    if (!d_assert_standalone(
            !d_test_session_enable_color(NULL, true),
            "enable_color(NULL) returns false",
            "enable_color(NULL) should return false",
            _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s enable_color unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s enable_color unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
