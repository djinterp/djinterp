/******************************************************************************
* djinterp [test]                              test_session_tests_sa_stats.c
*
*   Unit tests for test_session.h statistics functions.
*
* path:      \test\test\test_session_tests_sa_stats.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/

#include "./test_session_tests_sa.h"

/*
d_tests_sa_test_session_get_stats
  Tests get_stats returns a valid pointer.
  Tests the following:
  - get_stats on a valid session returns non-NULL
  - get_stats on NULL returns NULL
*/
bool
d_tests_sa_test_session_get_stats
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session*          session;
    const struct d_test_statistics* stats;

    printf("  --- Testing get_stats ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();

    if (session)
    {
        stats = d_test_session_get_stats(session);

        if (!d_assert_standalone(stats != NULL,
                                 "get_stats returns non-NULL",
                                 "get_stats should return non-NULL",
                                 _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }

    if (!d_assert_standalone(
            d_test_session_get_stats(NULL) == NULL,
            "get_stats(NULL) returns NULL",
            "get_stats(NULL) should return NULL",
            _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s get_stats unit test passed\n", D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s get_stats unit test failed\n", D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_session_all_passed
  Tests all_passed on fresh and NULL sessions.
  Tests the following:
  - fresh session with no failures: all_passed returns true
  - NULL session: all_passed returns false
*/
bool
d_tests_sa_test_session_all_passed
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;

    printf("  --- Testing all_passed ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();

    if (session)
    {
        // fresh session: failure_count == 0 -> all_passed
        if (!d_assert_standalone(
                d_test_session_all_passed(session),
                "fresh session all_passed is true",
                "fresh session all_passed should be true",
                _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }

    if (!d_assert_standalone(
            !d_test_session_all_passed(NULL),
            "all_passed(NULL) returns false",
            "all_passed(NULL) should return false",
            _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s all_passed unit test passed\n", D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s all_passed unit test failed\n", D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_session_total_counters
  Tests total_run, total_passed, total_failed, total_skipped on fresh session.
  Tests the following:
  - all counters are 0 on a fresh session
  - all counters return 0 for NULL session
*/
bool
d_tests_sa_test_session_total_counters
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;

    printf("  --- Testing total counters ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    session = d_test_session_new();

    if (session)
    {
        if (!d_assert_standalone(
                d_test_session_total_run(session) == 0,
                "total_run is 0",
                "total_run should be 0",
                _test_info))
        {
            all_passed = false;
        }

        if (!d_assert_standalone(
                d_test_session_total_passed(session) == 0,
                "total_passed is 0",
                "total_passed should be 0",
                _test_info))
        {
            all_passed = false;
        }

        if (!d_assert_standalone(
                d_test_session_total_failed(session) == 0,
                "total_failed is 0",
                "total_failed should be 0",
                _test_info))
        {
            all_passed = false;
        }

        if (!d_assert_standalone(
                d_test_session_total_skipped(session) == 0,
                "total_skipped is 0",
                "total_skipped should be 0",
                _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }

    // NULL session
    if (!d_assert_standalone(
            d_test_session_total_run(NULL) == 0,
            "total_run(NULL) is 0",
            "total_run(NULL) should be 0",
            _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s total counters unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s total counters unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_session_duration_ms
  Tests duration_ms on a fresh session.
  Tests the following:
  - duration_ms on NULL returns 0.0
  - duration_ms on unrun session does not crash
*/
bool
d_tests_sa_test_session_duration_ms
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;
    double                 dur;

    printf("  --- Testing duration_ms ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    if (!d_assert_standalone(
            d_test_session_duration_ms(NULL) == 0.0,
            "duration_ms(NULL) is 0.0",
            "duration_ms(NULL) should be 0.0",
            _test_info))
    {
        all_passed = false;
    }

    session = d_test_session_new();

    if (session)
    {
        dur = d_test_session_duration_ms(session);

        // unrun session: start_time_ms is 0, so duration may be large
        // (current_time - 0); at minimum it should not be negative
        if (!d_assert_standalone(dur >= 0.0,
                                 "duration_ms is non-negative",
                                 "duration_ms should be non-negative",
                                 _test_info))
        {
            all_passed = false;
        }

        d_test_session_free(session);
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s duration_ms unit test passed\n", D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s duration_ms unit test failed\n", D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}


/*
d_tests_sa_test_session_stats_null
  Tests all stats functions with NULL.
*/
bool
d_tests_sa_test_session_stats_null
(
    struct d_test_counter* _test_info
)
{
    size_t initial_tests_passed;
    bool   all_passed;

    printf("  --- Testing stats (NULL) ---\n");

    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;

    if (!d_assert_standalone(
            d_test_session_total_passed(NULL) == 0,
            "total_passed(NULL) is 0",
            "total_passed(NULL) should be 0",
            _test_info))
    {
        all_passed = false;
    }

    if (!d_assert_standalone(
            d_test_session_total_failed(NULL) == 0,
            "total_failed(NULL) is 0",
            "total_failed(NULL) should be 0",
            _test_info))
    {
        all_passed = false;
    }

    if (!d_assert_standalone(
            d_test_session_total_skipped(NULL) == 0,
            "total_skipped(NULL) is 0",
            "total_skipped(NULL) should be 0",
            _test_info))
    {
        all_passed = false;
    }

    if (all_passed)
    {
        _test_info->tests_passed++;
        printf("  %s stats (NULL) unit test passed\n",
               D_TEST_SYMBOL_PASS);
    }
    else
    {
        printf("  %s stats (NULL) unit test failed\n",
               D_TEST_SYMBOL_FAIL);
    }

    _test_info->tests_total++;

    return (_test_info->tests_passed > initial_tests_passed);
}
