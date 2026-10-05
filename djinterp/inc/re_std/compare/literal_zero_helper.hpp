/*******************************************************************************
* djinterp [re_std]                                      literal_zero_helper.hpp
*
* literal_zero_helper internal type:
*   Pseudo-type accepted ONLY when its argument is the literal 0.
* Used as the RHS-of-comparison parameter type for the ordering
* category classes (strong_ordering, weak_ordering, partial_ordering)
* so that `cmp == 0`, `cmp < 0`, etc. compile while `cmp == 42` or
* `cmp == some_int_variable` does NOT.
*
*   THE TRICK:
*   The class accepts a pointer-to-itself in its converting ctor.
* When the user writes `cmp == 0`, the literal 0 undergoes the
* null-pointer-literal conversion to literal_zero_helper*, which then
* converts to literal_zero_helper via that ctor. Non-zero integer
* literals (or int-typed variables) are NOT null pointer literals
* per [conv.ptr]/1, so they cannot reach the ctor — the comparison
* is ill-formed.
*
*   This is the same mechanism libstdc++ uses for its
* __cmp_cat::__unspec sentinel.
*
*     strong_ordering cmp = ...;
*     (cmp == 0)         // OK: 0 -> literal_zero_helper* -> literal_zero_helper
*     (cmp == 42)        // ILL-FORMED: no implicit conversion path
*     int z = 0;
*     (cmp == z)         // ILL-FORMED: z is not a null pointer literal
*
*   PORTABILITY:
*   C++11+ (uses constexpr). The literal-0 trick itself works on any
* C++ tier but the constexpr usage in the ordering category classes
* requires C++11+.
*
*
* path:      /inc/re_std/compare/literal_zero_helper.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.17
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_COMPARE_LITERAL_ZERO_HELPER_HPP
#define RE_STD_COMPARE_LITERAL_ZERO_HELPER_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


// gate: the ordering category classes that use this helper need
// constexpr ctors and rvalue-friendly machinery (C++11+).
#if RE_STD_LANG_IS_CPP11_OR_HIGHER


namespace re_std
{


namespace internal
{


// =============================================================================
// I.   LITERAL_ZERO_HELPER
// =============================================================================

// literal_zero_helper
//   class: pseudo-type that accepts only the literal 0 (via the
// null-pointer-literal conversion through its self-pointer ctor).
// Non-literal-0 integer values cannot reach this type.
class literal_zero_helper
{
public:
    // ctor: takes a pointer to literal_zero_helper. The only way to
    // produce such a pointer from a non-pointer-typed expression in
    // C++ is via the literal 0 → null-pointer-literal conversion.
    RE_STD_CONSTEXPR
    literal_zero_helper(
        literal_zero_helper*
    ) RE_STD_NOEXCEPT
    {}
};


}  // internal


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_COMPARE_LITERAL_ZERO_HELPER_HPP
