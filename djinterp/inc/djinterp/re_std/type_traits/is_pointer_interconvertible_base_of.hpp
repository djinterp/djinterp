/******************************************************************************
* re_std [type_traits]                  is_pointer_interconvertible_base_of.hpp
*
*   pointer-interconvertible base detection:
*   `is_pointer_interconvertible_base_of<_Base, _Derived>` reports whether
* _Derived is unambiguously derived from _Base (disregarding cv-qualification)
* AND every object of type _Derived is pointer-interconvertible with its _Base
* subobject - or whether _Base and _Derived are the same NON-UNION class type.
* When it is true, `reinterpret_cast<_Base*>(derived_ptr)` has a defined
* result.
*
*   STD IS C++20; re_std IS C++98.
*   As with is_layout_compatible, the builtin behind this trait is accepted in
* every language mode and yields a core constant expression at every tier, so
* re_std ships the trait from C++98 with no language gate.  Only the builtin is
* gated.
*
*   DEGRADATION (no #error, ever):
*   Without the builtin the trait answers from a SOUND SUBSET: a non-union
* class type is pointer-interconvertible with itself, so that case still
* reports true; every genuine base/derived relationship reports false because
* it cannot be established portably.  Never a false POSITIVE, sometimes a false
* NEGATIVE.  Note the subset arm leans on is_class, which itself degrades to
* false without __is_class - so on a compiler missing both builtins this trait
* is uniformly false, which is still sound.  Test
* D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF for the real thing.
*
*   PRECONDITION:
*   _Derived shall be a complete type when it is a non-union class type.  This
* mirrors std and cannot be enforced portably.
*
*
* path:      /inc/djinterp/re_std/type_traits/
*                                     is_pointer_interconvertible_base_of.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.12
******************************************************************************/

#ifndef RESTD_TYPE_TRAITS_IS_POINTER_INTERCONVERTIBLE_BASE_OF_
#define RESTD_TYPE_TRAITS_IS_POINTER_INTERCONVERTIBLE_BASE_OF_ 1

// re_std
#include "./type_traits.hpp"    // integral_constant, is_class, is_same, remove_cv


// =============================================================================
// INTRINSIC DETECTION
// =============================================================================

// D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF
//   constant: 1 if the __is_pointer_interconvertible_base_of builtin is
// available.  Detected independently of the other three members of the
// layout-compatibility family - vendors have shipped them at different times,
// and a family-wide macro would mis-report on at least one live compiler.
#ifndef D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF
    #if defined(__has_builtin)
        #if __has_builtin(__is_pointer_interconvertible_base_of)
            #define D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF  1
        #endif
    #endif

    #ifndef D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF
        #if ( defined(D_ENV_COMPILER_GCC) &&                                  \
              D_ENV_COMPILER_VERSION_AT_LEAST(12, 0, 0) )
            #define D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF  1
        #elif ( defined(D_ENV_COMPILER_MSVC) &&                               \
                D_ENV_COMPILER_VERSION_AT_LEAST(19, 29, 0) )
            #define D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF  1
        #else
            #define D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF  0
        #endif
    #endif  // D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF (fallback)
#endif  // D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF (outer guard)


NS_DJINTERP
NS_RESTD

NS_INTERNAL

    // is_pointer_interconvertible_base_of_base
    //   trait: classification core for is_pointer_interconvertible_base_of.
    // The builtin disregards cv-qualification on both operands, so the
    // intrinsic arm forwards its arguments untouched.
#if D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF

    template<typename _Base,
             typename _Derived>
    struct is_pointer_interconvertible_base_of_base
        : integral_constant<bool,
              __is_pointer_interconvertible_base_of(_Base, _Derived)>
    {};

#else

    // is_pointer_interconvertible_base_of_base (degraded)
    //   trait: sound-subset classification used when the builtin is absent.
    // Only the same-non-union-class-type arm of the definition survives; a
    // real base/derived relationship cannot be decided without the builtin.
    template<typename _Base,
             typename _Derived>
    struct is_pointer_interconvertible_base_of_base
        : integral_constant<bool,
              (   is_class<typename remove_cv<_Base>::type>::value
               && is_same<typename remove_cv<_Base>::type,
                          typename remove_cv<_Derived>::type>::value )>
    {};

#endif  // D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF

NS_END  // internal


// is_pointer_interconvertible_base_of
//   trait: true if every _Derived object is pointer-interconvertible with its
// _Base subobject (or _Base and _Derived are the same non-union class type).
template<typename _Base,
         typename _Derived>
struct is_pointer_interconvertible_base_of
    : internal::is_pointer_interconvertible_base_of_base<_Base, _Derived>
{};

// is_pointer_interconvertible_base_of_v (C++14+)
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Base,
             typename _Derived>
    D_CONSTEXPR bool is_pointer_interconvertible_base_of_v
        = is_pointer_interconvertible_base_of<_Base, _Derived>::value;
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

NS_END  // re_std
NS_END  // djinterp

#endif  // RESTD_TYPE_TRAITS_IS_POINTER_INTERCONVERTIBLE_BASE_OF_
