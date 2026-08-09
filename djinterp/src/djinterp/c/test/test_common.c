/******************************************************************************
* djinterp [test]                                         test_common_common.c
*
*   The DTest vocabulary's operations.  Pure functions over a five-value set.
*
* path:      /src/djinterp/test/test_common_common.c
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.30
******************************************************************************/

#include "./test_common_common.h"

static const char* const k_status[D_TEST_STATUS_COUNT] =
{ "passed", "failed", "skipped", "pending", "error" };

/*
d_test_status_name
  The status's own spelling.  A parity record carries the NAME, never the
integer, so a renumbering cannot pass unnoticed on one side.

Parameter(s):
  _status: the status to name.
Return:
  A static, NUL-terminated name, or "unknown".
*/
const char*
d_test_status_name(int32_t _status)
{
    return ( (_status < 0) ||                                                 \
	         (_status >= D_TEST_STATUS_COUNT) )
         ? "unknown" 
		 : k_status[_status];
}

int
d_test_status_is_valid(int32_t _status)
{
    return ( (_status >= 0) && (_status < D_TEST_STATUS_COUNT) ) ? 1 : 0;
}

/*
d_test_status_is_terminal
  Whether a status is an outcome rather than a state on the way to one.

  `pending` is the only non-terminal value: a node in it has not been
evaluated. `skipped` IS terminal -- the decision not to run is itself an
outcome, and treating it as pending would make a skipped suite look unfinished.

Parameter(s):
  _status: the status to classify.
Return:
  1 when the status is an outcome, 0 for pending or an invalid value.
*/
int
d_test_status_is_terminal(int32_t _status)
{
    if (!d_test_status_is_valid(_status))
    {
        return 0;
    }

    return (_status != D_TEST_STATUS_PENDING) ? 1 : 0;
}

/*
d_test_status_worse_of
  Combine two statuses into the one a parent should carry.

  THIS IS WHERE A TREE'S RESULT COMES FROM, so the ordering is a semantic
decision rather than a convenience. Severity runs:

      error > failed > [skipped] > pending > passed

  `error` outranks `failed` because the two mean different things: a failure is
a test that ran and disagreed, an error is a test that could not run. A suite
reporting "3 failed" when one of them never executed is telling the reader
something untrue.

  `pending` outranks `passed` so that a partly-evaluated tree never rolls up as
passed. A parent whose children are half unevaluated is pending, not green --
which is the whole reason this function is not just a max over the enum values.

  Where `skipped` sits is the one configurable part; see
D_CFG_TEST_COMMON_SKIP_IS_FAILURE.

Parameter(s):
  _a: the first status.
  _b: the second.
Return:
  The more severe of the two.  An invalid input yields `error`, because a
status nobody can classify is exactly the case error exists for.
*/
int32_t
d_test_status_worse_of(int32_t _a, int32_t _b)
{
    /* severity rank, indexed by status value. higher is worse. */
    static const int k_rank[D_TEST_STATUS_COUNT] =
    {
        0,      /* passed  */
        3,      /* failed  */
#if D_INTERNAL_TEST_COMMON_SKIP_IS_FAILURE
        3,      /* skipped -- ranks with failed under this policy */
#else
        2,      /* skipped */
#endif
        1,      /* pending */
        4       /* error   */
    };

    if ( (!d_test_status_is_valid(_a)) ||
         (!d_test_status_is_valid(_b)) )
    {
        return D_TEST_STATUS_ERROR;
    }

    return (k_rank[_a] >= k_rank[_b]) ? _a : _b;
}

/*
d_test_event_make
  Build an event record.

Parameter(s):
  _event:   the lifecycle event id.
  _status:  the status at the time of the event.
  _message: a borrowed, optional message; may be null.
Return:
  The record.  An invalid status is stored as `error` rather than passed
through, so a malformed event cannot later be mistaken for a passing one.
*/
struct d_test_event
d_test_event_make(
    d_test_event_id _event,
    int32_t         _status,
    const char*     _message
)
{
    struct d_test_event e;

    e.event   = _event;
    e.status  = d_test_status_is_valid(_status) ? _status
                                                : D_TEST_STATUS_ERROR;
    e.message = _message;

    return e;
}
