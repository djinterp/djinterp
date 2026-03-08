#include "../../../inc/c/test/test_handler.h"
#include "../../../inc/c/test/test_session.h"


/******************************************************************************
 * STATIC HELPERS (MODULE)
 *****************************************************************************/

/*
d_test_handler_module_is_enabled
  Checks whether a test module is eligible for execution. A module is
considered enabled unless its status is explicitly set to SKIPPED.

Parameter(s):
  _module: the module to check.
Return:
  A boolean value corresponding to either:
  - true, if the module is non-NULL and not SKIPPED, or
  - false, if the module is NULL or has status SKIPPED.
*/
static bool
d_test_handler_module_is_enabled
(
    const struct d_test_module* _module
)
{
    // require non-NULL module
    if (!_module)
    {
        return false;
    }

    return _module->status != D_TEST_MODULE_STATUS_SKIPPED;
}

/******************************************************************************
 * MODULE CONVENIENCE FUNCTIONS
 *****************************************************************************/

/*
d_test_module_add_test
  Wraps a d_test in a d_test_type discriminated union node and adds it
as a child of the given module.

Parameter(s):
  _module: the module to which the test will be added.
  _test:   the test to add.
Return:
  A boolean value corresponding to either:
  - true, if the test was successfully wrapped and added, or
  - false, if either parameter is NULL or allocation failed.
*/
bool
d_test_module_add_test
(
    struct d_test_module* _module,
    struct d_test*        _test
)
{
    struct d_test_type* wrapped;

    // validate parameters
    if ( (!_module) ||
         (!_test) )
    {
        return false;
    }

    // wrap the test in a discriminated union node
    wrapped = d_test_type_new(D_TEST_TYPE_TEST, _test);

    // ensure allocation succeeded
    if (!wrapped)
    {
        return false;
    }

    return d_test_module_add_child(
               _module,
               (const struct d_test_tree_node*)wrapped);
}

/*
d_test_module_add_block
  Wraps a d_test_block in a d_test_type discriminated union node and
adds it as a child of the given module.

Parameter(s):
  _module: the module to which the block will be added.
  _block:  the test block to add.
Return:
  A boolean value corresponding to either:
  - true, if the block was successfully wrapped and added, or
  - false, if either parameter is NULL or allocation failed.
*/
bool
d_test_module_add_block
(
    struct d_test_module* _module,
    struct d_test_block*  _block
)
{
    struct d_test_type* wrapped;

    // validate parameters
    if ( (!_module) ||
         (!_block) )
    {
        return false;
    }

    // wrap the block in a discriminated union node
    wrapped = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, _block);

    // ensure allocation succeeded
    if (!wrapped)
    {
        return false;
    }

    return d_test_module_add_child(
               _module,
               (const struct d_test_tree_node*)wrapped);
}

/*
d_test_module_set_metadata
  Sets a metadata value on the module's configuration by enum key.
Provisional implementation: accepts metadata but does not yet store it
pending finalization of the d_test_options enum-keyed map API.

Parameter(s):
  _module: the module whose metadata is being set.
  _flag:   the metadata key (DTestMetadataFlag enum value).
  _value:  the metadata value (typically const char* cast to void*).
Return:
  A boolean value corresponding to either:
  - true, if the module and its config are non-NULL, or
  - false, if either the module or its config is NULL.
*/
bool
d_test_module_set_metadata
(
    struct d_test_module*  _module,
    enum DTestMetadataFlag _flag,
    void*                  _value
)
{
    // validate parameters
    if ( (!_module)         ||
         (!_module->config) )
    {
        return false;
    }

    // TODO: delegate to d_test_options once its enum-keyed map API
    // is finalized.  For now metadata is accepted but not stored.
    (void)_flag;
    (void)_value;

    return true;
}

/******************************************************************************
 * INTERNAL HELPERS
 *****************************************************************************/

/*
d_test_handler_get_time_ms
  Returns the current process time in milliseconds.

Parameter(s):
  none.
Return:
  The current clock time converted to milliseconds as a double.
*/
static double
d_test_handler_get_time_ms
(
    void
)
{
    return (double)clock() / CLOCKS_PER_SEC * 1000.0;
}

/******************************************************************************
 * CREATION AND DESTRUCTION
 *****************************************************************************/

/*
d_test_handler_new
  Creates a test handler with default settings and the given
configuration.

Parameter(s):
  _default_config: optional default test configuration; may be NULL.
Return:
  A pointer to the newly-created d_test_handler, or NULL on failure.
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
  Creates a test handler with event emission enabled.

Parameter(s):
  _default_config: optional default test configuration; may be NULL.
  _event_capacity: initial capacity of the event queue.
Return:
  A pointer to the newly-created d_test_handler, or NULL on failure.
*/
struct d_test_handler*
d_test_handler_new_with_events
(
    struct d_test_options* _default_config,
    size_t                 _event_capacity
)
{
    return d_test_handler_new_full(_default_config,
                                   _event_capacity,
                                   0,
                                   D_TEST_HANDLER_FLAG_EMIT_EVENTS);
}

/*
d_test_handler_new_full
  Creates a test handler with full control over event capacity, stack
capacity, and behavior flags.

Parameter(s):
  _default_config: optional default test configuration; may be NULL.
  _event_capacity: initial capacity of the event queue; 0 disables
                   events.
  _stack_capacity: initial capacity of the result/context stacks; 0
                   uses the default if TRACK_STACK flag is set.
  _flags:          bit flags controlling handler behavior
                   (see DTestHandlerFlag).
Return:
  A pointer to the newly-created d_test_handler, or NULL on failure.
*/
struct d_test_handler*
d_test_handler_new_full
(
    struct d_test_options* _default_config,
    size_t                 _event_capacity,
    size_t                 _stack_capacity,
    uint32_t               _flags
)
{
    struct d_test_handler* handler;

    handler = malloc(sizeof(struct d_test_handler));

    // ensure allocation succeeded
    if (!handler)
    {
        return NULL;
    }

    memset(handler, 0, sizeof(struct d_test_handler));

    // create event handler if events are requested
    if (_event_capacity > 0)
    {
        handler->event_handler =
            d_event_handler_new(_event_capacity, 16);

        // ensure event handler allocation succeeded
        if (!handler->event_handler)
        {
            free(handler);

            return NULL;
        }
    }

    handler->default_config = _default_config;

    // create stacks if tracking is requested
    if ( (_stack_capacity > 0) ||
         (_flags & D_TEST_HANDLER_FLAG_TRACK_STACK) )
    {
        handler->result_stack  = d_min_stack_new();
        handler->context_stack = d_min_stack_new();

        // ensure both stack allocations succeeded
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
    handler->abort_on_failure =
        (_flags & D_TEST_HANDLER_FLAG_ABORT_ON_FAIL) != 0;
    handler->output_stream    = stdout;

    return handler;
}

/*
d_test_handler_free
  Frees a d_test_handler and all owned resources including the event
handler, result/context stacks, and output buffer.

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
    if (_handler)
    {
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
    }

    return;
}

/******************************************************************************
 * FLAG MANAGEMENT
 *****************************************************************************/

/*
d_test_handler_set_flag
  Sets a behavior flag on the handler. If the ABORT_ON_FAIL flag is
set, the abort_on_failure field is also updated.

Parameter(s):
  _handler: the handler to modify; no-op if NULL.
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

        // sync the abort_on_failure convenience field
        if (_flag == D_TEST_HANDLER_FLAG_ABORT_ON_FAIL)
        {
            _handler->abort_on_failure = true;
        }
    }

    return;
}

/*
d_test_handler_clear_flag
  Clears a behavior flag on the handler. If the ABORT_ON_FAIL flag is
cleared, the abort_on_failure field is also updated.

Parameter(s):
  _handler: the handler to modify; no-op if NULL.
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

        // sync the abort_on_failure convenience field
        if (_flag == D_TEST_HANDLER_FLAG_ABORT_ON_FAIL)
        {
            _handler->abort_on_failure = false;
        }
    }

    return;
}

/*
d_test_handler_has_flag
  Tests whether a behavior flag is set on the handler.

Parameter(s):
  _handler: the handler to query; returns false if NULL.
  _flag:    the flag to test.
Return:
  A boolean value corresponding to either:
  - true, if the handler is non-NULL and the flag is set, or
  - false, if the handler is NULL or the flag is not set.
*/
bool
d_test_handler_has_flag
(
    const struct d_test_handler* _handler,
    enum DTestHandlerFlag        _flag
)
{
    if (!_handler)
    {
        return false;
    }

    return (_handler->flags & _flag) == (uint32_t)_flag;
}

/******************************************************************************
 * EVENT LISTENER REGISTRATION
 *****************************************************************************/

/*
d_test_handler_register_listener
  Creates an event listener and binds it to the handler's event system.

Parameter(s):
  _handler:  the handler whose event system receives the listener.
  _event_id: the event ID to listen for.
  _callback: the callback function invoked when the event fires.
  _enabled:  whether the listener starts enabled.
Return:
  A boolean value corresponding to either:
  - true, if the listener was created and bound successfully, or
  - false, if any parameter is invalid or allocation failed.
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

    // validate parameters
    if ( (!_handler)                ||
         (!_handler->event_handler) ||
         (!_callback) )
    {
        return false;
    }

    // create the listener
    listener = d_event_listener_new(_event_id,
                                    _callback,
                                    _enabled);

    // ensure allocation succeeded
    if (!listener)
    {
        return false;
    }

    // attempt to bind; free listener on failure
    if (!d_event_handler_bind(_handler->event_handler,
                               listener))
    {
        d_event_listener_free(listener);

        return false;
    }

    return true;
}

/*
d_test_handler_unregister_listener
  Removes an event listener from the handler's event system by ID.

Parameter(s):
  _handler:  the handler whose event system loses the listener.
  _event_id: the event ID of the listener to remove.
Return:
  A boolean value corresponding to either:
  - true, if the listener was found and removed, or
  - false, if the handler is NULL, has no event system, or the ID
    was not found.
*/
bool
d_test_handler_unregister_listener
(
    struct d_test_handler* _handler,
    d_event_id             _event_id
)
{
    // validate parameters
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
  Enables an event listener by ID.

Parameter(s):
  _handler: the handler whose event system contains the listener.
  _id:      the event ID of the listener to enable.
Return:
  A boolean value corresponding to either:
  - true, if the listener was found and enabled, or
  - false, if the handler is NULL, has no event system, or the ID
    was not found.
*/
bool
d_test_handler_enable_listener
(
    struct d_test_handler* _handler,
    d_event_id             _id
)
{
    // validate parameters
    if ( (!_handler) ||
         (!_handler->event_handler) )
    {
        return false;
    }

    return d_event_handler_enable_listener(
               _handler->event_handler,
               _id);
}

/*
d_test_handler_disable_listener
  Disables an event listener by ID.

Parameter(s):
  _handler: the handler whose event system contains the listener.
  _id:      the event ID of the listener to disable.
Return:
  A boolean value corresponding to either:
  - true, if the listener was found and disabled, or
  - false, if the handler is NULL, has no event system, or the ID
    was not found.
*/
bool
d_test_handler_disable_listener
(
    struct d_test_handler* _handler,
    d_event_id             _id
)
{
    // validate parameters
    if ( (!_handler) ||
         (!_handler->event_handler) )
    {
        return false;
    }

    return d_event_handler_disable_listener(
               _handler->event_handler,
               _id);
}

/******************************************************************************
 * EVENT EMISSION
 *****************************************************************************/

/*
d_test_handler_emit_event
  Creates a transient event carrying the given context and fires it
through the handler's event system. The event is freed after dispatch.
No-op if the handler is NULL, has no event system, or the EMIT_EVENTS
flag is not set.

Parameter(s):
  _handler:    the handler whose event system fires the event.
  _event_type: the DTestEvent type to emit.
  _context:    the test context to attach; may be NULL.
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

    // validate handler and event system
    if ( (!_handler) ||
         (!_handler->event_handler) )
    {
        return;
    }

    // check that event emission is enabled
    if (!d_test_handler_has_flag(_handler,
                                 D_TEST_HANDLER_FLAG_EMIT_EVENTS))
    {
        return;
    }

    // create, fire, and free the transient event
    event = d_event_new_ctx((d_event_id)_event_type, _context);

    if (event)
    {
        d_event_handler_fire_event(_handler->event_handler,
                                   event);
        d_event_free(event);
    }

    return;
}

/******************************************************************************
 * TEST EXECUTION
 *****************************************************************************/

/*
d_test_handler_run_test
  Executes a single test through the handler, emitting lifecycle events
and updating result statistics.

Parameter(s):
  _handler:    the handler coordinating execution.
  _test:       the test to execute.
  _run_config: optional run-time configuration; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the test passed, or
  - false, if the test failed or any parameter is invalid.
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

    // validate parameters
    if ( (!_handler)        ||
         (!_test)           ||
         (!_test->children) )
    {
        return false;
    }

    // initialize context for this test
    d_test_context_init(&context, _handler);
    context.current_test  = _test;
    context.depth         = _handler->current_depth;
    context.start_time_ms = d_test_handler_get_time_ms();

    // emit setup event
    context.event_type = D_TEST_EVENT_SETUP;
    d_test_handler_emit_event(_handler,
                               D_TEST_EVENT_SETUP,
                               &context);

    // emit start event
    context.event_type = D_TEST_EVENT_START;
    d_test_handler_emit_event(_handler,
                               D_TEST_EVENT_START,
                               &context);

    // execute the test
    result         = d_test_run(_test, _run_config);
    context.result = result;

    // update statistics and emit pass/fail event
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

    // emit end event
    context.end_time_ms = d_test_handler_get_time_ms();
    context.event_type  = D_TEST_EVENT_END;
    d_test_handler_emit_event(_handler,
                               D_TEST_EVENT_END,
                               &context);

    // emit teardown event
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

    // abort early on failure if configured
    if ( (!result) &&
         (_handler->abort_on_failure) )
    {
        return false;
    }

    return result;
}

/*
d_test_handler_run_block
  Executes all children in a test block through the handler, emitting
lifecycle events and updating result statistics.

Parameter(s):
  _handler:    the handler coordinating execution.
  _block:      the test block to execute.
  _run_config: optional run-time configuration; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if all children passed, or
  - false, if any child failed or a parameter is invalid.
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
    struct d_test_type*    child;
    struct d_test_options* config;

    // validate parameters
    if ( (!_handler) ||
         (!_block) )
    {
        return false;
    }

    // initialize context for this block
    all_passed = true;
    d_test_context_init(&context, _handler);
    context.current_block = _block;
    context.depth         = _handler->current_depth;
    context.start_time_ms = d_test_handler_get_time_ms();

    // update depth tracking
    _handler->current_depth++;

    if (_handler->current_depth > _handler->results.max_depth)
    {
        _handler->results.max_depth = _handler->current_depth;
    }

    _handler->results.blocks_run++;

    // emit setup event
    context.event_type = D_TEST_EVENT_SETUP;
    d_test_handler_emit_event(_handler,
                               D_TEST_EVENT_SETUP,
                               &context);

    // execute each child in the block
    child_count = d_test_block_child_count(_block);

    for (i = 0; i < child_count; i++)
    {
        child  = d_test_block_get_child_at(_block, i);
        config = _run_config ? _run_config
                             : _handler->default_config;

        if (!d_test_handler_run_test_type(_handler,
                                          child,
                                          config))
        {
            all_passed = false;

            if (_handler->abort_on_failure)
            {
                break;
            }
        }
    }

    // update block pass/fail statistics
    if (all_passed)
    {
        _handler->results.blocks_passed++;
    }
    else
    {
        _handler->results.blocks_failed++;
    }

    // emit teardown event
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
  Dispatches execution of a d_test_type node to the appropriate
type-specific run function.

Parameter(s):
  _handler:    the handler coordinating execution.
  _test_type:  the discriminated union node to execute.
  _run_config: optional run-time configuration; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the child passed, or
  - false, if the child failed or a parameter is invalid.
*/
bool
d_test_handler_run_test_type
(
    struct d_test_handler* _handler,
    struct d_test_type*    _test_type,
    struct d_test_options* _run_config
)
{
    // validate parameters
    if ( (!_handler)   ||
         (!_test_type) )
    {
        return false;
    }

    // dispatch by node type
    switch (_test_type->type)
    {
        case D_TEST_TYPE_TEST:
            return d_test_handler_run_test(
                       _handler,
                       _test_type->test,
                       _run_config);

        case D_TEST_TYPE_TEST_BLOCK:
            return d_test_handler_run_block(
                       _handler,
                       _test_type->block,
                       _run_config);

        case D_TEST_TYPE_MODULE:
            return d_test_handler_run_module(
                       _handler,
                       _test_type->module,
                       _run_config);

        default:
            return false;
    }
}

/*
d_test_handler_run_module
  Executes all children in a test module through the handler, emitting
lifecycle events and updating result statistics. Skipped modules are
counted but not executed.

Parameter(s):
  _handler:    the handler coordinating execution.
  _module:     the module to execute.
  _run_config: optional run-time configuration; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if all children passed or the module was skipped, or
  - false, if any child failed or a parameter is invalid.
*/
bool
d_test_handler_run_module
(
    struct d_test_handler* _handler,
    struct d_test_module*  _module,
    struct d_test_options* _run_config
)
{
    size_t                i;
    size_t                child_count;
    bool                  all_passed;
    struct d_test_context context;
    struct d_test_type*   child;

    // validate parameters
    if ( (!_handler) ||
         (!_module) )
    {
        return false;
    }

    // skip disabled modules
    if (!d_test_handler_module_is_enabled(_module))
    {
        _handler->results.modules_skipped++;

        return true;
    }

    // initialize context for this module
    all_passed = true;
    d_test_context_init(&context, _handler);
    context.current_module = _module;
    context.depth          = _handler->current_depth;
    context.start_time_ms  = d_test_handler_get_time_ms();

    _handler->current_depth++;
    _handler->results.modules_run++;

    // emit setup event
    context.event_type = D_TEST_EVENT_SETUP;
    d_test_handler_emit_event(_handler,
                               D_TEST_EVENT_SETUP,
                               &context);

    // execute each child in the module
    child_count = d_test_module_child_count(_module);

    for (i = 0; i < child_count; i++)
    {
        child = d_test_module_get_child_at(_module, i);

        if ( (child) &&
             (!d_test_handler_run_test_type(_handler,
                                            child,
                                            _run_config)) )
        {
            all_passed = false;

            if (_handler->abort_on_failure)
            {
                break;
            }
        }
    }

    // update module pass/fail statistics
    if (all_passed)
    {
        _handler->results.modules_passed++;
    }
    else
    {
        _handler->results.modules_failed++;
    }

    // emit teardown event
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
  Executes a test tree through the handler, emitting lifecycle events.

Parameter(s):
  _handler:    the handler coordinating execution.
  _tree:       the test tree to execute.
  _run_config: optional run-time configuration; may be NULL.
Return:
  A boolean value corresponding to either:
  - true, if the tree passed, or
  - false, if the tree failed or a parameter is invalid.
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

    // validate parameters
    if ( (!_handler) ||
         (!_tree) )
    {
        return false;
    }

    // initialize context for this tree
    d_test_context_init(&context, _handler);
    context.current_tree  = _tree;
    context.depth         = _handler->current_depth;
    context.start_time_ms = d_test_handler_get_time_ms();

    // emit setup event
    context.event_type = D_TEST_EVENT_SETUP;
    d_test_handler_emit_event(_handler,
                               D_TEST_EVENT_SETUP,
                               &context);

    // execute the tree
    result         = d_test_tree_run(_tree, _run_config);
    context.result = result;

    // emit teardown event
    context.end_time_ms = d_test_handler_get_time_ms();

    context.event_type  = D_TEST_EVENT_TEAR_DOWN;

    d_test_handler_emit_event(_handler,
                               D_TEST_EVENT_TEAR_DOWN,
                               &context);

    return result;
}

/*
d_test_handler_run_session
  Executes a test session through the handler, emitting lifecycle
events.

Parameter(s):
  _handler: the handler coordinating execution.
  _session: the test session to execute.
Return:
  A boolean value corresponding to either:
  - true, if the session passed, or
  - false, if the session failed or a parameter is invalid.
*/
bool
d_test_handler_run_session
(
    struct d_test_handler* _handler,
    struct d_test_session* _session
)
{
    struct d_test_context context;
    bool                  result;

    // validate parameters
    if ( (!_handler) ||
         (!_session) )
    {
        return false;
    }

    // initialize context for this session
    d_test_context_init(&context, _handler);
    context.current_session = _session;
    context.depth           = _handler->current_depth;
    context.start_time_ms   = d_test_handler_get_time_ms();

    // emit setup event
    context.event_type = D_TEST_EVENT_SETUP;
    d_test_handler_emit_event(_handler,
                               D_TEST_EVENT_SETUP,
                               &context);

    // execute the session
    result         = d_test_session_run(_session);
    context.result = result;

    // emit teardown event
    context.end_time_ms = d_test_handler_get_time_ms();
    context.event_type  = D_TEST_EVENT_TEAR_DOWN;
    d_test_handler_emit_event(_handler,
                               D_TEST_EVENT_TEAR_DOWN,
                               &context);

    return result;
}

/******************************************************************************
 * ASSERTION TRACKING
 *****************************************************************************/

/*
d_test_handler_record_assertion
  Records an assertion result in the handler's statistics.

Parameter(s):
  _handler:   the handler whose statistics will be updated.
  _assertion: the assertion to record.
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
    // validate parameters
    if ( (!_handler)   ||
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

/******************************************************************************
 * STACK OPERATIONS
 *****************************************************************************/

/*
d_test_handler_push_result
  Pushes a heap-allocated copy of the given result onto the handler's
result stack.

Parameter(s):
  _handler: the handler whose result stack receives the copy.
  _result:  the result to copy and push.
Return:
  A boolean value corresponding to either:
  - true, if the copy was allocated and pushed successfully, or
  - false, if any parameter is NULL or allocation failed.
*/
bool
d_test_handler_push_result
(
    struct d_test_handler*      _handler,
    const struct d_test_result* _result
)
{
    struct d_test_result* copy;

    // validate parameters
    if ( (!_handler)               ||
         (!_handler->result_stack) ||
         (!_result) )
    {
        return false;
    }

    // allocate a heap copy of the result
    copy = malloc(sizeof(struct d_test_result));

    // ensure allocation succeeded
    if (!copy)
    {
        return false;
    }

    memcpy(copy, _result, sizeof(struct d_test_result));

    return d_min_stack_push(_handler->result_stack, copy) != NULL;
}

/*
d_test_handler_pop_result
  Pops the top result from the handler's result stack, copies it into
the output parameter, and frees the heap allocation.

Parameter(s):
  _handler: the handler whose result stack will be popped.
  _out:     destination for the popped result; may be NULL to discard.
Return:
  A boolean value corresponding to either:
  - true, if a result was successfully popped, or
  - false, if the handler is NULL, has no stack, or the stack is
    empty.
*/
bool
d_test_handler_pop_result
(
    struct d_test_handler* _handler,
    struct d_test_result*  _out
)
{
    struct d_test_result* top;

    // validate parameters
    if ( (!_handler)               ||
         (!_handler->result_stack) )
    {
        return false;
    }

    // pop the top element
    top = (struct d_test_result*)d_min_stack_pop(
              _handler->result_stack);

    if (!top)
    {
        return false;
    }

    // copy to output if provided
    if (_out)
    {
        memcpy(_out, top, sizeof(struct d_test_result));
    }

    free(top);

    return true;
}

/*
d_test_handler_push_context
  Pushes a heap-allocated copy of the given context onto the handler's
context stack.

Parameter(s):
  _handler: the handler whose context stack receives the copy.
  _context: the context to copy and push.
Return:
  A boolean value corresponding to either:
  - true, if the copy was allocated and pushed successfully, or
  - false, if any parameter is NULL or allocation failed.
*/
bool
d_test_handler_push_context
(
    struct d_test_handler*       _handler,
    const struct d_test_context* _context
)
{
    struct d_test_context* copy;

    // validate parameters
    if ( (!_handler)                ||
         (!_handler->context_stack) ||
         (!_context) )
    {
        return false;
    }

    // allocate a heap copy of the context
    copy = malloc(sizeof(struct d_test_context));

    // ensure allocation succeeded
    if (!copy)
    {
        return false;
    }

    memcpy(copy, _context, sizeof(struct d_test_context));

    return d_min_stack_push(_handler->context_stack,
                            copy) != NULL;
}

/*
d_test_handler_pop_context
  Pops the top context from the handler's context stack, copies it
into the output parameter, and frees the heap allocation.

Parameter(s):
  _handler: the handler whose context stack will be popped.
  _out:     destination for the popped context; may be NULL to
            discard.
Return:
  A boolean value corresponding to either:
  - true, if a context was successfully popped, or
  - false, if the handler is NULL, has no stack, or the stack is
    empty.
*/
bool
d_test_handler_pop_context
(
    struct d_test_handler* _handler,
    struct d_test_context* _out
)
{
    struct d_test_context* top;

    // validate parameters
    if ( (!_handler)                ||
         (!_handler->context_stack) )
    {
        return false;
    }

    // pop the top element
    top = (struct d_test_context*)d_min_stack_pop(
              _handler->context_stack);

    if (!top)
    {
        return false;
    }

    // copy to output if provided
    if (_out)
    {
        memcpy(_out, top, sizeof(struct d_test_context));
    }

    free(top);

    return true;
}

/******************************************************************************
 * RESULT QUERIES
 *****************************************************************************/

/*
d_test_handler_get_results
  Returns a copy of the handler's aggregated test results.

Parameter(s):
  _handler: the handler to query; returns zeroed struct if NULL.
Return:
  A d_test_result struct containing the current statistics.
*/
struct d_test_result
d_test_handler_get_results
(
    const struct d_test_handler* _handler
)
{
    struct d_test_result empty = {0};

    if (!_handler)
    {
        return empty;
    }

    return _handler->results;
}

/*
d_test_handler_reset_results
  Zeroes all result counters and resets the current depth to 0.

Parameter(s):
  _handler: the handler to reset; no-op if NULL.
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

    memset(&_handler->results, 0, sizeof(struct d_test_result));
    _handler->current_depth = 0;

    return;
}

/*
d_test_handler_print_results
  Prints a formatted summary of test results to stdout.

Parameter(s):
  _handler: the handler to report; no-op if NULL.
  _label:   a label for the header; defaults to "Unnamed".
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
    printf("Tests:      %zu run, %zu passed,"
           " %zu failed, %zu skipped\n",
           r->tests_run,
           r->tests_passed,
           r->tests_failed,
           r->tests_skipped);
    printf("Assertions: %zu run, %zu passed,"
           " %zu failed\n",
           r->assertions_run,
           r->assertions_passed,
           r->assertions_failed);
    printf("Blocks:     %zu run, %zu passed,"
           " %zu failed\n",
           r->blocks_run,
           r->blocks_passed,
           r->blocks_failed);
    printf("Modules:    %zu run, %zu passed,"
           " %zu failed, %zu skipped\n",
           r->modules_run,
           r->modules_passed,
           r->modules_failed,
           r->modules_skipped);
    printf("Max depth:  %zu\n", r->max_depth);
    printf("Pass Rate:  %.2f%% (tests),"
           " %.2f%% (assertions)\n",
           d_test_handler_get_pass_rate(_handler),
           d_test_handler_get_assertion_rate(_handler));
    printf("========================================\n");

    return;
}

/*
d_test_handler_get_pass_rate
  Returns the test pass rate as a percentage.

Parameter(s):
  _handler: the handler to query.
Return:
  The pass rate as a double in [0.0, 100.0], or 0.0 if the handler
is NULL or no tests have been run.
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
  Returns the assertion pass rate as a percentage.

Parameter(s):
  _handler: the handler to query.
Return:
  The assertion pass rate as a double in [0.0, 100.0], or 0.0 if the
handler is NULL or no assertions have been run.
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

/******************************************************************************
 * CONTEXT HELPERS
 *****************************************************************************/

/*
d_test_context_new
  Allocates and initializes a new d_test_context on the heap.

Parameter(s):
  _handler: the handler to associate with the context.
Return:
  A pointer to the newly-created d_test_context, or NULL on failure.
*/
struct d_test_context*
d_test_context_new
(
    struct d_test_handler* _handler
)
{
    struct d_test_context* context;

    context = malloc(sizeof(struct d_test_context));

    // initialize if allocation succeeded
    if (context)
    {
        d_test_context_init(context, _handler);
    }

    return context;
}

/*
d_test_context_init
  Initializes a d_test_context to default values.

Parameter(s):
  _context: the context to initialize; no-op if NULL.
  _handler: the handler to associate with the context.
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

    memset(_context, 0, sizeof(struct d_test_context));
    _context->handler = _handler;
    _context->result  = true;

    return;
}

/*
d_test_context_free
  Frees a heap-allocated d_test_context.

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

/******************************************************************************
 * DSL HELPER FUNCTIONS
 *****************************************************************************/

/*
d_test_handler_create_module_with_metadata
  Creates a test module and populates it with the given metadata
key-value pairs.

Parameter(s):
  _name:     the module name to set via metadata.
  _metadata: array of key-value pairs to apply.
  _count:    number of elements in the metadata array.
Return:
  A pointer to the newly-created d_test_module, or NULL on failure.
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
    size_t                i;

    module = d_test_module_new(NULL, 0);

    // ensure allocation succeeded
    if (!module)
    {
        return NULL;
    }

    // set the module name via metadata
    d_test_module_set_metadata_str(module, "name", _name);

    // apply remaining metadata entries
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
  Maps a string key to a DTestMetadataFlag and delegates to
d_test_module_set_metadata.

Parameter(s):
  _module: the module whose metadata is being set.
  _key:    the string key (e.g. "name", "author", "version").
  _value:  the string value to store.
Return:
  A boolean value corresponding to either:
  - true, if the key was recognized and the value was set, or
  - false, if the module or key is NULL, or the key is unrecognized.
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

    // validate parameters
    if ( (!_module) ||
         (!_key) )
    {
        return false;
    }

    // map the string key to a metadata flag
    if (strcmp(_key, "name") == 0)
    {
        flag = D_TEST_METADATA_NAME;
    }
    else if ( (strcmp(_key, "author") == 0) ||
              (strcmp(_key, "authors") == 0) )
    {
        flag = D_TEST_METADATA_AUTHORS;
    }
    else if (strcmp(_key, "version") == 0)
    {
        flag = D_TEST_METADATA_VERSION_STRING;
    }
    else if (strcmp(_key, "description") == 0)
    {
        flag = D_TEST_METADATA_DESCRIPTION;
    }
    else if (strcmp(_key, "category") == 0)
    {
        flag = D_TEST_METADATA_CATEGORY;
    }
    else if (strcmp(_key, "module") == 0)
    {
        flag = D_TEST_METADATA_MODULE_NAME;
    }
    else if (strcmp(_key, "tags") == 0)
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
  Creates an empty d_test from a DSL node array. The nodes are not yet
wired as children (placeholder for future DSL expansion).

Parameter(s):
  _nodes: array of DSL nodes.
  _count: number of nodes in the array.
Return:
  A pointer to the newly-created d_test, or NULL if the array is NULL
or empty.
*/
struct d_test*
d_test_handler_create_test_from_nodes
(
    const struct d_test_dsl_node* _nodes,
    size_t                        _count
)
{
    // validate parameters
    if ( (!_nodes)     ||
         (_count == 0) )
    {
        return NULL;
    }

    return d_test_new(NULL, 0);
}

/*
d_test_handler_create_block_from_nodes
  Creates a d_test_block and populates it with children from the given
DSL node array.

Parameter(s):
  _name:  the block name (reserved for future use).
  _nodes: array of DSL nodes to add as children.
  _count: number of nodes in the array.
Return:
  A pointer to the newly-created d_test_block, or NULL on failure.
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

    // validate parameters
    if ( (!_nodes)     ||
         (_count == 0) )
    {
        return NULL;
    }

    block = d_test_block_new(NULL, 0);

    // ensure allocation succeeded
    if (!block)
    {
        return NULL;
    }

    // add each node as a child based on its type
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
  Creates a d_test_module from metadata key-value pairs and populates
it with children from the given DSL node array.

Parameter(s):
  _metadata:       array of metadata key-value pairs.
  _metadata_count: number of metadata entries.
  _children:       array of DSL nodes to add as children.
  _children_count: number of child nodes.
Return:
  A pointer to the newly-created d_test_module, or NULL on failure.
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

    name = "Unnamed Module";

    // search metadata for a "name" entry
    for (i = 0; i < _metadata_count; i++)
    {
        if ( (_metadata[i].key) &&
             (strcmp(_metadata[i].key, "name") == 0) )
        {
            name = _metadata[i].value;
            break;
        }
    }

    // create the module with all metadata applied
    module = d_test_handler_create_module_with_metadata(
                 name,
                 _metadata,
                 _metadata_count);

    // ensure allocation succeeded
    if (!module)
    {
        return NULL;
    }

    // add each child node based on its type
    for (i = 0; i < _children_count; i++)
    {
        if ( (_children[i].type == D_TEST_DSL_NODE_TEST) &&
             (_children[i].test) )
        {
            d_test_module_add_test(module, _children[i].test);
        }
        else if ( (_children[i].type == D_TEST_DSL_NODE_BLOCK) &&
                  (_children[i].block) )
        {
            d_test_module_add_block(module, _children[i].block);
        }
    }

    return module;
}