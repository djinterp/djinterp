/*******************************************************************************
* djinterp [c][functional]                                          fn_builder.h
*
* Fluent builder pattern for constructing function chains.
*   Provides a builder that records a chain of steps - transformers (map,
* and_then) and predicates (filter, where) - in the order they are added,
* and runs the chain over an array in that order: a predicate tests the value
* the steps before it produced, as the C++ face (fn_builder.hpp) does.
*
*
* path:      /inc/djinterp/c/functional/fn_builder.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.02.06
*                                                            revised: 2026.11.11
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES AND CONSTANTS
    -------------------
    1.  Constants
         1.  D_FN_BUILDER_INITIAL_CAPACITY
    2.  Types
         1.  dj_fn_builder_step_kind
         2.  dj_fn_builder_step
         3.  dj_fn_builder
2.  OPERATIONS
    ----------
    1.  Creation
    2.  Fluent operations
    3.  Execution
    4.  Cleanup
3.  DEPRECATED NAMES
    ----------------
    1.  d_funtional_builder_*
*/

#ifndef DJINTERP_C_FUNCTIONAL_FN_BUILDER_H
#define DJINTERP_C_FUNCTIONAL_FN_BUILDER_H 1

// std
#include <stddef.h>  // size_t
// djinterp
#include "../djinterp.h"           // framework root
#include "./functional_common.h"  // fn_transformer, fn_predicate


//==============================================================================
// 1.  TYPES AND CONSTANTS
//==============================================================================
// The builder's step record, the builder, and its first capacity.


// 1.1    constants
//------------------------------------------------------------------------------
// 1.1.1
// D_FN_BUILDER_INITIAL_CAPACITY
//   constant: the number of steps a new builder has room for.
#define D_FN_BUILDER_INITIAL_CAPACITY 8

// 1.2    types
//------------------------------------------------------------------------------
// 1.2.1
// dj_fn_builder_step_kind
//   enum: what a step does to the value in hand.
enum dj_fn_builder_step_kind
{
    D_FN_BUILDER_STEP_TRANSFORM = 0,  // replaces the value with its image
    D_FN_BUILDER_STEP_PREDICATE = 1   // keeps the element, or drops it
};

// 1.2.2
// dj_fn_builder_step
//   struct: one step of the chain: its kind, and the function the kind names.
struct dj_fn_builder_step
{
    enum dj_fn_builder_step_kind kind;
    union
    {
        fn_transformer transform;  // when kind is D_FN_BUILDER_STEP_TRANSFORM
        fn_predicate   predicate;  // when kind is D_FN_BUILDER_STEP_PREDICATE
    } fn;
};

// 1.2.3
// dj_fn_builder
//   struct: fluent builder for a function chain, its steps in call order.
// The fields are the builder's own: read them, and change them only through
// the operations below.
struct dj_fn_builder
{
    struct dj_fn_builder_step* steps;     // the chain, in the order added
    size_t                     count;     // steps in the chain
    size_t                     capacity;  // steps room is allocated for
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================
// Builds a chain step by step, runs it over arrays, and frees it.


// 2.1    creation
//------------------------------------------------------------------------------
/**
 * @brief Allocates an empty builder with room for
 *        D_FN_BUILDER_INITIAL_CAPACITY steps.
 *
 * @return the builder, which the caller frees with d_fn_builder_free, or NULL
 *         when an allocation fails.
 */
struct dj_fn_builder* d_fn_builder_new(void);

// 2.2    fluent operations
//------------------------------------------------------------------------------
/**
 * @brief Appends a transformer step: the value in hand is replaced by
 *        _transform's image of it.
 *
 * @param[in,out] _builder    the builder; may be NULL.
 * @param[in]     _transform  the transformer; NULL adds nothing.
 * @return _builder, to chain the next call; NULL when _builder is NULL or the
 *         chain cannot grow. A builder that cannot grow is unchanged and still
 *         the caller's to free, so keep the original pointer when chaining.
 */
struct dj_fn_builder* d_fn_builder_map(
    struct dj_fn_builder* _builder,
    fn_transformer        _transform
);

/**
 * @brief Appends a predicate step: the element is kept only if _test accepts
 *        the value in hand - the value the steps before it produced.
 *
 * @param[in,out] _builder  the builder; may be NULL.
 * @param[in]     _test     the predicate; NULL adds nothing.
 * @return as d_fn_builder_map.
 */
struct dj_fn_builder* d_fn_builder_filter(
    struct dj_fn_builder* _builder,
    fn_predicate          _test
);

// d_fn_builder_and_then and d_fn_builder_where are d_fn_builder_map and
// d_fn_builder_filter under the names a reader of a chain may expect.
struct dj_fn_builder* d_fn_builder_and_then(
    struct dj_fn_builder* _builder,
    fn_transformer        _transform
);
struct dj_fn_builder* d_fn_builder_where(
    struct dj_fn_builder* _builder,
    fn_predicate          _test
);

// 2.3    execution
//------------------------------------------------------------------------------
/**
 * @brief Runs the chain over each element of _input, writing the elements it
 *        keeps, as transformed, packed and in order into _output.
 *
 * @note Each element starts as the value in hand and meets the steps in the
 *       order they were added: a transformer replaces the value, and a
 *       predicate that rejects the value drops the element. Transformers keep
 *       the element size. Callbacks receive a NULL context.
 *
 * @param[in]  _builder       the chain; an empty chain copies every element.
 * @param[in]  _input         the elements read; may be NULL only when _count
 *                            is 0.
 * @param[in]  _count         the number of elements in _input.
 * @param[in]  _element_size  the size of each element, in bytes.
 * @param[out] _output        room for _count elements; may be _input itself,
 *                            to run the chain in place. Elements past
 *                            *_out_count are left as they were.
 * @param[out] _out_count     the number of elements written; may be NULL.
 * @return `true` if every element ran; `false` if _builder is NULL, or
 *         _input or _output is NULL or _element_size is 0 while _count is not
 *         0, or the working memory cannot be allocated, or a transformer
 *         fails. A failed transformer stops the run, and *_out_count is then
 *         the number of elements completed before it.
 */
bool d_fn_builder_execute(
    const struct dj_fn_builder* _builder,
    const void*                 _input,
    size_t                      _count,
    size_t                      _element_size,
    void*                       _output,
    size_t*                     _out_count
);

// 2.4    cleanup
//------------------------------------------------------------------------------
/**
 * @brief Frees the builder and its chain.
 *
 * @param[in] _builder  the builder; NULL does nothing.
 * @post _builder no longer points to a builder.
 */
void d_fn_builder_free(
    struct dj_fn_builder* _builder
);


//==============================================================================
// 3.  DEPRECATED NAMES
//==============================================================================
// The fluent operations' first, misspelt names. They name the same functions,
// and go at 0.9.0-rc.1.


// 3.1    d_funtional_builder_*
//------------------------------------------------------------------------------
#define d_funtional_builder_map      d_fn_builder_map
#define d_funtional_builder_filter   d_fn_builder_filter
#define d_funtional_builder_and_then d_fn_builder_and_then
#define d_funtional_builder_where    d_fn_builder_where


#endif  // DJINTERP_C_FUNCTIONAL_FN_BUILDER_H
