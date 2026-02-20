/******************************************************************************
* djinterp [functional]                                             filter.h
*
* Collection filtering with expression-based selection.
*   Provides a comprehensive filtering framework for arrays that supports
* positional operations (take, skip, range, slice), predicate-based
* selection, index-based access, transformation operations (distinct,
* reverse), sequential chaining, and set-theoretic combinators (union,
* intersection, difference).
*
*   Predicate logic is handled by the predicate.h module; filter.h
* consumes fn_predicate function pointers and d_predicate_{and,or,not,xor}
* combinators rather than reimplementing its own expression tree.
*
* DESIGN PHILOSOPHY:
*   - Operations are composable (can be chained sequentially)
*   - Operations are combinable (via union/intersection/difference)
*   - Each operation is independent and self-contained
*   - Reuses functional module types (fn_predicate, fn_function_comparator)
*   - Zero-cost abstractions where possible
*
* TYPICAL USAGE PATTERNS:
*   - Take first N elements
*   - Skip M elements, then take N
*   - Filter by predicate(s) with complex boolean logic
*   - Combine multiple filters (union, intersection, difference)
*   - Chain operations (skip -> filter -> take)
*   - Range-based selection (elements [start, end))
*   - Iterate lazily over filtered results
*
*
* path:      \inc\functional\filter.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.06
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CONFIGURATION
      ---------------
      1.  Include guards and dependencies
      2.  Tuning constants

II.   CORE FILTER TYPES
      ------------------
      1.  Filter operation enumeration
      2.  Filter result status enumeration
      3.  Filter operation parameter structure
      4.  Filter operation structure
      5.  Filter chain structure
      6.  Filter result structure

III.  FILTER OPERATIONS
      ------------------
      1.  Take operations (first, last, nth, head, tail)
      2.  Skip operations (first, last, init, rest)
      3.  Range and slice operations
      4.  Predicate-based filtering
      5.  Index-based selection
      6.  Transformation operations (distinct, reverse)
      7.  Operation cleanup

IV.   FILTER CHAIN MANAGEMENT
      --------------------------
      1.  Chain creation and cloning
      2.  Adding operations
      3.  Convenience add helpers
      4.  Combining chains
      5.  Chain manipulation
      6.  Chain properties
      7.  Chain cleanup

V.    FILTER COMBINATORS
      -------------------
      1.  Union (OR semantics)
      2.  Intersection (AND semantics)
      3.  Difference (A - B)

VI.   EXECUTION AND APPLICATION
      --------------------------
      1.  Apply single operation
      2.  Apply filter chain
      3.  Apply combinators
      4.  Counting and querying
      5.  Index retrieval
      6.  In-place filtering
      7.  Result management

VII.  UTILITY FUNCTIONS
      ------------------
      1.  Validation
      2.  Description / serialization
      3.  Parsing (from string)
      4.  Optimization
      5.  Statistics

VIII. ITERATOR INTERFACE
      -------------------------
      1.  Iterator creation
      2.  Iterator operations
      3.  Iterator cleanup

IX.   FLUENT FILTER BUILDER
      ----------------------
      1.  Builder creation
      2.  Fluent operations
      3.  Builder finalization
      4.  Builder error handling
      5.  Builder cleanup

X.    CONVENIENCE MACROS
      -------------------
      1.  Single-expression shortcuts
      2.  Chain construction helpers
      3.  Fluent chain syntax

XI.   TYPED FILTER WRAPPERS (C11+)
*/

#ifndef DJINTERP_FILTER_
#define DJINTERP_FILTER_ 1


///////////////////////////////////////////////////////////////////////////////
///             I.    CONFIGURATION                                         ///
///////////////////////////////////////////////////////////////////////////////

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "..\djinterp.h"
#include "..\dmemory.h"
#include "..\dio.h"
#include ".\functional.h"

// D_FILTER_MAX_CHAIN_LENGTH
//   constant: maximum number of operations in a filter chain.
#ifndef D_FILTER_MAX_CHAIN_LENGTH
    #define D_FILTER_MAX_CHAIN_LENGTH 32
#endif


///////////////////////////////////////////////////////////////////////////////
///             II.   CORE FILTER TYPES                                     ///
///////////////////////////////////////////////////////////////////////////////

// d_filter_op_type
//   enum: types of filter operations.
enum d_filter_op_type
{
    D_FILTER_OP_NONE        = 0x00,  // no operation
    D_FILTER_OP_TAKE_FIRST  = 0x01,  // take first n elements
    D_FILTER_OP_TAKE_LAST   = 0x02,  // take last n elements
    D_FILTER_OP_SKIP_FIRST  = 0x03,  // skip first n elements
    D_FILTER_OP_SKIP_LAST   = 0x04,  // skip last n elements
    D_FILTER_OP_TAKE_NTH    = 0x05,  // take every nth element
    D_FILTER_OP_RANGE       = 0x06,  // take elements in range [start, end)
    D_FILTER_OP_WHERE       = 0x07,  // filter by predicate
    D_FILTER_OP_WHERE_NOT   = 0x08,  // filter by negated predicate
    D_FILTER_OP_INDICES     = 0x09,  // take elements at specific indices
    D_FILTER_OP_DISTINCT    = 0x0A,  // remove duplicate elements
    D_FILTER_OP_REVERSE     = 0x0B,  // reverse element order
    D_FILTER_OP_SLICE       = 0x0C,  // take slice [start:end:step]
    D_FILTER_OP_HEAD        = 0x0D,  // alias for take_first(1)
    D_FILTER_OP_TAIL        = 0x0E,  // alias for take_last(1)
    D_FILTER_OP_INIT        = 0x0F,  // all except last
    D_FILTER_OP_REST        = 0x10   // all except first
};

// d_filter_result_type
//   enum: result type of filter operation.
enum d_filter_result_type
{
    D_FILTER_RESULT_SUCCESS    =  0,  // operation succeeded
    D_FILTER_RESULT_EMPTY      =  1,  // no elements matched
    D_FILTER_RESULT_ERROR      = -1,  // operation failed
    D_FILTER_RESULT_INVALID    = -2,  // invalid parameters
    D_FILTER_RESULT_NO_MEMORY  = -3   // allocation failed
};

// struct d_filter_op_params
//   struct: parameters for a filter operation.
struct d_filter_op_params
{
    size_t                 count;          // count parameter (for take/skip)
    size_t                 start;          // start index (for range/slice)
    size_t                 end;            // end index (for range/slice)
    size_t                 step;           // step size (for slice/nth)
    size_t*                indices;        // array of indices (for index-based ops)
    size_t                 indices_count;  // number of indices
    fn_predicate           test;           // predicate function
    void*                  context;        // context for predicate
    fn_function_comparator comparator;     // comparator (for distinct)
};

// struct d_filter_operation
//   struct: single filter operation in a chain.
struct d_filter_operation
{
    enum d_filter_op_type     type;    // operation type
    struct d_filter_op_params params;  // operation parameters
    char*                     name;    // optional name/description
};

// struct d_filter_chain
//   struct: chain of sequential filter operations.
struct d_filter_chain
{
    struct d_filter_operation* operations;      // array of operations
    size_t                     count;           // number of operations
    size_t                     capacity;        // allocated capacity
    bool                       owns_operations; // whether chain owns ops
};

// struct d_filter_result
//   struct: result of applying a filter.
struct d_filter_result
{
    void*                     elements;      // resulting elements
    size_t                    count;         // number of results
    size_t*                   indices;       // original indices (if tracked)
    enum d_filter_result_type status;        // operation status
    char*                     error_message; // error description if failed
};


///////////////////////////////////////////////////////////////////////////////
///             III.  FILTER OPERATIONS                                     ///
///////////////////////////////////////////////////////////////////////////////

// i.    take operations
struct d_filter_operation* d_filter_take_first(size_t _n);
struct d_filter_operation* d_filter_take_last(size_t _n);
struct d_filter_operation* d_filter_take_nth(size_t _n);
struct d_filter_operation* d_filter_head(void);
struct d_filter_operation* d_filter_tail(void);

// ii.   skip operations
struct d_filter_operation* d_filter_skip_first(size_t _n);
struct d_filter_operation* d_filter_skip_last(size_t _n);
struct d_filter_operation* d_filter_init(void);
struct d_filter_operation* d_filter_rest(void);

// iii.  range and slice operations
struct d_filter_operation* d_filter_range(size_t _start, size_t _end);
struct d_filter_operation* d_filter_slice(size_t _start, size_t _end,
                                         size_t _step);

// iv.   predicate-based operations
struct d_filter_operation* d_filter_where(fn_predicate _test);
struct d_filter_operation* d_filter_where_context(fn_predicate _test,
                                              void* _context);
struct d_filter_operation* d_filter_where_not(fn_predicate _test);
struct d_filter_operation* d_filter_where_not_context(fn_predicate _test,
                                                  void* _context);

// v.    index-based operations
struct d_filter_operation* d_filter_at(size_t _index);
struct d_filter_operation* d_filter_at_indices(const size_t* _indices,
                                               size_t _count);

// vi.   transformation operations
struct d_filter_operation* d_filter_distinct(fn_function_comparator _comparator);
struct d_filter_operation* d_filter_reverse(void);

// vii.  operation cleanup
void d_filter_operation_free(struct d_filter_operation* _op);


///////////////////////////////////////////////////////////////////////////////
///             IV.   FILTER CHAIN MANAGEMENT                              ///
///////////////////////////////////////////////////////////////////////////////

// i.    chain creation
struct d_filter_chain* d_filter_chain_new(void);
struct d_filter_chain* d_filter_chain_new_with_capacity(size_t _capacity);
struct d_filter_chain* d_filter_chain_clone(
                           const struct d_filter_chain* _chain);

// ii.   adding operations (generic)
bool d_filter_chain_add(struct d_filter_chain* _chain,
                         const struct d_filter_operation* _op);

// iii.  convenience add helpers
bool d_filter_chain_add_take_first(struct d_filter_chain* _chain,
                                    size_t _n);
bool d_filter_chain_add_take_last(struct d_filter_chain* _chain,
                                   size_t _n);
bool d_filter_chain_add_skip_first(struct d_filter_chain* _chain,
                                    size_t _n);
bool d_filter_chain_add_skip_last(struct d_filter_chain* _chain,
                                   size_t _n);
bool d_filter_chain_add_range(struct d_filter_chain* _chain,
                               size_t _start, size_t _end);
bool d_filter_chain_add_where(struct d_filter_chain* _chain,
                               fn_predicate _test);
bool d_filter_chain_add_where_context(struct d_filter_chain* _chain,
                                   fn_predicate _test, void* _context);

// iv.   combining chains
struct d_filter_chain* d_filter_chain_concat(
                           const struct d_filter_chain* _first,
                           const struct d_filter_chain* _second);
bool d_filter_chain_append(struct d_filter_chain* _dest,
                            const struct d_filter_chain* _source);

// v.    chain manipulation
bool d_filter_chain_insert(struct d_filter_chain* _chain, size_t _index,
                            const struct d_filter_operation* _op);
bool d_filter_chain_remove(struct d_filter_chain* _chain, size_t _index);
void d_filter_chain_clear(struct d_filter_chain* _chain);

// vi.   chain properties
size_t d_filter_chain_length(const struct d_filter_chain* _chain);
bool   d_filter_chain_is_empty(const struct d_filter_chain* _chain);

// vii.  chain cleanup
void d_filter_chain_free(struct d_filter_chain* _chain);


///////////////////////////////////////////////////////////////////////////////
///             V.    FILTER COMBINATORS                                    ///
///////////////////////////////////////////////////////////////////////////////

// struct d_filter_union
//   struct: union of multiple filter chains (OR semantics).
// An element is included if it passes any of the contained chains.
struct d_filter_union
{
    struct d_filter_chain** filters;
    size_t                  count;
    size_t                  capacity;
};

// struct d_filter_intersection
//   struct: intersection of multiple filter chains (AND semantics).
// An element is included only if it passes all contained chains.
struct d_filter_intersection
{
    struct d_filter_chain** filters;
    size_t                  count;
    size_t                  capacity;
};

// struct d_filter_difference
//   struct: difference of two filter chains (A - B).
// An element is included if it passes `include` but not `exclude`.
struct d_filter_difference
{
    struct d_filter_chain* include;
    struct d_filter_chain* exclude;
};

// i.    combinator creation
struct d_filter_union*        d_filter_union_new(size_t _capacity);
struct d_filter_intersection* d_filter_intersection_new(
                                  size_t _capacity);
struct d_filter_difference*   d_filter_difference_new(
                                  struct d_filter_chain* _include,
                                  struct d_filter_chain* _exclude);

// ii.   adding filters to combinators
bool d_filter_union_add(struct d_filter_union* _union,
                         const struct d_filter_chain* _filter);
bool d_filter_intersection_add(
         struct d_filter_intersection* _intersection,
         const struct d_filter_chain* _filter);

// iii.  combinator cleanup
void d_filter_union_free(struct d_filter_union* _union);
void d_filter_intersection_free(
         struct d_filter_intersection* _intersection);
void d_filter_difference_free(
         struct d_filter_difference* _difference);


///////////////////////////////////////////////////////////////////////////////
///             VI.   EXECUTION AND APPLICATION                             ///
///////////////////////////////////////////////////////////////////////////////

// i.    apply single operation
struct d_filter_result* d_filter_apply_operation(
                           const struct d_filter_operation* _op,
                           const void* _input, size_t _count,
                           size_t _element_size);

// ii.   apply filter chain
struct d_filter_result* d_filter_apply_chain(
                           const struct d_filter_chain* _chain,
                           const void* _input, size_t _count,
                           size_t _element_size);

// iii.  apply combinators
struct d_filter_result* d_filter_apply_union(
                           const struct d_filter_union* _union,
                           const void* _input, size_t _count,
                           size_t _element_size);
struct d_filter_result* d_filter_apply_intersection(
                           const struct d_filter_intersection* _inter,
                           const void* _input, size_t _count,
                           size_t _element_size);
struct d_filter_result* d_filter_apply_difference(
                           const struct d_filter_difference* _diff,
                           const void* _input, size_t _count,
                           size_t _element_size);

// iv.   counting and querying
size_t d_filter_count_matches(const struct d_filter_chain* _chain,
                               const void* _input, size_t _count,
                               size_t _element_size);
bool   d_filter_any_match(const struct d_filter_chain* _chain,
                           const void* _input, size_t _count,
                           size_t _element_size);
bool   d_filter_all_match(const struct d_filter_chain* _chain,
                           const void* _input, size_t _count,
                           size_t _element_size);
bool   d_filter_none_match(const struct d_filter_chain* _chain,
                            const void* _input, size_t _count,
                            size_t _element_size);

// v.    index retrieval (no element copying)
size_t* d_filter_get_indices(const struct d_filter_chain* _chain,
                              const void* _input, size_t _count,
                              size_t _element_size,
                              size_t* _out_count);

// vi.   in-place filtering (modifies original array)
size_t d_filter_apply_in_place(const struct d_filter_chain* _chain,
                                void* _array, size_t _count,
                                size_t _element_size);

// vii.  single-element evaluation
bool d_filter_chain_matches_element(
         const struct d_filter_chain* _chain,
         const void* _element,
         size_t _element_size);

// viii. result management
void d_filter_result_free(struct d_filter_result* _result);


///////////////////////////////////////////////////////////////////////////////
///             VII.  UTILITY FUNCTIONS                                     ///
///////////////////////////////////////////////////////////////////////////////

// i.    validation
bool d_filter_operation_is_valid(
         const struct d_filter_operation* _op);
bool d_filter_chain_is_valid(
         const struct d_filter_chain* _chain);

// ii.   description / serialization
char* d_filter_operation_to_string(
          const struct d_filter_operation* _op);
char* d_filter_chain_to_string(
          const struct d_filter_chain* _chain);

// iii.  parsing (from string)
struct d_filter_operation* d_filter_operation_from_string(
                               const char* _str);
struct d_filter_chain*     d_filter_chain_from_string(
                               const char* _str);

// iv.   optimization
struct d_filter_chain* d_filter_chain_optimize(
                           const struct d_filter_chain* _chain);

// v.    statistics
size_t d_filter_estimate_result_size(
           const struct d_filter_chain* _chain,
           size_t _input_count);


///////////////////////////////////////////////////////////////////////////////
///             VIII. ITERATOR INTERFACE                                    ///
///////////////////////////////////////////////////////////////////////////////

// struct d_filter_iterator
//   struct: iterator for lazily evaluating filtered results.
// Precomputes matching indices on creation, then yields elements one
// at a time via next(). The iterator does not own the input data.
struct d_filter_iterator
{
    const void*                  input;         // input array
    size_t                       input_count;   // input element count
    size_t                       element_size;  // size of each element
    const struct d_filter_chain* chain;         // filter chain (ref)
    size_t*                      indices;       // precomputed indices
    size_t                       indices_count; // number of indices
    size_t                       indices_pos;   // current position
    bool                         exhausted;     // no more elements
};

// i.    iterator creation
struct d_filter_iterator* d_filter_iterator_new(
                              const struct d_filter_chain* _chain,
                              const void* _input, size_t _count,
                              size_t _element_size);

// ii.   iterator operations
bool  d_filter_iterator_has_next(
          const struct d_filter_iterator* _iter);
void* d_filter_iterator_next(struct d_filter_iterator* _iter);
void  d_filter_iterator_reset(struct d_filter_iterator* _iter);

// iii.  iterator cleanup
void d_filter_iterator_free(struct d_filter_iterator* _iter);


///////////////////////////////////////////////////////////////////////////////
///             IX.   FLUENT FILTER BUILDER                                 ///
///////////////////////////////////////////////////////////////////////////////

// struct d_filter_builder
//   struct: fluent builder for constructing filter chains.
// Provides method-chaining style API where each operation returns the
// builder. If an error occurs, subsequent operations are no-ops and
// the error is propagated.
struct d_filter_builder
{
    struct d_filter_chain* chain;
    int                    error_code;
    char*                  error_message;
};

// i.    builder creation
struct d_filter_builder* d_filter_builder_new(void);
struct d_filter_builder* d_filter_builder_from_chain(
                             struct d_filter_chain* _chain);

// ii.   fluent operations (return builder for chaining)
struct d_filter_builder* d_filter_builder_take_first(
                             struct d_filter_builder* _builder,
                             size_t _n);
struct d_filter_builder* d_filter_builder_take_last(
                             struct d_filter_builder* _builder,
                             size_t _n);
struct d_filter_builder* d_filter_builder_take_nth(
                             struct d_filter_builder* _builder,
                             size_t _n);
struct d_filter_builder* d_filter_builder_skip_first(
                             struct d_filter_builder* _builder,
                             size_t _n);
struct d_filter_builder* d_filter_builder_skip_last(
                             struct d_filter_builder* _builder,
                             size_t _n);
struct d_filter_builder* d_filter_builder_where(
                             struct d_filter_builder* _builder,
                             fn_predicate _test);
struct d_filter_builder* d_filter_builder_where_context(
                             struct d_filter_builder* _builder,
                             fn_predicate _test, void* _context);
struct d_filter_builder* d_filter_builder_where_not(
                             struct d_filter_builder* _builder,
                             fn_predicate _test);
struct d_filter_builder* d_filter_builder_range(
                             struct d_filter_builder* _builder,
                             size_t _start, size_t _end);
struct d_filter_builder* d_filter_builder_slice(
                             struct d_filter_builder* _builder,
                             size_t _start, size_t _end,
                             size_t _step);
struct d_filter_builder* d_filter_builder_distinct(
                             struct d_filter_builder* _builder,
                             fn_function_comparator _comparator);
struct d_filter_builder* d_filter_builder_reverse(
                             struct d_filter_builder* _builder);
struct d_filter_builder* d_filter_builder_at(
                             struct d_filter_builder* _builder,
                             size_t _index);
struct d_filter_builder* d_filter_builder_at_indices(
                             struct d_filter_builder* _builder,
                             const size_t* _indices, size_t _n);

// iii.  builder finalization
struct d_filter_chain*  d_filter_builder_build(
                            struct d_filter_builder* _builder);
struct d_filter_result* d_filter_builder_apply(
                            struct d_filter_builder* _builder,
                            const void* _input, size_t _count,
                            size_t _element_size);

// iv.   builder error handling
bool  d_filter_builder_has_error(
          const struct d_filter_builder* _builder);
int   d_filter_builder_get_error_code(
          const struct d_filter_builder* _builder);
char* d_filter_builder_get_error_message(
          const struct d_filter_builder* _builder);

// v.    builder cleanup
void d_filter_builder_free(struct d_filter_builder* _builder);


///////////////////////////////////////////////////////////////////////////////
///             X.    CONVENIENCE MACROS                                    ///
///////////////////////////////////////////////////////////////////////////////

// D_FILTER
//   macro: creates and applies a simple filter in one expression.
#define D_FILTER(INPUT, COUNT, SIZE, OP)                             \
    d_filter_apply_operation((OP), (INPUT), (COUNT), (SIZE))

// D_FILTER_CHAIN_BEGIN
//   macro: creates a new filter chain.
#define D_FILTER_CHAIN_BEGIN() d_filter_chain_new()

// D_FILTER_CHAIN_ADD
//   macro: adds an operation to a chain.
#define D_FILTER_CHAIN_ADD(CHAIN, OP)                                \
    d_filter_chain_add((CHAIN), (OP))

// D_FILTER_CHAIN_END
//   macro: identity macro for chain construction readability.
#define D_FILTER_CHAIN_END(CHAIN) (CHAIN)

// D_FILTER_FIRST_N
//   macro: shorthand for taking first n elements.
#define D_FILTER_FIRST_N(INPUT, COUNT, SIZE, N)                      \
    d_filter_apply_operation(                                        \
        &(struct d_filter_operation){                                \
            .type = D_FILTER_OP_TAKE_FIRST,                          \
            .params = { .count = (N) }                               \
        },                                                           \
        (INPUT), (COUNT), (SIZE))

// D_FILTER_LAST_N
//   macro: shorthand for taking last n elements.
#define D_FILTER_LAST_N(INPUT, COUNT, SIZE, N)                       \
    d_filter_apply_operation(                                        \
        &(struct d_filter_operation){                                \
            .type = D_FILTER_OP_TAKE_LAST,                           \
            .params = { .count = (N) }                               \
        },                                                           \
        (INPUT), (COUNT), (SIZE))

// D_FILTER_WHERE
//   macro: shorthand for predicate-based filtering.
#define D_FILTER_WHERE(INPUT, COUNT, SIZE, PRED)                     \
    d_filter_apply_operation(                                        \
        &(struct d_filter_operation){                                \
            .type = D_FILTER_OP_WHERE,                               \
            .params = { .test = (PRED) }                             \
        },                                                           \
        (INPUT), (COUNT), (SIZE))

// D_FILTER_BEGIN
//   macro: starts a fluent filter chain.
#define D_FILTER_BEGIN() d_filter_builder_new()

// D_THEN_TAKE_FIRST
//   macro: chains take_first operation.
#define D_THEN_TAKE_FIRST(BUILDER, N)                                \
    d_filter_builder_take_first((BUILDER), (N))

// D_THEN_TAKE_LAST
//   macro: chains take_last operation.
#define D_THEN_TAKE_LAST(BUILDER, N)                                 \
    d_filter_builder_take_last((BUILDER), (N))

// D_THEN_SKIP_FIRST
//   macro: chains skip_first operation.
#define D_THEN_SKIP_FIRST(BUILDER, N)                                \
    d_filter_builder_skip_first((BUILDER), (N))

// D_THEN_SKIP_LAST
//   macro: chains skip_last operation.
#define D_THEN_SKIP_LAST(BUILDER, N)                                 \
    d_filter_builder_skip_last((BUILDER), (N))

// D_THEN_WHERE
//   macro: chains where (filter) operation.
#define D_THEN_WHERE(BUILDER, PRED)                                  \
    d_filter_builder_where((BUILDER), (PRED))

// D_THEN_WHERE_NOT
//   macro: chains where_not (negated filter) operation.
#define D_THEN_WHERE_NOT(BUILDER, PRED)                              \
    d_filter_builder_where_not((BUILDER), (PRED))

// D_THEN_RANGE
//   macro: chains range operation.
#define D_THEN_RANGE(BUILDER, START, END)                            \
    d_filter_builder_range((BUILDER), (START), (END))

// D_THEN_DISTINCT
//   macro: chains distinct operation.
#define D_THEN_DISTINCT(BUILDER, CMP)                                \
    d_filter_builder_distinct((BUILDER), (CMP))

// D_THEN_REVERSE
//   macro: chains reverse operation.
#define D_THEN_REVERSE(BUILDER)                                      \
    d_filter_builder_reverse((BUILDER))

// D_FILTER_END
//   macro: finalizes and executes filter chain.
#define D_FILTER_END(BUILDER, INPUT, COUNT, SIZE)                    \
    d_filter_builder_apply((BUILDER), (INPUT), (COUNT), (SIZE))


///////////////////////////////////////////////////////////////////////////////
///             XI.   TYPED FILTER WRAPPERS (C11+)                         ///
///////////////////////////////////////////////////////////////////////////////

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)

// D_FILTER_TYPED
//   macro: type-safe filter application using _Generic.
#define D_FILTER_TYPED(INPUT, COUNT, OP)                             \
    _Generic((INPUT),                                                \
        int*:    d_filter_apply_operation(                           \
                     &(OP), (INPUT), (COUNT), sizeof(int)),          \
        double*: d_filter_apply_operation(                           \
                     &(OP), (INPUT), (COUNT), sizeof(double)),       \
        char*:   d_filter_apply_operation(                           \
                     &(OP), (INPUT), (COUNT), sizeof(char)),         \
        default: d_filter_apply_operation(                           \
                     &(OP), (INPUT), (COUNT), sizeof(*(INPUT))))

#endif  // C11+


#endif  // DJINTERP_FILTER_
