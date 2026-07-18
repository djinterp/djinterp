/*****************************************************************************
* djinterp [core]                                            cfg_common.h
*
*   Root of the configuration subframework. #included FIRST by every *_cfg.h.
* Responsibilities:
*     - pick up the user's overrides (cfg_user.h) before ANY default, so a
*       value the user set anywhere is always seen first
*     - apply the testing preset (cfg_testing.h) when in testing mode
*     - provide the shared sentinels / helpers the per-knob cascade uses
*
*   Depends on nothing: it is the root; nothing in the framework sits below it
* (in particular it does NOT include env.h -- env.h's own config includes this).
*
* path:      /inc/djinterp/config/cfg_common.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim
*****************************************************************************/

#ifndef DJINTERP_CFG_COMMON_
#define DJINTERP_CFG_COMMON_ 1

/*
TABLE OF CONTENTS
=================
0.    CONFIG COMMON
      -------------
      1.  Master custom flag        (D_CFG_CUSTOM)
      2.  Boolean sentinels         (D_CFG_ON / D_CFG_OFF)
      3.  Flag helpers (#if-safe)   (D_CFG_NORM / D_CFG_IS_ON / D_CFG_IS_OFF)
      4.  User overrides            (cfg_user.h auto-pickup)
      5.  Canonical testing flag    (D_CFG_TESTING)
      6.  Testing preset            (cfg_testing.h auto-apply)
      7.  Validation
*/


// ===========================================================================
// 0.   CONFIG COMMON
// ===========================================================================

// --- 0.1  Master custom flag ---

// D_CFG_CUSTOM
//   brief: set 1 to signal you are providing custom configuration. Optional --
// cfg_user.h is auto-detected via __has_include regardless -- but this
// flag is a clear opt-in and can gate full up-front resolution (see dconfig.h).
#ifndef D_CFG_CUSTOM
#  define D_CFG_CUSTOM 0
#endif


// --- 0.2  Boolean sentinels ---
//   Framework convention: every config knob is the integer 0 or 1.

// D_CFG_ON / D_CFG_OFF
//   brief: canonical enabled / disabled values (usable in #if).
#ifndef D_CFG_ON
#  define D_CFG_ON  1
#endif
#ifndef D_CFG_OFF
#  define D_CFG_OFF 0
#endif


// --- 0.3  Flag helpers (safe in #if) ---

// D_CFG_NORM(x)
//   brief: normalize a possibly-empty / possibly-undefined flag to an int.
// Robust to `-DFLAG` (empty -> 0), `-DFLAG=1`, `-DFLAG=0`, and undefined (0).
// NOTE: no parentheses around x, so an empty expansion becomes unary `+ 0`.
#define D_CFG_NORM(x)   (x + 0)

// D_CFG_IS_ON(x) / D_CFG_IS_OFF(x)
//   brief: #if-safe predicates, e.g. `#if D_CFG_IS_ON(D_CFG_FOO)`.
#define D_CFG_IS_ON(x)  (D_CFG_NORM(x) == 1)
#define D_CFG_IS_OFF(x) (D_CFG_NORM(x) == 0)


// --- 0.4  User overrides (highest priority; seen before any default) ---
//   The user may (a) #define knobs before including any djinterp header,
// (b) pass -D flags, or (c) drop a cfg_user.h on the include path. (c) is
// auto-detected here so overrides are visible before module defaults in every
// TU, independent of include order. Pre-define D_CFG_USER_HEADER to point at a
// custom path instead.
#ifndef D_CFG_USER_HEADER
#  if defined(__has_include)
#    if __has_include("cfg_user.h")
#      define D_CFG_USER_HEADER "cfg_user.h"
#    endif
#  endif
#endif
#ifdef D_CFG_USER_HEADER
#  include D_CFG_USER_HEADER
#endif


// --- 0.5  Canonical testing-mode flag ---

// D_CFG_TESTING
//   brief: single source of truth for "framework testing / debug mode active".
// Follows the D_TESTING build flag by default; override to force on/off, or
// tie it to env.h's D_ENV_BUILD_DEBUG in your user header if you prefer.
#ifndef D_CFG_TESTING
#  if D_CFG_IS_ON(D_TESTING)
#    define D_CFG_TESTING 1
#  else
#    define D_CFG_TESTING 0
#  endif
#endif


// --- 0.6  Testing preset (applied only in testing mode) ---
//   Seeds testing-appropriate values BEFORE module defaults. The preset's
// knobs are #ifndef-guarded, so explicit user overrides (0.4) still win.
// Priority:  user overrides  >  testing preset  >  module defaults.
#if (D_CFG_TESTING == 1) && !defined(D_CFG_NO_TESTING_PRESET)
#  ifndef D_CFG_TESTING_HEADER
#    define D_CFG_TESTING_HEADER "cfg_testing.h"
#  endif
#  include D_CFG_TESTING_HEADER
#endif


// --- 0.7  Validation ---
#if !defined(D_CFG_TESTING) || (D_CFG_TESTING != 0 && D_CFG_TESTING != 1)
#  error "D_CFG_TESTING must resolve to 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_CUSTOM) && !D_CFG_IS_OFF(D_CFG_CUSTOM)
#  error "D_CFG_CUSTOM must be 0 or 1"
#endif


#endif  // DJINTERP_CFG_COMMON_
