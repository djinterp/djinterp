/*******************************************************************************
* djinterp [c][container]                                        vector_common.h
*
*   This module provides common building blocks for vector data structures.
* Vectors are dynamic arrays that maintain both a capacity (allocated storage)
* and a count (number of active elements), allowing for efficient growth and
* shrinkage operations.
*   Functions in this module operate on raw pointers and size values, enabling
* reuse across different vector implementations such as `dj_vector` and
* `d_vector_s`.
*
*
* path:      /inc/djinterp/c/container/vector/vector_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.14
*                                                            revised: 2026.09.29
*******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_VECTOR_VECTOR_COMMON_H
#define DJINTERP_C_CONTAINER_VECTOR_VECTOR_COMMON_H 1

// std
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
// djinterp
#include "../../djinterp.h"
#include "../../memory/dmemory.h"
#include "../container.h"
#include "../array/array_common.h"


#ifndef D_VECTOR_DEFAULT_CAPACITY
    // D_VECTOR_DEFAULT_CAPACITY
    //   definition: the default size, in number of elements, that a new vector
    // has by default.
    #define D_VECTOR_DEFAULT_CAPACITY D_ARRAY_DEFAULT_CAPACITY
#endif  // D_VECTOR_DEFAULT_CAPACITY

// The growth ratio. A vector's capacity grows to
// capacity * D_VECTOR_GROWTH_NUM / D_VECTOR_GROWTH_DEN, in checked integer
// arithmetic. Set the ratio either as those two integers, or as the double
// D_VECTOR_GROWTH_FACTOR, from which they are derived; not both.
#if ( defined(D_VECTOR_GROWTH_FACTOR) &&                                     \
      ( defined(D_VECTOR_GROWTH_NUM) ||                                      \
        defined(D_VECTOR_GROWTH_DEN) ) )
    #error "Set D_VECTOR_GROWTH_FACTOR or D_VECTOR_GROWTH_NUM/_DEN, not both."
#endif

#ifdef D_VECTOR_GROWTH_FACTOR
    // D_INTERNAL_VECTOR_GROWTH_FACTOR_OK
    //   constant (internal): true when D_VECTOR_GROWTH_FACTOR exceeds 1 and
    // its 1024ths, rounded, fit a size_t. It is false for a NaN.
    #define D_INTERNAL_VECTOR_GROWTH_FACTOR_OK                               \
        ( ((double)(D_VECTOR_GROWTH_FACTOR) > 1.0) &&                        \
          ((((double)(D_VECTOR_GROWTH_FACTOR) * 1024.0) + 0.5) <             \
           (double)((size_t)-1)) )

    // D_VECTOR_GROWTH_DEN, D_VECTOR_GROWTH_NUM
    //   constants: the ratio, derived from D_VECTOR_GROWTH_FACTOR in steps of
    // 1/1024: 2.0 is 2048/1024, 1.5 is 1536/1024. The value is folded
    // at compile time, so no floating point runs in an allocation path. The
    // derivation is defined for every factor, so that a compiler which
    // can't refuse a bad one (below) still builds a working vector: a factor
    // of 1 or less grows as 1025/1024, plus the one element every growth
    // adds, and one too large saturates rather than overflowing.
    #define D_VECTOR_GROWTH_DEN 1024
    #define D_VECTOR_GROWTH_NUM                                              \
        ( (!((double)(D_VECTOR_GROWTH_FACTOR) > 1.0))                        \
              ? (size_t)1025                                                 \
              : ( (D_INTERNAL_VECTOR_GROWTH_FACTOR_OK)                       \
                      ? (size_t)(((double)(D_VECTOR_GROWTH_FACTOR) *         \
                                  1024.0) + 0.5)                             \
                      : (size_t)-1 ) )

    // The preprocessor can't compare doubles, so a bad factor is refused by
    // an assertion, where the compiler can fold a floating comparison in one:
    // every C++11 compiler, and GCC and Clang in C, where __extension__ keeps
    // -pedantic from calling the condition non-integral. Elsewhere the
    // derivation above applies.
    #if ( defined(D_ENV_LANG_DETECTED_CPP) && D_ENV_LANG_IS_CPP11_OR_HIGHER )
        static_assert(D_INTERNAL_VECTOR_GROWTH_FACTOR_OK,
            "D_VECTOR_GROWTH_FACTOR must be over 1 and under SIZE_MAX/1024.");
    #elif ( (!defined(D_ENV_LANG_DETECTED_CPP))                &&            \
            ( (defined(__clang__))                             ||            \
              ( (defined(__GNUC__))                            &&            \
                ( (__GNUC__ > 4) ||                                          \
                  ( (__GNUC__ == 4) && (__GNUC_MINOR__ >= 6) ) ) ) ) )
        __extension__ _Static_assert(D_INTERNAL_VECTOR_GROWTH_FACTOR_OK,
            "D_VECTOR_GROWTH_FACTOR must be over 1 and under SIZE_MAX/1024.");
    #endif
#else
    #ifndef D_VECTOR_GROWTH_NUM
        // D_VECTOR_GROWTH_NUM
        //   constant: the growth ratio's numerator: 2/1 doubles a capacity,
        // 3/2 adds half.
        #define D_VECTOR_GROWTH_NUM 2
    #endif  // D_VECTOR_GROWTH_NUM

    #ifndef D_VECTOR_GROWTH_DEN
        // D_VECTOR_GROWTH_DEN
        //   constant: the growth ratio's denominator.
        #define D_VECTOR_GROWTH_DEN 1
    #endif  // D_VECTOR_GROWTH_DEN

    #if ( ((D_VECTOR_GROWTH_DEN) < 1) ||                                     \
          ((D_VECTOR_GROWTH_NUM) <= (D_VECTOR_GROWTH_DEN)) )
        #error "D_VECTOR_GROWTH_NUM / D_VECTOR_GROWTH_DEN must exceed 1."
    #endif

    // D_VECTOR_GROWTH_FACTOR
    //   constant: the growth ratio as a double, for reading only.
    #define D_VECTOR_GROWTH_FACTOR                                           \
        ((double)(D_VECTOR_GROWTH_NUM) / (double)(D_VECTOR_GROWTH_DEN))
#endif  // D_VECTOR_GROWTH_FACTOR

#ifndef D_VECTOR_SHRINK_THRESHOLD
    // D_VECTOR_SHRINK_THRESHOLD
    //   constant: the ratio of count to capacity below which a vector may be
    // considered for shrinking. A value of 0.25 means shrink when less than
    // 25% of capacity is in use.
    #define D_VECTOR_SHRINK_THRESHOLD 0.25
#endif  // D_VECTOR_SHRINK_THRESHOLD

#ifndef D_VECTOR_MIN_CAPACITY
    // D_VECTOR_MIN_CAPACITY
    //   constant: the minimum capacity a vector will shrink to. Prevents
    // excessive reallocation for small vectors.
    #define D_VECTOR_MIN_CAPACITY 4
#endif  // D_VECTOR_MIN_CAPACITY


// initialization functions
// I.    creation and initialization
// I.    creation and initialization
bool d_vector_common_init(
    void**  _elements,
    size_t* _count,
    size_t* _capacity,
    size_t  _element_size,
    size_t  _initial_capacity
);
bool d_vector_common_init_from_array(
    void**      _elements,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    const void* _source,
    size_t      _source_count
);
bool d_vector_common_init_from_args(
    void**  _elements,
    size_t* _count,
    size_t* _capacity,
    size_t  _element_size,
    size_t  _arg_count,
    va_list _args
);
bool d_vector_common_init_copy(
    void**      _elements,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    const void* _source,
    size_t      _source_count,
    size_t      _source_capacity
);
bool d_vector_common_init_fill(
    void**      _elements,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    size_t      _size,
    const void* _value
);

// II.   element access
bool d_vector_common_push_back(
    void**      _elements,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    const void* _value
);
bool d_vector_common_push_front(
    void**      _elements,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    const void* _value
);
bool d_vector_common_pop_back(
    void*   _elements,
    size_t* _count,
    size_t  _element_size,
    void*   _out_value
);
bool d_vector_common_pop_front(
    void*   _elements,
    size_t* _count,
    size_t  _element_size,
    void*   _out_value
);
void* d_vector_common_at(
    const void* _elements,
    size_t      _count,
    size_t      _element_size,
    dj_index    _index
);
void* d_vector_common_front(
    const void* _elements,
    size_t      _count
);
void* d_vector_common_back(
    const void* _elements,
    size_t      _count,
    size_t      _element_size
);
void* d_vector_common_data(
    const void* _elements
);

// III.  insertion and removal
bool d_vector_common_insert_element(
    void**      _destination,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    dj_index    _index,
    const void* _value
);
bool d_vector_common_insert_elements(
    void**      _destination,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    dj_index    _index,
    const void* _source,
    size_t      _source_count
);
bool d_vector_common_erase(
    void*    _destination,
    size_t*  _count,
    size_t   _element_size,
    dj_index _index
);
bool d_vector_common_erase_range(
    void*    _destination,
    size_t*  _count,
    size_t   _element_size,
    dj_index _start,
    dj_index _end
);
bool d_vector_common_append_element(
    void**      _destination,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    const void* _element
);
bool d_vector_common_append_elements(
    void**      _destination,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    const void* _source,
    size_t      _source_count
);
bool d_vector_common_prepend_element(
    void**      _destination,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    const void* _element
);
bool d_vector_common_prepend_elements(
    void**      _destination,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    const void* _source,
    size_t      _source_count
);

// IV.   capacity and state queries
bool d_vector_common_ensure_capacity(
    void**  _elements,
    size_t  _count,
    size_t* _capacity,
    size_t  _element_size,
    size_t  _required
);
bool d_vector_common_is_empty(
    size_t _count
);
bool d_vector_common_is_full(
    size_t _count,
    size_t _capacity
);
size_t d_vector_common_size(
    size_t _count
);
size_t d_vector_common_capacity(
    size_t _capacity
);

// V.    mutation and reordering
bool d_vector_common_reserve(
    void**  _elements,
    size_t  _count,
    size_t* _capacity,
    size_t  _element_size,
    size_t  _new_capacity
);
size_t d_vector_common_next_capacity(
    size_t _current,
    size_t _element_size
);
bool d_vector_common_grow(
    void**  _elements,
    size_t  _count,
    size_t* _capacity,
    size_t  _element_size
);
bool d_vector_common_maybe_shrink(
    void**  _elements,
    size_t  _count,
    size_t* _capacity,
    size_t  _element_size
);
bool d_vector_common_resize(
    void**  _elements,
    size_t* _count,
    size_t* _capacity,
    size_t  _element_size,
    size_t  _new_count
);
bool d_vector_common_resize_fill(
    void**      _elements,
    size_t*     _count,
    size_t*     _capacity,
    size_t      _element_size,
    size_t      _new_count,
    const void* _fill_value
);
bool d_vector_common_swap(
    void*    _elements,
    size_t   _count,
    size_t   _element_size,
    dj_index _index_a,
    dj_index _index_b
);

// VI.   conversion and reporting
bool d_vector_common_shrink_to_fit(
    void**  _elements,
    size_t  _count,
    size_t* _capacity,
    size_t  _element_size
);
bool d_vector_common_copy_to(
    const void* _source,
    size_t      _count,
    size_t      _element_size,
    void*       _destination,
    size_t      _dest_capacity
);

// VII.  operations
size_t d_vector_common_available(
    size_t _count,
    size_t _capacity
);

// VIII. destruction
void d_vector_common_free_elements(
    void* _elements
);

#endif  // DJINTERP_C_CONTAINER_VECTOR_VECTOR_COMMON_H
