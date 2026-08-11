/******************************************************************************
* djinterp [utility]                                            merge_sort.hpp
*
*   Merge sort: the sequential driver.
* Stable, comparison-based, O(n log n) in every case -- no input defeats it,
* which is what separates it from quicksort.  The only algorithm in the
* subsystem that is not in place, and so the only one whose C++ entry point
* acquires storage.
*
*   WHO OWNS THE BUFFER.  The C module cannot allocate, so it takes the scratch
* and reports D_SORT_STATUS_BUFFER_TOO_SMALL.  A C++ caller writing
* merge_sort(v.begin(), v.end()) expects it to work, so the plain entry points
* acquire a buffer and release it, and merge_sort_buffered takes one instead
* for a caller who is sorting in a loop and would rather reuse storage than
* reacquire it.  Same algorithm, two ownership stories.
*
*   The buffer is a std::vector copy-constructed from the range rather than a
* new[] array.  Two reasons: new[] would require the element type to be default
* constructible, which merge sort has no need of, and it would leak the array
* if a comparison threw between the allocation and the delete[].  Copying the
* range in also means both halves of the ping-pong start holding valid objects,
* so no pass ever assigns over an uninitialised one.
*
*   complexity:
*     best:       O(n)      comparisons (ordered input -- an ordered run pair
*                           is detected in one comparison and copied)
*     average:    O(n log n)
*     worst:      O(n log n)   (guaranteed)
*     space:      O(n)      auxiliary
*     stable:     yes
*
*   WHEN TO REACH FOR IT.  When the worst case matters -- it has no bad input,
* where quicksort does -- or when stability is required at O(n log n), which no
* other algorithm here offers.  The price is the buffer.
*
*   REQUIREMENTS.  _RandomIterator must be a random-access iterator; the
* element type must be copy-constructible and copy-assignable.  _Comparator
* must be a std::sort-convention binary predicate, so the composed comparators
* from the functional layer drop in unchanged:
*
*       merge_sort(v.begin(), v.end(),
*                  by_key(&person::age) | then(by_member(&person::name)));
*
*
* path:      /djinterp/cpp/util/sort/merge_sort.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                         created: 2026.03.22
*                                                         revised: 2026.08.10
******************************************************************************/

#ifndef DJINTERP_UTILITY_SORT_MERGE_HPP_
#define DJINTERP_UTILITY_SORT_MERGE_HPP_ 1

// std
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "./sort_common.hpp"
#include "./merge_sort_common.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                    I.   IMPLEMENTATION                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // merge_sort_apply
    //   function: performs merge sort on [_first, _last) using [_buffer,
    // _buffer + (_last - _first)) as scratch.
    //
    //   THE PING-PONG.  A merge cannot write into what it reads, so each pass
    // reads one buffer and writes the other and the two exchange roles.  The
    // alternative -- merge into scratch, copy back, repeat -- is correct and
    // copies every element twice per pass instead of once.
    //
    //   The two buffers are different types, so the alternation is a branch
    // over two instantiations rather than a swap of pointers.  That costs one
    // predictable test per pass, which is log n tests for the whole sort.
    //
    //   After the loop the sorted elements are wherever the last pass wrote
    // them, so the range is written back only when that was the scratch.
    template<typename _RandomIterator,
             typename _BufferIterator,
             typename _Comparator>
    void merge_sort_apply(_RandomIterator _first,
                          _RandomIterator _last,
                          _BufferIterator _buffer,
                          _Comparator     _comparator)
    {
        typedef typename std::iterator_traits<_RandomIterator>::difference_type
            difference_type;

        difference_type count;
        difference_type width;
        difference_type index;
        bool            in_buffer;

        count = _last - _first;

        // a range of 0 or 1 elements is already sorted
        if (count < 2)
        {
            return;
        }

        in_buffer = false;

        // runs of width 1 are ordered by definition; each pass doubles the
        // width until one run covers the range
        for (width = 1; width < count; width *= 2)
        {
            if (!in_buffer)
            {
                merge_pass(_first, _buffer, count, width, _comparator);
            }
            else
            {
                merge_pass(_buffer, _first, count, width, _comparator);
            }

            in_buffer = !in_buffer;
        }

        // bring the result home only if the last pass left it in the scratch
        if (in_buffer)
        {
            for (index = 0; index < count; ++index)
            {
                _first[index] = _buffer[index];
            }
        }

        return;
    }

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                    II.  ENTRY POINTS                                    ///
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// A.  merge_sort(first, last, comp)
// ----------------------------------------------------------------------------

// merge_sort
//   function: sorts the range [_first, _last) using merge sort with the
// comparator _comparator, acquiring and releasing its own scratch.
template<typename _RandomIterator,
         typename _Comparator>
void merge_sort(_RandomIterator _first,
                _RandomIterator _last,
                _Comparator     _comparator)
{
    typedef typename std::iterator_traits<_RandomIterator>::value_type
        value_type;

    // a range of 0 or 1 elements is already sorted, and buying a buffer to
    // discover that would be the only allocation this header could avoid
    if ((_last - _first) < 2)
    {
        return;
    }

    // copy-constructed from the range: no default constructor is required of
    // the element type, and nothing leaks if a comparison throws
    std::vector<value_type> buffer(_first, _last);

    internal::merge_sort_apply(_first,
                               _last,
                               buffer.begin(),
                               _comparator);

    return;
}

// ----------------------------------------------------------------------------
// B.  merge_sort(first, last)      (C++11+)
//     Uses operator< via less<value_type>.
// ----------------------------------------------------------------------------

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// merge_sort
//   function: sorts the range [_first, _last) using merge sort with the
// default ascending comparator.
template<typename _RandomIterator>
void merge_sort(_RandomIterator _first,
                _RandomIterator _last)
{
    typedef typename std::iterator_traits<_RandomIterator>::value_type
        value_type;

    merge_sort(_first,
               _last,
               less<value_type>());

    return;
}

#endif  // C++11

// ----------------------------------------------------------------------------
// C.  merge_sort_buffered(first, last, comp, buffer)
//     The caller's scratch; nothing is acquired.
// ----------------------------------------------------------------------------

// merge_sort_buffered
//   function: sorts the range [_first, _last) using merge sort with the
// caller's scratch, which must hold at least (_last - _first) elements and
// must already hold constructed objects of the element type.
//
//   The C++ counterpart of the C module's two-call protocol: there the caller
// measures and supplies bytes, here it supplies a range.  For a caller sorting
// repeatedly this reuses one buffer instead of acquiring a new one per call;
// there is no other difference, and no status to check, because a buffer that
// is too short is a precondition violation rather than a run-time condition.
template<typename _RandomIterator,
         typename _BufferIterator,
         typename _Comparator>
void merge_sort_buffered(_RandomIterator _first,
                         _RandomIterator _last,
                         _Comparator     _comparator,
                         _BufferIterator _buffer)
{
    internal::merge_sort_apply(_first,
                               _last,
                               _buffer,
                               _comparator);

    return;
}

// ----------------------------------------------------------------------------
// D.  merge_sort_ordered(first, last, comp, order)
//     Wraps the comparator to honour sort_order at runtime.
// ----------------------------------------------------------------------------

// merge_sort_ordered
//   function: sorts the range [_first, _last) using merge sort, adapting
// _comparator to the requested _order.  The C++ counterpart of the C module's
// _order parameter: there the direction is passed to the call, here it is
// folded into the comparator, which costs the same and composes better.
template<typename _RandomIterator,
         typename _Comparator>
void merge_sort_ordered(_RandomIterator _first,
                        _RandomIterator _last,
                        _Comparator     _comparator,
                        sort_order      _order)
{
    internal::order_comparator<_Comparator> wrapped(_comparator, _order);

    merge_sort(_first,
               _last,
               wrapped);

    return;
}


NS_END  // djinterp


#endif  // DJINTERP_UTILITY_SORT_MERGE_HPP_
