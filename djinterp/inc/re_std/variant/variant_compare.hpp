/******************************************************************************
* djinterp [re_std]                                             variant_compare.hpp
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
* path:      /inc/djinterp/re_std/variant/variant_compare.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_RE_STD_VARIANT_COMPARE_
#define DJINTERP_RE_STD_VARIANT_COMPARE_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "./variant.hpp"
#include "./variant_npos.hpp"


NS_RESTD


// ===========================================================================
// 0.   INTERNAL — index-dispatched element-wise op== and op<
// ===========================================================================

NS_INTERNAL

    template<std::size_t _I, std::size_t _N>
    struct compare_at_impl
    {
        template<typename _Variant>
        static bool eq(_Variant const& _a, _Variant const& _b)
        {
            if (_a.index() == _I)
            {
                return _a.template _ref<_I>() == _b.template _ref<_I>();
            }
            return compare_at_impl<_I + 1, _N>::eq(_a, _b);
        }

        template<typename _Variant>
        static bool lt(_Variant const& _a, _Variant const& _b)
        {
            if (_a.index() == _I)
            {
                return _a.template _ref<_I>() < _b.template _ref<_I>();
            }
            return compare_at_impl<_I + 1, _N>::lt(_a, _b);
        }
    };

    template<std::size_t _N>
    struct compare_at_impl<_N, _N>
    {
        template<typename _Variant>
        static bool eq(_Variant const&, _Variant const&) { return false; }
        template<typename _Variant>
        static bool lt(_Variant const&, _Variant const&) { return false; }
    };

NS_END  // internal


// ===========================================================================
// I.   OPERATOR==
// ===========================================================================

template<typename... _Types>
bool
operator==(
    variant<_Types...> const& _lhs,
    variant<_Types...> const& _rhs
)
{
    if (_lhs.index() != _rhs.index())   return false;
    if (_lhs.valueless_by_exception())  return true;  // both valueless
    return internal::compare_at_impl<0, sizeof...(_Types)>::eq(_lhs, _rhs);
}


// ===========================================================================
// II.  OPERATOR<
// ===========================================================================

template<typename... _Types>
bool
operator<(
    variant<_Types...> const& _lhs,
    variant<_Types...> const& _rhs
)
{
    // valueless rules per [variant.relops]: valueless < non-valueless.
    if (_rhs.valueless_by_exception())  return false;
    if (_lhs.valueless_by_exception())  return true;
    if (_lhs.index() != _rhs.index())   return _lhs.index() < _rhs.index();
    return internal::compare_at_impl<0, sizeof...(_Types)>::lt(_lhs, _rhs);
}


// ===========================================================================
// III. REFLECTED OPERATORS
// ===========================================================================

#if !D_ENV_LANG_IS_CPP20_OR_HIGHER

template<typename... _Types>
bool
operator!=(
    variant<_Types...> const& _lhs,
    variant<_Types...> const& _rhs
)
{
    return !(_lhs == _rhs);
}

#endif  // !C++20

template<typename... _Types>
bool
operator<=(
    variant<_Types...> const& _lhs,
    variant<_Types...> const& _rhs
)
{
    return !(_rhs < _lhs);
}

template<typename... _Types>
bool
operator>(
    variant<_Types...> const& _lhs,
    variant<_Types...> const& _rhs
)
{
    return _rhs < _lhs;
}

template<typename... _Types>
bool
operator>=(
    variant<_Types...> const& _lhs,
    variant<_Types...> const& _rhs
)
{
    return !(_lhs < _rhs);
}


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_VARIANT_COMPARE_
