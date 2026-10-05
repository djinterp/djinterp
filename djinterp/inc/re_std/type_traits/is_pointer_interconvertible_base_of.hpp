/*******************************************************************************
* djinterp [re_std]                      is_pointer_interconvertible_base_of.hpp
*
* is_pointer_interconvertible_base_of trait header:
*   pointer-interconvertible base detection:
*   `is_pointer_interconvertible_base_of<Base, Derived>` reports whether
* Derived is unambiguously derived from Base (disregarding cv-qualification)
* AND every object of type Derived is pointer-interconvertible with its Base
* subobject - or whether Base and Derived are the same NON-UNION class type.
* When it is true, `reinterpret_cast<Base*>(derived_ptr)` has a defined
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
* RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF for the real thing.
*
*   PRECONDITION:
*   Derived shall be a complete type when it is a non-union class type.  This
* mirrors std and cannot be enforced portably.
*
*
* path:      /inc/re_std/type_traits/is_pointer_interconvertible_base_of.hpp
*                                     is_pointer_interconvertible_base_of.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.12
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_POINTER_INTERCONVERTIBLE_BASE_OF_HPP
#define RE_STD_TYPE_TRAITS_IS_POINTER_INTERCONVERTIBLE_BASE_OF_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./type_traits.hpp"    // integral_constant, is_class, is_same, remove_cv


// =============================================================================
// INTRINSIC DETECTION
// =============================================================================

// RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF
//   constant: 1 if the __is_pointer_interconvertible_base_of builtin is
// available.  Detected independently of the other three members of the
// layout-compatibility family - vendors have shipped them at different times,
// and a family-wide macro would mis-report on at least one live compiler.
#ifndef RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF
    #if defined(__has_builtin)
        #if __has_builtin(__is_pointer_interconvertible_base_of)
            #define RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF  1
        #endif
    #endif

    #ifndef RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF
        #if ( defined(RE_STD_COMPILER_GCC) &&                                  \
              RE_STD_COMPILER_VERSION_AT_LEAST(12, 0, 0) )
            #define RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF  1
        #elif ( defined(RE_STD_COMPILER_MSVC) &&                               \
                RE_STD_COMPILER_VERSION_AT_LEAST(19, 29, 0) )
            #define RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF  1
        #else
            #define RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF  0
        #endif
    #endif  // RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF (fallback)
#endif  // RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF (outer guard)


namespace re_std
{

namespace internal
{

    // is_pointer_interconvertible_base_of_base
    //   trait: classification core for is_pointer_interconvertible_base_of.
    // The builtin disregards cv-qualification on both operands, so the
    // intrinsic arm forwards its arguments untouched.
#if RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF

    template<typename Base,
             typename Derived>
    struct is_pointer_interconvertible_base_of_base
        : integral_constant<bool,
              __is_pointer_interconvertible_base_of(Base, Derived)>
    {};

#else

    // is_pointer_interconvertible_base_of_base (degraded)
    //   trait: sound-subset classification used when the builtin is absent.
    // Only the same-non-union-class-type arm of the definition survives; a
    // real base/derived relationship cannot be decided without the builtin.
    template<typename Base,
             typename Derived>
    struct is_pointer_interconvertible_base_of_base
        : integral_constant<bool,
              (   is_class<typename remove_cv<Base>::type>::value
               && is_same<typename remove_cv<Base>::type,
                          typename remove_cv<Derived>::type>::value )>
    {};

#endif  // RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_BASE_OF

}  // internal


// is_pointer_interconvertible_base_of
//   trait: true if every Derived object is pointer-interconvertible with its
// Base subobject (or Base and Derived are the same non-union class type).
template<typename Base,
         typename Derived>
struct is_pointer_interconvertible_base_of
    : internal::is_pointer_interconvertible_base_of_base<Base, Derived>
{};

// is_pointer_interconvertible_base_of_v (C++14+)
#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES
    template<typename Base,
             typename Derived>
    RE_STD_CONSTEXPR bool is_pointer_interconvertible_base_of_v
        = is_pointer_interconvertible_base_of<Base, Derived>::value;
#endif  // RE_STD_LANG_HAS_VARIABLE_TEMPLATES

}  // re_std

#endif  // floor, for now


#endif  // RE_STD_TYPE_TRAITS_IS_POINTER_INTERCONVERTIBLE_BASE_OF_HPP
