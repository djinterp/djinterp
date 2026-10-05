/*******************************************************************************
* djinterp [re_std]                        has_unique_object_representations.hpp
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
* path:      /inc/re_std/type_traits/has_unique_object_representations.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_HAS_UNIQUE_OBJECT_REPRESENTATIONS_HPP
#define RE_STD_TYPE_TRAITS_HAS_UNIQUE_OBJECT_REPRESENTATIONS_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   RE_STD_HAS_HAS_UNIQUE_OBJECT_REPRESENTATIONS  (intrinsic detection)
// =============================================================================

#ifndef RE_STD_HAS_HAS_UNIQUE_OBJECT_REPRESENTATIONS
    #if defined(__has_builtin)
        #if __has_builtin(__has_unique_object_representations)
            #define RE_STD_HAS_HAS_UNIQUE_OBJECT_REPRESENTATIONS  1
        #else
            #define RE_STD_HAS_HAS_UNIQUE_OBJECT_REPRESENTATIONS  0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC)   ||                                  \
            defined(RE_STD_COMPILER_CLANG) ||                                  \
            defined(RE_STD_COMPILER_MSVC)  ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_HAS_UNIQUE_OBJECT_REPRESENTATIONS      1
    #else
        #define RE_STD_HAS_HAS_UNIQUE_OBJECT_REPRESENTATIONS      0
    #endif
#endif  // RE_STD_HAS_HAS_UNIQUE_OBJECT_REPRESENTATIONS


namespace re_std
{


// =============================================================================
// I.   HAS_UNIQUE_OBJECT_REPRESENTATIONS
// =============================================================================

#if RE_STD_HAS_HAS_UNIQUE_OBJECT_REPRESENTATIONS

// has_unique_object_representations
//   trait: intrinsic-backed -- every distinct value has exactly one object representation.
template<typename Type>
struct has_unique_object_representations : integral_constant<bool, __has_unique_object_representations(Type)>
{};

#else

// has_unique_object_representations
//   trait: degraded fallback (always false) when the intrinsic is absent.
// False is emphatically the safe direction: a wrong true would authorise
// byte-wise hashing of a type with padding, producing different hashes
// for equal values -- a bug that appears only under specific padding
// contents and is close to unreproducible.
template<typename Type>
struct has_unique_object_representations : false_type
{};

#endif  // RE_STD_HAS_HAS_UNIQUE_OBJECT_REPRESENTATIONS


// =============================================================================
// II.  HAS_UNIQUE_OBJECT_REPRESENTATIONS_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool has_unique_object_representations_v = has_unique_object_representations<Type>::value;

#endif


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_HAS_UNIQUE_OBJECT_REPRESENTATIONS_HPP
