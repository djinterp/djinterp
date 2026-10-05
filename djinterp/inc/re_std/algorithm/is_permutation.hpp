/*******************************************************************************
* djinterp [re_std]                                           is_permutation.hpp
*
* is_permutation algorithm header:
*   True iff the second range is a rearrangement of the first.
*
*   Four overloads:
*     is_permutation(f1, l1, f2)                 3-arg, operator==
*     is_permutation(f1, l1, f2, pred)           3-arg, predicate
*     is_permutation(f1, l1, f2, l2)             4-arg, operator==
*     is_permutation(f1, l1, f2, l2, pred)       4-arg, predicate
*
*   STRATEGY:
*   Strip the common matching prefix first -- the usual case where the
* ranges are equal then costs O(N) and never reaches the quadratic
* phase. For the remaining suffix, each distinct element is counted in
* both ranges and the counts compared; elements already seen earlier
* in the suffix are skipped so each distinct value is counted once.
* Worst case O(N^2), which is what the standard permits for forward
* iterators.
*
*   PORTABILITY:
*   - std::is_permutation is C++11 (3-arg); the 4-arg forms arrived in
*     C++14. re_std back-ports all four to C++98.
*   - constexpr in std from C++20 (P0202); re_std lifts to C++14.
*   - The 4-arg forms compare lengths up front and return false on a
*     mismatch, so an unequal-length pair costs only the distance walk.
*
*   The predicate must be an EQUIVALENCE relation, not an ordering.
*
*
* path:      /inc/re_std/algorithm/is_permutation.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.24
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ALGORITHM_IS_PERMUTATION_HPP
#define RE_STD_ALGORITHM_IS_PERMUTATION_HPP 1

// std
#include <cstddef>
// re_std
#include "../config.hpp"  // RE_STD_* configuration


// ===========================================================================
// 0.   COMPATIBILITY MACROS
// ===========================================================================


namespace re_std
{


namespace internal
{

    // is_permutation_tail
    //   function: counting comparison of two equal-length suffixes whose
    // common prefix has already been stripped. Pred is an equivalence.
    template<typename ForwardIt1,
             typename ForwardIt2,
             typename Pred>
    RE_STD_CONSTEXPR_CPP14 bool
    is_permutation_tail(
        ForwardIt1 _first1,
        ForwardIt1 _last1,
        ForwardIt2 _first2,
        ForwardIt2 _last2,
        Pred       _pred
    )
    {
        for (ForwardIt1 _i = _first1; _i != _last1; ++_i)
        {
            // skip a value already accounted for by an earlier pass
            bool _seen = false;
            for (ForwardIt1 _j = _first1; _j != _i; ++_j)
            {
                if (_pred(*_j, *_i))
                {
                    _seen = true;
                    break;
                }
            }
            if (_seen)
            {
                continue;
            }

            std::size_t _count2 = 0;
            for (ForwardIt2 _k = _first2; _k != _last2; ++_k)
            {
                if (_pred(*_k, *_i))
                {
                    ++_count2;
                }
            }
            if (_count2 == 0)
            {
                return false;
            }

            std::size_t _count1 = 0;
            for (ForwardIt1 _j = _i; _j != _last1; ++_j)
            {
                if (_pred(*_j, *_i))
                {
                    ++_count1;
                }
            }
            if (_count1 != _count2)
            {
                return false;
            }
        }
        return true;
    }

    // is_permutation_eq
    //   trait: the default equivalence, operator==. A named functor
    // rather than a lambda so the C++98 tier can use it too.
    struct is_permutation_eq
    {
        template<typename A,
                 typename B>
        RE_STD_CONSTEXPR bool
        operator()(
            const A& _a,
            const B& _b
        ) const
        {
            return _a == _b;
        }
    };

}  // internal


// ===========================================================================
// I.   IS_PERMUTATION (3-ARG, PREDICATE)
// ===========================================================================

// is_permutation (3-arg, predicate)
//   function: true iff the range beginning at _first2 -- taken to be as
// long as [_first1, _last1) -- is a rearrangement of it.
template<typename ForwardIt1,
         typename ForwardIt2,
         typename Pred>
RE_STD_CONSTEXPR_CPP14 bool
is_permutation(
    ForwardIt1 _first1,
    ForwardIt1 _last1,
    ForwardIt2 _first2,
    Pred       _pred
)
{
    // strip the common prefix
    for (; _first1 != _last1; ++_first1, (void)++_first2)
    {
        if (!_pred(*_first1, *_first2))
        {
            break;
        }
    }
    if (_first1 == _last1)
    {
        return true;
    }

    // walk out the second range's end rather than calling distance /
    // advance: keeps this header free of iterator_traits, so it holds at
    // the C++98 floor and needs only forward iterators.
    ForwardIt2 _last2 = _first2;
    for (ForwardIt1 _walk = _first1; _walk != _last1; ++_walk)
    {
        ++_last2;
    }

    return internal::is_permutation_tail(_first1, _last1,
                                         _first2, _last2, _pred);
}


// ===========================================================================
// II.  IS_PERMUTATION (3-ARG, operator==)
// ===========================================================================

// is_permutation (3-arg)
//   function: as above, comparing with operator==.
template<typename ForwardIt1,
         typename ForwardIt2>
RE_STD_CONSTEXPR_CPP14 bool
is_permutation(
    ForwardIt1 _first1,
    ForwardIt1 _last1,
    ForwardIt2 _first2
)
{
    return re_std::is_permutation(_first1, _last1, _first2,
                                 internal::is_permutation_eq());
}


// ===========================================================================
// III. IS_PERMUTATION (4-ARG, PREDICATE)
// ===========================================================================

// is_permutation (4-arg, predicate)
//   function: true iff the two explicitly bounded ranges are
// rearrangements of one another. Unequal lengths are false.
template<typename ForwardIt1,
         typename ForwardIt2,
         typename Pred>
RE_STD_CONSTEXPR_CPP14 bool
is_permutation(
    ForwardIt1 _first1,
    ForwardIt1 _last1,
    ForwardIt2 _first2,
    ForwardIt2 _last2,
    Pred       _pred
)
{
    {
        // lengths must match; walk in lockstep rather than calling
        // distance twice.
        ForwardIt1 _w1 = _first1;
        ForwardIt2 _w2 = _first2;
        for (; (_w1 != _last1) && (_w2 != _last2); ++_w1, (void)++_w2)
        {
            // empty
        }
        if ((_w1 != _last1) || (_w2 != _last2))
        {
            return false;
        }
    }

    for (; _first1 != _last1; ++_first1, (void)++_first2)
    {
        if (!_pred(*_first1, *_first2))
        {
            break;
        }
    }
    if (_first1 == _last1)
    {
        return true;
    }

    return internal::is_permutation_tail(_first1, _last1,
                                         _first2, _last2, _pred);
}


// ===========================================================================
// IV.  IS_PERMUTATION (4-ARG, operator==)
// ===========================================================================

// is_permutation (4-arg)
//   function: as above, comparing with operator==.
template<typename ForwardIt1,
         typename ForwardIt2>
RE_STD_CONSTEXPR_CPP14 bool
is_permutation(
    ForwardIt1 _first1,
    ForwardIt1 _last1,
    ForwardIt2 _first2,
    ForwardIt2 _last2
)
{
    return re_std::is_permutation(_first1, _last1, _first2, _last2,
                                 internal::is_permutation_eq());
}


}  // re_std


#endif  // RE_STD_ALGORITHM_IS_PERMUTATION_HPP
