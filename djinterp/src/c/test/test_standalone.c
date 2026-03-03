#include "../../../inc/c/test/test_standalone.h"


/******************************************************************************
 * INTERNAL: METADATA BLOCK ALLOCATION
 *****************************************************************************/

/*
d_test_internal_alloc_metadata
  Allocates a single block containing a d_test_metadata struct followed
by an array of d_test_metadata_arg entries. The entries pointer inside
the metadata struct is set to point at the inline array.

Parameter(s):
  _count: number of metadata entries
Return:
  Pointer to the d_test_metadata, or NULL on failure. Free the returned
  pointer to free the entire block.
*/
static struct d_test_metadata*
d_test_internal_alloc_metadata
(
    size_t _count
)
{
    size_t                  block_size;
    struct d_test_metadata* meta;

    block_size = sizeof(struct d_test_metadata) +
                 (_count * sizeof(struct d_test_metadata_arg));

    meta = (struct d_test_metadata*)malloc(block_size);

    if (!meta)
    {
        return NULL;
    }

    meta->count   = _count;
    meta->entries = (const struct d_test_metadata_arg*)
                        (meta + 1);

    return meta;
}

/*
d_test_internal_metadata_set_entry
  Sets a metadata entry at the given index in a metadata block allocated
by d_test_internal_alloc_metadata. Entries must be filled in ascending
key order.
*/
static void
d_test_internal_metadata_set_entry
(
    struct d_test_metadata* _meta,
    size_t                  _index,
    uint16_t                _key,
    const char*             _value
)
{
    struct d_test_metadata_arg* mutable_entries;

    if ( (!_meta) ||
         (_index >= _meta->count) )
    {
        return;
    }

    // cast away const for initial population only
    mutable_entries = (struct d_test_metadata_arg*)
                          _meta->entries;

    mutable_entries[_index].key   = _key;
    mutable_entries[_index].value = _value;

    return;
}


/******************************************************************************
 * STANDALONE TEST OBJECT FUNCTIONS
 *****************************************************************************/

/*
d_test_standalone_object_new_leaf
  Creates a new leaf test object (assertion). Name and message are
stored as metadata on the object's arg list. The context pointer holds
the heap-allocated metadata block for cleanup.

Parameter(s):
  _name:    name of the assertion
  _message: result message
  _result:  whether the assertion passed (true) or failed (false)
Return:
  Pointer to newly created test object, or NULL on allocation failure.
*/
struct d_test_object*
d_test_standalone_object_new_leaf
(
    const char* _name,
    const char* _message,
    bool        _result
)
{
    struct d_test_object*      obj;
    struct d_test_arg_list*    args;
    struct d_test_metadata*    meta;

    // allocate metadata block with 2 entries: NAME, MESSAGE
    meta = d_test_internal_alloc_metadata(2);

    if (!meta)
    {
        return NULL;
    }

    // entries must be in ascending key order
    d_test_internal_metadata_set_entry(
        meta, 0, D_TEST_META_NAME, _name);
    d_test_internal_metadata_set_entry(
        meta, 1, D_TEST_META_MESSAGE, _message);

    // create arg list
    args = d_test_arg_list_new(2);

    if (!args)
    {
        free(meta);

        return NULL;
    }

    d_test_arg_list_set(args, D_TEST_ARG_METADATA,
                        (void*)meta);

    // create the test object
    obj = d_test_object_new(D_TEST_OBJECT_LEAF,
                            _result,
                            (void*)meta);

    if (!obj)
    {
        d_test_arg_list_free(args);
        free(meta);

        return NULL;
    }

    obj->args = args;

    return obj;
}

/*
d_test_standalone_object_new_interior
  Creates a new interior test object (test group/block). Name is stored
as metadata. Children array and child count are stored as args.

Parameter(s):
  _name:        name of the test group
  _child_count: number of children this group will have
Return:
  Pointer to newly created test object with allocated child array,
or NULL on allocation failure.
*/
struct d_test_object*
d_test_standalone_object_new_interior
(
    const char* _name,
    size_t      _child_count
)
{
    struct d_test_object*      obj;
    struct d_test_arg_list*    args;
    struct d_test_metadata*    meta;
    struct d_test_object**     children;

    // allocate metadata block with 1 entry: NAME
    meta = d_test_internal_alloc_metadata(1);

    if (!meta)
    {
        return NULL;
    }

    d_test_internal_metadata_set_entry(
        meta, 0, D_TEST_META_NAME, _name);

    // allocate children array
    children = NULL;

    if (_child_count > 0)
    {
        children = (struct d_test_object**)calloc(
                       _child_count,
                       sizeof(struct d_test_object*));

        if (!children)
        {
            free(meta);

            return NULL;
        }
    }

    // create arg list
    args = d_test_arg_list_new(4);

    if (!args)
    {
        if (children) { free(children); }
        free(meta);

        return NULL;
    }

    d_test_arg_list_set(args, D_TEST_ARG_METADATA,
                        (void*)meta);
    d_test_arg_list_set(args, D_TEST_ARG_CHILDREN,
                        (void*)children);
    d_test_arg_list_set(args, D_TEST_ARG_CHILD_COUNT,
                        D_TEST_ARG_FROM_SIZE(
                            _child_count));

    // create the test object
    obj = d_test_object_new(D_TEST_OBJECT_INTERIOR,
                            false,
                            (void*)meta);

    if (!obj)
    {
        d_test_arg_list_free(args);
        if (children) { free(children); }
        free(meta);

        return NULL;
    }

    obj->args = args;

    return obj;
}

/*
d_test_standalone_object_free
  Recursively frees a standalone test object, its children, its
metadata block (via context), and its arg list.

Parameter(s):
  _obj: test object to free; may be NULL.
Return:
  none.
*/
void
d_test_standalone_object_free
(
    struct d_test_object* _obj
)
{
    struct d_test_object** children;
    size_t                 child_count;
    size_t                 i;

    if (!_obj)
    {
        return;
    }

    // recurse into children if present
    if (_obj->args)
    {
        children = (struct d_test_object**)
                       d_test_arg_list_get(
                           _obj->args,
                           D_TEST_ARG_CHILDREN);

        child_count = d_test_arg_list_get_size(
                          _obj->args,
                          D_TEST_ARG_CHILD_COUNT,
                          0);

        if (children)
        {
            for (i = 0; i < child_count; i++)
            {
                d_test_standalone_object_free(
                    children[i]);
            }

            free(children);
        }
    }

    // free metadata block (stored in context)
    if (_obj->context)
    {
        free(_obj->context);
        _obj->context = NULL;
    }

    // free arg list and object
    if (_obj->args)
    {
        d_test_arg_list_free(_obj->args);
        _obj->args = NULL;
    }

    free(_obj);

    return;
}

/*
d_test_standalone_object_add_child
  Adds a child test object to a parent at the specified index.

Parameter(s):
  _parent: parent test object (must be interior type)
  _child:  child test object to add
  _index:  index position to add child at
Return:
  none.
*/
void
d_test_standalone_object_add_child
(
    struct d_test_object* _parent,
    struct d_test_object* _child,
    size_t                _index
)
{
    struct d_test_object** children;
    size_t                 child_count;

    if ( (!_parent)   ||
         (!_child)    ||
         (!_parent->args) )
    {
        return;
    }

    if (_parent->is_leaf == true)
    {
        return;
    }

    children = (struct d_test_object**)
                   d_test_arg_list_get(
                       _parent->args,
                       D_TEST_ARG_CHILDREN);

    child_count = d_test_arg_list_get_size(
                      _parent->args,
                      D_TEST_ARG_CHILD_COUNT,
                      0);

    if ( (!children)          ||
         (_index >= child_count) )
    {
        return;
    }

    children[_index] = _child;

    return;
}


/******************************************************************************
 * STANDALONE ASSERTION FUNCTION
 *****************************************************************************/

/*
d_assert_standalone
  Performs an assertion and updates the counter. This is a data-only
operation; no output is produced.

Parameter(s):
  _condition: the condition to test
  _test_name: name of the test/assertion
  _message:   message to associate on failure
  _counter:   assertion counter to update (can be NULL)
Return:
  The condition value (true if passed, false if failed).
*/
bool
d_assert_standalone
(
    bool                   _condition,
    const char*            _test_name,
    const char*            _message,
    struct d_test_counter* _counter
)
{
    // suppress unused parameter warnings
    (void)_test_name;
    (void)_message;

    if (!_counter)
    {
        return _condition;
    }

    d_test_counter_increment(_counter,
                             D_TEST_COUNT_TOTAL);

    if (_condition)
    {
        d_test_counter_increment(_counter,
                                 D_TEST_COUNT_PASSED);
    }
    else
    {
        d_test_counter_increment(_counter,
                                 D_TEST_COUNT_FAILED);
    }

    return _condition;
}


/******************************************************************************
 * FAILURE TRACKING FUNCTIONS
 *****************************************************************************/

void
d_test_standalone_failure_list_init
(
    struct d_test_standalone_failure_list* _list
)
{
    if (!_list)
    {
        return;
    }

    _list->count    = 0;
    _list->capacity = 64;
    _list->entries  = (struct d_test_standalone_failure_entry*)
        calloc(_list->capacity,
               sizeof(struct d_test_standalone_failure_entry));

    if (!_list->entries)
    {
        _list->capacity = 0;
    }

    return;
}

/*
d_test_standalone_failure_list_add
  Records a failure entry. Grows the list if needed. Respects the
D_TEST_ARG_MAX_FAILURES limit from the runner's arg list.

Parameter(s):
  _list:        the failure list to add to
  _runner_args: runner's arg list for limit lookup (can be NULL)
  _module:      name of the module where the failure occurred
  _name:        name of the assertion or test that failed
  _message:     failure message/description
Return:
  none.
*/
void
d_test_standalone_failure_list_add
(
    struct d_test_standalone_failure_list* _list,
    const struct d_test_arg_list*          _runner_args,
    const char*                            _module,
    const char*                            _name,
    const char*                            _message
)
{
    struct d_test_standalone_failure_entry* new_entries;
    size_t                                 new_capacity;
    intptr_t                               max_failures;

    if (!_list)
    {
        return;
    }

    // check limit from runner args
    max_failures = D_TEST_ARG_NO_LIMIT;

    if (_runner_args)
    {
        max_failures = d_test_arg_list_get_intptr(
                           _runner_args,
                           D_TEST_ARG_MAX_FAILURES,
                           D_TEST_ARG_NO_LIMIT);
    }

    if ( (max_failures != D_TEST_ARG_NO_LIMIT) &&
         ((intptr_t)_list->count >= max_failures) )
    {
        return;
    }

    // grow if needed
    if (_list->count >= _list->capacity)
    {
        new_capacity = (_list->capacity > 0)
                           ? _list->capacity * 2
                           : 64;

        if ( (max_failures != D_TEST_ARG_NO_LIMIT) &&
             (new_capacity > (size_t)max_failures) )
        {
            new_capacity = (size_t)max_failures;
        }

        new_entries =
            (struct d_test_standalone_failure_entry*)
                realloc(
                    _list->entries,
                    new_capacity *
                        sizeof(struct
                            d_test_standalone_failure_entry));

        if (!new_entries)
        {
            return;
        }

        _list->entries  = new_entries;
        _list->capacity = new_capacity;
    }

    _list->entries[_list->count].module_name = _module;
    _list->entries[_list->count].test_name   = _name;
    _list->entries[_list->count].message     = _message;
    _list->count++;

    return;
}

void
d_test_standalone_failure_list_free
(
    struct d_test_standalone_failure_list* _list
)
{
    if (!_list)
    {
        return;
    }

    if (_list->entries)
    {
        free(_list->entries);
        _list->entries = NULL;
    }

    _list->count    = 0;
    _list->capacity = 0;

    return;
}


/******************************************************************************
 * MODULE ENTRY FUNCTIONS
 *****************************************************************************/

/*
d_test_standalone_module_entry_args_new
  Creates and populates an arg list for a module entry, including
metadata (name, description) and optional notes.

Parameter(s):
  _name:        module name
  _description: module description
  _note_count:  number of implementation note sections
  _notes:       array of note sections (can be NULL if count is 0)
Return:
  Newly allocated arg list, or NULL on failure. Caller must free via
  d_test_arg_list_free (metadata block is freed separately).
*/
struct d_test_arg_list*
d_test_standalone_module_entry_args_new
(
    const char*                       _name,
    const char*                       _description,
    size_t                            _note_count,
    const struct d_test_note_section* _notes
)
{
    struct d_test_arg_list*  args;
    struct d_test_metadata*  meta;
    size_t                   meta_count;
    size_t                   meta_idx;

    // count non-null metadata entries
    meta_count = 0;
    if (_name)        { meta_count++; }
    if (_description) { meta_count++; }

    meta = d_test_internal_alloc_metadata(meta_count);

    if (!meta)
    {
        return NULL;
    }

    // fill entries in ascending key order
    meta_idx = 0;

    if (_name)
    {
        d_test_internal_metadata_set_entry(
            meta, meta_idx++,
            D_TEST_META_NAME, _name);
    }

    if (_description)
    {
        d_test_internal_metadata_set_entry(
            meta, meta_idx++,
            D_TEST_META_DESCRIPTION, _description);
    }

    // create arg list
    args = d_test_arg_list_new(4);

    if (!args)
    {
        free(meta);

        return NULL;
    }

    d_test_arg_list_set(args, D_TEST_ARG_METADATA,
                        (void*)meta);

    if ( (_note_count > 0) &&
         (_notes) )
    {
        d_test_arg_list_set(args, D_TEST_ARG_NOTES,
                            (void*)_notes);
        d_test_arg_list_set(args, D_TEST_ARG_NOTE_COUNT,
                            D_TEST_ARG_FROM_SIZE(
                                _note_count));
    }

    return args;
}

const char*
d_test_standalone_module_entry_get_name
(
    const struct d_test_standalone_module_entry* _entry
)
{
    if ( (!_entry) ||
         (!_entry->args) )
    {
        return "(unknown)";
    }

    return d_test_arg_list_get_meta(
               _entry->args,
               D_TEST_META_NAME,
               "(unknown)");
}

const char*
d_test_standalone_module_entry_get_description
(
    const struct d_test_standalone_module_entry* _entry
)
{
    if ( (!_entry) ||
         (!_entry->args) )
    {
        return NULL;
    }

    return d_test_arg_list_get_meta(
               _entry->args,
               D_TEST_META_DESCRIPTION,
               NULL);
}


/******************************************************************************
 * INTERNAL HELPERS
 *****************************************************************************/

intptr_t
d_test_internal_runner_get_max_modules
(
    const struct d_test_standalone_runner* _runner
)
{
    if ( (!_runner) ||
         (!_runner->args) )
    {
        return D_TEST_ARG_NO_LIMIT;
    }

    return d_test_arg_list_get_intptr(
               _runner->args,
               D_TEST_ARG_MAX_MODULES,
               D_TEST_ARG_NO_LIMIT);
}

intptr_t
d_test_internal_runner_get_max_failures
(
    const struct d_test_standalone_runner* _runner
)
{
    if ( (!_runner) ||
         (!_runner->args) )
    {
        return D_TEST_ARG_NO_LIMIT;
    }

    return d_test_arg_list_get_intptr(
               _runner->args,
               D_TEST_ARG_MAX_FAILURES,
               D_TEST_ARG_NO_LIMIT);
}


/******************************************************************************
 * INTERNAL: MODULE ARRAY MANAGEMENT
 *****************************************************************************/

/*
d_test_internal_runner_grow_modules
  Grows the module array stored in the runner's args if needed.
Reads D_TEST_ARG_MODULES, MODULE_COUNT, MODULE_CAPACITY from the
arg list. Writes updated pointer and capacity back.
*/
static int
d_test_internal_runner_grow_modules
(
    struct d_test_standalone_runner* _runner
)
{
    struct d_test_standalone_module_entry* modules;
    struct d_test_standalone_module_entry* new_modules;
    size_t                                count;
    size_t                                capacity;
    size_t                                new_capacity;
    intptr_t                              max_modules;

    if ( (!_runner) ||
         (!_runner->args) )
    {
        return -1;
    }

    modules = (struct d_test_standalone_module_entry*)
                  d_test_arg_list_get(
                      _runner->args,
                      D_TEST_ARG_MODULES);

    count = d_test_arg_list_get_size(
                _runner->args,
                D_TEST_ARG_MODULE_COUNT,
                0);

    capacity = d_test_arg_list_get_size(
                   _runner->args,
                   D_TEST_ARG_MODULE_CAPACITY,
                   0);

    max_modules =
        d_test_internal_runner_get_max_modules(
            _runner);

    // check hard limit
    if ( (max_modules != D_TEST_ARG_NO_LIMIT) &&
         ((intptr_t)count >= max_modules) )
    {
        return -1;
    }

    // no growth needed
    if (count < capacity)
    {
        return 0;
    }

    new_capacity = (capacity > 0)
                       ? capacity * 2
                       : 16;

    if ( (max_modules != D_TEST_ARG_NO_LIMIT) &&
         (new_capacity > (size_t)max_modules) )
    {
        new_capacity = (size_t)max_modules;
    }

    new_modules =
        (struct d_test_standalone_module_entry*)
            realloc(
                modules,
                new_capacity *
                    sizeof(struct
                        d_test_standalone_module_entry));

    if (!new_modules)
    {
        return -1;
    }

    d_test_arg_list_set(_runner->args,
        D_TEST_ARG_MODULES,
        (void*)new_modules);
    d_test_arg_list_set(_runner->args,
        D_TEST_ARG_MODULE_CAPACITY,
        D_TEST_ARG_FROM_SIZE(new_capacity));

    return 0;
}


/******************************************************************************
 * INTERNAL: FREE MODULE ENTRY ARGS
 *****************************************************************************/

/*
d_test_internal_free_module_entry_args
  Frees the arg list and its owned metadata block for a module entry.
*/
static void
d_test_internal_free_module_entry_args
(
    struct d_test_standalone_module_entry* _entry
)
{
    struct d_test_metadata* meta;

    if ( (!_entry) ||
         (!_entry->args) )
    {
        return;
    }

    // free metadata block
    meta = (struct d_test_metadata*)
               d_test_arg_list_get(
                   _entry->args,
                   D_TEST_ARG_METADATA);

    if (meta)
    {
        free(meta);
    }

    d_test_arg_list_free(_entry->args);
    _entry->args = NULL;

    return;
}


/******************************************************************************
 * UNIFIED TEST RUNNER FUNCTIONS
 *****************************************************************************/

/*
d_test_standalone_runner_init
  Initializes a test runner. All state is stored in the arg list:
suite metadata, default options, heap-allocated counters and failure
list, and initial module array state (NULL/0/0).

Parameter(s):
  _runner:            the runner to initialize
  _suite_name:        name of the test suite
  _suite_description: description of the test suite
Return:
  none.
*/
void
d_test_standalone_runner_init
(
    struct d_test_standalone_runner* _runner,
    const char*                      _suite_name,
    const char*                      _suite_description
)
{
    struct d_test_metadata*              meta;
    struct d_test_counter*               a_totals;
    struct d_test_counter*               t_totals;
    struct d_test_standalone_failure_list* fails;
    double*                              elapsed;
    size_t                               meta_count;
    size_t                               meta_idx;

    if (!_runner)
    {
        return;
    }

    // zero the struct (just the args pointer)
    _runner->args = NULL;

    // create arg list with generous capacity
    _runner->args = d_test_arg_list_new(32);

    if (!_runner->args)
    {
        return;
    }

    // --- suite metadata ---
    meta_count = 0;
    if (_suite_name)        { meta_count++; }
    if (_suite_description) { meta_count++; }

    if (meta_count > 0)
    {
        meta = d_test_internal_alloc_metadata(
                   meta_count);

        if (meta)
        {
            meta_idx = 0;

            if (_suite_name)
            {
                d_test_internal_metadata_set_entry(
                    meta, meta_idx++,
                    D_TEST_META_SUITE_NAME,
                    _suite_name);
            }

            if (_suite_description)
            {
                d_test_internal_metadata_set_entry(
                    meta, meta_idx++,
                    D_TEST_META_DESCRIPTION,
                    _suite_description);
            }

            d_test_arg_list_set(
                _runner->args,
                D_TEST_ARG_METADATA,
                (void*)meta);
        }
    }

    // --- default options ---
    d_test_arg_list_set(_runner->args,
        D_TEST_ARG_OPT_NUMBER_ASSERTIONS,
        D_TEST_ARG_FALSE);
    d_test_arg_list_set(_runner->args,
        D_TEST_ARG_OPT_NUMBER_TESTS,
        D_TEST_ARG_FALSE);
    d_test_arg_list_set(_runner->args,
        D_TEST_ARG_OPT_GLOBAL_NUMBERING,
        D_TEST_ARG_FALSE);
    d_test_arg_list_set(_runner->args,
        D_TEST_ARG_OPT_SHOW_INFO,
        D_TEST_ARG_TRUE);
    d_test_arg_list_set(_runner->args,
        D_TEST_ARG_OPT_SHOW_MODULE_FOOTER,
        D_TEST_ARG_TRUE);
    d_test_arg_list_set(_runner->args,
        D_TEST_ARG_OPT_LIST_FAILURES,
        D_TEST_ARG_FALSE);
    d_test_arg_list_set(_runner->args,
        D_TEST_ARG_OPT_WAIT_FOR_INPUT,
        D_TEST_ARG_TRUE);
    d_test_arg_list_set(_runner->args,
        D_TEST_ARG_OPT_SHOW_NOTES,
        D_TEST_ARG_TRUE);

    // --- module registration (empty) ---
    d_test_arg_list_set(_runner->args,
        D_TEST_ARG_MODULES,
        NULL);
    d_test_arg_list_set(_runner->args,
        D_TEST_ARG_MODULE_COUNT,
        D_TEST_ARG_FROM_SIZE(0));
    d_test_arg_list_set(_runner->args,
        D_TEST_ARG_MODULE_CAPACITY,
        D_TEST_ARG_FROM_SIZE(0));

    // --- heap-allocated failure list ---
    fails = (struct d_test_standalone_failure_list*)
                malloc(sizeof(
                    struct d_test_standalone_failure_list));

    if (fails)
    {
        d_test_standalone_failure_list_init(fails);
        d_test_arg_list_set(_runner->args,
            D_TEST_ARG_FAILURES,
            (void*)fails);
    }

    // --- heap-allocated suite counters ---
    a_totals = (struct d_test_counter*)
                   malloc(sizeof(struct d_test_counter));

    if (a_totals)
    {
        d_test_counter_init(a_totals,
                            D_TEST_COUNTER_ASSERT_STD);
        d_test_arg_list_set(_runner->args,
            D_TEST_ARG_ASSERTION_COUNTER,
            (void*)a_totals);
    }

    t_totals = (struct d_test_counter*)
                   malloc(sizeof(struct d_test_counter));

    if (t_totals)
    {
        d_test_counter_init(t_totals,
                            D_TEST_COUNTER_TEST_STD);
        d_test_arg_list_set(_runner->args,
            D_TEST_ARG_TEST_COUNTER,
            (void*)t_totals);
    }

    // --- heap-allocated elapsed time ---
    elapsed = (double*)malloc(sizeof(double));

    if (elapsed)
    {
        *elapsed = 0.0;
        d_test_arg_list_set(_runner->args,
            D_TEST_ARG_ELAPSED_TIME,
            (void*)elapsed);
    }

    // --- nesting defaults ---
    d_test_arg_list_set(_runner->args,
        D_TEST_ARG_DEPTH,
        D_TEST_ARG_FROM_SIZE(0));

    return;
}

/*
d_test_standalone_runner_add_module
  Registers a tree-based test module with the runner.
*/
void
d_test_standalone_runner_add_module
(
    struct d_test_standalone_runner*  _runner,
    const char*                       _name,
    const char*                       _description,
    fn_test_module                    _run_fn,
    size_t                            _note_count,
    const struct d_test_note_section* _notes
)
{
    struct d_test_standalone_module_entry* modules;
    size_t                                count;

    if ( (!_runner) ||
         (!_runner->args) ||
         (!_run_fn) )
    {
        return;
    }

    if (d_test_internal_runner_grow_modules(
            _runner) != 0)
    {
        return;
    }

    // re-read after potential realloc
    modules = (struct d_test_standalone_module_entry*)
                  d_test_arg_list_get(
                      _runner->args,
                      D_TEST_ARG_MODULES);

    count = d_test_arg_list_get_size(
                _runner->args,
                D_TEST_ARG_MODULE_COUNT,
                0);

    modules[count].run_fn      = _run_fn;
    modules[count].run_counter = NULL;
    modules[count].args        =
        d_test_standalone_module_entry_args_new(
            _name, _description,
            _note_count, _notes);

    d_test_arg_list_set(_runner->args,
        D_TEST_ARG_MODULE_COUNT,
        D_TEST_ARG_FROM_SIZE(count + 1));

    return;
}

/*
d_test_standalone_runner_add_module_counter
  Registers a counter-based test module with the runner.
*/
void
d_test_standalone_runner_add_module_counter
(
    struct d_test_standalone_runner*  _runner,
    const char*                       _name,
    const char*                       _description,
    fn_test_module_counter            _run_fn,
    size_t                            _note_count,
    const struct d_test_note_section* _notes
)
{
    struct d_test_standalone_module_entry* modules;
    size_t                                count;

    if ( (!_runner) ||
         (!_runner->args) ||
         (!_run_fn) )
    {
        return;
    }

    if (d_test_internal_runner_grow_modules(
            _runner) != 0)
    {
        return;
    }

    // re-read after potential realloc
    modules = (struct d_test_standalone_module_entry*)
                  d_test_arg_list_get(
                      _runner->args,
                      D_TEST_ARG_MODULES);

    count = d_test_arg_list_get_size(
                _runner->args,
                D_TEST_ARG_MODULE_COUNT,
                0);

    modules[count].run_fn      = NULL;
    modules[count].run_counter = _run_fn;
    modules[count].args        =
        d_test_standalone_module_entry_args_new(
            _name, _description,
            _note_count, _notes);

    d_test_arg_list_set(_runner->args,
        D_TEST_ARG_MODULE_COUNT,
        D_TEST_ARG_FROM_SIZE(count + 1));

    return;
}

/*
d_test_standalone_runner_add_sub_runner
  Registers a nested sub-runner as a module. When executed, the
sub-runner is executed recursively and its results are aggregated
into this runner's totals.

The sub-runner's D_TEST_ARG_PARENT_RUNNER is set to this runner
and its D_TEST_ARG_DEPTH is set to this runner's depth + 1.

Parameter(s):
  _runner:      the parent runner
  _name:        display name for this nested module
  _description: description for this nested module
  _sub_runner:  the sub-runner to nest (not owned; caller manages
                lifetime)
Return:
  none.
*/
void
d_test_standalone_runner_add_sub_runner
(
    struct d_test_standalone_runner* _runner,
    const char*                      _name,
    const char*                      _description,
    struct d_test_standalone_runner*  _sub_runner
)
{
    struct d_test_standalone_module_entry* modules;
    size_t                                count;
    size_t                                parent_depth;

    if ( (!_runner) ||
         (!_runner->args) ||
         (!_sub_runner) )
    {
        return;
    }

    if (d_test_internal_runner_grow_modules(
            _runner) != 0)
    {
        return;
    }

    // re-read after potential realloc
    modules = (struct d_test_standalone_module_entry*)
                  d_test_arg_list_get(
                      _runner->args,
                      D_TEST_ARG_MODULES);

    count = d_test_arg_list_get_size(
                _runner->args,
                D_TEST_ARG_MODULE_COUNT,
                0);

    modules[count].run_fn      = NULL;
    modules[count].run_counter = NULL;
    modules[count].args        =
        d_test_standalone_module_entry_args_new(
            _name, _description, 0, NULL);

    if (modules[count].args)
    {
        d_test_arg_list_set(
            modules[count].args,
            D_TEST_ARG_SUB_RUNNER,
            (void*)_sub_runner);
    }

    d_test_arg_list_set(_runner->args,
        D_TEST_ARG_MODULE_COUNT,
        D_TEST_ARG_FROM_SIZE(count + 1));

    // set nesting info on the sub-runner
    parent_depth = d_test_arg_list_get_size(
                       _runner->args,
                       D_TEST_ARG_DEPTH,
                       0);

    if (_sub_runner->args)
    {
        d_test_arg_list_set(
            _sub_runner->args,
            D_TEST_ARG_PARENT_RUNNER,
            (void*)_runner);
        d_test_arg_list_set(
            _sub_runner->args,
            D_TEST_ARG_DEPTH,
            D_TEST_ARG_FROM_SIZE(
                parent_depth + 1));
    }

    return;
}


/******************************************************************************
 * RUNNER OPTION CONVENIENCE
 *****************************************************************************/

void
d_test_standalone_runner_set_opt
(
    struct d_test_standalone_runner* _runner,
    int16_t                          _key,
    bool                             _value
)
{
    if ( (!_runner) ||
         (!_runner->args) )
    {
        return;
    }

    d_test_arg_list_set(_runner->args, _key,
                        _value ? D_TEST_ARG_TRUE
                               : D_TEST_ARG_FALSE);

    return;
}


/******************************************************************************
 * RUNNER EXECUTE
 *****************************************************************************/

/*
d_test_standalone_runner_execute
  Executes all registered modules and aggregates results. For modules
with D_TEST_ARG_SUB_RUNNER, the sub-runner is executed recursively.
Results are stored as args on the runner and per-module result
arg_lists.

Per-module counter and time arrays are heap-allocated and stored
as D_TEST_ARG_RESULT_ASSERTION_CTRS, D_TEST_ARG_RESULT_TEST_CTRS,
and D_TEST_ARG_RESULT_ELAPSED_TIMES for cleanup.

Parameter(s):
  _runner: the runner to execute
Return:
  0 if all tests passed, 1 if any tests failed.
*/
int
d_test_standalone_runner_execute
(
    struct d_test_standalone_runner* _runner
)
{
    size_t                  i;
    clock_t                 suite_start;
    clock_t                 suite_end;
    clock_t                 module_start;
    clock_t                 module_end;
    bool                    overall_result;
    size_t                  modules_passed;
    size_t                  n;

    struct d_test_standalone_module_entry* modules;
    struct d_test_counter*                a_totals;
    struct d_test_counter*                t_totals;
    double*                               total_time;

    // per-module result backing arrays (owned)
    struct d_test_arg_list**  result_lists;
    struct d_test_counter*    result_a_ctrs;
    struct d_test_counter*    result_t_ctrs;
    double*                   result_times;

    if ( (!_runner) ||
         (!_runner->args) )
    {
        return 1;
    }

    // read module registration from args
    modules = (struct d_test_standalone_module_entry*)
                  d_test_arg_list_get(
                      _runner->args,
                      D_TEST_ARG_MODULES);

    n = d_test_arg_list_get_size(
            _runner->args,
            D_TEST_ARG_MODULE_COUNT,
            0);

    // read owned suite counters and time
    a_totals = (struct d_test_counter*)
                   d_test_arg_list_get(
                       _runner->args,
                       D_TEST_ARG_ASSERTION_COUNTER);

    t_totals = (struct d_test_counter*)
                   d_test_arg_list_get(
                       _runner->args,
                       D_TEST_ARG_TEST_COUNTER);

    total_time = (double*)
                     d_test_arg_list_get(
                         _runner->args,
                         D_TEST_ARG_ELAPSED_TIME);

    // reset counters
    if (a_totals) { d_test_counter_reset(a_totals); }
    if (t_totals) { d_test_counter_reset(t_totals); }
    if (total_time) { *total_time = 0.0; }

    // allocate per-module result storage
    result_lists  = NULL;
    result_a_ctrs = NULL;
    result_t_ctrs = NULL;
    result_times  = NULL;

    if (n > 0)
    {
        result_lists =
            (struct d_test_arg_list**)calloc(
                n, sizeof(struct d_test_arg_list*));
        result_a_ctrs =
            (struct d_test_counter*)calloc(
                n, sizeof(struct d_test_counter));
        result_t_ctrs =
            (struct d_test_counter*)calloc(
                n, sizeof(struct d_test_counter));
        result_times =
            (double*)calloc(n, sizeof(double));

        if ( (!result_lists)  ||
             (!result_a_ctrs) ||
             (!result_t_ctrs) ||
             (!result_times) )
        {
            if (result_lists)  { free(result_lists); }
            if (result_a_ctrs) { free(result_a_ctrs); }
            if (result_t_ctrs) { free(result_t_ctrs); }
            if (result_times)  { free(result_times); }

            return 1;
        }

        // store backing arrays in args for cleanup
        d_test_arg_list_set(_runner->args,
            D_TEST_ARG_RESULT_ASSERTION_CTRS,
            (void*)result_a_ctrs);
        d_test_arg_list_set(_runner->args,
            D_TEST_ARG_RESULT_TEST_CTRS,
            (void*)result_t_ctrs);
        d_test_arg_list_set(_runner->args,
            D_TEST_ARG_RESULT_ELAPSED_TIMES,
            (void*)result_times);
    }

    overall_result = true;
    modules_passed = 0;

    suite_start = clock();

    for (i = 0; i < n; i++)
    {
        struct d_test_counter* a_ctr;
        struct d_test_counter* t_ctr;
        bool                   module_result;
        const char*            mod_name;
        const char*            mod_desc;
        struct d_test_standalone_runner* sub;

        // initialize per-module counters
        a_ctr = &result_a_ctrs[i];
        t_ctr = &result_t_ctrs[i];

        d_test_counter_init(a_ctr,
                            D_TEST_COUNTER_ASSERT_STD);
        d_test_counter_init(t_ctr,
                            D_TEST_COUNTER_TEST_STD);

        module_result = false;
        mod_name =
            d_test_standalone_module_entry_get_name(
                &modules[i]);
        mod_desc =
            d_test_standalone_module_entry_get_description(
                &modules[i]);

        module_start = clock();

        // check for sub-runner (nesting)
        sub = NULL;

        if (modules[i].args)
        {
            sub = (struct d_test_standalone_runner*)
                      d_test_arg_list_get(
                          modules[i].args,
                          D_TEST_ARG_SUB_RUNNER);
        }

        if (sub)
        {
            // nested execution
            int sub_result;

            sub_result =
                d_test_standalone_runner_execute(sub);
            module_result = (sub_result == 0);

            // pull sub-runner's totals into this
            // module's counters
            {
                struct d_test_counter* sub_a;
                struct d_test_counter* sub_t;

                sub_a = (struct d_test_counter*)
                            d_test_arg_list_get(
                                sub->args,
                                D_TEST_ARG_ASSERTION_COUNTER);
                sub_t = (struct d_test_counter*)
                            d_test_arg_list_get(
                                sub->args,
                                D_TEST_ARG_TEST_COUNTER);

                if (sub_a) { d_test_counter_add(a_ctr, sub_a); }
                if (sub_t) { d_test_counter_add(t_ctr, sub_t); }
            }
        }
        else if (modules[i].run_fn)
        {
            // tree-based test function
            struct d_test_object* test_results;

            test_results =
                modules[i].run_fn();

            if (test_results)
            {
                // traversal and counting deferred
                // to test_printer
                d_test_standalone_object_free(
                    test_results);
            }
        }
        else if (modules[i].run_counter)
        {
            // counter-based test function
            module_result =
                modules[i].run_counter(
                    a_ctr, t_ctr);
        }

        module_end = clock();

        // store elapsed time
        result_times[i] =
            d_test_standalone_get_elapsed_time(
                module_start, module_end);

        // build per-module result arg list
        result_lists[i] =
            d_test_arg_list_new(8);

        if (result_lists[i])
        {
            struct d_test_metadata* rmeta;
            size_t                  rmeta_count;
            size_t                  rmeta_idx;

            // metadata for module name/description
            rmeta_count = 0;
            if (mod_name) { rmeta_count++; }
            if (mod_desc) { rmeta_count++; }

            rmeta = NULL;

            if (rmeta_count > 0)
            {
                rmeta = d_test_internal_alloc_metadata(
                            rmeta_count);

                if (rmeta)
                {
                    rmeta_idx = 0;

                    if (mod_name)
                    {
                        d_test_internal_metadata_set_entry(
                            rmeta, rmeta_idx++,
                            D_TEST_META_NAME,
                            mod_name);
                    }

                    if (mod_desc)
                    {
                        d_test_internal_metadata_set_entry(
                            rmeta, rmeta_idx++,
                            D_TEST_META_DESCRIPTION,
                            mod_desc);
                    }

                    d_test_arg_list_set(
                        result_lists[i],
                        D_TEST_ARG_METADATA,
                        (void*)rmeta);
                }
            }

            d_test_arg_list_set(
                result_lists[i],
                D_TEST_ARG_ASSERTION_COUNTER,
                (void*)a_ctr);
            d_test_arg_list_set(
                result_lists[i],
                D_TEST_ARG_TEST_COUNTER,
                (void*)t_ctr);
            d_test_arg_list_set(
                result_lists[i],
                D_TEST_ARG_PASSED,
                module_result ? D_TEST_ARG_TRUE
                              : D_TEST_ARG_FALSE);
            d_test_arg_list_set(
                result_lists[i],
                D_TEST_ARG_ELAPSED_TIME,
                (void*)&result_times[i]);
        }

        // aggregate into totals
        if (a_totals) { d_test_counter_add(a_totals, a_ctr); }
        if (t_totals) { d_test_counter_add(t_totals, t_ctr); }

        if (module_result)
        {
            modules_passed++;
        }

        overall_result = overall_result && module_result;
    }

    suite_end = clock();

    if (total_time)
    {
        *total_time =
            d_test_standalone_get_elapsed_time(
                suite_start, suite_end);
    }

    // wire results into runner args
    d_test_arg_list_set(_runner->args,
        D_TEST_ARG_PASSED,
        overall_result ? D_TEST_ARG_TRUE
                       : D_TEST_ARG_FALSE);
    d_test_arg_list_set(_runner->args,
        D_TEST_ARG_MODULES_TOTAL,
        D_TEST_ARG_FROM_SIZE(n));
    d_test_arg_list_set(_runner->args,
        D_TEST_ARG_MODULES_PASSED,
        D_TEST_ARG_FROM_SIZE(modules_passed));
    d_test_arg_list_set(_runner->args,
        D_TEST_ARG_MODULE_RESULTS,
        (void*)result_lists);
    d_test_arg_list_set(_runner->args,
        D_TEST_ARG_MODULE_RESULT_COUNT,
        D_TEST_ARG_FROM_SIZE(n));

    return overall_result ? 0 : 1;
}


/******************************************************************************
 * RUNNER CLEANUP
 *****************************************************************************/

/*
d_test_standalone_runner_cleanup
  Frees all resources owned by the runner's arg list: module entry
args and their metadata, result arg lists and their metadata, per-module
backing arrays (counters, times), the failure list, suite counters,
the suite elapsed time, the module array itself, and finally the
runner's own metadata and arg list.

Parameter(s):
  _runner: the runner to clean up
Return:
  none.
*/
void
d_test_standalone_runner_cleanup
(
    struct d_test_standalone_runner* _runner
)
{
    size_t                                i;
    struct d_test_metadata*               meta;
    struct d_test_standalone_module_entry* modules;
    size_t                                module_count;
    struct d_test_arg_list**              result_lists;
    size_t                                result_count;

    if ( (!_runner) ||
         (!_runner->args) )
    {
        return;
    }

    // --- free module entry args (and their metadata) ---
    modules = (struct d_test_standalone_module_entry*)
                  d_test_arg_list_get(
                      _runner->args,
                      D_TEST_ARG_MODULES);

    module_count = d_test_arg_list_get_size(
                       _runner->args,
                       D_TEST_ARG_MODULE_COUNT,
                       0);

    if (modules)
    {
        for (i = 0; i < module_count; i++)
        {
            d_test_internal_free_module_entry_args(
                &modules[i]);
        }

        free(modules);
    }

    // --- free per-module result arg lists and metadata ---
    result_lists = (struct d_test_arg_list**)
                       d_test_arg_list_get(
                           _runner->args,
                           D_TEST_ARG_MODULE_RESULTS);

    result_count = d_test_arg_list_get_size(
                       _runner->args,
                       D_TEST_ARG_MODULE_RESULT_COUNT,
                       0);

    if (result_lists)
    {
        for (i = 0; i < result_count; i++)
        {
            if (!result_lists[i])
            {
                continue;
            }

            // free result metadata block
            meta = (struct d_test_metadata*)
                       d_test_arg_list_get(
                           result_lists[i],
                           D_TEST_ARG_METADATA);

            if (meta)
            {
                free(meta);
            }

            d_test_arg_list_free(result_lists[i]);
        }

        free(result_lists);
    }

    // --- free per-module backing arrays ---
    {
        struct d_test_counter* ra;
        struct d_test_counter* rt;
        double*                re;
        size_t                 rc;

        ra = (struct d_test_counter*)
                 d_test_arg_list_get(
                     _runner->args,
                     D_TEST_ARG_RESULT_ASSERTION_CTRS);

        rt = (struct d_test_counter*)
                 d_test_arg_list_get(
                     _runner->args,
                     D_TEST_ARG_RESULT_TEST_CTRS);

        re = (double*)
                 d_test_arg_list_get(
                     _runner->args,
                     D_TEST_ARG_RESULT_ELAPSED_TIMES);

        rc = d_test_arg_list_get_size(
                 _runner->args,
                 D_TEST_ARG_MODULE_RESULT_COUNT,
                 0);

        // free each counter's amount array before
        // freeing the backing arrays themselves
        if (ra)
        {
            for (i = 0; i < rc; i++)
            {
                d_test_counter_free(&ra[i]);
            }

            free(ra);
        }

        if (rt)
        {
            for (i = 0; i < rc; i++)
            {
                d_test_counter_free(&rt[i]);
            }

            free(rt);
        }

        if (re) { free(re); }
    }

    // --- free failure list ---
    {
        struct d_test_standalone_failure_list* fails;

        fails = (struct d_test_standalone_failure_list*)
                    d_test_arg_list_get(
                        _runner->args,
                        D_TEST_ARG_FAILURES);

        if (fails)
        {
            d_test_standalone_failure_list_free(fails);
            free(fails);
        }
    }

    // --- free suite counters ---
    {
        struct d_test_counter* ac;
        struct d_test_counter* tc;

        ac = (struct d_test_counter*)
                 d_test_arg_list_get(
                     _runner->args,
                     D_TEST_ARG_ASSERTION_COUNTER);

        tc = (struct d_test_counter*)
                 d_test_arg_list_get(
                     _runner->args,
                     D_TEST_ARG_TEST_COUNTER);

        if (ac) { d_test_counter_free(ac); free(ac); }
        if (tc) { d_test_counter_free(tc); free(tc); }
    }

    // --- free elapsed time ---
    {
        double* et;

        et = (double*)
                 d_test_arg_list_get(
                     _runner->args,
                     D_TEST_ARG_ELAPSED_TIME);

        if (et) { free(et); }
    }

    // --- free runner's own metadata block and arg list ---
    meta = (struct d_test_metadata*)
               d_test_arg_list_get(
                   _runner->args,
                   D_TEST_ARG_METADATA);

    if (meta)
    {
        free(meta);
    }

    d_test_arg_list_free(_runner->args);
    _runner->args = NULL;

    return;
}


/******************************************************************************
 * RUNNER ARG ACCESSORS
 *****************************************************************************/

struct d_test_standalone_module_entry*
d_test_runner_get_modules
(
    const struct d_test_standalone_runner* _runner
)
{
    if ( (!_runner) ||
         (!_runner->args) )
    {
        return NULL;
    }

    return (struct d_test_standalone_module_entry*)
               d_test_arg_list_get(
                   _runner->args,
                   D_TEST_ARG_MODULES);
}

size_t
d_test_runner_get_module_count
(
    const struct d_test_standalone_runner* _runner
)
{
    if ( (!_runner) ||
         (!_runner->args) )
    {
        return 0;
    }

    return d_test_arg_list_get_size(
               _runner->args,
               D_TEST_ARG_MODULE_COUNT,
               0);
}

struct d_test_counter*
d_test_runner_get_assertion_counter
(
    const struct d_test_standalone_runner* _runner
)
{
    if ( (!_runner) ||
         (!_runner->args) )
    {
        return NULL;
    }

    return (struct d_test_counter*)
               d_test_arg_list_get(
                   _runner->args,
                   D_TEST_ARG_ASSERTION_COUNTER);
}

struct d_test_counter*
d_test_runner_get_test_counter
(
    const struct d_test_standalone_runner* _runner
)
{
    if ( (!_runner) ||
         (!_runner->args) )
    {
        return NULL;
    }

    return (struct d_test_counter*)
               d_test_arg_list_get(
                   _runner->args,
                   D_TEST_ARG_TEST_COUNTER);
}

struct d_test_standalone_failure_list*
d_test_runner_get_failure_list
(
    const struct d_test_standalone_runner* _runner
)
{
    if ( (!_runner) ||
         (!_runner->args) )
    {
        return NULL;
    }

    return (struct d_test_standalone_failure_list*)
               d_test_arg_list_get(
                   _runner->args,
                   D_TEST_ARG_FAILURES);
}

size_t
d_test_runner_get_depth
(
    const struct d_test_standalone_runner* _runner
)
{
    if ( (!_runner) ||
         (!_runner->args) )
    {
        return 0;
    }

    return d_test_arg_list_get_size(
               _runner->args,
               D_TEST_ARG_DEPTH,
               0);
}

struct d_test_standalone_runner*
d_test_runner_get_parent
(
    const struct d_test_standalone_runner* _runner
)
{
    if ( (!_runner) ||
         (!_runner->args) )
    {
        return NULL;
    }

    return (struct d_test_standalone_runner*)
               d_test_arg_list_get(
                   _runner->args,
                   D_TEST_ARG_PARENT_RUNNER);
}


/******************************************************************************
 * UTILITY FUNCTIONS
 *****************************************************************************/

double
d_test_standalone_get_elapsed_time
(
    clock_t _start,
    clock_t _end
)
{
    return (double)(_end - _start) / CLOCKS_PER_SEC;
}
