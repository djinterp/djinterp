/***********************************************************************
* re_std                                                    make_pair.hpp
*
* pair factory function:
*   Constructs a `pair` deducing element types from arguments.
*
*   Tiered implementation:
*     C++11+   perfect-forwarding overload using unwrap_ref_decay<T>,
*              which decays the argument and then unwraps a
*              reference_wrapper<X> to X&, exactly as [pairs.spec]
*              specifies.
*     C++98/03 pass-by-value overload. References and cv-qualifiers
*              are stripped naturally by parameter passing. Move-only
*              types are unsupported (irrelevant pre-C++11).
*
*   REFERENCE_WRAPPER UNWRAP (completed 2026-08-25):
*   make_pair(ref(n)) yields pair<int&, ...>, not
* pair<reference_wrapper<int>, ...>. This is what makes
*
*       int n = 0;
*       auto p = make_pair(re_std::ref(n), 1);
*       p.first = 42;                       // writes through to n
*
* behave as the standard requires. The C++98 pass-by-value overload
* cannot unwrap -- reference_wrapper is C++11+ -- so the deviation
* survives only on that tier.
*
*
* path:      /inc/djinterp/re_std/utility/make_pair.hpp
* link(s):   TBA
* author(s): re_std team                                date: 2026.04.30
***********************************************************************/

#ifndef DJINTERP_RE_STD_UTILITY_MAKE_PAIR_
#define DJINTERP_RE_STD_UTILITY_MAKE_PAIR_ 1

#include "djinterp.hpp"
#include "../utility/pair.hpp"

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES
    #include "../utility/forward.hpp"
    #include "../type_traits/decay.hpp"
    // decay + reference_wrapper unwrap, per [pairs.spec]/p7
    #include "../functional/unwrap_ref_decay.hpp"
#endif

NS_RESTD

// =============================================================================
// MAKE_PAIR
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_RVALUE_REFERENCES

    // make_pair (C++11+: perfect forwarding, decay + unwrap)
    //   function: constructs a pair<V1, V2> from forwarded arguments,
    //   where Vi is unwrap_ref_decay<Ti> -- decay applied first
    //   (stripping references and cv, and applying array-to-pointer /
    //   function-to-pointer), then reference_wrapper<X> collapsed to
    //   X&. Matches std::make_pair exactly.
    template<typename _T1, typename _T2>
    D_CONSTEXPR
    pair<typename unwrap_ref_decay<_T1>::type,
         typename unwrap_ref_decay<_T2>::type>
    make_pair(_T1&& _x,
              _T2&& _y)
    {
        return pair<typename unwrap_ref_decay<_T1>::type,
                    typename unwrap_ref_decay<_T2>::type>(
            re_std::forward<_T1>(_x),
            re_std::forward<_T2>(_y));
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

NS_END  // re_std

#endif  // DJINTERP_RE_STD_UTILITY_MAKE_PAIR_
