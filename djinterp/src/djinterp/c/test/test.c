#include "../../../inc/c/test/test.h"
#include "../../../inc/c/test/test_cvar.h"


/******************************************************************************
 * INTERNAL HELPERS
 *****************************************************************************/

/*
d_internal_test_process_args
  Processes key-value arguments and applies them to the test's configuration.

Parameter(s):
  _test:      the test to configure.
  _args:      array of key-value argument pairs.
  _arg_count: number of arguments.
Return:
  true if all arguments were processed successfully.
*/
static bool
d_internal_test_process_args
(
    struct d_test*           _test,
    const struct d_test_arg* _args,
    size_t                   _arg_count
)
{
    size_t                      i;
    struct d_test_registry_row* row;

    if ( (!_test) ||
         (!_args && _arg_count > 0) )
    {
        return false;
    }

    for (i = 0; i < _arg_count; i++)
    {
        // look up the key in the test registry
        row = d_test_registry_find(_args[i].key);

        if (!row)
        {
            // unknown key - could log a warning here
            continue;
        }

        // apply the value based on row type
        if (_test->config && _test->config->settings)
        {
            d_min_enum_map_put(_test->config->settings,
                               (int)row->flag,
                               (void*)_args[i].value);
        }
    }   

    return true;
}

/*
d_internal_test_add_children
  Adds child test types to the test.

Parameter(s):
  _test:        the test to add children to.
  _children:    array of child test type pointers.
  _child_count: number of children.
Return:
  true if all children were added successfully.
*/
static bool
d_internal_test_add_children
(
    struct d_test*       _test,
    struct d_test_type** _children,
    size_t               _child_count
)
{
    size_t i;

    if ( (!_test)                    ||
         (!_test->children)          ||
         (!_children && _child_count > 0) )
    {
        return false;
    }

    for (i = 0; i < _child_count; i++)
    {
        if (!_children[i])
        {
            continue;
        }

        // tests can only contain assertions and test functions
        if ( (_children[i]->type != D_TEST_TYPE_ASSERT) &&
             (_children[i]->type != D_TEST_TYPE_TEST_FN) )
        {
            // skip invalid child types
            continue;
        }

        if (!d_ptr_vector_push_back(_test->children, _children[i]))
        {
            return false;
        }
    }

    return true;
}

/*
d_internal_test_resolve_config
  Resolves effective configuration by merging test's own config with run config.
  The run config takes precedence for overridable settings.

Parameter(s):
  _test:       the test whose config to resolve.
  _run_config: the runtime configuration passed from parent.
Return:
  Pointer to the resolved config (may be test's own config or run config).
  Does not allocate new memory - returns one of the input configs.
*/
static const struct d_test_options*
d_internal_test_resolve_config
(
    const struct d_test*        _test,
    const struct d_test_options* _run_config
)
{
    // If run config provided, it takes precedence
    if (_run_config)
    {
        return _run_config;
    }

    // Fall back to test's own config
    if (_test && _test->config)
    {
        return _test->config;
    }

    return NULL;
}

/******************************************************************************
 * VALIDATE_ARGS FUNCTIONS
 *****************************************************************************/

/*
d_test_validate_args
  Validates and converts argument array to a test config.

Parameter(s):
  _args:      array of key-value argument pairs.
  _arg_count: number of arguments.
Return:
  Pointer to the test config, or NULL if no valid args.
*/
struct d_test_options*
d_test_validate_args
(
    struct d_test_arg* _args,
    size_t             _arg_count
)
{
    struct d_test_options*       config;
    struct d_test_registry_row* row;
    size_t                      i;

    if ( (!_args) || 
         (_arg_count == 0) )
    {
        return NULL;
    }

    config = d_test_options_new(D_TEST_MODE_NORMAL);

    if (!config)
    {
        return NULL;
    }

    if (!config->settings)
    {
        config->settings = d_min_enum_map_new();

        if (!config->settings)
        {
            d_test_options_free(config);

            return NULL;
        }
    }

    for (i = 0; i < _arg_count; i++)
    {
        if (!_args[i].key)
        {
            continue;
        }

        // look up the key in the registry
        row = d_test_registry_find(_args[i].key);

        // validate: key must exist AND have TESTS flag set
        if ( (!row) || 
             ((row->command_flags & D_TEST_REGISTRY_FLAG_TESTS) == 0) )
        {
            continue;
        }

        // store with the registry flag as the key
        d_min_enum_map_put(config->settings, (int)row->flag, (void*)_args[i].value);
    }

    return config;
}

/*
d_test_fn_validate_args
  Validates and converts argument array to a test config for test functions.

Parameter(s):
  _args:      array of key-value argument pairs.
  _arg_count: number of arguments.
Return:
  Pointer to the test config, or NULL if no valid args.
*/
struct d_test_options*
d_test_fn_validate_args
(
    struct d_test_arg* _args,
    size_t             _arg_count
)
{
    struct d_test_options*       config;
    struct d_test_registry_row* row;
    size_t                      i;

    if (!_args || _arg_count == 0)
    {
        return NULL;
    }

    config = d_test_options_new(D_TEST_MODE_NORMAL);

    if (!config)
    {
        return NULL;
    }

    if (!config->settings)
    {
        config->settings = d_min_enum_map_new();

        if (!config->settings)
        {
            d_test_options_free(config);

            return NULL;
        }
    }

    for (i = 0; i < _arg_count; i++)
    {
        if (!_args[i].key)
        {
            continue;
        }

        // look up the key in the registry
        row = d_test_registry_find(_args[i].key);

        // validate: key must exist AND have TEST_FN flag set
        if ( (!row) || 
             ((row->command_flags & D_TEST_REGISTRY_FLAG_TEST_FN) == 0) )
        {
            continue;
        }

        // store with the registry flag as the key
        d_min_enum_map_put(config->settings, (int)row->flag, (void*)_args[i].value);
    }

    for (i = 0; i < _arg_count; i++)
    {
        if (!_args[i].key)
        {
            continue;
        }

        // look up the key in the test registry
        row = d_test_registry_find(_args[i].key);

        if (row)
        {
            // store with the registry flag as the key
            d_min_enum_map_put(config->settings, (int)row->flag, (void*)_args[i].value);
        }
    }

    return config;
}

/******************************************************************************
 * CONSTRUCTOR FUNCTIONS
 *****************************************************************************/

/*
d_test_new
  Creates a new test with the specified children.

Parameter(s):
  _children:    array of child test type pointers.
  _child_count: number of children.
Return:
  Pointer to the newly created test, or NULL on failure.
*/
struct d_test*
d_test_new
(
    struct d_test_type** _children,
    size_t               _child_count
)
{
    struct d_test* new_test;

    new_test = malloc(sizeof(struct d_test));

    // ensure that memory allocation was successful
    if (!new_test)
    {
        return NULL;
    }

    // create children vector
    new_test->children = d_ptr_vector_new_default();

    if (!new_test->children)
    {
        free(new_test);

        return NULL;
    }

    // create default config
    new_test->config = d_test_options_new(D_TEST_MODE_NORMAL);

    if (!new_test->config)
    {
        d_ptr_vector_free(new_test->children);
        free(new_test);

        return NULL;
    }

    new_test->stage_hooks = NULL;

    // add children if provided
    if (_children && _child_count > 0)
    {
        if (!d_internal_test_add_children(new_test, _children, _child_count))
        {
            d_test_free(new_test);

            return NULL;
        }
    }

    return new_test;
}

/*
d_test_new_args
  Creates a new test with configuration arguments and children.

Parameter(s):
  _args:        array of key-value configuration arguments.
  _arg_count:   number of arguments.
  _children:    array of child test type pointers.
  _child_count: number of children.
Return:
  Pointer to the newly created test, or NULL on failure.
*/
struct d_test*
d_test_new_args
(
    struct d_test_arg*   _args,
    size_t               _arg_count,
    struct d_test_type** _children,
    size_t               _child_count
)
{
    struct d_test* test;

    test = malloc(sizeof(struct d_test));

    // ensure that memory allocation was successful
    if (!test)
    {
        return NULL;
    }

    // create children vector
    test->children = d_ptr_vector_new_default();

    // ensure that memory allocation was successful
    if (!test->children)
    {
        free(test);

        return NULL;
    }

    // create config with settings map
    test->config = d_test_options_new(D_TEST_MODE_NORMAL);

    if (!test->config)
    {
        d_ptr_vector_free(test->children);
        free(test);

        return NULL;
    }

    // ensure the config has a settings map
    if (!test->config->settings)
    {
        test->config->settings = d_min_enum_map_new();

        if (!test->config->settings)
        {
            d_test_free(test);

            return NULL;
        }
    }

    test->stage_hooks = NULL;

    // process configuration arguments
    if (_args && _arg_count > 0)
    {
        d_internal_test_process_args(test, _args, _arg_count);
    }

    // add children if provided
    if (_children && _child_count > 0)
    {
        if (!d_internal_test_add_children(test, _children, _child_count))
        {
            d_test_free(test);

            return NULL;
        }
    }

    return test;
}

/******************************************************************************
 * DESTRUCTOR FUNCTIONS
 *****************************************************************************/

/*
d_test_fn_free
  Frees a test function wrapper.

Parameter(s):
  _test_fn: the test function to free.
Return:
  none
*/
void
d_test_fn_free
(
    struct d_test_fn* _test_fn
)
{
    if (!_test_fn)
    {
        return;
    }

    if (_test_fn->args)
    {
        free(_test_fn->args);
    }

    free(_test_fn);

    return;
}

/*
d_test_free
  Frees the test and all its resources.

Parameter(s):
  _test: the test to free.
Return:
  none
*/
void
d_test_free
(
    struct d_test* _test
)
{
    if (!_test)
    {
        return;
    }

    if (_test->children)
    {
        // Note: children should be freed by caller or via d_test_type_free
        d_ptr_vector_free(_test->children);
    }

    if (_test->config)
    {
        d_test_options_free(_test->config);
    }

    if (_test->stage_hooks)
    {
        d_min_enum_map_free(_test->stage_hooks);
    }

    free(_test);

    return;
}

/******************************************************************************
 * TEST FUNCTION WRAPPER
 *****************************************************************************/

/*
d_test_fn_new
  Creates a new test function wrapper.

Parameter(s):
  _fn: the test function to wrap.
Return:
  Pointer to the newly created test function wrapper, or NULL on failure.
*/
struct d_test_fn*
d_test_fn_new
(
    fn_test _fn
)
{
    struct d_test_fn* test_fn;

    test_fn = (struct d_test_fn*)calloc(1, sizeof(struct d_test_fn));

    if (!test_fn)
    {
        return NULL;
    }

    test_fn->test_fn = _fn;
    test_fn->count   = 0;
    test_fn->args    = NULL;

    return test_fn;
}

/******************************************************************************
 * TEST TYPE FUNCTIONS
 *****************************************************************************/

/*
d_test_type_new
  Creates a new test type wrapper with the specified type tag and data.

Parameter(s):
  _type: the type flag identifying the wrapped item.
  _data: pointer to the wrapped item (d_assert*, d_test_fn*, etc.).
Return:
  Pointer to the newly created test type, or NULL on failure.
*/
struct d_test_type*
d_test_type_new
(
    enum DTestTypeFlag _type,
    void*              _data
)
{
    struct d_test_type* type;

    type = (struct d_test_type*)calloc(1, sizeof(struct d_test_type));

    if (!type)
    {
        return NULL;
    }

    type->type = _type;

    switch (_type)
    {
        case D_TEST_TYPE_ASSERT:
            type->assertion = (struct d_assert*)_data;
            break;

        case D_TEST_TYPE_TEST_FN:
            type->test_fn = (struct d_test_fn*)_data;
            break;

        case D_TEST_TYPE_TEST:
            type->test = (struct d_test*)_data;
            break;

        case D_TEST_TYPE_TEST_BLOCK:
            type->block = (struct d_test_block*)_data;
            break;

        case D_TEST_TYPE_MODULE:
            type->module = (struct d_test_module*)_data;
            break;

        case D_TEST_TYPE_UNKNOWN:
        default:
            break;
    }

    return type;
}

/*
d_test_type_free
  Frees a test type wrapper. Does NOT free the wrapped item.

Parameter(s):
  _type: the test type to free.
*/
void
d_test_type_free
(
    struct d_test_type* _type
)
{
    if (!_type)
    {
        return;
    }

    free(_type);

    return;
}

/******************************************************************************
 * CHILD MANAGEMENT FUNCTIONS
 *****************************************************************************/

/*
d_test_add_child
  Adds a child to the test.

Parameter(s):
  _test:  the test to add to.
  _child: the child to add.
Return:
  true if the child was added successfully.
*/
bool
d_test_add_child
(
    struct d_test*      _test,
    struct d_test_type* _child
)
{
    if ( (!_test)           ||
         (!_test->children) ||
         (!_child) )
    {
        return false;
    }

    // tests can only contain assertions and test functions
    if ( (_child->type != D_TEST_TYPE_ASSERT) &&
         (_child->type != D_TEST_TYPE_TEST_FN) )
    {
        return false;
    }

    return d_ptr_vector_push_back(_test->children, _child);
}

/*
d_test_add_assertion
  Adds an assertion to the test.

Parameter(s):
  _test:      the test to add to.
  _assertion: the assertion to add.
Return:
  true if the assertion was added successfully.
*/
bool
d_test_add_assertion
(
    struct d_test*      _test,
    struct d_assert* _assertion
)
{
    struct d_test_type* test_type;

    if ( (!_test) ||
         (!_assertion) )
    {
        return false;
    }

    test_type = d_test_type_new(D_TEST_TYPE_ASSERT, _assertion);

    if (!test_type)
    {
        return false;
    }

    return d_test_add_child(_test, test_type);
}

/*
d_test_add_function
  Adds a test function to the test.

Parameter(s):
  _test: the test to add to.
  _fn:   the test function to add.
Return:
  true if the function was added successfully.
*/
bool
d_test_add_function
(
    struct d_test* _test,
    fn_test        _fn
)
{
    struct d_test_fn*   test_fn;
    struct d_test_type* test_type;

    if ( (!_test) ||
         (!_fn) )
    {
        return false;
    }

    test_fn = d_test_fn_new(_fn);

    if (!test_fn)
    {
        return false;
    }

    test_type = d_test_type_new(D_TEST_TYPE_TEST_FN, test_fn);

    if (!test_type)
    {
        d_test_fn_free(test_fn);

        return false;
    }

    return d_test_add_child(_test, test_type);
}

/*
d_test_child_count
  Returns the number of children in the test.

Parameter(s):
  _test: the test to query.
Return:
  Number of children in the test.
*/
size_t
d_test_child_count
(
    const struct d_test* _test
)
{
    if ( (!_test) ||
         (!_test->children) )
    {
        return 0;
    }

    return d_ptr_vector_size(_test->children);
}

/*
d_test_get_child_at
  Gets the child at the specified index.

Parameter(s):
  _test:  the test to query.
  _index: the index of the child to get.
Return:
  Pointer to the child's test type, or NULL if invalid.
*/
struct d_test_type*
d_test_get_child_at
(
    const struct d_test* _test,
    size_t               _index
)
{
    if ( (!_test) ||
         (!_test->children) )
    {
        return NULL;
    }

    return (struct d_test_type*)d_ptr_vector_at(_test->children, (d_index)_index);
}

/******************************************************************************
 * STAGE HOOK FUNCTIONS
 *****************************************************************************/

/*
d_test_set_stage_hook
  Sets a lifecycle hook for the specified stage.

Parameter(s):
  _test:  the test.
  _stage: the stage to set the hook for.
  _hook:  the hook function.
Return:
  true if the hook was set successfully.
*/
bool
d_test_set_stage_hook
(
    struct d_test*  _test,
    enum DTestStage _stage,
    fn_stage        _hook
)
{
    if (!_test)
    {
        return false;
    }

    // create stage hooks map if needed
    if (!_test->stage_hooks)
    {
        _test->stage_hooks = d_min_enum_map_new();

        if (!_test->stage_hooks)
        {
            return false;
        }
    }

    return d_min_enum_map_put(_test->stage_hooks, (int)_stage, (void*)_hook);
}

/*
d_test_get_stage_hook
  Gets the lifecycle hook for the specified stage.

Parameter(s):
  _test:  the test.
  _stage: the stage to get the hook for.
Return:
  The hook function, or NULL if not set.
*/
fn_stage
d_test_get_stage_hook
(
    const struct d_test* _test,
    enum DTestStage      _stage
)
{
    if ( (!_test) || 
         (!_test->stage_hooks) )
    {
        return NULL;
    }

    return (fn_stage)d_min_enum_map_get(_test->stage_hooks, (int)_stage);
}

/******************************************************************************
 * EXECUTION FUNCTIONS
 *****************************************************************************/

/*
d_test_run
  Runs all children in the test.

Parameter(s):
  _test:       the test to run.
  _run_config: runtime configuration (from parent, may be NULL).
Return:
  true if all children passed, false otherwise.
*/
bool
d_test_run
(
    struct d_test*              _test,
    const struct d_test_options* _run_config
)
{
    size_t                      i;
    size_t                      count;
    struct d_test_type*         child;
    const struct d_test_options* effective_config;
    fn_stage                    setup_hook;
    fn_stage                    teardown_hook;
    fn_stage                    success_hook;
    fn_stage                    failure_hook;
    bool                        all_passed;
    bool                        child_result;

    if (!_test)
    {
        return false;
    }

    // resolve effective configuration
    effective_config = d_internal_test_resolve_config(_test, _run_config);
    (void)effective_config;  // may be used for future config-based behavior

    all_passed = true;

    // get stage hooks from test's own hooks only
    setup_hook    = d_test_get_stage_hook(_test, D_TEST_STAGE_SETUP);
    teardown_hook = d_test_get_stage_hook(_test, D_TEST_STAGE_TEAR_DOWN);
    success_hook  = d_test_get_stage_hook(_test, D_TEST_STAGE_ON_SUCCESS);
    failure_hook  = d_test_get_stage_hook(_test, D_TEST_STAGE_ON_FAILURE);

    // run setup hook if present
    if (setup_hook)
    {
        if (!setup_hook(_test))
        {
            if (teardown_hook)
            {
                teardown_hook(_test);
            }

            return false;
        }
    }

    // run all children
    count = d_test_child_count(_test);

    for (i = 0; i < count; i++)
    {
        child = d_test_get_child_at(_test, i);

        if (!child)
        {
            continue;
        }

        child_result = false;

        switch (child->type)
        {
            case D_TEST_TYPE_ASSERT:
                if (child->D_KEYWORD_TEST_ASSERTION)
                {
                    child_result = child->D_KEYWORD_TEST_ASSERTION->result;
                }
                break;

            case D_TEST_TYPE_TEST_FN:
                if (child->D_KEYWORD_TEST_TEST_FN && child->D_KEYWORD_TEST_TEST_FN->test_fn)
                {
                    child_result = child->D_KEYWORD_TEST_TEST_FN->test_fn();
                }
                break;

            default:
                // tests can only contain assertions and test functions
                break;
        }

        if (!child_result)
        {
            all_passed = false;
        }
    }

    // call success/failure hooks
    if (all_passed)
    {
        if (success_hook)
        {
            success_hook(_test);
        }
    }
    else
    {
        if (failure_hook)
        {
            failure_hook(_test);
        }
    }

    // run teardown hook if present
    if (teardown_hook)
    {
        teardown_hook(_test);
    }

    return all_passed;
}

/******************************************************************************
 * UTILITY FUNCTIONS
 *****************************************************************************/

/*
d_test_print
  Prints test information for debugging.

Parameter(s):
  _test:          test to print.
  _prefix:        indentation prefix.
  _prefix_length: length of prefix.
*/
void
d_test_print
(
    const struct d_test* _test,
    const char*          _prefix,
    size_t               _prefix_length
)
{
    const char* actual_prefix;

    (void)_prefix_length;

    actual_prefix = _prefix ? _prefix : "";

    if (!_test)
    {
        printf("%sTest: NULL\n", actual_prefix);

        return;
    }

    printf("%s%s Test [%zu children]\n",
           actual_prefix,
           D_TEST_SYMBOL_LEAF,
           d_test_child_count(_test));

    return;
}

/*
d_test_type_flag_to_string
  Converts a test type flag to string representation.

Parameter(s):
  _type: the type to convert.
Return:
  String representation of the type.
*/
const char*
d_test_type_flag_to_string
(
    enum DTestTypeFlag _type
)
{
    switch (_type)
    {
        case D_TEST_TYPE_TEST_FN:
            return "TEST_FN";

        case D_TEST_TYPE_ASSERT:
            return "ASSERTION";

        case D_TEST_TYPE_TEST:
            return "TEST";

        case D_TEST_TYPE_TEST_BLOCK:
            return "TEST_BLOCK";

        case D_TEST_TYPE_MODULE:
            return "MODULE";

        case D_TEST_TYPE_UNKNOWN:
        default:
            return "UNKNOWN";
    }
}
