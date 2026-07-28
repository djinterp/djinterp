/******************************************************************************
* djinterp [restd]                                                variant_npos.hpp
*
* variant_npos sentinel header:
*   Constant returned by variant<Ts...>::index() when the variant
* is in the valueless-by-exception state (entered when an
* alternative's assignment throws and the variant cannot recover).
*
*   Value: static_cast<size_t>(-1).
*
*
* path:      /inc/djinterp/restd/variant/variant_npos.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_RESTD_VARIANT_NPOS_
#define DJINTERP_RESTD_VARIANT_NPOS_ 1

#include <cstddef>
#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_RESTD


// ===========================================================================
// I.   VARIANT_NPOS
// ===========================================================================

D_CONSTEXPR std::size_t variant_npos = static_cast<std::size_t>(-1);


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_VARIANT_NPOS_
