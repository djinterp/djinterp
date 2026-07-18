/*****************************************************************************
* djinterp [core]                                                   cfg_all.h
*
*   Configuration umbrella. Including this file resolves the ENTIRE config
* graph up front. Use it when you want cross-cutting deductions applied
* deterministically, or to precompile all configuration into a PCH.
*
*   You do NOT need this for normal use: each module pulls its own *_cfg.h,
* which pulls dconfig_common.h -- so you only pay for the modules you include
* (demand-loading). This umbrella is the opt-in "resolve everything" path.
*
* path:      /inc/djinterp/config/cfg_all.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim
*****************************************************************************/

#ifndef DJINTERP_CFG_ALL_
#define DJINTERP_CFG_ALL_ 1

// Root first: user overrides + testing preset + shared helpers.
#include "cfg_common.h"


// ===========================================================================
// I.   SUBFRAMEWORK CONFIGURATIONS   (include in dependency order)
// ===========================================================================
//   Each of these also includes dconfig_common.h at its top (guarded, so it is
// a cheap skip here). List env first; others depend on its detection tuning.

#include "core/env/env_config.h" // environment detection tuning
#include "cfg_qualifiers.h"      // storage / linkage qualifiers
#include "core/container/table/cfg_table.h"  // the table DSL subframework
// #include "core/<sub>/cfg_<sub>.h"  // <- add future subframeworks here


// ===========================================================================
// II.  CROSS-CUTTING DEDUCTIONS
// ===========================================================================
//   Long-range propagation that demand-loading cannot order correctly belongs
// here (module A influencing an otherwise-unrelated module B). Every rule is
// #ifndef-guarded so explicit user overrides still win. Example:
//
//     #if D_CFG_IS_ON(D_CFG_FOO_ADVANCED)
//     #  ifndef D_CFG_BAR_BACKEND
//     #    define D_CFG_BAR_BACKEND 1
//     #  endif
//     #endif
//
//   (none defined yet)


#endif  // DJINTERP_CFG_ALL_
