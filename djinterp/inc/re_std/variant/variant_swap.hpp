/*******************************************************************************
* djinterp [re_std]                                             variant_swap.hpp
*
* variant swap header:
*   ADL-friendly non-member swap delegating to the member swap.
*
*
* path:      /inc/re_std/variant/variant_swap.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.20
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_VARIANT_VARIANT_SWAP_HPP
#define RE_STD_VARIANT_VARIANT_SWAP_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER

#include "./variant.hpp"


namespace re_std
{


template<typename... Types>
void
swap(
    variant<Types...>& _lhs,
    variant<Types...>& _rhs
)
{
    _lhs.swap(_rhs);

    return;
}


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_VARIANT_VARIANT_SWAP_HPP
