/*******************************************************************************
* djinterp [re_std]                                  array_compare_three_way.hpp
*
* array three-way comparison header:
*   Provides operator<=> for re_std::array. C++20-only — the spaceship
* operator is a language feature with no back-port. The result type
* is the element-wise three-way comparison result of Type with
* itself (typically std::strong_ordering for arithmetic types).
*
*   IMPLEMENTATION:
*   Lexicographic three-way comparison. Iterates element-wise via
* an if-constexpr-driven recursive helper (internal::array_3way_impl);
* the identity for the empty case is strong_ordering::equal cast to
* the result type, matching re_std::tuple's three-way overload.
*
*   ELEMENT-TYPE REQUIREMENT:
*   Type must satisfy three_way_comparable — i.e. it must have its
* own operator<=>. The C++20 standard specifies a synth-three-way
* fallback (compose <=> from < and == when <=> is absent); re_std
* deliberately omits it here, matching the same back-port
* simplification made in tuple/tuple_compare_three_way.hpp. Element
* types without operator<=> produce a compile error at the decltype
* site, not a runtime mis-comparison.
*
*   Uses:
*     array.hpp        - the array class
*     <compare>        - common_comparison_category_t, strong_ordering
*
*
* path:      /inc/re_std/array/array_compare_three_way.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.19
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_ARRAY_ARRAY_COMPARE_THREE_WAY_HPP
#define RE_STD_ARRAY_ARRAY_COMPARE_THREE_WAY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

// gate: C++20 spaceship operator is a language feature
#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// std
#include <compare>
#include <cstddef>

#include "./array.hpp"


namespace re_std
{


namespace internal
{

    // array_3way_result
    //   alias: element-wise three-way comparison result type for
    // two same-type, same-extent arrays. Equivalent to
    // compare_three_way_result_t<Type> for the homogeneous case.
    template<typename Type>
    using array_3way_result = decltype(
        std::declval<Type const&>() <=> std::declval<Type const&>());

}  // internal


// ===========================================================================
// I.   OPERATOR<=>
// ===========================================================================

// operator<=>
//   function: lexicographic three-way comparison. Returns the
// first non-equal element's <=> result; returns strong_ordering::
// equal (cast to the common result type) when all elements compare
// equal or Size is 0.
template<typename    Type,
         std::size_t Size>
constexpr internal::array_3way_result<Type>
operator<=>(
    array<Type, Size> const& _lhs,
    array<Type, Size> const& _rhs
)
{
    typedef internal::array_3way_result<Type> _result_t;

    for (std::size_t _i = 0; _i < Size; ++_i)
    {
        _result_t _cmp = _lhs[_i] <=> _rhs[_i];
        if (_cmp != 0)
        {
            return _cmp;
        }
    }

    return static_cast<_result_t>(std::strong_ordering::equal);
}


}  // re_std


#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER


#endif  // RE_STD_ARRAY_ARRAY_COMPARE_THREE_WAY_HPP
