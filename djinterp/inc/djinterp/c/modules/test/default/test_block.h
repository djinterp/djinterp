/*******************************************************************************
* djinterp [c/test]                                                 test_block.h
*
*   Test block structures for grouping multiple tests with shared configuration
* and lifecycle hooks. A test block can contain individual tests, other test
* blocks, assertions, or test functions as children.
*
*   Configuration and lifecycle hooks are managed externally through the
* dj_test_options structure, which is attached at the dj_test_type wrapper
* level via D_TEST_TYPE_FROM_TEST_BLOCK_CONFIG.
*
*
* path:      /inc/djinterp/c/modules/test/default/test_block.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2023.05.25
*                                                            revised: 2026.09.29
*******************************************************************************/

#ifndef DJINTERP_C_TEST_DEFAULT_TEST_BLOCK_H
#define DJINTERP_C_TEST_DEFAULT_TEST_BLOCK_H 1

// std
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
// djinterp
#include "../../../djinterp.h"
#include "../../../memory/dmemory.h"
#include "../../../container/map/min_enum_map.h"
#include "../../../container/vector/ptr_vector.h"
#include "test.h"


/*******************************************************************************
 * FORWARD DECLARATIONS
 *****************************************************************************/

struct dj_test;
struct dj_test_module;


/*******************************************************************************
 * D_TEST_BLOCK                                                            MACRO
 *****************************************************************************/

#define D_INTERNAL_TEST_BLOCK_CONFIG_FROM_ARGS(paren_args)                   \
    d_test_block_validate_args(                                              \
        D_INTERNAL_TEST_ARG_ARRAY(paren_args),                               \
        D_INTERNAL_TEST_ARG_COUNT(paren_args)                                \
    )

#define D_INTERNAL_TEST_BLOCK_ARGS(paren_args, ...)                          \
    D_TEST_TYPE_FROM_TEST_BLOCK_CONFIG(                                      \
        d_test_block_new(                                                    \
            D_INTERNAL_TEST_CHILD_ARRAY(__VA_ARGS__),                        \
            D_VARG_COUNT(__VA_ARGS__)                                        \
        ),                                                                   \
        D_INTERNAL_TEST_BLOCK_CONFIG_FROM_ARGS(paren_args)                   \
    )

#define D_INTERNAL_TEST_BLOCK_NOARGS(...)                                    \
    D_TEST_TYPE_FROM_TEST_BLOCK_CONFIG(                                      \
        d_test_block_new(                                                    \
            D_INTERNAL_TEST_CHILD_ARRAY(__VA_ARGS__),                        \
            D_VARG_COUNT(__VA_ARGS__)                                        \
        ),                                                                   \
        NULL                                                                 \
    )

#define D_TEST_BLOCK(...)                                                    \
    D_INTERNAL_OPTARGS_DISPATCH(                                             \
        D_INTERNAL_TEST_BLOCK_ARGS,                                          \
        D_INTERNAL_TEST_BLOCK_NOARGS,                                        \
        __VA_ARGS__                                                          \
    )


/*******************************************************************************
 * TEST BLOCK STRUCTURE
 *****************************************************************************/

// dj_test_block
//   struct: collection of tests with shared execution control.
// Children can be sub-blocks, tests, assertions, or test functions.
// Configuration and lifecycle hooks are managed externally through
// the dj_test_options attached at the dj_test_type wrapper level.
struct dj_test_block
{
    struct dj_ptr_vector*   children;       // child tree nodes
};


/*******************************************************************************
 * CONSTRUCTOR/DESTRUCTOR                                              FUNCTIONS
 *****************************************************************************/

struct dj_test_block* d_test_block_new(
    struct dj_test_type** _children,
    size_t                _child_count
);
struct dj_test_block* d_test_block_new_args(
    struct dj_test_arg*   _args,
    size_t                _arg_count,
    struct dj_test_type** _children,
    size_t                _child_count
);
struct dj_test_options* d_test_block_validate_args(
    struct dj_test_arg* _args,
    size_t              _arg_count
);

void d_test_block_free(
    struct dj_test_block* _block
);


/*******************************************************************************
 * CHILD MANAGEMENT FUNCTIONS
 *****************************************************************************/

bool d_test_block_add_child(
    struct dj_test_block* _block,
    struct dj_test_type*  _child
);
bool d_test_block_add_test(
    struct dj_test_block* _block,
    struct dj_test*       _test
);
bool d_test_block_add_block(
    struct dj_test_block* _parent,
    struct dj_test_block* _child
);
size_t d_test_block_child_count(
    const struct dj_test_block* _block
);
struct dj_test_type* d_test_block_get_child_at(
    const struct dj_test_block* _block,
    size_t                      _index
);


/*******************************************************************************
 * EXECUTION FUNCTIONS
 *****************************************************************************/

bool d_test_block_run(
    struct dj_test_block*         _block,
    const struct dj_test_options* _run_config
);


/*******************************************************************************
 * UTILITY FUNCTIONS
 *****************************************************************************/

void d_test_block_print(
    const struct dj_test_block* _block,
    const char*                 _prefix,
    size_t                      _prefix_length
);
size_t d_test_block_count_tests(
    const struct dj_test_block* _block
);
size_t d_test_block_count_blocks(
    const struct dj_test_block* _block
);


#endif  // DJINTERP_C_TEST_DEFAULT_TEST_BLOCK_H
