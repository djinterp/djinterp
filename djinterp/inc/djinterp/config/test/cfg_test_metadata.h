/******************************************************************************
* djinterp [config/test]                                    cfg_test_metadata.h 
*
*   Configuration for the DTest metadata module.
*
*   targets:  c/test/test_metadata.h
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
* looking for d_test_key_id finds it in the file that uses it.  A config header
* that also typedefs is a second place to look for one answer.
*
* path:      /inc/djinterp/config/c/test/cfg_test_metadata.h
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.11
******************************************************************************/

#ifndef DJINTERP_CONFIG_C_TEST_METADATA
#define DJINTERP_CONFIG_C_TEST_METADATA 1

#include "../../cfg_common.h"


// =============================================================================
// I.   USER KNOBS
// =============================================================================

// D_CFG_TEST_KEY_ID_TYPE
//   knob: type of d_test_key_id, the metadata key. Must be UNSIGNED integral.
//
//   THE KEY IS AN INTERNED ID, NOT A STRING.  It was const char* and compared
// with strcmp; it is now a number compared with ==. Lookup stops depending on
// string content, so a key cannot be misspelled at a call site and silently
// miss, and the container no longer borrows key storage it does not own.
//   Zero is reserved as D_TEST_NO_KEY, the same convention D_TEST_NO_CALLABLE
// already uses for the other handle in this tier.
#ifndef D_CFG_TEST_KEY_ID_TYPE
#   define D_CFG_TEST_KEY_ID_TYPE uint32_t
#endif

// D_CFG_TEST_METADATA
//   brief: whether the metadata module is compiled at all.  On by default.
//   NEW IN THIS PASS, and the node's member is a pointer that may be null
// (revision.md §6), so a build with this off simply never points it anywhere.
// This gate is C-ONLY. In C++ metadata is mandatory: test_object<> holds its
// container BY VALUE as a default template argument, which must name a complete
// type, so test_object.hpp includes test_metadata.hpp unconditionally. Making
// it conditional would change what basic_test IS under a knob -- the one-layout
// hazard reappearing as template identity.
//   UNCOVERED. No gate in this package exercises the off path; see the log.
#ifndef D_CFG_TEST_METADATA
#   define D_CFG_TEST_METADATA 1
#endif

// D_CFG_TEST_METADATA_REPLACE
//   brief: set() on an existing key replaces the row in place.  On by
// default.  Off appends, which makes lookup depend on scan direction -- so
// off is a compatibility escape hatch and not a supported configuration for a
// build the oracle runs against.
//   UNCOVERED. The differential harness runs with this ON only; whether the off
// path is supported or vestigial is still unrecorded.
#ifndef D_CFG_TEST_METADATA_REPLACE
#   define D_CFG_TEST_METADATA_REPLACE 1
#endif

// D_CFG_TEST_METADATA_MAX
//   brief: the advisory row count for a caller sizing an array with no better
// information.  The real bound is the capacity the caller binds.
#ifndef D_CFG_TEST_METADATA_MAX
#   define D_CFG_TEST_METADATA_MAX 8
#endif


// =============================================================================
// II.  VALIDATION
// =============================================================================

#if !D_CFG_IS_BOOL(D_CFG_TEST_METADATA)
#   error "D_CFG_TEST_METADATA must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_TEST_METADATA_REPLACE)
#   error "D_CFG_TEST_METADATA_REPLACE must be 0 or 1"
#endif
#if ( D_CFG_NORM(D_CFG_TEST_METADATA_MAX) < 1 )
#   error "D_CFG_TEST_METADATA_MAX must be at least 1"
#endif

// =============================================================================
// III. RESOLVED
// =============================================================================
//   What the module reads.  A test build forces the checks ON regardless of the
// user knob, for the same reason cfg_test_counter.h forces its bounds check:
// the oracle's whole job is to catch the case the knob would hide.

#define D_INTERNAL_TEST_METADATA_MAX    D_CFG_TEST_METADATA_MAX

#if ( D_CFG_IS_ON(D_CFG_TESTING) || D_CFG_IS_ON(D_CFG_TEST_METADATA) )
#   define D_INTERNAL_TEST_METADATA 1
#else
#   define D_INTERNAL_TEST_METADATA 0
#endif

#if ( D_CFG_IS_ON(D_CFG_TESTING) ||                                            \
      D_CFG_IS_ON(D_CFG_TEST_METADATA_REPLACE) )
#   define D_INTERNAL_TEST_METADATA_REPLACE 1
#else
#   define D_INTERNAL_TEST_METADATA_REPLACE 0
#endif

//   THE OFF PATH IS UNREACHABLE IN THE BUILD THAT MATTERS, and saying so is
// the point of this note.  D_INTERNAL_TEST_METADATA is (TESTING || METADATA),
// so under D_TESTING=1 the knob reads 0 and the module is still compiled in --
// knob_probe.c prints exactly that.  The forcing matches cfg_test_counter.h's
// convention and is intended; what was missing was anywhere that said so, and
// rev07 recorded the off path as tested when it had been tested in a build the
// oracle does not use.

#endif  // DJINTERP_CONFIG_C_TEST_METADATA
