/***********************************************************************
* restd                                                     make_pair.hpp
*
* pair factory function:
*   Constructs a `pair` deducing element types from arguments.
*
*   Tiered implementation:
*     C++11+   perfect-forwarding overload using decay<T> to strip
*              references and cv-qualifiers (matching std::make_pair's
*              type-decay behavior except for reference_wrapper, which
*              restd does not yet provide).
*     C++98/03 pass-by-value overload. References and cv-qualifiers
*              are stripped naturally by parameter passing. Move-only
*              types are unsupported (irrelevant pre-C++11).
*
*   Deviation from std::make_pair: reference_wrapper unwrapping is
* not performed (no reference_wrapper in restd yet).
*
*
* path:      /inc/djinterp/re_std/utility/make_pair.hpp
* link(s):   TBA
* author(s): restd team                                 date: 2026.04.30
***********************************************************************/

#ifndef RESTD_UTILITY_MAKE_PAIR_
#define RESTD_UTILITY_MAKE_PAIR_ 1

#include "djinterp.hpp"
#include "../utility/pair.hpp"

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
    #include "../utility/forward.hpp"
    #include "../type_traits/decay.hpp"
#endif

NS_RESTD

// =============================================================================
// MAKE_PAIR
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

    // make_pair (C++11+: perfect forwarding, decay-correct)
    //   function: constructs a pair<decay<T1>, decay<T2>> from
    //   forwarded arguments. decay strips references, cv-qualifiers,
    //   and applies array-to-pointer / function-to-pointer conversions
    //   exactly as std::make_pair does.
    template<typename _T1, typename _T2>
    D_CONSTEXPR
    pair<typename decay<_T1>::type,
         typename decay<_T2>::type>
    make_pair(_T1&& _x,
              _T2&& _y)
    {
        return pair<typename decay<_T1>::type,
                    typename decay<_T2>::type>(
            restd::forward<_T1>(_x),
            restd::forward<_T2>(_y));
    }

#else

    // make_pair (C++98/03: pass-by-value)
    //   function: constructs a pair<T1, T2> from copied arguments.
    //   Pass-by-value strips references and cv-qualifiers naturally.
    template<typename _T1, typename _T2>
    pair<_T1, _T2>
    make_pair(_T1 _x,
              _T2 _y)
    {
        return pair<_T1, _T2>(_x, _y);
    }

#endif

NS_END  // restd

#endif  // RESTD_UTILITY_MAKE_PAIR_
