/*******************************************************************************
* djinterp [c][net]                                           net_tests_assert.c
*
* The check net's suites make, in DTest's C face: definitions for
* net_tests_assert.h.
*
*
* path:      /tests/djinterp/c/modules/net/net_tests_assert.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.11.19
*******************************************************************************/
#include "./net_tests_assert.h"  // corresponding header


/*
d_tests_net_assert
  A check with a name and a message is what d_test_suite_check_msg records;
without a suite there is nothing to record into, and the condition is the
answer.
*/
bool
d_tests_net_assert(
    bool                  _condition,
    const char*           _test_name,
    const char*           _message,
    struct dj_test_suite* _suite,
    const char*           _file,
    int                   _line
)
{
    // no suite: the condition alone
    if (!_suite)
    {
        return _condition;
    }

    return d_test_suite_check_msg(_suite,
                                  _condition,
                                  _test_name,
                                  _message,
                                  _file,
                                  _line);
}
