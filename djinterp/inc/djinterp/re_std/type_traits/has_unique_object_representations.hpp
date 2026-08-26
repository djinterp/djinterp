/******************************************************************************
* djinterp [re_std]                          has_unique_object_representations.hpp
*
* has_unique_object_representations trait header:
*   has_unique_object_representations<T>::value is true iff T is trivially
* copyable AND any two objects with the same value have the same object
* representation -- no padding bytes, no redundant encodings.
*
*   THE PRACTICAL MEANING IS: MAY I HASH THIS BY ITS BYTES? If the answer
* is true, hashing the object representation is sound. If it is false --
* because of padding, or because the type is a float with a negative zero
* -- then two equal values can hash differently, which breaks every
* hash-based container quietly and non-reproducibly.
*
*   BACK-PORT: std added this in C++17; re_std surfaces it from C++11 --
* a six-year lead.
*
*   PORTABILITY:
*   C++11 baseline. The _v spelling is C++14+, as elsewhere.
*
*
* path:      /inc/djinterp/re_std/type_traits/has_unique_object_representations.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_HAS_UNIQUE_OBJECT_REPRESENTATIONS_
#define DJINTERP_RE_STD_TYPE_TRAITS_HAS_UNIQUE_OBJECT_REPRESENTATIONS_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   D_RE_STD_HAS_HAS_UNIQUE_OBJECT_REPRESENTATIONS  (intrinsic detection)
// =============================================================================

#ifndef D_RE_STD_HAS_HAS_UNIQUE_OBJECT_REPRESENTATIONS
    #if defined(__has_builtin)
        #if __has_builtin(__has_unique_object_representations)
            #define D_RE_STD_HAS_HAS_UNIQUE_OBJECT_REPRESENTATIONS  1
        #else
            #define D_RE_STD_HAS_HAS_UNIQUE_OBJECT_REPRESENTATIONS  0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC)   ||                                  \
            defined(D_ENV_COMPILER_CLANG) ||                                  \
            defined(D_ENV_COMPILER_MSVC)  ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RE_STD_HAS_HAS_UNIQUE_OBJECT_REPRESENTATIONS      1
    #else
        #define D_RE_STD_HAS_HAS_UNIQUE_OBJECT_REPRESENTATIONS      0
    #endif
#endif  // D_RE_STD_HAS_HAS_UNIQUE_OBJECT_REPRESENTATIONS


NS_RESTD


// =============================================================================
// I.   HAS_UNIQUE_OBJECT_REPRESENTATIONS
// =============================================================================

#if D_RE_STD_HAS_HAS_UNIQUE_OBJECT_REPRESENTATIONS

// has_unique_object_representations
//   trait: intrinsic-backed -- every distinct value has exactly one object representation.
template<typename _Type>
struct has_unique_object_representations : integral_constant<bool, __has_unique_object_representations(_Type)>
{};

#else

// has_unique_object_representations
//   trait: degraded fallback (always false) when the intrinsic is absent.
// False is emphatically the safe direction: a wrong true would authorise
// byte-wise hashing of a type with padding, producing different hashes
// for equal values -- a bug that appears only under specific padding
// contents and is close to unreproducible.
template<typename _Type>
struct has_unique_object_representations : false_type
{};

#endif  // D_RE_STD_HAS_HAS_UNIQUE_OBJECT_REPRESENTATIONS


// =============================================================================
// II.  HAS_UNIQUE_OBJECT_REPRESENTATIONS_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool has_unique_object_representations_v = has_unique_object_representations<_Type>::value;

#endif


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_HAS_UNIQUE_OBJECT_REPRESENTATIONS_
