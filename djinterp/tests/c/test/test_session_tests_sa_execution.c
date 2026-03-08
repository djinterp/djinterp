/******************************************************************************
* djinterp [test]                          test_session_tests_sa_execution.c
*
*   Unit tests for test_session.h execution functions.
*
* path:      \test\test\test_session_tests_sa_execution.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/

#include "./test_session_tests_sa.h"


//=============================================================================
// EXECUTION TESTS
//=============================================================================

/*
d_tests_sa_test_session_run_empty
  Tests running a session with no children.
  Tests the following:
  - run on empty session returns true (vacuous pass)
  - status transitions to COMPLETED
*/
bool
d_tests_sa_test_session_run_empty
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;
    bool                   result;

    printf("  --- Testing d_test_session_run (empty) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    // use SILENT output to avoid cluttering test output
    session = d_test_session_new();

    if (!session)
    {
        _test_info->tests_total++;

        return false;
    }

    d_test_session_set_output_format(session, D_TEST_OUTPUT_SILENT);
    result = d_test_session_run(session);

    if (!d_assert_standalone(result,
                             "empty session run returned true",
                             "empty session run should return true",
                             _test_info))
    {
        all_passed = false;
    }

    if (!d_assert_standalone(
            session->status == D_TEST_SESSION_STATUS_COMPLETED,
            "status is COMPLETED",
            "status should be COMPLETED after run",
            _test_info))
    {
        all_passed = false;
    }

    d_test_session_free(session);

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s run (empty) unit test passed\n", D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s run (empty) unit test failed\n", D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_session_run_null
  Tests running with NULL session.
  Tests the following:
  - run(NULL) returns false
  - run_child(NULL,...) returns false
  - run_by_name(NULL,...) returns false
*/
bool
d_tests_sa_test_session_run_null
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;

    printf("  --- Testing run (NULL) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    if (!d_assert_standalone(!d_test_session_run(NULL),
                             "run(NULL) returns false",
                             "run(NULL) should return false",
                             _test_info))
    {
        all_passed = false;
    }

    if (!d_assert_standalone(!d_test_session_run_child(NULL, 0),
                             "run_child(NULL) returns false",
                             "run_child(NULL) should return false",
                             _test_info))
    {
        all_passed = false;
    }

    if (!d_assert_standalone(!d_test_session_run_by_name(NULL, "test"),
                             "run_by_name(NULL) returns false",
                             "run_by_name(NULL) should return false",
                             _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s run (NULL) unit test passed\n", D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s run (NULL) unit test failed\n", D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_session_run_child_invalid
  Tests run_child with invalid index.
  Tests the following:
  - run_child on empty session returns false
  - run_child with out-of-bounds index returns false
*/
bool
d_tests_sa_test_session_run_child_invalid
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;

    printf("  --- Testing run_child (invalid) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();

    if (session)
    {
        if (!d_assert_standalone(
                !d_test_session_run_child(session, 0),
                "run_child(0) on empty returns false",
                "run_child(0) on empty should return false",
                _test_info))
        {
            all_passed = false;
        }

        if (!d_assert_standalone(
                !d_test_session_run_child(session, 9999),
                "run_child(9999) returns false",
                "run_child(9999) should return false",
                _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s run_child (invalid) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s run_child (invalid) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_session_abort
  Tests aborting a session.
  Tests the following:
  - abort returns true for valid session
  - status transitions to ABORTED
  - abort on NULL returns false
*/
bool
d_tests_sa_test_session_abort
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;

    printf("  --- Testing d_test_session_abort ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();

    if (session)
    {
        if (!d_assert_standalone(d_test_session_abort(session),
                                 "abort returned true",
                                 "abort should return true",
                                 _test_info))
        {
            all_passed = false;
        }

        if (!d_assert_standalone(
                session->status == D_TEST_SESSION_STATUS_ABORTED,
                "status is ABORTED",
                "status should be ABORTED",
                _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }

    if (!d_assert_standalone(!d_test_session_abort(NULL),
                             "abort(NULL) returns false",
                             "abort(NULL) should return false",
                             _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s abort unit test passed\n", D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s abort unit test failed\n", D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_session_reset
  Tests resetting a session after abort.
  Tests the following:
  - reset returns true
  - status returns to CREATED
  - failure_count returns to 0
  - reset on NULL returns false
*/
bool
d_tests_sa_test_session_reset
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;

    printf("  --- Testing d_test_session_reset ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();

    if (session)
    {
        // abort first, then reset
        d_test_session_abort(session);

        if (!d_assert_standalone(d_test_session_reset(session),
                                 "reset returned true",
                                 "reset should return true",
                                 _test_info))
        {
            all_passed = false;
        }

        if (!d_assert_standalone(
                session->status == D_TEST_SESSION_STATUS_CREATED,
                "status is CREATED after reset",
                "status should be CREATED after reset",
                _test_info))
        {
            all_passed = false;
        }

        if (!d_assert_standalone(session->failure_count == 0,
                                 "failure_count is 0 after reset",
                                 "failure_count should be 0 after reset",
                                 _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }

    if (!d_assert_standalone(!d_test_session_reset(NULL),
                             "reset(NULL) returns false",
                             "reset(NULL) should return false",
                             _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s reset unit test passed\n", D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s reset unit test failed\n", D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_session_pause_resume
  Tests pause and resume on a non-running session.
  Tests the following:
  - pause on a CREATED session returns false (not running)
  - resume on a CREATED session returns false (not paused)
  - pause on NULL returns false
  - resume on NULL returns false
*/
bool
d_tests_sa_test_session_pause_resume
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;

    printf("  --- Testing pause / resume ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();

    if (session)
    {
        // pause on non-running session fails
        if (!d_assert_standalone(
                !d_test_session_pause(session),
                "pause on CREATED returns false",
                "pause on CREATED should return false",
                _test_info))
        {
            all_passed = false;
        }

        // resume on non-paused session fails
        if (!d_assert_standalone(
                !d_test_session_resume(session),
                "resume on CREATED returns false",
                "resume on CREATED should return false",
                _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }

    if (!d_assert_standalone(!d_test_session_pause(NULL),
                             "pause(NULL) returns false",
                             "pause(NULL) should return false",
                             _test_info))
    {
        all_passed = false;
    }

    if (!d_assert_standalone(!d_test_session_resume(NULL),
                             "resume(NULL) returns false",
                             "resume(NULL) should return false",
                             _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s pause / resume unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s pause / resume unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
