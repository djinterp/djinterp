/******************************************************************************
* djinterp [test]                             test_session_tests_sa_status.c
*
*   Unit tests for test_session.h status and utility functions.
*
* path:      \test\test\test_session_tests_sa_status.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/

#include "./test_session_tests_sa.h"


//=============================================================================
// STATUS TESTS
//=============================================================================

/*
d_tests_sa_test_session_get_status
  Tests get_status on fresh, aborted, and NULL sessions.
  Tests the following:
  - fresh session status is CREATED
  - aborted session status is ABORTED
  - NULL session returns UNKNOWN
*/
bool
d_tests_sa_test_session_get_status
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session*  session;
    enum DTestSessionStatus status;

    printf("  --- Testing get_status ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();

    if (session)
    {
        status = d_test_session_get_status(session);

        if (!d_assert_standalone(
                status == D_TEST_SESSION_STATUS_CREATED,
                "fresh session status is CREATED",
                "fresh session status should be CREATED",
                _test_info))
        {
            all_passed = false;
        }

        d_test_session_abort(session);
        status = d_test_session_get_status(session);

        if (!d_assert_standalone(
                status == D_TEST_SESSION_STATUS_ABORTED,
                "aborted session status is ABORTED",
                "aborted session status should be ABORTED",
                _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }

    status = d_test_session_get_status(NULL);

    if (!d_assert_standalone(
            status == D_TEST_SESSION_STATUS_UNKNOWN,
            "NULL session status is UNKNOWN",
            "NULL session status should be UNKNOWN",
            _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s get_status unit test passed\n", D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s get_status unit test failed\n", D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_session_status_to_string
  Tests string conversion for key status values.
  Tests the following:
  - CREATED converts to "CREATED"
  - COMPLETED converts to "COMPLETED"
  - ABORTED converts to "ABORTED"
*/
bool
d_tests_sa_test_session_status_to_string
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    const char* str;

    printf("  --- Testing status_to_string ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    str = d_test_session_status_to_string(D_TEST_SESSION_STATUS_CREATED);

    if (str)
    {
        size_t len = d_strnlen(str, 64);

        if (!d_assert_standalone(
                d_strequals(str, len, "CREATED", 7),
                "CREATED converts to 'CREATED'",
                "CREATED should convert to 'CREATED'",
                _test_info))
        {
            all_passed = false;
        }
    }

    str = d_test_session_status_to_string(D_TEST_SESSION_STATUS_COMPLETED);

    if (str)
    {
        size_t len = d_strnlen(str, 64);

        if (!d_assert_standalone(
                d_strequals(str, len, "COMPLETED", 9),
                "COMPLETED converts to 'COMPLETED'",
                "COMPLETED should convert to 'COMPLETED'",
                _test_info))
        {
            all_passed = false;
        }
    }

    str = d_test_session_status_to_string(D_TEST_SESSION_STATUS_ABORTED);

    if (str)
    {
        size_t len = d_strnlen(str, 64);

        if (!d_assert_standalone(
                d_strequals(str, len, "ABORTED", 7),
                "ABORTED converts to 'ABORTED'",
                "ABORTED should convert to 'ABORTED'",
                _test_info))
        {
            all_passed = false;
        }
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s status_to_string unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s status_to_string unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_session_status_to_string_all
  Tests all 7 DTestSessionStatus enum values.
  Tests the following:
  - all values convert to non-NULL, non-empty strings
  - all strings are distinct
*/
bool
d_tests_sa_test_session_status_to_string_all
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    enum DTestSessionStatus statuses[7];
    const char*             strings[7];
    size_t                  i;
    size_t                  j;

    printf("  --- Testing status_to_string (all) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    statuses[0] = D_TEST_SESSION_STATUS_UNKNOWN;
    statuses[1] = D_TEST_SESSION_STATUS_CREATED;
    statuses[2] = D_TEST_SESSION_STATUS_RUNNING;
    statuses[3] = D_TEST_SESSION_STATUS_PAUSED;
    statuses[4] = D_TEST_SESSION_STATUS_COMPLETED;
    statuses[5] = D_TEST_SESSION_STATUS_ABORTED;
    statuses[6] = D_TEST_SESSION_STATUS_ERROR;

    for (i = 0; i < 7; i++)
    {
        strings[i] = d_test_session_status_to_string(statuses[i]);

        if (!d_assert_standalone(strings[i] != NULL,
                                 "status converts to non-NULL",
                                 "every status should convert to non-NULL",
                                 _test_info))
        {
            all_passed = false;
        }
    }

    for (i = 0; i < 7; i++)
    {
        if (strings[i])
        {
            if (!d_assert_standalone(
                    d_strnlen(strings[i], 64) > 0,
                    "status string is non-empty",
                    "every status string should be non-empty",
                    _test_info))
            {
                all_passed = false;
            }
        }
    }

    for (i = 0; i < 7; i++)
    {
        for (j = i + 1; j < 7; j++)
        {
            if ( (strings[i]) && (strings[j]) )
            {
                size_t li = d_strnlen(strings[i], 64);
                size_t lj = d_strnlen(strings[j], 64);

                if (!d_assert_standalone(
                        !d_strequals(strings[i], li, strings[j], lj),
                        "status strings are distinct",
                        "each status string should be distinct",
                        _test_info))
                {
                    all_passed = false;
                }
            }
        }
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s status_to_string (all) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s status_to_string (all) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_session_is_running
  Tests is_running on various states.
  Tests the following:
  - is_running on CREATED session returns false
  - is_running on NULL returns false
*/
bool
d_tests_sa_test_session_is_running
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;

    printf("  --- Testing is_running ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();

    if (session)
    {
        if (!d_assert_standalone(
                !d_test_session_is_running(session),
                "CREATED session is not running",
                "CREATED session should not be running",
                _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }

    if (!d_assert_standalone(
            !d_test_session_is_running(NULL),
            "NULL session is not running",
            "NULL session should not be running",
            _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s is_running unit test passed\n", D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s is_running unit test failed\n", D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_session_is_complete
  Tests is_complete on various states.
  Tests the following:
  - is_complete on CREATED session returns false
  - is_complete on ABORTED session returns true
  - is_complete on NULL returns false
*/
bool
d_tests_sa_test_session_is_complete
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;

    printf("  --- Testing is_complete ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();

    if (session)
    {
        if (!d_assert_standalone(
                !d_test_session_is_complete(session),
                "CREATED session is not complete",
                "CREATED session should not be complete",
                _test_info))
        {
            all_passed = false;
        }

        d_test_session_abort(session);

        if (!d_assert_standalone(
                d_test_session_is_complete(session),
                "ABORTED session is complete",
                "ABORTED session should be complete",
                _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }

    if (!d_assert_standalone(
            !d_test_session_is_complete(NULL),
            "NULL session is not complete",
            "NULL session should not be complete",
            _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s is_complete unit test passed\n", D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s is_complete unit test failed\n", D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


//=============================================================================
// UTILITY TESTS
//=============================================================================

/*
d_tests_sa_test_session_print
  Tests print does not crash.
*/
bool
d_tests_sa_test_session_print
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;

    printf("  --- Testing d_test_session_print ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();

    if (session)
    {
        d_test_session_print(session);

        if (!d_assert_standalone(true, "print did not crash",
                                 "print should not crash", _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }

    d_test_session_print(NULL);

    if (!d_assert_standalone(true, "print(NULL) did not crash",
                             "print(NULL) should not crash", _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s print unit test passed\n", D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s print unit test failed\n", D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_session_exit_code
  Tests exit_code on various states.
  Tests the following:
  - fresh session (no failures) returns 0
  - ABORTED session returns 2
  - NULL session returns 2
*/
bool
d_tests_sa_test_session_exit_code
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;
    int                    code;

    printf("  --- Testing exit_code ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();

    if (session)
    {
        // fresh session: no failures -> exit code 0
        code = d_test_session_exit_code(session);

        if (!d_assert_standalone(code == 0,
                                 "fresh session exit code is 0",
                                 "fresh session exit code should be 0",
                                 _test_info))
        {
            all_passed = false;
        }

        // abort: exit code 2
        d_test_session_abort(session);
        code = d_test_session_exit_code(session);

        if (!d_assert_standalone(code == 2,
                                 "aborted session exit code is 2",
                                 "aborted session exit code should be 2",
                                 _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }

    // NULL: exit code 2
    code = d_test_session_exit_code(NULL);

    if (!d_assert_standalone(code == 2,
                             "NULL session exit code is 2",
                             "NULL session exit code should be 2",
                             _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s exit_code unit test passed\n", D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s exit_code unit test failed\n", D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
