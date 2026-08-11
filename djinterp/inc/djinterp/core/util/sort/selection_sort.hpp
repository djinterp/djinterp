/******************************************************************************
* djinterp [utility]                                        selection_sort.hpp
*
*   Selection sort: the sequential driver.
* In-place, iterative, comparison-based, NOT stable.  Each position takes the
* minimum of everything that remains, exchanged in from wherever it was found.
* The driver is a single call into selection_sort_common.hpp, so a concurrent
* driver -- which parallelises the SCAN rather than the sort -- shares the
* primitives rather than restating them.
*
*   CHOOSE IT FOR THE WRITES, NEVER FOR THE COMPARISONS.  It always performs
* n(n-1)/2 comparisons, so insertion_sort beats it on every input, ordered or
* not.  What it offers is at most n-1 exchanges regardless of how disordered
* the input is: an element travelling from the end of the range to the front
* costs one exchange, where bubble and insertion would touch every slot
* between.  That trade pays when elements are large and comparison is cheap.
*
*   WHY THIS IS NOT THE C MODULE WITH A TEMPLATE ON TOP, AND WHY THE ANSWER IS
* DIFFERENT THIS TIME.  The practical half is unchanged: calling
* d_selection_sort would cost an indirect call per comparison, and this
* algorithm is nothing but comparisons, so it pays for that more heavily than
* the others do.
*
*   The other half changed.  For bubble and insertion the duplication was free
* of risk, because those are stable and "sorted and stable" names exactly one
* arrangement -- two correct implementations could not disagree.  Selection is
* NOT stable, so the sorted arrangement is not unique and two correct
* implementations genuinely can differ.  Agreement here is bought, not given,
* and what buys it is that both modules implement the same two normative
* choices: the scan runs forward, and only a strict precedence displaces the
* incumbent.  Both are stated in selection_sort_common.hpp, and the parity
* check exercises them -- for this algorithm that check can actually fail,
* which is not true of the two before it.
*
*   complexity:
*     best:       O(n^2)    (the scan cannot be cut short)
*     average:    O(n^2)
*     worst:      O(n^2)
*     exchanges:  O(n)      (at most n-1; the reason to choose it)
*     space:      O(1)
*     stable:     no
*
*   REQUIREMENTS.  _RandomIterator must be a random-access iterator; the
* element type must be swappable.  _Comparator must be a std::sort-convention
* binary predicate -- which every model of is_comparator is, so the composed
* comparators from the functional layer drop in unchanged:
*
*       selection_sort(v.begin(), v.end(), by_key(&person::age));
*
*   Note that composing a tie-break onto the comparator does NOT make this
* sort stable; it makes ties rarer.  Only a total order on the elements would,
* at which point stability has no meaning.
*
*
* path:      /djinterp/cpp/util/sort/selection_sort.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.03.22
*                                                         revised: 2026.08.10
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_SELECTION_HPP_
#define DJINTERP_UTILITY_SORT_SELECTION_HPP_ 1

// djinterp
#include "../../djinterp.hpp"
#include "./sort_common.hpp"
#include "./selection_sort_common.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                    I.   IMPLEMENTATION                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // selection_sort_apply
    //   function: performs selection sort on [_first, _last).
    //
    //   A single sweep is the whole sort, so like insertion_sort_apply and
    // unlike bubble_sort_apply there is no loop here.
    template<typename _RandomIterator,
             typename _Comparator>
    void selection_sort_apply(_RandomIterator _first,
                              _RandomIterator _last,
                              _Comparator     _comparator)
    {
        typedef typename std::iterator_traits<_RandomIterator>::difference_type
            difference_type;

        selection_pass(_first,
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
// A.  selection_sort(first, last, comp)
// ----------------------------------------------------------------------------

// selection_sort
//   function: sorts the range [_first, _last) using selection sort with the
// comparator _comparator.
template<typename _RandomIterator,
         typename _Comparator>
void selection_sort(_RandomIterator _first,
                    _RandomIterator _last,
                    _Comparator     _comparator)
{
    internal::selection_sort_apply(_first,
                                   _last,
                                   _comparator);

    return;
}

// ----------------------------------------------------------------------------
// B.  selection_sort(first, last)      (C++11+)
//     Uses operator< via less<value_type>.
// ----------------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// selection_sort
//   function: sorts the range [_first, _last) using selection sort with the
// default ascending comparator.
template<typename _RandomIterator>
void selection_sort(_RandomIterator _first,
                    _RandomIterator _last)
{
    typedef typename std::iterator_traits<_RandomIterator>::value_type
        value_type;

    internal::selection_sort_apply(_first,
                                   _last,
                                   less<value_type>());

    return;
}

#endif  // C++11

// ----------------------------------------------------------------------------
// C.  selection_sort_ordered(first, last, comp, order)
//     Wraps the comparator to honour sort_order at runtime.
// ----------------------------------------------------------------------------

// selection_sort_ordered
//   function: sorts the range [_first, _last) using selection sort, adapting
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
void selection_sort_ordered(_RandomIterator _first,
                            _RandomIterator _last,
                            _Comparator     _comparator,
                            sort_order      _order)
{
    internal::order_comparator<_Comparator> wrapped(_comparator, _order);

    internal::selection_sort_apply(_first,
                                   _last,
                                   wrapped);

    return;
}


NS_END  // djinterp


#endif  // DJINTERP_UTILITY_SORT_SELECTION_HPP_
