/******************************************************************************
* djinterp [functional]                                           functional.h
*
*   Root header for the functional programming module.
*   Includes all functional sub-modules and provides convenience macros
* for type-inferred operations, predicate/transformer/accumulator generation,
* compound operations, array queries, pipeline shorthands, predicate algebra,
* and guarded execution.
*
* TABLE OF CONTENTS
* =================
* i-v.      Sub-module includes
* VI.       Type-inferred operation macros (no-context)
* VII.      Type-inferred operation macros (with context)
* VIII.     Predicate generator macros (compile-time threshold)
* IX.       Predicate generator macros (context-based threshold)
* X.        Transformer generator macros (compile-time operand)
* XI.       Transformer generator macros (context-based operand)
* XII.      Accumulator generator macros
* XIII.     Inline predicate algebra
* XIV.      Compound operation macros
* XV.       Array query macros
* XVI.      Pipeline shorthand macros
* XVII.     Guarded execution macros
*
*
* path:      \inc\functional\functional.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.02.10
******************************************************************************/

#ifndef DJINTERP_C_FUNCTIONAL_
#define DJINTERP_C_FUNCTIONAL_ 1

#include "..\djinterp.h"
#include ".\functional_common.h"
#include ".\predicate.h"
#include ".\compose.h"
#include ".\fn_builder.h"
#include ".\pipeline.h"


///////////////////////////////////////////////////////////////////////////////
///        VI.   TYPE-INFERRED OPERATION MACROS (NO CONTEXT)                ///
///////////////////////////////////////////////////////////////////////////////
// Wrappers that eliminate sizeof(type) and NULL context boilerplate from
// the higher-order functions in functional_common.h.  Each macro infers
// element size from the TYPE parameter and passes NULL as context.
// For operations requiring a non-NULL context, use the _CONTEXT variants
// in section VII or call the underlying function directly.

// D_FUNCTIONAL_MAP
//   macro: type-inferred map.  Applies _fn to each element of _in,
// writing transformed results to _out.
// Returns bool (true on success).
#define D_FUNCTIONAL_MAP(type,                                              \
                         in,                                                \
                         out,                                               \
                         count,                                             \
                         func)                                              \
    d_functional_map((in),                                                  \
                     (out),                                                 \
                     (count),                                               \
                     sizeof(type),                                          \
                     (fn),                                                  \
                     NULL)

// D_FUNCTIONAL_FOLD_LEFT
//   macro: type-inferred left fold.  Accumulates from left to right.
// Returns bool (true on success).
#define D_FUNCTIONAL_FOLD_LEFT(type,                                        \
                               in,                                          \
                               count,                                       \
                               acc,                                         \
                               combine)                                     \
    d_functional_fold_left((in),                                            \
                           (count),                                         \
                           sizeof(type),                                    \
                           (acc),                                           \
                           (combine),                                       \
                           NULL)

// D_FUNCTIONAL_FOLD_RIGHT
//   macro: type-inferred right fold.  Accumulates from right to left.
// Returns bool (true on success).
#define D_FUNCTIONAL_FOLD_RIGHT(type,                                       \
                                in,                                         \
                                count,                                      \
                                acc,                                        \
                                combine)                                    \
    d_functional_fold_right((in),                                           \
                            (count),                                        \
                            sizeof(type),                                   \
                            (acc),                                          \
                            (combine),                                      \
                            NULL)

// D_FUNCTIONAL_FOR_EACH
//   macro: type-inferred mutable for-each.
#define D_FUNCTIONAL_FOR_EACH(type,                                         \
                              in,                                           \
                              count,                                        \
                              apply)                                        \
    d_functional_for_each((in),                                             \
                          (count),                                          \
                          sizeof(type),                                     \
                          (apply),                                          \
                          NULL)

// D_FUNCTIONAL_FOR_EACH_CONST
//   macro: type-inferred immutable for-each.
#define D_FUNCTIONAL_FOR_EACH_CONST(type,                                   \
                                    in,                                     \
                                    count,                                  \
                                    apply)                                  \
    d_functional_for_each_const((in),                                       \
                                (count),                                    \
                                sizeof(type),                               \
                                (apply),                                    \
                                NULL)

// D_FUNCTIONAL_ANY
//   macro: type-inferred any.  True if at least one element matches.
#define D_FUNCTIONAL_ANY(type,                                              \
                         in,                                                \
                         count,                                             \
                         test)                                              \
    d_functional_any((in),                                                  \
                     (count),                                               \
                     sizeof(type),                                          \
                     (test),                                                \
                     NULL)

// D_FUNCTIONAL_ALL
//   macro: type-inferred all.  True if every element matches.
#define D_FUNCTIONAL_ALL(type,                                              \
                         in,                                                \
                         count,                                             \
                         test)                                              \
    d_functional_all((in),                                                  \
                     (count),                                               \
                     sizeof(type),                                          \
                     (test),                                                \
                     NULL)

// D_FUNCTIONAL_NONE
//   macro: type-inferred none.  True if no element matches.
#define D_FUNCTIONAL_NONE(type,                                             \
                          in,                                               \
                          count,                                            \
                          test)                                             \
    d_functional_none((in),                                                 \
                      (count),                                              \
                      sizeof(type),                                         \
                      (test),                                               \
                      NULL)

// D_FUNCTIONAL_COUNT_IF
//   macro: type-inferred count_if.  Returns number of matches.
#define D_FUNCTIONAL_COUNT_IF(type,                                         \
                   in,                                                      \
                   count,                                                   \
                   test)                                                    \
    d_functional_count_if((in),                                             \
                          (count),                                          \
                          sizeof(type),                                     \
                          (test),                                           \
                          NULL)

// D_FUNCTIONAL_FIND_IF
//   macro: type-inferred find_if.  Returns void* to first match
// or NULL.
#define D_FUNCTIONAL_FIND_IF(type,                                          \
                             in,                                            \
                             count,                                         \
                             test)                                          \
    d_functional_find_if((in),                                              \
                         (count),                                           \
                         sizeof(type),                                      \
                         (test),                                            \
                         NULL)


///////////////////////////////////////////////////////////////////////////////
///        VII.  TYPE-INFERRED OPERATION MACROS (WITH CONTEXT)              ///
///////////////////////////////////////////////////////////////////////////////
//
// Identical to section VI but accept an explicit context parameter
// that is forwarded to the callback.
//

// D_FUNCTIONAL_MAP_CONTEXT
//   macro: type-inferred map with explicit context forwarded to
// the transformer.
#define D_FUNCTIONAL_MAP_CONTEXT(type,                                      \
                                 in,                                        \
                                 out,                                       \
                                 count,                                     \
                                 fn,                                        \
                                 context)                                   \
    d_functional_map((in),                                                  \
                     (out),                                                 \
                     (count),                                               \
                     sizeof(type),                                          \
                     (fn),                                                  \
                     (context))

// D_FUNCTIONAL_FOLD_LEFT_CONTEXT
//   macro: type-inferred left fold with explicit context.
#define D_FUNCTIONAL_FOLD_LEFT_CONTEXT(type,                                \
                                       in,                                  \
                                       count,                               \
                                       acc,                                 \
                                       combine,                             \
                                       context)                             \
    d_functional_fold_left((in),                                            \
                           (count),                                         \
                           sizeof(type),                                    \
                           (acc),                                           \
                           (combine),                                       \
                           (context))

// D_FUNCTIONAL_FOLD_RIGHT_CONTEXT
//   macro: type-inferred right fold with explicit context.
#define D_FUNCTIONAL_FOLD_RIGHT_CONTEXT(type,                               \
                                        in,                                 \
                                        count,                              \
                                        acc,                                \
                                        combine,                            \
                                        context)                            \
    d_functional_fold_right((in),                                           \
                            (count),                                        \
                            sizeof(type),                                   \
                            (acc),                                          \
                            (combine),                                      \
                            (context))

// D_FUNCTIONAL_FOR_EACH_CONTEXT
//   macro: type-inferred mutable for-each with explicit context.
#define D_FUNCTIONAL_FOR_EACH_CONTEXT(type,                                 \
                                      in,                                   \
                                      count,                                \
                                      apply,                                \
                                      context)                              \
    d_functional_for_each((in),                                             \
                          (count),                                          \
                          sizeof(type),                                     \
                          (apply),                                          \
                          (context))

// D_FUNCTIONAL_FOR_EACH_CONST_CONTEXT
//   macro: type-inferred immutable for-each with explicit context.
#define D_FUNCTIONAL_FOR_EACH_CONST_CONTEXT(type,                           \
                                            in,                             \
                                            count,                          \
                                            apply,                          \
                                            context)                        \
    d_functional_for_each_const((in),                                       \
                                (count),                                    \
                                sizeof(type),                               \
                                (apply),                                    \
                                (context))

// D_FUNCTIONAL_ANY_CONTEXT
//   macro: type-inferred any with explicit context.
#define D_FUNCTIONAL_ANY_CONTEXT(type,                                      \
                      in,                                                   \
                      count,                                                \
                      test,                                                 \
                      context)                                              \
    d_functional_any((in),                                                  \
                     (count),                                               \
                     sizeof(type),                                          \
                     (test),                                                \
                     (context))

// D_FUNCTIONAL_ALL_CONTEXT
//   macro: type-inferred all with explicit context.
#define D_FUNCTIONAL_ALL_CONTEXT(type,                                      \
                                 in,                                        \
                                 count,                                     \
                                 test,                                      \
                                 context)                                   \
    d_functional_all((in),                                                  \
                     (count),                                               \
                     sizeof(type),                                          \
                     (test),                                                \
                     (context))

// D_FUNCTIONAL_NONE_CONTEXT
//   macro: type-inferred none with explicit context.
#define D_FUNCTIONAL_NONE_CONTEXT(type,                                     \
                                  in,                                       \
                                  count,                                    \
                                  test,                                     \
                                  context)                                  \
    d_functional_none((in),                                                 \
                      (count),                                              \
                      sizeof(type),                                         \
                      (test),                                               \
                      (context))

// D_FUNCTIONAL_COUNT_IF_CONTEXT
//   macro: type-inferred count_if with explicit context.
#define D_FUNCTIONAL_COUNT_IF_CONTEXT(type,                                 \
                                      in,                                   \
                                      count,                                \
                                      test,                                 \
                                      context)                              \
    d_functional_count_if((in),                                             \
                          (count),                                          \
                          sizeof(type),                                     \
                          (test),                                           \
                          (context))

// D_FUNCTIONAL_FIND_IF_CONTEXT
//   macro: type-inferred find_if with explicit context.
#define D_FUNCTIONAL_FIND_IF_CONTEXT(type,                                  \
                                     in,                                    \
                                     count,                                 \
                                     test,                                  \
                                     context)                               \
    d_functional_find_if((in),                                              \
                         (count),                                           \
                         sizeof(type),                                      \
                         (test),                                            \
                         (context))


///////////////////////////////////////////////////////////////////////////////
///        VIII. PREDICATE GENERATOR MACROS (COMPILE-TIME THRESHOLD)        ///
///////////////////////////////////////////////////////////////////////////////
//
// Generate inline predicate functions from typed comparisons.
// Each macro defines a D_INLINE fn_predicate-compatible function
// with the threshold baked in at compile time.
//

// D_GEN_FUNCTIONAL_PREDICATE_GT
//   macro: generates a predicate returning true when the element
// (interpreted as TYPE) is strictly greater than THRESHOLD.
#define D_GEN_FUNCTIONAL_PREDICATE_GT(name,                                 \
                                      type,                                 \
                                      threshold)                            \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if (!_element)                                                      \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        return (*(const type*)_element > (threshold));                      \
    }

// D_GEN_FUNCTIONAL_PREDICATE_GE
//   macro: generates a predicate returning true when element >=
// THRESHOLD.
#define D_GEN_FUNCTIONAL_PREDICATE_GE(name,                                 \
                                      type,                                 \
                                      threshold)                            \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( !_element )                                                    \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        return (*(const type*)_element >= (threshold));                     \
    }

// D_GEN_FUNCTIONAL_PREDICATE_LT
//   macro: generates a predicate returning true when element <
// THRESHOLD.
#define D_GEN_FUNCTIONAL_PREDICATE_LT(name,                                 \
                                      type,                                 \
                                      threshold)                            \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if (!_element)                                                      \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        return (*(const type*)_element < (threshold));                      \
    }

// D_GEN_FUNCTIONAL_PREDICATE_LE
//   macro: generates a predicate returning true when element <=
// THRESHOLD.
#define D_GEN_FUNCTIONAL_PREDICATE_LE(name,                                 \
                                      type,                                 \
                                      threshold)                            \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( !_element )                                                    \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        return (*(const type*)_element <= (threshold));                     \
    }

// D_GEN_FUNCTIONAL_PREDICATE_EQ
//   macro: generates a predicate returning true when element ==
// VALUE.
#define D_GEN_FUNCTIONAL_PREDICATE_EQ(name,                                 \
                                      type,                                 \
                                      value)                                \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( !_element )                                                    \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        return (*(const type*)_element == (value));                         \
    }

// D_GEN_FUNCTIONAL_PREDICATE_NE
//   macro: generates a predicate returning true when element !=
// VALUE.
#define D_GEN_FUNCTIONAL_PREDICATE_NE(name,                                 \
                                      type,                                 \
                                      value)                                \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( !_element )                                                    \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        return (*(const type*)_element != (value));                         \
    }

// D_GEN_FUNCTIONAL_PREDICATE_BETWEEN
//   macro: generates a predicate returning true when element is
// within [LOW, HIGH] inclusive.
#define D_GEN_FUNCTIONAL_PREDICATE_BETWEEN(name,                            \
                                           type,                            \
                                           low,                             \
                                           high)                            \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
        if ( !_element )                                                    \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
        return ( (*(const type*)_element >= (low)) &&                       \
                 (*(const type*)_element <= (high)) );                      \
    }

// D_GEN_FUNCTIONAL_PREDICATE_OUTSIDE
//   macro: generates a predicate returning true when element is
// strictly outside (LOW, HIGH).
#define D_GEN_FUNCTIONAL_PREDICATE_OUTSIDE(name,                            \
                                           type,                            \
                                           low,                             \
                                           high)                            \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( !_element )                                                    \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        return ( (*(const type*)_element < (low)) ||                        \
                 (*(const type*)_element > (high)) );                       \
    }

// D_GEN_FUNCTIONAL_PREDICATE_DIVISIBLE
//   macro: generates a predicate returning true when element is
// evenly divisible by DIVISOR.  TYPE must be an integral type.
#define D_GEN_FUNCTIONAL_PREDICATE_DIVISIBLE(name,                          \
                                             type,                          \
                                             divisor)                       \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( !_element )                                                    \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        return ((*(const type*)_element % (divisor)) == 0);                 \
    }

// D_GEN_FUNCTIONAL_PREDICATE_ODD
//   macro: generates a predicate returning true when element is
// odd.  TYPE must be an integral type.                                     
#define D_GEN_FUNCTIONAL_PREDICATE_ODD(name,                                \
                                       type)                                \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( !_element )                                                    \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        return ((*(const type*)_element % 2) != 0);                         \
    }

// D_GEN_FUNCTIONAL_PREDICATE_POSITIVE
//   macro: generates a predicate returning true when element > 0.
#define D_GEN_FUNCTIONAL_PREDICATE_POSITIVE(name,                           \
                                            type)                           \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( !_element )                                                    \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        return (*(const type*)_element > 0);                                \
    }

// D_GEN_FUNCTIONAL_PREDICATE_NEGATIVE
//   macro: generates a predicate returning true when element < 0.
#define D_GEN_FUNCTIONAL_PREDICATE_NEGATIVE(name,                           \
                                            type)                           \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( !_element )                                                    \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        return (*(const type*)_element < 0);                                \
    }

// D_GEN_FUNCTIONAL_PREDICATE_ZERO
//   macro: generates a predicate returning true when element == 0.
#define D_GEN_FUNCTIONAL_PREDICATE_ZERO(name,                               \
                                        type)                               \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( !_element )                                                    \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        return (*(const type*)_element == 0);                               \
    }

// D_GEN_FUNCTIONAL_PREDICATE_NONZERO
//   macro: generates a predicate returning true when element != 0.
#define D_GEN_FUNCTIONAL_PREDICATE_NONZERO(name,                            \
                                           type)                            \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( !_element )                                                    \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        return (*(const type*)_element != 0);                               \
    }


///////////////////////////////////////////////////////////////////////////////
///        IX.   PREDICATE GENERATOR MACROS (CONTEXT-BASED THRESHOLD)       ///
///////////////////////////////////////////////////////////////////////////////
//
// These read the comparison target from _context, allowing a
// single predicate function to be reused with different runtime
// threshold values.  Pass a pointer to a TYPE value as context.
//

// D_GEN_FUNCTIONAL_PREDICATE_GT_CONTEXT
//   macro: generates a predicate returning true when element >
// *(TYPE*)_context.
#define D_GEN_FUNCTIONAL_PREDICATE_GT_CONTEXT(name,                         \
                                              type)                         \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        if ( (!_element) ||                                                 \
             (!_context) )                                                  \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        return (*(const type*)_element >                                    \
                *(const type*)_context);                                    \
    }

// D_GEN_FUNCTIONAL_PREDICATE_GE_CONTEXT
//   macro: generates a predicate returning true when element >=
// *(TYPE*)_context.
#define D_GEN_FUNCTIONAL_PREDICATE_GE_CONTEXT(name,                         \
                                              type)                         \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        if ( (!_element) ||                                                 \
             (!_context) )                                                  \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        return (*(const type*)_element >=                                   \
                *(const type*)_context);                                    \
    }

// D_GEN_FUNCTIONAL_PREDICATE_LT_CONTEXT
//   macro: generates a predicate returning true when element <
// *(TYPE*)_context.
#define D_GEN_FUNCTIONAL_PREDICATE_LT_CONTEXT(name,                         \
                                              type)                         \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        if ( (!_element) ||                                                 \
             (!_context) )                                                  \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        return (*(const type*)_element <                                    \
                *(const type*)_context);                                    \
    }

// D_GEN_FUNCTIONAL_PREDICATE_LE_CONTEXT
//   macro: generates a predicate returning true when element <=
// *(TYPE*)_context.
#define D_GEN_FUNCTIONAL_PREDICATE_LE_CONTEXT(name,                         \
                                              type)                         \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        if ( (!_element) ||                                                 \
             (!_context) )                                                  \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        return (*(const type*)_element <=                                   \
                *(const type*)_context);                                    \
    }

// D_GEN_FUNCTIONAL_PREDICATE_EQ_CONTEXT
//   macro: generates a predicate returning true when element ==
// *(TYPE*)_context.
#define D_GEN_FUNCTIONAL_PREDICATE_EQ_CONTEXT(name,                         \
                                              type)                         \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        if ( (!_element) ||                                                 \
             (!_context) )                                                  \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        return (*(const type*)_element ==                                   \
                *(const type*)_context);                                    \
    }

// D_GEN_FUNCTIONAL_PREDICATE_NE_CONTEXT
//   macro: generates a predicate returning true when element !=
// *(TYPE*)_context.
#define D_GEN_FUNCTIONAL_PREDICATE_NE_CONTEXT(name,                         \
                                              type)                         \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        if ( (!_element) ||                                                 \
             (!_context) )                                                  \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        return (*(const type*)_element !=                                   \
                *(const type*)_context);                                    \
    }


///////////////////////////////////////////////////////////////////////////////
///        X.    TRANSFORMER GENERATOR MACROS (COMPILE-TIME OPERAND)        ///
///////////////////////////////////////////////////////////////////////////////
//
// Generate inline transformer functions for common arithmetic and
// clamping operations on typed values.
//

// D_GEN_FUNCTIONAL_DEFINE_XFORM_ADD
//   macro: generates a transformer that adds ADDEND to each
// element.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_ADD(name,                             \
                           type,                                            \
                           addend)                                          \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( (!_input) ||                                                   \
             (!_output) )                                                   \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        *(type*)_output = *(const type*)_input + (addend);                  \
                                                                            \
        return true;                                                        \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_SUB
//   macro: generates a transformer that subtracts SUBTRAHEND
// from each element.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_SUB(name,                             \
                           type,                                            \
                           subtrahend)                                      \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( (!_input) ||                                                   \
             (!_output) )                                                   \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        *(type*)_output = *(const type*)_input - (subtrahend);              \
                                                                            \
        return true;                                                        \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_MUL
//   macro: generates a transformer that multiplies each element
// by FACTOR.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_MUL(name,                             \
                           type,                                            \
                           factor)                                          \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
        if ( (!_input) ||                                                   \
             (!_output) )                                                   \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        *(type*)_output = *(const type*)_input * (factor);                  \
                                                                            \
        return true;                                                        \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_DIV
//   macro: generates a transformer that divides each element
// by DIVISOR.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_DIV(name,                             \
                           type,                                            \
                           divisor)                                         \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( (!_input) ||                                                   \
             (!_output) )                                                   \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        *(type*)_output = *(const type*)_input / (divisor);                 \
                                                                            \
        return true;                                                        \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_MOD
//   macro: generates a transformer that computes element %
// DIVISOR.  TYPE must be an integral type.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_MOD(name,                             \
                           type,                                            \
                           divisor)                                         \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
        if ( (!_input) ||                                                   \
             (!_output) )                                                   \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        *(type*)_output = *(const type*)_input % (divisor);                 \
                                                                            \
        return true;                                                        \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_NEGATE
//   macro: generates a transformer that negates each element.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_NEGATE(name,                          \
                              type)                                         \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( (!_input) ||                                                   \
             (!_output) )                                                   \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        *(type*)_output = -(*(const type*)_input);                          \
                                                                            \
        return true;                                                        \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_ABS
//   macro: generates a transformer that computes the absolute
// value of each element.  TYPE must be a signed type.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_ABS(name,                             \
                                          type)                             \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        type _val;                                                          \
        (void)_context;                                                     \
                                                                            \
        if ( (!_input) ||                                                   \
             (!_output) )                                                   \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        _val = *(const type*)_input;                                        \
        *(type*)_output = (_val < 0) ? -_val : _val;                        \
                                                                            \
        return true;                                                        \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_CLAMP
//   macro: generates a transformer that clamps each element to
// [LOW, HIGH].
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_CLAMP(name,                           \
                                            type,                           \
                                            low,                            \
                                            high)                           \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        type _val;                                                          \
        (void)_context;                                                     \
                                                                            \
        if ( (!_input) ||                                                   \
             (!_output) )                                                   \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        _val = *(const type*)_input;                                        \
                                                                            \
        if ( _val < (low) )                                                 \
        {                                                                   \
            _val = (low);                                                   \
        }                                                                   \
                                                                            \
        if ( _val > (high) )                                                \
        {                                                                   \
            _val = (high);                                                  \
        }                                                                   \
                                                                            \
        *(type*)_output = _val;                                             \
                                                                            \
        return true;                                                        \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_CAST
//   macro: generates a transformer that casts each element from
// FROM_TYPE to TO_TYPE.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_CAST(name,                            \
                                           from_type,                       \
                                           to_type)                         \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( (!_input) ||                                                   \
             (!_output) )                                                   \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        *(to_type*)_output = (to_type)(*(const from_type*)_input);          \
                                                                            \
        return true;                                                        \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_SQUARE
//   macro: generates a transformer that squares each element.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_SQUARE(name,                          \
                                             type)                          \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        type _val;                                                          \
        (void)_context;                                                     \
                                                                            \
        if ( (!_input) ||                                                   \
             (!_output) )                                                   \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        _val = *(const type*)_input;                                        \
        *(type*)_output = _val * _val;                                      \
                                                                            \
        return true;                                                        \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_SHIFT_LEFT
//   macro: generates a transformer that left-shifts each element
// by AMOUNT bits.  TYPE must be an integral type.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_SHIFT_LEFT(name,                      \
                                                 type,                      \
                                                 amount)                    \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( (!_input) ||                                                   \
             (!_output) )                                                   \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        *(type*)_output = (type)(*(const type*)_input << (amount));         \
                                                                            \
        return true;                                                        \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_SHIFT_RIGHT
//   macro: generates a transformer that right-shifts each element
// by AMOUNT bits.  TYPE must be an integral type.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_SHIFT_RIGHT(name,                     \
                                                  type,                     \
                                                  amount)                   \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( (!_input) ||                                                   \
             (!_output) )                                                   \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        *(type*)_output = (type)(*(const type*)_input >> (amount));         \
                                                                            \
        return true;                                                        \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_BITWISE_AND
//   macro: generates a transformer that ANDs each element with
// MASK.  TYPE must be an integral type.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_BITWISE_AND(name,                     \
                                                  type,                     \
                                                  mask)                     \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( (!_input) ||                                                   \
             (!_output) )                                                   \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        *(type*)_output = *(const type*)_input & (mask);                    \
                                                                            \
        return true;                                                        \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_BITWISE_OR
//   macro: generates a transformer that ORs each element with
// MASK.  TYPE must be an integral type.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_BITWISE_OR(name,                      \
                                                 type,                      \
                                                 mask)                      \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( (!_input) ||                                                   \
             (!_output) )                                                   \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        *(type*)_output = *(const type*)_input | (mask);                    \
                                                                            \
        return true;                                                        \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_BITWISE_XOR
//   macro: generates a transformer that XORs each element with
// MASK.  TYPE must be an integral type.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_BITWISE_XOR(name,                     \
                                                  type,                     \
                                                  mask)                     \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( (!_input) ||                                                   \
             (!_output) )                                                   \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        *(type*)_output = *(const type*)_input ^ (mask);                    \
                                                                            \
        return true;                                                        \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_BITWISE_NOT
//   macro: generates a transformer that flips all bits in each
// element.  TYPE must be an integral type.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_BITWISE_NOT(name,                     \
                                                  type)                     \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( (!_input) ||                                                   \
             (!_output) )                                                   \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        *(type*)_output = (type)(~(*(const type*)_input));                  \
                                                                            \
        return true;                                                        \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_MAP_TO
//   macro: generates a transformer that ignores the input and
// always writes CONSTANT to the output.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_MAP_TO(name,                          \
                                             type,                          \
                                             constant)                      \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        (void)_input;                                                       \
        (void)_context;                                                     \
                                                                            \
        if ( !_output )                                                     \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        *(type*)_output = (constant);                                       \
                                                                            \
        return true;                                                        \
    }


///////////////////////////////////////////////////////////////////////////////
///        XI.   TRANSFORMER GENERATOR MACROS (CONTEXT-BASED OPERAND)       ///
///////////////////////////////////////////////////////////////////////////////
//
// These read the operand from _context, allowing a single
// transformer to be reused with different runtime values.
// Pass a pointer to a TYPE value as context.
//

// D_GEN_FUNCTIONAL_DEFINE_XFORM_ADD_CONTEXT
//   macro: generates a transformer that adds *(TYPE*)_context
// to each element.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_ADD_CONTEXT(name,                     \
                                                  type)                     \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        if ( (!_input)  ||                                                  \
             (!_output) ||                                                  \
             (!_context) )                                                  \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        *(type*)_output = *(const type*)_input + *(const type*)_context;    \
                                                                            \
        return true;                                                        \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_SUB_CONTEXT
//   macro: generates a transformer that subtracts
// *(TYPE*)_context from each element.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_SUB_CONTEXT(name,                     \
                                                  type)                     \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        if ( (!_input)  ||                                                  \
             (!_output) ||                                                  \
             (!_context) )                                                  \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        *(type*)_output = *(const type*)_input - *(const type*)_context;    \
                                                                            \
        return true;                                                        \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_MUL_CONTEXT
//   macro: generates a transformer that multiplies each element
// by *(TYPE*)_context.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_MUL_CONTEXT(name,                     \
                                                  type)                     \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        if ( (!_input)  ||                                                  \
             (!_output) ||                                                  \
             (!_context) )                                                  \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        *(type*)_output = *(const type*)_input * *(const type*)_context;    \
                                                                            \
        return true;                                                        \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_DIV_CONTEXT
//   macro: generates a transformer that divides each element
// by *(TYPE*)_context.  Returns false on zero divisor.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_DIV_CONTEXT(name,                     \
                                                  type)                     \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        if ( (!_input)  ||                                                  \
             (!_output) ||                                                  \
             (!_context) )                                                  \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        *(type*)_output = *(const type*)_input / *(const type*)_context;    \
                                                                            \
        return true;                                                        \
    }


///////////////////////////////////////////////////////////////////////////////
///        XII.  ACCUMULATOR GENERATOR MACROS                               ///
///////////////////////////////////////////////////////////////////////////////
//
// Generate inline fn_accumulator-compatible functions for common
// reduction patterns.  Each reads both _accumulated and _element
// as pointers to TYPE.
//

// D_DEFINE_ACC_SUM
//   macro: generates an accumulator that computes a running sum.
#define D_DEFINE_ACC_SUM(name,                          \
                         type)                          \
    D_INLINE bool                                       \
    name                                                \
    (                                                   \
        void*       _accumulated,                       \
        const void* _element,                           \
        void*       _context                            \
    )                                                   \
    {                                                   \
        (void)_context;                                 \
        if ( (!_accumulated) || (!_element) )           \
        {                                               \
            return false;                               \
        }                                               \
        *(type*)_accumulated += *(const type*)_element; \
        return true;                                    \
    }

// D_DEFINE_ACC_PRODUCT
//   macro: generates an accumulator that computes a running
// product.
#define D_DEFINE_ACC_PRODUCT(name,                      \
                             type)                      \
    D_INLINE bool                                       \
    name                                                \
    (                                                   \
        void*       _accumulated,                       \
        const void* _element,                           \
        void*       _context                            \
    )                                                   \
    {                                                   \
        (void)_context;                                 \
        if ( (!_accumulated) || (!_element) )           \
        {                                               \
            return false;                               \
        }                                               \
        *(type*)_accumulated *= *(const type*)_element; \
        return true;                                    \
    }

// D_DEFINE_ACC_MIN
//   macro: generates an accumulator that tracks the minimum.
#define D_DEFINE_ACC_MIN(name,                             \
                         type)                             \
    D_INLINE bool                                          \
    name                                                   \
    (                                                      \
        void*       _accumulated,                          \
        const void* _element,                              \
        void*       _context                               \
    )                                                      \
    {                                                      \
        (void)_context;                                    \
        if ( (!_accumulated) || (!_element) )              \
        {                                                  \
            return false;                                  \
        }                                                  \
        if (*(const type*)_element < *(type*)_accumulated) \
        {                                                  \
            *(type*)_accumulated = *(const type*)_element; \
        }                                                  \
        return true;                                       \
    }

// D_DEFINE_ACC_MAX
//   macro: generates an accumulator that tracks the maximum.
#define D_DEFINE_ACC_MAX(name,                             \
                         type)                             \
    D_INLINE bool                                          \
    name                                                   \
    (                                                      \
        void*       _accumulated,                          \
        const void* _element,                              \
        void*       _context                               \
    )                                                      \
    {                                                      \
        (void)_context;                                    \
        if ( (!_accumulated) || (!_element) )              \
        {                                                  \
            return false;                                  \
        }                                                  \
        if (*(const type*)_element > *(type*)_accumulated) \
        {                                                  \
            *(type*)_accumulated = *(const type*)_element; \
        }                                                  \
        return true;                                       \
    }

// D_DEFINE_ACC_BITWISE_OR
//   macro: generates an accumulator that computes a running
// bitwise OR.  TYPE must be an integral type.
#define D_DEFINE_ACC_BITWISE_OR(name,                   \
                                type)                   \
    D_INLINE bool                                       \
    name                                                \
    (                                                   \
        void*       _accumulated,                       \
        const void* _element,                           \
        void*       _context                            \
    )                                                   \
    {                                                   \
        (void)_context;                                 \
        if ( (!_accumulated) || (!_element) )           \
        {                                               \
            return false;                               \
        }                                               \
        *(type*)_accumulated |= *(const type*)_element; \
        return true;                                    \
    }

// D_DEFINE_ACC_BITWISE_AND
//   macro: generates an accumulator that computes a running
// bitwise AND.  TYPE must be an integral type.
#define D_DEFINE_ACC_BITWISE_AND(name,                  \
                                 type)                  \
    D_INLINE bool                                       \
    name                                                \
    (                                                   \
        void*       _accumulated,                       \
        const void* _element,                           \
        void*       _context                            \
    )                                                   \
    {                                                   \
        (void)_context;                                 \
        if ( (!_accumulated) || (!_element) )           \
        {                                               \
            return false;                               \
        }                                               \
        *(type*)_accumulated &= *(const type*)_element; \
        return true;                                    \
    }

// D_DEFINE_ACC_COUNT
//   macro: generates an accumulator that counts elements.
// _accumulated is a pointer to size_t; the element value is
// ignored.
#define D_DEFINE_ACC_COUNT(name)    \
    D_INLINE bool                   \
    name                            \
    (                               \
        void*       _accumulated,   \
        const void* _element,       \
        void*       _context        \
    )                               \
    {                               \
        (void)_element;             \
        (void)_context;             \
        if ( !_accumulated )        \
        {                           \
            return false;           \
        }                           \
        (*(size_t*)_accumulated)++; \
        return true;                \
    }

// D_DEFINE_ACC_MEAN_STATE
//   struct: intermediate state for computing a running mean.
struct d_acc_mean_state
{
    double sum;
    size_t count;
};

// D_DEFINE_ACC_MEAN
//   macro: generates an accumulator that computes a running mean.
// _accumulated must point to a struct d_acc_mean_state, and
// _element must point to a TYPE value.  The mean can be read
// from state.sum / state.count after the fold completes.
#define D_DEFINE_ACC_MEAN(name,                       \
                          type)                       \
    D_INLINE bool                                     \
    name                                              \
    (                                                 \
        void*       _accumulated,                     \
        const void* _element,                         \
        void*       _context                          \
    )                                                 \
    {                                                 \
        struct d_acc_mean_state* _st;                 \
        (void)_context;                               \
        if ( (!_accumulated) || (!_element) )         \
        {                                             \
            return false;                             \
        }                                             \
        _st = (struct d_acc_mean_state*)_accumulated; \
        _st->sum += (double)(*(const type*)_element); \
        _st->count++;                                 \
        return true;                                  \
    }


///////////////////////////////////////////////////////////////////////////////
///        XIII. INLINE PREDICATE ALGEBRA                                   ///
///////////////////////////////////////////////////////////////////////////////
//
// Generate new predicates by combining or negating existing ones
// without allocating predicate combinator structs.  For heap-
// allocated combinators with separate contexts, use predicate.h.
//

// D_GEN_FUNCTIONAL_PREDICATE_NEGATE
//   macro: generates a predicate that returns the logical NOT of ORIGINAL.
// Context is forwarded to ORIGINAL.
#define D_GEN_FUNCTIONAL_PREDICATE_NEGATE(name,                             \
                                          original)                         \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        return !(original)(_element, _context);                             \
    }

// D_GEN_FUNCTIONAL_PREDICATE_AND
//   macro: generates a predicate that returns (PRED1 && PRED2).
// Both predicates receive the same element; context is forwarded to both.
#define D_GEN_FUNCTIONAL_PREDICATE_AND(name,                                \
                                       predicate1,                          \
                                       predicate2)                          \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        return ( (predicate1)(_element, _context) &&                        \
                 (predicate2)(_element, _context) );                        \
    }

// D_GEN_FUNCTIONAL_PREDICATE_OR
//   macro: generates a predicate that returns (PRED1 || PRED2).
// Both predicates receive the same element; context is forwarded to both.
#define D_GEN_FUNCTIONAL_PREDICATE_OR(name,                                 \
                                      predicate1,                           \
                                      predicate2)                           \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        return ( (predicate1)(_element, _context) ||                        \
                 (predicate2)(_element, _context) );                        \
    }

// D_GEN_FUNCTIONAL_PREDICATE_XOR
//   macro: generates a predicate that returns (PRED1 ^ PRED2).
#define D_GEN_FUNCTIONAL_PREDICATE_XOR(name,                                \
                                       predicate1,                          \
                                       predicate2)                          \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        bool _r1;                                                           \
        bool _r2;                                                           \
                                                                            \
        _r1 = (predicate1)(_element, _context);                             \
        _r2 = (predicate2)(_element, _context);                             \
                                                                            \
        return (_r1 != _r2);                                                \
    }

// D_GEN_FUNCTIONAL_PREDICATE_IMPLIES
//   macro: generates a predicate for logical implication (!PRED1 || PRED2), 
// i.e. PRED1 => PRED2.
#define D_GEN_FUNCTIONAL_PREDICATE_IMPLIES(name,                            \
                                           predicate1,                      \
                                           predicate2)                      \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        return ( !(predicate1)(_element, _context) ||                       \
                  (predicate2)(_element, _context) );                       \
    }

// D_GEN_FUNCTIONAL_PREDICATE_AND3
//   macro: conjunction of three predicates.
#define D_GEN_FUNCTIONAL_PREDICATE_AND3(name,                               \
                                        predicate1,                         \
                                        predicate2,                         \
                                        predicate3)                         \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        return ( (predicate1)(_element, _context) &&                        \
                 (predicate2)(_element, _context) &&                        \
                 (predicate3)(_element, _context) );                        \
    }

// D_GEN_FUNCTIONAL_PREDICATE_OR3
//   macro: disjunction of three predicates.
#define D_GEN_FUNCTIONAL_PREDICATE_OR3(name,                                \
                                       predicate1,                          \
                                       predicate2,                          \
                                       predicate3)                          \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        return ( (predicate1)(_element, _context) ||                        \
                 (predicate2)(_element, _context) ||                        \
                 (predicate3)(_element, _context) );                        \
    }

// D_GEN_FUNCTIONAL_PREDICATE_N_OF
//   macro: generates a predicate that returns true when at least N of three
// predicates are true.  Useful for majority-vote logic.
#define D_GEN_FUNCTIONAL_PREDICATE_N_OF(name,                               \
                                        n,                                  \
                                        pred1,                              \
                                        pred2,                              \
                                        pred3)                              \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _element,                                               \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        int _hits;                                                          \
        _hits = 0;                                                          \
                                                                            \
        if ( (pred1)(_element, _context) )                                  \
        {                                                                   \
            _hits++;                                                        \
        }                                                                   \
                                                                            \
        if ( (pred2)(_element, _context) )                                  \
        {                                                                   \
            _hits++;                                                        \
        }                                                                   \
                                                                            \
        if ( (pred3)(_element, _context) )                                  \
        {                                                                   \
            _hits++;                                                        \
        }                                                                   \
                                                                            \
        return (_hits >= (n));                                              \
    }


///////////////////////////////////////////////////////////////////////////////
///        XIV.  COMPOUND OPERATION MACROS                                  ///
///////////////////////////////////////////////////////////////////////////////
//
// Common multi-step patterns composed from the higher-order
// primitives.  Where a temporary buffer is required, the caller
// supplies it.
//

// D_FUNCTIONAL_MAP_FOLD
//   macro: map each element into _tmp, then fold _tmp into _acc.
// _tmp must be a writable buffer of at least _count * sizeof(TYPE).
#define D_FUNCTIONAL_MAP_FOLD(type,                                         \
                              in,                                           \
                              tmp,                                          \
                              count,                                        \
                              transform,                                    \
                              acc,                                          \
                              combine)                                      \
    ( D_FUNCTIONAL_MAP(type, (in), (tmp), (count), (transform)) &&          \
      D_FUNCTIONAL_FOLD_LEFT( type,                                         \
                              (tmp),                                        \
                              (count),                                      \
                              (acc),                                        \
                              (combine)) )                                  
                                                                            
// D_FUNCTIONAL_MAP_ANY                                                     
//   macro: true if any element, after transformation to _tmp,              
// satisfies _test.                                                         
#define D_FUNCTIONAL_MAP_ANY(type,                                          \
                             in,                                            \
                             tmp,                                           \
                             count,                                         \
                             transform,                                     \
                             test)                                          \
    ( D_FUNCTIONAL_MAP(type,                                                \
                       (in),                                                \
                       (tmp),                                               \
                       (count),                                             \
                       (transform)) &&                                      \
      D_FUNCTIONAL_ANY(type,                                                \
                       (_tmp),                                              \
                       (_count),                                            \
                       (_test)) )

// D_FUNCTIONAL_MAP_ALL
//   macro: true if every element, after transformation to _tmp,
// satisfies _test.
#define D_FUNCTIONAL_MAP_ALL(type,                                          \
                             in,                                            \
                             tmp,                                           \
                             count,                                         \
                             transform,                                     \
                             test)                                          \
    ( D_FUNCTIONAL_MAP(type,                                                \
                       (in),                                                \
                       (tmp),                                               \
                       (count),                                             \
                       (transform)) &&                                      \
      D_FUNCTIONAL_ALL(type,                                                \
                       (tmp),                                               \
                       (count),                                             \
                       (test)) )

// D_FUNCTIONAL_MAP_NONE
//   macro: true if no element, after transformation to _tmp,
// satisfies _test.
#define D_FUNCTIONAL_MAP_NONE(type,                                         \
                              in,                                           \
                              tmp,                                          \
                              count,                                        \
                              transform,                                    \
                              test)                                         \
    ( D_FUNCTIONAL_MAP(type,                                                \
                       (in),                                                \
                       (tmp),                                               \
                       (count),                                             \
                       (transform)) &&                                      \
      D_FUNCTIONAL_NONE(type,                                               \
                        (_tmp),                                             \
                        (_count),                                           \
                        (_test)) )

// D_FUNCTIONAL_MAP_COUNT_IF
//   macro: count how many elements satisfy _test after
// transformation.  _tmp required.
// Note: evaluates to bool from map; count must be read via
// separate D_FUNCTIONAL_COUNT_IF call.  Use the pipeline API for tighter
// integration.
#define D_FUNCTIONAL_MAP_COUNT_IF(type,                                     \
                                  in,                                       \
                                  tmp,                                      \
                                  count,                                    \
                                  transform,                                \
                                  test)                                     \
    ( D_FUNCTIONAL_MAP(type,                                                \
                       (in),                                                \
                       (tmp),                                               \
                       (count),                                             \
                       (transform))                                         \
      ? D_FUNCTIONAL_COUNT_IF(type,                                         \
                              (tmp),                                        \
                              (count),                                      \
                              (test))                                       \
      : 0 )

// D_FUNCTIONAL_FOLD_BOTH
//   macro: performs both a left fold and a right fold over the
// same input into two separate accumulators.
#define D_FUNCTIONAL_FOLD_BOTH(type,                                        \
                               in,                                          \
                               count,                                       \
                               acc_left,                                    \
                               combine_left,                                \
                               acc_right,                                   \
                               combine_right)                               \
    ( D_FUNCTIONAL_FOLD_LEFT(type,                                          \
                             (in),                                          \
                             (count),                                       \
                             (acc_left),                                    \
                             (combine_left)) &&                             \
      D_FUNCTIONAL_FOLD_RIGHT(type,                                         \
                              (in),                                         \
                              (count),                                      \
                              (acc_right),                                  \
                              (combine_right) )


///////////////////////////////////////////////////////////////////////////////
///        XV.   ARRAY QUERY MACROS                                         ///
///////////////////////////////////////////////////////////////////////////////
// Shorthand macros for common array interrogation patterns.

// D_FUNCTIONAL_CONTAINS
//   macro: true if any element satisfies _test.  Alias for D_FUNCTIONAL_ANY
// for readability in query context.
#define D_FUNCTIONAL_CONTAINS(type,                                         \
                              in,                                           \
                              count,                                        \
                              test)                                         \
    D_FUNCTIONAL_ANY(type,                                                  \
                     (in),                                                  \
                     (count),                                               \
                     (test))

// D_FUNCTIONAL_FIRST_WHERE
//   macro: returns a typed pointer to the first element that satisfies test,
// or NULL.
#define D_FUNCTIONAL_FIRST_WHERE(type,                                      \
                                 in,                                        \
                                 count,                                     \
                                 test)                                      \
    ( (type*)D_FUNCTIONAL_FIND_IF(type,                                     \
                                  (in),                                     \
                                  (count),                                  \
                                  (test)) )

// D_FUNCTIONAL_IS_EMPTY
//   macro: true if no element satisfies the predicate. Equivalent to 
// D_FUNCTIONAL_NONE; provided for readability.
#define D_FUNCTIONAL_IS_EMPTY(type,                                         \
                              in,                                           \
                              count,                                        \
                              test)                                         \
    D_FUNCTIONAL_NONE(type,                                                 \
                      (in),                                                 \
                      (count),                                              \
                      (test)) )

// D_FUNCTIONAL_IS_SORTED
//   macro: true if an array of TYPE is sorted according to a `fn_comparator`.
// Walks adjacent pairs and short-circuits on the first inversion.
#define D_FUNCTIONAL_IS_SORTED(type,                                        \
                               in,                                          \
                               count,                                       \
                               comparator)                                  \
    d_functional_is_sorted((in),                                            \
                           (count),                                         \
                           sizeof(type),                                    \
                           (comparator))

// D_FUNCTIONAL_INDEX_OF
//   macro: returns the zero-based index of the first element satisfying test,
// or (size_t)-1 if not found.
#define D_FUNCTIONAL_INDEX_OF(type,                                         \
                              in,                                           \
                              count,                                        \
                              test)                                         \
    d_functional_index_of((in),                                             \
                          (count),                                          \
                          sizeof(type),                                     \
                          (test))

// D_FUNCTIONAL_LAST_WHERE
//   macro: returns a typed pointer to the last element that satisfies test, or
// NULL.  Walks the array in reverse.
#define D_FUNCTIONAL_LAST_WHERE(type,                                       \
                                in,                                         \
                                count,                                      \
                                test)                                       \
    ( (type*)d_functional_find_last((in),                                   \
                                    (count),                                \
                                    sizeof(type),                           \
                                    (test)) )

// D_FUNCTIONAL_LAST_INDEX_OF
//   macro: returns the zero-based index of the last element satisfying test,
// or (size_t)-1 if not found.
#define D_FUNCTIONAL_LAST_INDEX_OF(type,                                    \
                                   in,                                      \
                                   count,                                   \
                                   test)                                    \
    d_functional_last_index_of((in),                                        \
                               (count),                                     \
                               sizeof(type),                                \
                               (test))



bool   d_functional_is_sorted(const void* _input, size_t _count, size_t _element_size, fn_function_comparator _function_comparator);
size_t d_functional_index_of(const void* _input, size_t _count, size_t _element_size, fn_predicate _test);
void*  d_functional_find_last(const void* _input, size_t _count, size_t _element_size, fn_predicate _test);
size_t d_functional_last_index_of(const void* _input, size_t _count, size_t _element_size, fn_predicate _test);


///////////////////////////////////////////////////////////////////////////////
///        XVI.  PIPELINE SHORTHAND MACROS                                  ///
///////////////////////////////////////////////////////////////////////////////
// NULL-context and typed wrappers around the pipeline API in
// pipeline.h for concise chaining.
//

// D_FUNCTIONAL_PIPE_BEGIN
//   macro: starts a mutable pipeline from a typed array.
#define D_FUNCTIONAL_PIPE_BEGIN(type,                                       \
                                data,                                       \
                                count)                                      \
    d_functional_pipeline_begin((data),                                     \
                                (count),                                    \
                                sizeof(type))

// D_FUNCTIONAL_PIPE_BEGIN_COPY
//   macro: starts a pipeline from a typed array by copying data.
#define D_FUNCTIONAL_PIPE_BEGIN_COPY(type,                                  \
                                     data,                                  \
                                     count)                                 \
    d_functional_pipeline_begin_copy((data),                                \
                                     (count),                               \
                                     sizeof(type))

// D_FUNCTIONAL_PIPE_MAP
//   macro: pipeline map with NULL context.
#define D_FUNCTIONAL_PIPE_MAP(pipe,                                         \
                              fn)                                           \
    d_functional_pipeline_map((pipe),                                       \
                              (fn),                                         \
                              NULL)

// D_FUNCTIONAL_PIPE_MAP_CONTEXT
//   macro: pipeline map with explicit context.
#define D_FUNCTIONAL_PIPE_MAP_CONTEXT(pipe,                                 \
                                      fn,                                   \
                                      context)                              \
    d_functional_pipeline_map((pipe),                                       \
                              (fn),                                         \
                              (context))

// D_FUNCTIONAL_PIPE_FILTER
//   macro: pipeline filter with NULL context.
#define D_FUNCTIONAL_PIPE_FILTER(pipe,                                      \
                                 fn)                                        \
    d_functional_pipeline_filter((pipe),                                    \
                                 (fn),                                      \
                                 NULL)

// D_FUNCTIONAL_PIPE_FILTER_CONTEXT
//   macro: pipeline filter with explicit context.
#define D_FUNCTIONAL_PIPE_FILTER_CONTEXT(pipe,                              \
                                         fn,                                \
                                         context)                           \
    d_functional_pipeline_filter((pipe),                                    \
                                 (fn),                                      \
                                 (context))

// D_FUNCTIONAL_PIPE_FOLD
//   macro: pipeline fold with NULL context and typed accumulator
// size.
#define D_FUNCTIONAL_PIPE_FOLD(type,                                        \
                               pipe,                                        \
                               init,                                        \
                               combine)                                     \
    d_functional_pipeline_fold((pipe),                                      \
                               (init),                                      \
                               sizeof(type),                                \
                               (combine),                                   \
                               NULL)

// D_FUNCTIONAL_PIPE_FOLD_CONTEXT
//   macro: pipeline fold with explicit context.
#define D_FUNCTIONAL_PIPE_FOLD_CONTEXT(type,                                \
                                       pipe,                                \
                                       init,                                \
                                       combine,                             \
                                       context)                             \
    d_functional_pipeline_fold((pipe),                                      \
                               (init),                                      \
                               sizeof(type),                                \
                               (combine),                                   \
                               (context))

// D_FUNCTIONAL_PIPE_FOR_EACH
//   macro: pipeline for-each with NULL context.
#define D_FUNCTIONAL_PIPE_FOR_EACH(pipe,                                    \
                                   fn)                                      \
    d_functional_pipeline_for_each((pipe),                                  \
                                   (fn),                                    \
                                   NULL)

// D_FUNCTIONAL_PIPE_FOR_EACH_CONTEXT
//   macro: pipeline for-each with explicit context.
#define D_FUNCTIONAL_PIPE_FOR_EACH_CONTEXT(pipe,                            \
                                           fn,                              \
                                           context)                         \
    d_functional_pipeline_for_each((pipe),                                  \
                                   (fn),                                    \
                                   (context))

// D_FUNCTIONAL_PIPE_TAKE
//   macro: pipeline take.
#define D_FUNCTIONAL_PIPE_TAKE(pipe,                                        \
                               n)                                           \
    d_functional_pipeline_take((pipe),                                      \
                               (n))

// D_FUNCTIONAL_PIPE_SKIP
//   macro: pipeline skip.
#define D_FUNCTIONAL_PIPE_SKIP(pipe,                                        \
                               n)                                           \
    d_functional_pipeline_skip((pipe),                                      \
                               (n))

// D_FUNCTIONAL_PIPE_END
//   macro: finalize pipeline and retrieve result.
#define D_FUNCTIONAL_PIPE_END(pipe,                                         \
                              out_count)                                    \
    d_functional_pipeline_end((pipe),                                       \
                               (out_count))

// D_FUNCTIONAL_PIPE_FREE
//   macro: free pipeline resources.
#define D_FUNCTIONAL_PIPE_FREE(pipe)                                        \
    d_functional_pipeline_free((pipe))


///////////////////////////////////////////////////////////////////////////////
///        XVII. GUARDED EXECUTION MACROS                                   ///
///////////////////////////////////////////////////////////////////////////////
// Conditional application patterns that combine predicates with
// consumers or transformers.
//

// D_GEN_FUNCTIONAL_DEFINE_CONSUMER_IF
//   macro: generates a consumer that applies `consumer` only when
// `predicate` returns true for the element.
#define D_GEN_FUNCTIONAL_DEFINE_CONSUMER_IF(name,                           \
                                            predicate,                      \
                                            consumer)                       \
    D_INLINE void                                                           \
    name                                                                    \
    (                                                                       \
        void* _element,                                                     \
        void* _context                                                      \
    )                                                                       \
    {                                                                       \
        if ((predicate)((const void*)_element, _context))                   \
        {                                                                   \
            (consumer)(_element, _context);                                 \
        }                                                                   \
                                                                            \
        return;                                                             \
    }

// D_GEN_FUNCTIONAL_DEFINE_CONSUMER_UNLESS
//   macro: generates a consumer that applies `consumer` only when `predicate`
// returns false for the element.
#define D_GEN_FUNCTIONAL_DEFINE_CONSUMER_UNLESS(name,                       \
                                                predicate,                  \
                                                consumer)                   \
    D_INLINE void                                                           \
    name                                                                    \
    (                                                                       \
        void* _element,                                                     \
        void* _context                                                      \
    )                                                                       \
    {                                                                       \
        if (!(predicate)((const void*)_element, _context))                  \
        {                                                                   \
            (consumer)(_element, _context);                                 \
        }                                                                   \
                                                                            \
        return;                                                             \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_IF
//   macro: generates a transformer that applies `transform` when `predicate`
// is true, otherwise copies input to output unchanged.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_IF(name,                              \
                                         type,                              \
                                         predicate,                         \
                                         transform)                         \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        if ( (!_input) ||                                                   \
             (!_output) )                                                   \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        if ((predicate)(_input, _context))                                  \
        {                                                                   \
            return (transform)(_input,                                      \
                               _output,                                     \
                               _context);                                   \
        }                                                                   \
                                                                            \
        *(type*)_output = *(const type*)_input;                             \
                                                                            \
        return true;                                                        \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_UNLESS
//   macro: generates a transformer that applies `transform` when `predicate`
// is false, otherwise copies input to output unchanged.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_UNLESS(name,                          \
                                             type,                          \
                                             predicate,                     \
                                             transform)                     \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        if ( (!_input) ||                                                   \
             (!_output) )                                                   \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        if (!(predicate)(_input, _context))                                 \
        {                                                                   \
            return (transform)(_input,                                      \
                               _output,                                     \
                               _context);                                   \
        }                                                                   \
                                                                            \
        *(type*)_output = *(const type*)_input;                             \
                                                                            \
        return true;                                                        \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_DEFAULT
//   macro: generates a transformer that applies `transform` and
// writes `default_val` to output if `transform` returns false.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_DEFAULT(name,                         \
                                              type,                         \
                                              transform,                    \
                                              default_val)                  \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        if (!_output )                                                      \
        {                                                                   \
            return false;                                                   \
        }                                                                   \
                                                                            \
        if ( (_input) &&                                                    \
             (transform)(_input,                                            \
                         _output,                                           \
                         _context) )                                        \
        {                                                                   \
            return true;                                                    \
        }                                                                   \
                                                                            \
        *(type*)_output = (default_val);                                    \
                                                                            \
        return true;                                                        \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_CHAIN
//   macro: generates a transformer that applies `first`, then applies 
// `second` to the intermediate result.  Both transformers operate on `type`.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_CHAIN(name,                           \
                                            type,                           \
                                            first,                          \
                                            second)                         \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        type _intermediate;                                                 \
                                                                            \
        return ( (_input)  &&                                               \
                 (_output) &&                                               \
                 ((first)(_input, &_intermediate, _context)) )              \
        ? (second)(&_intermediate,                                          \
                    _output,                                                \
                    _context)                                               \
        : false;                                                            \
    }

// D_GEN_FUNCTIONAL_DEFINE_XFORM_CHAIN3
//   macro: generates a transformer that chains three transformers
// sequentially: `first` -> `first` -> `third`.  All operate on type.
#define D_GEN_FUNCTIONAL_DEFINE_XFORM_CHAIN3(name,                          \
                                             type,                          \
                                             first,                         \
                                             second,                        \
                                             third)                         \
    D_INLINE bool                                                           \
    name                                                                    \
    (                                                                       \
        const void* _input,                                                 \
        void*       _output,                                                \
        void*       _context                                                \
    )                                                                       \
    {                                                                       \
        type _tmp1;                                                         \
        type _tmp2;                                                         \
                                                                            \
        return ( (_input)  &&                                               \
                 (_output) &&                                               \
                 ( (first)(_input, &_tmp1, _context)) &&                    \
                   (second)(&_tmp1, &_tmp2, _context)) )                    \
        ? (third)(&_tmp2,                                                   \
                  _output,                                                  \
                  _context)                                                 \
        : false;                                                            \
    }


#endif  // DJINTERP_C_FUNCTIONAL_
