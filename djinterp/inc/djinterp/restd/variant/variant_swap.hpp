/******************************************************************************
* djinterp [restd]                                               variant_swap.hpp
*
* variant swap header:
*   ADL-friendly non-member swap delegating to the member swap.
*
*
* path:      /inc/djinterp/restd/variant/variant_swap.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_RESTD_VARIANT_SWAP_
#define DJINTERP_RESTD_VARIANT_SWAP_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

#include "./variant.hpp"


NS_RESTD


template<typename... _Types>
void
swap(
    variant<_Types...>& _lhs,
    variant<_Types...>& _rhs
)
{
    _lhs.swap(_rhs);

    return;
}


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_VARIANT_SWAP_
