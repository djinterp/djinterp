/*******************************************************************************
* djinterp [test]                                                 test_counter.c
*
* The two operations on the field-form tally, test_counter.h: reset and sum.
*   Moved here from test_standalone.c, which defined them while its header
* declared them, so a suite that uses the tally alone no longer links the
* standalone framework to get them.
*
*
* path:      /src/djinterp/c/modules/test/test_counter.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.05
*******************************************************************************/
#include "../../../../../inc/djinterp/c/modules/test/test_counter.h"


/*
d_test_counter_reset
  Field by field rather than memset, so a field added to the struct is a
compile-time question here rather than a silent zero.
*/
void
d_test_counter_reset(
    struct dj_test_counter* _counter
)
{
    if (!_counter)
    {
        return;
    }

    _counter->assertions_total  = 0;
    _counter->assertions_passed = 0;
    _counter->tests_total       = 0;
    _counter->tests_passed      = 0;

    return;
}

/*
d_test_counter_add
  A plain sum: the tallies are counts of things that happened, so a total
past SIZE_MAX would need more checks than a process can make.
*/
void
d_test_counter_add(
    struct dj_test_counter*       _dest,
    const struct dj_test_counter* _source
)
{
    if ( (!_dest) ||
         (!_source) )
    {
        return;
    }

    _dest->assertions_total  += _source->assertions_total;
    _dest->assertions_passed += _source->assertions_passed;
    _dest->tests_total       += _source->tests_total;
    _dest->tests_passed      += _source->tests_passed;

    return;
}
