/******************************************************************************
* djinterp [utility]                                    bubble_sort_common.hpp
*
*   The primitives every bubble sort is built from, sequential or concurrent.
* A bubble sort is a driver wrapped around two operations: exchange an adjacent
* pair if it is out of order, and sweep a run of adjacent pairs doing so.  What
* distinguishes one bubble sort from another is only which pairs get swept, in
* what order, and by how many threads.
*
*     - The sequential driver (bubble_sort.hpp) sweeps [1, end) repeatedly,
*       shrinking `end` to the last exchange each time.
*     - A block-partitioned concurrent driver hands each worker a sub-range and
*       calls bubble_pass on it, then reconciles the boundaries.
*     - An odd-even transposition driver alternates the phases (0,1),(2,3),...
*       and (1,2),(3,4),...  Within a phase every pair is disjoint, so the
*       pairs may be exchanged in any order or all at once -- which is what
*       makes that algorithm parallel.  It uses bubble_compare_exchange
*       directly and never needs a sweep.
*
*   THE CARRIED ELEMENT.  bubble_pass keeps the element it is carrying in a
* local rather than re-reading it from the range on every comparison.  Same
* comparisons, same order, same result -- but it halves the loads, and it keeps
* the exchange from being a read-modify-write of two ADJACENT slots, which
* GCC's SLP vectoriser folds into a paired load and shuffle whose overlapping
* store-to-load forwarding then stalls.  Measured at ~46 ms against ~54 ms for
* the textbook shape on 9,000 random ints (GCC 13, -O2).
*
*   The C module cannot do this: with the element width known only at run time
* there is nowhere to put the carried element, so it re-reads it from the range
* -- which is exactly where the carried element already sits.  This is why the
* two languages have separate implementations rather than one shared text; see
* the note in bubble_sort.hpp.
*
*   THE ATOM IS WHERE STABILITY LIVES.  bubble_compare_exchange exchanges only
* on a STRICT precedence, so equivalent elements are never moved past one
* another, and every driver built on it inherits that.
*
*
* path:      /djinterp/cpp/util/sort/bubble_sort_common.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.08.10
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_BUBBLE_COMMON_HPP_
#define DJINTERP_UTILITY_SORT_BUBBLE_COMMON_HPP_ 1

// djinterp
#include "../../djinterp.hpp"
#include "./sort_common.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                    I.   ALGORITHM TRAITS                                ///
///////////////////////////////////////////////////////////////////////////////

// bubble_sort_traits
//   struct: compile-time properties of the bubble sort family, so a caller
// choosing between algorithms can branch on the property rather than the name.
// The C module publishes the same three as D_BUBBLE_SORT_IS_STABLE,
// _IS_IN_PLACE and _IS_ADAPTIVE.
//   - is_stable:   yes  (only a strict precedence exchanges a pair)
//   - is_in_place: yes  (O(1) auxiliary memory; one carried element)
//   - is_adaptive: yes  (a pass reports its last exchange, so a driver can
//                        shrink its bound and finish ordered input in one)
struct bubble_sort_traits
{
    static const bool is_stable   = true;
    static const bool is_in_place = true;
    static const bool is_adaptive = true;
};


///////////////////////////////////////////////////////////////////////////////
///                    II.  PRIMITIVES                                      ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // bubble_compare_exchange
    //   function: exchanges the pair at _earlier and _later when the later
    // element strictly precedes the earlier one, and reports whether it did.
    //
    //   The atom of every bubble sort.  STRICT precedence is what makes the
    // family stable: an exchange on equivalence would reorder equal elements,
    // so this is the one comparison no driver should open-code.
    template<typename _RandomIterator,
             typename _Comparator>
    bool bubble_compare_exchange(_RandomIterator _earlier,
                                 _RandomIterator _later,
                                 _Comparator     _comparator)
    {
        // out of order only when the LATER element strictly precedes
        if (!_comparator(*_later, *_earlier))
        {
            return false;
        }

        std::iter_swap(_earlier, _later);

        return true;
    }

    // bubble_pass
    //   function: sweeps the adjacent pairs (i-1, i) for i in [_begin, _end),
    // exchanging each pair that is out of order, and returns the index of the
    // last exchange -- or 0 when the sweep exchanged nothing.
    //
    //   That return is what makes the family adaptive.  Nothing at or after
    // the last exchange moved, so a driver may take it as the next bound
    // rather than stepping down by one; a sweep that exchanged nothing returns
    // 0, which ends the driver's loop without a separate flag.
    //
    //   The carried element is held across the sweep and written only when it
    // moves, so the exchange is two assignments rather than a three-assignment
    // swap: the temporary a swap would need is the thing already in hand.
    //
    //   The caller owns the range.  A pass does not check it, because it is
    // called once per pass by a driver that checked once per sort.
    template<typename _RandomIterator,
             typename _Difference,
             typename _Comparator>
    _Difference bubble_pass(_RandomIterator _first,
                            _Difference     _begin,
                            _Difference     _end,
                            _Comparator     _comparator)
    {
        typedef typename std::iterator_traits<_RandomIterator>::value_type
            value_type;

        _Difference last_exchange;
        _Difference index;

        // an empty or degenerate run has no pairs to sweep
        if ( (_begin < 1)     ||
             (_end <= _begin) )
        {
            return 0;
        }

        last_exchange = 0;

        // the element being carried right.  Copy-initialised here rather than
        // declared above, so the element type needs no default constructor.
        value_type carried = _first[_begin - 1];

        for (index = _begin; index < _end; ++index)
        {
            // out of order: the smaller element drops back one place and the
            // carried element takes the slot it vacated, still in hand
            if (_comparator(_first[index], carried))
            {
                _first[index - 1] = _first[index];
                _first[index]     = carried;

                last_exchange = index;
            }
            // in order: this element is the new largest seen
            else
            {
                carried = _first[index];
            }
        }

        return last_exchange;
    }

NS_END  // internal


NS_END  // djinterp


#endif  // DJINTERP_UTILITY_SORT_BUBBLE_COMMON_HPP_
