#include "../../../inc/c/test/test_handler.h"
#include "../../../inc/c/test/test_module.h"
#include "../../../inc/c/test/test_session.h"
#include <time.h>


//
// internal helpers
//

static double
d_test_handler_get_time_ms
(
    void
)
{
    return (double)clock() / CLOCKS_PER_SEC * 1000.0;
}


//
// creation and destruction
//

/*
d_test_handler_new
  Creates a handler with default flags and no event system or stacks.

Parameter(s):
  _default_config: optional default test options; may be NULL.
Return:
  A pointer to the new handler, or NULL on allocation failure.
*/
struct d_test_handler*
d_test_handler_new
(
    struct d_test_options* _default_config
)
{
    return d_test_handler_new_full(_default_config,
                                   0,
                                   0,
                                   D_TEST_HANDLER_FLAG_NONE);
}


/*
d_test_handler_new_with_events
  Creates a handler with an event system of the given capacity and the
EMIT_EVENTS flag set automatically.

Parameter(s):
  _default_config: optional default test options; may be NULL.
  _event_capacity: initial capacity for the event handler.
Return:
  A pointer to the new handler, or NULL on allocation failure.
*/
struct d_test_handler*
d_test_handler_new_with_events
(
    struct d_test_options* _default_config,
    size_t                _event_capacity
)
{
    return d_test_handler_new_full(_default_config,
                                   _event_capacity,
                                   0,
                                   D_TEST_HANDLER_FLAG_EMIT_EVENTS);
}


/*
d_test_handler_new_full
  Creates a handler with full control over event capacity, stack capacity,
and flags.

Parameter(s):
  _default_config: optional default test options; may be NULL.
  _event_capacity: initial capacity for the event handler; 0 skips
                   allocation.
  _stack_capacity: initial capacity for result and context stacks; 0 uses
                   default when TRACK_STACK is set.
  _flags:          bitwise combination of DTestHandlerFlag values.
Return:
  A pointer to the new handler, or NULL on allocation failure.
*/
struct d_test_handler*
d_test_handler_new_full
(
    struct d_test_options* _default_config,
    size_t                _event_capacity,
    size_t                _stack_capacity,
    uint32_t              _flags
)
{
    struct d_test_handler* handler;

    handler = malloc(sizeof(struct d_test_handler));

    // check if allocation succeeded
    if (!handler)
    {
        return NULL;
    }

    d_memset(handler, 0, sizeof(struct d_test_handler));

    // allocate event handler if capacity requested
    if (_event_capacity > 0)
    {
        handler->event_handler = d_event_handler_new(_event_capacity,
                                                      16);

        if (!handler->event_handler)
        {
            free(handler);

            return NULL;
        }
    }

    handler->default_config = _default_config;

    // allocate stacks if capacity or TRACK_STACK flag set
    if ( (_stack_capacity > 0) ||
         (_flags & D_TEST_HANDLER_FLAG_TRACK_STACK) )
    {
        size_t cap;

        cap = _stack_capacity > 0 ? _stack_capacity : 32;

        handler->result_stack  = d_min_stack_new(cap,
                                     sizeof(struct d_test_result));
        handler->context_stack = d_min_stack_new(cap,
                                     sizeof(struct d_test_context));

        // clean up on partial allocation failure
        if ( (!handler->result_stack) ||
             (!handler->context_stack) )
        {
            if (handler->result_stack)
            {
                d_min_stack_free(handler->result_stack);
            }

            if (handler->context_stack)
            {
                d_min_stack_free(handler->context_stack);
            }

            if (handler->event_handler)
            {
                d_event_handler_free(handler->event_handler);
            }

            free(handler);

            return NULL;
        }
    }

    handler->flags            = _flags;
    handler->abort_on_failure = (_flags & D_TEST_HANDLER_FLAG_ABORT_ON_FAIL) != 0;
    handler->output_stream    = stdout;

    return handler;
}


/*
d_test_handler_free
  Frees a test handler and all owned resources (event handler, stacks,
output buffer).

Parameter(s):
  _handler: the handler to free; may be NULL.
Return:
  none.
*/
void
d_test_handler_free
(
    struct d_test_handler* _handler
)
{
    if (!_handler)
    {
        return;
    }

    if (_handler->event_handler)
    {
        d_event_handler_free(_handler->event_handler);
    }

    if (_handler->result_stack)
    {
        d_min_stack_free(_handler->result_stack);
    }

    if (_handler->context_stack)
    {
        d_min_stack_free(_handler->context_stack);
    }

    if (_handler->output_buffer)
    {
        free(_handler->output_buffer);
    }

    free(_handler);

    return;
}


//
// flag management
//

/*
d_test_handler_set_flag
  Sets a flag on the handler. Setting ABORT_ON_FAIL also updates the
abort_on_failure field.

Parameter(s):
  _handler: the handler to modify; may be NULL.
  _flag:    the flag to set.
Return:
  none.
*/
void
d_test_handler_set_flag
(
    struct d_test_handler* _handler,
    enum DTestHandlerFlag  _flag
)
{
    if (_handler)
    {
        _handler->flags |= _flag;

        if (_flag == D_TEST_HANDLER_FLAG_ABORT_ON_FAIL)
        {
            _handler->abort_on_failure = true;
        }
    }

    return;
}


/*
d_test_handler_clear_flag
  Clears a flag on the handler. Clearing ABORT_ON_FAIL also updates the
abort_on_failure field.

Parameter(s):
  _handler: the handler to modify; may be NULL.
  _flag:    the flag to clear.
Return:
  none.
*/
void
d_test_handler_clear_flag
(
    struct d_test_handler* _handler,
    enum DTestHandlerFlag  _flag
)
{
    if (_handler)
    {
        _handler->flags &= ~_flag;

        if (_flag == D_TEST_HANDLER_FLAG_ABORT_ON_FAIL)
        {
            _handler->abort_on_failure = false;
        }
    }

    return;
}


/*
d_test_handler_has_flag
  Checks whether a flag (or combination of flags) is set on the handler.

Parameter(s):
  _handler: the handler to query; may be NULL.
  _flag:    the flag or flags to check.
Return:
  true if all bits in _flag are set, false otherwise or if _handler is
NULL.
*/
bool
d_test_handler_has_flag
(
    const struct d_test_handler* _handler,
    enum DTestHandlerFlag        _flag
)
{
    return _handler
               ? ((_handler->flags & _flag) == (uint32_t)_flag)
               : false;
}


//
// event listener registration
//

/*
d_test_handler_register_listener
  Registers an event listener on the handler's event system.

Parameter(s):
  _handler:  the handler; must have an event handler.
  _event_id: the event identifier to listen for.
  _callback: the callback function; must not be NULL.
  _enabled:  whether the listener starts enabled.
Return:
  true on success, false if any parameter is invalid or binding fails.
*/
bool
d_test_handler_register_listener
(
    struct d_test_handler* _handler,
    d_event_id             _event_id,
    fn_callback            _callback,
    bool                   _enabled
)
{
    struct d_event_listener* listener;

    if ( (!_handler)                ||
         (!_handler->event_handler) ||
         (!_callback) )
    {
        return false;
    }

    listener = d_event_listener_new(_event_id,
                                    _callback,
                                    _enabled);

    if (!listener)
    {
        return false;
    }

    // attempt to bind; free listener on failure
    if (!d_event_handler_bind(_handler->event_handler, listener))
    {
        d_event_listener_free(listener);

        return false;
    }

    return true;
}


/*
d_test_handler_unregister_listener
  Removes a listener by event identifier from the handler's event system.

Parameter(s):
  _handler:  the handler; must have an event handler.
  _event_id: the event identifier to unbind.
Return:
  true on success, false if handler or event handler is NULL.
*/
bool
d_test_handler_unregister_listener
(
    struct d_test_handler* _handler,
    d_event_id             _event_id
)
{
    if ( (!_handler) ||
         (!_handler->event_handler) )
    {
        return false;
    }

    return d_event_handler_unbind(_handler->event_handler,
                                  _event_id);
}


/*
d_test_handler_enable_listener
  Enables a previously registered listener by event identifier.

Parameter(s):
  _handler: the handler; must have an event handler.
  _id:      the event identifier of the listener to enable.
Return:
  true on success, false if handler or event handler is NULL.
*/
bool
d_test_handler_enable_listener
(
    struct d_test_handler* _handler,
    d_event_id             _id
)
{
    if ( (!_handler) ||
         (!_handler->event_handler) )
    {
        return false;
    }

    return d_event_handler_enable_listener(_handler->event_handler,
                                            _id);
}


/*
d_test_handler_disable_listener
  Disables a previously registered listener by event identifier.

Parameter(s):
  _handler: the handler; must have an event handler.
  _id:      the event identifier of the listener to disable.
Return:
  true on success, false if handler or event handler is NULL.
*/
bool
d_test_handler_disable_listener
(
    struct d_test_handler* _handler,
    d_event_id             _id
)
{
    if ( (!_handler) ||
         (!_handler->event_handler) )
    {
        return false;
    }

    return d_event_handler_disable_listener(_handler->event_handler,
                                             _id);
}


//
// event emission
//

/*
d_test_handler_emit_event
  Fires an event through the handler's event system if EMIT_EVENTS is set.

Parameter(s):
  _handler:    the handler; may be NULL.
  _event_type: the test lifecycle event type.
  _context:    the current execution context passed as event payload.
Return:
  none.
*/
void
d_test_handler_emit_event
(
    struct d_test_handler* _handler,
    enum DTestEvent        _event_type,
    struct d_test_context* _context
)
{
    struct d_event* event;

    if ( (!_handler)                ||
         (!_handler->event_handler) )
    {
        return;
    }

    if (!d_test_handler_has_flag(_handler,
             D_TEST_HANDLER_FLAG_EMIT_EVENTS))
    {
        return;
    }

    // create event with context as the opaque caller-owned pointer
    event = d_event_new_ctx((d_event_id)_event_type,
                            (void*)_context);
    if (event)
    {
        d_event_handler_fire_event(_handler->event_handler,
                                   event);
        d_event_free(event);
    }

    return;
}


//
// test execution
//

/*
d_test_handler_run_test
  Executes a single d_test through the handler, emitting lifecycle events
and tracking pass/fail statistics.
  The test's children (assertions and test functions) are executed by
delegating to d_test_run(), which iterates the test's child vector.

Parameter(s):
  _handler:    the handler coordinating execution.
  _test:       the test to run.
  _run_config: optional config override; may be NULL.
Return:
  true if the test passed, false otherwise or on invalid parameters.
*/
bool
d_test_handler_run_test
(
    struct d_test_handler* _handler,
    struct d_test*         _test,
    struct d_test_options* _run_config
)
{
    bool                  result;
    struct d_test_context context;

    if ( (!_handler) ||
         (!_test) )
    {
        return false;
    }

    d_test_context_init(&context, _handler);
    context.current_test  = _test;
    context.depth         = _handler->current_depth;
    context.start_time_ms = d_test_handler_get_time_ms();

    context.event_type = D_TEST_EVENT_SETUP;
    d_test_handler_emit_event(_handler,
                               D_TEST_EVENT_SETUP,
                               &context);

    context.event_type = D_TEST_EVENT_START;
    d_test_handler_emit_event(_handler,
                               D_TEST_EVENT_START,
                               &context);

    // delegate to the test's own run logic which iterates its
    // children (assertions and test functions)
    result         = d_test_run(_test, _run_config);
    context.result = result;

    _handler->results.tests_run++;

    if (result)
    {
        _handler->results.tests_passed++;
        context.event_type = D_TEST_EVENT_SUCCESS;
        d_test_handler_emit_event(_handler,
                                   D_TEST_EVENT_SUCCESS,
                                   &context);
    }
    else
    {
        _handler->results.tests_failed++;
        context.event_type = D_TEST_EVENT_FAILURE;
        d_test_handler_emit_event(_handler,
                                   D_TEST_EVENT_FAILURE,
                                   &context);
    }

    context.end_time_ms = d_test_handler_get_time_ms();
    context.event_type  = D_TEST_EVENT_END;
    d_test_handler_emit_event(_handler,
                               D_TEST_EVENT_END,
                               &context);

    context.event_type = D_TEST_EVENT_TEAR_DOWN;
    d_test_handler_emit_event(_handler,
                               D_TEST_EVENT_TEAR_DOWN,
                               &context);

    // push context to stack if tracking is enabled
    if (d_test_handler_has_flag(_handler,
             D_TEST_HANDLER_FLAG_TRACK_STACK))
    {
        d_test_handler_push_context(_handler, &context);
    }

    // abort if failure and abort_on_failure is set
    if ( (!result) &&
         (_handler->abort_on_failure) )
    {
        return false;
    }

    return result;
}


/*
d_test_handler_run_block
  Executes a test block through the handler. Iterates the block's child
vector via d_test_block_child_count / d_test_block_get_child_at,
dispatching each child through d_test_handler_run_test_type.
  Config resolution: _run_config if provided, otherwise the handler's
default_config.

Parameter(s):
  _handler:    the handler coordinating execution.
  _block:      the block to run.
  _run_config: optional config override; may be NULL.
Return:
  true if all children passed, false otherwise or on invalid parameters.
*/
bool
d_test_handler_run_block
(
    struct d_test_handler* _handler,
    struct d_test_block*   _block,
    struct d_test_options* _run_config
)
{
    size_t                 i;
    size_t                 child_count;
    bool                   all_passed;
    struct d_test_context  context;
    struct d_test_options* config;

    if ( (!_handler) ||
         (!_block) )
    {
        return false;
    }

    all_passed = true;

    d_test_context_init(&context, _handler);
    context.current_block = _block;
    context.depth         = _handler->current_depth;
    context.start_time_ms = d_test_handler_get_time_ms();

    _handler->current_depth++;

    // update max depth if this is a new high
    if (_handler->current_depth > _handler->results.max_depth)
    {
        _handler->results.max_depth = _handler->current_depth;
    }

    _handler->results.blocks_run++;

    context.event_type = D_TEST_EVENT_SETUP;
    d_test_handler_emit_event(_handler,
                               D_TEST_EVENT_SETUP,
                               &context);

    // resolve effective config: run_config > handler default
    config      = _run_config
                      ? _run_config
                      : _handler->default_config;
    child_count = d_test_block_child_count(_block);

    for (i = 0; i < child_count; i++)
    {
        struct d_test_type* child;

        child = d_test_block_get_child_at(_block, i);

        if ( (child) &&
             (!d_test_handler_run_test_type(_handler,
                                            child,
                                            config)) )
        {
            all_passed = false;

            if (_handler->abort_on_failure)
            {
                break;
            }
        }
    }

    if (all_passed)
    {
        _handler->results.blocks_passed++;
    }
    else
    {
        _handler->results.blocks_failed++;
    }

    context.end_time_ms = d_test_handler_get_time_ms();
    context.result      = all_passed;
    context.event_type  = D_TEST_EVENT_TEAR_DOWN;
    d_test_handler_emit_event(_handler,
                               D_TEST_EVENT_TEAR_DOWN,
                               &context);

    _handler->current_depth--;

    return all_passed;
}


/*
d_test_handler_run_test_type
  Dispatches a d_test_type to the appropriate run function based on its
type discriminator.

Parameter(s):
  _handler:    the handler coordinating execution.
  _test_type:  the tagged union to dispatch.
  _run_config: optional config override; may be NULL.
Return:
  true if the dispatched child passed, false otherwise or on invalid
parameters.
*/
bool
d_test_handler_run_test_type
(
    struct d_test_handler* _handler,
    struct d_test_type*    _test_type,
    struct d_test_options* _run_config
)
{
    if ( (!_handler) ||
         (!_test_type) )
    {
        return false;
    }

    switch (_test_type->type)
    {
        case D_TEST_TYPE_TEST:
            return d_test_handler_run_test(_handler,
                                           _test_type->test,
                                           _run_config);

        case D_TEST_TYPE_TEST_BLOCK:
            return d_test_handler_run_block(_handler,
                                            _test_type->block,
                                            _run_config);

        case D_TEST_TYPE_MODULE:
            return d_test_handler_run_module(_handler,
                                             _test_type->module,
                                             _run_config);

        default:
            return false;
    }
}


/*
d_test_handler_run_module
  Executes a test module through the handler. Iterates the module's child
vector via d_test_module_child_count / d_test_module_get_child_at,
dispatching each child through d_test_handler_run_test_type.
  Skipped modules (status == D_TEST_MODULE_STATUS_SKIPPED) are counted
but not executed.

Parameter(s):
  _handler:    the handler coordinating execution.
  _module:     the module to run.
  _run_config: optional config override; may be NULL.
Return:
  true if all children passed (or module was skipped), false otherwise or
on invalid parameters.
*/
bool
d_test_handler_run_module
(
    struct d_test_handler* _handler,
    struct d_test_module*  _module,
    struct d_test_options* _run_config
)
{
    size_t                 i;
    size_t                 child_count;
    bool                   all_passed;
    struct d_test_context  context;
    struct d_test_options* config;

    if ( (!_handler) ||
         (!_module) )
    {
        return false;
    }

    // check if the module has been marked as skipped
    if (_module->status == D_TEST_MODULE_STATUS_SKIPPED)
    {
        _handler->results.modules_skipped++;

        return true;
    }

    all_passed = true;

    d_test_context_init(&context, _handler);
    context.current_module = _module;
    context.depth          = _handler->current_depth;
    context.start_time_ms  = d_test_handler_get_time_ms();

    _handler->current_depth++;
    _handler->results.modules_run++;

    context.event_type = D_TEST_EVENT_SETUP;
    d_test_handler_emit_event(_handler,
                               D_TEST_EVENT_SETUP,
                               &context);

    // resolve config: run_config > module's own config
    config      = _run_config
                      ? _run_config
                      : _module->config;
    child_count = d_test_module_child_count(_module);

    for (i = 0; i < child_count; i++)
    {
        struct d_test_type* child;

        child = d_test_module_get_child_at(_module, i);

        if ( (child) &&
             (!d_test_handler_run_test_type(_handler,
                                            child,
                                            config)) )
        {
            all_passed = false;

            if (_handler->abort_on_failure)
            {
                break;
            }
        }
    }

    if (all_passed)
    {
        _handler->results.modules_passed++;
    }
    else
    {
        _handler->results.modules_failed++;
    }

    context.end_time_ms = d_test_handler_get_time_ms();
    context.result      = all_passed;
    context.event_type  = D_TEST_EVENT_TEAR_DOWN;
    d_test_handler_emit_event(_handler,
                               D_TEST_EVENT_TEAR_DOWN,
                               &context);

    _handler->current_depth--;

    return all_passed;
}


/*
d_test_handler_run_tree
  Executes a test tree through the handler, emitting setup and teardown
events.

Parameter(s):
  _handler:    the handler coordinating execution.
  _tree:       the tree to run.
  _run_config: optional config override; may be NULL.
Return:
  true if the tree passed, false otherwise or on invalid parameters.
*/
bool
d_test_handler_run_tree
(
    struct d_test_handler* _handler,
    struct d_test_tree*    _tree,
    struct d_test_options* _run_config
)
{
    struct d_test_context context;
    bool                  result;

    if ( (!_handler) ||
         (!_tree) )
    {
        return false;
    }

    d_test_context_init(&context, _handler);
    context.current_tree  = _tree;
    context.depth         = _handler->current_depth;
    context.start_time_ms = d_test_handler_get_time_ms();

    context.event_type = D_TEST_EVENT_SETUP;
    d_test_handler_emit_event(_handler,
                               D_TEST_EVENT_SETUP,
                               &context);

    result         = d_test_tree_run(_tree, _run_config);
    context.result = result;

    context.end_time_ms = d_test_handler_get_time_ms();
    context.event_type  = D_TEST_EVENT_TEAR_DOWN;
    d_test_handler_emit_event(_handler,
                               D_TEST_EVENT_TEAR_DOWN,
                               &context);

    return result;
}


/*
d_test_handler_run_session
  Executes a test session through the handler, emitting setup and teardown
events.

Parameter(s):
  _handler: the handler coordinating execution.
  _session: the session to run.
Return:
  true if the session passed, false otherwise or on invalid parameters.
*/
bool
d_test_handler_run_session
(
    struct d_test_handler*  _handler,
    struct d_test_session*  _session
)
{
    struct d_test_context context;
    bool                  result;

    if ( (!_handler) ||
         (!_session) )
    {
        return false;
    }

    d_test_context_init(&context, _handler);
    context.current_session = _session;
    context.depth           = _handler->current_depth;
    context.start_time_ms   = d_test_handler_get_time_ms();

    context.event_type = D_TEST_EVENT_SETUP;
    d_test_handler_emit_event(_handler,
                               D_TEST_EVENT_SETUP,
                               &context);

    result         = d_test_session_run(_session);
    context.result = result;

    context.end_time_ms = d_test_handler_get_time_ms();
    context.event_type  = D_TEST_EVENT_TEAR_DOWN;
    d_test_handler_emit_event(_handler,
                               D_TEST_EVENT_TEAR_DOWN,
                               &context);

    return result;
}


//
// assertion tracking
//

/*
d_test_handler_record_assertion
  Records an assertion result in the handler's statistics.

Parameter(s):
  _handler:   the handler tracking assertions; may be NULL.
  _assertion: the assertion to record; may be NULL.
Return:
  none.
*/
void
d_test_handler_record_assertion
(
    struct d_test_handler* _handler,
    struct d_assert*       _assertion
)
{
    if ( (!_handler) ||
         (!_assertion) )
    {
        return;
    }

    _handler->results.assertions_run++;

    if (_assertion->result)
    {
        _handler->results.assertions_passed++;
    }
    else
    {
        _handler->results.assertions_failed++;
    }

    return;
}


//
// stack operations
//

/*
d_test_handler_push_result
  Pushes a test result snapshot onto the handler's result stack.

Parameter(s):
  _handler: the handler with a result stack.
  _result:  the result to push.
Return:
  true on success, false if any parameter is invalid or stack is missing.
*/
bool
d_test_handler_push_result
(
    struct d_test_handler*      _handler,
    const struct d_test_result* _result
)
{
    if ( (!_handler)               ||
         (!_handler->result_stack) ||
         (!_result) )
    {
        return false;
    }

    return d_min_stack_push(_handler->result_stack, _result);
}


/*
d_test_handler_pop_result
  Pops the most recent test result from the handler's result stack.

Parameter(s):
  _handler: the handler with a result stack.
  _out:     output parameter receiving the popped result.
Return:
  true on success, false if handler or stack is NULL.
*/
bool
d_test_handler_pop_result
(
    struct d_test_handler* _handler,
    struct d_test_result*  _out
)
{
    if ( (!_handler) ||
         (!_handler->result_stack) )
    {
        return false;
    }

    return d_min_stack_pop(_handler->result_stack, _out);
}


/*
d_test_handler_push_context
  Pushes a test context snapshot onto the handler's context stack.

Parameter(s):
  _handler: the handler with a context stack.
  _context: the context to push.
Return:
  true on success, false if any parameter is invalid or stack is missing.
*/
bool
d_test_handler_push_context
(
    struct d_test_handler*       _handler,
    const struct d_test_context* _context
)
{
    if ( (!_handler)                ||
         (!_handler->context_stack) ||
         (!_context) )
    {
        return false;
    }

    return d_min_stack_push(_handler->context_stack, _context);
}


/*
d_test_handler_pop_context
  Pops the most recent test context from the handler's context stack.

Parameter(s):
  _handler: the handler with a context stack.
  _out:     output parameter receiving the popped context.
Return:
  true on success, false if handler or stack is NULL.
*/
bool
d_test_handler_pop_context
(
    struct d_test_handler* _handler,
    struct d_test_context* _out
)
{
    if ( (!_handler) ||
         (!_handler->context_stack) )
    {
        return false;
    }

    return d_min_stack_pop(_handler->context_stack, _out);
}


//
// result queries
//

/*
d_test_handler_get_results
  Returns a copy of the handler's current result statistics.

Parameter(s):
  _handler: the handler to query; may be NULL.
Return:
  A copy of the results, or a zeroed struct if _handler is NULL.
*/
struct d_test_result
d_test_handler_get_results
(
    const struct d_test_handler* _handler
)
{
    struct d_test_result empty = {0};

    return _handler ? _handler->results : empty;
}


/*
d_test_handler_reset_results
  Resets all result counters and depth tracking to zero.

Parameter(s):
  _handler: the handler to reset; may be NULL.
Return:
  none.
*/
void
d_test_handler_reset_results
(
    struct d_test_handler* _handler
)
{
    if (!_handler)
    {
        return;
    }

    d_memset(&_handler->results, 0, sizeof(struct d_test_result));
    _handler->current_depth = 0;

    return;
}


/*
d_test_handler_print_results
  Prints a formatted summary of the handler's test results.

Parameter(s):
  _handler: the handler to print; may be NULL.
  _label:   optional label for the summary; defaults to "Unnamed".
Return:
  none.
*/
void
d_test_handler_print_results
(
    const struct d_test_handler* _handler,
    const char*                  _label
)
{
    const struct d_test_result* r;

    if (!_handler)
    {
        return;
    }

    r = &_handler->results;

    printf("\n========================================\n");
    printf("Test Results: %s\n",
           _label ? _label : "Unnamed");
    printf("========================================\n");
    printf("Tests:      %zu run, %zu passed, "
           "%zu failed, %zu skipped\n",
           r->tests_run,
           r->tests_passed,
           r->tests_failed,
           r->tests_skipped);
    printf("Assertions: %zu run, %zu passed, "
           "%zu failed\n",
           r->assertions_run,
           r->assertions_passed,
           r->assertions_failed);
    printf("Blocks:     %zu run, %zu passed, "
           "%zu failed\n",
           r->blocks_run,
           r->blocks_passed,
           r->blocks_failed);
    printf("Modules:    %zu run, %zu passed, "
           "%zu failed, %zu skipped\n",
           r->modules_run,
           r->modules_passed,
           r->modules_failed,
           r->modules_skipped);
    printf("Max depth:  %zu\n", r->max_depth);
    printf("Pass Rate:  %.2f%% (tests), "
           "%.2f%% (assertions)\n",
           d_test_handler_get_pass_rate(_handler),
           d_test_handler_get_assertion_rate(_handler));
    printf("========================================\n");

    return;
}


/*
d_test_handler_get_pass_rate
  Computes the percentage of tests that passed.

Parameter(s):
  _handler: the handler to query; may be NULL.
Return:
  The pass rate as a percentage (0.0 - 100.0), or 0.0 if no tests have
been run.
*/
double
d_test_handler_get_pass_rate
(
    const struct d_test_handler* _handler
)
{
    if ( (!_handler) ||
         (_handler->results.tests_run == 0) )
    {
        return 0.0;
    }

    return (double)_handler->results.tests_passed
         / (double)_handler->results.tests_run
         * 100.0;
}


/*
d_test_handler_get_assertion_rate
  Computes the percentage of assertions that passed.

Parameter(s):
  _handler: the handler to query; may be NULL.
Return:
  The assertion pass rate as a percentage (0.0 - 100.0), or 0.0 if no
assertions have been run.
*/
double
d_test_handler_get_assertion_rate
(
    const struct d_test_handler* _handler
)
{
    if ( (!_handler) ||
         (_handler->results.assertions_run == 0) )
    {
        return 0.0;
    }

    return (double)_handler->results.assertions_passed
         / (double)_handler->results.assertions_run
         * 100.0;
}


//
// context helpers
//

/*
d_test_context_new
  Allocates and initializes a new test context.

Parameter(s):
  _handler: the handler to associate; may be NULL.
Return:
  A pointer to the new context, or NULL on allocation failure.
*/
struct d_test_context*
d_test_context_new
(
    struct d_test_handler* _handler
)
{
    struct d_test_context* context;

    context = malloc(sizeof(struct d_test_context));

    if (context)
    {
        d_test_context_init(context, _handler);
    }

    return context;
}


/*
d_test_context_init
  Initializes a test context, zeroing all fields and setting handler and
result.

Parameter(s):
  _context: the context to initialize; may be NULL.
  _handler: the handler to associate; may be NULL.
Return:
  none.
*/
void
d_test_context_init
(
    struct d_test_context* _context,
    struct d_test_handler* _handler
)
{
    if (!_context)
    {
        return;
    }

    d_memset(_context, 0, sizeof(struct d_test_context));
    _context->handler = _handler;
    _context->result  = true;

    return;
}


/*
d_test_context_free
  Frees a heap-allocated test context.

Parameter(s):
  _context: the context to free; may be NULL.
Return:
  none.
*/
void
d_test_context_free
(
    struct d_test_context* _context
)
{
    if (_context)
    {
        free(_context);
    }

    return;
}


//
// DSL helper functions
//

/*
d_test_handler_create_module_with_metadata
  Creates a test module and populates it with key-value metadata.

Parameter(s):
  _name:     the module name (currently unused; metadata sets name).
  _metadata: array of key-value pairs.
  _count:    number of pairs in the array.
Return:
  A pointer to the new module, or NULL on allocation failure.
*/
struct d_test_module*
d_test_handler_create_module_with_metadata
(
    const char*             _name,
    const struct d_test_kv* _metadata,
    size_t                  _count
)
{
    struct d_test_module* module;
    size_t               i;

    (void)_name;

    module = d_test_module_new(NULL, 0);

    if (!module)
    {
        return NULL;
    }

    // apply each metadata pair to the module
    for (i = 0; i < _count; i++)
    {
        d_test_module_set_metadata_str(module,
                                       _metadata[i].key,
                                       _metadata[i].value);
    }

    return module;
}


/*
d_test_module_set_metadata_str
  Sets a single metadata field on a module by matching the key string to
a known flag.

Parameter(s):
  _module: the module to update; may be NULL.
  _key:    the metadata key string (e.g. "name", "author").
  _value:  the value to set.
Return:
  true if the key was recognized and set, false otherwise.
*/
bool
d_test_module_set_metadata_str
(
    struct d_test_module* _module,
    const char*           _key,
    const char*           _value
)
{
    enum DTestMetadataFlag flag;
    size_t                 key_len;

    if ( (!_module) ||
         (!_key) )
    {
        return false;
    }

    key_len = d_strnlen(_key, 256);

    if (d_strequals(_key, key_len, "name", 4))
    {
        flag = D_TEST_METADATA_NAME;
    }
    else if ( (d_strequals(_key, key_len, "author",  6)) ||
              (d_strequals(_key, key_len, "authors", 7)) )
    {
        flag = D_TEST_METADATA_AUTHORS;
    }
    else if (d_strequals(_key, key_len, "version", 7))
    {
        flag = D_TEST_METADATA_VERSION_STRING;
    }
    else if (d_strequals(_key, key_len, "description", 11))
    {
        flag = D_TEST_METADATA_DESCRIPTION;
    }
    else if (d_strequals(_key, key_len, "category", 8))
    {
        flag = D_TEST_METADATA_CATEGORY;
    }
    else if (d_strequals(_key, key_len, "module", 6))
    {
        flag = D_TEST_METADATA_MODULE_NAME;
    }
    else if (d_strequals(_key, key_len, "tags", 4))
    {
        flag = D_TEST_METADATA_TAGS;
    }
    else
    {
        return false;
    }

    return d_test_module_set_metadata(_module,
                                      flag,
                                      (void*)_value);
}


/*
d_test_handler_create_test_from_nodes
  Creates a test from an array of DSL nodes.

Parameter(s):
  _nodes: array of DSL nodes.
  _count: number of nodes.
Return:
  A pointer to the new test, or NULL if parameters are invalid.
*/
struct d_test*
d_test_handler_create_test_from_nodes
(
    const struct d_test_dsl_node* _nodes,
    size_t                        _count
)
{
    if ( (!_nodes) ||
         (_count == 0) )
    {
        return NULL;
    }

    return d_test_new(NULL, 0);
}


/*
d_test_handler_create_block_from_nodes
  Creates a test block from an array of DSL nodes, adding each test or
nested block as a child.

Parameter(s):
  _name:  the block name (currently unused).
  _nodes: array of DSL nodes.
  _count: number of nodes.
Return:
  A pointer to the new block, or NULL on failure or invalid parameters.
*/
struct d_test_block*
d_test_handler_create_block_from_nodes
(
    const char*                   _name,
    const struct d_test_dsl_node* _nodes,
    size_t                        _count
)
{
    struct d_test_block* block;
    size_t               i;

    (void)_name;

    if ( (!_nodes) ||
         (_count == 0) )
    {
        return NULL;
    }

    block = d_test_block_new(NULL, 0);

    if (!block)
    {
        return NULL;
    }

    // add each node as a child of the block
    for (i = 0; i < _count; i++)
    {
        if ( (_nodes[i].type == D_TEST_DSL_NODE_TEST) &&
             (_nodes[i].test) )
        {
            d_test_block_add_test(block, _nodes[i].test);
        }
        else if ( (_nodes[i].type == D_TEST_DSL_NODE_BLOCK) &&
                  (_nodes[i].block) )
        {
            d_test_block_add_block(block, _nodes[i].block);
        }
    }

    return block;
}


/*
d_test_handler_create_module_from_decl
  Creates a module from metadata key-value pairs and an array of child
DSL nodes.

Parameter(s):
  _metadata:       array of key-value metadata pairs.
  _metadata_count: number of metadata pairs.
  _children:       array of child DSL nodes.
  _children_count: number of child nodes.
Return:
  A pointer to the new module, or NULL on failure.
*/
struct d_test_module*
d_test_handler_create_module_from_decl
(
    const struct d_test_kv*       _metadata,
    size_t                        _metadata_count,
    const struct d_test_dsl_node* _children,
    size_t                        _children_count
)
{
    const char*           name;
    struct d_test_module* module;
    size_t                i;
    size_t                key_len;

    name = "Unnamed Module";

    // search metadata for a "name" key
    for (i = 0; i < _metadata_count; i++)
    {
        if (_metadata[i].key)
        {
            key_len = d_strnlen(_metadata[i].key, 256);

            if (d_strequals(_metadata[i].key,
                            key_len,
                            "name",
                            4))
            {
                name = _metadata[i].value;
                break;
            }
        }
    }

    module = d_test_handler_create_module_with_metadata(
                 name,
                 _metadata,
                 _metadata_count);

    if (!module)
    {
        return NULL;
    }

    // add each child node to the module
    for (i = 0; i < _children_count; i++)
    {
        struct d_test_type* child_type;

        child_type = NULL;

        if ( (_children[i].type == D_TEST_DSL_NODE_TEST) &&
             (_children[i].test) )
        {
            child_type = d_test_type_new(D_TEST_TYPE_TEST,
                                         _children[i].test);
        }
        else if ( (_children[i].type == D_TEST_DSL_NODE_BLOCK) &&
                  (_children[i].block) )
        {
            child_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK,
                                         _children[i].block);
        }

        if (child_type)
        {
            d_test_module_add_child(
                module,
                (const struct d_test_tree_node*)child_type);
        }
    }

    return module;
}
