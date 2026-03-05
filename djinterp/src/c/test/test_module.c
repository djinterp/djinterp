#include "../../../inc/c/test/test_module.h"

// no args
#define D_TEST_MODULE(...)                               \
    &(struct d_test_type)                                \
    {                                                    \
        .type   = D_TEST_TYPE_MODULE,                    \
        .config = NULL,                                  \
        .module = &(struct d_test_module)                \
         {                                               \
            .children = D_PTR_VECTOR_PTR(__VA_ARGS__),   \
            .result   = D_TEST_MODULE_RESULTS_DEFAULT,   \
            .status   = D_TEST_MODULE_STATUS_PENDING     \
         }                                               \
    }

// args
#define D_TEST_MODULE(args, count, ...)                  \
    &(struct d_test_type)                                \
    {                                                    \
        .type   = D_TEST_TYPE_MODULE,                    \
        .config = d_test_config_new_args(args, count),   \
        .module = &(struct d_test_module)                \
         {                                               \
            .children = D_PTR_VECTOR_PTR(__VA_ARGS__),   \
            .result   = D_TEST_MODULE_RESULTS_DEFAULT,   \
            .status   = D_TEST_MODULE_STATUS_PENDING     \
         }                                               \
    }


/******************************************************************************
 * INTERNAL HELPERS
 *****************************************************************************/

/*
d_internal_test_module_init_result
  Initializes a module result structure to default values.
*/
D_STATIC_INLINE void
d_internal_test_module_init_result
(
    struct d_test_module_result* _result
)
{
    if (!_result)
    {
        return;
    }

    _result->tests_total       = 0;
    _result->tests_passed      = 0;
    _result->tests_failed      = 0;
    _result->tests_skipped     = 0;
    _result->assertions_total  = 0;
    _result->assertions_passed = 0;
    _result->blocks_total      = 0;
    _result->blocks_passed     = 0;
    _result->status            = D_TEST_MODULE_STATUS_UNKNOWN;
    _result->duration_ms       = 0.0;

    return;
}


/*
d_internal_test_module_add_children
  Adds child test types to the module.
*/
static bool
d_internal_test_module_add_children
(
    struct d_test_module* _module,
    struct d_test_type**  _children,
    size_t                _child_count
)
{
    size_t i;

    if ( (!_module)           ||
         (!_module->children) ||
         ( (!_children) && _child_count > 0) )
    {
        return false;
    }

    for (i = 0; i < _child_count; i++)
    {
        if (!_children[i])
        {
            continue;
        }

        // module only accepts test blocks as children
        if (_children[i]->type != D_TEST_TYPE_TEST_BLOCK)
        {
            continue;
        }

        if (!d_ptr_vector_push_back(_module->children, _children[i]))
        {
            return false;
        }
    }

    return true;
}


/******************************************************************************
 * VALIDATE_ARGS FUNCTION
 *****************************************************************************/

struct d_test_options*
d_test_module_validate_args
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

        // validate: key must exist AND have MODULES flag set
        if ( (!row) || 
             ((row->command_flags & D_TEST_REGISTRY_FLAG_MODULES) == 0) )
        {
            continue;
        }

        // store with the registry flag as the key
        d_min_enum_map_put(config->settings, (int)row->flag, (void*)_args[i].value);
    }

    return config;
}

/******************************************************************************
 * CONSTRUCTOR/DESTRUCTOR FUNCTIONS
 *****************************************************************************/

struct d_test_module*
d_test_module_new
(
    struct d_test_type** _children,
    size_t               _child_count
)
{
    struct d_test_module* new_module;

    new_module = (struct d_test_module*)calloc(1, sizeof(struct d_test_module));

    if (!new_module)
    {
        return NULL;
    }

    // create children vector
    new_module->children = d_ptr_vector_new_default();

    if (!new_module->children)
    {
        free(new_module);

        return NULL;
    }

    // create config
    new_module->config = d_test_options_new(D_TEST_DEFAULT_MODULE_FLAGS);

    if (!new_module->config)
    {
        d_ptr_vector_free(new_module->children);
        free(new_module);

        return NULL;
    }

    // allocate result structure
    new_module->result = malloc(sizeof(struct d_test_module_result));

    // ensure that memory allocation was successful
    if (!new_module->result)
    {
        d_test_options_free(new_module->config);
        d_ptr_vector_free(new_module->children);
        free(new_module);

        return NULL;
    }

    d_internal_test_module_init_result(new_module->result);
    new_module->status = D_TEST_MODULE_STATUS_PENDING;

    // add children if provided
    if (_children && _child_count > 0)
    {
        if (!d_internal_test_module_add_children(new_module, _children, _child_count))
        {
            d_test_module_free(new_module);

            return NULL;
        }
    }

    return new_module;
}

struct d_test_module*
d_test_module_new_args
(
    struct d_test_arg*   _args,
    size_t               _arg_count,
    struct d_test_type** _children,
    size_t               _child_count
)
{
    struct d_test_module* new_module;
    struct d_test_options* options;

    // create base module with children
    new_module = d_test_module_new(_children, _child_count);

    if (!new_module)
    {
        return NULL;
    }

    // process args if provided
    if ( (_args) && 
         (_arg_count > 0) )
    {
        options = d_test_module_validate_args(_args, _arg_count);

        if (options)
        {
            // replace default options with args-based options
            if (new_module->config)
            {
                d_test_options_free(new_module->config);
            }

            new_module->config = options;
        }
    }

    return new_module;
}


/******************************************************************************
 * CHILD MANAGEMENT FUNCTIONS
 *****************************************************************************/

bool
d_test_module_add_child
(
    struct d_test_module*          _module,
    const struct d_test_tree_node* _child
)
{
    if ( (!_module) || (!_module->children) || (!_child) )
    {
        return false;
    }

    return d_ptr_vector_push_back(_module->children, (void*)_child);
}


size_t
d_test_module_child_count
(
    const struct d_test_module* _module
)
{
    if ( (!_module) || (!_module->children) )
    {
        return 0;
    }

    return d_ptr_vector_size(_module->children);
}


struct d_test_type*
d_test_module_get_child_at
(
    const struct d_test_module* _module,
    size_t                      _index
)
{
    if ( (!_module) || (!_module->children) )
    {
        return NULL;
    }

    return (struct d_test_type*)d_ptr_vector_at(_module->children, 
                                                 (d_index)_index);
}


/******************************************************************************
 * CONFIGURATION FUNCTIONS
 *****************************************************************************/

/*
d_test_module_get_effective_settings
  Resolves effective settings by merging parent config with module config.
  Parent settings take precedence (allowing run-time override).
*/
struct d_test_options*
d_test_module_get_effective_settings
(
    const struct d_test_module* _module,
    struct d_test_options*       _parent_settings,
    struct d_min_enum_map*      _node_config
)
{
    (void)_node_config;  // unused for now

    if (!_module)
    {
        return _parent_settings;
    }

    // if parent settings provided, use those (they take precedence)
    if (_parent_settings)
    {
        return _parent_settings;
    }

    // fall back to module's own config
    return _module->config;
}


/*
d_test_module_get_name
  Gets the module name from config.
*/
const char*
d_test_module_get_name
(
    const struct d_test_module* _module
)
{
    if ( (!_module)         || 
         (!_module->config) || 
         (!_module->config->settings) )
    {
        return NULL;
    }

    return (const char*)d_min_enum_map_get(_module->config->settings,
                                           D_TEST_METADATA_NAME);
}


/******************************************************************************
 * STAGE HOOK FUNCTIONS
 *****************************************************************************/

bool
d_test_module_set_stage_hook
(
    struct d_test_module* _module,
    enum DTestStage       _stage,
    fn_stage              _hook
)
{
    return ( (!_module)         ||
             (!_module->config) ||
             (!_module->config->stage_hooks) )
        ? false
        : d_min_enum_map_put(_module->config->stage_hooks, 
                             _stage, 
                             _hook);
}

fn_stage
d_test_module_get_stage_hook
(
    const struct d_test_module* _module,
    enum DTestStage             _stage
)
{
    return ( (!_module)         ||
             (!_module->config) ||
             (!_module->config->stage_hooks) )
        ? NULL
        : d_min_enum_map_get(_module->config->stage_hooks, 
                             _stage);
}


/******************************************************************************
 * EXECUTION FUNCTIONS
 *****************************************************************************/

/*
d_test_module_run
  Runs all children in the module.

Parameter(s):
  _module:          the module to run.
  _parent_settings: parent configuration (takes precedence).
Return:
  true if all children passed.
*/
bool
d_test_module_run
(
    struct d_test_module* _module,
    struct d_test_options* _parent_settings
)
{
    size_t                i;
    size_t                child_count;
    struct d_test_type*   child;
    struct d_test_options* effective;
    fn_stage              setup_hook;
    fn_stage              teardown_hook;
    bool                  all_passed;
    bool                  child_passed;

    if ( (!_module) || (!_module->result) )
    {
        return false;
    }

    // get effective settings
    effective = d_test_module_get_effective_settings(_module,
                                                     _parent_settings,
                                                     NULL);

    // reset results
    d_test_module_reset_result(_module);
    _module->status = D_TEST_MODULE_STATUS_RUNNING;

    // run setup hook if present
    setup_hook = d_test_module_get_stage_hook(_module, D_TEST_STAGE_SETUP);

    if (setup_hook)
    {
        // setup hook - stubbed for now
    }

    // run all children
    all_passed  = true;
    child_count = d_test_module_child_count(_module);

    _module->result->blocks_total = child_count;

    for (i = 0; i < child_count; i++)
    {
        child = d_test_module_get_child_at(_module, i);

        if ( (!child) || 
             (!child->D_KEYWORD_TEST_BLOCK) )
        {
            continue;
        }

        child_passed = d_test_block_run(child->D_KEYWORD_TEST_BLOCK, effective);

        if (child_passed)
        {
            _module->result->blocks_passed++;
        }
        else
        {
            all_passed = false;
        }
    }

    // run teardown hook if present
    teardown_hook = d_test_module_get_stage_hook(_module, D_TEST_STAGE_TEAR_DOWN);

    if (teardown_hook)
    {
        // teardown hook - stubbed for now
    }

    // update status
    _module->status = all_passed
                          ? D_TEST_MODULE_STATUS_PASSED
                          : D_TEST_MODULE_STATUS_FAILED;

    _module->result->status = _module->status;

    return all_passed;
}


/*
d_test_module_run_child
  Runs a specific child by index.
*/
bool
d_test_module_run_child
(
    struct d_test_module* _module,
    size_t                _child_index,
    struct d_test_options* _parent_settings
)
{
    struct d_test_type*   child;
    struct d_test_options* effective;

    if (!_module)
    {
        return false;
    }

    child = d_test_module_get_child_at(_module, _child_index);

    if ( (!child) || (!child->D_KEYWORD_TEST_BLOCK) )
    {
        return false;
    }

    effective = d_test_module_get_effective_settings(_module,
                                                     _parent_settings,
                                                     NULL);

    return d_test_block_run(child->D_KEYWORD_TEST_BLOCK, effective);
}


/******************************************************************************
 * RESULT FUNCTIONS
 *****************************************************************************/

struct d_test_module_result*
d_test_module_get_result
(
    const struct d_test_module* _module
)
{
    return (_module) 
        ? _module->result
        : NULL;
}


void
d_test_module_reset_result
(
    struct d_test_module* _module
)
{
    if (!_module)
    {
        return;
    }

    if (_module->result)
    {
        d_internal_test_module_init_result(_module->result);
    }

    _module->status = D_TEST_MODULE_STATUS_PENDING;

    return;
}

double
d_test_module_get_pass_rate
(
    const struct d_test_module* _module
)
{
    if ( (!_module)         ||
         (!_module->result) ||
         (_module->result->tests_total == 0) )
    {
        return 0.0;
    }

    return ( (double)_module->result->tests_passed /
             ( (double)_module->result->tests_total * 100.0) );
}


/******************************************************************************
 * OUTPUT FUNCTIONS
 *****************************************************************************/

void
d_test_module_print
(
    const struct d_test_module* _module,
    const char*                 _prefix,
    size_t                      _prefix_length
)
{
    const char* name;
    size_t      child_count;

    (void)_prefix_length;

    if (!_module)
    {
        return;
    }

    name        = d_test_module_get_name(_module);
    child_count = d_test_module_child_count(_module);

    printf("%s%s %s [%zu blocks]\n",
           _prefix 
               ? _prefix 
               : "",
           D_TEST_SYMBOL_MODULE,
           name 
               ? name
               : "Unnamed Module",
           child_count);

    return;
}


void
d_test_module_print_result
(
    const struct d_test_module* _module
)
{
    if (!_module)
    {
        return;
    }

    printf("Module Results:\n");
    printf("%sStatus:     %s\n",
        D_INDENT,
        d_test_module_status_to_string(_module->status));
    printf("%sBlocks:     %zu/%zu passed\n",
           D_INDENT,
           _module->result->blocks_passed,
           _module->result->blocks_total);
    printf("%sTests:      %zu/%zu passed, %zu failed, %zu skipped\n",
           D_INDENT,
           _module->result->tests_passed,
           _module->result->tests_total,
           _module->result->tests_failed,
           _module->result->tests_skipped);
    printf("%sAssertions: %zu/%zu passed\n",
           D_INDENT,
           _module->result->assertions_passed,
           _module->result->assertions_total);
    printf("%sDuration:   %.2f ms\n", D_INDENT, _module->result->duration_ms);
    printf("%sPass Rate:  %.1f%%\n", D_INDENT, d_test_module_get_pass_rate(_module));

    return;
}

const char*
d_test_module_status_to_string
(
    enum DTestModuleStatus _status
)
{
    switch (_status)
    {
        case D_TEST_MODULE_STATUS_UNKNOWN:
            return "UNKNOWN";
        case D_TEST_MODULE_STATUS_PENDING:
            return "PENDING";
        case D_TEST_MODULE_STATUS_RUNNING:
            return "RUNNING";
        case D_TEST_MODULE_STATUS_PASSED:
            return "PASSED";
        case D_TEST_MODULE_STATUS_FAILED:
            return "FAILED";
        case D_TEST_MODULE_STATUS_SKIPPED:
            return "SKIPPED";
        case D_TEST_MODULE_STATUS_ERROR:
            return "ERROR";
        default:
            return "INVALID";
    }
}


void
d_test_module_free
(
    struct d_test_module* _test_module
)
{
    if (!_test_module)
    {
        return;
    }

    if (_test_module->children)
    {
        d_ptr_vector_free(_test_module->children);
    }

    if (_test_module->config)
    {
        d_test_options_free(_test_module->config);
    }

    if (_test_module->result)
    {
        free(_test_module->result);
    }

    free(_test_module);

    return;
}