/*******************************************************************************
* djinterp [re_std]                                  is_nothrow_destructible.hpp
*
* is_nothrow_destructible trait header:
*   Yields true_type if Type is destructible AND the destructor is
* `noexcept`, false_type otherwise. Intrinsic-backed via
* `__is_nothrow_destructible`; falls back to `is_destructible` plus a
* `noexcept` probe on the destructor expression.
*
*   DETECTION MACRO:
*   RE_STD_HAS_IS_NOTHROW_DESTRUCTIBLE.
*
*
* path:      /inc/re_std/type_traits/is_nothrow_destructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_NOTHROW_DESTRUCTIBLE_HPP
#define RE_STD_TYPE_TRAITS_IS_NOTHROW_DESTRUCTIBLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER


// re_std
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./is_destructible.hpp"
#include "./is_reference.hpp"
#include "./remove_all_extents.hpp"


#ifndef RE_STD_HAS_IS_NOTHROW_DESTRUCTIBLE
    #if defined(__has_builtin)
        #if __has_builtin(__is_nothrow_destructible)
            #define RE_STD_HAS_IS_NOTHROW_DESTRUCTIBLE     1
        #else
            #define RE_STD_HAS_IS_NOTHROW_DESTRUCTIBLE     0
        #endif
    #elif defined(RE_STD_COMPILER_MSVC)
        #define RE_STD_HAS_IS_NOTHROW_DESTRUCTIBLE         1
    #else
        #define RE_STD_HAS_IS_NOTHROW_DESTRUCTIBLE         0
    #endif
#endif


namespace re_std
{


// =============================================================================
// I.   IS_NOTHROW_DESTRUCTIBLE
// =============================================================================

#if RE_STD_HAS_IS_NOTHROW_DESTRUCTIBLE

    template<typename Type>
    struct is_nothrow_destructible
        : integral_constant<bool, __is_nothrow_destructible(Type)>
    {};

#else


    namespace internal
    {

        // declval-style lvalue maker (private to this header).
        template<typename T>
        T& is_nothrow_destruct_lref() RE_STD_NOEXCEPT;

        // is_nothrow_destruct_probe
        //   trait: gated on is_destructible. Then probes whether the
        // destructor expression itself is noexcept.
        template<typename Type,
                 bool     IsDestructible>
        struct is_nothrow_destruct_probe
        {
            RE_STD_STATIC_CONSTEXPR bool value = false;
        };

        template<typename Type>
        struct is_nothrow_destruct_probe<Type, true>
        {
        private:
            // Reference types are vacuously nothrow destructible.
            // Otherwise, peel arrays and probe the element destructor.
            typedef typename remove_all_extents<Type>::type _U;

        public:
            RE_STD_STATIC_CONSTEXPR bool value =
                is_reference<Type>::value
                ? true
                : noexcept(is_nothrow_destruct_lref<_U>().~_U());
        };

    }  // internal


    template<typename Type>
    struct is_nothrow_destructible
        : integral_constant<bool,
              internal::is_nothrow_destruct_probe<
                  Type,
                  is_destructible<Type>::value
              >::value>
    {};


#endif  // RE_STD_HAS_IS_NOTHROW_DESTRUCTIBLE


// =============================================================================
// II.  IS_NOTHROW_DESTRUCTIBLE_V
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    template<typename Type>
    RE_STD_CONSTEXPR bool is_nothrow_destructible_v =
        is_nothrow_destructible<Type>::value;

#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_TYPE_TRAITS_IS_NOTHROW_DESTRUCTIBLE_HPP
