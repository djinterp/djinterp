/*******************************************************************************
* djinterp [config]                                                cfg_testing.h
*
*   The testing/debug preset: the ONE visible home for "what differs in a test
* build".  Auto-applied by cfg_common.h when D_CFG_TESTING is 1.
*
*   targets:  no single module -- infrastructure
*   requires: nothing; cfg_common.h includes this AFTER resolving
*             D_CFG_TESTING and BEFORE any module default runs
*
*   PRIORITY: user overrides > this preset > module defaults.  Every knob below
* is #ifndef-guarded, so a value the user set on the command line or in
* cfg_custom.h still wins.  Suppress the whole file with
* -DD_CFG_NO_TESTING_PRESET.
*
*   THIS FILE WAS MISSING.  cfg_common.h has always included it under
* D_CFG_TESTING, so every build with -DD_TESTING=1 failed to preprocess.  It is
* reconstructed here from the contract cfg_common.h and the dconfig README
* state, and each block below is labelled with the subframework it serves --
* which is the convention the README asks for.
*
*   A MODULE'S DEFAULTS DO NOT BELONG HERE.  Only the values that DIFFER in a
* test build.  A knob whose test value equals its normal value should be absent
* from this file entirely, or the two homes drift.
*
*
* path:      /inc/djinterp/config/test/cfg_testing.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_CONFIG_TEST_CFG_TESTING_H
#define DJINTERP_CONFIG_TEST_CFG_TESTING_H 1

//   No include of cfg_common.h here: this file is included BY it, part-way
// through its own cascade, so including it back would be a cycle that the
// guard would silently truncate at the wrong point.


// --- parity oracle ---------------------------------------------------------
//   The oracle exists to be run in a test build, so it is on by default there.

// D_CFG_TEST_PARITY
//   brief: compile the parity oracle. On in a test build.
#ifndef D_CFG_TEST_PARITY
#   define D_CFG_TEST_PARITY 1
#endif

// D_CFG_TEST_PARITY_STRICT
//   brief: a truncated path or a refused sink fails the run. On in a test
// build, because a record that silently lost rows compares EQUAL to another
// record that lost the same rows -- so a truncation affecting both forks is
// invisible to diff, and only the recorder's own failure flag catches it.
#ifndef D_CFG_TEST_PARITY_STRICT
#   define D_CFG_TEST_PARITY_STRICT 1
#endif


// --- compression / archive -------------------------------------------------
//   The determinism knobs are ALSO forced on inside cfg_compress.h and
// cfg_archive.h, which is deliberate belt-and-braces: setting them here makes
// the intent visible in the one place a reader looks for "what differs in a
// test build", and the fold there makes it true even if this preset is
// suppressed with D_CFG_NO_TESTING_PRESET.

// D_CFG_COMPRESS_PIN_THREADS
//   brief: pin every codec to one worker. On in a test build: a multi-threaded
// encoder partitions by worker count, so a parity suite run on machines with
// different core counts would diverge for a reason that is not a bug.
#ifndef D_CFG_COMPRESS_PIN_THREADS
#   define D_CFG_COMPRESS_PIN_THREADS 1
#endif

// D_CFG_COMPRESS_PIN_DEFAULTS
//   brief: resolve unspecified knobs from the core's table rather than the
// backend's. On in a test build, so two builds linking different library
// VERSIONS still produce identical bytes.
#ifndef D_CFG_COMPRESS_PIN_DEFAULTS
#   define D_CFG_COMPRESS_PIN_DEFAULTS 1
#endif

// D_CFG_COMPRESS_VALIDATE
//   brief: range-check option knobs. On in a test build, so an out-of-range
// knob is reported by NAME rather than surfacing as a backend error.
#ifndef D_CFG_COMPRESS_VALIDATE
#   define D_CFG_COMPRESS_VALIDATE 1
#endif

// D_CFG_ARCHIVE_REPRODUCIBLE
//   brief: an entry with no mtime records the pinned epoch, not the clock. On
// in a test build: a container embedding "now" cannot be compared to anything.
#ifndef D_CFG_ARCHIVE_REPRODUCIBLE
#   define D_CFG_ARCHIVE_REPRODUCIBLE 1
#endif

// D_CFG_ARCHIVE_PREFER_BUILTIN
//   brief: use the dependency-free tar and ZIP writers even where a library
// exists. On in a test build: a parity suite against library-written containers
// measures the library's version, not this framework.
#ifndef D_CFG_ARCHIVE_PREFER_BUILTIN
#   define D_CFG_ARCHIVE_PREFER_BUILTIN 1
#endif

// D_CFG_ARCHIVE_CHECK_ENTRY_NAMES
//   brief: enforce the path-traversal guard. On in a test build, because a
// suite that skips the check cannot assert it.
#ifndef D_CFG_ARCHIVE_CHECK_ENTRY_NAMES
#   define D_CFG_ARCHIVE_CHECK_ENTRY_NAMES 1
#endif


// --- test counter ----------------------------------------------------------

// D_CFG_TEST_COUNTER_ENFORCE_BOUNDS / _EVENTS
//   brief: clamp at the bounds, and notify observers. Both on in a test build:
// a counter that neither clamps nor notifies is not the counter this framework
// ships, so a suite against one is not testing it.
#ifndef D_CFG_TEST_COUNTER_ENFORCE_BOUNDS
#   define D_CFG_TEST_COUNTER_ENFORCE_BOUNDS 1
#endif
#ifndef D_CFG_TEST_COUNTER_EVENTS
#   define D_CFG_TEST_COUNTER_EVENTS 1
#endif


#endif  // DJINTERP_CONFIG_TEST_CFG_TESTING_H
