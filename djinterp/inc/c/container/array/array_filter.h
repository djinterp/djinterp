/******************************************************************************
* djinterp [container]                                          array_filter.h
*
* Zero-overhead filter functionality for array-based containers.
*   Provides thin wrappers around the functional module's filter.h that are
* tailored for contiguous, array-based data structures (d_array, d_vector,
* raw C arrays). All operations are gated behind compile-time configuration
* toggles so that disabled functionality produces zero code.
*
* CONFIGURATION:
*   Per-array-type toggles are resolved in array_config.h, which this header
* includes.  The toggle checked here is D_CFG_CONTAINER_FILTER_ARRAY, whose
* inheritance chain is:
*       D_CFG_FILTER -> D_CFG_CONTAINER_FILTER ->
*       D_CFG_CONTAINER_FILTER_CONTIGUOUS -> D_CFG_CONTAINER_FILTER_ARRAY.
*
*   Include this header from a concrete array container header (e.g.
* array.h, ptr_array.h) to gain filter support for that container.  Each
* concrete container checks its own per-type toggle from array_config.h.
*
* ZERO-OVERHEAD GUARANTEE:
*   When D_CFG_CONTAINER_FILTER_ARRAY is 0, this header expands to nothing
* beyond its include guard.  No structs, no function declarations, no macros.
*
* DESIGN NOTES:
*   - All functions accept raw (void*, count, element_size) triples so that
*     any contiguous container can use them.
*   - Higher-level typed wrappers are provided as macros that infer
*     element_size from the container struct.
*   - Filter chains and results use the types from filter.h directly;
*     array_filter.h does not duplicate them.
*   - Predicate logic comes from predicate.h via fn_predicate.
*
*
* path:      \inc\container\array\array_filter.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.19
******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_ARRAY_FILTER_
#define DJINTERP_C_CONTAINER_ARRAY_FILTER_ 1

#include <stdlib.h>
#include "..\..\djinterp.h"
#include "..\..\dmemory.h"
#include "..\..\functional\filter.h"
#include "..\..\core\config\container\array\array_config.h"


///////////////////////////////////////////////////////////////////////////////
///             I.    ARRAY FILTER RESULT                                   ///
///////////////////////////////////////////////////////////////////////////////

// d_array_filter_result
//   struct: result of an array filter operation.
// Wraps the generic d_filter_result with array-specific metadata.
// The filtered data is a contiguous block owned by this result.
struct d_array_filter_result
{
    void*                     data;           // filtered elements (owned)
    size_t                    count;          // number of filtered elements
    size_t                    element_size;   // size of each element
    size_t*                   source_indices; // original indices (may be NULL)
    enum d_filter_result_type status;         // operation status
};


///////////////////////////////////////////////////////////////////////////////
///             II.   SINGLE-OPERATION FILTER FUNCTIONS                     ///
///////////////////////////////////////////////////////////////////////////////

// i.    take operations
struct d_array_filter_result d_array_filter_take_first(const void* _elements, size_t _count, size_t _element_size, size_t _n);
struct d_array_filter_result d_array_filter_take_last(const void* _elements, size_t _count, size_t _element_size, size_t _n);
struct d_array_filter_result d_array_filter_take_nth(const void* _elements, size_t _count, size_t _element_size, size_t _n);
struct d_array_filter_result d_array_filter_head(const void* _elements, size_t _count, size_t _element_size);
struct d_array_filter_result d_array_filter_tail(const void* _elements, size_t _count, size_t _element_size);

// ii.   skip operations
struct d_array_filter_result d_array_filter_skip_first(const void* _elements, size_t _count, size_t _element_size, size_t _n);
struct d_array_filter_result d_array_filter_skip_last(const void* _elements, size_t _count, size_t _element_size, size_t _n);
struct d_array_filter_result d_array_filter_init(const void* _elements, size_t _count, size_t _element_size);
struct d_array_filter_result d_array_filter_rest(const void* _elements, size_t _count, size_t _element_size);

// iii.  range and slice operations
struct d_array_filter_result d_array_filter_range(const void* _elements, size_t _count, size_t _element_size, size_t _start, size_t _end);
struct d_array_filter_result d_array_filter_slice(const void* _elements, size_t _count, size_t _element_size, size_t _start, size_t _end, size_t _step);

// iv.   predicate-based operations
struct d_array_filter_result d_array_filter_where(const void* _elements, size_t _count, size_t _element_size, fn_predicate _test, void* _context);
struct d_array_filter_result d_array_filter_where_not(const void* _elements, size_t _count, size_t _element_size, fn_predicate _test, void* _context);

// v.    index-based operations
struct d_array_filter_result d_array_filter_at_indices(const void* _elements, size_t _count, size_t _element_size, const size_t* _indices, size_t _index_count);

// vi.   transformation operations
struct d_array_filter_result d_array_filter_distinct(const void* _elements, size_t _count, size_t _element_size, fn_function_comparator _comparator);
struct d_array_filter_result d_array_filter_reverse(const void* _elements, size_t _count, size_t _element_size);


///////////////////////////////////////////////////////////////////////////////
///             III.  IN-PLACE FILTER OPERATIONS                           ///
///////////////////////////////////////////////////////////////////////////////
// These modify the source array directly, returning the new count.
// Useful when the caller owns the data and wants to avoid allocation.

// i.    in-place predicate filter
size_t d_array_filter_in_place(void* _elements, size_t _count, size_t _element_size, fn_predicate _test, void* _context);
size_t d_array_filter_in_place_not(void* _elements, size_t _count, size_t _element_size, fn_predicate _test, void* _context);

// ii.   in-place positional operations
size_t d_array_filter_in_place_take_first(void* _elements, size_t _count, size_t _element_size, size_t _n);
size_t d_array_filter_in_place_skip_first(void* _elements, size_t _count, size_t _element_size, size_t _n);
size_t d_array_filter_in_place_distinct(void* _elements, size_t _count, size_t _element_size, fn_function_comparator _comparator);


///////////////////////////////////////////////////////////////////////////////
///             IV.   CHAIN AND COMBINATOR APPLICATION                      ///
///////////////////////////////////////////////////////////////////////////////

// i.    chain application
struct d_array_filter_result d_array_filter_apply_chain(const void* _elements, size_t _count, size_t _element_size, const struct d_filter_chain* _chain);

// ii.   combinator application
struct d_array_filter_result d_array_filter_apply_union(const void* _elements, size_t _count, size_t _element_size, const struct d_filter_union* _combo, fn_function_comparator _comparator);
struct d_array_filter_result d_array_filter_apply_intersection(const void* _elements, size_t _count, size_t _element_size, const struct d_filter_intersection* _combo, fn_function_comparator _comparator);
struct d_array_filter_result d_array_filter_apply_difference(const void* _elements, size_t _count, size_t _element_size, const struct d_filter_difference* _diff, fn_function_comparator _comparator);


///////////////////////////////////////////////////////////////////////////////
///             V.    QUERY FUNCTIONS                                       ///
///////////////////////////////////////////////////////////////////////////////

// i.    counting
size_t   d_array_filter_count_where(const void* _elements, size_t _count, size_t _element_size, fn_predicate _test, void* _context);

// ii.   existence
bool     d_array_filter_any_match(const void* _elements, size_t _count, size_t _element_size, fn_predicate _test, void* _context);
bool     d_array_filter_all_match(const void* _elements, size_t _count, size_t _element_size, fn_predicate _test, void* _context);
bool     d_array_filter_none_match(const void* _elements, size_t _count, size_t _element_size, fn_predicate _test, void* _context);

// iii.  search
void*    d_array_filter_find_first(const void* _elements, size_t _count, size_t _element_size, fn_predicate _test, void* _context);
void*    d_array_filter_find_last(const void* _elements, size_t _count, size_t _element_size, fn_predicate _test, void* _context);


///////////////////////////////////////////////////////////////////////////////
///             VI.   RESULT MANAGEMENT                                     ///
///////////////////////////////////////////////////////////////////////////////

// i.    result access
void*  d_array_filter_result_data(const struct d_array_filter_result* _result);
size_t d_array_filter_result_count(const struct d_array_filter_result* _result);
bool   d_array_filter_result_ok(const struct d_array_filter_result* _result);

// ii.   result ownership transfer
void*  d_array_filter_result_release(struct d_array_filter_result* _result, size_t* _out_count);

// iii.  result cleanup
void   d_array_filter_result_free(struct d_array_filter_result* _result);


///////////////////////////////////////////////////////////////////////////////
///             VII.  CONVENIENCE MACROS                                    ///
///////////////////////////////////////////////////////////////////////////////

// D_ARRAY_FILTER_WHERE
//   macro: filter a raw array by predicate.
// _arr must be a pointer to contiguous elements; _type is the element type.
#define D_ARRAY_FILTER_WHERE(type,                                          \
                             arr,                                           \
                             count,                                         \
                             predicate)                                     \
    d_array_filter_where((arr),                                             \
                         (count),                                           \
                         sizeof(type),                                      \
                         (predicate),                                       \
                         NULL)

// D_ARRAY_FILTER_WHERE_CTX
//   macro: filter with an explicit context pointer.
#define D_ARRAY_FILTER_WHERE_CTX(type,                                      \
                                 arr,                                       \
                                 count,                                     \
                                 predicate,                                 \
                                 ctx)                                       \
    d_array_filter_where((arr),                                             \
                         (count),                                           \
                         sizeof(type),                                      \
                         (predicate),                                       \
                         (ctx))

// D_ARRAY_FILTER_FIRST_N
//   macro: take the first _n elements of a typed array.
#define D_ARRAY_FILTER_FIRST_N(type,                                        \
                               arr,                                         \
                               count,                                       \
                               n)                                           \
    d_array_filter_take_first((arr),                                        \
                              (count),                                      \
                              sizeof(type),                                 \
                              (n))

// D_ARRAY_FILTER_LAST_N
//   macro: take the last _n elements of a typed array.
#define D_ARRAY_FILTER_LAST_N(type,                                         \
                              arr,                                          \
                              count,                                        \
                              n)                                            \
    d_array_filter_take_last((arr),                                         \
                             (count),                                       \
                             sizeof(type),                                  \
                             (n))

// D_ARRAY_FILTER_RANGE
//   macro: select elements in the half-open range [start, end).
#define D_ARRAY_FILTER_RANGE(type,                                          \
                             arr,                                           \
                             count,                                         \
                             start,                                         \
                             end)                                           \
    d_array_filter_range((arr),                                             \
                         (count),                                           \
                         sizeof(type),                                      \
                         (start),                                           \
                         (end))

// D_ARRAY_FILTER_SLICE
//   macro: select elements with [start:end:step] semantics.
#define D_ARRAY_FILTER_SLICE(type,                                          \
                             arr,                                           \
                             count,                                         \
                             start,                                         \
                             end,                                           \
                             step)                                          \
    d_array_filter_slice((arr),                                             \
                         (count),                                           \
                         sizeof(type),                                      \
                         (start),                                           \
                         (end),                                             \
                         (step))

// D_ARRAY_FILTER_DISTINCT
//   macro: remove duplicates from a typed array.
#define D_ARRAY_FILTER_DISTINCT(type,                                       \
                                arr,                                        \
                                count,                                      \
                                cmp)                                        \
    d_array_filter_distinct((arr),                                          \
                            (count),                                        \
                            sizeof(type),                                   \
                            (cmp))

// D_ARRAY_FILTER_IN_PLACE
//   macro: filter a typed array in-place by predicate.  Returns new count.
#define D_ARRAY_FILTER_IN_PLACE(type,                                       \
                                arr,                                        \
                                count,                                      \
                                predicate)                                  \
    d_array_filter_in_place((arr),                                          \
                            (count),                                        \
                            sizeof(type),                                   \
                            (predicate),                                    \
                            NULL)


///////////////////////////////////////////////////////////////////////////////
///             VIII. FLUENT BUILDER HELPERS                                ///
///////////////////////////////////////////////////////////////////////////////

// D_ARRAY_FILTER_BEGIN
//   macro: starts a fluent filter builder for an array.
#define D_ARRAY_FILTER_BEGIN() d_filter_builder_new()

// D_ARRAY_FILTER_END
//   macro: finalizes a fluent builder and applies it to a typed array.
#define D_ARRAY_FILTER_END(builder,                                         \
                           type,                                            \
                           arr,                                             \
                           count)                                           \
    d_array_filter_apply_builder((builder),                                 \
                                 (arr),                                     \
                                 (count),                                   \
                                 sizeof(type))

// d_array_filter_apply_builder
//   function: applies a filter builder to array data and returns an
// array_filter_result.  Bridges the fluent builder API to array semantics.
struct d_array_filter_result d_array_filter_apply_builder(struct d_filter_builder* _builder, const void* _elements, size_t _count, size_t _element_size);


#endif  // DJINTERP_C_CONTAINER_ARRAY_FILTER_