/******************************************************************************
* djinterp [restd]                                  is_nothrow_destructible.hpp
*
* is_nothrow_destructible trait header:
*   Yields true_type if _Type is destructible AND the destructor is
* `noexcept`, false_type otherwise. Intrinsic-backed via
* `__is_nothrow_destructible`; falls back to `is_destructible` plus a
* `noexcept` probe on the destructor expression.
*
*   DETECTION MACRO:
*   D_RESTD_HAS_IS_NOTHROW_DESTRUCTIBLE.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_nothrow_destructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RESTD_TYPE_TRAITS_IS_NOTHROW_DESTRUCTIBLE_
#define DJINTERP_RESTD_TYPE_TRAITS_IS_NOTHROW_DESTRUCTIBLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// djinterp
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./is_destructible.hpp"
#include "./is_reference.hpp"
#include "./remove_all_extents.hpp"


#ifndef D_RESTD_HAS_IS_NOTHROW_DESTRUCTIBLE
    #if defined(__has_builtin)
        #if __has_builtin(__is_nothrow_destructible)
            #define D_RESTD_HAS_IS_NOTHROW_DESTRUCTIBLE     1
        #else
            #define D_RESTD_HAS_IS_NOTHROW_DESTRUCTIBLE     0
        #endif
    #elif defined(D_ENV_COMPILER_MSVC)
        #define D_RESTD_HAS_IS_NOTHROW_DESTRUCTIBLE         1
    #else
        #define D_RESTD_HAS_IS_NOTHROW_DESTRUCTIBLE         0
    #endif
#endif


NS_RESTD


// =============================================================================
// I.   IS_NOTHROW_DESTRUCTIBLE
// =============================================================================

#if D_RESTD_HAS_IS_NOTHROW_DESTRUCTIBLE

    template<typename _Type>
    struct is_nothrow_destructible
        : integral_constant<bool, __is_nothrow_destructible(_Type)>
    {};

#else


    NS_INTERNAL

        // declval-style lvalue maker (private to this header).
        template<typename _T>
        _T& is_nothrow_destruct_lref() D_NOEXCEPT;

        // is_nothrow_destruct_probe
        //   helper: gated on is_destructible. Then probes whether the
        // destructor expression itself is noexcept.
        template<typename _Type,
                 bool     _IsDestructible>
        struct is_nothrow_destruct_probe
        {
            D_STATIC_CONSTEXPR bool value = false;
        };

        template<typename _Type>
        struct is_nothrow_destruct_probe<_Type, true>
        {
        private:
            // Reference types are vacuously nothrow destructible.
            // Otherwise, peel arrays and probe the element destructor.
            typedef typename remove_all_extents<_Type>::type _U;

        public:
            D_STATIC_CONSTEXPR bool value =
                is_reference<_Type>::value
                ? true
                : noexcept(is_nothrow_destruct_lref<_U>().~_U());
        };

    NS_END  // internal


    template<typename _Type>
    struct is_nothrow_destructible
        : integral_constant<bool,
              internal::is_nothrow_destruct_probe<
                  _Type,
                  is_destructible<_Type>::value
              >::value>
    {};


#endif  // D_RESTD_HAS_IS_NOTHROW_DESTRUCTIBLE


// =============================================================================
// II.  IS_NOTHROW_DESTRUCTIBLE_V
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    D_CONSTEXPR bool is_nothrow_destructible_v =
        is_nothrow_destructible<_Type>::value;

#endif


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_TYPE_TRAITS_IS_NOTHROW_DESTRUCTIBLE_
