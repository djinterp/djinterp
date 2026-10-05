/*******************************************************************************
* djinterp [re_std]                                                make_pair.hpp
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
* path:      /inc/re_std/utility/make_pair.hpp
* link(s):   TBA
* author(s): re_std team                                     created: 2026.04.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_UTILITY_MAKE_PAIR_HPP
#define RE_STD_UTILITY_MAKE_PAIR_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "pair.hpp"

#if RE_STD_LANG_HAS_RVALUE_REFERENCES
    #include "forward.hpp"
    #include "../type_traits/decay.hpp"
    // decay + reference_wrapper unwrap, per [pairs.spec]/p7
    #include "../functional/unwrap_ref_decay.hpp"
#endif

namespace re_std
{

// =============================================================================
// MAKE_PAIR
// =============================================================================

#if RE_STD_LANG_HAS_RVALUE_REFERENCES

    // make_pair (C++11+: perfect forwarding, decay + unwrap)
    //   function: constructs a pair<V1, V2> from forwarded arguments,
    //   where Vi is unwrap_ref_decay<Ti> -- decay applied first
    //   (stripping references and cv, and applying array-to-pointer /
    //   function-to-pointer), then reference_wrapper<X> collapsed to
    //   X&. Matches std::make_pair exactly.
    template<typename T1, typename T2>
    RE_STD_CONSTEXPR
    pair<typename unwrap_ref_decay<T1>::type,
         typename unwrap_ref_decay<T2>::type>
    make_pair(T1&& _x,
              T2&& _y)
    {
        return pair<typename unwrap_ref_decay<T1>::type,
                    typename unwrap_ref_decay<T2>::type>(
            re_std::forward<T1>(_x),
            re_std::forward<T2>(_y));
    }

#else

    // make_pair (C++98/03: pass-by-value)
    //   function: constructs a pair<T1, T2> from copied arguments.
    //   Pass-by-value strips references and cv-qualifiers naturally.
    template<typename T1, typename T2>
    pair<T1, T2>
    make_pair(T1 _x,
              T2 _y)
    {
        return pair<T1, T2>(_x, _y);
    }

#endif

}  // re_std

#endif  // RE_STD_UTILITY_MAKE_PAIR_HPP
