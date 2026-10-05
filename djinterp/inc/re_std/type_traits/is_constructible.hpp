/*******************************************************************************
* djinterp [re_std]                                         is_constructible.hpp
*
* is_constructible trait header:
*   Yields true_type if a hypothetical variable definition
* `Type t(declval<Args>()...);` is well-formed, false_type otherwise.
* Implemented via the `__is_constructible` builtin where available;
* otherwise via a SFINAE probe on direct-initialization syntax.
*
*     is_constructible<int>::value                 -> true   (default)
*     is_constructible<int, int>::value            -> true   (from int)
*     is_constructible<int, void*>::value          -> false
*
*     struct A { A(int, char); };
*     is_constructible<A, int, char>::value        -> true
*     is_constructible<A>::value                   -> false (no default ctor)
*
*   PORTABILITY:
*   The C++11+ portable fallback uses variadic templates and decltype.
* The trait is omitted entirely on C++98/03; consumer code must gate
* on RE_STD_LANG_HAS_VARIADIC_TEMPLATES.
*
*   DETECTION MACRO:
*   RE_STD_HAS_IS_CONSTRUCTIBLE.
*
*
* path:      /inc/re_std/type_traits/is_constructible.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_CONSTRUCTIBLE_HPP
#define RE_STD_TYPE_TRAITS_IS_CONSTRUCTIBLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// gate: variadic templates required
#if RE_STD_LANG_HAS_VARIADIC_TEMPLATES


// re_std
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./add_rvalue_reference.hpp"


// =============================================================================
// 0.   RE_STD_HAS_IS_CONSTRUCTIBLE  (intrinsic detection)
// =============================================================================

#ifndef RE_STD_HAS_IS_CONSTRUCTIBLE
    #if defined(__has_builtin)
        #if __has_builtin(__is_constructible)
            #define RE_STD_HAS_IS_CONSTRUCTIBLE    1
        #else
            #define RE_STD_HAS_IS_CONSTRUCTIBLE    0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC)   ||                                  \
            defined(RE_STD_COMPILER_CLANG) ||                                  \
            defined(RE_STD_COMPILER_MSVC)  ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_IS_CONSTRUCTIBLE        1
    #else
        #define RE_STD_HAS_IS_CONSTRUCTIBLE        0
    #endif
#endif  // RE_STD_HAS_IS_CONSTRUCTIBLE


namespace re_std
{


// =============================================================================
// I.   IS_CONSTRUCTIBLE
// =============================================================================

#if RE_STD_HAS_IS_CONSTRUCTIBLE

    // is_constructible (intrinsic)
    template<typename    Type,
             typename... Args>
    struct is_constructible
        : integral_constant<bool, __is_constructible(Type, Args...)>
    {};

#else


    namespace internal
    {

        // declval shim for is_constructible (private to this header).
        template<typename T>
        typename add_rvalue_reference<T>::type
            is_ctor_declval() RE_STD_NOEXCEPT;

        // is_ctor_probe
        //   trait: SFINAE probe on `Type(declval<Args>()...)`.
        template<typename    Type,
                 typename... Args>
        struct is_ctor_probe
        {
        private:
            template<typename    T,
                     typename... A,
                     typename = decltype(T(is_ctor_declval<A>()...))>
            static true_type test(int);

            template<typename, typename...>
            static false_type test(...);

        public:
            typedef decltype(test<Type, Args...>(0)) type;
            RE_STD_STATIC_CONSTEXPR bool value = type::value;
        };

    }  // internal


    // is_constructible (portable C++11+)
    template<typename    Type,
             typename... Args>
    struct is_constructible
        : integral_constant<bool,
              internal::is_ctor_probe<Type, Args...>::value>
    {};


#endif  // RE_STD_HAS_IS_CONSTRUCTIBLE


// =============================================================================
// II.  IS_CONSTRUCTIBLE_V (C++14+ variable template)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    template<typename    Type,
             typename... Args>
    RE_STD_CONSTEXPR bool is_constructible_v =
        is_constructible<Type, Args...>::value;

#endif


}  // re_std


#endif  // RE_STD_LANG_HAS_VARIADIC_TEMPLATES


#endif  // RE_STD_TYPE_TRAITS_IS_CONSTRUCTIBLE_HPP
