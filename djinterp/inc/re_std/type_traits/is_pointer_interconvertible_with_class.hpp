/*******************************************************************************
* djinterp [re_std]                   is_pointer_interconvertible_with_class.hpp
*
* is_pointer_interconvertible_with_class trait header:
*   pointer-interconvertible member detection:
*   `is_pointer_interconvertible_with_class(Member Struct::* mp)` reports
* whether, for an object `s` of type Struct, `s.*mp` names a subobject that
* `s` is pointer-interconvertible with.  When it is true,
* `reinterpret_cast<Member&>(s)` has a defined result and designates the same
* subobject as `s.*mp`.
*
*   THIS IS A FUNCTION, NOT A TRAIT CLASS.
*   The answer depends on WHICH member is named, not merely on the types, so
* std spells it as a function template taking a pointer-to-member.  re_std
* keeps that shape exactly.  Note that `&Struct::m` does not always have type
* `Member Struct::*` when m is inherited; specify the template arguments
* explicitly when that distinction matters.
*
*   STD IS C++20; re_std IS C++98 (constexpr from C++11).
*   The builtin is accepted in every language mode, so the function is
* declared from C++98 down at the language floor.  RE_STD_CONSTEXPR and RE_STD_NOEXCEPT
* widen it to a constexpr noexcept function from C++11 - nine years ahead of
* std, which only has it constexpr at C++20.  On C++98/03 it is an ordinary
* runtime call that still returns the right answer.
*
*   DEGRADATION (no #error, ever):
*   Whether a given member sits at offset zero of a standard-layout class is
* not derivable from the type system, so there is NO sound non-trivial subset
* here.  Without the builtin the function still exists and returns false
* unconditionally - never a false positive, always safe to guard a cast on -
* and RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS is 0 so callers can
* tell.  Clang shipped the two layout-compatibility TRAITS well before this
* FUNCTION's builtin, so this arm is live on real compilers, not theoretical.
*
*
* path:      /inc/re_std/type_traits/is_pointer_interconvertible_with_class.hpp
*                                  is_pointer_interconvertible_with_class.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.12
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS_HPP
#define RE_STD_TYPE_TRAITS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// re_std
#include "./type_traits.hpp"


// =============================================================================
// INTRINSIC DETECTION
// =============================================================================

// RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS
//   constant: 1 if the __builtin_is_pointer_interconvertible_with_class
// builtin is available.  Detected independently of the layout-compatibility
// TRAITS: this builtin has historically lagged them on Clang, so folding all
// four into one macro would silently disable a working trait or enable a
// missing one.
#ifndef RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS
    #if defined(__has_builtin)
        #if __has_builtin(__builtin_is_pointer_interconvertible_with_class)
            #define RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS  1
        #endif
    #endif

    #ifndef RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS
        #if ( defined(RE_STD_COMPILER_GCC) &&                                  \
              RE_STD_COMPILER_VERSION_AT_LEAST(12, 0, 0) )
            #define RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS  1
        #elif ( defined(RE_STD_COMPILER_MSVC) &&                               \
                RE_STD_COMPILER_VERSION_AT_LEAST(19, 29, 0) )
            #define RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS  1
        #else
            #define RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS  0
        #endif
    #endif  // RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS (fallback)
#endif  // RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS (outer guard)


namespace re_std
{

// is_pointer_interconvertible_with_class
//   function: true if an object of type Struct is pointer-interconvertible
// with the subobject named by mp.
#if RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS

    template<typename Struct,
             typename Member>
    RE_STD_NODISCARD RE_STD_CONSTEXPR bool is_pointer_interconvertible_with_class(
        Member Struct::* mp) RE_STD_NOEXCEPT
    {
        return __builtin_is_pointer_interconvertible_with_class(mp);
    }

#else

    // is_pointer_interconvertible_with_class (degraded)
    //   function: conservative stand-in used when the builtin is absent.
    // Always false.  The parameter is left unnamed so the degraded arm stays
    // -Wunused-parameter clean.
    template<typename Struct,
             typename Member>
    RE_STD_NODISCARD RE_STD_CONSTEXPR bool is_pointer_interconvertible_with_class(
        Member Struct::*) RE_STD_NOEXCEPT
    {
        return false;
    }

#endif  // RE_STD_HAS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS

}  // re_std

#endif  // floor, for now


#endif  // RE_STD_TYPE_TRAITS_IS_POINTER_INTERCONVERTIBLE_WITH_CLASS_HPP
