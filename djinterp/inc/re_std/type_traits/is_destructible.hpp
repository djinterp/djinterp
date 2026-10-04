/******************************************************************************
* djinterp [re_std]                                         is_destructible.hpp
*
* is_destructible trait header:
*   Yields true_type if `_Type` can be destroyed (the expression
* `t.~_U()` is well-formed for an lvalue `t` of type `_U`, where `_U`
* is `_Type` with all array dimensions removed), false_type otherwise.
* Per [meta.unary.prop]:
*   - void / function / unbounded-array          -> false
*   - reference type                              -> true (vacuously)
*   - other object types                          -> probe destructor
*
*     is_destructible<int>::value             -> true
*     is_destructible<int&>::value            -> true
*     is_destructible<int[5]>::value          -> true
*     is_destructible<int[]>::value           -> false  (unbounded)
*     is_destructible<void>::value            -> false
*     is_destructible<int()>::value           -> false  (function)
*
*     struct A { ~A() = delete; };
*     is_destructible<A>::value               -> false
*
*   PORTABILITY:
*   The portable C++11+ fallback uses a declval-style helper and the
* pseudo-destructor expression. Intrinsic-backed where
* `__is_destructible` is available.
*
*   DETECTION MACRO:
*   D_RE_STD_HAS_IS_DESTRUCTIBLE.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_destructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_DESTRUCTIBLE_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_DESTRUCTIBLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// djinterp
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./is_void.hpp"
#include "./is_function.hpp"
#include "./is_reference.hpp"
#include "./is_unbounded_array.hpp"
#include "./remove_all_extents.hpp"


#ifndef D_RE_STD_HAS_IS_DESTRUCTIBLE
    #if defined(__has_builtin)
        #if __has_builtin(__is_destructible)
            #define D_RE_STD_HAS_IS_DESTRUCTIBLE     1
        #else
            #define D_RE_STD_HAS_IS_DESTRUCTIBLE     0
        #endif
    #elif defined(D_ENV_COMPILER_MSVC)
        #define D_RE_STD_HAS_IS_DESTRUCTIBLE         1
    #else
        #define D_RE_STD_HAS_IS_DESTRUCTIBLE         0
    #endif
#endif


NS_RESTD


// =============================================================================
// I.   IS_DESTRUCTIBLE
// =============================================================================

#if D_RE_STD_HAS_IS_DESTRUCTIBLE

    template<typename _Type>
    struct is_destructible
        : integral_constant<bool, __is_destructible(_Type)>
    {};

#else


    NS_INTERNAL

        // declval-style lvalue maker (private to this header).
        template<typename _T>
        _T& is_destruct_lref() D_NOEXCEPT;

        // is_destruct_probe
        //   helper: SFINAE on `lref().~_U()`.
        template<typename _U>
        struct is_destruct_probe
        {
        private:
            template<typename _T>
            static auto test(int) ->
                decltype(is_destruct_lref<_T>().~_T(), true_type{});

            template<typename>
            static false_type test(...);

        public:
            typedef decltype(test<_U>(0)) type;
            D_STATIC_CONSTEXPR bool value = type::value;
        };

        // is_destructible_dispatch
        //   helper: routes to the four cases per [meta.unary.prop].
        template<typename _Type,
                 bool     _IsExcluded =
                     ( is_void<_Type>::value           ||
                       is_function<_Type>::value       ||
                       is_unbounded_array<_Type>::value ),
                 bool     _IsRef = is_reference<_Type>::value>
        struct is_destructible_dispatch;

        // void / function / unbounded array -> false
        template<typename _Type,
                 bool     _IsRef>
        struct is_destructible_dispatch<_Type, true, _IsRef>
            : false_type
        {};

        // reference -> true
        template<typename _Type>
        struct is_destructible_dispatch<_Type, false, true>
            : true_type
        {};

        // ordinary object -> probe destructor on innermost element type
        template<typename _Type>
        struct is_destructible_dispatch<_Type, false, false>
            : integral_constant<bool,
                  is_destruct_probe<
                      typename remove_all_extents<_Type>::type
                  >::value>
        {};

    NS_END  // internal


    template<typename _Type>
    struct is_destructible
        : integral_constant<bool,
              internal::is_destructible_dispatch<_Type>::value>
    {};


#endif  // D_RE_STD_HAS_IS_DESTRUCTIBLE


// =============================================================================
// II.  IS_DESTRUCTIBLE_V
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Type>
    D_CONSTEXPR bool is_destructible_v = is_destructible<_Type>::value;

#endif


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_DESTRUCTIBLE_
