/*******************************************************************************
* djinterp [re_std]                                             variant_npos.hpp
*
* variant_npos sentinel header:
*   Constant returned by variant<Ts...>::index() when the variant
* is in the valueless-by-exception state (entered when an
* alternative's assignment throws and the variant cannot recover).
*
*   Value: static_cast<size_t>(-1).
*
*
* path:      /inc/re_std/variant/variant_npos.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.20
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_VARIANT_VARIANT_NPOS_HPP
#define RE_STD_VARIANT_VARIANT_NPOS_HPP 1

// std
#include <cstddef>
// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER


namespace re_std
{


// ===========================================================================
// I.   VARIANT_NPOS
// ===========================================================================

RE_STD_CONSTEXPR std::size_t variant_npos = static_cast<std::size_t>(-1);


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_VARIANT_VARIANT_NPOS_HPP
