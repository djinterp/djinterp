/*******************************************************************************
* djinterp [config]                                                    dconfig.h
*
*   Configuration umbrella. Including this file resolves the ENTIRE config
* graph up front. Use it when you want cross-cutting deductions applied
* deterministically, or to precompile all configuration into a PCH.
*
*   You do NOT need this for normal use: each module pulls its own *_cfg.h,
* which pulls dconfig_common.h -- so you only pay for the modules you include
* (demand-loading). This umbrella is the opt-in "resolve everything" path.
*
*
* path:      /inc/djinterp/config/dconfig.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_CONFIG_DCONFIG_H
#define DJINTERP_CONFIG_DCONFIG_H 1

// djinterp
// Root first: user overrides + testing preset + shared helpers.
#include "cfg_common.h"
#include "djinterp/config/core/env/cfg_env.h" // environment detection tuning
#include "cfg_qualifiers.h"      // storage / linkage qualifiers
#include "djinterp/config/core/container/table/cfg_table.h"  // the table DSL subframework
#include "djinterp/config/parse/cfg_parse.h"        // the parse substrate
#include "djinterp/config/parsegen/cfg_parsegen.h"  // parser generation (after parse)
#include "djinterp/config/net/pop/cfg_pop.h"        // POP3 common kernel
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


#endif  // DJINTERP_CONFIG_DCONFIG_H
