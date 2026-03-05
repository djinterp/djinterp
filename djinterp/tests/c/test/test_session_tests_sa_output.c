/******************************************************************************
* djinterp [test]                             test_session_tests_sa_output.c
*
*   Unit tests for test_session.h output functions.
*
* path:      \test\test\test_session_tests_sa_output.c
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.16
******************************************************************************/

#include "./test_session_tests_sa.h"

/*
d_tests_sa_test_session_write_header
  Tests write_header does not crash on valid and NULL sessions.
*/
bool d_tests_sa_test_session_write_header(struct d_test_counter* _test_info)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;
    printf("  --- Testing write_header ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;
    session = d_test_session_new();
    if (session)
    {
        d_test_session_set_output_format(session, D_TEST_OUTPUT_SILENT);
        d_test_session_write_header(session);
        if (!d_assert_standalone(true, "write_header did not crash",
                                 "write_header should not crash", _test_info))
        { all_passed = false; }
        d_test_session_free(session);
    }
    d_test_session_write_header(NULL);
    if (!d_assert_standalone(true, "write_header(NULL) did not crash",
                             "write_header(NULL) should not crash", _test_info))
    { all_passed = false; }
    if (all_passed) { _test_info->tests_passed++; printf("  %s write_header unit test passed\n", D_TEST_SYMBOL_PASS); }
    else { printf("  %s write_header unit test failed\n", D_TEST_SYMBOL_FAIL); }
    _test_info->tests_total++;
    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_session_write_footer
  Tests write_footer does not crash.
*/
bool d_tests_sa_test_session_write_footer(struct d_test_counter* _test_info)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;
    printf("  --- Testing write_footer ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;
    session = d_test_session_new();
    if (session)
    {
        d_test_session_set_output_format(session, D_TEST_OUTPUT_SILENT);
        d_test_session_write_footer(session);
        if (!d_assert_standalone(true, "write_footer did not crash",
                                 "write_footer should not crash", _test_info))
        { all_passed = false; }
        d_test_session_free(session);
    }
    d_test_session_write_footer(NULL);
    if (!d_assert_standalone(true, "write_footer(NULL) did not crash",
                             "write_footer(NULL) should not crash", _test_info))
    { all_passed = false; }
    if (all_passed) { _test_info->tests_passed++; printf("  %s write_footer unit test passed\n", D_TEST_SYMBOL_PASS); }
    else { printf("  %s write_footer unit test failed\n", D_TEST_SYMBOL_FAIL); }
    _test_info->tests_total++;
    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_session_write_summary
  Tests write_summary does not crash.
*/
bool d_tests_sa_test_session_write_summary(struct d_test_counter* _test_info)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;
    printf("  --- Testing write_summary ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;
    session = d_test_session_new();
    if (session)
    {
        d_test_session_set_output_format(session, D_TEST_OUTPUT_SILENT);
        d_test_session_write_summary(session);
        if (!d_assert_standalone(true, "write_summary did not crash",
                                 "write_summary should not crash", _test_info))
        { all_passed = false; }
        d_test_session_free(session);
    }
    d_test_session_write_summary(NULL);
    if (!d_assert_standalone(true, "write_summary(NULL) did not crash",
                             "write_summary(NULL) should not crash", _test_info))
    { all_passed = false; }
    if (all_passed) { _test_info->tests_passed++; printf("  %s write_summary unit test passed\n", D_TEST_SYMBOL_PASS); }
    else { printf("  %s write_summary unit test failed\n", D_TEST_SYMBOL_FAIL); }
    _test_info->tests_total++;
    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_session_flush
  Tests flush does not crash on valid and NULL sessions.
*/
bool d_tests_sa_test_session_flush(struct d_test_counter* _test_info)
{
    size_t initial_tests_passed;
    bool   all_passed;
    struct d_test_session* session;
    printf("  --- Testing flush ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;
    session = d_test_session_new();
    if (session)
    {
        d_test_session_flush(session);
        if (!d_assert_standalone(true, "flush did not crash",
                                 "flush should not crash", _test_info))
        { all_passed = false; }
        d_test_session_free(session);
    }
    d_test_session_flush(NULL);
    if (!d_assert_standalone(true, "flush(NULL) did not crash",
                             "flush(NULL) should not crash", _test_info))
    { all_passed = false; }
    if (all_passed) { _test_info->tests_passed++; printf("  %s flush unit test passed\n", D_TEST_SYMBOL_PASS); }
    else { printf("  %s flush unit test failed\n", D_TEST_SYMBOL_FAIL); }
    _test_info->tests_total++;
    return (_test_info->tests_passed > initial_tests_passed);
}

/*
d_tests_sa_test_session_output_null
  Tests all output functions tolerate NULL session.
*/
bool d_tests_sa_test_session_output_null(struct d_test_counter* _test_info)
{
    size_t initial_tests_passed;
    bool   all_passed;
    printf("  --- Testing output functions (NULL) ---\n");
    initial_tests_passed = _test_info->tests_passed;
    all_passed           = true;
    d_test_session_write(NULL, "test %s", "msg");
    d_test_session_writeln(NULL, "test %s", "msg");
    d_test_session_write_module_start(NULL, NULL);
    d_test_session_write_module_end(NULL, NULL, true);
    d_test_session_write_test_result(NULL, "test", true, "msg", 0.0);
    if (!d_assert_standalone(true, "all output NULLs did not crash",
                             "all output NULLs should not crash", _test_info))
    { all_passed = false; }
    if (all_passed) { _test_info->tests_passed++; printf("  %s output (NULL) unit test passed\n", D_TEST_SYMBOL_PASS); }
    else { printf("  %s output (NULL) unit test failed\n", D_TEST_SYMBOL_FAIL); }
    _test_info->tests_total++;
    return (_test_info->tests_passed > initial_tests_passed);
}
