/******************************************************************************
* djinterp [re_std]                                           ratio_typedefs.hpp
*
* SI prefix typedefs header:
*   The twenty named ratios of the SI prefix system, from atto to exa
* unconditionally and the four wider ones when intmax_t can hold them.
*
*     milli = ratio<1, 1000>        kilo = ratio<1000, 1>
*     micro = ratio<1, 1000000>     mega = ratio<1000000, 1>
*
*   WHY FOUR OF THEM ARE CONDITIONAL:
*   zepto/zetta need 10^21 and yocto/yotta need 10^24. A 64-bit
* intmax_t tops out near 9.22 x 10^18, so those four literals are not
* representable and merely WRITING them would be ill-formed -- not a
* silently wrong value, a hard error. The standard makes them
* conditionally present for exactly this reason, and the #if below is
* the condition, tested against INTMAX_MAX rather than against a
* guessed width.
*
*   exa and atto (10^18) are the largest that always fit, which is why
* the unconditional block stops there.
*
*   These are typedefs, not aliases, so they work as written on every
* tier this module supports.
*
*   PORTABILITY:
*   C++11, matching std. C++26 adds quecto/ronto/ronna/quetta (10^-30
* through 10^30); those need at least a 128-bit intmax_t and are not
* provided here, since no platform re_std targets has one.
*
*
* path:      /inc/djinterp/re_std/ratio/ratio_typedefs.hpp
* link(s):   TBA
* author(s): TBA                                           created: 2026.08.25
******************************************************************************/

#ifndef DJINTERP_RE_STD_RATIO_RATIO_TYPEDEFS_
#define DJINTERP_RE_STD_RATIO_RATIO_TYPEDEFS_ 1

// djinterp
#include "../../core/djinterp.hpp"


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstdint>
#include <climits>

// djinterp
#include "./ratio.hpp"


NS_RESTD


// ===========================================================================
// I.   SUB-UNIT PREFIXES (always available)
// ===========================================================================

typedef ratio<1, 1000000000000000000> atto;
typedef ratio<1,    1000000000000000> femto;
typedef ratio<1,       1000000000000> pico;
typedef ratio<1,          1000000000> nano;
typedef ratio<1,             1000000> micro;
typedef ratio<1,                1000> milli;
typedef ratio<1,                 100> centi;
typedef ratio<1,                  10> deci;


// ===========================================================================
// II.  MULTIPLE PREFIXES (always available)
// ===========================================================================

typedef ratio<                  10, 1> deca;
typedef ratio<                 100, 1> hecto;
typedef ratio<                1000, 1> kilo;
typedef ratio<             1000000, 1> mega;
typedef ratio<          1000000000, 1> giga;
typedef ratio<       1000000000000, 1> tera;
typedef ratio<    1000000000000000, 1> peta;
typedef ratio< 1000000000000000000, 1> exa;


// ===========================================================================
// III. WIDE PREFIXES (only when intmax_t can represent them)
// ===========================================================================

// 10^21 and 10^24 exceed a 64-bit intmax_t. The literals themselves would
// be ill-formed, so the whole block is gated rather than the typedefs
// being defined with clamped values.
#if defined(INTMAX_MAX) && (INTMAX_MAX / 1000000000000000000) >= 1000

    typedef ratio<1, 1000000000000000000000>    zepto;
    typedef ratio<1, 1000000000000000000000000> yocto;

    typedef ratio<1000000000000000000000,    1> zetta;
    typedef ratio<1000000000000000000000000, 1> yotta;

    #define D_RE_STD_HAS_WIDE_SI_PREFIXES 1

#else

    // Signals to a consumer that zepto / yocto / zetta / yotta are absent
    // on this platform, so it can diagnose rather than fail to find them.
    #define D_RE_STD_HAS_WIDE_SI_PREFIXES 0

#endif


NS_END  // re_std


#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


#endif  // DJINTERP_RE_STD_RATIO_RATIO_TYPEDEFS_
