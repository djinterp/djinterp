/******************************************************************************
* djinterp [utility]                                  insertion_sort_common.hpp
*
*   The primitives every insertion sort is built from, sequential or otherwise.
* An insertion sort is a driver wrapped around two operations: place one
* element into the ordered run behind it, and do that for each element of a
* run.
*
*     - The sequential driver (insertion_sort.hpp) places every element of
*       [0, count) into the run growing behind it.  One sweep is the whole
*       sort, which is the difference between this family and the bubble
*       family: both move elements one place at a time between adjacent
*       neighbours, but bubble needs O(n) sweeps to finish and insertion needs
*       exactly one.
*
*     - A block-partitioned concurrent driver hands each worker a sub-range and
*       calls insertion_pass on it, then merges the ordered blocks -- the
*       standard first phase of a parallel merge sort, and the reason this is a
*       range operation rather than a whole-array one.
*
*     - quick_sort.hpp calls into this family for the sub-ranges it stops
*       subdividing, where insertion's low constant factor wins outright.
*
*   PLACEMENT SHIFTS; IT DOES NOT SWAP.  The candidate is moved out once, the
* run is assigned over the slot it vacated, and it is moved back -- k+2 element
* assignments to travel k places, against the 3k a chain of swaps would cost.
* This is what separates insertion from bubble in cost rather than in shape,
* and it is why insertion_place holds the candidate rather than calling
* bubble_compare_exchange in a loop.
*
*   STABILITY LIVES IN THE SCAN.  Placement stops at the first element the
* candidate does NOT strictly precede, so a candidate never travels past an
* equivalent and equal elements keep their input order.
*
*
* path:      /djinterp/cpp/util/sort/insertion_sort_common.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.08.10
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_INSERTION_COMMON_HPP_
#define DJINTERP_UTILITY_SORT_INSERTION_COMMON_HPP_ 1

// djinterp
#include "../../djinterp.hpp"
#include "./sort_common.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                    I.   ALGORITHM TRAITS                                ///
///////////////////////////////////////////////////////////////////////////////

// insertion_sort_traits
//   struct: compile-time properties of the insertion sort family, so a caller
// choosing between algorithms can branch on the property rather than the name.
// The C module publishes the same three as D_INSERTION_SORT_IS_STABLE,
// _IS_IN_PLACE and _IS_ADAPTIVE.
//   - is_stable:   yes  (a candidate never travels past an equivalent)
//   - is_in_place: yes  (O(1) auxiliary memory; one held candidate)
//   - is_adaptive: yes  (an element already in order is never written)
struct insertion_sort_traits
{
    static const bool is_stable   = true;
    static const bool is_in_place = true;
    static const bool is_adaptive = true;
};


///////////////////////////////////////////////////////////////////////////////
///                    II.  PRIMITIVES                                      ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // insertion_place
    //   function: places the element at _index into the ordered run
    // [_lower, _index) and returns the index it landed at.
    //
    //   The candidate is compared in place first, so an element already in
    // order relative to its predecessor is never moved out at all -- that test
    // is what gives ordered input its O(n) best case and its zero writes.
    //
    //   Otherwise the candidate is held while the run is assigned forward over
    // the slot it vacated.  The loop stops at the first element the candidate
    // does not strictly precede, which is what keeps equivalents in their
    // input order.
    template<typename _RandomIterator,
             typename _Difference,
             typename _Comparator>
    _Difference insertion_place(_RandomIterator _first,
                                _Difference     _lower,
                                _Difference     _index,
                                _Comparator     _comparator)
    {
        typedef typename std::iterator_traits<_RandomIterator>::value_type
            value_type;

        _Difference hole;

        // the run's first element has nothing behind it to be placed among
        if (_index <= _lower)
        {
            return _index;
        }

        // already in order relative to its predecessor: the O(n) best case
        if (!_comparator(_first[_index], _first[_index - 1]))
        {
            return _index;
        }

        hole = _index;

        // the candidate, held out of the range while the run moves over the
        // slot it vacated.  Copy-initialised, so the element type needs no
        // default constructor.
        value_type candidate = _first[_index];

        // walk back over everything the candidate strictly precedes
        while ( (hole > _lower) &&
                (_comparator(candidate, _first[hole - 1])) )
        {
            _first[hole] = _first[hole - 1];

            --hole;
        }

        _first[hole] = candidate;

        return hole;
    }

    // insertion_pass
    //   function: sorts the run [_begin, _end) by placing each of its elements
    // into the ordered prefix growing behind it.
    //
    //   One sweep is the whole sort, which is where this differs from
    // bubble_pass: that one must be called until it reports no exchange, this
    // one is called once.
    //
    //   The caller owns the range; a pass does not check it, because it is
    // called by a driver that checked once per sort.
    template<typename _RandomIterator,
             typename _Difference,
             typename _Comparator>
    void insertion_pass(_RandomIterator _first,
                        _Difference     _begin,
                        _Difference     _end,
                        _Comparator     _comparator)
    {
        _Difference index;

        // a run of 0 or 1 elements is already sorted
        if ((_end - _begin) < 2)
        {
            return;
        }

        // the run's first element is the ordered prefix; place each of the
        // rest into it
        for (index = _begin + 1; index < _end; ++index)
        {
            insertion_place(_first,
                            _begin,
                            index,
                            _comparator);
        }

        return;
    }

NS_END  // internal


NS_END  // djinterp


#endif  // DJINTERP_UTILITY_SORT_INSERTION_COMMON_HPP_
