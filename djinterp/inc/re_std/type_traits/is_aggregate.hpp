/*******************************************************************************
* djinterp [re_std]                                             is_aggregate.hpp
*
* is_aggregate trait header:
*   is_aggregate<T>::value is true iff T is an array type or a class type
* eligible for aggregate initialisation -- no user-declared or inherited
* constructors, no private or protected non-static data members, no
* virtual functions, no virtual or private or protected base classes.
*
*   The rules moved between C++11, C++14, C++17 and C++20, so the
* intrinsic's answer is the compiler's own reading of the tier it is
* compiling for. That is the correct behaviour and a library-side
* reimplementation could not track it.
*
*   BACK-PORT: std added this in C++17; re_std surfaces it from C++11
* wherever the compiler publishes the intrinsic -- a six-year lead.
*
*   PORTABILITY:
*   C++11 baseline. The _v spelling is C++14+, as elsewhere.
*
*
* path:      /inc/re_std/type_traits/is_aggregate.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_AGGREGATE_HPP
#define RE_STD_TYPE_TRAITS_IS_AGGREGATE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   RE_STD_HAS_IS_AGGREGATE  (intrinsic detection)
// =============================================================================

#ifndef RE_STD_HAS_IS_AGGREGATE
    #if defined(__has_builtin)
        #if __has_builtin(__is_aggregate)
            #define RE_STD_HAS_IS_AGGREGATE  1
        #else
            #define RE_STD_HAS_IS_AGGREGATE  0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC)   ||                                  \
            defined(RE_STD_COMPILER_CLANG) ||                                  \
            defined(RE_STD_COMPILER_MSVC)  ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_IS_AGGREGATE      1
    #else
        #define RE_STD_HAS_IS_AGGREGATE      0
    #endif
#endif  // RE_STD_HAS_IS_AGGREGATE


namespace re_std
{


// =============================================================================
// I.   IS_AGGREGATE
// =============================================================================

#if RE_STD_HAS_IS_AGGREGATE

// is_aggregate
//   trait: intrinsic-backed -- an array or a class with no user-declared constructors.
template<typename Type>
struct is_aggregate : integral_constant<bool, __is_aggregate(Type)>
{};

#else

// is_aggregate
//   trait: degraded fallback (always false) when the intrinsic is absent.
// False is conservative: callers use a constructor call rather than
// brace initialisation.
template<typename Type>
struct is_aggregate : false_type
{};

#endif  // RE_STD_HAS_IS_AGGREGATE


// =============================================================================
// II.  IS_AGGREGATE_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool is_aggregate_v = is_aggregate<Type>::value;

#endif


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_AGGREGATE_HPP
