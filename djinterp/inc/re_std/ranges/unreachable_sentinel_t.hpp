/*******************************************************************************
* djinterp [re_std]                                   unreachable_sentinel_t.hpp
*
* unreachable_sentinel_t header:
*   Provides the C++20 never-equal sentinel. unreachable_sentinel_t
* compares unequal to every iterator unconditionally — useful as the
* sentinel for unbounded ranges (the infinite iota_view, counted
* iteration where the count is the stopping condition rather than
* position, etc.).
*
*   PORTABILITY:
*   - Type itself is available C++11+ (uses templated friend
*     operators). The class is empty and trivially constructible.
*   - The convenience constant re_std::unreachable_sentinel is defined
*     as an inline constexpr variable on C++17+ (avoiding ODR
*     conflicts across translation units), or as a TU-local
*     static constexpr instance on C++11/14.
*
*
* path:      /inc/re_std/ranges/unreachable_sentinel_t.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.13
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_RANGES_UNREACHABLE_SENTINEL_T_HPP
#define RE_STD_RANGES_UNREACHABLE_SENTINEL_T_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER


namespace re_std
{


// ===========================================================================
// I.   UNREACHABLE_SENTINEL_T
// ===========================================================================

// unreachable_sentinel_t
//   class: sentinel that compares unequal to any iterator. Used as
// the end-of-range marker for unbounded ranges.
struct unreachable_sentinel_t
{
    // iterator == unreachable: always false
    template<typename Iter>
    friend RE_STD_CONSTEXPR bool
    operator==(
        unreachable_sentinel_t const&,
        Iter const&
    )
    RE_STD_NOEXCEPT
    {
        return false;
    }

    template<typename Iter>
    friend RE_STD_CONSTEXPR bool
    operator==(
        Iter const&,
        unreachable_sentinel_t const&
    )
    RE_STD_NOEXCEPT
    {
        return false;
    }

    // iterator != unreachable: always true
    template<typename Iter>
    friend RE_STD_CONSTEXPR bool
    operator!=(
        unreachable_sentinel_t const&,
        Iter const&
    )
    RE_STD_NOEXCEPT
    {
        return true;
    }

    template<typename Iter>
    friend RE_STD_CONSTEXPR bool
    operator!=(
        Iter const&,
        unreachable_sentinel_t const&
    )
    RE_STD_NOEXCEPT
    {
        return true;
    }
};


// ===========================================================================
// II.  UNREACHABLE_SENTINEL (convenience constant)
// ===========================================================================

#if RE_STD_LANG_IS_CPP17_OR_HIGHER

// unreachable_sentinel
//   constant: inline constexpr instance. Matches the C++20
// std::unreachable_sentinel convenience variable.
inline RE_STD_CONSTEXPR unreachable_sentinel_t unreachable_sentinel = unreachable_sentinel_t();

#else

// unreachable_sentinel
//   constant: TU-local static constexpr instance. Without C++17
// inline variables, the constant is given internal linkage to
// avoid ODR conflicts when this header is included from multiple
// translation units. Address-taking yields a different pointer per
// TU; equality semantics (the only meaningful operation) are
// unaffected.
static RE_STD_CONSTEXPR unreachable_sentinel_t unreachable_sentinel = unreachable_sentinel_t();

#endif


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_RANGES_UNREACHABLE_SENTINEL_T_HPP
