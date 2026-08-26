/******************************************************************************
* djinterp [re_std]                               is_nothrow_swappable_with.hpp
*
* is_nothrow_swappable_with trait:
*   true_type if is_swappable_with<_T, _U> is true_type AND both directional
* swap calls are noexcept; false_type otherwise.
*
*   TWO-STEP DESIGN:
*   The trait is implemented in two stages because noexcept(expr) is only
* well-formed when expr itself is well-formed. The outer template first
* checks is_swappable_with (does the swap call exist?), and only when the
* answer is true does it dispatch to a helper specialization that probes
* noexcept(swap(...)) in both directions. If swappability fails, the helper
* short-circuits to false_type without ever instantiating the noexcept probe.
*
*   LOOKUP:
*   Same lookup rules as is_swappable_with: a using-declaration brings
* re_std::swap into a dedicated detection namespace, and the unqualified call
* picks up ADL-found overloads.
*
*   PORTABILITY:
*   Available on C++11 and later (requires noexcept operator). C++98/03 omits
* the trait.
*
*   DEPENDENCIES:
*   is_swappable_with, re_std::swap, re_std::declval, integral_constant.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_nothrow_swappable_with.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                     created: 2026.04.29
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_NOTHROW_SWAPPABLE_WITH_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_NOTHROW_SWAPPABLE_WITH_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./integral_constant.hpp"
#include "./is_swappable_with.hpp"
#include "../utility/declval.hpp"
#include "../utility/swap.hpp"


NS_RESTD


    NS_INTERNAL

        // nothrow_swappable_lookup
        //   namespace: dedicated lookup context, mirroring swappable_lookup
        //              from is_swappable_with.hpp. The using-declaration on
        //              re_std::swap ensures the noexcept(swap(...)) probe
        //              evaluates noexceptness against the same swap that
        //              is_swappable_with would have selected.
        namespace nothrow_swappable_lookup
        {

            using re_std::swap;

            // is_nothrow_swappable_with_helper
            //   trait: primary; gated by the boolean value parameter
            //          _Swappable. When false, short-circuits to false_type
            //          without instantiating the noexcept probe.
            template<typename _T,
                     typename _U,
                     bool     _Swappable>
            struct is_nothrow_swappable_with_helper
                : false_type
            {};

            // is_nothrow_swappable_with_helper<_T, _U, true>
            //   trait: specialization; selected only when the swap calls
            //          are known to be well-formed. Probes noexcept on both
            //          directional swap calls and combines the results.
            template<typename _T,
                     typename _U>
            struct is_nothrow_swappable_with_helper<_T, _U, true>
                : integral_constant<
                      bool,
                      (    noexcept( swap( re_std::declval<_T>(),
                                           re_std::declval<_U>() ) )
                        && noexcept( swap( re_std::declval<_U>(),
                                           re_std::declval<_T>() ) ) ) >
            {};

        }  // namespace nothrow_swappable_lookup

    NS_END  // internal


    // is_nothrow_swappable_with
    //   trait: true_type if swap(declval<_T>(), declval<_U>()) and the
    //          reverse call are both well-formed AND both noexcept;
    //          false_type otherwise.
    template<typename _T,
             typename _U>
    struct is_nothrow_swappable_with
        : internal::nothrow_swappable_lookup::is_nothrow_swappable_with_helper<
              _T,
              _U,
              is_swappable_with<_T, _U>::value >
    {};


    // is_nothrow_swappable_with_v (C++14+)
    #if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
        template<typename _T,
                 typename _U>
        D_CONSTEXPR bool is_nothrow_swappable_with_v
            = is_nothrow_swappable_with<_T, _U>::value;
    #endif


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_NOTHROW_SWAPPABLE_WITH_
