/******************************************************************************
* djinterp [container]                                    contiguous_filter.h
*
* Zero-overhead filter functionality for contiguous containers.
*   Provides thin wrappers around the functional module's filter.h that are
* tailored for contiguous data structures (d_array, d_vector, raw C arrays).
* All operations are gated behind compile-time configuration toggles so that
* disabled functionality produces zero code.
*
* CONFIGURATION:
*   The toggle checked here is D_CFG_CONTAINER_ARRAY_FILTER, whose
* inheritance chain is:
*       D_CFG_FILTER -> D_CFG_CONTAINER_FILTER ->
*       D_CFG_CONTAINER_ARRAY_FILTER.
*
*   Include this header from a concrete container header (e.g.
* array.h, vector.h) to gain filter support for that container.
*
* ZERO-OVERHEAD GUARANTEE:
*   When D_CFG_CONTAINER_ARRAY_FILTER is 0, this header expands to nothing
* beyond its include guard.  No structs, no function declarations, no macros.
*
* DESIGN NOTES:
*   - All functions accept raw (void*, count, element_size) triples so that
*     any contiguous container can use them.
*   - Higher-level typed wrappers are provided as macros that infer
*     element_size from the container struct.
*   - Filter chains and results use the types from filter.h directly;
*     contiguous_filter.h does not duplicate them.
*   - Predicate logic comes from predicate.h via fn_predicate.
*
*
* path:      \inc\container\contiguous_filter.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.19
******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_CONTIGUOUS_FILTER_
#define DJINTERP_C_CONTAINER_CONTIGUOUS_FILTER_ 1

#include "..\..\djinterp.h"
#include "..\..\dmemory.h"
#include "..\..\functional\filter.h"


///////////////////////////////////////////////////////////////////////////////
///             I.    CONTIGUOUS FILTER RESULT                                   ///
///////////////////////////////////////////////////////////////////////////////

// d_contiguous_filter_result
//   struct: result of a contiguous filter operation.
// Wraps the generic d_filter_result with container-specific metadata.
// The filtered data is a contiguous block owned by this result.
struct d_contiguous_filter_result
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
struct d_contiguous_filter_result d_contiguous_filter_take_first(const void* _elements, size_t _count, size_t _element_size, size_t _n);
struct d_contiguous_filter_result d_contiguous_filter_take_last(const void* _elements, size_t _count, size_t _element_size, size_t _n);
struct d_contiguous_filter_result d_contiguous_filter_take_nth(const void* _elements, size_t _count, size_t _element_size, size_t _n);
struct d_contiguous_filter_result d_contiguous_filter_head(const void* _elements, size_t _count, size_t _element_size);
struct d_contiguous_filter_result d_contiguous_filter_tail(const void* _elements, size_t _count, size_t _element_size);

// ii.   skip operations
struct d_contiguous_filter_result d_contiguous_filter_skip_first(const void* _elements, size_t _count, size_t _element_size, size_t _n);
struct d_contiguous_filter_result d_contiguous_filter_skip_last(const void* _elements, size_t _count, size_t _element_size, size_t _n);
struct d_contiguous_filter_result d_contiguous_filter_init(const void* _elements, size_t _count, size_t _element_size);
struct d_contiguous_filter_result d_contiguous_filter_rest(const void* _elements, size_t _count, size_t _element_size);

// iii.  range and slice operations
struct d_contiguous_filter_result d_contiguous_filter_range(const void* _elements, size_t _count, size_t _element_size, size_t _start, size_t _end);
struct d_contiguous_filter_result d_contiguous_filter_slice(const void* _elements, size_t _count, size_t _element_size, size_t _start, size_t _end, size_t _step);

// iv.   predicate-based operations
struct d_contiguous_filter_result d_contiguous_filter_where(const void* _elements, size_t _count, size_t _element_size, fn_predicate _test, void* _context);
struct d_contiguous_filter_result d_contiguous_filter_where_not(const void* _elements, size_t _count, size_t _element_size, fn_predicate _test, void* _context);

// v.    index-based operations
struct d_contiguous_filter_result d_contiguous_filter_at_indices(const void* _elements, size_t _count, size_t _element_size, const size_t* _indices, size_t _index_count);

// vi.   transformation operations
struct d_contiguous_filter_result d_contiguous_filter_distinct(const void* _elements, size_t _count, size_t _element_size, fn_function_comparator _comparator);
struct d_contiguous_filter_result d_contiguous_filter_reverse(const void* _elements, size_t _count, size_t _element_size);


///////////////////////////////////////////////////////////////////////////////
///             III.  IN-PLACE FILTER OPERATIONS                           ///
///////////////////////////////////////////////////////////////////////////////

// i.    in-place predicate filter
size_t d_contiguous_filter_in_place(void* _elements, size_t _count, size_t _element_size, fn_predicate _test, void* _context);
size_t d_contiguous_filter_in_place_not(void* _elements, size_t _count, size_t _element_size, fn_predicate _test, void* _context);

// ii.   in-place positional operations
size_t d_contiguous_filter_in_place_take_first(void* _elements, size_t _count, size_t _element_size, size_t _n);
size_t d_contiguous_filter_in_place_skip_first(void* _elements, size_t _count, size_t _element_size, size_t _n);
size_t d_contiguous_filter_in_place_distinct(void* _elements, size_t _count, size_t _element_size, fn_function_comparator _comparator);


///////////////////////////////////////////////////////////////////////////////
///             IV.   CHAIN AND COMBINATOR APPLICATION                      ///
///////////////////////////////////////////////////////////////////////////////

// i.    chain application
struct d_contiguous_filter_result d_contiguous_filter_apply_chain(const void* _elements, size_t _count, size_t _element_size, const struct d_filter_chain* _chain);

// ii.   combinator application
struct d_contiguous_filter_result d_contiguous_filter_apply_union(const void* _elements, size_t _count, size_t _element_size, const struct d_filter_union* _combo, fn_function_comparator _comparator);
struct d_contiguous_filter_result d_contiguous_filter_apply_intersection(const void* _elements, size_t _count, size_t _element_size, const struct d_filter_intersection* _combo, fn_function_comparator _comparator);
struct d_contiguous_filter_result d_contiguous_filter_apply_difference(const void* _elements, size_t _count, size_t _element_size, const struct d_filter_difference* _diff, fn_function_comparator _comparator);


///////////////////////////////////////////////////////////////////////////////
///             V.    QUERY FUNCTIONS                                       ///
///////////////////////////////////////////////////////////////////////////////

// i.    counting
size_t d_contiguous_filter_count_where(const void* _elements, size_t _count, size_t _element_size, fn_predicate _test, void* _context);

// ii.   existence
bool   d_contiguous_filter_any_match(const void* _elements, size_t _count, size_t _element_size, fn_predicate _test, void* _context);
bool   d_contiguous_filter_all_match(const void* _elements, size_t _count, size_t _element_size, fn_predicate _test, void* _context);
bool   d_contiguous_filter_none_match(const void* _elements, size_t _count, size_t _element_size, fn_predicate _test, void* _context);

// iii.  search
void*  d_contiguous_filter_find_first(const void* _elements, size_t _count, size_t _element_size, fn_predicate _test, void* _context);
void*  d_contiguous_filter_find_last(const void* _elements, size_t _count, size_t _element_size, fn_predicate _test, void* _context);


///////////////////////////////////////////////////////////////////////////////
///             VI.   RESULT MANAGEMENT                                     ///
///////////////////////////////////////////////////////////////////////////////

// i.    result access
void*  d_contiguous_filter_result_data(const struct d_contiguous_filter_result* _result);
size_t d_contiguous_filter_result_count(const struct d_contiguous_filter_result* _result);
bool   d_contiguous_filter_result_ok(const struct d_contiguous_filter_result* _result);

// ii.   result ownership transfer
void*  d_contiguous_filter_result_release(struct d_contiguous_filter_result* _result, size_t* _out_count);

// iii.  result cleanup
void   d_contiguous_filter_result_free(struct d_contiguous_filter_result* _result);


///////////////////////////////////////////////////////////////////////////////
///             VII.  CONVENIENCE MACROS                                    ///
///////////////////////////////////////////////////////////////////////////////

#define D_CONTIGUOUS_FILTER_WHERE(type,                                          \
                             arr,                                           \
                             count,                                         \
                             predicate)                                     \
    d_contiguous_filter_where((arr),                                             \
                         (count),                                           \
                         sizeof(type),                                      \
                         (predicate),                                       \
                         NULL)

#define D_CONTIGUOUS_FILTER_WHERE_CTX(type,                                      \
                                 arr,                                       \
                                 count,                                     \
                                 predicate,                                 \
                                 ctx)                                       \
    d_contiguous_filter_where((arr),                                             \
                         (count),                                           \
                         sizeof(type),                                      \
                         (predicate),                                       \
                         (ctx))

#define D_CONTIGUOUS_FILTER_FIRST_N(type,                                        \
                               arr,                                         \
                               count,                                       \
                               n)                                           \
    d_contiguous_filter_take_first((arr),                                        \
                              (count),                                      \
                              sizeof(type),                                 \
                              (n))

#define D_CONTIGUOUS_FILTER_LAST_N(type,                                         \
                              arr,                                          \
                              count,                                        \
                              n)                                            \
    d_contiguous_filter_take_last((arr),                                         \
                             (count),                                       \
                             sizeof(type),                                  \
                             (n))

#define D_CONTIGUOUS_FILTER_RANGE(type,                                          \
                             arr,                                           \
                             count,                                         \
                             start,                                         \
                             end)                                           \
    d_contiguous_filter_range((arr),                                             \
                         (count),                                           \
                         sizeof(type),                                      \
                         (start),                                           \
                         (end))

#define D_CONTIGUOUS_FILTER_SLICE(type,                                          \
                             arr,                                           \
                             count,                                         \
                             start,                                         \
                             end,                                           \
                             step)                                          \
    d_contiguous_filter_slice((arr),                                             \
                         (count),                                           \
                         sizeof(type),                                      \
                         (start),                                           \
                         (end),                                             \
                         (step))

#define D_CONTIGUOUS_FILTER_DISTINCT(type,                                       \
                                arr,                                        \
                                count,                                      \
                                cmp)                                        \
    d_contiguous_filter_distinct((arr),                                          \
                            (count),                                        \
                            sizeof(type),                                   \
                            (cmp))

#define D_CONTIGUOUS_FILTER_IN_PLACE(type,                                       \
                                arr,                                        \
                                count,                                      \
                                predicate)                                  \
    d_contiguous_filter_in_place((arr),                                          \
                            (count),                                        \
                            sizeof(type),                                   \
                            (predicate),                                    \
                            NULL)


///////////////////////////////////////////////////////////////////////////////
///             VIII. FLUENT BUILDER HELPERS                                ///
///////////////////////////////////////////////////////////////////////////////

#define D_CONTIGUOUS_FILTER_BEGIN() d_filter_builder_new()

#define D_CONTIGUOUS_FILTER_END(builder,                                         \
                           type,                                            \
                           arr,                                             \
                           count)                                           \
    d_contiguous_filter_apply_builder((builder),                                 \
                                 (arr),                                     \
                                 (count),                                   \
                                 sizeof(type))

struct d_contiguous_filter_result d_contiguous_filter_apply_builder(struct d_filter_builder* _builder, const void* _elements, size_t _count, size_t _element_size);


#endif  // DJINTERP_C_CONTAINER_CONTIGUOUS_FILTER_
