/******************************************************************************
* djinterp [restd]                                            array_compare.hpp
*
* array comparison operators header:
*   Provides the six legacy relational operators for array<_Type, _Size>:
*   operator== (element-wise equality), operator!= (defined as !(==)),
*   operator< (lexicographic less-than), and operator<=, operator>,
*   operator>= (defined via the canonical reflection through op<).
*
*   In C++20, operator!= and the three ordering operators are
* synthesised from operator<=> per [array.syn] — std no longer ships
* explicit overloads. restd ships explicit overloads on every tier
* for portability: on C++11–C++17 they are the only way to compare
* arrays; on C++20+ they coexist with the three-way overload (in
* array_compare_three_way.hpp).
*
*   CONSTEXPR:
*   - C++98/03: not constexpr (no constexpr keyword).
*   - C++11:    not constexpr (loops disallowed in constexpr function
*     bodies; would need recursion which is awkward across all six).
*   - C++14+:   constexpr — restd is ahead of std (std waited for
*     C++20 / P1614). The relaxed-constexpr rules from C++14 permit
*     for-loops directly.
*
*   ELEMENT-TYPE REQUIREMENTS:
*   _Type must be EqualityComparable for op== / op!=, and
* LessThanComparable for op< and friends. Mismatches produce
* compile errors at the instantiation site of the relevant operator.
* No SFINAE constraint — matches std and avoids dragging in the
* has_op_eq / has_op_lt trait infrastructure.
*
*
* path:      /inc/djinterp/re_std/array/array_compare.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.19
******************************************************************************/

#ifndef DJINTERP_RESTD_ARRAY_COMPARE_
#define DJINTERP_RESTD_ARRAY_COMPARE_ 1

#include <cstddef>

#include "../../core/djinterp.hpp"
#include "./array.hpp"


#ifndef D_CONSTEXPR_CPP14
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        #define D_CONSTEXPR_CPP14   constexpr
    #else
        #define D_CONSTEXPR_CPP14
    #endif
#endif


NS_RESTD


// ===========================================================================
// I.   OPERATOR== / OPERATOR!=
// ===========================================================================

// operator==
//   function: true if every pair of corresponding elements compares
// equal. Zero-size arrays always compare equal.
template<typename    _Type,
         std::size_t _Size>
D_CONSTEXPR_CPP14 bool
operator==(
    array<_Type, _Size> const& _lhs,
    array<_Type, _Size> const& _rhs
)
{
    for (std::size_t _i = 0; _i < _Size; ++_i)
    {
        if (!(_lhs[_i] == _rhs[_i]))
        {
            return false;
        }
    }

    return true;
}

// operator!=
//   function: defined as !(_lhs == _rhs). On C++20 the standard
// synthesises this from op<=>; restd keeps it explicit so user
// code compiled at C++11–C++17 can still rely on it.
template<typename    _Type,
         std::size_t _Size>
D_CONSTEXPR_CPP14 bool
operator!=(
    array<_Type, _Size> const& _lhs,
    array<_Type, _Size> const& _rhs
)
{
    return !(_lhs == _rhs);
}


// ===========================================================================
// II.  OPERATOR< / OPERATOR<= / OPERATOR> / OPERATOR>=
// ===========================================================================

// operator<
//   function: lexicographic less-than. Element-wise comparison
// using operator<; returns the result of the first mismatched pair.
// note: zero-size arrays compare equal, so op< returns false.
template<typename    _Type,
         std::size_t _Size>
D_CONSTEXPR_CPP14 bool
operator<(
    array<_Type, _Size> const& _lhs,
    array<_Type, _Size> const& _rhs
)
{
    for (std::size_t _i = 0; _i < _Size; ++_i)
    {
        if (_lhs[_i] < _rhs[_i])  return true;
        if (_rhs[_i] < _lhs[_i])  return false;
    }

    return false;
}

// operator<=
//   function: defined as !(_rhs < _lhs).
template<typename    _Type,
         std::size_t _Size>
D_CONSTEXPR_CPP14 bool
operator<=(
    array<_Type, _Size> const& _lhs,
    array<_Type, _Size> const& _rhs
)
{
    return !(_rhs < _lhs);
}

// operator>
//   function: defined as _rhs < _lhs.
template<typename    _Type,
         std::size_t _Size>
D_CONSTEXPR_CPP14 bool
operator>(
    array<_Type, _Size> const& _lhs,
    array<_Type, _Size> const& _rhs
)
{
    return _rhs < _lhs;
}

// operator>=
//   function: defined as !(_lhs < _rhs).
template<typename    _Type,
         std::size_t _Size>
D_CONSTEXPR_CPP14 bool
operator>=(
    array<_Type, _Size> const& _lhs,
    array<_Type, _Size> const& _rhs
)
{
    return !(_lhs < _rhs);
}


NS_END  // restd


#endif  // DJINTERP_RESTD_ARRAY_COMPARE_
