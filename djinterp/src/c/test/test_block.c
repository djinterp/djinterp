#include "../../../inc/c/test/test_block.h"


/******************************************************************************
 * INTERNAL HELPERS
 *****************************************************************************/

/*
d_internal_test_block_add_children
  Adds child test types to the block.

Parameter(s):
  _block:       the block to add children to.
  _children:    array of child test type pointers.
  _child_count: number of children.
Return:
  true if all children were added successfully.
*/
static bool
d_internal_test_block_add_children
(
    struct d_test_block* _block,
    struct d_test_type** _children,
    size_t               _child_count
)
{
    size_t i;

    if ( (!_block)           ||
         (!_block->children) ||
         ( (!_children) && 
           (_child_count > 0) ) )
    {
        return false;
    }

    for (i = 0; i < _child_count; i++)
    {
        if (!_children[i])
        {
            continue;
        }

        // blocks can contain: sub-blocks, tests, assertions, test_fns
        if (!d_ptr_vector_push_back(_block->children, _children[i]))
        {
            return false;
        }
    }

    return true;
}

/******************************************************************************
 * VALIDATE_ARGS FUNCTION
 *****************************************************************************/

/*
d_test_block_validate_args
  Validates and converts argument array to a test options structure.
The resulting options are not stored on the block itself; they are attached
externally at the d_test_type wrapper level.

Parameter(s):
  _args:      array of key-value argument pairs.
  _arg_count: number of arguments.
Return:
  Pointer to the test options, or NULL if no valid args.
*/
struct d_test_options*
d_test_block_validate_args
(
    struct d_test_arg* _args,
    size_t             _arg_count
)
{
    struct d_test_options*       options;
    struct d_test_registry_row* row;
    size_t                      i;

    if ( (!_args) || 
         (_arg_count == 0) )
    {
        return NULL;
    }

    options = d_test_options_new(D_TEST_MODE_NORMAL);

    if (!options)
    {
        return NULL;
    }

    if (!options->settings)
    {
        options->settings = d_min_enum_map_new();

        if (!options->settings)
        {
            d_test_options_free(options);

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

        // validate: key must exist AND have BLOCKS flag set
        if ( (!row) || 
             ((row->command_flags & D_TEST_REGISTRY_FLAG_BLOCKS) == 0) )
        {
            continue;
        }

        // store with the registry flag as the key
        d_min_enum_map_put(options->settings, 
                           (int)row->flag, 
                           (void*)_args[i].value);
    }

    return options;
}

/******************************************************************************
 * CONSTRUCTOR FUNCTIONS
 *****************************************************************************/

/*
d_test_block_new
  Creates a new test block with the specified children.

Parameter(s):
  _children:    array of child test type pointers.
  _child_count: number of children.
Return:
  Pointer to the newly created block, or NULL on failure.
*/
struct d_test_block*
d_test_block_new
(
    struct d_test_type** _children,
    size_t               _child_count
)
{
    struct d_test_block* block;

    // reject NULL children with nonzero count
    if ( (!_children) &&
         (_child_count > 0) )
    {
        return NULL;
    }

    block = (struct d_test_block*)calloc(1, sizeof(struct d_test_block));

    if (!block)
    {
        return NULL;
    }

    // create children vector
    block->children = d_ptr_vector_new_default();

    if (!block->children)
    {
        free(block);

        return NULL;
    }

    // add children if provided
    if (_children && _child_count > 0)
    {
        if (!d_internal_test_block_add_children(block,
                                                _children,
                                                _child_count))
        {
            d_test_block_free(block);

            return NULL;
        }
    }

    return block;
}

/*
d_test_block_new_args
  Creates a new test block with configuration arguments and children.
Configuration arguments are validated but not stored on the block; the caller
is responsible for attaching the resulting options at the d_test_type level
via d_test_block_validate_args.

Parameter(s):
  _args:        array of key-value configuration arguments (unused by block).
  _arg_count:   number of arguments (unused by block).
  _children:    array of child test type pointers.
  _child_count: number of children.
Return:
  Pointer to the newly created block, or NULL on failure.
*/
struct d_test_block*
d_test_block_new_args
(
    struct d_test_arg*   _args,
    size_t               _arg_count,
    struct d_test_type** _children,
    size_t               _child_count
)
{
    (void)_args;
    (void)_arg_count;

    return d_test_block_new(_children, _child_count);
}

/******************************************************************************
 * DESTRUCTOR FUNCTION
 *****************************************************************************/

/*
d_test_block_free
  Frees a test block and all associated resources.

Parameter(s):
  _block: the block to free.
Return:
  none.
*/
void
d_test_block_free
(
    struct d_test_block* _block
)
{
    size_t              i;
    size_t              count;
    struct d_test_type* child;

    if (!_block)
    {
        return;
    }

    // free children
    if (_block->children)
    {
        count = d_ptr_vector_size(_block->children);

        for (i = 0; i < count; i++)
        {
            child = (struct d_test_type*)d_ptr_vector_at(_block->children,
                                                         (d_index)i);

            if (child)
            {
                d_test_type_free(child);
            }
        }

        d_ptr_vector_free(_block->children);
    }

    free(_block);

    return;
}

/******************************************************************************
 * CHILD MANAGEMENT FUNCTIONS
 *****************************************************************************/

/*
d_test_block_add_child
  Adds a child to the block.

Parameter(s):
  _block: the block to add to.
  _child: the child to add.
Return:
  true if the child was added successfully.
*/
bool
d_test_block_add_child
(
    struct d_test_block* _block,
    struct d_test_type*  _child
)
{
    if ( (!_block)           ||
         (!_block->children) ||
         (!_child) )
    {
        return false;
    }

    return d_ptr_vector_push_back(_block->children, _child);
}

/*
d_test_block_add_test
  Adds a test to the block.

Parameter(s):
  _block: the block to add to.
  _test:  the test to add.
Return:
  true if the test was added successfully.
*/
bool
d_test_block_add_test
(
    struct d_test_block* _block,
    struct d_test*       _test
)
{
    struct d_test_type* test_type;

    if ( (!_block)           ||
         (!_block->children) ||
         (!_test) )
    {
        return false;
    }

    test_type = d_test_type_new(D_TEST_TYPE_TEST, _test);

    if (!test_type)
    {
        return false;
    }

    return d_test_block_add_child(_block, test_type);
}

/*
d_test_block_add_block
  Adds a child block to this block.

Parameter(s):
  _parent: the parent block.
  _child:  the child block to add.
Return:
  true if the child block was added successfully.
*/
bool
d_test_block_add_block
(
    struct d_test_block* _parent,
    struct d_test_block* _child
)
{
    struct d_test_type* test_type;

    if ( (!_parent)           ||
         (!_parent->children) ||
         (!_child) )
    {
        return false;
    }

    test_type = d_test_type_new(D_TEST_TYPE_TEST_BLOCK, _child);

    if (!test_type)
    {
        return false;
    }

    return d_test_block_add_child(_parent, test_type);
}

/*
d_test_block_child_count
  Returns the number of children in the block.

Parameter(s):
  _block: the block to query.
Return:
  Number of children in the block.
*/
size_t
d_test_block_child_count
(
    const struct d_test_block* _block
)
{
    if ( (!_block) ||
         (!_block->children) )
    {
        return 0;
    }

    return d_ptr_vector_size(_block->children);
}

/*
d_test_block_get_child_at
  Gets the child at the specified index.

Parameter(s):
  _block: the block to query.
  _index: the index of the child to get.
Return:
  Pointer to the child's test type, or NULL if invalid.
*/
struct d_test_type*
d_test_block_get_child_at
(
    const struct d_test_block* _block,
    size_t                     _index
)
{
    if ( (!_block)           ||
         (!_block->children) )
    {
        return NULL;
    }

    return (struct d_test_type*)d_ptr_vector_at(_block->children,
                                                (d_index)_index);
}

/******************************************************************************
 * EXECUTION FUNCTIONS
 *****************************************************************************/

/*
d_test_block_run
  Runs all children in the block. Lifecycle hooks are retrieved from the
run configuration's stage_hooks map, not from the block itself.

Parameter(s):
  _block:      the block to run.
  _run_config: runtime configuration (from parent, may be NULL).
Return:
  true if all children passed, false otherwise.
*/
bool
d_test_block_run
(
    struct d_test_block*          _block,
    const struct d_test_options*  _run_config
)
{
    size_t              i;
    size_t              count;
    struct d_test_type* child;
    fn_stage            setup_hook;
    fn_stage            teardown_hook;
    bool                all_passed;
    bool                child_result;

    if (!_block)
    {
        return false;
    }

    all_passed = true;

    // retrieve lifecycle hooks from run config if available
    setup_hook    = NULL;
    teardown_hook = NULL;

    if (_run_config && _run_config->stage_hooks)
    {
        setup_hook = (fn_stage)d_min_enum_map_get(
                         _run_config->stage_hooks,
                         (int)D_TEST_STAGE_SETUP);

        teardown_hook = (fn_stage)d_min_enum_map_get(
                            _run_config->stage_hooks,
                            (int)D_TEST_STAGE_TEAR_DOWN);
    }

    // run setup hook if present
    if (setup_hook)
    {
        if (!setup_hook(NULL))
        {
            return false;
        }
    }

    // run all children
    count = d_test_block_child_count(_block);

    for (i = 0; i < count; i++)
    {
        child = d_test_block_get_child_at(_block, i);

        if (!child)
        {
            continue;
        }

        child_result = false;

        switch (child->type)
        {
            case D_TEST_TYPE_TEST:
                if (child->D_KEYWORD_TEST_TEST)
                {
                    child_result = d_test_run(child->D_KEYWORD_TEST_TEST,
                                              _run_config);
                }
                break;

            case D_TEST_TYPE_TEST_BLOCK:
                if (child->D_KEYWORD_TEST_BLOCK)
                {
                    child_result = d_test_block_run(
                                       child->D_KEYWORD_TEST_BLOCK,
                                       _run_config);
                }
                break;

            case D_TEST_TYPE_ASSERT:
                if (child->D_KEYWORD_TEST_ASSERTION)
                {
                    child_result =
                        child->D_KEYWORD_TEST_ASSERTION->result;
                }
                break;

            case D_TEST_TYPE_TEST_FN:
                if ( (child->D_KEYWORD_TEST_TEST_FN) &&
                     (child->D_KEYWORD_TEST_TEST_FN->test_fn) )
                {
                    child_result =
                        child->D_KEYWORD_TEST_TEST_FN->test_fn();
                }
                break;

            default:
                break;
        }

        if (!child_result)
        {
            all_passed = false;
        }
    }

    // run teardown hook if present
    if (teardown_hook)
    {
        teardown_hook(NULL);
    }

    return all_passed;
}

/******************************************************************************
 * UTILITY FUNCTIONS
 *****************************************************************************/

/*
d_test_block_print
  Prints block information for debugging.

Parameter(s):
  _block:         block to print.
  _prefix:        indentation prefix.
  _prefix_length: length of prefix.
Return:
  none.
*/
void
d_test_block_print
(
    const struct d_test_block* _block,
    const char*                _prefix,
    size_t                     _prefix_length
)
{
    const char* actual_prefix;

    (void)_prefix_length;

    actual_prefix = _prefix ? _prefix : "";

    if (!_block)
    {
        printf("%sTest Block: NULL\n", actual_prefix);

        return;
    }

    printf("%s%s Test Block [%zu children]\n",
           actual_prefix,
           D_TEST_SYMBOL_INTERIOR,
           d_test_block_child_count(_block));

    return;
}

/*
d_test_block_count_tests
  Counts the number of individual tests in the block (non-recursive).

Parameter(s):
  _block: block to count tests in.
Return:
  Number of individual tests.
*/
size_t
d_test_block_count_tests
(
    const struct d_test_block* _block
)
{
    size_t              i;
    size_t              count;
    size_t              test_count;
    struct d_test_type* child;

    if ( (!_block) ||
         (!_block->children) )
    {
        return 0;
    }

    test_count = 0;
    count      = d_ptr_vector_size(_block->children);

    for (i = 0; i < count; i++)
    {
        child = (struct d_test_type*)d_ptr_vector_at(_block->children,
                                                      (d_index)i);

        if (child && child->type == D_TEST_TYPE_TEST)
        {
            test_count++;
        }
    }

    return test_count;
}

/*
d_test_block_count_blocks
  Counts the number of child blocks in the block (non-recursive).

Parameter(s):
  _block: block to count child blocks in.
Return:
  Number of child blocks.
*/
size_t
d_test_block_count_blocks
(
    const struct d_test_block* _block
)
{
    size_t              i;
    size_t              count;
    size_t              block_count;
    struct d_test_type* child;

    if ( (!_block) ||
         (!_block->children) )
    {
        return 0;
    }

    block_count = 0;
    count       = d_ptr_vector_size(_block->children);

    for (i = 0; i < count; i++)
    {
        child = (struct d_test_type*)d_ptr_vector_at(_block->children,
                                                      (d_index)i);

        if (child && child->type == D_TEST_TYPE_TEST_BLOCK)
        {
            block_count++;
        }
    }

    return block_count;
}