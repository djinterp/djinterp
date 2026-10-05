/*******************************************************************************
* djinterp [re_std]                                               is_base_of.hpp
*
* is_base_of trait header:
*   is_base_of<Base, Derived>::value is true iff Base is a base class of
* Derived (or the two are the same class type), ignoring cv-qualification.
* Both must be complete class types for a meaningful answer; a non-class
* operand yields false.
*
*   IMPLEMENTATION:
*   Compiler intrinsic (__is_base_of) where available -- it is the only way to
* see private and ambiguous bases, which the standard requires to count.  The
* portable fallback is the classic conversion probe: a host type convertible to
* both `Base*` and `Derived*` is passed to an overload pair, and only a
* derived-to-base relation makes the `Derived*` overload viable.  That
* fallback sees PUBLIC unambiguous bases only, which is the best a
* library-level implementation can do.
*
*   PORTABILITY:
*   C++11 baseline.  The _v spelling is C++14+, as elsewhere.
*
*
* path:      /inc/re_std/type_traits/is_base_of.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.27
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_BASE_OF_HPP
#define RE_STD_TYPE_TRAITS_IS_BASE_OF_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"
#include "./is_class.hpp"
#include "./is_same.hpp"
#include "./remove_cv.hpp"


// =============================================================================
// 0.   RE_STD_HAS_IS_BASE_OF  (intrinsic detection)
// =============================================================================

#ifndef RE_STD_HAS_IS_BASE_OF
    #if defined(__has_builtin)
        #if __has_builtin(__is_base_of)
            #define RE_STD_HAS_IS_BASE_OF  1
        #else
            #define RE_STD_HAS_IS_BASE_OF  0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC)   ||                                  \
            defined(RE_STD_COMPILER_CLANG) ||                                  \
            defined(RE_STD_COMPILER_MSVC)  ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_IS_BASE_OF      1
    #else
        #define RE_STD_HAS_IS_BASE_OF      0
    #endif
#endif  // RE_STD_HAS_IS_BASE_OF


namespace re_std
{


// =============================================================================
// I.   IS_BASE_OF
// =============================================================================

#if RE_STD_HAS_IS_BASE_OF

// is_base_of
//   trait: true if Base is a base of Derived, or they are the same class.
template<typename Base,
         typename Derived>
struct is_base_of
    : integral_constant<bool, __is_base_of(Base, Derived)>
{};

#else

namespace internal
{

    // is_base_of_probe_
    //   trait: conversion probe.  `host_` converts to `const Base*` always
    // and to `const Derived*` only through its non-const operator; the
    // `Derived*` overload of probe_ is therefore viable exactly when a
    // derived-to-base conversion exists.
    template<typename Base,
             typename Derived>
    struct is_base_of_probe_
    {
        typedef char yes_type_[1];
        typedef char no_type_[2];

        struct host_
        {
            operator const Base*() const;
            operator const Derived*();
        };

        template<typename T>
        static yes_type_& probe_(const Derived*, T);
        static no_type_&  probe_(const Base*, int);

        static const bool value =
            ( sizeof(probe_(host_(), 0)) == sizeof(yes_type_) );
    };

}  // internal

// is_base_of
//   trait: portable fallback. The probe's two conversions give the
// builtin's answer for a public, protected, private or ambiguous base alike.
// A non-class operand is never a base; an identical class type counts as
// its own base.
template<typename Base,
         typename Derived>
struct is_base_of
    : integral_constant<bool,
        ( is_class<typename remove_cv<Base>::type>::value    &&
          is_class<typename remove_cv<Derived>::type>::value &&
          ( is_same<typename remove_cv<Base>::type,
                    typename remove_cv<Derived>::type>::value ||
            internal::is_base_of_probe_<
                typename remove_cv<Base>::type,
                typename remove_cv<Derived>::type>::value ) )>
{};

#endif  // RE_STD_HAS_IS_BASE_OF


// =============================================================================
// II.  IS_BASE_OF_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Base,
         typename Derived>
RE_STD_CONSTEXPR bool is_base_of_v = is_base_of<Base, Derived>::value;

#endif


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_BASE_OF_HPP
