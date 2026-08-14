/******************************************************************************
* djinterp [restd]                                                  mismatch.hpp
*
* mismatch algorithm header:
*   Walks two ranges in parallel and returns the first position where
* they differ. Four overloads:
*   - 3-arg, default operator==        (C++98)
*   - 3-arg, custom binary predicate   (C++98)
*   - 4-arg, default operator==        (C++14 in std; back-ported)
*   - 4-arg, custom binary predicate   (C++14 in std; back-ported)
*
*   PORTABILITY:
*   - std::mismatch is C++98 for the 3-arg forms; the 4-arg forms (with
*     a second end iterator) were added in C++14. restd back-ports the
*     4-arg forms to C++98 (no language blocker).
*   - Return type is restd::pair<_InputIt1, _InputIt2>.
*   - constexpr in std from C++20 (P0202); restd lifts to C++14.
*
*
* path:      /inc/djinterp/re_std/algorithm/mismatch.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.13
******************************************************************************/

#ifndef DJINTERP_RESTD_ALGORITHM_MISMATCH_
#define DJINTERP_RESTD_ALGORITHM_MISMATCH_ 1

// djinterp
#include "../../core/djinterp.hpp"
// restd
#include "../utility/pair.hpp"


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
// I.   MISMATCH (3-ARG, DEFAULT ==)
// ===========================================================================

// mismatch
//   function: walks [_first1, _last1) against the parallel range
// starting at _first2, returning the first pair of positions whose
// elements do not compare equal. Second range is assumed long enough.
template<typename _InputIt1,
         typename _InputIt2>
D_CONSTEXPR_CPP14 pair<_InputIt1, _InputIt2>
mismatch(
    _InputIt1 _first1,
    _InputIt1 _last1,
    _InputIt2 _first2
)
{
    while ( (_first1 != _last1) &&
            (*_first1 == *_first2) )
    {
        ++_first1;
        ++_first2;
    }

    return pair<_InputIt1, _InputIt2>(_first1, _first2);
}


// ===========================================================================
// II.  MISMATCH (3-ARG + CUSTOM PRED)
// ===========================================================================

// mismatch (predicate)
//   function: as above, but element comparison is via the user-supplied
// binary predicate _pred.
template<typename _InputIt1,
         typename _InputIt2,
         typename _BinaryPred>
D_CONSTEXPR_CPP14 pair<_InputIt1, _InputIt2>
mismatch(
    _InputIt1   _first1,
    _InputIt1   _last1,
    _InputIt2   _first2,
    _BinaryPred _pred
)
{
    while ( (_first1 != _last1) &&
            _pred(*_first1, *_first2) )
    {
        ++_first1;
        ++_first2;
    }

    return pair<_InputIt1, _InputIt2>(_first1, _first2);
}


// ===========================================================================
// III. MISMATCH (4-ARG, DEFAULT ==)
// ===========================================================================

// mismatch (two ranges)
//   function: walks [_first1, _last1) against [_first2, _last2),
// stopping at whichever range exhausts first. Returns the first pair of
// positions whose elements do not compare equal.
template<typename _InputIt1,
         typename _InputIt2>
D_CONSTEXPR_CPP14 pair<_InputIt1, _InputIt2>
mismatch(
    _InputIt1 _first1,
    _InputIt1 _last1,
    _InputIt2 _first2,
    _InputIt2 _last2
)
{
    while ( (_first1 != _last1) &&
            (_first2 != _last2) &&
            (*_first1 == *_first2) )
    {
        ++_first1;
        ++_first2;
    }

    return pair<_InputIt1, _InputIt2>(_first1, _first2);
}


// ===========================================================================
// IV.  MISMATCH (4-ARG + CUSTOM PRED)
// ===========================================================================

// mismatch (two ranges, predicate)
//   function: as the 4-arg form, but element comparison is via the
// user-supplied binary predicate _pred.
template<typename _InputIt1,
         typename _InputIt2,
         typename _BinaryPred>
D_CONSTEXPR_CPP14 pair<_InputIt1, _InputIt2>
mismatch(
    _InputIt1   _first1,
    _InputIt1   _last1,
    _InputIt2   _first2,
    _InputIt2   _last2,
    _BinaryPred _pred
)
{
    while ( (_first1 != _last1) &&
            (_first2 != _last2) &&
            _pred(*_first1, *_first2) )
    {
        ++_first1;
        ++_first2;
    }

    return pair<_InputIt1, _InputIt2>(_first1, _first2);
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ALGORITHM_MISMATCH_
