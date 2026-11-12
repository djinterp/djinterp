/*******************************************************************************
* djinterp [c][test]                                                    assert.h
*
* A runtime equivalent for asserting.
*
* HOW TO USE
* ----------
*   Not yet. This header is part of the classic C tree, which does not compile
* while it is ported (docs/djinterp_testing.md, section 13). Write C suites
* against test_suite.h.
*
* OPTIONS
* -------
*   None: nothing in this header is chosen at run time.
*
* CONFIGURATION
* -------------
*   None: no D_CFG_* knob reaches this header.
*
*
* path:      /inc/djinterp/c/modules/test/default/assert.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2023.04.29
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_C_TEST_DEFAULT_ASSERT_H
#define DJINTERP_C_TEST_DEFAULT_ASSERT_H 1

// std
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// djinterp
#include "../../../djinterp.h"


// D_INTERNAL_TEST_ASSERT_BOOLEAN_FN_BODY
//   macro (internal)
#define D_INTERNAL_TEST_ASSERT_FN_BODY(fn_body)                             \
    struct dj_assert* new_assertion = malloc(sizeof(struct dj_assert));       \
                                                                            \
    if (!new_assertion)                                                     \
    {                                                                       \
        return NULL;                                                        \
    }                                                                       \
                                                                            \
    fn_body                                                                 \
                                                                            \
    return new_assertion;

// D_INTERNAL_TEST_ASSERT_BOOLEAN_FN_BODY
//   macro (internal)
#define D_INTERNAL_TEST_ASSERT_FN_BOOL_COMPARE(oper)                        \
    D_INTERNAL_TEST_ASSERT_FN_BODY(                                         \
        new_assertion->result = (_comparator)                               \
                                ? (_comparator(_a, _b) oper 0)              \
                                : _a oper _b;                               \
        new_assertion->message = new_assertion->result                      \
                                 ? _message_true                            \
                                 : _message_false;                          \
    )


/*******************************************************************************
 * ASSERTION STRUCTURE
 *****************************************************************************/

// dj_assert
//   struct: a type representing the result of a boolean condition, with the
// message corresponding to its value. Used for unit testing; one or more
// assertions can be used to compose a thorough unit test function.
struct dj_assert
{
    bool        result;
    const char* message;
};


/*******************************************************************************
 * FUNCTION DECLARATIONS
 *****************************************************************************/

int d_assert_default_compare(
    const void* _expected,
    const void* _actual
);

struct dj_assert* d_assert_new(
    bool        _expression,
    const char* _message_true,
    const char* _message_false
);

struct dj_assert* d_assert_true(
    bool        _expression,
    const char* _message_true,
    const char* _message_false
);

struct dj_assert* d_assert_false(
    bool        _expression,
    const char* _message_true,
    const char* _message_false
);

struct dj_assert* d_assert_eq(
    const void*   _a,
    const void*   _b,
    fn_comparator _comparator,
    const char*   _message_true,
    const char*   _message_false
);

struct dj_assert* d_assert_neq(
    const void*   _a,
    const void*   _b,
    fn_comparator _comparator,
    const char*   _message_true,
    const char*   _message_false
);

struct dj_assert* d_assert_lt(
    const void*   _a,
    const void*   _b,
    fn_comparator _comparator,
    const char*   _message_true,
    const char*   _message_false
);

struct dj_assert* d_assert_lt_eq(
    const void*   _a,
    const void*   _b,
    fn_comparator _comparator,
    const char*   _message_true,
    const char*   _message_false
);

struct dj_assert* d_assert_gt(
    const void*   _a,
    const void*   _b,
    fn_comparator _comparator,
    const char*   _message_true,
    const char*   _message_false
);

struct dj_assert* d_assert_gt_eq(
    const void*   _a,
    const void*   _b,
    fn_comparator _comparator,
    const char*   _message_true,
    const char*   _message_false
);

struct dj_assert* d_assert_str_eq(
    const char* _str1,
    size_t      _str1_length,
    const char* _str2,
    size_t      _str2_length,
    const char* _message_true,
    const char* _message_false
);

struct dj_assert* d_assert_str_neq(
    const char* _str1,
    size_t      _str1_length,
    const char* _str2,
    size_t      _str2_length,
    const char* _message_true,
    const char* _message_false
);

struct dj_assert* d_assert_null(
    const void* _expression,
    const char* _message_true,
    const char* _message_false
);

struct dj_assert* d_assert_nonnull(
    const void* _expression,
    const char* _message_true,
    const char* _message_false
);

struct dj_assert* d_assert_array_is_valid(
    const void** _arr,
    size_t       _arr_length,
    const char*  _message_true,
    const char*  _message_false
);

struct dj_assert* d_assert_arrays_eq(
    const void*   _arr1,
    const void*   _arr2,
    size_t        _arr_size,
    size_t        _element_size,
    fn_comparator _comparator,
    const char*   _message_pass,
    const char*   _message_fail
);

void d_assert_free(
    struct dj_assert* _assertion
);


#endif  // DJINTERP_C_TEST_DEFAULT_ASSERT_H
