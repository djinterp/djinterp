/******************************************************************************
* djinterp [core]                                            djinterp_qual_cfg.h
*
*   Build-time configuration for the djinterp storage / linkage qualifier
* layer (D_STATIC, D_INLINE, D_CONSTEXPR and their compounds).  This header
* owns every user-overridable knob, normalizes the D_TESTING build flag, and
* publishes the "effective" gates that djinterp.h / djinterp.hpp consume when
* they actually define the qualifiers.  It is language-agnostic (pure
* preprocessor), depends on nothing, and may be included as early as desired;
* include it before the qualifier definitions in both the C and C++ headers.
*
*   TWO CONFIGURATION MODELS
*     define-gate   D_CFG[_TESTING]_DEFINE_<Q> == 0 means "do NOT define this
*                   qualifier -- the user will supply it".  Used by STATIC and
*                   INLINE, whose spelling must stay linker-correct.
*     strip         D_CFG_TESTING_STRIP_CONSTEXPR == 1 means "define D_CONSTEXPR
*                   as EMPTY in a test build" so constant-evaluated paths can be
*                   run and instrumented.  Used by CONSTEXPR only.
*
*   All boolean knobs are strict 0/1 and are validated below.
*
*   path:      /inc/c/djinterp_qual_cfg.h
*   link(s):   TBA
*   author(s): Sam 'teer' Neal-Blim
******************************************************************************/

#ifndef DJINTERP_QUAL_CFG_
#define DJINTERP_QUAL_CFG_ 1

/*
TABLE OF CONTENTS
=================
0.    QUALIFIER CONFIGURATION
      ----------------------
      1.  Master Switch
          a.  D_CFG_DEFINE_QUALIFIERS
      2.  Per-Qualifier Toggles (all build modes)
          a.  D_CFG_DEFINE_STATIC
          b.  D_CFG_DEFINE_INLINE
          c.  D_CFG_DEFINE_CONSTEXPR
      3.  Testing-Mode Toggles (consulted only when D_TESTING == 1)
          a.  D_CFG_TESTING_DEFINE_STATIC
          b.  D_CFG_TESTING_DEFINE_INLINE
          c.  D_CFG_TESTING_STRIP_CONSTEXPR
      4.  Legacy / Compatibility Bridges
          a.  D_TESTING_CONSTEXPR   (deprecated alias)
      5.  Configuration Validation
      6.  Effective (Derived) Values
          a.  D_INTERNAL_QUAL_TESTING
          b.  D_INTERNAL_CFG_STATIC, D_INTERNAL_CFG_INLINE
          c.  D_INTERNAL_CFG_CONSTEXPR
          d.  D_INTERNAL_QUAL_STRIP_CONSTEXPR
      7.  Public Query Macros
          a.  D_QUAL_TESTING_IS_ACTIVE
          b.  D_QUAL_CONSTEXPR_IS_STRIPPED
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
// all D_STATIC / D_INLINE / D_CONSTEXPR (+ compound) definitions and take
// ownership externally; every per-qualifier toggle below is ANDed with it.
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


// --- 0.3  Testing-Mode Toggles (only when D_TESTING == 1) ---

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
//   Each boolean knob must be exactly 0 or 1; a typo (e.g. "true", 2, empty)
// is caught here rather than silently mis-gating a definition.
#if ( (D_CFG_DEFINE_QUALIFIERS)     != 0 && (D_CFG_DEFINE_QUALIFIERS)     != 1 )
    #error "D_CFG_DEFINE_QUALIFIERS must be 0 or 1"
#endif
#if ( (D_CFG_DEFINE_STATIC)         != 0 && (D_CFG_DEFINE_STATIC)         != 1 )
    #error "D_CFG_DEFINE_STATIC must be 0 or 1"
#endif
#if ( (D_CFG_DEFINE_INLINE)         != 0 && (D_CFG_DEFINE_INLINE)         != 1 )
    #error "D_CFG_DEFINE_INLINE must be 0 or 1"
#endif
#if ( (D_CFG_DEFINE_CONSTEXPR)      != 0 && (D_CFG_DEFINE_CONSTEXPR)      != 1 )
    #error "D_CFG_DEFINE_CONSTEXPR must be 0 or 1"
#endif
#if ( (D_CFG_TESTING_DEFINE_STATIC) != 0 && (D_CFG_TESTING_DEFINE_STATIC) != 1 )
    #error "D_CFG_TESTING_DEFINE_STATIC must be 0 or 1"
#endif
#if ( (D_CFG_TESTING_DEFINE_INLINE) != 0 && (D_CFG_TESTING_DEFINE_INLINE) != 1 )
    #error "D_CFG_TESTING_DEFINE_INLINE must be 0 or 1"
#endif
#if ( (D_CFG_TESTING_STRIP_CONSTEXPR) != 0 &&                                 \
      (D_CFG_TESTING_STRIP_CONSTEXPR) != 1 )
    #error "D_CFG_TESTING_STRIP_CONSTEXPR must be 0 or 1"
#endif


// --- 0.6  Effective (Derived) Values ---
//   Internal 0/1 results consumed by djinterp.h / djinterp.hpp.  Do not
// override these directly; set the D_CFG_* knobs above instead.

// D_INTERNAL_QUAL_TESTING
//   brief: strict 0/1 normalization of the D_TESTING build flag.  The unary
// (D_TESTING + 0) makes an empty -DD_TESTING evaluate to 0, so only an
// explicit D_TESTING == 1 activates test mode.
#if defined(D_TESTING) && ( (D_TESTING + 0) == 1 )
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


// --- 0.7  Public Query Macros ---

// D_QUAL_TESTING_IS_ACTIVE
//   brief: 1 in a test build (D_TESTING == 1), else 0.  Safe in #if.
#define D_QUAL_TESTING_IS_ACTIVE            D_INTERNAL_QUAL_TESTING

// D_QUAL_CONSTEXPR_IS_STRIPPED
//   brief: 1 when D_CONSTEXPR currently expands to nothing, else 0.
#define D_QUAL_CONSTEXPR_IS_STRIPPED        D_INTERNAL_QUAL_STRIP_CONSTEXPR


#endif  // DJINTERP_QUAL_CFG_
