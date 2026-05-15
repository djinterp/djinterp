/******************************************************************************
* djinterp [restd]                                               unique_copy.hpp
*
* unique_copy algorithm header:
*   Out-of-place sibling of unique. Copies elements from
* [_first, _last) into the output range starting at _d_first, collapsing
* each run of consecutive duplicates to a single representative.
* Returns the iterator one past the last element written. Two overloads:
*   - default operator==
*   - custom binary predicate
*
*   IMPLEMENTATION NOTE:
*   std::unique_copy dispatches between three forms (based on whether
*   the input or output is a forward iterator) to decide where to
*   remember the "previous" value. restd takes the simplest correct
*   approach for every case: save the previous value in a local. This
*   imposes the requirement that the value type be copy-constructible
*   and copy-assignable (and for C++11+, optionally move-assignable),
*   which the std contract requires anyway for the "neither side is
*   forward" case.
*
*   PORTABILITY:
*   - std::unique_copy is C++98.
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*   - The local saved-value pattern naturally uses copy semantics; no
*     conditional move dance.
*
*
* path:      /inc/djinterp/restd/algorithm/unique_copy.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_UNIQUE_COPY_
#define DJINTERP_RESTD_ALGORITHM_UNIQUE_COPY_ 1

// djinterp
#include "../../core/djinterp.hpp"
// restd
#include "../iterator/iterator_traits.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================

#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14 constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


NS_RESTD


// ===========================================================================
// I.   UNIQUE_COPY (DEFAULT ==)
// ===========================================================================

// unique_copy
//   function: copies [_first, _last) to _d_first, collapsing runs of
// consecutive equal elements to one. Returns one past the last
// element written. The empty-input case returns _d_first unchanged.
template<typename _InputIt,
         typename _OutputIt>
D_CONSTEXPR_CPP14 _OutputIt
unique_copy(
    _InputIt  _first,
    _InputIt  _last,
    _OutputIt _d_first
)
{
    if (_first == _last)
    {
        return _d_first;
    }

    // remember the most recently written value to compare incoming
    // elements against; written-out type is the input value_type
    typedef typename iterator_traits<_InputIt>::value_type _Value;

    _Value _prev = *_first;
    *_d_first    = _prev;
    ++_d_first;
    ++_first;

    for (; _first != _last; ++_first)
    {
        if (!(_prev == *_first))
        {
            _prev     = *_first;
            *_d_first = _prev;
            ++_d_first;
        }
    }

    return _d_first;
}


// ===========================================================================
// II.  UNIQUE_COPY (CUSTOM PRED)
// ===========================================================================

// unique_copy (predicate)
//   function: as above but adjacent equality is determined by the
// user-supplied binary predicate _pred.
template<typename _InputIt,
         typename _OutputIt,
         typename _BinaryPred>
D_CONSTEXPR_CPP14 _OutputIt
unique_copy(
    _InputIt    _first,
    _InputIt    _last,
    _OutputIt   _d_first,
    _BinaryPred _pred
)
{
    if (_first == _last)
    {
        return _d_first;
    }

    typedef typename iterator_traits<_InputIt>::value_type _Value;

    _Value _prev = *_first;
    *_d_first    = _prev;
    ++_d_first;
    ++_first;

    for (; _first != _last; ++_first)
    {
        if (!_pred(_prev, *_first))
        {
            _prev     = *_first;
            *_d_first = _prev;
            ++_d_first;
        }
    }

    return _d_first;
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_UNIQUE_COPY_
