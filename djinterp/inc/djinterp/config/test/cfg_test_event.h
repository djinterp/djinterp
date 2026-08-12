/******************************************************************************
* djinterp [config/test]                                    cfg_test_event.h    
*
*   Configuration for the DTest event module.
*
*   targets:  c/test/test_event.h
*   requires: cfg_common.h
*
*   ONE FILE, ONE MODULE, BOTH FORKS.  Granularity is the module, and after
* rev13 that is what the file set actually is: cfg_test.h had grown to serve
* four modules at once, which made it the thing the rule forbids at a larger
* scale than the sibling it was written to prevent.  There is deliberately no
* copy under config/test/ for the C++ fork -- a knob's value must be identical
* in both forks (dconfig 6.2), and two files, one per fork, would be that rule
* broken by directory structure rather than by anyone's decision.
*
*   THIS FILE DECLARES KNOBS.  IT DOES NOT DECLARE TYPES.  D_CFG_* originates
* here with a default and a validation block; the typedef that turns it into a
* type lives at the top of the module header that owns the type, where a reader
* looking for d_test_event_id finds it in the file that uses it.  A config header
* that also typedefs is a second place to look for one answer.
*
* path:      /inc/djinterp/config/c/test/cfg_test_event.h
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.11
******************************************************************************/

#ifndef DJINTERP_CONFIG_C_TEST_EVENT
#define DJINTERP_CONFIG_C_TEST_EVENT 1

#include "../../cfg_common.h"


// =============================================================================
// I.   USER KNOBS
// =============================================================================

// D_CFG_TEST_EVENT_ID_TYPE
//   knob: type of d_test_event_id. Must be UNSIGNED integral.
//
//   DEFAULT uint64_t, AND THAT IS A DELIBERATE CHANGE.  The type was size_t,
// so its width followed the data model: 8 bytes on LP64, 4 on ILP32. That is
// the platform-varying layout 6.2 forbids, inside a struct two forks must
// agree on byte for byte. uint64_t keeps the common case identical and makes
// the 32-bit case a stated choice rather than a compiler property.
//   ON A 32-BIT TARGET THIS WIDENS THE TYPE. Set uint32_t for the old size.
#ifndef D_CFG_TEST_EVENT_ID_TYPE
#   define D_CFG_TEST_EVENT_ID_TYPE uint64_t
#endif

//   D_CFG_TEST_EVENT_VALIDATE_PAYLOAD IS GONE, DELETED RATHER THAN LEFT.  It
// gated a fire-site check that a payload's slots matched the signature table.
// rev14 replaced the slots with an opaque context, so there is no shape left to
// disagree with and the branch it guarded no longer exists.  A knob normalised
// into an internal symbol that nothing reads compiles clean in every setting
// and reads as supported -- which is what rev08 found for
// D_CFG_EVENT_TABLE_ORDERED_ITERATION and rev11 for
// D_CFG_TEST_EVENT_STABLE_ORDER.  Two deletions on that argument make it the
// convention rather than a one-off, so this one went with the branch instead of
// outliving it by a revision.

// D_CFG_TEST_EVENT_MAX_LISTENERS
//   brief: the advisory ceiling on one dispatch table's listener count.  The
// table's real bound is the capacity the caller binds; this is the number a
// caller sizing an array with no better information should use, and it is
// recorded so that two forks configured differently are visibly different
// rather than silently so.
//   ADVISORY, LIKE D_CFG_TEST_METADATA_MAX, so the kernel reading it nowhere
// is the design rather than a defect -- it is a number for CALLERS.  That is
// the one distinction inert_knobs.py cannot draw on its own, and it is why the
// knob below went and this one stayed.
#ifndef D_CFG_TEST_EVENT_MAX_LISTENERS
#   define D_CFG_TEST_EVENT_MAX_LISTENERS 64
#endif

//   D_CFG_TEST_EVENT_STABLE_ORDER IS GONE, AND WAS DELETED RATHER THAN WIRED.
// It promised that listeners for one kind fire in bind order, was validated,
// and normalised into D_INTERNAL_TEST_EVENT_STABLE_ORDER -- which the kernel
// never read.  So the off path compiled clean under -Werror, produced
// byte-identical object code, and reported as supported.
//   The operation it offered to switch off is unconditional and load-bearing:
// test_event.h's banner states bind order as a CONTRACT, on the grounds that an
// event stream is comparable across forks only if its order is a function of
// its inputs.  A knob offering to disable the thing the parity law rests on is
// worth less than no knob, and the danger is a future session wiring it as a
// tidy-up.  This is rev08's finding for D_CFG_EVENT_TABLE_ORDERED_ITERATION,
// same shape, and here it was in scope because the merge forced a decision on
// every knob in the file.
//   DELETING IT CHANGED NO BEHAVIOUR, because it never had any: a build
// setting it to 0 was already being ignored.


// =============================================================================
// II.  VALIDATION
// =============================================================================

#if ( D_CFG_NORM(D_CFG_TEST_EVENT_MAX_LISTENERS) < 1 )
#   error "D_CFG_TEST_EVENT_MAX_LISTENERS must be at least 1"
#endif

// =============================================================================
// III. RESOLVED
// =============================================================================
//   What the module reads.  A test build forces the checks ON regardless of the
// user knob, for the same reason cfg_test_counter.h forces its bounds check:
// the oracle's whole job is to catch the case the knob would hide.

#define D_INTERNAL_TEST_EVENT_MAX_LISTENERS D_CFG_TEST_EVENT_MAX_LISTENERS

//   NOTHING IS FORCED ON UNDER D_CFG_TESTING HERE, because there is nothing
// left to force.  The testing preset used to override VALIDATE_PAYLOAD on the
// grounds that the oracle's job is to catch what a knob would hide; with the
// check gone the preset has no opinion about this module.


#endif  // DJINTERP_CONFIG_C_TEST_EVENT
