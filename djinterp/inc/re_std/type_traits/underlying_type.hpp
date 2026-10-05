/*******************************************************************************
* djinterp [re_std]                                          underlying_type.hpp
*
* underlying_type trait header:
*   Yields the underlying integral type of an enumeration. Implemented
* via the __underlying_type compiler builtin, supported by GCC 4.7+,
* all Clang, MSVC 2012+ (v11), and Intel 13.0+.
*
*     enum E { a, b };
*     underlying_type<E>::type  -> int (typical default)
*
*     enum class F : unsigned long;
*     underlying_type<F>::type  -> unsigned long
*
*   FALLBACK BEHAVIOR:
*   When the intrinsic is unavailable, underlying_type is NOT defined
* (no fallback typedef). Code that uses it must be gated on
* RE_STD_HAS_UNDERLYING_TYPE. This is safe in re_std::any because the
* only consumer is itself gated on is_enum::value, which is false when
* intrinsics are absent.
*
*   DETECTION MACRO:
*   RE_STD_HAS_UNDERLYING_TYPE is set to 1 if the intrinsic is
* available, 0 otherwise. Users may pre-define it to override detection.
*
*
* path:      /inc/re_std/type_traits/underlying_type.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.28
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_UNDERLYING_TYPE_HPP
#define RE_STD_TYPE_TRAITS_UNDERLYING_TYPE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// =============================================================================
// 0.   RE_STD_HAS_UNDERLYING_TYPE DETECTION
// =============================================================================

// RE_STD_HAS_UNDERLYING_TYPE
//   constant: 1 if the __underlying_type compiler intrinsic is
// available, 0 otherwise. Users may pre-define to override.
#ifndef RE_STD_HAS_UNDERLYING_TYPE
    #if defined(__has_builtin)
        #if __has_builtin(__underlying_type)
            #define RE_STD_HAS_UNDERLYING_TYPE 1
        #else
            #define RE_STD_HAS_UNDERLYING_TYPE 0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC) ||                                   \
            defined(RE_STD_COMPILER_CLANG) ||                                 \
            defined(RE_STD_COMPILER_MSVC) ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_UNDERLYING_TYPE 1
    #else
        #define RE_STD_HAS_UNDERLYING_TYPE 0
    #endif
#endif


#if RE_STD_HAS_UNDERLYING_TYPE


namespace re_std
{


// =============================================================================
// I.   UNDERLYING_TYPE
// =============================================================================

// underlying_type
//   trait: yields the underlying integral type of enumeration Type.
// Uses __underlying_type builtin.
template<typename Type>
struct underlying_type
{
    typedef __underlying_type(Type) type;
};


// =============================================================================
// II.  UNDERLYING_TYPE_T (C++11+ alias)
// =============================================================================

#if RE_STD_LANG_HAS_ALIAS_TEMPLATES

    // underlying_type_t
    //   alias: convenience alias for underlying_type<Type>::type.
    template<typename Type>
    using underlying_type_t = typename underlying_type<Type>::type;

#endif  // RE_STD_LANG_HAS_ALIAS_TEMPLATES


}  // re_std


#endif  // RE_STD_HAS_UNDERLYING_TYPE


#endif  // RE_STD_TYPE_TRAITS_UNDERLYING_TYPE_HPP
