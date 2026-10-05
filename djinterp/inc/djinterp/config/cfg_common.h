/*******************************************************************************
* djinterp [config]                                                 cfg_common.h
*
*   Root of the configuration subframework. #included FIRST by every cfg_*.h.
* Responsibilities:
*     - pick up the user's overrides (cfg_custom.h) before ANY default, so a
*       value the user set anywhere is always seen first
*     - apply the testing preset (cfg_testing.h) when in testing mode
*     - provide the shared sentinels / helpers the per-knob cascade uses
*
*   Depends on nothing: it is the root; nothing in the framework sits below it
* (in particular it does NOT include env.h -- env.h's own config includes this).
*
*
* path:      /inc/djinterp/config/cfg_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CFG_COMMON_H
#define DJINTERP_CONFIG_CFG_COMMON_H 1

/*
TABLE OF CONTENTS
=================
0.    CONFIG COMMON
      -------------
      1.    Boolean sentinels          (D_CFG_ON / D_CFG_OFF)
      2.    Flag helpers (#if-safe)    (D_CFG_NORM / D_CFG_IS_ON / D_CFG_IS_OFF)
      3.    Literal helpers (#if-only) (D_CFG_IS_LITERAL_1 / _0 / D_CFG_IS_BOOL)
      4.    Custom overrides           (cfg_custom.h auto-pickup)
      5.    Master custom flag         (D_CFG_CUSTOM + D_CFG_CUSTOM_IS_APPLIED)
      6.    Canonical testing flag     (D_CFG_TESTING)
      7.    Testing preset             (cfg_testing.h auto-apply)
      8.    Validation
*/


// ===========================================================================
// 0.   CONFIG COMMON
// ===========================================================================

// --- 0.1  Boolean sentinels ---
//   Framework convention: every config knob is the integer 0 or 1.

// D_CFG_ON / D_CFG_OFF
//   brief: canonical enabled / disabled values (usable in #if).
#ifndef D_CFG_ON
#  define D_CFG_ON  1
#endif
#ifndef D_CFG_OFF
#  define D_CFG_OFF 0
#endif


// --- 0.2  Flag helpers (safe in #if) ---

// D_CFG_NORM(x)
//   brief: normalize a possibly-empty / possibly-undefined flag to an int.
// Robust to `-DFLAG` (empty -> 0), `-DFLAG=1`, `-DFLAG=0`, and undefined (0).
// NOTE: no parentheses around x, so an empty expansion becomes unary `+ 0`.
//   It also reads any non-numeric identifier as 0, so `-DFLAG=yes` normalizes
// to OFF. That is the arithmetic the language specifies rather than a defect
// here, so it is left alone -- 0.3 is how a knob asks whether it was handed a
// real boolean in the first place.
#define D_CFG_NORM(x)   (x + 0)

// D_CFG_IS_ON(x) / D_CFG_IS_OFF(x)
//   brief: #if-safe predicates, e.g. `#if D_CFG_IS_ON(D_CFG_FOO)`.
#define D_CFG_IS_ON(x)  (D_CFG_NORM(x) == 1)
#define D_CFG_IS_OFF(x) (D_CFG_NORM(x) == 0)


// --- 0.3  Literal helpers (VALID IN #if ONLY) ---
//   D_CFG_IS_ON READS a knob; these VALIDATE one. The question they answer is
// "was this knob given a boolean literal", which arithmetic cannot answer:
// `-DD_CFG_FOO=yes` and `-DD_CFG_FOO=0` are the same value to D_CFG_NORM.
//
//   Mechanism: paste the knob's VALUE onto a marker prefix. Every valid
// spelling lands on a pad -- 1 where it is the literal asked about, 0 where it
// is the other one -- and anything else lands on nothing, which #if reads as
// 0. Because a well-formed knob always lands on a pad, validating one
// evaluates no undefined identifier and stays -Wundef-clean; only a malformed
// value reaches an undefined name, on its way to the #error.
//
//   THREE THINGS THIS COSTS, all load-bearing:
//
//   1. #if ONLY. An unmatched paste leaves an undefined IDENTIFIER -- which is
//      0 to the #if evaluator and a syntax error to the compiler proper.
//      D_CFG_IS_ON works in both places because (x + 0) is arithmetic; these
//      cannot. Never write D_CFG_IS_BOOL outside a preprocessor conditional.
//
//   2. THE PREFIX MUST REACH ## UNEXPANDED. D_INTERNAL_CFG_LIT0_ is itself a
//      macro -- it is the empty-value marker -- so an expanding paste turns
//      `LIT0_ ## yes` into `1 ## yes` -> `1yes` -> "invalid suffix on integer
//      constant". Hence the split below: the PROBE macros expand the VALUE one
//      level up, and the prefix goes straight into a raw `_a ## _b`.
//
//   3. UNDEFINED IS NOT A BOOLEAN. Apply these only AFTER the knob's #ifndef
//      default has run -- before that, "the user did not set it" and "the user
//      set it to garbage" are the same undefined identifier.
//
//   `#define D_CFG_FOO` with no value stays legal and reads as OFF: accepting
// that documented spelling is exactly what D_INTERNAL_CFG_LIT0_ exists for.

// D_INTERNAL_CFG_PASTE(_a, _b)
//   internal: the raw paste. Neither operand is expanded -- that is the point.
#define D_INTERNAL_CFG_PASTE(_a, _b)      _a ## _b

// D_INTERNAL_CFG_LIT1_PROBE(x) / D_INTERNAL_CFG_LIT0_PROBE(x)
//   internal: expand x one level, so a knob defined as a macro is followed to
// its value, then paste that result onto the marker prefix.
#define D_INTERNAL_CFG_LIT1_PROBE(x)      D_INTERNAL_CFG_LIT1_PROBE_(x)
#define D_INTERNAL_CFG_LIT1_PROBE_(_v)                                        \
    D_INTERNAL_CFG_PASTE(D_INTERNAL_CFG_LIT1_, _v)
#define D_INTERNAL_CFG_LIT0_PROBE(x)      D_INTERNAL_CFG_LIT0_PROBE_(x)
#define D_INTERNAL_CFG_LIT0_PROBE_(_v)                                        \
    D_INTERNAL_CFG_PASTE(D_INTERNAL_CFG_LIT0_, _v)

//   The landing pads: 1 for the literal probed, 0 for the other valid
// spellings. Every other paste lands on nothing.
#define D_INTERNAL_CFG_LIT1_1             1
#define D_INTERNAL_CFG_LIT1_0             0
#define D_INTERNAL_CFG_LIT1_              0
#define D_INTERNAL_CFG_LIT0_0             1
#define D_INTERNAL_CFG_LIT0_              1
#define D_INTERNAL_CFG_LIT0_1             0

// D_CFG_IS_LITERAL_1(x) / D_CFG_IS_LITERAL_0(x)
//   brief: 1 when x is (or expands to) exactly the literal 1 / 0. #if only.
// The empty value counts as literal 0, since that is the documented spelling
// of "off" for a bare -DFLAG.
#define D_CFG_IS_LITERAL_1(x)                                                 \
    D_CFG_NORM(D_INTERNAL_CFG_LIT1_PROBE(x))
#define D_CFG_IS_LITERAL_0(x)                                                 \
    D_CFG_NORM(D_INTERNAL_CFG_LIT0_PROBE(x))

// D_CFG_IS_BOOL(x)
//   brief: 1 when x is a usable boolean knob value. #if only. Use it to
// VALIDATE a knob after defaulting it; use D_CFG_IS_ON to READ one.
//     #if !D_CFG_IS_BOOL(D_CFG_FOO)
//     #   error "D_CFG_FOO must be 0 or 1"
//     #endif
#define D_CFG_IS_BOOL(x)                                                      \
    (D_CFG_IS_LITERAL_1(x) || D_CFG_IS_LITERAL_0(x))

// D_INTERNAL_CFG_INT_PROBE(x)
//   internal: as the literal probes, onto the integer marker prefix.
#define D_INTERNAL_CFG_INT_PROBE(x)       D_INTERNAL_CFG_INT_PROBE_(x)
#define D_INTERNAL_CFG_INT_PROBE_(_v)                                         \
    D_INTERNAL_CFG_PASTE(D_INTERNAL_CFG_INT_, _v)

//   The integer landing pads: the decimal literals 0 to 16, and 32 and 64 --
// every value an enumerated knob in the tree takes. A family that needs
// another value adds its pad here. The empty value lands too, reading as 0,
// as it does for a boolean knob.
#define D_INTERNAL_CFG_INT_               1
#define D_INTERNAL_CFG_INT_0              1
#define D_INTERNAL_CFG_INT_1              1
#define D_INTERNAL_CFG_INT_2              1
#define D_INTERNAL_CFG_INT_3              1
#define D_INTERNAL_CFG_INT_4              1
#define D_INTERNAL_CFG_INT_5              1
#define D_INTERNAL_CFG_INT_6              1
#define D_INTERNAL_CFG_INT_7              1
#define D_INTERNAL_CFG_INT_8              1
#define D_INTERNAL_CFG_INT_9              1
#define D_INTERNAL_CFG_INT_10             1
#define D_INTERNAL_CFG_INT_11             1
#define D_INTERNAL_CFG_INT_12             1
#define D_INTERNAL_CFG_INT_13             1
#define D_INTERNAL_CFG_INT_14             1
#define D_INTERNAL_CFG_INT_15             1
#define D_INTERNAL_CFG_INT_16             1
#define D_INTERNAL_CFG_INT_32             1
#define D_INTERNAL_CFG_INT_64             1

// D_CFG_IS_INT_LITERAL(x)
//   brief: 1 when x is (or expands to) one of the integer literals above. #if
// only. Validate every ENUMERATED knob with it after defaulting it: a range
// check cannot see a misspelled value name, because an undefined identifier
// reads as 0 -- usually the AUTO member -- and passes.
//     #if !D_CFG_IS_INT_LITERAL(D_CFG_FOO_BACKEND)
//     #   error "D_CFG_FOO_BACKEND must name one of its values"
//     #endif
#define D_CFG_IS_INT_LITERAL(x)                                               \
    D_CFG_NORM(D_INTERNAL_CFG_INT_PROBE(x))


// --- 0.4  Custom overrides (highest priority; seen before any default) ---
//   The user may (a) #define knobs before including any djinterp header,
// (b) pass -D flags, or (c) drop a cfg_custom.h on the include path. (c) is
// auto-detected here so overrides are visible before module defaults in every
// TU, independent of include order. Pre-define D_CFG_CUSTOM_HEADER to point at
// a custom path instead.
//
//   cfg_user.h is the LEGACY name and is still honoured -- both as a filename
// and through D_CFG_USER_HEADER -- because that is what the code looked for
// first. cfg_custom.h wins where both exist: it is the name the README
// documents, and following the documentation should not be the case that
// silently finds nothing.
#if defined(D_CFG_USER_HEADER) && !defined(D_CFG_CUSTOM_HEADER)
#  define D_CFG_CUSTOM_HEADER D_CFG_USER_HEADER
#endif

#ifndef D_CFG_CUSTOM_HEADER
#  if defined(__has_include)
#    if __has_include("cfg_custom.h")
#      define D_CFG_CUSTOM_HEADER "cfg_custom.h"
#    elif __has_include("cfg_user.h")
#      define D_CFG_CUSTOM_HEADER "cfg_user.h"
#    endif
#  endif
#endif

#ifdef D_CFG_CUSTOM_HEADER
#  include D_CFG_CUSTOM_HEADER
#  define D_INTERNAL_CFG_CUSTOM_APPLIED 1
#else
#  define D_INTERNAL_CFG_CUSTOM_APPLIED 0
#endif


// --- 0.5  Master custom flag ---
//   Defaulted HERE, after the pickup, and deliberately not before: a custom
// header is entitled to define D_CFG_CUSTOM itself, and an earlier default
// would collide with that.

// D_CFG_CUSTOM
//   brief: the user's ASSERTION that custom configuration is being supplied.
// Set it via -D or before any djinterp header. It is a contract, not a hint:
//     1 + header found   -> applied
//     1 + header MISSING -> #error. That is the whole reason to set it.
//     0 + header found   -> applied silently (auto-pickup, unchanged)
//     0 + no header      -> stock defaults, no complaint
#ifndef D_CFG_CUSTOM
#  define D_CFG_CUSTOM 0
#endif

// D_CFG_CUSTOM_IS_APPLIED
//   brief: the resolved FACT -- 1 when a custom header was actually included.
// Deliberately a separate name from D_CFG_CUSTOM: that one is the user's
// input, this one is the output, and auto-pickup makes the two diverge
// routinely. One name could not answer both questions.
#define D_CFG_CUSTOM_IS_APPLIED         D_INTERNAL_CFG_CUSTOM_APPLIED

#if D_CFG_IS_ON(D_CFG_CUSTOM) && (D_CFG_CUSTOM_IS_APPLIED == 0)
#  error "D_CFG_CUSTOM is 1 but no custom configuration header was found. Put cfg_custom.h on the include path, point D_CFG_CUSTOM_HEADER at it, or set D_CFG_CUSTOM to 0."
#endif


// --- 0.6  Canonical testing-mode flag ---

// D_CFG_TESTING
//   brief: single source of truth for "framework testing / debug mode active".
// Follows the D_TESTING build flag by default; override to force on/off, or
// tie it to env.h's D_ENV_BUILD_DEBUG in your custom header if you prefer.
#ifndef D_CFG_TESTING
#  if defined(D_TESTING) && D_CFG_IS_ON(D_TESTING)
#    define D_CFG_TESTING 1
#  else
#    define D_CFG_TESTING 0
#  endif
#endif


// --- 0.7  Testing preset (applied only in testing mode) ---
//   Seeds testing-appropriate values BEFORE module defaults. The preset's
// knobs are #ifndef-guarded, so explicit user overrides (0.4) still win.
// Priority:  user overrides  >  testing preset  >  module defaults.
//   D_INTERNAL_CFG_APPLYING_TESTING_PRESET brackets the include and nothing
// else. It is how cfg_testing.h tells this application from a direct include,
// which it redirects here; asking whether this file is defined cannot, since
// it stays defined long after this point.
#if D_CFG_IS_ON(D_CFG_TESTING) && !defined(D_CFG_NO_TESTING_PRESET)
#  ifndef D_CFG_TESTING_HEADER
#    define D_CFG_TESTING_HEADER "cfg_testing.h"
#  endif
#  define D_INTERNAL_CFG_APPLYING_TESTING_PRESET 1
#  include D_CFG_TESTING_HEADER
#  undef D_INTERNAL_CFG_APPLYING_TESTING_PRESET
#endif


// --- 0.8  Validation ---
//   D_TESTING is checked as a LITERAL rather than arithmetically, because
// -DD_TESTING=ON normalizes to 0: it would DISABLE testing mode, which is the
// opposite of what was asked, and leave no trace of having done so.
#if defined(D_TESTING) && !D_CFG_IS_BOOL(D_TESTING)
#  error "D_TESTING must be 0 or 1. Spellings like ON / true / yes normalize to 0 and would silently DISABLE testing mode; use -DD_TESTING=1."
#endif
#if !D_CFG_IS_BOOL(D_CFG_TESTING)
#  error "D_CFG_TESTING must resolve to 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_CUSTOM)
#  error "D_CFG_CUSTOM must be 0 or 1"
#endif


// djinterp's settings carried into re_std's own switches (the owner's ruling
// of 2026.10.02); last, so D_CFG_TESTING is resolved first
#include "./cfg_re_std.h"  // RE_STD_CFG_ISO_STRICT, RE_STD_CFG_TESTING

#endif  // DJINTERP_CONFIG_CFG_COMMON_H
