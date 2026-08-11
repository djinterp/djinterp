/******************************************************************************
* djinterp [utility]                                           bubble_sort.hpp
*
*   Bubble sort: the sequential driver.
* In-place, iterative, stable, comparison-based.  Each pass sweeps the adjacent
* pairs of the unsorted region and exchanges those out of order, which carries
* the largest remaining element to the region's end.  The driver is a loop
* around bubble_pass; everything it does lives in bubble_sort_common.hpp, so a
* concurrent driver added later shares those primitives rather than restating
* them.
*
*   WHY THIS IS NOT THE C MODULE WITH A TEMPLATE ON TOP.  Two reasons, one
* practical and one about what the algorithm guarantees.
*
*   The practical one: calling d_bubble_sort would cost everything the erased
* interface costs -- an indirect call per comparison and a width-driven
* exchange, neither visible to the optimiser, measured at roughly 3x.
*
*   The other one is why the duplication is safe.  Bubble sort is STABLE, and
* sortedness plus stability leaves exactly one possible arrangement of any
* input.  Two correct stable implementations therefore cannot disagree about
* the result: their agreement follows from the algorithm's contract, not from
* shared code.  (This does NOT extend to the unstable algorithms -- selection,
* quick and heap leave the arrangement of equivalent elements to choices no
* contract pins down, so the case for sharing text there must be made on its
* own.)
*
*   complexity:
*     best:     O(n)        (already sorted -- one pass, no exchanges)
*     average:  O(n^2)
*     worst:    O(n^2)
*     space:    O(1)        (one carried element)
*     stable:   yes         (only a STRICT precedence exchanges a pair)
*
*   REQUIREMENTS.  _RandomIterator must be a random-access iterator; the
* element type must be copy-constructible and copy-assignable.  _Comparator
* must be a std::sort-convention binary predicate -- which every model of
* is_comparator is, so the composed comparators from the functional layer drop
* in unchanged:
*
*       bubble_sort(v.begin(), v.end(),
*                   by_key(&person::age) | then(by_member(&person::name)));
*
*
* path:      /djinterp/cpp/util/sort/bubble_sort.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.03.22
*                                                         revised: 2026.08.10
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_BUBBLE_HPP_
#define DJINTERP_UTILITY_SORT_BUBBLE_HPP_ 1

// djinterp
#include "../../djinterp.hpp"
#include "./sort_common.hpp"
#include "./bubble_sort_common.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                    I.   IMPLEMENTATION                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // bubble_sort_apply
    //   function: performs bubble sort on [_first, _last).
    //
    //   THE SHRINKING BOUND.  Each pass returns the index of its last
    // exchange, and that becomes the next pass's bound: nothing at or after it
    // moved, so every element from there on is in final position.  A pass that
    // exchanges nothing returns 0, which ends the loop -- so the O(n) best
    // case on ordered input costs no separate test to obtain.
    template<typename _RandomIterator,
             typename _Comparator>
    void bubble_sort_apply(_RandomIterator _first,
                           _RandomIterator _last,
                           _Comparator     _comparator)
    {
        typedef typename std::iterator_traits<_RandomIterator>::difference_type
            difference_type;

        difference_type end;

        end = _last - _first;

        // a range of 0 or 1 elements is already sorted
        while (end > 1)
        {
            end = bubble_pass(_first,
                              static_cast<difference_type>(1),
                              end,
                              _comparator);
        }

        return;
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                    II.  ENTRY POINTS                                    ///
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// A.  bubble_sort(first, last, comp)
// ----------------------------------------------------------------------------

// bubble_sort
//   function: sorts the range [_first, _last) using bubble sort with the
// comparator _comparator.
template<typename _RandomIterator,
         typename _Comparator>
void bubble_sort(_RandomIterator _first,
                 _RandomIterator _last,
                 _Comparator     _comparator)
{
    internal::bubble_sort_apply(_first,
                                _last,
                                _comparator);

    return;
}

// ----------------------------------------------------------------------------
// B.  bubble_sort(first, last)      (C++11+)
//     Uses operator< via less<value_type>.
// ----------------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// bubble_sort
//   function: sorts the range [_first, _last) using bubble sort with the
// default ascending comparator.
template<typename _RandomIterator>
void bubble_sort(_RandomIterator _first,
                 _RandomIterator _last)
{
    typedef typename std::iterator_traits<_RandomIterator>::value_type
        value_type;

    internal::bubble_sort_apply(_first,
                                _last,
                                less<value_type>());

    return;
}

#endif  // C++11

// ----------------------------------------------------------------------------
// C.  bubble_sort_ordered(first, last, comp, order)
//     Wraps the comparator to honour sort_order at runtime.
// ----------------------------------------------------------------------------

// bubble_sort_ordered
//   function: sorts the range [_first, _last) using bubble sort, adapting
// _comparator to the requested _order.  The C++ counterpart of the C module's
// _order parameter: there the direction is passed to the call, here it is
// folded into the comparator, which costs the same and composes better.
//
//   Takes a sort_order rather than a bool, matching merge_sort_ordered,
// heap_sort_ordered and quick_sort_ordered, and matching what
// internal::order_comparator's constructor accepts -- the previous
// `bool _ascending` parameter could not compile, since sort_order is a scoped
// enum and admits no implicit conversion from bool.
template<typename _RandomIterator,
         typename _Comparator>
void bubble_sort_ordered(_RandomIterator _first,
                         _RandomIterator _last,
                         _Comparator     _comparator,
                         sort_order      _order)
{
    internal::order_comparator<_Comparator> wrapped(_comparator, _order);

    internal::bubble_sort_apply(_first,
                                _last,
                                wrapped);

    return;
}


NS_END  // djinterp


#endif  // DJINTERP_UTILITY_SORT_BUBBLE_HPP_
