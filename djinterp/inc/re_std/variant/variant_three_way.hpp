/*******************************************************************************
* djinterp [re_std]                                        variant_three_way.hpp
*
* variant_three_way support header:
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
*
* path:      /inc/re_std/variant/variant_three_way.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef RE_STD_VARIANT_VARIANT_THREE_WAY_HPP
#define RE_STD_VARIANT_VARIANT_THREE_WAY_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP20_OR_HIGHER

#include "../type_traits/type_traits.hpp"
#include "../compare/compare"
#include "./variant.hpp"
#include "./variant_get.hpp"

namespace re_std
{
namespace internal
{

    template<typename Category, size_t Index, size_t Size>
    struct variant_cmp_dispatch
    {
        template<typename Variant>
        static Category apply(const Variant& a, const Variant& b,
                               size_t index)
        {
            if (index == Index)
            {
                return static_cast<Category>(
                    re_std::get<Index>(a) <=> re_std::get<Index>(b));
            }
            return variant_cmp_dispatch<Category, Index + 1, Size>::apply(
                a, b, index);
        }
    };

    template<typename Category, size_t Size>
    struct variant_cmp_dispatch<Category, Size, Size>
    {
        template<typename Variant>
        static Category apply(const Variant&, const Variant&, size_t)
        {
            //   Unreachable: valueless is handled before dispatch.
            return static_cast<Category>(strong_ordering::equal);
        }
    };

}  // internal

// operator<=>
//   function: index-first ordering with valueless sorting below everything.
template<typename... Types>
RE_STD_CONSTEXPR typename common_comparison_category<
    typename compare_three_way_result<Types, Types>::type...>::type
operator<=>(const variant<Types...>& a, const variant<Types...>& b)
{
    typedef typename common_comparison_category<
        typename compare_three_way_result<Types, Types>::type...>::type
        Category;

    //   Valueless first - its index is variant_npos and would otherwise sort
    // above every real alternative.
    if (a.valueless_by_exception() && b.valueless_by_exception())
    { return static_cast<Category>(strong_ordering::equal); }
    if (a.valueless_by_exception())
    { return static_cast<Category>(strong_ordering::less); }
    if (b.valueless_by_exception())
    { return static_cast<Category>(strong_ordering::greater); }

    if (a.index() != b.index())
    { return static_cast<Category>(a.index() <=> b.index()); }

    return internal::variant_cmp_dispatch<
        Category, 0, sizeof...(Types)>::apply(a, b, a.index());
}

}

#endif  // RE_STD_LANG_IS_CPP20_OR_HIGHER

#endif  // RE_STD_VARIANT_VARIANT_THREE_WAY_HPP
