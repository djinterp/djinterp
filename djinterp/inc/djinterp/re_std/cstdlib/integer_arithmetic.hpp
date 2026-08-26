/******************************************************************************
* djinterp [re_std]                                       integer_arithmetic.hpp
*
* the integer abs and div families (re-exports):
*   abs / labs / llabs, div / ldiv / lldiv, and the div_t / ldiv_t /
* lldiv_t result structs.
*
*   WHY RE-EXPORT SOMETHING THIS SIMPLE:
*   abs is four characters of arithmetic, so reimplementing it looks
* tempting. It would be a mistake for one reason: OVERLOAD IDENTITY. The
* set of std::abs overloads visible at a call site decides which one
* wins, and a separate re_std::abs would create a second, differently
* populated set. Code that included both headers would get different
* answers depending on include order -- the classic abs(-3.5) yielding 3
* bug, reintroduced by the library that was meant to prevent it.
*
*   WHICH abs YOU GET DEPENDS ON WHAT ELSE IS INCLUDED:
*   <cstdlib> declares the integer overloads. The floating-point ones
* live in <cmath>, which re_std has not implemented (priority 118). A
* using-declaration captures whatever std::abs overloads are visible when
* this header is parsed, so a translation unit that included <cmath>
* first gets the floating-point ones too. That is the standard's own
* behaviour rather than something re_std introduces, but it means
* re_std::abs on a double is not portable and should not be relied on
* until <cmath> lands.
*
*   THE ONE VALUE THAT HAS NO ABSOLUTE VALUE:
*   abs(INT_MIN) is undefined behaviour on a two's complement machine --
* the magnitude is not representable in the same type. This is not a
* corner case in practice: it is how overflow reaches sanitiser builds
* from timestamp and hash code. re_std::ratio rejects INTMAX_MIN at
* definition time for exactly this reason.
*
*   div RETURNS BOTH RESULTS AT ONCE, and its truncation direction is
* guaranteed toward zero -- which since C++11 is also what / and % do, so
* the family's original reason for existing has largely lapsed.
*
*
* path:      /inc/djinterp/re_std/cstdlib/integer_arithmetic.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_CSTDLIB_INTEGER_ARITHMETIC_
#define DJINTERP_RE_STD_CSTDLIB_INTEGER_ARITHMETIC_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstdlib>


NS_RESTD

    // div_t / ldiv_t / lldiv_t
    //   struct: quotient and remainder pairs returned by the div family.
    using ::std::div_t;
    using ::std::ldiv_t;
    using ::std::lldiv_t;

    // abs / labs / llabs
    //   function: magnitude. Undefined for the most-negative value of the
    // type -- see the header comment.
    using ::std::abs;
    using ::std::labs;
    using ::std::llabs;

    // div / ldiv / lldiv
    //   function: quotient and remainder in one call, truncating toward
    // zero.
    using ::std::div;
    using ::std::ldiv;
    using ::std::lldiv;

NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_CSTDLIB_INTEGER_ARITHMETIC_
