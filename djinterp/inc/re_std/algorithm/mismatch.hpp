/*******************************************************************************
* djinterp [re_std]                                                 mismatch.hpp
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
*     a second end iterator) were added in C++14. re_std back-ports the
*     4-arg forms to C++98 (no language blocker).
*   - Return type is re_std::pair<InputIt1, InputIt2>.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*
*
* path:      /inc/re_std/algorithm/mismatch.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_MISMATCH_HPP
#define RE_STD_ALGORITHM_MISMATCH_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
// re_std
#include "../utility/pair.hpp"


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


// ===========================================================================
// I.   MISMATCH (3-ARG, DEFAULT ==)
// ===========================================================================

// mismatch
//   function: walks [_first1, _last1) against the parallel range
// starting at _first2, returning the first pair of positions whose
// elements do not compare equal. Second range is assumed long enough.
template<typename InputIt1,
         typename InputIt2>
RE_STD_CONSTEXPR_CPP14 pair<InputIt1, InputIt2>
mismatch(
    InputIt1 _first1,
    InputIt1 _last1,
    InputIt2 _first2
)
{
    while ( (_first1 != _last1) &&
            (*_first1 == *_first2) )
    {
        ++_first1;
        ++_first2;
    }

    return pair<InputIt1, InputIt2>(_first1, _first2);
}


// ===========================================================================
// II.  MISMATCH (3-ARG + CUSTOM PRED)
// ===========================================================================

// mismatch (predicate)
//   function: as above, but element comparison is via the user-supplied
// binary predicate _pred.
template<typename InputIt1,
         typename InputIt2,
         typename BinaryPred>
RE_STD_CONSTEXPR_CPP14 pair<InputIt1, InputIt2>
mismatch(
    InputIt1   _first1,
    InputIt1   _last1,
    InputIt2   _first2,
    BinaryPred _pred
)
{
    while ( (_first1 != _last1) &&
            _pred(*_first1, *_first2) )
    {
        ++_first1;
        ++_first2;
    }

    return pair<InputIt1, InputIt2>(_first1, _first2);
}


// ===========================================================================
// III. MISMATCH (4-ARG, DEFAULT ==)
// ===========================================================================

// mismatch (two ranges)
//   function: walks [_first1, _last1) against [_first2, _last2),
// stopping at whichever range exhausts first. Returns the first pair of
// positions whose elements do not compare equal.
template<typename InputIt1,
         typename InputIt2>
RE_STD_CONSTEXPR_CPP14 pair<InputIt1, InputIt2>
mismatch(
    InputIt1 _first1,
    InputIt1 _last1,
    InputIt2 _first2,
    InputIt2 _last2
)
{
    while ( (_first1 != _last1) &&
            (_first2 != _last2) &&
            (*_first1 == *_first2) )
    {
        ++_first1;
        ++_first2;
    }

    return pair<InputIt1, InputIt2>(_first1, _first2);
}


// ===========================================================================
// IV.  MISMATCH (4-ARG + CUSTOM PRED)
// ===========================================================================

// mismatch (two ranges, predicate)
//   function: as the 4-arg form, but element comparison is via the
// user-supplied binary predicate _pred.
template<typename InputIt1,
         typename InputIt2,
         typename BinaryPred>
RE_STD_CONSTEXPR_CPP14 pair<InputIt1, InputIt2>
mismatch(
    InputIt1   _first1,
    InputIt1   _last1,
    InputIt2   _first2,
    InputIt2   _last2,
    BinaryPred _pred
)
{
    while ( (_first1 != _last1) &&
            (_first2 != _last2) &&
            _pred(*_first1, *_first2) )
    {
        ++_first1;
        ++_first2;
    }

    return pair<InputIt1, InputIt2>(_first1, _first2);
}


}  // re_std


#endif  // RE_STD_ALGORITHM_MISMATCH_HPP
