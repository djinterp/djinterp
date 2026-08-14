/******************************************************************************
* re_std [type_traits]               is_pointer_interconvertible_with_class.hpp
*
*   pointer-interconvertible member detection:
*   `is_pointer_interconvertible_with_class(_Member _Struct::* mp)` reports
* whether, for an object `s` of type _Struct, `s.*mp` names a subobject that
* `s` is pointer-interconvertible with.  When it is true,
* `reinterpret_cast<_Member&>(s)` has a defined result and designates the same
* subobject as `s.*mp`.
*
*   THIS IS A FUNCTION, NOT A TRAIT CLASS.
*   The answer depends on WHICH member is named, not merely on the types, so
* std spells it as a function template taking a pointer-to-member.  re_std
* keeps that shape exactly.  Note that `&_Struct::m` does not always have type
* `_Member _Struct::*` when m is inherited; specify the template arguments
* explicitly when that distinction matters.
*
*   STD IS C++20; re_std IS C++98 (constexpr from C++11).
*   The builtin is accepted in every language mode, so the function is
* declared from C++98 down at the language floor.  D_CONSTEXPR and D_NOEXCEPT
* widen it to a constexpr noexcept function from C++11 - nine years ahead of
* std, which only has it constexpr at C++20.  On C++98/03 it is an ordinary
* runtime call that still returns the right answer.
*
*   DEGRADATION (no #error, ever):
*   Whether a given member sits at offset zero of a standard-layout class is
* not derivable from the type system, so there is NO sound non-trivial subset
* here.  Without the builtin the function still exists and returns false
* unconditionally - never a false positive, always safe to guard a cast on -
* and D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS is 0 so callers can
* tell.  Clang shipped the two layout-compatibility TRAITS well before this
* FUNCTION's builtin, so this arm is live on real compilers, not theoretical.
*
*
* path:      /inc/djinterp/re_std/type_traits/
*                                  is_pointer_interconvertible_with_class.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.12
******************************************************************************/

#ifndef RESTD_TYPE_TRAITS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS_
#define RESTD_TYPE_TRAITS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS_ 1

// re_std
#include "./type_traits.hpp"


// =============================================================================
// INTRINSIC DETECTION
// =============================================================================

// D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS
//   constant: 1 if the __builtin_is_pointer_interconvertible_with_class
// builtin is available.  Detected independently of the layout-compatibility
// TRAITS: this builtin has historically lagged them on Clang, so folding all
// four into one macro would silently disable a working trait or enable a
// missing one.
#ifndef D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS
    #if defined(__has_builtin)
        #if __has_builtin(__builtin_is_pointer_interconvertible_with_class)
            #define D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS  1
        #endif
    #endif

    #ifndef D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS
        #if ( defined(D_ENV_COMPILER_GCC) &&                                  \
              D_ENV_COMPILER_VERSION_AT_LEAST(12, 0, 0) )
            #define D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS  1
        #elif ( defined(D_ENV_COMPILER_MSVC) &&                               \
                D_ENV_COMPILER_VERSION_AT_LEAST(19, 29, 0) )
            #define D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS  1
        #else
            #define D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS  0
        #endif
    #endif  // D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS (fallback)
#endif  // D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS (outer guard)


NS_DJINTERP
NS_RESTD

// is_pointer_interconvertible_with_class
//   function: true if an object of type _Struct is pointer-interconvertible
// with the subobject named by mp.
#if D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS

    template<typename _Struct,
             typename _Member>
    D_NODISCARD D_CONSTEXPR bool is_pointer_interconvertible_with_class(
        _Member _Struct::* mp) D_NOEXCEPT
    {
        return __builtin_is_pointer_interconvertible_with_class(mp);
    }

#else

    // is_pointer_interconvertible_with_class (degraded)
    //   function: conservative stand-in used when the builtin is absent.
    // Always false.  The parameter is left unnamed so the degraded arm stays
    // -Wunused-parameter clean.
    template<typename _Struct,
             typename _Member>
    D_NODISCARD D_CONSTEXPR bool is_pointer_interconvertible_with_class(
        _Member _Struct::*) D_NOEXCEPT
    {
        return false;
    }

#endif  // D_RESTD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS

NS_END  // re_std
NS_END  // djinterp

#endif  // RESTD_TYPE_TRAITS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS_
