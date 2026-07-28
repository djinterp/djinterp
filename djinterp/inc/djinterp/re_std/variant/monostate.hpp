/******************************************************************************
* djinterp [restd]                                                  monostate.hpp
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
* path:      /inc/djinterp/restd/variant/monostate.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.05.20
******************************************************************************/

#ifndef DJINTERP_RESTD_MONOSTATE_
#define DJINTERP_RESTD_MONOSTATE_ 1

#include "../../core/djinterp.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER


NS_RESTD


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

D_CONSTEXPR inline bool operator==(monostate, monostate) D_NOEXCEPT { return true;  }
D_CONSTEXPR inline bool operator!=(monostate, monostate) D_NOEXCEPT { return false; }
D_CONSTEXPR inline bool operator< (monostate, monostate) D_NOEXCEPT { return false; }
D_CONSTEXPR inline bool operator> (monostate, monostate) D_NOEXCEPT { return false; }
D_CONSTEXPR inline bool operator<=(monostate, monostate) D_NOEXCEPT { return true;  }
D_CONSTEXPR inline bool operator>=(monostate, monostate) D_NOEXCEPT { return true;  }


NS_END  // restd


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RESTD_MONOSTATE_
