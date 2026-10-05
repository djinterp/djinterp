/*******************************************************************************
* djinterp [re_std]                                          variant_compare.hpp
*
* variant comparison operators header:
*   The six legacy relational operators for two same-type variants
* (==, !=, <, <=, >, >=).
*
*   SEMANTICS (per [variant.relops]):
*     - If lhs.index() != rhs.index(), op<  -> lhs.index() < rhs.index().
*                                      op== -> false.
*     - If both valueless,             op== -> true; op< -> false.
*     - If one valueless,              op== -> false.
*                                      op<  -> rhs is non-valueless
*                                              (valueless < everything).
*     - Else (same index, both valued): defer to the held alternative's
*                                      own op== / op<.
*
*   Operator!= is synthesised from op== by C++20; re_std ships it
* explicitly on every tier and gates it out on C++20+ to avoid
* ambiguity with the compiler-synthesised version.
*
*
* path:      /inc/re_std/variant/variant_compare.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.20
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_VARIANT_VARIANT_COMPARE_HPP
#define RE_STD_VARIANT_VARIANT_COMPARE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "./variant.hpp"
#include "./variant_npos.hpp"


namespace re_std
{


// ===========================================================================
// 0.   INTERNAL — index-dispatched element-wise op== and op<
// ===========================================================================

namespace internal
{

    template<std::size_t I, std::size_t N>
    struct compare_at_impl
    {
        template<typename Variant>
        static bool eq(Variant const& _a, Variant const& _b)
        {
            if (_a.index() == I)
            {
                return _a.template _ref<I>() == _b.template _ref<I>();
            }
            return compare_at_impl<I + 1, N>::eq(_a, _b);
        }

        template<typename Variant>
        static bool lt(Variant const& _a, Variant const& _b)
        {
            if (_a.index() == I)
            {
                return _a.template _ref<I>() < _b.template _ref<I>();
            }
            return compare_at_impl<I + 1, N>::lt(_a, _b);
        }
    };

    template<std::size_t N>
    struct compare_at_impl<N, N>
    {
        template<typename Variant>
        static bool eq(Variant const&, Variant const&) { return false; }
        template<typename Variant>
        static bool lt(Variant const&, Variant const&) { return false; }
    };

}  // internal


// ===========================================================================
// I.   OPERATOR==
// ===========================================================================

template<typename... Types>
bool
operator==(
    variant<Types...> const& _lhs,
    variant<Types...> const& _rhs
)
{
    if (_lhs.index() != _rhs.index())   return false;
    if (_lhs.valueless_by_exception())  return true;  // both valueless
    return internal::compare_at_impl<0, sizeof...(Types)>::eq(_lhs, _rhs);
}


// ===========================================================================
// II.  OPERATOR<
// ===========================================================================

template<typename... Types>
bool
operator<(
    variant<Types...> const& _lhs,
    variant<Types...> const& _rhs
)
{
    // valueless rules per [variant.relops]: valueless < non-valueless.
    if (_rhs.valueless_by_exception())  return false;
    if (_lhs.valueless_by_exception())  return true;
    if (_lhs.index() != _rhs.index())   return _lhs.index() < _rhs.index();
    return internal::compare_at_impl<0, sizeof...(Types)>::lt(_lhs, _rhs);
}


// ===========================================================================
// III. REFLECTED OPERATORS
// ===========================================================================

#if !RE_STD_LANG_IS_CPP20_OR_HIGHER

template<typename... Types>
bool
operator!=(
    variant<Types...> const& _lhs,
    variant<Types...> const& _rhs
)
{
    return !(_lhs == _rhs);
}

#endif  // !C++20

template<typename... Types>
bool
operator<=(
    variant<Types...> const& _lhs,
    variant<Types...> const& _rhs
)
{
    return !(_rhs < _lhs);
}

template<typename... Types>
bool
operator>(
    variant<Types...> const& _lhs,
    variant<Types...> const& _rhs
)
{
    return _rhs < _lhs;
}

template<typename... Types>
bool
operator>=(
    variant<Types...> const& _lhs,
    variant<Types...> const& _rhs
)
{
    return !(_lhs < _rhs);
}


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_VARIANT_VARIANT_COMPARE_HPP
