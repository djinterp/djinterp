/******************************************************************************
* djinterp [restd]                                          expected_compare.hpp
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
* synthesised in C++20 from op== but restd ships it explicitly on
* every tier (gated out for C++20+ to avoid ambiguity with the
* compiler-synthesised version).
*
*
* path:      /inc/djinterp/restd/expected/expected_compare.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.19
******************************************************************************/

#ifndef DJINTERP_RESTD_EXPECTED_COMPARE_
#define DJINTERP_RESTD_EXPECTED_COMPARE_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "./expected.hpp"
#include "./unexpected.hpp"


#ifndef D_CONSTEXPR_CPP20
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        #define D_CONSTEXPR_CPP20   constexpr
    #else
        #define D_CONSTEXPR_CPP20
    #endif
#endif


NS_RESTD


// ===========================================================================
// I.   OPERATOR==(expected, expected)
// ===========================================================================

// Two expecteds compare equal iff:
//   - both have values and the values compare equal, OR
//   - both have errors and the errors compare equal.
// Cross-type comparison (different T/U or E/G) is permitted iff the
// relevant cross-type op== exists.
template<typename _T1, typename _E1,
         typename _T2, typename _E2>
D_CONSTEXPR_CPP20 bool
operator==(
    expected<_T1, _E1> const& _lhs,
    expected<_T2, _E2> const& _rhs
)
{
    return ( _lhs.has_value() == _rhs.has_value() )
        && ( _lhs.has_value()
                 ? (*_lhs == *_rhs)
                 : (_lhs.error() == _rhs.error()) );
}

// expected<void, E1> vs expected<void, E2>
template<typename _E1,
         typename _E2>
D_CONSTEXPR_CPP20 bool
operator==(
    expected<void, _E1> const& _lhs,
    expected<void, _E2> const& _rhs
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
template<typename _T,
         typename _E,
         typename _U>
D_CONSTEXPR_CPP20 bool
operator==(
    expected<_T, _E> const& _lhs,
    _U const&                _rhs
)
{
    return _lhs.has_value() && (*_lhs == _rhs);
}


// ===========================================================================
// III. OPERATOR==(expected, unexpected)
// ===========================================================================

// An expected compares equal to an unexpected iff it does not have
// a value and the errors compare equal.
template<typename _T,
         typename _E,
         typename _G>
D_CONSTEXPR_CPP20 bool
operator==(
    expected<_T, _E> const&    _lhs,
    unexpected<_G> const&      _rhs
)
{
    return !_lhs.has_value() && (_lhs.error() == _rhs.error());
}

// expected<void, E> vs unexpected<G>
template<typename _E,
         typename _G>
D_CONSTEXPR_CPP20 bool
operator==(
    expected<void, _E> const&  _lhs,
    unexpected<_G> const&      _rhs
)
{
    return !_lhs.has_value() && (_lhs.error() == _rhs.error());
}


// ===========================================================================
// IV.  OPERATOR!=  (C++11-C++17 only)
// ===========================================================================
// C++20 synthesises these from op==; we provide them explicitly on
// earlier tiers and skip on C++20+ to avoid ambiguity.

#if !D_ENV_LANG_IS_CPP20_OR_HIGHER

template<typename _T1, typename _E1,
         typename _T2, typename _E2>
D_CONSTEXPR_CPP20 bool
operator!=(
    expected<_T1, _E1> const& _lhs,
    expected<_T2, _E2> const& _rhs
)
{
    return !(_lhs == _rhs);
}

template<typename _E1,
         typename _E2>
D_CONSTEXPR_CPP20 bool
operator!=(
    expected<void, _E1> const& _lhs,
    expected<void, _E2> const& _rhs
)
{
    return !(_lhs == _rhs);
}

template<typename _T,
         typename _E,
         typename _U>
D_CONSTEXPR_CPP20 bool
operator!=(
    expected<_T, _E> const& _lhs,
    _U const&                _rhs
)
{
    return !(_lhs == _rhs);
}

template<typename _T,
         typename _E,
         typename _G>
D_CONSTEXPR_CPP20 bool
operator!=(
    expected<_T, _E> const&    _lhs,
    unexpected<_G> const&      _rhs
)
{
    return !(_lhs == _rhs);
}

template<typename _E,
         typename _G>
D_CONSTEXPR_CPP20 bool
operator!=(
    expected<void, _E> const&  _lhs,
    unexpected<_G> const&      _rhs
)
{
    return !(_lhs == _rhs);
}

#endif  // !C++20


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_EXPECTED_COMPARE_
