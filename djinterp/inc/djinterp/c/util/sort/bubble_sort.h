/******************************************************************************
* djinterp [utility]                                             bubble_sort.h
*
* Bubble sort, the C face.
*   This header is notation over bubble_sort_common.h and computes nothing.
* Every function it declares or generates has the same body -- the expansion of
* D_BUBBLE_SORT_BODY -- and differs from its neighbours only in how the element
* type and the ordering are supplied.  If a change here would move an element,
* the change belongs in bubble_sort_common.h instead, where the C++ face gets
* it too.
*
*   THREE WAYS TO SORT, IN DESCENDING ORDER OF WHAT THE COMPILER CAN SEE:
*
*     1. TYPED, NATIVE ORDER.  D_BUBBLE_SORT_DEFINE(name, type) emits a
*        function taking (type*, type*) and ordering by `<`.  The comparison is
*        an operator and the carried element lives in a register; nothing is
*        called and nothing is passed.  This is the free one, and it is what
*        the C++ face lowers to as well.
*
*     2. TYPED, CALLER'S ORDER.  D_BUBBLE_SORT_DEFINE_BY(name, type) emits a
*        function taking a `struct d_sort_comparator*`.  The elements stay
*        typed -- the carry is still a register -- but the comparison is an
*        indirect call, because the caller chose the ordering at run time.
*        That is the cost of the choice, not the cost of the sharing.
*
*     3. ERASED.  d_bubble_sort() takes (void*, count, element_size).  The
*        comparison is an indirect call, the exchange is a byte loop, and the
*        carried element cannot be held at all -- it stays at base[i-1] and is
*        re-read.  Measured at roughly 1.7x the typed path.  Use it when the
*        width really is a run-time value; reach for a typed instantiation
*        otherwise.
*
*   All three run the same text, so all three produce the same permutation.
*
*   ADDING A TYPE IS ONE LINE.  The instantiations below cover the built-in
* scalars.  For anything else:
*
*       // header
*       D_BUBBLE_SORT_DECLARE(sort_vertices, struct vertex);
*
*       // source
*       #define VERTEX_BEFORE(_c, _carry, _b, _i)  ((_b)[_i].z < (_carry).z)
*       D_BUBBLE_SORT_DEFINE_WITH(sort_vertices, struct vertex, VERTEX_BEFORE, 0)
*
*   Prefix a definition with D_STATIC_INLINE to keep it local to one
* translation unit -- the generators expand to a definition beginning with its
* return type, so a qualifier in front of the macro lands where it should.
*
*
* path:      /inc/djinterp/core/util/sort/bubble_sort.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.08.07
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_BUBBLE_C_
#define DJINTERP_UTILITY_SORT_BUBBLE_C_ 1

// std
#include <stddef.h>
// djinterp
#include "../../djinterp.h"
#include "./sort_common.h"          // d_sort_comparator, d_sort_status
#include "./bubble_sort_common.h"   // D_BUBBLE_SORT_BODY


D_EXTERN_C_BEGIN


///////////////////////////////////////////////////////////////////////////////
///             I.    PROPERTIES                                            ///
///////////////////////////////////////////////////////////////////////////////
//   The compile-time property set, matching bubble_sort_traits in the C++
// face.  These are facts about the text in bubble_sort_common.h, so the two
// faces cannot disagree about them without one of them being wrong.

// D_BUBBLE_SORT_IS_STABLE
//   constant: 1 -- only a strict precedence moves an element.
#define D_BUBBLE_SORT_IS_STABLE     1

// D_BUBBLE_SORT_IS_IN_PLACE
//   constant: 1 -- O(1) auxiliary storage; one carried element.
#define D_BUBBLE_SORT_IS_IN_PLACE   1

// D_BUBBLE_SORT_IS_ADAPTIVE
//   constant: 1 -- the shrinking bound gives O(n) on sorted input.
#define D_BUBBLE_SORT_IS_ADAPTIVE   1


///////////////////////////////////////////////////////////////////////////////
///             II.   THE ORDERING HOOK                                     ///
///////////////////////////////////////////////////////////////////////////////

// D_SORT_LESS_COMPARATOR
//   macro: LESS hook reading a `const struct d_sort_comparator*`.  Converts
// the framework's three-way ordering to the strict precedence the body wants,
// and applies the direction: descending asks the same question with the
// operands exchanged, which is what the C++ face's order_comparator does and
// what `reversed()` does at compile time.  D_SORT_ORDER_NONE is ascending.
//
//   Belongs in sort_common.h once the remaining five algorithms land -- all
// six share it -- and is here only because bubble sort is the first through.
#define D_SORT_LESS_COMPARATOR(_cmp,                                        \
                               _carry,                                      \
                               _base,                                       \
                               _i)                                          \
    ( ( ((_cmp)->order == D_SORT_ORDER_DESCENDING)                          \
            ? (_cmp)->compare(&(_carry), &(_base)[_i], (_cmp)->context)     \
            : (_cmp)->compare(&(_base)[_i], &(_carry), (_cmp)->context)     \
      ) < 0 )


///////////////////////////////////////////////////////////////////////////////
///             III.  TYPED GENERATORS                                      ///
///////////////////////////////////////////////////////////////////////////////
//   Each DECLARE emits a prototype and each DEFINE the matching definition, so
// a header and its source stay in step by construction.  Neither ends in a
// semicolon: the call site supplies it after a DECLARE and a brace after a
// DEFINE, which is how the framework's other generators read.

// D_BUBBLE_SORT_DECLARE
//   macro: prototype for a typed, natively ordered bubble sort.
#define D_BUBBLE_SORT_DECLARE(_name,                                        \
                              _type)                                        \
    void _name(_type* _first,                                               \
               _type* _last)

// D_BUBBLE_SORT_DECLARE_BY
//   macro: prototype for a typed bubble sort taking the caller's ordering.
#define D_BUBBLE_SORT_DECLARE_BY(_name,                                     \
                                 _type)                                     \
    void _name(_type*                          _first,                      \
               _type*                          _last,                       \
               const struct d_sort_comparator* _comparator)

// D_BUBBLE_SORT_DEFINE_WITH
//   macro: definition of a typed bubble sort under an arbitrary LESS hook and
// ordering token.  The two DEFINEs below are this one with the hook filled in;
// call it directly to order by something the framework has no name for (a
// struct member, a computed key, a lexicographic tie-break).
//
//   The range is the precondition, exactly as it is for the C++ face: [_first,
// _last) must be a valid range over one array.  A null or inverted range is
// not checked here, because there is no check a typed entry could perform that
// the erased entry does not already perform for the caller who needs it.
#define D_BUBBLE_SORT_DEFINE_WITH(_name,                                    \
                                  _type,                                    \
                                  _less,                                    \
                                  _cmp)                                     \
    void _name(_type* _first,                                               \
               _type* _last)                                                \
    {                                                                       \
        D_BUBBLE_SORT_BODY(ptrdiff_t,                                       \
                           _type,                                           \
                           _less,                                           \
                           D_SORT_CARRY_VALUE,                              \
                           D_SORT_EXCHANGE_VALUE,                           \
                           _cmp,                                            \
                           _first,                                          \
                           _last - _first);                                 \
                                                                            \
        return;                                                             \
    }

// D_BUBBLE_SORT_DEFINE
//   macro: definition of a typed bubble sort ordering by `<`.
#define D_BUBBLE_SORT_DEFINE(_name,                                         \
                             _type)                                         \
    D_BUBBLE_SORT_DEFINE_WITH(_name, _type, D_SORT_LESS_NATIVE, 0)

// D_BUBBLE_SORT_DEFINE_BY
//   macro: definition of a typed bubble sort taking the caller's ordering.
// A null comparator leaves the range untouched -- the ordering is the whole
// operation, so there is nothing to fall back to.
#define D_BUBBLE_SORT_DEFINE_BY(_name,                                      \
                                _type)                                      \
    void _name(_type*                          _first,                      \
               _type*                          _last,                       \
               const struct d_sort_comparator* _comparator)                 \
    {                                                                       \
        /* an absent ordering is not an ascending one */                    \
        if ( (!_comparator) ||                                              \
             (!_comparator->compare) )                                      \
        {                                                                   \
            return;                                                         \
        }                                                                   \
                                                                            \
        D_BUBBLE_SORT_BODY(ptrdiff_t,                                       \
                           _type,                                           \
                           D_SORT_LESS_COMPARATOR,                          \
                           D_SORT_CARRY_VALUE,                              \
                           D_SORT_EXCHANGE_VALUE,                           \
                           _comparator,                                     \
                           _first,                                          \
                           _last - _first);                                 \
                                                                            \
        return;                                                             \
    }


///////////////////////////////////////////////////////////////////////////////
///             IV.   PROVIDED INSTANTIATIONS                               ///
///////////////////////////////////////////////////////////////////////////////

// D_BUBBLE_SORT_SCALAR_LIST
//   macro: the built-in scalar types the framework instantiates for, as a list
// the DECLARE and DEFINE passes both walk.  One list, two expansions, so a
// type cannot appear in the header and be missing from the object file.
#define D_BUBBLE_SORT_SCALAR_LIST(_entry)                                   \
    _entry(d_bubble_sort_char,    char)                                     \
    _entry(d_bubble_sort_schar,   signed char)                              \
    _entry(d_bubble_sort_uchar,   unsigned char)                            \
    _entry(d_bubble_sort_short,   short)                                    \
    _entry(d_bubble_sort_ushort,  unsigned short)                           \
    _entry(d_bubble_sort_int,     int)                                      \
    _entry(d_bubble_sort_uint,    unsigned int)                             \
    _entry(d_bubble_sort_long,    long)                                     \
    _entry(d_bubble_sort_ulong,   unsigned long)                            \
    _entry(d_bubble_sort_size,    size_t)                                   \
    _entry(d_bubble_sort_ptrdiff, ptrdiff_t)                                \
    _entry(d_bubble_sort_float,   float)                                    \
    _entry(d_bubble_sort_double,  double)

// D_INTERNAL_BUBBLE_SORT_DECLARE_ENTRY
//   macro: one row of the declaration pass -- the natively ordered form and
// the caller-ordered form of one type.
#define D_INTERNAL_BUBBLE_SORT_DECLARE_ENTRY(_name,                         \
                                             _type)                         \
    D_BUBBLE_SORT_DECLARE(_name, _type);                                    \
    D_BUBBLE_SORT_DECLARE_BY(_name##_by, _type);

D_BUBBLE_SORT_SCALAR_LIST(D_INTERNAL_BUBBLE_SORT_DECLARE_ENTRY)


///////////////////////////////////////////////////////////////////////////////
///             V.    ERASED ENTRY                                          ///
///////////////////////////////////////////////////////////////////////////////

// d_bubble_sort
//   function: sorts _count elements of _element_size bytes at _base under
// _comparator, in place.
//
//   The width is a run-time value here, so the exchange is a byte loop, the
// comparison an indirect call, and the carried element stays in memory.  The
// control flow is still the one text, so the permutation is the one a typed
// instantiation would have produced.
//
//   Parameter(s):
//     _base:         the range; may be NULL only when _count is 0.
//     _count:        element count.
//     _element_size: element width in bytes; must be non-zero.
//     _comparator:   the ordering; must be non-NULL with a non-NULL compare.
//   Return:
//     D_SORT_STATUS_OK, or D_SORT_STATUS_INVALID_ARGUMENT with the range left
//   untouched.
enum d_sort_status
        d_bubble_sort(void*                           _base,
                      size_t                          _count,
                      size_t                          _element_size,
                      const struct d_sort_comparator* _comparator);


D_EXTERN_C_END


#endif  // DJINTERP_UTILITY_SORT_BUBBLE_C_
