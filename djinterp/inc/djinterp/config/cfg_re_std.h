/*******************************************************************************
* djinterp [config]                                                 cfg_re_std.h
*
* djinterp's settings for re_std.
*   re_std is djinterp-agnostic and reads only its own switches (config.h,
* config.hpp). The owner's ruling of 2026.10.02: djinterp's config may set
* re_std's config, which in turn configures re_std. So a djinterp build that
* turns on D_CFG_ENV_ISO_STRICT or testing carries the setting into re_std;
* a switch the user sets directly still wins, as in every cascade here.
*
*   targets:  re_std/config.h   -> RE_STD_CFG_ISO_STRICT
*             re_std/config.hpp -> RE_STD_CFG_TESTING
*   requires: cfg_common.h (D_CFG_IS_ON, D_CFG_TESTING); included at its end,
*             so it is seen before any re_std header a djinterp unit reaches
*             through djinterp's roots or config
*
* path:      /inc/djinterp/config/cfg_re_std.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.02
*                                                            revised: 2026.10.03
*******************************************************************************/
#ifndef DJINTERP_CONFIG_CFG_RE_STD_H
#define DJINTERP_CONFIG_CFG_RE_STD_H 1

#include "./cfg_common.h"  // D_CFG_IS_ON, D_CFG_TESTING


// A switch carried here reaches re_std only where this file is read before
// re_std's own config.h, which fixes re_std's default otherwise: the
// framework root, or cfg_common.h, comes before any re_std header (the
// guides put re_std's includes in a // re_std group after // djinterp). Where
// re_std was configured first, or a build sets the two switches apart, they
// disagree, and the build stops here rather than run re_std on another
// configuration than djinterp's. The fix: djinterp's root first, or the
// RE_STD_CFG_* switch set beside its D_CFG_* one (owner's ruling, L2).

// RE_STD_CFG_ISO_STRICT
//   switch: re_std's ISO strict mode, turned on when a djinterp build turns
// on D_CFG_ENV_ISO_STRICT; otherwise re_std's own default (0) stands.
#if ( (defined(D_CFG_ENV_ISO_STRICT)) &&                                       \
      (D_CFG_IS_ON(D_CFG_ENV_ISO_STRICT)) )
    #ifndef RE_STD_CFG_ISO_STRICT
        #define RE_STD_CFG_ISO_STRICT 1
    #elif !D_CFG_IS_ON(RE_STD_CFG_ISO_STRICT)
        #error "RE_STD_CFG_ISO_STRICT is off under D_CFG_ENV_ISO_STRICT"
    #endif  // RE_STD_CFG_ISO_STRICT
#endif

// RE_STD_CFG_TESTING
//   switch: re_std's testing mode, turned on with djinterp's D_CFG_TESTING.
#if D_CFG_IS_ON(D_CFG_TESTING)
    #ifndef RE_STD_CFG_TESTING
        #define RE_STD_CFG_TESTING 1
    #elif !D_CFG_IS_ON(RE_STD_CFG_TESTING)
        #error "RE_STD_CFG_TESTING is off under D_CFG_TESTING"
    #endif  // RE_STD_CFG_TESTING
#endif


#endif  // DJINTERP_CONFIG_CFG_RE_STD_H
