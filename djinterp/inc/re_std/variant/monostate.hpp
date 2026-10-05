/*******************************************************************************
* djinterp [re_std]                                                monostate.hpp
*
* monostate header:
*   Empty trivial unit type. Used as the first alternative in a
* variant when the natural first alternative isn't default-
* constructible. variant<monostate, expensive_t> is default-
* constructible to the monostate state; variant<expensive_t>
* would not be unless expensive_t had a default ctor.
*
*   All instances compare equal; ordering is reflexive (always
* false for < and > between two monostates, true for == and >=
* and <=).
*
*
* path:      /inc/re_std/variant/monostate.hpp
* link(s):   TBA
* author(s): TBA                                             created: 2026.05.20
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef RE_STD_VARIANT_MONOSTATE_HPP
#define RE_STD_VARIANT_MONOSTATE_HPP 1

// re_std
#include "../config.hpp"  // RE_STD_* configuration

#if RE_STD_LANG_IS_CPP11_OR_HIGHER


namespace re_std
{


// ===========================================================================
// I.   MONOSTATE
// ===========================================================================

// monostate
//   class: empty type. Trivially default-constructible, trivially
// copyable, trivially destructible.
struct monostate {};


// ===========================================================================
// II.  COMPARISON OPERATORS
// ===========================================================================
// All monostates compare equal. Ordering relations are total but
// trivial.

RE_STD_CONSTEXPR inline bool operator==(monostate, monostate) RE_STD_NOEXCEPT { return true;  }
RE_STD_CONSTEXPR inline bool operator!=(monostate, monostate) RE_STD_NOEXCEPT { return false; }
RE_STD_CONSTEXPR inline bool operator< (monostate, monostate) RE_STD_NOEXCEPT { return false; }
RE_STD_CONSTEXPR inline bool operator> (monostate, monostate) RE_STD_NOEXCEPT { return false; }
RE_STD_CONSTEXPR inline bool operator<=(monostate, monostate) RE_STD_NOEXCEPT { return true;  }
RE_STD_CONSTEXPR inline bool operator>=(monostate, monostate) RE_STD_NOEXCEPT { return true;  }


}  // re_std


#endif  // RE_STD_LANG_IS_CPP11_OR_HIGHER


#endif  // RE_STD_VARIANT_MONOSTATE_HPP
