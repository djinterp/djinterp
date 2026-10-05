/*******************************************************************************
* djinterp [re_std]                                         expected_compare.hpp
*
* expected comparison header:
*   Provides operator== overloads for expected. Three flavours mirror
* the C++23 standard:
*
*     operator==(expected<T,E>, expected<U,G>)   - both expecteds
*     operator==(expected<T,E>, U)               - expected vs value
*     operator==(expected<T,E>, unexpected<G>)   - expected vs error
*
*   Plus the same three for expected<void, E>. Operator!= is
* synthesised in C++20 from op== but re_std ships it explicitly on
* every tier (gated out for C++20+ to avoid ambiguity with the
* compiler-synthesised version).
*
*
* path:      /inc/re_std/expected/expected_compare.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.19
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_EXPECTED_EXPECTED_COMPARE_HPP
#define RE_STD_EXPECTED_EXPECTED_COMPARE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "./expected.hpp"
#include "./unexpected.hpp"


namespace re_std
{


// ===========================================================================
// I.   OPERATOR==(expected, expected)
// ===========================================================================

// Two expecteds compare equal iff:
//   - both have values and the values compare equal, OR
//   - both have errors and the errors compare equal.
// Cross-type comparison (different T/U or E/G) is permitted iff the
// relevant cross-type op== exists.
template<typename T1, typename E1,
         typename T2, typename E2>
RE_STD_CONSTEXPR_CPP20 bool
operator==(
    expected<T1, E1> const& _lhs,
    expected<T2, E2> const& _rhs
)
{
    return ( _lhs.has_value() == _rhs.has_value() )
        && ( _lhs.has_value()
                 ? (*_lhs == *_rhs)
                 : (_lhs.error() == _rhs.error()) );
}

// expected<void, E1> vs expected<void, E2>
template<typename E1,
         typename E2>
RE_STD_CONSTEXPR_CPP20 bool
operator==(
    expected<void, E1> const& _lhs,
    expected<void, E2> const& _rhs
)
{
    return ( _lhs.has_value() == _rhs.has_value() )
        && ( _lhs.has_value()
                 ? true
                 : (_lhs.error() == _rhs.error()) );
}


// ===========================================================================
// II.  OPERATOR==(expected, value)
// ===========================================================================

// An expected compares equal to a bare value iff it has a value
// and that value compares equal to the bare one.
template<typename T,
         typename E,
         typename U>
RE_STD_CONSTEXPR_CPP20 bool
operator==(
    expected<T, E> const& _lhs,
    U const&                _rhs
)
{
    return _lhs.has_value() && (*_lhs == _rhs);
}


// ===========================================================================
// III. OPERATOR==(expected, unexpected)
// ===========================================================================

// An expected compares equal to an unexpected iff it does not have
// a value and the errors compare equal.
template<typename T,
         typename E,
         typename G>
RE_STD_CONSTEXPR_CPP20 bool
operator==(
    expected<T, E> const&    _lhs,
    unexpected<G> const&      _rhs
)
{
    return !_lhs.has_value() && (_lhs.error() == _rhs.error());
}

// expected<void, E> vs unexpected<G>
template<typename E,
         typename G>
RE_STD_CONSTEXPR_CPP20 bool
operator==(
    expected<void, E> const&  _lhs,
    unexpected<G> const&      _rhs
)
{
    return !_lhs.has_value() && (_lhs.error() == _rhs.error());
}


// ===========================================================================
// IV.  OPERATOR!=  (C++11-C++17 only)
// ===========================================================================
// C++20 synthesises these from op==; we provide them explicitly on
// earlier tiers and skip on C++20+ to avoid ambiguity.

#if !RE_STD_LANG_IS_CPP20_OR_HIGHER

template<typename T1, typename E1,
         typename T2, typename E2>
RE_STD_CONSTEXPR_CPP20 bool
operator!=(
    expected<T1, E1> const& _lhs,
    expected<T2, E2> const& _rhs
)
{
    return !(_lhs == _rhs);
}

template<typename E1,
         typename E2>
RE_STD_CONSTEXPR_CPP20 bool
operator!=(
    expected<void, E1> const& _lhs,
    expected<void, E2> const& _rhs
)
{
    return !(_lhs == _rhs);
}

template<typename T,
         typename E,
         typename U>
RE_STD_CONSTEXPR_CPP20 bool
operator!=(
    expected<T, E> const& _lhs,
    U const&                _rhs
)
{
    return !(_lhs == _rhs);
}

template<typename T,
         typename E,
         typename G>
RE_STD_CONSTEXPR_CPP20 bool
operator!=(
    expected<T, E> const&    _lhs,
    unexpected<G> const&      _rhs
)
{
    return !(_lhs == _rhs);
}

template<typename E,
         typename G>
RE_STD_CONSTEXPR_CPP20 bool
operator!=(
    expected<void, E> const&  _lhs,
    unexpected<G> const&      _rhs
)
{
    return !(_lhs == _rhs);
}

#endif  // !C++20


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_EXPECTED_EXPECTED_COMPARE_HPP
