/******************************************************************************
* djinterp [utility]                                      bubble_sort_common.h
*
* Bubble sort, tier 0: the algorithm itself, written once, for both languages.
*   This header contains no functions and declares no types.  It contains ONE
* macro -- D_BUBBLE_SORT_BODY -- which is the bubble sort, and the only one in
* the framework.  The C face (bubble_sort.h / .c) expands it to produce typed
* functions; the C++ face (bubble_sort.hpp) expands it inside a template.  The
* two faces therefore do not merely agree, and are not merely tested to agree:
* they are the same token sequence handed to the same compiler.
*
*   WHY A MACRO AND NOT AN INLINE FUNCTION.
*   The obvious shape for a shared kernel is a `D_INLINE` function in a common
* header that both faces call.  It does not work, for one reason: C cannot
* write a function generic over the element type.  Such a function must take
* `void*` and an `element_size`, which costs an indirect call per comparison
* and a byte loop per exchange, and hides both from the optimiser.  That is a
* real cost -- measured at roughly 1.7x on the erased path below -- and a C++
* caller sorting a std::vector<int> would pay it on every comparison purely
* because the algorithm was shared.
*
*   The framework already answers this question, in maybe.h, for a type rather
* than for code:
*
*       "A C++ face cannot reuse D_MAYBE_DECLARE inside a template -- the macro
*        names the struct -- so the FIELDS were factored out here, and both the
*        C macro below and the C++ template in functional_face.hpp expand this
*        same text.  Neither language restates the members."
*
*   D_MAYBE_FIELDS is a layout that neither language restates.  D_BUBBLE_SORT_
* BODY is an ALGORITHM that neither language restates.  It is the same rule
* (goal 4) applied to the other half of a module, and it buys the same thing:
* one declaration, no drift, no second implementation to keep honest.
*
*   THE ELEMENT OPERATIONS ARE HOOKS; THE CONTROL FLOW IS NOT.
*   The body is parameterised by three operations -- compare, carry, exchange
* -- and by the index and element types.  Everything else (the pass structure,
* the shrinking bound, the early exit) is fixed text.  This is the seam that
* matters:
*
*     - The CONTROL FLOW decides which permutation comes out.  Bubble sort is
*       stable, so that permutation is unique, and both faces produce it by
*       construction rather than by testing.
*     - The ELEMENT OPERATIONS decide only how one comparison and one exchange
*       are spelled.  A typed range holds the carried element in a register; a
*       run-time-width range cannot, and re-reads it from memory instead.
*       Neither changes what is compared with what, or what ends up where.
*
*   THE CARRIED ELEMENT.
*   A pass walks the range keeping the largest element seen so far in hand and
* dropping it one place to its right whenever the next element is smaller.
* Textbook bubble sort re-reads that element from base[i-1] on every iteration;
* holding it instead costs one load per element rather than two, and -- more
* usefully -- keeps the exchange from becoming a read-modify-write of two
* ADJACENT slots, which GCC's SLP vectoriser folds into a paired 64-bit load
* and shuffle whose overlapping store-to-load forwarding then stalls.  On
* 9,000 random ints (GCC 13, -O2) the three shapes measured:
*
*       adjacent-swap, indexed    175 ms  (99 ms with -fno-tree-slp-vectorize)
*       adjacent-swap, pointers    99 ms  (the shape this replaces)
*       carried element            77 ms
*
*   So the shared text is not a compromise struck to make sharing possible: it
* is faster than the hand-written implementation it replaces, in both faces.
* The carry is a HOOK rather than a plain local because the erased path has
* nowhere to put it -- there, the carried element is by definition the one at
* base[i-1], which is exactly where it physically is, so the hooks degrade to
* re-reading and the control flow is untouched.
*
*   ELEMENTS ARE ADDRESSED BY INDEX, NOT BY CURSOR.
*   `base[i]` is the one element access that C pointers, C++ random-access
* iterators, and a byte-stepped erased range can all express.  A cursor-based
* body would need four more hooks (advance, retreat, compare, dereference) and
* would stop being readable, which for a normative text is a real cost.
*
*   COMPLEXITY (the property set the faces publish):
*     best:     O(n)        (already sorted -- one pass, no exchanges)
*     average:  O(n^2)
*     worst:    O(n^2)
*     space:    O(1)        (one carried element, three index locals)
*     stable:   yes         (only a STRICT precedence moves an element)
*
*
* path:      /inc/djinterp/core/util/sort/bubble_sort_common.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.08.07
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_BUBBLE_COMMON_
#define DJINTERP_UTILITY_SORT_BUBBLE_COMMON_ 1


///////////////////////////////////////////////////////////////////////////////
///             I.    THE ALGORITHM                                         ///
///////////////////////////////////////////////////////////////////////////////

// D_BUBBLE_SORT_BODY
//   macro: THE bubble sort, and the only one.  Sorts the _count elements at
// _base into ascending order under _less.
//
//   Parameter(s):
//     _idx_t:    index type spanning [0, _count]; signed or unsigned.
//     _val_t:    the element type, for the carried element.
//     _less:     LESS(cmp, carry, base, i)  -- base[i] strictly precedes the
//                carried element.
//     _carry:    CARRY(carry, base, i)      -- take base[i] in hand.
//     _exchange: EXCHANGE(carry, base, i)   -- drop base[i] one place left and
//                put the carried element at i, keeping it in hand.
//     _cmp:      the ordering _less reads; any single token (may be unused).
//     _base:     the range; indexable as base[i] over [0, _count).
//     _count:    the element count.
//
//   THE PASS.  The carried element starts as base[0] and moves right.  When
// base[i] precedes it the two exchange, which drops the smaller element to
// i-1 and leaves the carried element at i -- still in hand, still the largest
// seen.  When base[i] does not precede it, base[i] becomes the new carried
// element.  Either way the loop advances, so a pass is exactly one traversal.
//
//   THE SHRINKING BOUND.  The index of the LAST exchange is the next pass's
// bound: nothing at or after it moved, so every element there is already at
// least as large as everything before it and is in final position.  Shrinking
// to the last exchange rather than by one is what makes nearly-sorted input
// cheap, and it subsumes the "did anything move?" flag -- a pass with no
// exchange leaves the bound at 0 and the loop ends.  That is the O(n) best
// case, and it costs no separate boolean to get.
//
//   STABILITY.  Only a STRICT precedence moves an element, so equivalents are
// never carried past one another and their relative order is preserved.  This
// is a property of the text below, so both faces have it.
//
//   _base, _count and _cmp are each evaluated exactly once, into a local.  The
// locals are prefixed d_bubble_ so a caller's own names cannot collide.
#define D_BUBBLE_SORT_BODY(_idx_t,                                          \
                           _val_t,                                          \
                           _less,                                           \
                           _carry,                                          \
                           _exchange,                                       \
                           _cmp,                                            \
                           _base,                                           \
                           _count)                                          \
    do                                                                      \
    {                                                                       \
        _idx_t d_bubble_end;                                                \
        _idx_t d_bubble_last;                                               \
        _idx_t d_bubble_i;                                                  \
                                                                            \
        d_bubble_end = (_idx_t)(_count);                                    \
                                                                            \
        /* a range of 0 or 1 elements is already sorted */                  \
        while (d_bubble_end > 1)                                            \
        {                                                                   \
            /* the pass carries the largest element seen so far; the    */  \
            /* bound guarantees at least two, so base[0] exists.  Copy- */  \
            /* initialised, so no element type needs a default ctor.    */  \
            _val_t d_bubble_carry = (_base)[0];                             \
                                                                            \
            d_bubble_last = 0;                                              \
                                                                            \
            for (d_bubble_i = 1;                                            \
                 d_bubble_i < d_bubble_end;                                 \
                 ++d_bubble_i)                                              \
            {                                                               \
                /* out of order: drop base[i] left, carry moves right */    \
                if (_less((_cmp), d_bubble_carry, (_base), d_bubble_i))     \
                {                                                           \
                    _exchange(d_bubble_carry, (_base), d_bubble_i);         \
                    d_bubble_last = d_bubble_i;                             \
                }                                                           \
                /* in order: base[i] is the new largest seen */             \
                else                                                        \
                {                                                           \
                    _carry(d_bubble_carry, (_base), d_bubble_i);            \
                }                                                           \
            }                                                               \
                                                                            \
            /* everything from the last exchange onward is final; a pass */ \
            /* with no exchange leaves 0 here and ends the sort          */ \
            d_bubble_end = d_bubble_last;                                   \
        }                                                                   \
    }                                                                       \
    while (0)


///////////////////////////////////////////////////////////////////////////////
///             II.   ELEMENT OPERATION HOOKS                               ///
///////////////////////////////////////////////////////////////////////////////
//   The hooks a face may pass to the body.  A face is free to write its own --
// the C++ face writes one, because a predicate object is not a `<` -- but the
// three below cover every typed instantiation in either language, which is why
// the two faces' expansions differ in exactly one hook.

// D_SORT_LESS_NATIVE
//   macro: LESS hook using the language's own `<`.  Ignores the ordering
// argument, so instantiations that use it pass any single token (0 reads
// clearly and emits nothing).
#define D_SORT_LESS_NATIVE(_cmp,                                            \
                           _carry,                                          \
                           _base,                                           \
                           _i)                                              \
    ( (_base)[_i] < (_carry) )

// D_SORT_CARRY_VALUE
//   macro: CARRY hook taking an element in hand by assignment.  The typed
// spelling; the erased path discards it, because on a run-time-width range the
// carried element is already the one at base[i-1].
#define D_SORT_CARRY_VALUE(_carry,                                          \
                           _base,                                           \
                           _i)                                              \
    ( (_carry) = (_base)[_i] )

// D_SORT_EXCHANGE_VALUE
//   macro: EXCHANGE hook.  Drops base[i] into the slot to its left and writes
// the carried element into the slot it vacated; the carried element stays in
// hand, since it may have further to travel.
//
//   Two assignments, not a three-assignment swap: the carried element is
// already held, so the temporary a swap would need is the thing being carried.
#define D_SORT_EXCHANGE_VALUE(_carry,                                       \
                              _base,                                        \
                              _i)                                           \
    do                                                                      \
    {                                                                       \
        (_base)[(_i) - 1] = (_base)[_i];                                    \
        (_base)[_i]       = (_carry);                                       \
    }                                                                       \
    while (0)


#endif  // DJINTERP_UTILITY_SORT_BUBBLE_COMMON_
