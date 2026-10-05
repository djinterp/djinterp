/*******************************************************************************
* djinterp [re_std]                                   is_nothrow_convertible.hpp
*
* is_nothrow_convertible trait:
*   true_type if From is implicitly convertible to To AND the conversion
* does not throw; false_type otherwise. Standardized in C++20, but provided
* here on C++11 and later (the implementation does not require any feature
* introduced after C++11).
*
*   IMPLEMENTATION TECHNIQUE:
*   The classic noexcept-of-conversion probe. We declare an internal helper
* function `void implicit_takes(To) noexcept;` and check
* `noexcept(implicit_takes(declval<From>()))`. Because the function itself
* is marked noexcept, the only operation in the call expression that could
* throw is the implicit conversion of the argument from From to To. Hence
* the noexcept-operator's result is the noexceptness of the conversion.
*
*   The trait short-circuits via the same two-step pattern used by
* is_nothrow_swappable_with: first check is_convertible<From, To>, and
* only when convertibility holds does it instantiate the noexcept probe.
* This avoids spurious hard errors when From cannot be converted to To
* at all (e.g. unrelated class types).
*
*   PORTABILITY:
*   Available on C++11 and later. C++98/03 omits the trait (no noexcept
* operator, no decltype, no rvalue-reference-based declval).
*
*   DEPENDENCIES:
*   is_convertible, integral_constant, false_type, re_std::declval.
*
*
* path:      /inc/re_std/type_traits/is_nothrow_convertible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.29
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_NOTHROW_CONVERTIBLE_HPP
#define RE_STD_TYPE_TRAITS_IS_NOTHROW_CONVERTIBLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./integral_constant.hpp"
#include "./is_convertible.hpp"
#include "../utility/declval.hpp"


namespace re_std
{


    namespace internal
    {

        // implicit_takes
        //   function: declaration-only noexcept function used as a probe.
        //             The body is never defined and the function is never
        //             called; it appears only inside an unevaluated noexcept
        //             operand. Because the function itself is noexcept, the
        //             call expression's noexceptness equals the noexceptness
        //             of the implicit conversion of the argument to To.
        template<typename To>
        void implicit_takes(To) RE_STD_NOEXCEPT;

        // is_nothrow_convertible_helper
        //   trait: primary; gated by the boolean parameter Convertible.
        //          When false, short-circuits to false_type without
        //          instantiating the noexcept probe.
        template<typename From,
                 typename To,
                 bool     Convertible>
        struct is_nothrow_convertible_helper
            : false_type
        {};

        // is_nothrow_convertible_helper<From, To, true>
        //   trait: specialization; selected when the conversion is known
        //          well-formed. Wraps the noexcept probe in an
        //          integral_constant<bool, ...>.
        template<typename From,
                 typename To>
        struct is_nothrow_convertible_helper<From, To, true>
            : integral_constant<
                  bool,
                  noexcept( implicit_takes<To>(
                                re_std::declval<From>() ) ) >
        {};

    }  // internal


    // is_nothrow_convertible
    //   trait: true_type if From is implicitly convertible to To and the
    //          conversion is noexcept, false_type otherwise.
    template<typename From,
             typename To>
    struct is_nothrow_convertible
        : internal::is_nothrow_convertible_helper<
              From,
              To,
              is_convertible<From, To>::value >
    {};


    // is_nothrow_convertible_v (C++14+)
    #if RE_STD_LANG_HAS_VARIABLE_TEMPLATES
        template<typename From,
                 typename To>
        RE_STD_CONSTEXPR bool is_nothrow_convertible_v
            = is_nothrow_convertible<From, To>::value;
    #endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER

#endif  // RE_STD_TYPE_TRAITS_IS_NOTHROW_CONVERTIBLE_HPP
