/*******************************************************************************
* djinterp [config]                                                cfg_testing.h
*
* The testing / debug preset.
*   The one visible home for what differs in a test or debug build.
* cfg_common.h applies it when D_CFG_TESTING is 1 (which follows the D_TESTING
* build flag), after the user's overrides and before any module default:
*     user overrides  >  this preset  >  module defaults.
* Every knob is #ifndef-guarded, which is what keeps a user override on top.
* Suppress the preset with -DD_CFG_NO_TESTING_PRESET, or substitute another
* with D_CFG_TESTING_HEADER.
*   An entry records what a test build requires, whether or not that equals
* the module's current default: a default may change for reasons of its own,
* and the requirement should not change with it. Module defaults themselves
* live in each module's cfg_*.h, never here. Where a value must hold even
* against the user's setting, the module folds D_CFG_TESTING into its derived
* value; this file cannot, because a user override wins here by design.
*   Including this file directly is correct, not merely compilable. Only
* cfg_common.h's own include applies the body; any other include goes to
* cfg_common.h instead, which applies the preset in testing mode and does
* nothing otherwise.
*
*   targets:  no single module (infrastructure); each block below names the
*             subframework it serves and the cfg_*.h that reads it
*   requires: cfg_common.h, which includes this file; a direct include is
*             redirected to it
*
* path:      /inc/djinterp/config/cfg_testing.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.30
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FRAMEWORK-WIDE
    --------------
    1.  Storage and linkage qualifiers
         1.  D_CFG_TESTING_DEFINE_STATIC
         2.  D_CFG_TESTING_DEFINE_INLINE
         3.  D_CFG_TESTING_STRIP_CONSTEXPR
    2.  Diagnostics and debug utilities
         1.  D_DEBUG_
         2.  D_CFG_DEBUG_ASSERTS
2.  SUBFRAMEWORK OVERRIDES
    ----------------------
    1.  File system
         1.  D_CFG_FILE_ASSERT_PARAMS
    2.  Archive
         1.  D_CFG_ARCHIVE_PREFER_BUILTIN
         2.  D_CFG_ARCHIVE_REPRODUCIBLE
         3.  D_CFG_ARCHIVE_CHECK_ENTRY_NAMES
    3.  Compression
         1.  D_CFG_COMPRESS_PIN_THREADS
         2.  D_CFG_COMPRESS_PIN_DEFAULTS
         3.  D_CFG_COMPRESS_VALIDATE
    4.  Parse
         1.  D_CFG_PARSE_MACHINE_TRACE
    5.  Test counter
         1.  D_CFG_TEST_COUNTER_ENFORCE_BOUNDS
         2.  D_CFG_TEST_COUNTER_EVENTS
    6.  Parity oracle
         1.  D_CFG_TEST_PARITY
         2.  D_CFG_TEST_PARITY_STRICT
*/

#ifndef DJINTERP_CONFIG_CFG_TESTING_H

//   cfg_common.h defines D_INTERNAL_CFG_APPLYING_TESTING_PRESET for exactly
// the span of its own include of this file, and only in testing mode. Any
// other include goes to cfg_common.h: the first time, that runs the whole
// cascade, which includes this file again at the right point when testing
// mode is on; after that, the root's guard makes it a no-op. Asking whether
// cfg_common.h is defined could not tell the root's include from a later
// direct one, which in a normal build would switch the test knobs on.
//   The guard is defined in the applying branch rather than straight after
// the #ifndef: a direct include that arrives first reaches cfg_common.h, which
// includes this file again, and a guard defined on the way in would turn that
// second, applying include into nothing.
#if !defined(D_INTERNAL_CFG_APPLYING_TESTING_PRESET)
    #include "cfg_common.h"  // applies this preset, in testing mode only
#else
#define DJINTERP_CONFIG_CFG_TESTING_H 1


//==============================================================================
// 1.  FRAMEWORK-WIDE
//==============================================================================


// 1.1    Storage and linkage qualifiers
//------------------------------------------------------------------------------
//   cfg_qualifiers.h consults these three only in testing mode, and gives them
// the same values. In testing mode the qualifier layer drops the force-inline
// hint by itself, so functions stay real, breakpoint-able and coverable while
// their definitions stay linker-safe.

// 1.1.1
// D_CFG_TESTING_DEFINE_STATIC
//   knob: define D_STATIC in a test build. Internal linkage is what keeps a
// header's definitions free of duplicate symbols, in testing as anywhere.
#ifndef D_CFG_TESTING_DEFINE_STATIC
    #define D_CFG_TESTING_DEFINE_STATIC 1
#endif  // D_CFG_TESTING_DEFINE_STATIC

// 1.1.2
// D_CFG_TESTING_DEFINE_INLINE
//   knob: define D_INLINE and D_STATIC_INLINE in a test build. The definitions
// stay header- and linker-safe; only the force-inline hint goes.
#ifndef D_CFG_TESTING_DEFINE_INLINE
    #define D_CFG_TESTING_DEFINE_INLINE 1
#endif  // D_CFG_TESTING_DEFINE_INLINE

// 1.1.3
// D_CFG_TESTING_STRIP_CONSTEXPR
//   knob: 1 makes D_CONSTEXPR expand to nothing in a test build, so code that
// would be constant-evaluated runs and can be instrumented. 0 here: stripping
// breaks every context that requires a constant expression (array bounds,
// template arguments, static assertions). Set it to 1 for a test build that
// needs it framework-wide.
#ifndef D_CFG_TESTING_STRIP_CONSTEXPR
    #define D_CFG_TESTING_STRIP_CONSTEXPR 0
#endif  // D_CFG_TESTING_STRIP_CONSTEXPR

// 1.2    Diagnostics and debug utilities
//------------------------------------------------------------------------------
// 1.2.1
// D_DEBUG_
//   flag: declare env.h's debug utilities (d_env_print_compiler_info). env.h
// tests it with #ifdef, so defining it to 0 does not turn it off;
// D_CFG_NO_TESTING_PRESET does. No source defines the function yet, so a test
// build may declare it but must not call it.
#ifndef D_DEBUG_
    #define D_DEBUG_ 1
#endif  // D_DEBUG_

// 1.2.2
// D_CFG_DEBUG_ASSERTS
//   knob: enable the framework's internal assertions in a test build. A
// placeholder: nothing reads it yet; the assertion subframework will.
#ifndef D_CFG_DEBUG_ASSERTS
    #define D_CFG_DEBUG_ASSERTS 1
#endif  // D_CFG_DEBUG_ASSERTS


//==============================================================================
// 2.  SUBFRAMEWORK OVERRIDES
//==============================================================================
//   One block per subframework, in the order of the config tree. Add a block
// the same way: labelled, #ifndef-guarded, one entry per knob that a test build
// requires. Where the module also forces its derived value on in a test build,
// the entry says so: that fold is what holds when the user has set the knob to
// 0, or suppressed this preset.


// 2.1    File system
//------------------------------------------------------------------------------
// 2.1.1
// D_CFG_FILE_ASSERT_PARAMS
//   knob: trip an assertion on a failed parameter check, so a contract
// violation is loud rather than an error code the caller ignores. Off by
// default in cfg_file_common.h, which names this preset as what turns it on.
// Nothing reads it yet; when a module does, the fs suites' tests that pass
// invalid parameters on purpose must expect the assertion, or set it to 0.
#ifndef D_CFG_FILE_ASSERT_PARAMS
    #define D_CFG_FILE_ASSERT_PARAMS 1
#endif  // D_CFG_FILE_ASSERT_PARAMS

// 2.2    Archive
//------------------------------------------------------------------------------
//   Read by cfg_archive.h.

// 2.2.1
// D_CFG_ARCHIVE_PREFER_BUILTIN
//   knob: use the dependency-free tar and ZIP writers even where a library
// exists. A parity suite against library-written containers measures the
// library's version, not this framework. cfg_archive.h also forces its gate on
// in a test build.
#ifndef D_CFG_ARCHIVE_PREFER_BUILTIN
    #define D_CFG_ARCHIVE_PREFER_BUILTIN 1
#endif  // D_CFG_ARCHIVE_PREFER_BUILTIN

// 2.2.2
// D_CFG_ARCHIVE_REPRODUCIBLE
//   knob: an entry with no mtime records the pinned epoch, not the clock. A
// container that embeds "now" cannot be compared with anything.
#ifndef D_CFG_ARCHIVE_REPRODUCIBLE
    #define D_CFG_ARCHIVE_REPRODUCIBLE 1
#endif  // D_CFG_ARCHIVE_REPRODUCIBLE

// 2.2.3
// D_CFG_ARCHIVE_CHECK_ENTRY_NAMES
//   knob: enforce the path-traversal guard; a suite that skips the check
// cannot assert it. cfg_archive.h also forces its gate on in a test build.
#ifndef D_CFG_ARCHIVE_CHECK_ENTRY_NAMES
    #define D_CFG_ARCHIVE_CHECK_ENTRY_NAMES 1
#endif  // D_CFG_ARCHIVE_CHECK_ENTRY_NAMES

// 2.3    Compression
//------------------------------------------------------------------------------
//   Read by cfg_compress.h.

// 2.3.1
// D_CFG_COMPRESS_PIN_THREADS
//   knob: pin every codec to one worker. A multi-threaded encoder partitions
// its input by worker count, so a parity suite run on machines with different
// core counts would diverge for a reason that is not a bug. cfg_compress.h
// also forces its gate on in a test build.
#ifndef D_CFG_COMPRESS_PIN_THREADS
    #define D_CFG_COMPRESS_PIN_THREADS 1
#endif  // D_CFG_COMPRESS_PIN_THREADS

// 2.3.2
// D_CFG_COMPRESS_PIN_DEFAULTS
//   knob: resolve unspecified knobs from the core's table rather than the
// backend's, so two builds linking different library versions still produce
// identical bytes. cfg_compress.h also forces its gate on in a test build.
#ifndef D_CFG_COMPRESS_PIN_DEFAULTS
    #define D_CFG_COMPRESS_PIN_DEFAULTS 1
#endif  // D_CFG_COMPRESS_PIN_DEFAULTS

// 2.3.3
// D_CFG_COMPRESS_VALIDATE
//   knob: range-check option knobs, so an out-of-range knob is reported by
// name rather than surfacing as a backend error.
#ifndef D_CFG_COMPRESS_VALIDATE
    #define D_CFG_COMPRESS_VALIDATE 1
#endif  // D_CFG_COMPRESS_VALIDATE

// 2.4    Parse
//------------------------------------------------------------------------------
// 2.4.1
// D_CFG_PARSE_MACHINE_TRACE
//   knob: the machine's per-dispatch trace hook is a debugging facility, so a
// test build gets it. Unlike the other entries this also defers to
// D_CFG_PARSE_ALL: an aggregate the user set is an explicit choice, and "all of
// parse off" must switch the trace off too. It changes the layout of struct
// d_parse_machine (cfg_parse.h), so every unit of a program must agree on
// testing mode.
#if ( !defined(D_CFG_PARSE_MACHINE_TRACE) &&                                   \
      !defined(D_CFG_PARSE_ALL) )
    #define D_CFG_PARSE_MACHINE_TRACE 1
#endif

// 2.5    Test counter
//------------------------------------------------------------------------------
//   Read by cfg_test_counter.h, which also forces both gates on in a test
// build: a counter that neither clamps nor notifies is not the counter this
// framework ships, so a suite against one is not testing it.

// 2.5.1
// D_CFG_TEST_COUNTER_ENFORCE_BOUNDS
//   knob: clamp at the bounds.
#ifndef D_CFG_TEST_COUNTER_ENFORCE_BOUNDS
    #define D_CFG_TEST_COUNTER_ENFORCE_BOUNDS 1
#endif  // D_CFG_TEST_COUNTER_ENFORCE_BOUNDS

// 2.5.2
// D_CFG_TEST_COUNTER_EVENTS
//   knob: notify observers on change; a suite that asserts event order cannot
// run against a counter built without them.
#ifndef D_CFG_TEST_COUNTER_EVENTS
    #define D_CFG_TEST_COUNTER_EVENTS 1
#endif  // D_CFG_TEST_COUNTER_EVENTS

// 2.6    Parity oracle
//------------------------------------------------------------------------------
//   Read by cfg_test_parity.h.

// 2.6.1
// D_CFG_TEST_PARITY
//   knob: compile the parity oracle; it exists to be run in a test build.
// cfg_test_parity.h defaults it to D_CFG_TESTING, so it follows testing mode
// even when this preset is suppressed.
#ifndef D_CFG_TEST_PARITY
    #define D_CFG_TEST_PARITY 1
#endif  // D_CFG_TEST_PARITY

// 2.6.2
// D_CFG_TEST_PARITY_STRICT
//   knob: a truncated path or a refused sink fails the run. A record that
// silently lost rows compares equal to another that lost the same rows, so a
// truncation affecting both forks is invisible to a diff; only the recorder's
// own failure flag catches it, and this makes that flag fail the build.
#ifndef D_CFG_TEST_PARITY_STRICT
    #define D_CFG_TEST_PARITY_STRICT 1
#endif  // D_CFG_TEST_PARITY_STRICT


#endif  // D_INTERNAL_CFG_APPLYING_TESTING_PRESET
#endif  // DJINTERP_CONFIG_CFG_TESTING_H
