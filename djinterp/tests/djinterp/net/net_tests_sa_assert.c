/*******************************************************************************
* djinterp [net]                                           net_tests_sa_assert.c
*
* The assertion the net standalone suites count with.
*   Definitions for net_tests_sa_assert.h. They carry on the stand-in the net
* lanes linked while the standalone runner did not compile
* (`_handoffs/net_foundation/test_standalone_standin.c`, retired with the
* runner on 2026.09.30), under the suites' own names.
*
*
* path:      /tests/djinterp/net/net_tests_sa_assert.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.09.30
*******************************************************************************/
#include "./net_tests_sa_assert.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stdio.h>    // printf


/*
d_tests_net_assert
  Without a counter the call is the condition alone, silent even on failure:
the retired runner behaved so, and a test that passes NULL is asking for a
check it reports itself. Only failures print, because each suite's runner
prints a line per test and a total, which a line per passing assertion would
bury.
*/
bool
d_tests_net_assert(
    bool                   _condition,
    const char*            _test_name,
    const char*            _message,
    struct d_test_counter* _counter
)
{
    // no counter: the condition alone
    if (!_counter)
    {
        return _condition;
    }

    _counter->assertions_total++;

    // a pass is counted, a failure reported
    if (_condition)
    {
        _counter->assertions_passed++;
    }
    else
    {
        printf("    FAIL %s: %s\n",
               _test_name,
               _message);
    }

    return _condition;
}

/*
d_tests_net_counter_reset
  Assigns a zero-initialized counter instead of naming the fields, so a field
added to `struct d_test_counter` later is reset too rather than silently kept.
*/
void
d_tests_net_counter_reset(
    struct d_test_counter* _counter
)
{
    // nothing to reset
    if (!_counter)
    {
        return;
    }

    *_counter = (struct d_test_counter){ 0 };

    return;
}
