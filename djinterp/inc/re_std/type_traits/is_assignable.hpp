/*******************************************************************************
* djinterp [re_std]                                            is_assignable.hpp
*
* is_assignable trait header:
*   Yields true_type if the expression `declval<To>() = declval<From>()`
* is well-formed when treated as an unevaluated operand, false_type
* otherwise. Implemented via the `__is_assignable` builtin where
* available; otherwise via a SFINAE probe.
*
*     is_assignable<int&, int>::value             -> true
*     is_assignable<int, int>::value              -> false (int rvalue)
*     is_assignable<const int&, int>::value       -> false
*
*     struct A { A& operator=(int); };
*     is_assignable<A&, int>::value               -> true
*
*   PORTABILITY:
*   Requires C++11+ (decltype, declval). Trait is omitted on C++98/03.
*
*   DETECTION MACRO:
*   RE_STD_HAS_IS_ASSIGNABLE.
*
*
* path:      /inc/re_std/type_traits/is_assignable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_ASSIGNABLE_HPP
#define RE_STD_TYPE_TRAITS_IS_ASSIGNABLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER


// re_std
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./add_rvalue_reference.hpp"


#ifndef RE_STD_HAS_IS_ASSIGNABLE
    #if defined(__has_builtin)
        #if __has_builtin(__is_assignable)
            #define RE_STD_HAS_IS_ASSIGNABLE   1
        #else
            #define RE_STD_HAS_IS_ASSIGNABLE   0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC)   ||                                  \
            defined(RE_STD_COMPILER_CLANG) ||                                  \
            defined(RE_STD_COMPILER_MSVC)  ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_IS_ASSIGNABLE       1
    #else
        #define RE_STD_HAS_IS_ASSIGNABLE       0
    #endif
#endif


namespace re_std
{


// =============================================================================
// I.   IS_ASSIGNABLE
// =============================================================================

#if RE_STD_HAS_IS_ASSIGNABLE

    template<typename To,
             typename From>
    struct is_assignable
        : integral_constant<bool, __is_assignable(To, From)>
    {};

#else


    namespace internal
    {

        // declval shim
        template<typename T>
        typename add_rvalue_reference<T>::type
            is_assign_declval() RE_STD_NOEXCEPT;

        // is_assign_probe
        //   trait: SFINAE on `declval<To>() = declval<From>()`.
        template<typename To,
                 typename From>
        struct is_assign_probe
        {
        private:
            template<typename T,
                     typename F,
                     typename = decltype(
                         is_assign_declval<T>() = is_assign_declval<F>())>
            static true_type test(int);

            template<typename, typename>
            static false_type test(...);

        public:
            typedef decltype(test<To, From>(0)) type;
            RE_STD_STATIC_CONSTEXPR bool value = type::value;
        };

    }  // internal


    template<typename To,
             typename From>
    struct is_assignable
        : integral_constant<bool,
              internal::is_assign_probe<To, From>::value>
    {};


#endif  // RE_STD_HAS_IS_ASSIGNABLE


// =============================================================================
// II.  IS_ASSIGNABLE_V
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

    template<typename To,
             typename From>
    RE_STD_CONSTEXPR bool is_assignable_v = is_assignable<To, From>::value;

#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_TYPE_TRAITS_IS_ASSIGNABLE_HPP
