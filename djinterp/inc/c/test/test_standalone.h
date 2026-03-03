/******************************************************************************
* djinterp [test]                                             test_standalone.h
*
*   Standalone test framework data structures for tests and assertions.
*   Supports nested test blocks/groups with chainable module execution.
*   All IO and text formatting is handled by test_printer.
*
*   The runner is a thin wrapper around a d_test_arg_list. Configuration,
*   options, module registration, result counters, failure tracking, and
*   nesting state are all stored as arg entries rather than dedicated
*   struct fields. This design keeps the struct count low and makes
*   every datum optional and extensible.
*
*   Runners may be nested inside one another for hierarchical suites.
*   A parent runner registers a child via add_sub_runner; on execute
*   the child is executed recursively and its results are aggregated
*   into the parent. The child's D_TEST_ARG_PARENT_RUNNER and
*   D_TEST_ARG_DEPTH are set automatically.
*
*
* path:      /inc/c/test/test_standalone.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.24
******************************************************************************/

#ifndef DJINTERP_TEST_STANDALONE_
#define DJINTERP_TEST_STANDALONE_ 1

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "../../../inc/c/djinterp.h"
#include "../../../inc/c/dmemory.h"
#include "../../../inc/c/dtime.h"
#include "./test_common.h"

#if D_CFG_TEST_FILE_ENABLE
    #include "../../../inc/c/dfile.h"
#endif


/******************************************************************************
 * ASSERTION MACROS
 *****************************************************************************/

// D_ASSERT_TRUE
//   macro: creates a leaf test object asserting that a condition is true.
#define D_ASSERT_TRUE(_name, _condition, _message)                          \
    d_test_standalone_object_new_leaf(_name, _message, (_condition))

// D_ASSERT_FALSE
//   macro: creates a leaf test object asserting that a condition is false.
#define D_ASSERT_FALSE(_name, _condition, _message)                         \
    d_test_standalone_object_new_leaf(_name, _message, !(_condition))

// D_ASSERT_NULL
//   macro: creates a leaf test object asserting that a pointer is NULL.
#define D_ASSERT_NULL(_name, _ptr, _message)                                \
    d_test_standalone_object_new_leaf(_name, _message, (_ptr) == NULL)

// D_ASSERT_NOT_NULL
//   macro: creates a leaf test object asserting that a pointer is not NULL.
#define D_ASSERT_NOT_NULL(_name, _ptr, _message)                            \
    d_test_standalone_object_new_leaf(_name, _message, (_ptr) != NULL)

// D_ASSERT_EQUAL
//   macro: creates a leaf test object asserting two values are equal.
#define D_ASSERT_EQUAL(_name, _val1, _val2, _message)                       \
    d_test_standalone_object_new_leaf(_name, _message,                      \
        (_val1) == (_val2))

// D_ASSERT_STR_EQUAL
//   macro: creates a leaf test object asserting two strings are equal.
#define D_ASSERT_STR_EQUAL(_name, _str1, _str2, _message)                   \
    d_test_standalone_object_new_leaf(_name, _message,                      \
        ( (_str1) &&                                                        \
          (_str2) &&                                                        \
          (strcmp(_str1, _str2) == 0)) )


/******************************************************************************
 * FAILURE TRACKING
 *****************************************************************************/

// d_test_standalone_failure_entry
//   struct: records a single test failure for the end-of-run summary.
struct d_test_standalone_failure_entry
{
    const char* module_name;
    const char* test_name;
    const char* message;
};

// d_test_standalone_failure_list
//   struct: growable list of recorded failure entries. Referenced from
// the runner's arg list via D_TEST_ARG_FAILURES.
struct d_test_standalone_failure_list
{
    size_t                                  count;
    size_t                                  capacity;
    struct d_test_standalone_failure_entry* entries;
};


/******************************************************************************
 * TEST MODULE REGISTRATION STRUCTURE
 *****************************************************************************/

// fn_test_module
//   function pointer: function that runs a test module and returns test
// results as a d_test_object tree.
typedef struct d_test_object* (*fn_test_module)(void);

// fn_test_module_counter
//   function pointer: function that runs a test module and updates
// counters directly, returning pass/fail status.
typedef bool (*fn_test_module_counter)(struct d_test_counter* _assertions,
                                      struct d_test_counter* _tests);

// d_test_standalone_module_entry
//   struct: registration entry for a test module. Function pointers are
// struct fields because they cannot be portably stored as void*.
// All other data (name, description, notes, sub-runner reference) lives
// in the arg list.
//
//   arg layout:
//     D_TEST_ARG_METADATA     --> d_test_metadata* (contains name, desc)
//     D_TEST_ARG_NOTES        --> const d_test_note_section*
//     D_TEST_ARG_NOTE_COUNT   --> size_t via intptr_t
//     D_TEST_ARG_SUB_RUNNER   --> d_test_standalone_runner* (for nesting)
struct d_test_standalone_module_entry
{
    fn_test_module          run_fn;
    fn_test_module_counter  run_counter;
    struct d_test_arg_list* args;
};


/******************************************************************************
 * TEST RUNNER
 *****************************************************************************/

// d_test_standalone_runner
//   struct: unified test runner. A thin wrapper around a d_test_arg_list
// that holds ALL state: configuration, module registration, execution
// results, nesting info, and failure tracking.
//
// The runner has no dedicated fields beyond the arg list. All data is
// accessed through typed arg keys. This keeps the struct minimal and
// makes every piece of state optional.
//
//   option args (set before execute):
//     D_TEST_ARG_OPT_NUMBER_ASSERTIONS  --> bool
//     D_TEST_ARG_OPT_NUMBER_TESTS       --> bool
//     D_TEST_ARG_OPT_GLOBAL_NUMBERING   --> bool
//     D_TEST_ARG_OPT_SHOW_INFO          --> bool
//     D_TEST_ARG_OPT_SHOW_MODULE_FOOTER --> bool
//     D_TEST_ARG_OPT_LIST_FAILURES      --> bool
//     D_TEST_ARG_OPT_WAIT_FOR_INPUT     --> bool
//     D_TEST_ARG_OPT_SHOW_NOTES         --> bool
//     D_TEST_ARG_OUTPUT_FILE            --> const char*
//     D_TEST_ARG_MAX_MODULES            --> intptr_t
//     D_TEST_ARG_MAX_FAILURES           --> intptr_t
//     D_TEST_ARG_METADATA               --> d_test_metadata* (suite name, desc)
//
//   module registration args (managed by add_module/add_sub_runner):
//     D_TEST_ARG_MODULES                --> d_test_standalone_module_entry*
//     D_TEST_ARG_MODULE_COUNT           --> size_t
//     D_TEST_ARG_MODULE_CAPACITY        --> size_t
//
//   owned data args (heap-allocated, freed during cleanup):
//     D_TEST_ARG_FAILURES               --> d_test_standalone_failure_list*
//     D_TEST_ARG_ASSERTION_COUNTER      --> d_test_counter* (suite totals)
//     D_TEST_ARG_TEST_COUNTER           --> d_test_counter* (suite totals)
//     D_TEST_ARG_ELAPSED_TIME           --> double*
//
//   nesting args (set automatically when nested):
//     D_TEST_ARG_PARENT_RUNNER          --> d_test_standalone_runner*
//     D_TEST_ARG_DEPTH                  --> size_t (0 = top-level)
//     D_TEST_ARG_RUNNER_ID              --> size_t (unique id, optional)
//
//   result args (populated after execute):
//     D_TEST_ARG_PASSED                 --> bool
//     D_TEST_ARG_MODULES_TOTAL          --> size_t
//     D_TEST_ARG_MODULES_PASSED         --> size_t
//     D_TEST_ARG_MODULE_RESULTS         --> d_test_arg_list** (array)
//     D_TEST_ARG_MODULE_RESULT_COUNT    --> size_t
//     D_TEST_ARG_RESULT_ASSERTION_CTRS  --> d_test_counter* (array)
//     D_TEST_ARG_RESULT_TEST_CTRS       --> d_test_counter* (array)
//     D_TEST_ARG_RESULT_ELAPSED_TIMES   --> double* (array)
//
//   per-module result arg lists (accessed via MODULE_RESULTS array):
//     D_TEST_ARG_METADATA               --> d_test_metadata* (name, desc)
//     D_TEST_ARG_ASSERTION_COUNTER      --> d_test_counter*
//     D_TEST_ARG_TEST_COUNTER           --> d_test_counter*
//     D_TEST_ARG_PASSED                 --> bool
//     D_TEST_ARG_ELAPSED_TIME           double*
struct d_test_standalone_runner
{
    struct d_test_arg_list* args;
};


/******************************************************************************
 * STANDALONE TEST OBJECT FUNCTIONS
 *****************************************************************************/

// I.    standalone test object creation and destruction
//
//   leaf objects store name and message as metadata on the d_test_object's
//   args list. interior objects additionally store children and child_count
//   as D_TEST_ARG_CHILDREN / D_TEST_ARG_CHILD_COUNT.
//
//   the d_test_object.context field holds the heap-allocated metadata
//   block for cleanup.
struct d_test_object* d_test_standalone_object_new_leaf(const char* _name, const char* _message, bool _result);
struct d_test_object* d_test_standalone_object_new_interior(const char* _name, size_t _child_count);
void                  d_test_standalone_object_free(struct d_test_object* _obj);
void                  d_test_standalone_object_add_child(struct d_test_object* _parent, struct d_test_object* _child, size_t _index);


/******************************************************************************
 * STANDALONE ASSERTION FUNCTION
 *****************************************************************************/

// II.   standalone assertion (data only, no IO)
bool d_assert_standalone(bool _condition, const char* _test_name, const char* _message, struct d_test_counter* _counter);


/******************************************************************************
 * FAILURE TRACKING FUNCTIONS
 *****************************************************************************/

// III.  failure list management (data only)
void d_test_standalone_failure_list_init(struct d_test_standalone_failure_list* _list);
void d_test_standalone_failure_list_add(struct d_test_standalone_failure_list* _list, const struct d_test_arg_list* _runner_args, const char* _module, const char* _name, const char* _message);
void d_test_standalone_failure_list_free(struct d_test_standalone_failure_list* _list);


/******************************************************************************
 * MODULE ENTRY FUNCTIONS
 *****************************************************************************/

// IV.   module entry arg helpers
struct d_test_arg_list* d_test_standalone_module_entry_args_new(const char* _name, const char* _description, size_t _note_count, const struct d_test_note_section* _notes);
const char*             d_test_standalone_module_entry_get_name(const struct d_test_standalone_module_entry* _entry);
const char*             d_test_standalone_module_entry_get_description(const struct d_test_standalone_module_entry* _entry);


/******************************************************************************
 * UNIFIED TEST RUNNER FUNCTIONS
 *****************************************************************************/

// V.    unified test runner (data management)
void d_test_standalone_runner_init(struct d_test_standalone_runner* _runner, const char* _suite_name, const char* _suite_description);
void d_test_standalone_runner_add_module(struct d_test_standalone_runner* _runner, const char* _name, const char* _description, fn_test_module _run_fn, size_t _note_count, const struct d_test_note_section* _notes);
void d_test_standalone_runner_add_module_counter(struct d_test_standalone_runner* _runner, const char* _name, const char* _description, fn_test_module_counter _run_fn, size_t _note_count, const struct d_test_note_section* _notes);
void d_test_standalone_runner_add_sub_runner(struct d_test_standalone_runner* _runner, const char* _name, const char* _description, struct d_test_standalone_runner* _sub_runner);
int  d_test_standalone_runner_execute(struct d_test_standalone_runner* _runner);
void d_test_standalone_runner_cleanup(struct d_test_standalone_runner* _runner);


/******************************************************************************
 * RUNNER OPTION CONVENIENCE FUNCTIONS
 *****************************************************************************/

// VI.   option setters (wrap d_test_arg_list_set on runner args)
void d_test_standalone_runner_set_opt(struct d_test_standalone_runner* _runner, int16_t _key, bool _value);


/******************************************************************************
 * RUNNER ARG ACCESSORS
 *****************************************************************************/

// VII.  typed accessors for runner arg data
//
//   These provide safe access to runner arg data with defaults.
//   Module array and counters are returned as pointers; callers must
//   not free them (the runner owns the storage).
struct d_test_standalone_module_entry* d_test_runner_get_modules(const struct d_test_standalone_runner* _runner);
size_t                                 d_test_runner_get_module_count(const struct d_test_standalone_runner* _runner);
struct d_test_counter*                 d_test_runner_get_assertion_counter(const struct d_test_standalone_runner* _runner);
struct d_test_counter*                 d_test_runner_get_test_counter(const struct d_test_standalone_runner* _runner);
struct d_test_standalone_failure_list* d_test_runner_get_failure_list(const struct d_test_standalone_runner* _runner);
size_t                                 d_test_runner_get_depth(const struct d_test_standalone_runner* _runner);
struct d_test_standalone_runner*       d_test_runner_get_parent(const struct d_test_standalone_runner* _runner);


/******************************************************************************
 * UTILITY FUNCTIONS
 *****************************************************************************/

// VIII. utility functions
double d_test_standalone_get_elapsed_time(clock_t _start, clock_t _end);


/******************************************************************************
 * INTERNAL HELPERS (exposed for test_printer)
 *****************************************************************************/

intptr_t d_test_internal_runner_get_max_modules(const struct d_test_standalone_runner* _runner);
intptr_t d_test_internal_runner_get_max_failures(const struct d_test_standalone_runner* _runner);


#endif  // DJINTERP_TEST_STANDALONE_
