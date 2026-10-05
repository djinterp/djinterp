/*******************************************************************************
* djinterp [re_std]                                      string_view_compare.hpp
*
* string_view comparison operators header:
*   The non-member relational operators for basic_string_view. Each is
* expressed in terms of the member compare(). To allow one operand to
* be something convertible to a view (e.g. a const CharT*), exactly
* one parameter of every overload is a non-deduced context via the
* internal sv_identity alias — mirroring the standard's use of
* type_identity_t.
*
*   PORTABILITY:
*   On C++11 through C++17 all six legacy operators (==, !=, <, <=, >,
* >=) are provided (std shipped them in C++17; re_std back-ports to
* C++11). On C++20+ only operator== and operator<=> are defined; the
* other four are synthesised by the compiler from those two, matching
* the standard's C++20 rewrite. operator<=> yields strong_ordering
* directly (the C++20 Traits::comparison_category hook is not
* consulted — see the basic_string_view notes).
*
*
* path:      /inc/re_std/string_view/string_view_compare.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.04
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_STRING_VIEW_STRING_VIEW_COMPARE_HPP
#define RE_STD_STRING_VIEW_STRING_VIEW_COMPARE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration


#if RE_STD_LANG_IS_CPP11_OR_HIGHER


// re_std
#include "./basic_string_view.hpp"

#if RE_STD_LANG_IS_CPP20_OR_HIGHER
    // std
    #include <compare>
#endif


namespace re_std
{


// =============================================================================
// I.   INTERNAL: NON-DEDUCED OPERAND
// =============================================================================

namespace internal
{

    // sv_identity
    //   trait: forces its template argument into a non-deduced context,
    // so the matching operand of a comparison operator is converted to
    // a view rather than deduced. Equivalent to type_identity for this
    // purpose, kept local to avoid a <type_traits> dependency.
    template<typename Type>
    struct sv_identity
    {
        typedef Type  type;
    };

}  // internal


// =============================================================================
// II.  EQUALITY  (all tiers)
// =============================================================================

template<typename CharT,
         typename Traits>
RE_STD_CONSTEXPR_CPP14 bool
operator==(
    basic_string_view<CharT, Traits>                                _lhs,
    typename internal::sv_identity<
        basic_string_view<CharT, Traits> >::type                    _rhs
) RE_STD_NOEXCEPT
{
    return _lhs.size() == _rhs.size() && _lhs.compare(_rhs) == 0;
}


#if RE_STD_LANG_IS_CPP20_OR_HIGHER

// =============================================================================
// III. THREE-WAY  (C++20+)
// =============================================================================

template<typename CharT,
         typename Traits>
constexpr std::strong_ordering
operator<=>(
    basic_string_view<CharT, Traits>                                _lhs,
    typename internal::sv_identity<
        basic_string_view<CharT, Traits> >::type                    _rhs
) noexcept
{
    return _lhs.compare(_rhs) <=> 0;
}

#else

// =============================================================================
// III. LEGACY RELATIONAL  (C++11 — C++17)
// =============================================================================

template<typename CharT,
         typename Traits>
RE_STD_CONSTEXPR_CPP14 bool
operator!=(
    basic_string_view<CharT, Traits>                                _lhs,
    typename internal::sv_identity<
        basic_string_view<CharT, Traits> >::type                    _rhs
) RE_STD_NOEXCEPT
{
    return !(_lhs == _rhs);
}

template<typename CharT,
         typename Traits>
RE_STD_CONSTEXPR_CPP14 bool
operator<(
    basic_string_view<CharT, Traits>                                _lhs,
    typename internal::sv_identity<
        basic_string_view<CharT, Traits> >::type                    _rhs
) RE_STD_NOEXCEPT
{
    return _lhs.compare(_rhs) < 0;
}

template<typename CharT,
         typename Traits>
RE_STD_CONSTEXPR_CPP14 bool
operator>(
    basic_string_view<CharT, Traits>                                _lhs,
    typename internal::sv_identity<
        basic_string_view<CharT, Traits> >::type                    _rhs
) RE_STD_NOEXCEPT
{
    return _lhs.compare(_rhs) > 0;
}

template<typename CharT,
         typename Traits>
RE_STD_CONSTEXPR_CPP14 bool
operator<=(
    basic_string_view<CharT, Traits>                                _lhs,
    typename internal::sv_identity<
        basic_string_view<CharT, Traits> >::type                    _rhs
) RE_STD_NOEXCEPT
{
    return _lhs.compare(_rhs) <= 0;
}

template<typename CharT,
         typename Traits>
RE_STD_CONSTEXPR_CPP14 bool
operator>=(
    basic_string_view<CharT, Traits>                                _lhs,
    typename internal::sv_identity<
        basic_string_view<CharT, Traits> >::type                    _rhs
) RE_STD_NOEXCEPT
{
    return _lhs.compare(_rhs) >= 0;
}

#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_STRING_VIEW_STRING_VIEW_COMPARE_HPP
