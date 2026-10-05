/*******************************************************************************
* djinterp [re_std]                                          is_destructible.hpp
*
* is_destructible trait header:
*   Yields true_type if `Type` can be destroyed (the expression
* `t.~U()` is well-formed for an lvalue `t` of type `U`, where `U`
* is `Type` with all array dimensions removed), false_type otherwise.
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
*   RE_STD_HAS_IS_DESTRUCTIBLE.
*
*
* path:      /inc/re_std/type_traits/is_destructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_DESTRUCTIBLE_HPP
#define RE_STD_TYPE_TRAITS_IS_DESTRUCTIBLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER


// re_std
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./is_void.hpp"
#include "./is_function.hpp"
#include "./is_reference.hpp"
#include "./is_unbounded_array.hpp"
#include "./remove_all_extents.hpp"


#ifndef RE_STD_HAS_IS_DESTRUCTIBLE
    #if defined(__has_builtin)
        #if __has_builtin(__is_destructible)
            #define RE_STD_HAS_IS_DESTRUCTIBLE     1
        #else
            #define RE_STD_HAS_IS_DESTRUCTIBLE     0
        #endif
    #elif defined(RE_STD_COMPILER_MSVC)
        #define RE_STD_HAS_IS_DESTRUCTIBLE         1
    #else
        #define RE_STD_HAS_IS_DESTRUCTIBLE         0
    #endif
#endif


namespace re_std
{


// =============================================================================
// I.   IS_DESTRUCTIBLE
// =============================================================================

#if RE_STD_HAS_IS_DESTRUCTIBLE

    template<typename Type>
    struct is_destructible
        : integral_constant<bool, __is_destructible(Type)>
    {};

#else


    namespace internal
    {

        // declval-style lvalue maker (private to this header).
        template<typename T>
        T& is_destruct_lref() RE_STD_NOEXCEPT;

        // is_destruct_probe
        //   trait: SFINAE on `lref().~U()`.
        template<typename U>
        struct is_destruct_probe
        {
        private:
            template<typename T>
            static auto test(int) ->
                decltype(is_destruct_lref<T>().~T(), true_type{});

            template<typename>
            static false_type test(...);

        public:
            typedef decltype(test<U>(0)) type;
            RE_STD_STATIC_CONSTEXPR bool value = type::value;
        };

        // is_destructible_dispatch
        //   function: routes to the four cases per [meta.unary.prop].
        template<typename Type,
                 bool     IsExcluded =
                     ( is_void<Type>::value           ||
                       is_function<Type>::value       ||
                       is_unbounded_array<Type>::value ),
                 bool     IsRef = is_reference<Type>::value>
        struct is_destructible_dispatch;

        // void / function / unbounded array -> false
        template<typename Type,
                 bool     IsRef>
        struct is_destructible_dispatch<Type, true, IsRef>
            : false_type
        {};

        // reference -> true
        template<typename Type>
        struct is_destructible_dispatch<Type, false, true>
            : true_type
        {};

        // ordinary object -> probe destructor on innermost element type
        template<typename Type>
        struct is_destructible_dispatch<Type, false, false>
            : integral_constant<bool,
                  is_destruct_probe<
                      typename remove_all_extents<Type>::type
                  >::value>
        {};

    }  // internal


    template<typename Type>
    struct is_destructible
        : integral_constant<bool,
              internal::is_destructible_dispatch<Type>::value>
    {};


#endif  // RE_STD_HAS_IS_DESTRUCTIBLE


// =============================================================================
// II.  IS_DESTRUCTIBLE_V
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    template<typename Type>
    RE_STD_CONSTEXPR bool is_destructible_v = is_destructible<Type>::value;

#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_TYPE_TRAITS_IS_DESTRUCTIBLE_HPP
