/*****************************************************************************
* djinterp [core]                                           cfg_testing.h
*
*   Testing / debug preset. Auto-included by cfg_common.h when
* D_CFG_TESTING == 1 (which follows the D_TESTING build flag by default).
* It is the single, visible home for "what differs in a test/debug build".
*
*   Every knob here is #ifndef-guarded, so an explicit user override always
* wins over the preset:
*     user overrides (dconfig_user.h)  >  THIS preset  >  module defaults
*
*   As the framework grows, add each subframework's testing overrides in its
* own labelled block below -- keep them guarded and documented.
*
* path:      /inc/djinterp/config/cfg_testing.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim
*****************************************************************************/

#ifndef DJINTERP_CFG_TESTING_
#define DJINTERP_CFG_TESTING_ 1

// This file is meant to be pulled in by cfg_common.h, which has already
// defined the sentinels/helpers. Guard against direct misuse.
#ifndef DJINTERP_CFG_COMMON_
#  error "include cfg_common.h (or cfg_all.h); do not include this directly"
#endif

/*
TABLE OF CONTENTS
=================
0.    TESTING / DEBUG PRESET
      ---------------------
      1.  Storage / linkage qualifiers
      2.  Diagnostics & debug utilities
      3.  (subframework testing overrides -- add as they land)
*/


// ===========================================================================
// 0.   TESTING / DEBUG PRESET
// ===========================================================================

// --- 0.1  Storage / linkage qualifiers ---
//   In testing mode the qualifier layer already drops the force-inline hint
// (functions stay real, breakpoint-able, coverable) while keeping definitions
// linker-safe -- that is automatic via D_CFG_TESTING. These affirm the intent
// and give one place to tune it.
#ifndef D_CFG_TESTING_DEFINE_INLINE
#  define D_CFG_TESTING_DEFINE_INLINE 1
#endif
#ifndef D_CFG_TESTING_DEFINE_STATIC
#  define D_CFG_TESTING_DEFINE_STATIC 1
#endif

// D_CFG_TESTING_STRIP_CONSTEXPR
//   brief: strip `constexpr` (-> empty) so constant-evaluated paths run and can
// be instrumented at runtime. Opt-in even in testing: stripping breaks
// constant-required contexts (array bounds, static_assert, template args).
// Flip to 1 here if a test build needs it framework-wide.
#ifndef D_CFG_TESTING_STRIP_CONSTEXPR
#  define D_CFG_TESTING_STRIP_CONSTEXPR 0
#endif


// --- 0.2  Diagnostics & debug utilities ---

// D_DEBUG_
//   brief: enable env.h's debug utilities (e.g. print_compiler_info). This
// pulls <stdio.h> in that header, so it is a real -- but debug-only -- side
// effect. Comment this block out (or set D_CFG_NO_TESTING_PRESET) to suppress.
#ifndef D_DEBUG_
#  define D_DEBUG_ 1
#endif

// D_CFG_DEBUG_ASSERTS
//   brief: enable internal framework assertions/checks in testing builds.
// (Placeholder knob: wire it to the assertion subframework when that lands.)
#ifndef D_CFG_DEBUG_ASSERTS
#  define D_CFG_DEBUG_ASSERTS 1
#endif


// --- 0.3  Subframework testing overrides ---
//   Template for future additions:
//
//     // <subframework>
//     #ifndef D_CFG_<SUB>_<KNOB>
//     #  define D_CFG_<SUB>_<KNOB> <testing-value>
//     #endif


#endif  // DJINTERP_CFG_TESTING_
