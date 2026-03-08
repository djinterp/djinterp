/******************************************************************************
* djinterp [test]                                   test_handler_link_stubs.c
*
*   TEMPORARY stub implementations for framework functions whose source files
* are not yet linked into the test_handler standalone test project.
*
*   Each stub below should be REMOVED once its real source file is added
* to the MSVS project. The real implementations live in:
*
*     d_assert         ->  \src\test\assert.c
*     d_event_new_args ->  \src\event\event.c
*     d_test_tree_run  ->  \src\test\test_tree.c
*
*   These stubs provide minimal behavior so that the project links and
* tests can execute. Replace with the real source files as soon as they
* are available.
*
*
* path:      \test\c\test\test_handler_link_stubs.c
* link:      TBA (temporary)
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.28
******************************************************************************/

#include "../../../inc/c/test/test_handler.h"
#include "../../../inc/c/test/test_tree.h"
#include <stdlib.h>
#include <string.h>


/******************************************************************************
 * STUB: d_assert  (real impl: \src\test\assert.c)
 *   Remove this section once assert.c is added to the project.
 *****************************************************************************/

/*
d_assert
  STUB: creates a d_assert from a boolean condition and pass/fail messages.
Replace with the real implementation from assert.c.

Parameter(s):
  _condition: the boolean condition to evaluate.
  _msg_pass:  message displayed on pass.
  _msg_fail:  message displayed on failure.
Return:
  A heap-allocated d_assert with the result set, or NULL on failure.
*/
struct d_assert*
d_assert
(
    bool        _condition,
    const char* _msg_pass,
    const char* _msg_fail
)
{
    struct d_assert* assertion;

    assertion = malloc(sizeof(struct d_assert));
    if (!assertion)
    {
        return NULL;
    }

    memset(assertion, 0, sizeof(struct d_assert));
    assertion->result = _condition;

    (void)_msg_pass;
    (void)_msg_fail;

    return assertion;
}

/******************************************************************************
 * STUB: d_event_new_args  (real impl: \src\event\event.c)
 *   Remove this section once event.c is added to the project.
 *****************************************************************************/

/*
d_event_new_args
  STUB: creates a d_event with the given id and argument array.
Replace with the real implementation from event.c.

Parameter(s):
  _id:        the event identifier.
  _args:      array of void* arguments.
  _count:     number of arguments.
Return:
  A heap-allocated d_event, or NULL on failure.
*/
struct d_event*
d_event_new_args
(
    d_event_id _id,
    void**     _args,
    size_t     _count
)
{
    struct d_event* event;

    event = malloc(sizeof(struct d_event));
    if (!event)
    {
        return NULL;
    }

    memset(event, 0, sizeof(struct d_event));

    (void)_id;
    (void)_args;
    (void)_count;

    return event;
}

/******************************************************************************
 * STUB: d_test_tree_run  (real impl: \src\test\test_tree.c)
 *   Remove this section once test_tree.c is added to the project.
 *****************************************************************************/

/*
d_test_tree_run
  STUB: runs a test tree. Currently returns true (pass) unconditionally.
Replace with the real implementation from test_tree.c.

Parameter(s):
  _tree:       the test tree to execute.
  _run_config: optional run-time configuration.
Return:
  true (stub always passes).
*/
bool
d_test_tree_run
(
    struct d_test_tree*          _tree,
    const struct d_test_options* _run_config
)
{
    (void)_tree;
    (void)_run_config;

    return true;
}
