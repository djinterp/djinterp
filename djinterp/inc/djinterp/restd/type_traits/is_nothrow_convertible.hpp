/******************************************************************************
* djinterp [restd]                                    is_nothrow_convertible.hpp
*
* is_nothrow_convertible trait:
*   true_type if _From is implicitly convertible to _To AND the conversion
* does not throw; false_type otherwise. Standardized in C++20, but provided
* here on C++11 and later (the implementation does not require any feature
* introduced after C++11).
*
*   IMPLEMENTATION TECHNIQUE:
*   The classic noexcept-of-conversion probe. We declare an internal helper
* function `void implicit_takes(_To) noexcept;` and check
* `noexcept(implicit_takes(declval<_From>()))`. Because the function itself
* is marked noexcept, the only operation in the call expression that could
* throw is the implicit conversion of the argument from _From to _To. Hence
* the noexcept-operator's result is the noexceptness of the conversion.
*
*   The trait short-circuits via the same two-step pattern used by
* is_nothrow_swappable_with: first check is_convertible<_From, _To>, and
* only when convertibility holds does it instantiate the noexcept probe.
* This avoids spurious hard errors when _From cannot be converted to _To
* at all (e.g. unrelated class types).
*
*   PORTABILITY:
*   Available on C++11 and later. C++98/03 omits the trait (no noexcept
* operator, no decltype, no rvalue-reference-based declval).
*
*   DEPENDENCIES:
*   is_convertible, integral_constant, false_type, restd::declval.
*
*
* path:      /inc/djinterp/restd/type_traits/is_nothrow_convertible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                     created: 2026.04.29
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_NOTHROW_CONVERTIBLE_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_NOTHROW_CONVERTIBLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// restd
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./integral_constant.hpp"
#include "./is_convertible.hpp"
#include "../utility/declval.hpp"


NS_RESTD


    NS_INTERNAL

        // implicit_takes
        //   function: declaration-only noexcept function used as a probe.
        //             The body is never defined and the function is never
        //             called; it appears only inside an unevaluated noexcept
        //             operand. Because the function itself is noexcept, the
        //             call expression's noexceptness equals the noexceptness
        //             of the implicit conversion of the argument to _To.
        template<typename _To>
        void implicit_takes(_To) D_NOEXCEPT;

        // is_nothrow_convertible_helper
        //   trait: primary; gated by the boolean parameter _Convertible.
        //          When false, short-circuits to false_type without
        //          instantiating the noexcept probe.
        template<typename _From,
                 typename _To,
                 bool     _Convertible>
        struct is_nothrow_convertible_helper
            : false_type
        {};

        // is_nothrow_convertible_helper<_From, _To, true>
        //   trait: specialization; selected when the conversion is known
        //          well-formed. Wraps the noexcept probe in an
        //          integral_constant<bool, ...>.
        template<typename _From,
                 typename _To>
        struct is_nothrow_convertible_helper<_From, _To, true>
            : integral_constant<
                  bool,
                  noexcept( implicit_takes<_To>(
                                restd::declval<_From>() ) ) >
        {};

    NS_END  // internal


    // is_nothrow_convertible
    //   trait: true_type if _From is implicitly convertible to _To and the
    //          conversion is noexcept, false_type otherwise.
    template<typename _From,
             typename _To>
    struct is_nothrow_convertible
        : internal::is_nothrow_convertible_helper<
              _From,
              _To,
              is_convertible<_From, _To>::value >
    {};


    // is_nothrow_convertible_v (C++14+)
    #if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
        template<typename _From,
                 typename _To>
        D_CONSTEXPR bool is_nothrow_convertible_v
            = is_nothrow_convertible<_From, _To>::value;
    #endif


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_NOTHROW_CONVERTIBLE_
