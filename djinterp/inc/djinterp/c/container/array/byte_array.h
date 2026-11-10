/*******************************************************************************
* djinterp [c]                                                      byte_array.h
*
*  Macros for building byte arrays with counted/sized elements at compile
* time.
*
*
* path:      /inc/djinterp/c/container/array/byte_array.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2025.12.06
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_C_CONTAINER_ARRAY_BYTE_ARRAY_H
#define DJINTERP_C_CONTAINER_ARRAY_BYTE_ARRAY_H 1

// std
#include <stddef.h>
// djinterp
#include "../../djinterp.h"
#include "../../dmacro.h"  // D_VARG_COUNT, D_EVAL, D_UNPACK
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // uint8_t


// endian-aware byte extraction macros
#if D_ENV_ARCH_IS_BIG_ENDIAN

    // D_INTERNAL_BYTE_0_OF_1
    //   macro: get byte 0 from 1-byte value
    #define D_INTERNAL_BYTE_0_OF_1(x)  ((uint8_t)((x) & 0xFFu))

    // D_INTERNAL_BYTE_0_OF_2
    //   macro: get high-order byte from 2-byte value
    #define D_INTERNAL_BYTE_0_OF_2(x)  ((uint8_t)(((x) >> 8)  & 0xFFu))

    // D_INTERNAL_BYTE_1_OF_2
    //   macro: get low-order byte from 2-byte value
    #define D_INTERNAL_BYTE_1_OF_2(x)  ((uint8_t)((x)         & 0xFFu))

    // D_INTERNAL_BYTE_0_OF_4
    //   macro: get highest-order byte from 4-byte value
    #define D_INTERNAL_BYTE_0_OF_4(x)  ((uint8_t)(((x) >> 24) & 0xFFu))

    // D_INTERNAL_BYTE_1_OF_4
    //   macro: get 2nd-highest byte from 4-byte value
    #define D_INTERNAL_BYTE_1_OF_4(x)  ((uint8_t)(((x) >> 16) & 0xFFu))

    // D_INTERNAL_BYTE_2_OF_4
    //   macro: get 3rd-highest byte from 4-byte value
    #define D_INTERNAL_BYTE_2_OF_4(x)  ((uint8_t)(((x) >> 8)  & 0xFFu))

    // D_INTERNAL_BYTE_3_OF_4
    //   macro: get lowest-order byte from 4-byte value
    #define D_INTERNAL_BYTE_3_OF_4(x)  ((uint8_t)((x)         & 0xFFu))

    // D_INTERNAL_BYTE_0_OF_8
    //   macro: get highest-order byte from 8-byte value
    #define D_INTERNAL_BYTE_0_OF_8(x) \
        ((uint8_t)(((uint64_t)(x) >> 56) & 0xFFu))

    // D_INTERNAL_BYTE_1_OF_8
    //   macro: get 2nd-highest byte from 8-byte value
    #define D_INTERNAL_BYTE_1_OF_8(x) \
        ((uint8_t)(((uint64_t)(x) >> 48) & 0xFFu))

    // D_INTERNAL_BYTE_2_OF_8
    //   macro: get 3rd-highest byte from 8-byte value
    #define D_INTERNAL_BYTE_2_OF_8(x) \
        ((uint8_t)(((uint64_t)(x) >> 40) & 0xFFu))

    // D_INTERNAL_BYTE_3_OF_8
    //   macro: get 4th-highest byte from 8-byte value
    #define D_INTERNAL_BYTE_3_OF_8(x) \
        ((uint8_t)(((uint64_t)(x) >> 32) & 0xFFu))

    // D_INTERNAL_BYTE_4_OF_8
    //   macro: get 5th-highest byte from 8-byte value
    #define D_INTERNAL_BYTE_4_OF_8(x) \
        ((uint8_t)(((uint64_t)(x) >> 24) & 0xFFu))

    // D_INTERNAL_BYTE_5_OF_8
    //   macro: get 6th-highest byte from 8-byte value
    #define D_INTERNAL_BYTE_5_OF_8(x) \
        ((uint8_t)(((uint64_t)(x) >> 16) & 0xFFu))

    // D_INTERNAL_BYTE_6_OF_8
    //   macro: get 7th-highest byte from 8-byte value
    #define D_INTERNAL_BYTE_6_OF_8(x) \
        ((uint8_t)(((uint64_t)(x) >> 8) & 0xFFu))

    // D_INTERNAL_BYTE_7_OF_8
    //   macro: get lowest-order byte from 8-byte value
    #define D_INTERNAL_BYTE_7_OF_8(x) \
        ((uint8_t)((uint64_t)(x) & 0xFFu))

#else   // little-endian (default)

    // D_INTERNAL_BYTE_0_OF_1
    //   macro: get byte 0 from 1-byte value
    #define D_INTERNAL_BYTE_0_OF_1(x)  ((uint8_t)((x) & 0xFFu))

    // D_INTERNAL_BYTE_0_OF_2
    //   macro: get lowest-order byte from 2-byte value
    #define D_INTERNAL_BYTE_0_OF_2(x)  ((uint8_t)((x)         & 0xFFu))

    // D_INTERNAL_BYTE_1_OF_2
    //   macro: get high-order byte from 2-byte value
    #define D_INTERNAL_BYTE_1_OF_2(x)  ((uint8_t)(((x) >> 8)  & 0xFFu))

    // D_INTERNAL_BYTE_0_OF_4
    //   macro: get lowest-order byte from 4-byte value
    #define D_INTERNAL_BYTE_0_OF_4(x)  ((uint8_t)((x)         & 0xFFu))

    // D_INTERNAL_BYTE_1_OF_4
    //   macro: get 2nd byte from 4-byte value
    #define D_INTERNAL_BYTE_1_OF_4(x)  ((uint8_t)(((x) >> 8)  & 0xFFu))

    // D_INTERNAL_BYTE_2_OF_4
    //   macro: get 3rd byte from 4-byte value
    #define D_INTERNAL_BYTE_2_OF_4(x)  ((uint8_t)(((x) >> 16) & 0xFFu))

    // D_INTERNAL_BYTE_3_OF_4
    //   macro: get highest-order byte from 4-byte value
    #define D_INTERNAL_BYTE_3_OF_4(x)  ((uint8_t)(((x) >> 24) & 0xFFu))

    // D_INTERNAL_BYTE_0_OF_8
    //   macro: get lowest-order byte from 8-byte value
    #define D_INTERNAL_BYTE_0_OF_8(x) \
        ((uint8_t)((uint64_t)(x) & 0xFFu))

    // D_INTERNAL_BYTE_1_OF_8
    //   macro: get 2nd byte from 8-byte value
    #define D_INTERNAL_BYTE_1_OF_8(x) \
        ((uint8_t)(((uint64_t)(x) >> 8) & 0xFFu))

    // D_INTERNAL_BYTE_2_OF_8
    //   macro: get 3rd byte from 8-byte value
    #define D_INTERNAL_BYTE_2_OF_8(x) \
        ((uint8_t)(((uint64_t)(x) >> 16) & 0xFFu))

    // D_INTERNAL_BYTE_3_OF_8
    //   macro: get 4th byte from 8-byte value
    #define D_INTERNAL_BYTE_3_OF_8(x) \
        ((uint8_t)(((uint64_t)(x) >> 24) & 0xFFu))

    // D_INTERNAL_BYTE_4_OF_8
    //   macro: get 5th byte from 8-byte value
    #define D_INTERNAL_BYTE_4_OF_8(x) \
        ((uint8_t)(((uint64_t)(x) >> 32) & 0xFFu))

    // D_INTERNAL_BYTE_5_OF_8
    //   macro: get 6th byte from 8-byte value
    #define D_INTERNAL_BYTE_5_OF_8(x) \
        ((uint8_t)(((uint64_t)(x) >> 40) & 0xFFu))

    // D_INTERNAL_BYTE_6_OF_8
    //   macro: get 7th byte from 8-byte value
    #define D_INTERNAL_BYTE_6_OF_8(x) \
        ((uint8_t)(((uint64_t)(x) >> 48) & 0xFFu))

    // D_INTERNAL_BYTE_7_OF_8
    //   macro: get highest-order byte from 8-byte value
    #define D_INTERNAL_BYTE_7_OF_8(x) \
        ((uint8_t)(((uint64_t)(x) >> 56) & 0xFFu))

#endif  // D_ENV_ARCH_IS_BIG_ENDIAN

// byte expansion macros

// D_BYTES_1
//   macro: expand value into 1 byte
#define D_BYTES_1(x) \
    D_INTERNAL_BYTE_0_OF_1(x)

// D_BYTES_2
//   macro: expand value into 2 bytes
#define D_BYTES_2(x)           \
    D_INTERNAL_BYTE_0_OF_2(x), \
    D_INTERNAL_BYTE_1_OF_2(x)

// D_BYTES_4
//   macro: expand value into 4 bytes
#define D_BYTES_4(x)           \
    D_INTERNAL_BYTE_0_OF_4(x), \
    D_INTERNAL_BYTE_1_OF_4(x), \
    D_INTERNAL_BYTE_2_OF_4(x), \
    D_INTERNAL_BYTE_3_OF_4(x)

// D_BYTES_8
//   macro: expand value into 8 bytes
#define D_BYTES_8(x)           \
    D_INTERNAL_BYTE_0_OF_8(x), \
    D_INTERNAL_BYTE_1_OF_8(x), \
    D_INTERNAL_BYTE_2_OF_8(x), \
    D_INTERNAL_BYTE_3_OF_8(x), \
    D_INTERNAL_BYTE_4_OF_8(x), \
    D_INTERNAL_BYTE_5_OF_8(x), \
    D_INTERNAL_BYTE_6_OF_8(x), \
    D_INTERNAL_BYTE_7_OF_8(x)

// D_INTERNAL_BYTES_N_IMPL
//   macro: internal helper to select D_BYTES_n by token pasting
#define D_INTERNAL_BYTES_N_IMPL(n, x) \
    D_BYTES_##n(x)

// D_BYTES_N
//   macro: expand value into n bytes using D_BYTES_n
#define D_BYTES_N(n, x) \
    D_INTERNAL_BYTES_N_IMPL(n, x)

// element mapping
//   Every element is emitted with a leading comma, so a list of any length,
// one included, maps the same way: the map's first step takes the first
// element and no element is split off ahead of the others. A builder writes
// its count with no comma after it, and the elements follow.

#if D_ENV_PP_HAS_VARIADIC_MACROS
// D_INTERNAL_BYTE_ARRAY_MAP_END
//   macro: sentinel for map termination
#define D_INTERNAL_BYTE_ARRAY_MAP_END(...)
#endif  // D_ENV_PP_HAS_VARIADIC_MACROS

// D_INTERNAL_BYTE_ARRAY_MAP_OUT
//   macro: defers the next step to the enclosing D_EVAL's next pass
#define D_INTERNAL_BYTE_ARRAY_MAP_OUT

// D_INTERNAL_BYTE_ARRAY_MAP_GET_END
//   macro: helper to retrieve sentinel in variadic context
#define D_INTERNAL_BYTE_ARRAY_MAP_GET_END() \
    0, D_INTERNAL_BYTE_ARRAY_MAP_END

#if D_ENV_PP_HAS_VARIADIC_MACROS
// D_INTERNAL_BYTE_ARRAY_MAP_NEXT0
//   macro: expand to `next` when not at sentinel (stage 0)
#define D_INTERNAL_BYTE_ARRAY_MAP_NEXT0(item, next, ...) \
    next D_INTERNAL_BYTE_ARRAY_MAP_OUT
#endif  // D_ENV_PP_HAS_VARIADIC_MACROS

// D_INTERNAL_BYTE_ARRAY_MAP_NEXT1
//   macro: expand to `next` when not at sentinel (stage 1)
#define D_INTERNAL_BYTE_ARRAY_MAP_NEXT1(item, next) \
    D_INTERNAL_BYTE_ARRAY_MAP_NEXT0(item, next, 0)

// D_INTERNAL_BYTE_ARRAY_MAP_NEXT
//   macro: choose continuation macro based on peeked token
#define D_INTERNAL_BYTE_ARRAY_MAP_NEXT(item, next)                          \
    D_INTERNAL_BYTE_ARRAY_MAP_NEXT1(D_INTERNAL_BYTE_ARRAY_MAP_GET_END item, \
                                    next)

#if D_ENV_PP_HAS_VARIADIC_MACROS
// D_INTERNAL_BYTE_ARRAY_MAP_DATA_COMMA0
//   macro: map step (even index), emits leading comma
#define D_INTERNAL_BYTE_ARRAY_MAP_DATA_COMMA0(f, data, x, peek, ...)        \
    , f(data, x)                                                            \
    D_INTERNAL_BYTE_ARRAY_MAP_NEXT(peek,                                    \
                                   D_INTERNAL_BYTE_ARRAY_MAP_DATA_COMMA1)(  \
        f, data, peek, __VA_ARGS__)

// D_INTERNAL_BYTE_ARRAY_MAP_DATA_COMMA1
//   macro: map step (odd index), emits leading comma
#define D_INTERNAL_BYTE_ARRAY_MAP_DATA_COMMA1(f, data, x, peek, ...)        \
    , f(data, x)                                                            \
    D_INTERNAL_BYTE_ARRAY_MAP_NEXT(peek,                                    \
                                   D_INTERNAL_BYTE_ARRAY_MAP_DATA_COMMA0)(  \
        f, data, peek, __VA_ARGS__)

// D_INTERNAL_BYTE_ARRAY_STEPS
//   macro: the elements, each as `, bytes`, with no D_EVAL of its own: the
// steps after the first are carried by the enclosing D_EVAL's passes
#define D_INTERNAL_BYTE_ARRAY_STEPS(element_size, ...)                      \
    D_INTERNAL_BYTE_ARRAY_MAP_DATA_COMMA0(D_INTERNAL_BYTE_ARRAY_ELEM,       \
                                          element_size,                     \
                                          __VA_ARGS__,                      \
                                          (),                               \
                                          0)

// D_INTERNAL_BYTE_ARRAY_ELEMS
//   macro: the elements, each as `, bytes`
#define D_INTERNAL_BYTE_ARRAY_ELEMS(element_size, ...)                      \
    D_EVAL(D_INTERNAL_BYTE_ARRAY_STEPS(element_size, __VA_ARGS__))
#endif  // D_ENV_PP_HAS_VARIADIC_MACROS

// D_INTERNAL_BYTE_ARRAY_ELEM
//   macro: expand value into element_size-byte view
#define D_INTERNAL_BYTE_ARRAY_ELEM(element_size, val)                       \
    D_BYTES_N(element_size, val)

// core array builders

#if D_ENV_PP_HAS_VARIADIC_MACROS
// D_INTERNAL_BYTE_ARRAY_IMPL
//   macro: build counted byte array with count_size and element_size
//   layout: [count_size bytes N] [element_size bytes x N elements]
#define D_INTERNAL_BYTE_ARRAY_IMPL(count_size, element_size, ...)           \
    {                                                                       \
        D_BYTES_N(count_size, D_VARG_COUNT(__VA_ARGS__))                    \
        D_INTERNAL_BYTE_ARRAY_ELEMS(element_size, __VA_ARGS__)              \
    }

// D_INTERNAL_BYTE_ARRAY_INIT_IMPL
//   macro: same as above but without outer braces (for embedding)
#define D_INTERNAL_BYTE_ARRAY_INIT_IMPL(count_size, element_size, ...)      \
    D_BYTES_N(count_size, D_VARG_COUNT(__VA_ARGS__))                        \
    D_INTERNAL_BYTE_ARRAY_ELEMS(element_size, __VA_ARGS__)

// public 1D array helpers

// D_BYTE_ARRAY
//   macro: default 1D array (1-byte count, 1-byte elements)
#define D_BYTE_ARRAY(...) \
    D_INTERNAL_BYTE_ARRAY_IMPL(1, 1, __VA_ARGS__)

// D_BYTE_ARRAY_INIT
//   macro: initializer-only form (no outer braces)
#define D_BYTE_ARRAY_INIT(...) \
    D_INTERNAL_BYTE_ARRAY_INIT_IMPL(1, 1, __VA_ARGS__)

// suffixed variants: D_BYTE_ARRAY_<count_size>_<elem_size>

// D_BYTE_ARRAY_1_1
//   macro: 1-byte count, 1-byte elements
#define D_BYTE_ARRAY_1_1(...) \
    D_INTERNAL_BYTE_ARRAY_IMPL(1, 1, __VA_ARGS__)

// D_BYTE_ARRAY_1_2
//   macro: 1-byte count, 2-byte elements
#define D_BYTE_ARRAY_1_2(...) \
    D_INTERNAL_BYTE_ARRAY_IMPL(1, 2, __VA_ARGS__)

// D_BYTE_ARRAY_1_4
//   macro: 1-byte count, 4-byte elements
#define D_BYTE_ARRAY_1_4(...) \
    D_INTERNAL_BYTE_ARRAY_IMPL(1, 4, __VA_ARGS__)

// D_BYTE_ARRAY_1_8
//   macro: 1-byte count, 8-byte elements
#define D_BYTE_ARRAY_1_8(...) \
    D_INTERNAL_BYTE_ARRAY_IMPL(1, 8, __VA_ARGS__)

// D_BYTE_ARRAY_2_1
//   macro: 2-byte count, 1-byte elements
#define D_BYTE_ARRAY_2_1(...) \
    D_INTERNAL_BYTE_ARRAY_IMPL(2, 1, __VA_ARGS__)

// D_BYTE_ARRAY_2_2
//   macro: 2-byte count, 2-byte elements
#define D_BYTE_ARRAY_2_2(...) \
    D_INTERNAL_BYTE_ARRAY_IMPL(2, 2, __VA_ARGS__)

// D_BYTE_ARRAY_2_4
//   macro: 2-byte count, 4-byte elements
#define D_BYTE_ARRAY_2_4(...) \
    D_INTERNAL_BYTE_ARRAY_IMPL(2, 4, __VA_ARGS__)

// D_BYTE_ARRAY_2_8
//   macro: 2-byte count, 8-byte elements
#define D_BYTE_ARRAY_2_8(...) \
    D_INTERNAL_BYTE_ARRAY_IMPL(2, 8, __VA_ARGS__)

// D_BYTE_ARRAY_4_1
//   macro: 4-byte count, 1-byte elements
#define D_BYTE_ARRAY_4_1(...) \
    D_INTERNAL_BYTE_ARRAY_IMPL(4, 1, __VA_ARGS__)

// D_BYTE_ARRAY_4_2
//   macro: 4-byte count, 2-byte elements
#define D_BYTE_ARRAY_4_2(...) \
    D_INTERNAL_BYTE_ARRAY_IMPL(4, 2, __VA_ARGS__)

// D_BYTE_ARRAY_4_4
//   macro: 4-byte count, 4-byte elements
#define D_BYTE_ARRAY_4_4(...) \
    D_INTERNAL_BYTE_ARRAY_IMPL(4, 4, __VA_ARGS__)

// D_BYTE_ARRAY_4_8
//   macro: 4-byte count, 8-byte elements
#define D_BYTE_ARRAY_4_8(...) \
    D_INTERNAL_BYTE_ARRAY_IMPL(4, 8, __VA_ARGS__)

// D_BYTE_ARRAY_8_1
//   macro: 8-byte count, 1-byte elements
#define D_BYTE_ARRAY_8_1(...) \
    D_INTERNAL_BYTE_ARRAY_IMPL(8, 1, __VA_ARGS__)

// D_BYTE_ARRAY_8_2
//   macro: 8-byte count, 2-byte elements
#define D_BYTE_ARRAY_8_2(...) \
    D_INTERNAL_BYTE_ARRAY_IMPL(8, 2, __VA_ARGS__)

// D_BYTE_ARRAY_8_4
//   macro: 8-byte count, 4-byte elements
#define D_BYTE_ARRAY_8_4(...) \
    D_INTERNAL_BYTE_ARRAY_IMPL(8, 4, __VA_ARGS__)

// D_BYTE_ARRAY_8_8
//   macro: 8-byte count, 8-byte elements
#define D_BYTE_ARRAY_8_8(...) \
    D_INTERNAL_BYTE_ARRAY_IMPL(8, 8, __VA_ARGS__)

// D_BYTE_ARRAY_CUSTOM
//   macro: customizable count/element sizes
#define D_BYTE_ARRAY_CUSTOM(count_size, element_size, ...) \
    D_INTERNAL_BYTE_ARRAY_IMPL(count_size, element_size, __VA_ARGS__)
#endif  // D_ENV_PP_HAS_VARIADIC_MACROS

// 2D array helpers

// D_BYTE_ARRAY_2D
//   macro: 2D rectangular array (1-byte row count, 1-byte col count)
//   layout: [nrows:1][ncols:1][row0_elem0]...[rowN_elemM]
//
//   usage:
//     uint8_t arr[] = D_BYTE_ARRAY_2D(1, (1,2,3), (4,5,6)); all rows must have
// same number of columns.

// D_INTERNAL_BYTE_ARRAY_ROW_ELEM
//   macro: expand one element in a row to element_size bytes
#define D_INTERNAL_BYTE_ARRAY_ROW_ELEM(_element_size, \
                                       _value)        \
    D_BYTES_N(_element_size,                          \
              _value)

// D_INTERNAL_BYTE_ARRAY_ROW_COUNT
//   macro: count elements in a parenthesized row
#define D_INTERNAL_BYTE_ARRAY_ROW_COUNT(_row) \
    D_VARG_COUNT(D_UNPACK _row)

#if D_ENV_PP_HAS_VARIADIC_MACROS
// D_INTERNAL_BYTE_ARRAY_HEAD
//   macro: the first of its arguments
#define D_INTERNAL_BYTE_ARRAY_HEAD(first, ...) \
    first

// D_INTERNAL_BYTE_ARRAY_ROW_TOTAL
//   macro: the number of rows, the first and those after it, as an
// expression; D_VARG_COUNT counts an empty list as 1, so the rows after the
// first count only when the first of them is a row
#define D_INTERNAL_BYTE_ARRAY_ROW_TOTAL(...)                                \
    (1 + (D_VARG_COUNT(__VA_ARGS__) *                                       \
          D_IS_PAREN(D_INTERNAL_BYTE_ARRAY_HEAD(__VA_ARGS__, ~))))
#endif  // D_ENV_PP_HAS_VARIADIC_MACROS

// row mapping (alternating pattern)
//   A list of rows is mapped as the elements are, every row in the same
// steps, with two differences. Rows are parenthesized, so the list ends at a
// bare sentinel token, told from a row by D_IS_PAREN: the element map's `()`
// sentinel cannot end it, because its end test,
// D_INTERNAL_BYTE_ARRAY_MAP_GET_END peek, calls a macro of no parameters with
// a row's elements whenever peek is a row of two or more. And every row
// expands inside the row map's D_EVAL, where a D_EVAL of its own would not be
// expanded again, so a row's elements go through the element steps directly
// (D_INTERNAL_BYTE_ARRAY_STEPS), carried by the row map's D_EVAL.

// D_INTERNAL_BYTE_ARRAY_ROWS_STOP
//   token: ends a list of rows; never defined, so it is never expanded.

#if D_ENV_PP_HAS_VARIADIC_MACROS
// D_INTERNAL_BYTE_ARRAY_2D_MAP_END
//   macro: sentinel for the row mappers
#define D_INTERNAL_BYTE_ARRAY_2D_MAP_END(...)
#endif  // D_ENV_PP_HAS_VARIADIC_MACROS

// D_INTERNAL_BYTE_ARRAY_2D_MAP_OUT
//   macro: defers the next row step to the enclosing D_EVAL's next pass
#define D_INTERNAL_BYTE_ARRAY_2D_MAP_OUT

// D_INTERNAL_BYTE_ARRAY_2D_MAP_PICK_1
//   macro: another row follows: continue with `next`
#define D_INTERNAL_BYTE_ARRAY_2D_MAP_PICK_1(next) \
    next D_INTERNAL_BYTE_ARRAY_2D_MAP_OUT

// D_INTERNAL_BYTE_ARRAY_2D_MAP_PICK_0
//   macro: no row follows: end the map
#define D_INTERNAL_BYTE_ARRAY_2D_MAP_PICK_0(next) \
    D_INTERNAL_BYTE_ARRAY_2D_MAP_END

// D_INTERNAL_BYTE_ARRAY_2D_MAP_PICK
//   macro: pastes the expanded D_IS_PAREN result onto PICK_
#define D_INTERNAL_BYTE_ARRAY_2D_MAP_PICK(is_row, next) \
    D_INTERNAL_BYTE_ARRAY_2D_MAP_PICK_##is_row(next)

// D_INTERNAL_BYTE_ARRAY_2D_MAP_NEXT_IMPL
//   macro: expands the D_IS_PAREN test before the paste
#define D_INTERNAL_BYTE_ARRAY_2D_MAP_NEXT_IMPL(is_row, next) \
    D_INTERNAL_BYTE_ARRAY_2D_MAP_PICK(is_row, next)

// D_INTERNAL_BYTE_ARRAY_2D_MAP_NEXT
//   macro: the next row step when the peeked item is a row; the end when it
// is the sentinel, or empty (the row list of a one-row array)
#define D_INTERNAL_BYTE_ARRAY_2D_MAP_NEXT(peek, next) \
    D_INTERNAL_BYTE_ARRAY_2D_MAP_NEXT_IMPL(D_IS_PAREN(peek), next)

#if D_ENV_PP_HAS_VARIADIC_MACROS
// D_INTERNAL_BYTE_ARRAY_ROW_STEPS
//   macro: a row's elements, each as `, bytes`
#define D_INTERNAL_BYTE_ARRAY_ROW_STEPS(element_size, row)                  \
    D_INTERNAL_BYTE_ARRAY_STEPS(element_size, D_UNPACK row)

// D_INTERNAL_BYTE_ARRAY_2D_ROW0
//   macro: map even-indexed rows
#define D_INTERNAL_BYTE_ARRAY_2D_ROW0(element_size, row, peek, ...)         \
    D_INTERNAL_BYTE_ARRAY_ROW_STEPS(element_size, row)                      \
    D_INTERNAL_BYTE_ARRAY_2D_MAP_NEXT(peek, D_INTERNAL_BYTE_ARRAY_2D_ROW1)( \
        element_size, peek, __VA_ARGS__)

// D_INTERNAL_BYTE_ARRAY_2D_ROW1
//   macro: map odd-indexed rows
#define D_INTERNAL_BYTE_ARRAY_2D_ROW1(element_size, row, peek, ...)         \
    D_INTERNAL_BYTE_ARRAY_ROW_STEPS(element_size, row)                      \
    D_INTERNAL_BYTE_ARRAY_2D_MAP_NEXT(peek, D_INTERNAL_BYTE_ARRAY_2D_ROW0)( \
        element_size, peek, __VA_ARGS__)

// D_INTERNAL_BYTE_ARRAY_2D_ROWS
//   macro: every row's elements, each as `, bytes`
#define D_INTERNAL_BYTE_ARRAY_2D_ROWS(element_size, ...)                    \
    D_EVAL(D_INTERNAL_BYTE_ARRAY_2D_ROW0(element_size,                      \
                                         __VA_ARGS__,                       \
                                         D_INTERNAL_BYTE_ARRAY_ROWS_STOP,   \
                                         0))

// D_BYTE_ARRAY_2D
//   macro: main 2D helper (1-byte row/col counts)
#define D_BYTE_ARRAY_2D(element_size, first_row, ...)                       \
    {                                                                       \
        D_BYTES_1(D_INTERNAL_BYTE_ARRAY_ROW_TOTAL(__VA_ARGS__)),            \
        D_BYTES_1(D_INTERNAL_BYTE_ARRAY_ROW_COUNT(first_row))               \
        D_INTERNAL_BYTE_ARRAY_2D_ROWS(element_size, first_row, __VA_ARGS__) \
    }

// D_BYTE_ARRAY_2D_EX
//   macro: 2D helper with explicit row/col count sizes
#define D_BYTE_ARRAY_2D_EX(row_count_size, col_count_size,                     \
                           element_size, first_row, ...)                       \
    {                                                                          \
        D_BYTES_N(row_count_size,                                              \
                  D_INTERNAL_BYTE_ARRAY_ROW_TOTAL(__VA_ARGS__)),               \
        D_BYTES_N(col_count_size, D_INTERNAL_BYTE_ARRAY_ROW_COUNT(first_row))  \
        D_INTERNAL_BYTE_ARRAY_2D_ROWS(element_size, first_row, __VA_ARGS__)    \
    }

// jagged array helpers

// D_BYTE_ARRAY_JAGGED
//   macro: jagged array (1-byte row count, 1-byte row lengths)
//   layout: [nrows:1][row0_len:1][row0_data...][row1_len:1]... each row stores
// its own length and payload.

// D_INTERNAL_BYTE_ARRAY_JAGGED_ROW_STEPS
//   macro: one jagged row as `, len bytes, element bytes...`
#define D_INTERNAL_BYTE_ARRAY_JAGGED_ROW_STEPS(len_count_size,                 \
                                               element_size,                   \
                                               row)                            \
    , D_BYTES_N(len_count_size, D_INTERNAL_BYTE_ARRAY_ROW_COUNT(row))          \
    D_INTERNAL_BYTE_ARRAY_ROW_STEPS(element_size, row)

// D_INTERNAL_BYTE_ARRAY_JAGGED_ROW0
//   macro: map even-indexed jagged rows
#define D_INTERNAL_BYTE_ARRAY_JAGGED_ROW0(len_count_size, element_size,        \
                                          row, peek, ...)                      \
    D_INTERNAL_BYTE_ARRAY_JAGGED_ROW_STEPS(len_count_size, element_size, row)  \
    D_INTERNAL_BYTE_ARRAY_2D_MAP_NEXT(peek,                                    \
                                      D_INTERNAL_BYTE_ARRAY_JAGGED_ROW1)(      \
        len_count_size, element_size, peek, __VA_ARGS__)

// D_INTERNAL_BYTE_ARRAY_JAGGED_ROW1
//   macro: map odd-indexed jagged rows
#define D_INTERNAL_BYTE_ARRAY_JAGGED_ROW1(len_count_size, element_size,        \
                                          row, peek, ...)                      \
    D_INTERNAL_BYTE_ARRAY_JAGGED_ROW_STEPS(len_count_size, element_size, row)  \
    D_INTERNAL_BYTE_ARRAY_2D_MAP_NEXT(peek,                                    \
                                      D_INTERNAL_BYTE_ARRAY_JAGGED_ROW0)(      \
        len_count_size, element_size, peek, __VA_ARGS__)

// D_INTERNAL_BYTE_ARRAY_JAGGED_ROWS
//   macro: every jagged row, each as `, len bytes, element bytes...`
#define D_INTERNAL_BYTE_ARRAY_JAGGED_ROWS(len_count_size, element_size, ...)   \
    D_EVAL(D_INTERNAL_BYTE_ARRAY_JAGGED_ROW0(len_count_size,                   \
                                             element_size,                     \
                                             __VA_ARGS__,                      \
                                             D_INTERNAL_BYTE_ARRAY_ROWS_STOP,  \
                                             0))

// D_BYTE_ARRAY_JAGGED
//   macro: main jagged helper (1-byte row count, 1-byte lengths)
#define D_BYTE_ARRAY_JAGGED(element_size, first_row, ...)                      \
    {                                                                          \
        D_BYTES_1(D_INTERNAL_BYTE_ARRAY_ROW_TOTAL(__VA_ARGS__))                \
        D_INTERNAL_BYTE_ARRAY_JAGGED_ROWS(1, element_size, first_row,          \
                                          __VA_ARGS__)                         \
    }

// D_BYTE_ARRAY_JAGGED_EX
//   macro: extended jagged helper with row/length count sizes
#define D_BYTE_ARRAY_JAGGED_EX(row_count_size, len_count_size,                 \
                               element_size, first_row, ...)                   \
    {                                                                          \
        D_BYTES_N(row_count_size,                                              \
                  D_INTERNAL_BYTE_ARRAY_ROW_TOTAL(__VA_ARGS__))                \
        D_INTERNAL_BYTE_ARRAY_JAGGED_ROWS(len_count_size, element_size,        \
                                          first_row, __VA_ARGS__)              \
    }
#endif  // D_ENV_PP_HAS_VARIADIC_MACROS

#endif  // DJINTERP_C_CONTAINER_ARRAY_BYTE_ARRAY_H
