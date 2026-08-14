/******************************************************************************
* re_std [type_traits]                              is_corresponding_member.hpp
*
*   common-initial-sequence member detection:
*   `is_corresponding_member(m1, m2)` reports whether _StructA and _StructB are
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
*   The builtin is accepted in every language mode; D_CONSTEXPR and D_NOEXCEPT
* widen the function from C++11 up, nine years ahead of std's C++20.
*
*   DEGRADATION (no #error, ever):
*   Member offsets within a common initial sequence are not derivable from the
* type system, so there is no sound non-trivial subset.  Without the builtin
* the function exists and returns false unconditionally - never a false
* positive - and D_RESTD_HAS_IS_CORRESPONDING_MEMBER is 0 so callers can tell.
*
*
* path:      /inc/djinterp/re_std/type_traits/is_corresponding_member.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.12
******************************************************************************/

#ifndef RESTD_TYPE_TRAITS_IS_CORRESPONDING_MEMBER_
#define RESTD_TYPE_TRAITS_IS_CORRESPONDING_MEMBER_ 1

// re_std
#include "./type_traits.hpp"


// =============================================================================
// INTRINSIC DETECTION
// =============================================================================

// D_RESTD_HAS_IS_CORRESPONDING_MEMBER
//   constant: 1 if the __builtin_is_corresponding_member builtin is
// available.  Detected independently of the rest of the family for the same
// reason as its three siblings: vendors shipped these four at different times.
#ifndef D_RESTD_HAS_IS_CORRESPONDING_MEMBER
    #if defined(__has_builtin)
        #if __has_builtin(__builtin_is_corresponding_member)
            #define D_RESTD_HAS_IS_CORRESPONDING_MEMBER  1
        #endif
    #endif

    #ifndef D_RESTD_HAS_IS_CORRESPONDING_MEMBER
        #if ( defined(D_ENV_COMPILER_GCC) &&                                  \
              D_ENV_COMPILER_VERSION_AT_LEAST(12, 0, 0) )
            #define D_RESTD_HAS_IS_CORRESPONDING_MEMBER  1
        #elif ( defined(D_ENV_COMPILER_MSVC) &&                               \
                D_ENV_COMPILER_VERSION_AT_LEAST(19, 29, 0) )
            #define D_RESTD_HAS_IS_CORRESPONDING_MEMBER  1
        #else
            #define D_RESTD_HAS_IS_CORRESPONDING_MEMBER  0
        #endif
    #endif  // D_RESTD_HAS_IS_CORRESPONDING_MEMBER (fallback)
#endif  // D_RESTD_HAS_IS_CORRESPONDING_MEMBER (outer guard)


NS_DJINTERP
NS_RESTD

// is_corresponding_member
//   function: true if m1 and m2 name members at the same position in the
// common initial sequence of _StructA and _StructB.
#if D_RESTD_HAS_IS_CORRESPONDING_MEMBER

    template<typename _StructA,
             typename _StructB,
             typename _MemberA,
             typename _MemberB>
    D_NODISCARD D_CONSTEXPR bool is_corresponding_member(
        _MemberA _StructA::* m1,
        _MemberB _StructB::* m2) D_NOEXCEPT
    {
        return __builtin_is_corresponding_member(m1, m2);
    }

#else

    // is_corresponding_member (degraded)
    //   function: conservative stand-in used when the builtin is absent.
    // Always false.  Parameters are left unnamed so the degraded arm stays
    // -Wunused-parameter clean.
    template<typename _StructA,
             typename _StructB,
             typename _MemberA,
             typename _MemberB>
    D_NODISCARD D_CONSTEXPR bool is_corresponding_member(
        _MemberA _StructA::*,
        _MemberB _StructB::*) D_NOEXCEPT
    {
        return false;
    }

#endif  // D_RESTD_HAS_IS_CORRESPONDING_MEMBER

NS_END  // re_std
NS_END  // djinterp

#endif  // RESTD_TYPE_TRAITS_IS_CORRESPONDING_MEMBER_
