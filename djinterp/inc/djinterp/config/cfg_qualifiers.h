/*******************************************************************************
* djinterp [config]                                             cfg_qualifiers.h
*
*   Build-time configuration for the djinterp storage / linkage qualifier
* layer (D_STATIC, D_INLINE, D_CONSTEXPR, D_EXTERN_C and their compounds).
* This header owns every user-overridable knob, follows the canonical testing
* flag (D_CFG_TESTING), and publishes the "effective" gates that djinterp.h /
* djinterp.hpp consume when they actually define the qualifiers.  It is
* language-agnostic (pure preprocessor) and may be included as early as
* desired; include it before the qualifier definitions in both the C and C++
* headers.
*
*   TWO CONFIGURATION MODELS
*     define-gate   D_CFG[_TESTING]_DEFINE_<Q> == 0 means "do NOT define this
*                   qualifier -- the user will supply it".  Used by STATIC,
*                   INLINE and EXTERN_C, whose spelling must stay
*                   linker-correct.
*     strip         D_CFG_TESTING_STRIP_CONSTEXPR == 1 means "define D_CONSTEXPR
*                   as EMPTY in a test build" so constant-evaluated paths can be
*                   run and instrumented.  Used by CONSTEXPR only.
*
*   EXTERN_C takes TWO knobs rather than one, because EXISTENCE and BEHAVIOUR
* are different questions: whether the family is defined at all
* (D_CFG_DEFINE_EXTERN_C) is separate from whether it emits `extern "C"` under
* a C++ compiler (D_CFG_EXTERN_C_LINKAGE).
*
*   All boolean knobs are strict 0/1 and are validated below with
* D_CFG_IS_BOOL, which rejects the spellings D_CFG_NORM would silently read as
* 0 (`yes`, `ON`, `2`, ...).
*
*
* path:      /inc/djinterp/config/cfg_qualifiers.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CFG_QUALIFIERS_H
#define DJINTERP_CONFIG_CFG_QUALIFIERS_H 1

// djinterp
//   Root first, per the framework rule that every cfg_*.h includes it: it
// supplies D_CFG_IS_BOOL (used throughout 0.6) and, more importantly, runs the
// custom-configuration pickup BEFORE the #ifndef defaults below.  Without this
// include a cfg_custom.h setting D_CFG_DEFINE_INLINE 0 was ignored in silence,
// because the defaults here had already won.
#include "cfg_common.h"

/*
TABLE OF CONTENTS
=================
0.    QUALIFIER CONFIGURATION
      -----------------------
      1.    Master Switch
      a. D_CFG_DEFINE_QUALIFIERS
      2.    Per-Qualifier Toggles (all build modes)
      a. D_CFG_DEFINE_STATIC
      b. D_CFG_DEFINE_INLINE
      c.    D_CFG_DEFINE_CONSTEXPR
            d. D_CFG_DEFINE_EXTERN_C
            e. D_CFG_EXTERN_C_LINKAGE
            3.    Testing-Mode Toggles (consulted only in testing mode)
            a. D_CFG_TESTING_DEFINE_STATIC
            b. D_CFG_TESTING_DEFINE_INLINE
      c.    D_CFG_TESTING_STRIP_CONSTEXPR
            4.    Legacy / Compatibility Bridges
            a. D_TESTING_CONSTEXPR   (deprecated alias)
            5.    Configuration Validation
            6.    Effective (Derived) Values
            a. D_INTERNAL_QUAL_TESTING
            b. D_INTERNAL_CFG_STATIC, D_INTERNAL_CFG_INLINE
      c.    D_INTERNAL_CFG_CONSTEXPR
            d. D_INTERNAL_QUAL_STRIP_CONSTEXPR
            e. D_INTERNAL_CFG_EXTERN_C, D_INTERNAL_QUAL_EXTERN_C_LINKAGE
            7.    Public Query Macros
            a. D_QUAL_TESTING_IS_ACTIVE
            b. D_QUAL_CONSTEXPR_IS_STRIPPED
      c.    D_QUAL_EXTERN_C_IS_ACTIVE
*/


// ===========================================================================
// 0.   QUALIFIER CONFIGURATION
// ===========================================================================
//   Every knob follows the framework convention: pre-define it before
// including this header to override; otherwise it takes the default here.
// A value of 1 enables, 0 disables.


// --- 0.1  Master Switch ---

// D_CFG_DEFINE_QUALIFIERS
//   brief: master enable for the ENTIRE qualifier layer.  Set 0 to suppress
// all D_STATIC / D_INLINE / D_CONSTEXPR / D_EXTERN_C (+ compound) definitions
// and take ownership externally; every per-qualifier toggle below is ANDed
// with it.
#ifndef D_CFG_DEFINE_QUALIFIERS
    #define D_CFG_DEFINE_QUALIFIERS         1
#endif


// --- 0.2  Per-Qualifier Toggles (all build modes) ---

// D_CFG_DEFINE_STATIC
//   brief: define D_STATIC (and feed it to the compounds).
#ifndef D_CFG_DEFINE_STATIC
    #define D_CFG_DEFINE_STATIC             1
#endif

// D_CFG_DEFINE_INLINE
//   brief: define D_INLINE / D_STATIC_INLINE.
#ifndef D_CFG_DEFINE_INLINE
    #define D_CFG_DEFINE_INLINE             1
#endif

// D_CFG_DEFINE_CONSTEXPR
//   brief: define D_CONSTEXPR and the constexpr compounds.
#ifndef D_CFG_DEFINE_CONSTEXPR
    #define D_CFG_DEFINE_CONSTEXPR          1
#endif

// D_CFG_DEFINE_EXTERN_C
//   brief: define the D_EXTERN_C family (D_EXTERN_C, D_EXTERN_C_BEGIN,
// D_EXTERN_C_END).  Set 0 to supply your own -- same contract as
// D_CFG_DEFINE_STATIC.  Headers that need the family say so with a one-line
// #error rather than failing as a syntax cascade, so a forgotten replacement
// is one sentence, not fifty diagnostics.
#ifndef D_CFG_DEFINE_EXTERN_C
    #define D_CFG_DEFINE_EXTERN_C           1
#endif

// D_CFG_EXTERN_C_LINKAGE
//   brief: whether the family EMITS `extern "C"` under a C++ compiler.  Under
// a C compiler the question does not arise and all three spellings expand to
// nothing regardless -- which is what makes them safe to write unconditionally
// in a shared header instead of scattering #ifdef __cplusplus.
//
//   Setting this to 0 has exactly one real use: compiling djinterp's own
// sources AS C++ and wanting C++ linkage (overloads, namespacing) for them.
// It is NOT a way to consume a C-compiled archive from C++ -- that combination
// cannot work, and fails loudly at link time
// (`undefined reference to 'd_file_read_all(char const*, unsigned long*)'`)
// rather than quietly:
//
//     build                    symbol the linker sees
//     C source,   C compiler   d_file_open_stream
//     C++ source, LINKAGE=1    d_file_open_stream        (links against the C archive)
//     C++ source, LINKAGE=0    _Z7d_fopenPKcS0_
#ifndef D_CFG_EXTERN_C_LINKAGE
    #define D_CFG_EXTERN_C_LINKAGE          1
#endif


// --- 0.3  Testing-Mode Toggles (only in testing mode, D_CFG_TESTING) ---

// D_CFG_TESTING_DEFINE_STATIC
//   brief: define D_STATIC in a test build.  Keep 1: internal linkage is what
// prevents duplicate-symbol errors, so it is wanted in testing too.
#ifndef D_CFG_TESTING_DEFINE_STATIC
    #define D_CFG_TESTING_DEFINE_STATIC     1
#endif

// D_CFG_TESTING_DEFINE_INLINE
//   brief: define D_INLINE / D_STATIC_INLINE in a test build.  Keep 1: the
// definitions stay header- and linker-safe; only the force-inline optimizer
// hint is dropped (that happens in the qualifier resolution, not here) so
// breakpoints and coverage work.
#ifndef D_CFG_TESTING_DEFINE_INLINE
    #define D_CFG_TESTING_DEFINE_INLINE     1
#endif

// D_CFG_TESTING_STRIP_CONSTEXPR
//   brief: STRIP semantics -- when 1, D_CONSTEXPR expands to nothing in a test
// build so otherwise constant-evaluated code can execute and be instrumented.
// Off by default: stripping breaks contexts that REQUIRE a constant
// expression (array bounds, template args, static_assert, ...).
#ifndef D_CFG_TESTING_STRIP_CONSTEXPR
    #define D_CFG_TESTING_STRIP_CONSTEXPR   0
#endif


// --- 0.4  Legacy / Compatibility Bridges ---

// D_TESTING_CONSTEXPR
//   brief: DEPRECATED predecessor of D_CFG_TESTING_STRIP_CONSTEXPR.  Not
// defaulted here (its absence is meaningful).  If a project defines it to 1,
// it is honoured as an alias that forces constexpr stripping in test builds
// (see 0.6.d).  New code should use D_CFG_TESTING_STRIP_CONSTEXPR instead.


// --- 0.5  Configuration Validation ---
//   Each boolean knob must be exactly 0 or 1.  D_CFG_IS_BOOL rather than the
// arithmetic `!= 0 && != 1` idiom: the arithmetic form accepts -DKNOB=yes,
// because `yes + 0` is 0, which is a legal value.  The literal probe does not.
#if !D_CFG_IS_BOOL(D_CFG_DEFINE_QUALIFIERS)
    #error "D_CFG_DEFINE_QUALIFIERS must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DEFINE_STATIC)
    #error "D_CFG_DEFINE_STATIC must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DEFINE_INLINE)
    #error "D_CFG_DEFINE_INLINE must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DEFINE_CONSTEXPR)
    #error "D_CFG_DEFINE_CONSTEXPR must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_DEFINE_EXTERN_C)
    #error "D_CFG_DEFINE_EXTERN_C must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_EXTERN_C_LINKAGE)
    #error "D_CFG_EXTERN_C_LINKAGE must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_TESTING_DEFINE_STATIC)
    #error "D_CFG_TESTING_DEFINE_STATIC must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_TESTING_DEFINE_INLINE)
    #error "D_CFG_TESTING_DEFINE_INLINE must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_TESTING_STRIP_CONSTEXPR)
    #error "D_CFG_TESTING_STRIP_CONSTEXPR must be 0 or 1"
#endif


// --- 0.6  Effective (Derived) Values ---
//   Internal 0/1 results consumed by djinterp.h / djinterp.hpp.  Do not
// override these directly; set the D_CFG_* knobs above instead.

// D_INTERNAL_QUAL_TESTING
//   brief: 1 in testing mode.  Follows the config layer's canonical
// D_CFG_TESTING, which cfg_common.h derives from the D_TESTING build flag and
// which can be forced either way on its own.  Keying on the raw D_TESTING
// instead made -DD_CFG_TESTING=1 apply the testing preset without the
// qualifier behaviour it tunes, and -DD_TESTING=1 -DD_CFG_TESTING=0 keep that
// behaviour on.
#if D_CFG_IS_ON(D_CFG_TESTING)
    #define D_INTERNAL_QUAL_TESTING         1
#else
    #define D_INTERNAL_QUAL_TESTING         0
#endif

// D_INTERNAL_CFG_STATIC
//   brief: effective enable for D_STATIC, folding in the master switch and the
// active (normal vs testing) gate.
#if D_INTERNAL_QUAL_TESTING
    #if ( (D_CFG_DEFINE_QUALIFIERS == 1) &&                                    \
          (D_CFG_TESTING_DEFINE_STATIC == 1) )
        #define D_INTERNAL_CFG_STATIC       1
    #else
        #define D_INTERNAL_CFG_STATIC       0
    #endif
#else
    #if ( (D_CFG_DEFINE_QUALIFIERS == 1) && (D_CFG_DEFINE_STATIC == 1) )
        #define D_INTERNAL_CFG_STATIC       1
    #else
        #define D_INTERNAL_CFG_STATIC       0
    #endif
#endif

// D_INTERNAL_CFG_INLINE
//   brief: effective enable for D_INLINE / D_STATIC_INLINE (master + active
// gate).
#if D_INTERNAL_QUAL_TESTING
    #if ( (D_CFG_DEFINE_QUALIFIERS == 1) &&                                    \
          (D_CFG_TESTING_DEFINE_INLINE == 1) )
        #define D_INTERNAL_CFG_INLINE       1
    #else
        #define D_INTERNAL_CFG_INLINE       0
    #endif
#else
    #if ( (D_CFG_DEFINE_QUALIFIERS == 1) && (D_CFG_DEFINE_INLINE == 1) )
        #define D_INTERNAL_CFG_INLINE       1
    #else
        #define D_INTERNAL_CFG_INLINE       0
    #endif
#endif

// D_INTERNAL_CFG_CONSTEXPR
//   brief: effective enable for the constexpr family.  Enabling is
// mode-independent (stripping, below, is the test-mode behavior).
#if ( (D_CFG_DEFINE_QUALIFIERS == 1) && (D_CFG_DEFINE_CONSTEXPR == 1) )
    #define D_INTERNAL_CFG_CONSTEXPR        1
#else
    #define D_INTERNAL_CFG_CONSTEXPR        0
#endif

// D_INTERNAL_QUAL_STRIP_CONSTEXPR
//   brief: 1 when D_CONSTEXPR must expand to nothing -- only in a test build,
// and only if the strip config or the legacy D_TESTING_CONSTEXPR alias asks
// for it.
#if ( D_INTERNAL_QUAL_TESTING &&                                              \
      ( (D_CFG_TESTING_STRIP_CONSTEXPR == 1) ||                               \
        ( defined(D_TESTING_CONSTEXPR) && ((D_TESTING_CONSTEXPR + 0) == 1) ) ) )
    #define D_INTERNAL_QUAL_STRIP_CONSTEXPR 1
#else
    #define D_INTERNAL_QUAL_STRIP_CONSTEXPR 0
#endif

// D_INTERNAL_CFG_EXTERN_C
//   brief: effective enable for the D_EXTERN_C family (master + toggle).
// Mode-independent: linkage is not a thing test builds should differ on.
#if ( (D_CFG_DEFINE_QUALIFIERS == 1) && (D_CFG_DEFINE_EXTERN_C == 1) )
    #define D_INTERNAL_CFG_EXTERN_C         1
#else
    #define D_INTERNAL_CFG_EXTERN_C         0
#endif

// D_INTERNAL_QUAL_EXTERN_C_LINKAGE
//   brief: 1 when the family must actually emit `extern "C"`.  ALWAYS 0 under
// a C compiler -- there is no such thing to emit -- which is the property that
// lets a shared header write D_EXTERN_C_BEGIN unconditionally.
#if ( defined(__cplusplus) && (D_CFG_EXTERN_C_LINKAGE == 1) )
    #define D_INTERNAL_QUAL_EXTERN_C_LINKAGE 1
#else
    #define D_INTERNAL_QUAL_EXTERN_C_LINKAGE 0
#endif


// --- 0.7  Public Query Macros ---

// D_QUAL_TESTING_IS_ACTIVE
//   brief: 1 in a test build (D_TESTING == 1), else 0.  Safe in #if.
#define D_QUAL_TESTING_IS_ACTIVE            D_INTERNAL_QUAL_TESTING

// D_QUAL_CONSTEXPR_IS_STRIPPED
//   brief: 1 when D_CONSTEXPR currently expands to nothing, else 0.
#define D_QUAL_CONSTEXPR_IS_STRIPPED        D_INTERNAL_QUAL_STRIP_CONSTEXPR

// D_QUAL_EXTERN_C_IS_ACTIVE
//   brief: 1 when the D_EXTERN_C family currently emits `extern "C"`, else 0.
// Safe in #if.  Ask this rather than `defined(__cplusplus)` -- the two differ
// whenever D_CFG_EXTERN_C_LINKAGE is 0.
#define D_QUAL_EXTERN_C_IS_ACTIVE           D_INTERNAL_QUAL_EXTERN_C_LINKAGE


#endif  // DJINTERP_CONFIG_CFG_QUALIFIERS_H
