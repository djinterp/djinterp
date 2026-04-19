/******************************************************************************
* djinterp [test]                                               test_module.h
*
*   Test module definition - a logical grouping of related tests.
*   A module represents a unit being tested (e.g., a source file, subsystem,
* or feature). It contains test blocks, configuration, and metadata.
*
* 
* path:      \inc\test\test_module.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.10.22
******************************************************************************/

#ifndef DJINTERP_TEST_MODULE_
#define DJINTERP_TEST_MODULE_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../djinterp.h"
#include "../container/map/min_enum_map.h"
#include "./test_common.h"
#include "./test_options.h"
#include "./test_block.h"


// D_TEST_DEFAULT_MODULE_FLAGS
//   macro:
#define D_TEST_DEFAULT_MODULE_FLAGS 0x00

/******************************************************************************
 * D_TEST_MODULE MACRO
 *****************************************************************************/

#define D_INTERNAL_TEST_MODULE_CONFIG_FROM_ARGS(paren_args)                  \
    d_test_module_validate_args(                                             \
        D_INTERNAL_TEST_ARG_ARRAY(paren_args),                               \
        D_INTERNAL_TEST_ARG_COUNT(paren_args)                                \
    )

#define D_INTERNAL_TEST_MODULE_ARGS(paren_args, ...)                         \
    D_TEST_TYPE_FROM_TEST_MODULE_CONFIG(                                     \
        d_test_module_new(                                                   \
            D_INTERNAL_TEST_CHILD_ARRAY(__VA_ARGS__),                        \
            D_VARG_COUNT(__VA_ARGS__)                                        \
        ),                                                                   \
        D_INTERNAL_TEST_MODULE_CONFIG_FROM_ARGS(paren_args)                  \
    )

#define D_INTERNAL_TEST_MODULE_NOARGS(...)                                   \
    D_TEST_TYPE_FROM_TEST_MODULE_CONFIG(                                     \
        d_test_module_new(                                                   \
            D_INTERNAL_TEST_CHILD_ARRAY(__VA_ARGS__),                        \
            D_VARG_COUNT(__VA_ARGS__)                                        \
        ),                                                                   \
        NULL                                                                 \
    )

#define D_TEST_MODULE(...)                                                   \
    D_INTERNAL_OPTARGS_DISPATCH(                                             \
        D_INTERNAL_TEST_MODULE_ARGS,                                         \
        D_INTERNAL_TEST_MODULE_NOARGS,                                       \
        __VA_ARGS__                                                          \
    )


/******************************************************************************
 * STATUS ENUM
 *****************************************************************************/

// DTestModuleStatus
//   enum: status of a test module.
enum DTestModuleStatus
{
    D_TEST_MODULE_STATUS_UNKNOWN = 0,
    D_TEST_MODULE_STATUS_PENDING = 1,   // not yet run
    D_TEST_MODULE_STATUS_RUNNING = 2,   // currently executing
    D_TEST_MODULE_STATUS_PASSED  = 3,   // all tests passed
    D_TEST_MODULE_STATUS_FAILED  = 4,   // one or more tests failed
    D_TEST_MODULE_STATUS_SKIPPED = 5,   // module was skipped
    D_TEST_MODULE_STATUS_ERROR   = 6    // setup/teardown error
};


/******************************************************************************
 * RESULT STRUCTURE
 *****************************************************************************/

// d_test_module_result
//   struct: results from running a test module.
struct d_test_module_result
{
    size_t                 tests_total;       // total tests in module
    size_t                 tests_passed;      // tests that passed
    size_t                 tests_failed;      // tests that failed
    size_t                 tests_skipped;     // tests that were skipped

    size_t                 assertions_total;  // total assertions
    size_t                 assertions_passed; // assertions that passed
    
    size_t                 blocks_total;      // total blocks in module
    size_t                 blocks_passed;     // blocks that passed
    
    enum DTestModuleStatus status;            // overall status
    double                 duration_ms;       // execution time in ms
};

/******************************************************************************
 * MODULE STRUCTURE
 *****************************************************************************/

// d_test_module
//   struct: a module containing related test blocks.
struct d_test_module
{
    struct d_ptr_vector*         children;
    struct d_test_options*        config;          // module-level configuration
    struct d_test_module_result* result;          // results from last run
    enum DTestModuleStatus       status;          // current status
};


/******************************************************************************
 * CONSTRUCTOR/DESTRUCTOR FUNCTIONS
 *****************************************************************************/

struct d_test_module* d_test_module_new(struct d_test_type** _children,
                                        size_t               _child_count);
struct d_test_module* d_test_module_new_args(struct d_test_arg*   _args,
                                             size_t               _arg_count,
                                             struct d_test_type** _children,
                                             size_t               _child_count);
struct d_test_options* d_test_module_validate_args(struct d_test_arg* _args,
                                                  size_t             _arg_count);


/******************************************************************************
 * CHILD MANAGEMENT FUNCTIONS
 *****************************************************************************/

bool                d_test_module_add_child(struct d_test_module*          _module,
                                            const struct d_test_tree_node* _child);
size_t              d_test_module_child_count(const struct d_test_module* _module);
struct d_test_type* d_test_module_get_child_at(const struct d_test_module* _module,
                                               size_t                      _index);


/******************************************************************************
 * CONFIGURATION FUNCTIONS
 *****************************************************************************/

struct d_test_options* d_test_module_get_effective_settings(
                          const struct d_test_module* _module,
                          struct d_test_options*       _parent_settings,
                          struct d_min_enum_map*      _node_config);

const char* d_test_module_get_name(const struct d_test_module* _module);


/******************************************************************************
 * STAGE HOOK FUNCTIONS
 *****************************************************************************/

bool     d_test_module_set_stage_hook(struct d_test_module* _module,
                                      enum DTestStage       _stage,
                                      fn_stage              _hook);
fn_stage d_test_module_get_stage_hook(const struct d_test_module* _module,
                                      enum DTestStage             _stage);


/******************************************************************************
 * EXECUTION FUNCTIONS
 *****************************************************************************/

bool d_test_module_run(struct d_test_module* _module,
                       struct d_test_options* _parent_settings);
bool d_test_module_run_child(struct d_test_module* _module,
                             size_t                _child_index,
                             struct d_test_options* _parent_settings);


/******************************************************************************
 * RESULT FUNCTIONS
 *****************************************************************************/

struct d_test_module_result* d_test_module_get_result(const struct d_test_module* _module);
void                         d_test_module_reset_result(struct d_test_module* _module);
double                       d_test_module_get_pass_rate(const struct d_test_module* _module);


/******************************************************************************
 * OUTPUT FUNCTIONS
 *****************************************************************************/

void        d_test_module_print(const struct d_test_module* _module,
                                const char*                 _prefix,
                                size_t                      _prefix_length);
void        d_test_module_print_result(const struct d_test_module* _module);
const char* d_test_module_status_to_string(enum DTestModuleStatus _status);

void        d_test_module_free(struct d_test_module* _test_module);


#endif  // DJINTERP_TEST_MODULE_