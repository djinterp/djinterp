/*******************************************************************************
* djinterp [re_std]                                  is_corresponding_member.hpp
*
* is_corresponding_member trait header:
*   common-initial-sequence member detection:
*   `is_corresponding_member(m1, m2)` reports whether StructA and StructB are
* standard-layout, non-union class types and m1 and m2 name members at the same
* position in their common initial sequence.  This is the query that makes
* reading the common prefix of two struct types through a union well-defined,
* and it completes the C++20 layout-compatibility family alongside
* is_layout_compatible.
*
*   THIS IS A FUNCTION, NOT A TRAIT CLASS.
*   As with is_pointer_interconvertible_with_class, the answer depends on WHICH
* members are named, so std spells it as a function template over two
* pointers-to-member.  The same inherited-member caveat applies: `&S::m` is not
* always `M S::*`, so specify the template arguments when it matters.
*
*   NOT NAMED IN THE ROADMAP ENTRY, SHIPPED ANYWAY.
*   The roadmap listed three symbols for this milestone.  is_corresponding_member
* is the fourth member of the same P0466R5 family, lives in the same header,
* rides the same builtin-detection pattern, and would otherwise be the only
* layout-compatibility symbol left uncatalogued.  Shipping it here costs one
* file and closes the family.
*
*   STD IS C++20; re_std IS C++98 (constexpr from C++11).
*   The builtin is accepted in every language mode; RE_STD_CONSTEXPR and RE_STD_NOEXCEPT
* widen the function from C++11 up, nine years ahead of std's C++20.
*
*   DEGRADATION (no #error, ever):
*   Member offsets within a common initial sequence are not derivable from the
* type system, so there is no sound non-trivial subset.  Without the builtin
* the function exists and returns false unconditionally - never a false
* positive - and RE_STD_HAS_IS_CORRESPONDING_MEMBER is 0 so callers can tell.
*
*
* path:      /inc/re_std/type_traits/is_corresponding_member.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.12
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_TYPE_TRAITS_IS_CORRESPONDING_MEMBER_HPP
#define RE_STD_TYPE_TRAITS_IS_CORRESPONDING_MEMBER_HPP 1

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

// RE_STD_HAS_IS_CORRESPONDING_MEMBER
//   constant: 1 if the __builtin_is_corresponding_member builtin is
// available.  Detected independently of the rest of the family for the same
// reason as its three siblings: vendors shipped these four at different times.
#ifndef RE_STD_HAS_IS_CORRESPONDING_MEMBER
    #if defined(__has_builtin)
        #if __has_builtin(__builtin_is_corresponding_member)
            #define RE_STD_HAS_IS_CORRESPONDING_MEMBER  1
        #endif
    #endif

    #ifndef RE_STD_HAS_IS_CORRESPONDING_MEMBER
        #if ( defined(RE_STD_COMPILER_GCC) &&                                  \
              RE_STD_COMPILER_VERSION_AT_LEAST(12, 0, 0) )
            #define RE_STD_HAS_IS_CORRESPONDING_MEMBER  1
        #elif ( defined(RE_STD_COMPILER_MSVC) &&                               \
                RE_STD_COMPILER_VERSION_AT_LEAST(19, 29, 0) )
            #define RE_STD_HAS_IS_CORRESPONDING_MEMBER  1
        #else
            #define RE_STD_HAS_IS_CORRESPONDING_MEMBER  0
        #endif
    #endif  // RE_STD_HAS_IS_CORRESPONDING_MEMBER (fallback)
#endif  // RE_STD_HAS_IS_CORRESPONDING_MEMBER (outer guard)


namespace re_std
{

// is_corresponding_member
//   function: true if m1 and m2 name members at the same position in the
// common initial sequence of StructA and StructB.
#if RE_STD_HAS_IS_CORRESPONDING_MEMBER

    template<typename StructA,
             typename StructB,
             typename MemberA,
             typename MemberB>
    RE_STD_NODISCARD RE_STD_CONSTEXPR bool is_corresponding_member(
        MemberA StructA::* m1,
        MemberB StructB::* m2) RE_STD_NOEXCEPT
    {
        return __builtin_is_corresponding_member(m1, m2);
    }

#else

    // is_corresponding_member (degraded)
    //   function: conservative stand-in used when the builtin is absent.
    // Always false.  Parameters are left unnamed so the degraded arm stays
    // -Wunused-parameter clean.
    template<typename StructA,
             typename StructB,
             typename MemberA,
             typename MemberB>
    RE_STD_NODISCARD RE_STD_CONSTEXPR bool is_corresponding_member(
        MemberA StructA::*,
        MemberB StructB::*) RE_STD_NOEXCEPT
    {
        return false;
    }

#endif  // RE_STD_HAS_IS_CORRESPONDING_MEMBER

}  // re_std

#endif  // floor, for now


#endif  // RE_STD_TYPE_TRAITS_IS_CORRESPONDING_MEMBER_HPP
