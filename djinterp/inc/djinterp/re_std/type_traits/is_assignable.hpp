/******************************************************************************
* djinterp [re_std]                                           is_assignable.hpp
*
* is_assignable trait header:
*   Yields true_type if the expression `declval<_To>() = declval<_From>()`
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
*   D_RE_STD_HAS_IS_ASSIGNABLE.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_assignable.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_ASSIGNABLE_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_ASSIGNABLE_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER


// djinterp
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./add_rvalue_reference.hpp"


#ifndef D_RE_STD_HAS_IS_ASSIGNABLE
    #if defined(__has_builtin)
        #if __has_builtin(__is_assignable)
            #define D_RE_STD_HAS_IS_ASSIGNABLE   1
        #else
            #define D_RE_STD_HAS_IS_ASSIGNABLE   0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC)   ||                                  \
            defined(D_ENV_COMPILER_CLANG) ||                                  \
            defined(D_ENV_COMPILER_MSVC)  ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RE_STD_HAS_IS_ASSIGNABLE       1
    #else
        #define D_RE_STD_HAS_IS_ASSIGNABLE       0
    #endif
#endif


NS_RESTD


// =============================================================================
// I.   IS_ASSIGNABLE
// =============================================================================

#if D_RE_STD_HAS_IS_ASSIGNABLE

    template<typename _To,
             typename _From>
    struct is_assignable
        : integral_constant<bool, __is_assignable(_To, _From)>
    {};

#else


    NS_INTERNAL

        // declval shim
        template<typename _T>
        typename add_rvalue_reference<_T>::type
            is_assign_declval() D_NOEXCEPT;

        // is_assign_probe
        //   helper: SFINAE on `declval<_To>() = declval<_From>()`.
        template<typename _To,
                 typename _From>
        struct is_assign_probe
        {
        private:
            template<typename _T,
                     typename _F,
                     typename = decltype(
                         is_assign_declval<_T>() = is_assign_declval<_F>())>
            static true_type test(int);

            template<typename, typename>
            static false_type test(...);

        public:
            typedef decltype(test<_To, _From>(0)) type;
            D_STATIC_CONSTEXPR bool value = type::value;
        };

    NS_END  // internal


    template<typename _To,
             typename _From>
    struct is_assignable
        : integral_constant<bool,
              internal::is_assign_probe<_To, _From>::value>
    {};


#endif  // D_RE_STD_HAS_IS_ASSIGNABLE


// =============================================================================
// II.  IS_ASSIGNABLE_V
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _To,
             typename _From>
    D_CONSTEXPR bool is_assignable_v = is_assignable<_To, _From>::value;

#endif


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_ASSIGNABLE_
