/******************************************************************************
* re_std [variant]                                       variant_three_way.hpp
*
*   operator<=> for variant.
*
*   THE ORDERING IS INDEX-FIRST, THEN VALUE, and the valueless state sorts
* BELOW everything. Spelled out, because the order of the checks is the whole
* implementation:
*
*     both valueless          -> equal
*     only left valueless     -> less
*     only right valueless    -> greater
*     indices differ          -> compare the INDICES
*     indices equal           -> compare the held alternatives
*
*   The valueless checks must come first: a valueless variant's index is
* variant_npos, so comparing indices would make it sort ABOVE every real
* alternative rather than below.
*
*   COMPARING THE ALTERNATIVES NEEDS THE INDEX, NOT THE TYPE, which is why
* this dispatches on a linear index chain rather than through visit. With
* duplicate alternative types - variant<int, int> - knowing the type tells you
* nothing about which alternative to read from the right-hand operand.
*
*   STD IS C++20; re_std IS C++20 - hard ceiling, operator<=> is a core
* language feature.
*
* path:      /inc/djinterp/re_std/variant/variant_three_way.hpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.13
******************************************************************************/

#ifndef RESTD_VARIANT_THREE_WAY_
#define RESTD_VARIANT_THREE_WAY_ 1

#include "../../djinterp.hpp"

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../compare/compare.hpp"
#include "./variant.hpp"

NS_DJINTERP
NS_RESTD
NS_INTERNAL

    template<typename _Category, size_t _Index, size_t _Size>
    struct variant_cmp_dispatch
    {
        template<typename _Variant>
        static _Category apply(const _Variant& a, const _Variant& b,
                               size_t index)
        {
            if (index == _Index)
            {
                return static_cast<_Category>(
                    re_std::get<_Index>(a) <=> re_std::get<_Index>(b));
            }
            return variant_cmp_dispatch<_Category, _Index + 1, _Size>::apply(
                a, b, index);
        }
    };

    template<typename _Category, size_t _Size>
    struct variant_cmp_dispatch<_Category, _Size, _Size>
    {
        template<typename _Variant>
        static _Category apply(const _Variant&, const _Variant&, size_t)
        {
            //   Unreachable: valueless is handled before dispatch.
            return static_cast<_Category>(strong_ordering::equal);
        }
    };

NS_END  // internal

// operator<=>
//   function: index-first ordering with valueless sorting below everything.
template<typename... _Types>
D_CONSTEXPR typename common_comparison_category<
    typename compare_three_way_result<_Types, _Types>::type...>::type
operator<=>(const variant<_Types...>& a, const variant<_Types...>& b)
{
    typedef typename common_comparison_category<
        typename compare_three_way_result<_Types, _Types>::type...>::type
        _Category;

    //   Valueless first - its index is variant_npos and would otherwise sort
    // above every real alternative.
    if (a.valueless_by_exception() && b.valueless_by_exception())
    { return static_cast<_Category>(strong_ordering::equal); }
    if (a.valueless_by_exception())
    { return static_cast<_Category>(strong_ordering::less); }
    if (b.valueless_by_exception())
    { return static_cast<_Category>(strong_ordering::greater); }

    if (a.index() != b.index())
    { return static_cast<_Category>(a.index() <=> b.index()); }

    return internal::variant_cmp_dispatch<
        _Category, 0, sizeof...(_Types)>::apply(a, b, a.index());
}

NS_END
NS_END

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // RESTD_VARIANT_THREE_WAY_
