/******************************************************************************
* djinterp [utility]                                        insertion_sort.hpp
*
*   Insertion sort: the sequential driver.
* In-place, iterative, stable, comparison-based.  Each element is placed into
* the ordered run growing behind it, so the sorted prefix extends by exactly
* one element per step until it is the whole range.  The driver is a single
* call into insertion_sort_common.hpp, so a concurrent driver -- or quicksort's
* small-range fallback -- shares the primitives rather than restating them.
*
*   ONE SWEEP, NOT MANY.  Insertion and bubble move elements the same way, one
* place at a time between adjacent neighbours, and differ in when they stop.
* Bubble sweeps until a sweep exchanges nothing; insertion sweeps once, because
* each step leaves the prefix fully ordered rather than only pushing one
* element to its end.  That, plus assigning rather than exchanging, is why
* insertion beats bubble on the same input despite sharing its complexity
* class.
*
*   WHY THIS IS NOT THE C MODULE WITH A TEMPLATE ON TOP.  Calling
* d_insertion_sort would cost what the erased interface costs -- an indirect
* call per comparison and a width-driven shift, neither visible to the
* optimiser.  The duplication is safe because insertion sort is STABLE, and
* sortedness plus stability leaves exactly one possible arrangement of any
* input: two correct stable implementations cannot disagree about the result.
* Their agreement follows from the algorithm's contract, not from shared code.
*
*   complexity:
*     best:     O(n)        (already sorted -- one comparison per element,
*                            and no writes at all)
*     average:  O(n^2)
*     worst:    O(n^2)
*     space:    O(1)        (one held candidate)
*     stable:   yes         (a candidate never travels past an equivalent)
*
*   REQUIREMENTS.  _RandomIterator must be a random-access iterator; the
* element type must be copy-constructible and copy-assignable.  _Comparator
* must be a std::sort-convention binary predicate -- which every model of
* is_comparator is, so the composed comparators from the functional layer drop
* in unchanged:
*
*       insertion_sort(v.begin(), v.end(),
*                      by_key(&person::age) | then(by_member(&person::name)));
*
*
* path:      /djinterp/cpp/util/sort/insertion_sort.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.03.22
*                                                         revised: 2026.08.10
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_INSERTION_HPP_
#define DJINTERP_UTILITY_SORT_INSERTION_HPP_ 1

// djinterp
#include "../../djinterp.hpp"
#include "./sort_common.hpp"
#include "./insertion_sort_common.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                    I.   IMPLEMENTATION                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // insertion_sort_apply
    //   function: performs insertion sort on [_first, _last).
    //
    //   A single sweep is the whole sort, so unlike bubble_sort_apply there is
    // no loop here.  What would be the driver's loop lives inside placement
    // instead, as the backward scan that finds where one element belongs.
    //
    //   quick_sort.hpp calls this for the sub-ranges it stops subdividing, so
    // the name is load-bearing beyond this header.
    template<typename _RandomIterator,
             typename _Comparator>
    void insertion_sort_apply(_RandomIterator _first,
                              _RandomIterator _last,
                              _Comparator     _comparator)
    {
        typedef typename std::iterator_traits<_RandomIterator>::difference_type
            difference_type;

        insertion_pass(_first,
                       static_cast<difference_type>(0),
                       _last - _first,
                       _comparator);

        return;
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                    II.  ENTRY POINTS                                    ///
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// A.  insertion_sort(first, last, comp)
// ----------------------------------------------------------------------------

// insertion_sort
//   function: sorts the range [_first, _last) using insertion sort with the
// comparator _comparator.
template<typename _RandomIterator,
         typename _Comparator>
void insertion_sort(_RandomIterator _first,
                    _RandomIterator _last,
                    _Comparator     _comparator)
{
    internal::insertion_sort_apply(_first,
                                   _last,
                                   _comparator);

    return;
}

// ----------------------------------------------------------------------------
// B.  insertion_sort(first, last)      (C++11+)
//     Uses operator< via less<value_type>.
// ----------------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// insertion_sort
//   function: sorts the range [_first, _last) using insertion sort with the
// default ascending comparator.
template<typename _RandomIterator>
void insertion_sort(_RandomIterator _first,
                    _RandomIterator _last)
{
    typedef typename std::iterator_traits<_RandomIterator>::value_type
        value_type;

    internal::insertion_sort_apply(_first,
                                   _last,
                                   less<value_type>());

    return;
}

#endif  // C++11

// ----------------------------------------------------------------------------
// C.  insertion_sort_ordered(first, last, comp, order)
//     Wraps the comparator to honour sort_order at runtime.
// ----------------------------------------------------------------------------

// insertion_sort_ordered
//   function: sorts the range [_first, _last) using insertion sort, adapting
// _comparator to the requested _order.  The C++ counterpart of the C module's
// _order parameter: there the direction is passed to the call, here it is
// folded into the comparator, which costs the same and composes better.
//
//   Takes a sort_order rather than a bool, matching the other _ordered entry
// points and matching what internal::order_comparator's constructor accepts --
// the previous `bool _ascending` parameter could not compile, since sort_order
// is a scoped enum and admits no implicit conversion from bool.
template<typename _RandomIterator,
         typename _Comparator>
void insertion_sort_ordered(_RandomIterator _first,
                            _RandomIterator _last,
                            _Comparator     _comparator,
                            sort_order      _order)
{
    internal::order_comparator<_Comparator> wrapped(_comparator, _order);

    internal::insertion_sort_apply(_first,
                                   _last,
                                   wrapped);

    return;
}


NS_END  // djinterp


#endif  // DJINTERP_UTILITY_SORT_INSERTION_HPP_
