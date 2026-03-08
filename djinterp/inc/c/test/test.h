/******************************************************************************
* djinterp [test]                                                       test.h
*
*   Core test framework providing individual test structures that can contain
* a mix of test functions and assertions. Tests can be created via the D_TEST
* macro which supports optional configuration arguments and variadic test items.
*
* A test can contain only:
*   - d_assert (assertions)
*   - d_test_fn (test functions)
*
* Configuration is resolved at runtime by combining the test's own config with
* the run config passed to d_test_run(). Parent types pass their resolved
* config down when executing children.
*
*
* path:      /inc/c/test/test.h       
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.14
******************************************************************************/

#ifndef DJINTERP_TEST_TEST_
#define DJINTERP_TEST_TEST_ 1

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../djinterp.h"
#include "../dmemory.h"
#include "../container/map/min_enum_map.h"
#include "../container/vector/ptr_vector.h"
#include "./test_common.h"
#include "./test_options.h"
#include "./assert.h"


/******************************************************************************
 * FORWARD DECLARATIONS
 *****************************************************************************/

struct d_test;
struct d_test_block;
struct d_test_module;


/******************************************************************************
 * TEST TYPE (DISCRIMINATED UNION)
 *****************************************************************************/

// d_test_type
//   struct: discriminated union for test tree node types.
// Wraps an assertion, test function, test, block, or module with a type tag.
struct d_test_type
{
    enum DTestTypeFlag type;

    union
    {
        struct d_assert*       assertion;
        struct d_test_fn*      test_fn;
        struct d_test*         test;
        struct d_test_block*   block;
        struct d_test_module*  module;
    };
};

// construction / destruction
struct d_test_type* d_test_type_new(enum DTestTypeFlag _type, void* _data);
void                d_test_type_free(struct d_test_type* _type);


/******************************************************************************
 * D_TEST MACRO
 *****************************************************************************/

#define D_INTERNAL_TEST_CONFIG_FROM_ARGS(paren_args)                         \
    d_test_validate_args(                                                    \
        D_INTERNAL_TEST_ARG_ARRAY(paren_args),                               \
        D_INTERNAL_TEST_ARG_COUNT(paren_args)                                \
    )

#define D_INTERNAL_TEST_ARGS(paren_args, ...)                                \
    D_TEST_TYPE_FROM_TEST_CONFIG(                                            \
        d_test_new(                                                          \
            D_INTERNAL_TEST_CHILD_ARRAY(__VA_ARGS__),                        \
            D_VARG_COUNT(__VA_ARGS__)                                        \
        ),                                                                   \
        D_INTERNAL_TEST_CONFIG_FROM_ARGS(paren_args)                         \
    )

#define D_INTERNAL_TEST_NOARGS(...)                                          \
    D_TEST_TYPE_FROM_TEST_CONFIG(                                            \
        d_test_new(                                                          \
            D_INTERNAL_TEST_CHILD_ARRAY(__VA_ARGS__),                        \
            D_VARG_COUNT(__VA_ARGS__)                                        \
        ),                                                                   \
        NULL                                                                 \
    )

#define D_TEST(...)                                                          \
    D_INTERNAL_OPTARGS_DISPATCH(                                             \
        D_INTERNAL_TEST_ARGS,                                                \
        D_INTERNAL_TEST_NOARGS,                                              \
        __VA_ARGS__                                                          \
    )


/******************************************************************************
 * D_TEST_FN MACRO
 *****************************************************************************/

#define D_INTERNAL_TEST_FN_CONFIG_FROM_ARGS(paren_args)                      \
    d_test_fn_validate_args(                                                 \
        D_INTERNAL_TEST_ARG_ARRAY(paren_args),                               \
        D_INTERNAL_TEST_ARG_COUNT(paren_args)                                \
    )

#define D_INTERNAL_TEST_FN_ARGS(paren_args, fn)                              \
    D_TEST_TYPE_FROM_TEST_FN_CONFIG(                                         \
        d_test_fn_new((fn_test)(fn)),                                        \
        D_INTERNAL_TEST_FN_CONFIG_FROM_ARGS(paren_args)                      \
    )

#define D_INTERNAL_TEST_FN_NOARGS(fn)                                        \
    D_TEST_TYPE_FROM_TEST_FN_CONFIG(                                         \
        d_test_fn_new((fn_test)(fn)),                                        \
        NULL                                                                 \
    )

#define D_TEST_FN(...)                                                       \
    D_INTERNAL_OPTARGS_DISPATCH(                                             \
        D_INTERNAL_TEST_FN_ARGS,                                             \
        D_INTERNAL_TEST_FN_NOARGS,                                           \
        __VA_ARGS__                                                          \
    )


/******************************************************************************
 * ASSERTION WRAPPER MACROS
 *****************************************************************************/

// D_TEST_ASSERT
//   macro: wraps a generic boolean assertion into a test type.
#define D_TEST_ASSERT(condition, msg_pass, msg_fail)                         \
    D_TEST_TYPE_FROM_ASSERT(                                                 \
        d_assert_new((condition), (msg_pass), (msg_fail)))

// D_TEST_ASSERT_TRUE
//   macro: asserts a condition is true.
#define D_TEST_ASSERT_TRUE(condition, msg_pass, msg_fail)                    \
    D_TEST_TYPE_FROM_ASSERT(                                                 \
        d_assert_new((condition), (msg_pass), (msg_fail)))

// D_TEST_ASSERT_FALSE
//   macro: asserts a condition is false.
#define D_TEST_ASSERT_FALSE(condition, msg_pass, msg_fail)                   \
    D_TEST_TYPE_FROM_ASSERT(                                                 \
        d_assert_false((condition), (msg_pass), (msg_fail)))

// D_TEST_ASSERT_EQ
//   macro: asserts two values are equal using a comparator.
#define D_TEST_ASSERT_EQ(a, b, cmp, msg_pass, msg_fail)                      \
    D_TEST_TYPE_FROM_ASSERT(                                                 \
        d_assert_eq((a), (b), (cmp), (msg_pass), (msg_fail)))

// D_TEST_ASSERT_NEQ
//   macro: asserts two values are not equal.
#define D_TEST_ASSERT_NEQ(a, b, cmp, msg_pass, msg_fail)                     \
    D_TEST_TYPE_FROM_ASSERT(                                                 \
        d_assert_neq((a), (b), (cmp), (msg_pass), (msg_fail)))

// D_TEST_ASSERT_NULL
//   macro: asserts a pointer is NULL.
#define D_TEST_ASSERT_NULL(ptr, msg_pass, msg_fail)                          \
    D_TEST_TYPE_FROM_ASSERT(                                                 \
        d_assert_null((ptr), (msg_pass), (msg_fail)))

// D_TEST_ASSERT_NONNULL
//   macro: asserts a pointer is not NULL.
#define D_TEST_ASSERT_NONNULL(ptr, msg_pass, msg_fail)                       \
    D_TEST_TYPE_FROM_ASSERT(                                                 \
        d_assert_nonnull((ptr), (msg_pass), (msg_fail)))

// D_TEST_ASSERT_STR_EQ
//   macro: asserts two strings are equal.
#define D_TEST_ASSERT_STR_EQ(s1, s2, msg_pass, msg_fail)                     \
    D_TEST_TYPE_FROM_ASSERT(                                                 \
        d_assert_str_eq((s1), (s2), (msg_pass), (msg_fail)))

// D_TEST_ASSERT_LT
//   macro: asserts a < b.
#define D_TEST_ASSERT_LT(a, b, cmp, msg_pass, msg_fail)                      \
    D_TEST_TYPE_FROM_ASSERT(                                                 \
        d_assert_lt((a), (b), (cmp), (msg_pass), (msg_fail)))

// D_TEST_ASSERT_GT
//   macro: asserts a > b.
#define D_TEST_ASSERT_GT(a, b, cmp, msg_pass, msg_fail)                      \
    D_TEST_TYPE_FROM_ASSERT(                                                 \
        d_assert_gt((a), (b), (cmp), (msg_pass), (msg_fail)))


/******************************************************************************
 * AUTO-MESSAGE ASSERTION MACROS
 *****************************************************************************/

// D_INTERNAL_ASSERT_MSG_PASS
//   macro (internal): generates a pass message from expression.
#define D_INTERNAL_ASSERT_MSG_PASS(expr)                                     \
    "PASS: " D_STRINGIFY(expr)

// D_INTERNAL_ASSERT_MSG_FAIL
//   macro (internal): generates a fail message from expression.
#define D_INTERNAL_ASSERT_MSG_FAIL(expr)                                     \
    "FAIL: " D_STRINGIFY(expr)

// D_ASSERTION_TRUE
//   macro: asserts condition is true with auto-generated messages.
#define D_ASSERTION_TRUE(condition)                                          \
    D_TEST_TYPE_FROM_ASSERT(                                                 \
        d_assert_new((condition),                                                \
                 D_INTERNAL_ASSERT_MSG_PASS(condition),                      \
                 D_INTERNAL_ASSERT_MSG_FAIL(condition)))

// D_ASSERTION_FALSE
//   macro: asserts condition is false with auto-generated messages.
#define D_ASSERTION_FALSE(condition)                                         \
    D_TEST_TYPE_FROM_ASSERT(                                                 \
        d_assert_false((condition),                                          \
                       D_INTERNAL_ASSERT_MSG_PASS(!(condition)),             \
                       D_INTERNAL_ASSERT_MSG_FAIL(!(condition))))

// D_ASSERTION_EQ
//   macro: asserts equality with auto-generated messages.
#define D_ASSERTION_EQ(a, b, cmp)                                            \
    D_TEST_TYPE_FROM_ASSERT(                                                 \
        d_assert_eq((a), (b), (cmp),                                         \
                    D_INTERNAL_ASSERT_MSG_PASS((a) == (b)),                  \
                    D_INTERNAL_ASSERT_MSG_FAIL((a) == (b))))

// D_ASSERTION_NEQ
//   macro: asserts inequality with auto-generated messages.
#define D_ASSERTION_NEQ(a, b, cmp)                                           \
    D_TEST_TYPE_FROM_ASSERT(                                                 \
        d_assert_neq((a), (b), (cmp),                                        \
                     D_INTERNAL_ASSERT_MSG_PASS((a) != (b)),                 \
                     D_INTERNAL_ASSERT_MSG_FAIL((a) != (b))))

// D_ASSERTION_LT
//   macro: asserts less-than with auto-generated messages.
#define D_ASSERTION_LT(a, b, cmp)                                            \
    D_TEST_TYPE_FROM_ASSERT(                                                 \
        d_assert_lt((a), (b), (cmp),                                         \
                    D_INTERNAL_ASSERT_MSG_PASS((a) < (b)),                   \
                    D_INTERNAL_ASSERT_MSG_FAIL((a) < (b))))

// D_ASSERTION_GT
//   macro: asserts greater-than with auto-generated messages.
#define D_ASSERTION_GT(a, b, cmp)                                            \
    D_TEST_TYPE_FROM_ASSERT(                                                 \
        d_assert_gt((a), (b), (cmp),                                         \
                    D_INTERNAL_ASSERT_MSG_PASS((a) > (b)),                   \
                    D_INTERNAL_ASSERT_MSG_FAIL((a) > (b))))

// D_ASSERTION_NULL
//   macro: asserts NULL with auto-generated messages.
#define D_ASSERTION_NULL(ptr)                                                \
    D_TEST_TYPE_FROM_ASSERT(                                                 \
        d_assert_null((ptr),                                                 \
                      D_INTERNAL_ASSERT_MSG_PASS((ptr) == NULL),             \
                      D_INTERNAL_ASSERT_MSG_FAIL((ptr) == NULL)))

// D_ASSERTION_NONNULL
//   macro: asserts non-NULL with auto-generated messages.
#define D_ASSERTION_NONNULL(ptr)                                             \
    D_TEST_TYPE_FROM_ASSERT(                                                 \
        d_assert_nonnull((ptr),                                              \
                         D_INTERNAL_ASSERT_MSG_PASS((ptr) != NULL),          \
                         D_INTERNAL_ASSERT_MSG_FAIL((ptr) != NULL)))


// d_test
//   struct: individual test containing configuration, lifecycle hooks, and
// a vector of child items (assertions and/or test functions).
struct d_test
{
    struct d_ptr_vector*   children;       // child test types (assertions/fns)
    struct d_test_options*  config;         // owned configuration
    struct d_min_enum_map* stage_hooks;    // lifecycle hooks by DTestStage
};


// construction
struct d_test*        d_test_new(struct d_test_type** _children, size_t _child_count);
struct d_test*        d_test_new_args(struct d_test_arg* _args, size_t _arg_count, struct d_test_type** _children, size_t _child_count);
struct d_test_options* d_test_validate_args(struct d_test_arg* _args, size_t _arg_count);

// test function wrapper
struct d_test_fn*     d_test_fn_new(fn_test _fn);
struct d_test_options* d_test_fn_validate_args(struct d_test_arg* _args, size_t _arg_count);

// child management
bool                  d_test_add_child(struct d_test* _test, struct d_test_type* _child);
bool                  d_test_add_assertion(struct d_test* _test, struct d_assert* _assertion);
bool                  d_test_add_function(struct d_test* _test, fn_test _fn);
size_t                d_test_child_count(const struct d_test* _test);
struct d_test_type*   d_test_get_child_at(const struct d_test* _test, size_t _index);

// stage hook functions
bool                  d_test_set_stage_hook(struct d_test* _test, enum DTestStage _stage, fn_stage _hook);
fn_stage              d_test_get_stage_hook(const struct d_test* _test, enum DTestStage _stage);

// test execution
bool                  d_test_run(struct d_test* _test, const struct d_test_options* _run_config);

// utility
void                  d_test_print(const struct d_test* _test, const char* _prefix, size_t _prefix_length);
const char*           d_test_type_flag_to_string(enum DTestTypeFlag _type);

// destruction
void                  d_test_fn_free(struct d_test_fn* _test_fn);
void                  d_test_free(struct d_test* _test);


#endif  // DJINTERP_TEST_TEST_