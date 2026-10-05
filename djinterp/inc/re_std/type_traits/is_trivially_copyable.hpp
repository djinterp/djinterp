/*******************************************************************************
* djinterp [re_std]                                    is_trivially_copyable.hpp
*
* is_trivially_copyable trait header:
*   is_trivially_copyable<T>::value is true iff T may be copied by copying
* its object representation -- that is, iff memcpy is a valid substitute
* for assignment. It is the precondition for bit_cast, for memcpy-based
* container relocation, and for treating a type as raw bytes at all.
*
*   THIS IS THE TRAIT THE REST OF THE LIBRARY WAITED ON. bit/bit_cast.hpp
* currently calls __is_trivially_copyable directly because this header did
* not exist, and optional/optional_base.hpp names re_std::
* is_trivially_copyable, which is why <optional> did not compile. Both
* should now route through here.
*
*   NO LIBRARY-LEVEL IMPLEMENTATION EXISTS -- intrinsic or nothing.
*
*   PORTABILITY:
*   C++11 baseline. The _v spelling is C++14+, as elsewhere.
*
*
* path:      /inc/re_std/type_traits/is_trivially_copyable.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.08.25
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_TRIVIALLY_COPYABLE_HPP
#define RE_STD_TYPE_TRAITS_IS_TRIVIALLY_COPYABLE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   RE_STD_HAS_IS_TRIVIALLY_COPYABLE  (intrinsic detection)
// =============================================================================

#ifndef RE_STD_HAS_IS_TRIVIALLY_COPYABLE
    #if defined(__has_builtin)
        #if __has_builtin(__is_trivially_copyable)
            #define RE_STD_HAS_IS_TRIVIALLY_COPYABLE  1
        #else
            #define RE_STD_HAS_IS_TRIVIALLY_COPYABLE  0
        #endif
    #elif ( defined(RE_STD_COMPILER_GCC)   ||                                  \
            defined(RE_STD_COMPILER_CLANG) ||                                  \
            defined(RE_STD_COMPILER_MSVC)  ||                                  \
            defined(RE_STD_COMPILER_INTEL) )
        #define RE_STD_HAS_IS_TRIVIALLY_COPYABLE      1
    #else
        #define RE_STD_HAS_IS_TRIVIALLY_COPYABLE      0
    #endif
#endif  // RE_STD_HAS_IS_TRIVIALLY_COPYABLE


namespace re_std
{


// =============================================================================
// I.   IS_TRIVIALLY_COPYABLE
// =============================================================================

#if RE_STD_HAS_IS_TRIVIALLY_COPYABLE

// is_trivially_copyable
//   trait: intrinsic-backed -- the object representation may be copied with memcpy.
template<typename Type>
struct is_trivially_copyable : integral_constant<bool, __is_trivially_copyable(Type)>
{};

#else

// is_trivially_copyable
//   trait: degraded fallback (always false) when the intrinsic is absent.
// False is emphatically the safe direction here: a wrong true would
// authorise memcpy over a type with a non-trivial copy constructor,
// which corrupts rather than merely slows.
template<typename Type>
struct is_trivially_copyable : false_type
{};

#endif  // RE_STD_HAS_IS_TRIVIALLY_COPYABLE


// =============================================================================
// II.  IS_TRIVIALLY_COPYABLE_V (C++14+ variable)
// =============================================================================

#if RE_STD_LANG_HAS_VARIABLE_TEMPLATES

template<typename Type>
RE_STD_CONSTEXPR bool is_trivially_copyable_v = is_trivially_copyable<Type>::value;

#endif


}  // re_std


#endif  // RE_STD_TYPE_TRAITS_IS_TRIVIALLY_COPYABLE_HPP
