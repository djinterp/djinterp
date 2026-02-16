/******************************************************************************
* djinterp [test]                                             test_standalone.h
*
*   Standalone test framework with simple tree structure for tests and 
* assertions. Supports nested test blocks/groups, template-based output,
* and unified test runner with chainable module execution.
*
* 
* path:      \inc\test\test_standalone.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.24
******************************************************************************/

#ifndef DJINTERP_TEST_STANDALONE_
#define DJINTERP_TEST_STANDALONE_ 1

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "..\djinterp.h"
#include "..\dmemory.h"
#include "..\dfile.h"
#include "..\dstring.h"
#include "..\dtime.h"
#include ".\test_common.h"


/******************************************************************************
 * ASSERTION MACROS
 *****************************************************************************/

// D_ASSERT_TRUE
//   macro: creates a leaf test object asserting that a condition is true.
#define D_ASSERT_TRUE(_name, _condition, _message)          \
    d_test_object_new_leaf(_name, _message, (_condition))

// D_ASSERT_FALSE
//   macro: creates a leaf test object asserting that a condition is false.
#define D_ASSERT_FALSE(_name, _condition, _message)         \
    d_test_object_new_leaf(_name, _message, !(_condition))

// D_ASSERT_NULL
//   macro: creates a leaf test object asserting that a pointer is NULL.
#define D_ASSERT_NULL(_name, _ptr, _message)                \
    d_test_object_new_leaf(_name, _message, (_ptr) == NULL)

// D_ASSERT_NOT_NULL
//   macro: creates a leaf test object asserting that a pointer is not NULL.
#define D_ASSERT_NOT_NULL(_name, _ptr, _message)            \
    d_test_object_new_leaf(_name, _message, (_ptr) != NULL)

// D_ASSERT_EQUAL
//   macro: creates a leaf test object asserting two values are equal.
#define D_ASSERT_EQUAL(_name, _val1, _val2, _message)       \
    d_test_object_new_leaf(_name, _message, (_val1) == (_val2))

// D_ASSERT_STR_EQUAL
//   macro: creates a leaf test object asserting two strings are equal.
#define D_ASSERT_STR_EQUAL(_name, _str1, _str2, _message)   \
    d_test_object_new_leaf(_name, _message,                 \
        ( (_str1) &&                                        \
          (_str2) &&                                        \
          (strcmp(_str1, _str2) == 0)) )


/******************************************************************************
 * TEST OBJECT CONSTANTS
 *****************************************************************************/

// D_TEST_OBJECT_LEAF
//   constant: indicates a leaf node (assertion) in the test tree.
#define D_TEST_OBJECT_LEAF     true

// D_TEST_OBJECT_INTERIOR
//   constant: indicates an interior node (group/block) in the test tree.
#define D_TEST_OBJECT_INTERIOR false

// D_TEST_SA_LINE_WIDTH
//   macro: standard line width for output formatting (80 characters).
#define D_TEST_SA_LINE_WIDTH 80

// D_TEST_SA_SEPARATOR_DOUBLE
//   macro: double-line separator for major sections.
#define D_TEST_SA_SEPARATOR_DOUBLE \
    "================================================================================"

// D_TEST_SA_SEPARATOR_SINGLE
//   macro: single-line separator for minor sections.
#define D_TEST_SA_SEPARATOR_SINGLE \
    "--------------------------------------------------------------------------------"

// D_TEST_SA_MAX_MODULES
//   constant: maximum number of modules that can be registered in a runner.
#define D_TEST_SA_MAX_MODULES 64


/******************************************************************************
 * TEST COUNTER
 *****************************************************************************/

// d_test_counter
//   struct: tracks passed and failed tests and assertions.
struct d_test_counter
{
    size_t assertions_total;
    size_t assertions_passed;
    size_t tests_total;
    size_t tests_passed;
};


/******************************************************************************
 * TEST OBJECT
 *****************************************************************************/

// d_test_object
//   struct: tree node representing either an assertion (leaf) or a test 
// group/block (interior). supports nested test organization.
struct d_test_object
{
    bool                    is_leaf;    // leaf or interior
    const char*             name;       // test/assertion name
    const char*             message;    // result message
    bool                    result;     // pass/fail (for leaves)
    
    struct d_test_arg_list* args;       // optional arguments
    size_t                  count;      // number of children
    struct d_test_object**  elements;   // array of child pointers
};


/******************************************************************************
 * MODULE RESULTS STRUCTURES
 *****************************************************************************/

// d_test_sa_module_results
//   struct: tracks results for a single test module.
struct d_test_sa_module_results
{
    const char*            name;           // module name
    const char*            description;    // module description
    struct d_test_counter  counter;        // assertion/test counters
    bool                   passed;         // overall pass/fail
    double                 elapsed_time;   // execution time in seconds
};

// d_test_sa_suite_results
//   struct: tracks results for an entire test suite (multiple modules).
struct d_test_sa_suite_results
{
    size_t                           modules_total;
    size_t                           modules_passed;
    struct d_test_counter            totals;        // aggregated counters
    struct d_test_sa_module_results* modules;       // array of module results
    double                           total_time;    // total execution time
};


/******************************************************************************
 * NOTE STRUCTURES FOR IMPLEMENTATION NOTES
 *****************************************************************************/

// d_test_sa_note_item
//   struct: represents a single note item for implementation notes.
struct d_test_sa_note_item
{
    const char* prefix;     // e.g., "[INFO]", "[WARN]"
    const char* message;    // the note text
};

// d_test_sa_note_section
//   struct: represents a section of notes (e.g., "CURRENT STATUS").
struct d_test_sa_note_section
{
    const char*                       title;    // section title
    size_t                            count;    // number of items
    const struct d_test_sa_note_item* items;    // array of note items
};


/******************************************************************************
 * TEST MODULE REGISTRATION STRUCTURE
 *****************************************************************************/

// fn_test_module
//   function pointer: function that runs a test module and returns test 
// results as a d_test_object tree.
typedef struct d_test_object* (*fn_test_module)(void);

// fn_test_module_counter
//   function pointer: function that runs a test module and updates a counter
// directly, returning pass/fail status.
typedef bool (*fn_test_module_counter)(struct d_test_counter* _counter);

// d_test_sa_module_entry
//   struct: registration entry for a test module.
struct d_test_sa_module_entry
{
    const char*                       name;          // module name
    const char*                       description;   // module description
    fn_test_module                    run_fn;        // test function (tree)
    fn_test_module_counter            run_counter;   // test function (counter)
    size_t                            note_count;    // implementation notes
    const struct d_test_sa_note_section* notes;      // note sections array
};


/******************************************************************************
 * TEST RUNNER CONTEXT
 *****************************************************************************/

// d_test_sa_runner
//   struct: unified test runner context that manages module registration,
// execution, and result aggregation.
struct d_test_sa_runner
{
    const char*                   suite_name;        // overall suite name
    const char*                   suite_description; // suite description
    size_t                        module_count;      // registered modules
    struct d_test_sa_module_entry modules[D_TEST_SA_MAX_MODULES];
    struct d_test_sa_suite_results results;          // aggregated results
    bool                          wait_for_input;    // pause before exit
    bool                          show_notes;        // display impl. notes
};


/******************************************************************************
 * FUNCTION POINTER TYPES
 *****************************************************************************/

// fn_print_object
//   function pointer: prints a test object tree to console.
typedef void (*fn_print_object)(const struct d_test_object* _obj,
                                size_t                      _indent_level,
                                struct d_test_counter*      _counter);

// fn_print_object_file
//   function pointer: prints a test object tree to a file.
typedef void (*fn_print_object_file)(FILE*                       _file,
                                     const struct d_test_object* _obj,
                                     size_t                      _indent_level,
                                     struct d_test_counter*      _counter);


/******************************************************************************
 * TEST OBJECT FUNCTIONS
 *****************************************************************************/

// I.    test object creation and destruction
struct d_test_object* d_test_object_new_leaf(const char* _name,
                                             const char* _message,
                                             bool        _result);
struct d_test_object* d_test_object_new_interior(const char* _name,
                                                 size_t      _child_count);
void                  d_test_object_free(struct d_test_object* _obj);
void                  d_test_object_add_child(struct d_test_object* _parent,
                                              struct d_test_object* _child,
                                              size_t                _index);


/******************************************************************************
 * TEST COUNTER FUNCTIONS
 *****************************************************************************/

// II.   test counter operations
void d_test_counter_reset(struct d_test_counter* _counter);
void d_test_counter_add(struct d_test_counter*       _dest,
                        const struct d_test_counter* _src);


/******************************************************************************
 * ASSERTION FUNCTION
 *****************************************************************************/

// III.  standalone assertion
bool d_assert_standalone(bool                   _condition,
                         const char*            _test_name,
                         const char*            _message,
                         struct d_test_counter* _test_info);


/******************************************************************************
 * TEMPLATE SUBSTITUTION
 *****************************************************************************/

// IV.   template substitution
char* d_test_substitute_template(const char* _template,
                                 const char* _delimiters[2],
                                 size_t      _kv_count,
                                 const char* _key_values[][2]);


/******************************************************************************
 * DEFAULT PRINT FUNCTIONS
 *****************************************************************************/

// V.    default print functions
void d_test_default_print_object(const struct d_test_object* _obj,
                                 size_t                      _indent_level,
                                 struct d_test_counter*      _counter);
void d_test_default_print_object_to_file(FILE*                       _file,
                                         const struct d_test_object* _obj,
                                         size_t                      _indent_level,
                                         struct d_test_counter*      _counter);


/******************************************************************************
 * OUTPUT FUNCTIONS
 *****************************************************************************/

// VI.   output functions
void d_test_standalone_output_console(const char*                  _delimiters[2],
                                      size_t                       _kv_count,
                                      const char*                  _key_values[][2],
                                      size_t                       _obj_count,
                                      struct d_test_object* const* _objects,
                                      fn_print_object              _print_fn);
void d_test_standalone_output_file(const char*                  _filepath,
                                   const char*                  _delimiters[2],
                                   size_t                       _kv_count,
                                   const char*                  _key_values[][2],
                                   size_t                       _obj_count,
                                   struct d_test_object* const* _objects,
                                   fn_print_object_file         _print_fn);


/******************************************************************************
 * STANDALONE TEST OUTPUT FORMATTING FUNCTIONS
 *****************************************************************************/

// VII.  output formatting
void d_test_sa_create_framework_header(const char* _suite_name,
                                       const char* _description);
void d_test_sa_create_module_test_header(const char* _module_name,
                                         const char* _description);
void d_test_sa_create_module_test_results(const char*                  _module_name,
                                          const struct d_test_counter* _counter);
void d_test_sa_create_comprehensive_results(const struct d_test_sa_suite_results* _suite);
void d_test_sa_create_implementation_notes(size_t                               _section_count,
                                           const struct d_test_sa_note_section* _sections);
void d_test_sa_create_final_status(const char* _framework_name,
                                   bool        _passed);


/******************************************************************************
 * UNIFIED TEST RUNNER FUNCTIONS
 *****************************************************************************/

// VIII. unified test runner
void d_test_sa_runner_init(struct d_test_sa_runner* _runner,
                           const char*              _suite_name,
                           const char*              _suite_description);
void d_test_sa_runner_add_module(struct d_test_sa_runner*             _runner,
                                 const char*                          _name,
                                 const char*                          _description,
                                 fn_test_module                       _run_fn,
                                 size_t                               _note_count,
                                 const struct d_test_sa_note_section* _notes);
void d_test_sa_runner_add_module_counter(struct d_test_sa_runner*             _runner,
                                         const char*                          _name,
                                         const char*                          _description,
                                         fn_test_module_counter               _run_fn,
                                         size_t                               _note_count,
                                         const struct d_test_sa_note_section* _notes);
int  d_test_sa_runner_execute(struct d_test_sa_runner* _runner);
void d_test_sa_runner_set_wait_for_input(struct d_test_sa_runner* _runner,
                                         bool                     _wait);
void d_test_sa_runner_set_show_notes(struct d_test_sa_runner* _runner,
                                     bool                     _show);
void d_test_sa_runner_cleanup(struct d_test_sa_runner* _runner);


/******************************************************************************
 * UTILITY FUNCTIONS
 *****************************************************************************/

// IX.   utility functions
void   d_test_sa_print_timestamp(void);
double d_test_sa_get_elapsed_time(clock_t _start, clock_t _end);


#endif  // DJINTERP_TEST_STANDALONE_