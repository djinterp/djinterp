/******************************************************************************
* djinterp [re_std]                                is_trivially_copyable.hpp
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
* path:      /inc/djinterp/re_std/type_traits/is_trivially_copyable.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_TYPE_TRAITS_IS_TRIVIALLY_COPYABLE_
#define DJINTERP_RE_STD_TYPE_TRAITS_IS_TRIVIALLY_COPYABLE_ 1

// djinterp
#include "../../core/djinterp.hpp"
#include "./integral_constant.hpp"
#include "./true_type.hpp"
#include "./false_type.hpp"


// =============================================================================
// 0.   D_RE_STD_HAS_IS_TRIVIALLY_COPYABLE  (intrinsic detection)
// =============================================================================

#ifndef D_RE_STD_HAS_IS_TRIVIALLY_COPYABLE
    #if defined(__has_builtin)
        #if __has_builtin(__is_trivially_copyable)
            #define D_RE_STD_HAS_IS_TRIVIALLY_COPYABLE  1
        #else
            #define D_RE_STD_HAS_IS_TRIVIALLY_COPYABLE  0
        #endif
    #elif ( defined(D_ENV_COMPILER_GCC)   ||                                  \
            defined(D_ENV_COMPILER_CLANG) ||                                  \
            defined(D_ENV_COMPILER_MSVC)  ||                                  \
            defined(D_ENV_COMPILER_INTEL) )
        #define D_RE_STD_HAS_IS_TRIVIALLY_COPYABLE      1
    #else
        #define D_RE_STD_HAS_IS_TRIVIALLY_COPYABLE      0
    #endif
#endif  // D_RE_STD_HAS_IS_TRIVIALLY_COPYABLE


NS_RESTD


// =============================================================================
// I.   IS_TRIVIALLY_COPYABLE
// =============================================================================

#if D_RE_STD_HAS_IS_TRIVIALLY_COPYABLE

// is_trivially_copyable
//   trait: intrinsic-backed -- the object representation may be copied with memcpy.
template<typename _Type>
struct is_trivially_copyable : integral_constant<bool, __is_trivially_copyable(_Type)>
{};

#else

// is_trivially_copyable
//   trait: degraded fallback (always false) when the intrinsic is absent.
// False is emphatically the safe direction here: a wrong true would
// authorise memcpy over a type with a non-trivial copy constructor,
// which corrupts rather than merely slows.
template<typename _Type>
struct is_trivially_copyable : false_type
{};

#endif  // D_RE_STD_HAS_IS_TRIVIALLY_COPYABLE


// =============================================================================
// II.  IS_TRIVIALLY_COPYABLE_V (C++14+ variable)
// =============================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

template<typename _Type>
D_CONSTEXPR bool is_trivially_copyable_v = is_trivially_copyable<_Type>::value;

#endif


NS_END  // re_std


#endif  // DJINTERP_RE_STD_TYPE_TRAITS_IS_TRIVIALLY_COPYABLE_
