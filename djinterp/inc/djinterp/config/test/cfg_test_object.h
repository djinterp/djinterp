/*******************************************************************************
* djinterp [config]                                            cfg_test_object.h
*
*   Configuration for the DTest node module.
*
*   targets:  c/test/test_object.h, c/test/test_common.h
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
* looking for d_test_status, d_test_type_id or d_test_callable_id finds it in
* the file that uses it.  A config header that also typedefs is a second place
* to look for one answer.
*
*   THE THREE TYPE KNOBS ARE VALIDATED BY STATIC ASSERTION, NOT BY `#error`.
* A knob that NAMES a type cannot be checked by the preprocessor -- there is no
* directive that can ask whether `int32_t` is signed.  The boolean knob below
* gets its `#error` in section II; the three type knobs get signedness and
* width assertions in test_common.h, beside the typedefs they produce.
*
*
* path:      /inc/djinterp/config/test/cfg_test_object.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.11
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_CONFIG_TEST_CFG_TEST_OBJECT_H
#define DJINTERP_CONFIG_TEST_CFG_TEST_OBJECT_H 1

// djinterp
#include "../cfg_common.h"


// =============================================================================
// I.   USER KNOBS
// =============================================================================

// D_CFG_TEST_STATUS_TYPE
//   knob: type of d_test_status. Must be SIGNED integral -- the value set is
// dense from 0 and d_test_status_is_valid rejects on `< 0`, which an unsigned
// type cannot express.
//   Default int32_t -- what the field was before it was configurable.
#ifndef D_CFG_TEST_STATUS_TYPE
#   define D_CFG_TEST_STATUS_TYPE int32_t
#endif

// D_CFG_TEST_TYPE_ID_TYPE
//   knob: type of d_test_type_id. Must be SIGNED integral.
//   Default int32_t -- what the type was before it was configurable.
#ifndef D_CFG_TEST_TYPE_ID_TYPE
#   define D_CFG_TEST_TYPE_ID_TYPE int32_t
#endif

// D_CFG_TEST_CALLABLE_ID_TYPE
//   knob: type of d_test_callable_id. Must be UNSIGNED integral.
//   Default uint32_t. Zero stays reserved as D_TEST_NO_CALLABLE at any width.
#ifndef D_CFG_TEST_CALLABLE_ID_TYPE
#   define D_CFG_TEST_CALLABLE_ID_TYPE uint32_t
#endif

// D_CFG_TEST_OBJECT_VALIDATE_STATUS
//   brief: an out-of-range status handed to set_status is stored as ERROR
// rather than passed through.  On by default, and it is the same rule
// d_test_event_make and d_test_event_status_change already apply -- three
// modules agreeing about what an invalid status means, which is a thing to
// state once and read three times rather than to reimplement.
#ifndef D_CFG_TEST_OBJECT_VALIDATE_STATUS
#   define D_CFG_TEST_OBJECT_VALIDATE_STATUS 1
#endif


// =============================================================================
// II.  VALIDATION
// =============================================================================

#if !D_CFG_IS_BOOL(D_CFG_TEST_OBJECT_VALIDATE_STATUS)
#   error "D_CFG_TEST_OBJECT_VALIDATE_STATUS must be 0 or 1"
#endif

// =============================================================================
// III. RESOLVED
// =============================================================================
//   What the module reads.  A test build forces the checks ON regardless of the
// user knob, for the same reason cfg_test_counter.h forces its bounds check:
// the oracle's whole job is to catch the case the knob would hide.

#if ( D_CFG_IS_ON(D_CFG_TESTING) ||                                            \
      D_CFG_IS_ON(D_CFG_TEST_OBJECT_VALIDATE_STATUS) )
#   define D_INTERNAL_TEST_OBJECT_VALIDATE_STATUS 1
#else
#   define D_INTERNAL_TEST_OBJECT_VALIDATE_STATUS 0
#endif

#endif  // DJINTERP_CONFIG_TEST_CFG_TEST_OBJECT_H
