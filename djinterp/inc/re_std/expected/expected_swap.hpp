/*******************************************************************************
* djinterp [re_std]                                            expected_swap.hpp
*
* expected swap specialization header:
*   Provides non-member swap overloads for re_std::expected and
* re_std::unexpected. ADL-friendly; delegate to the member swap on
* each.
*
*   CONSTEXPR:
*   constexpr from C++20 (matches std).
*
*
* path:      /inc/re_std/expected/expected_swap.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.19
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_EXPECTED_EXPECTED_SWAP_HPP
#define RE_STD_EXPECTED_EXPECTED_SWAP_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "./expected.hpp"
#include "./unexpected.hpp"


namespace re_std
{


// ===========================================================================
// I.   swap (expected)
// ===========================================================================

// swap (expected<T, E>)
template<typename T,
         typename E>
RE_STD_CONSTEXPR_CPP20 void
swap(
    expected<T, E>& _lhs,
    expected<T, E>& _rhs
)
{
    _lhs.swap(_rhs);

    return;
}

// swap (expected<void, E>)
template<typename E>
RE_STD_CONSTEXPR_CPP20 void
swap(
    expected<void, E>& _lhs,
    expected<void, E>& _rhs
)
{
    _lhs.swap(_rhs);

    return;
}


// ===========================================================================
// II.  swap (unexpected)
// ===========================================================================

// swap (unexpected<E>)
template<typename E>
RE_STD_CONSTEXPR_CPP20 void
swap(
    unexpected<E>& _lhs,
    unexpected<E>& _rhs
) RE_STD_NOEXCEPT
{
    _lhs.swap(_rhs);

    return;
}


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_EXPECTED_EXPECTED_SWAP_HPP
