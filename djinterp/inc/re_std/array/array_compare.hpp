/*******************************************************************************
* djinterp [re_std]                                            array_compare.hpp
*
* array comparison operators header:
*   Provides the six legacy relational operators for array<Type, Size>:
*   operator== (element-wise equality), operator!= (defined as !(==)),
*   operator< (lexicographic less-than), and operator<=, operator>,
*   operator>= (defined via the canonical reflection through op<).
*
*   In C++20, operator!= and the three ordering operators are
* synthesised from operator<=> per [array.syn] — std no longer ships
* explicit overloads. re_std ships explicit overloads on every tier
* for portability: on C++11–C++17 they are the only way to compare
* arrays; on C++20+ they coexist with the three-way overload (in
* array_compare_three_way.hpp).
*
*   CONSTEXPR:
*   - C++98/03: not constexpr (no constexpr keyword).
*   - C++11:    not constexpr (loops disallowed in constexpr function
*     bodies; would need recursion which is awkward across all six).
*   - C++14+:   constexpr — re_std is ahead of std (std waited for
*     C++20 / P1614). The relaxed-constexpr rules from C++14 permit
*     for-loops directly.
*
*   ELEMENT-TYPE REQUIREMENTS:
*   Type must be EqualityComparable for op== / op!=, and
* LessThanComparable for op< and friends. Mismatches produce
* compile errors at the instantiation site of the relevant operator.
* No SFINAE constraint — matches std and avoids dragging in the
* has_op_eq / has_op_lt trait infrastructure.
*
*
* path:      /inc/re_std/array/array_compare.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.19
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_ARRAY_ARRAY_COMPARE_HPP
#define RE_STD_ARRAY_ARRAY_COMPARE_HPP 1

// FLOOR, FOR NOW: below C++11 this header is empty rather than an error
// (README rule 5; re_std omits rather than degrades). The owner's ruling:
// compile at every level first; port to C++98 only where something needs it.
#include "../config.hpp"  // RE_STD_* configuration
#if RE_STD_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
// re_std
#include "./array.hpp"


namespace re_std
{


// ===========================================================================
// I.   OPERATOR== / OPERATOR!=
// ===========================================================================

// operator==
//   function: true if every pair of corresponding elements compares
// equal. Zero-size arrays always compare equal.
template<typename    Type,
         std::size_t Size>
RE_STD_CONSTEXPR_CPP14 bool
operator==(
    array<Type, Size> const& _lhs,
    array<Type, Size> const& _rhs
)
{
    for (std::size_t _i = 0; _i < Size; ++_i)
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
// synthesises this from op<=>; re_std keeps it explicit so user
// code compiled at C++11–C++17 can still rely on it.
template<typename    Type,
         std::size_t Size>
RE_STD_CONSTEXPR_CPP14 bool
operator!=(
    array<Type, Size> const& _lhs,
    array<Type, Size> const& _rhs
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
template<typename    Type,
         std::size_t Size>
RE_STD_CONSTEXPR_CPP14 bool
operator<(
    array<Type, Size> const& _lhs,
    array<Type, Size> const& _rhs
)
{
    for (std::size_t _i = 0; _i < Size; ++_i)
    {
        if (_lhs[_i] < _rhs[_i])  return true;
        if (_rhs[_i] < _lhs[_i])  return false;
    }

    return false;
}

// operator<=
//   function: defined as !(_rhs < _lhs).
template<typename    Type,
         std::size_t Size>
RE_STD_CONSTEXPR_CPP14 bool
operator<=(
    array<Type, Size> const& _lhs,
    array<Type, Size> const& _rhs
)
{
    return !(_rhs < _lhs);
}

// operator>
//   function: defined as _rhs < _lhs.
template<typename    Type,
         std::size_t Size>
RE_STD_CONSTEXPR_CPP14 bool
operator>(
    array<Type, Size> const& _lhs,
    array<Type, Size> const& _rhs
)
{
    return _rhs < _lhs;
}

// operator>=
//   function: defined as !(_lhs < _rhs).
template<typename    Type,
         std::size_t Size>
RE_STD_CONSTEXPR_CPP14 bool
operator>=(
    array<Type, Size> const& _lhs,
    array<Type, Size> const& _rhs
)
{
    return !(_lhs < _rhs);
}


}  // re_std

#endif  // floor, for now


#endif  // RE_STD_ARRAY_ARRAY_COMPARE_HPP
