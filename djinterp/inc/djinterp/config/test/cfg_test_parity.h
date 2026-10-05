/*******************************************************************************
* djinterp [config]                                            cfg_test_parity.h
*
*   Build-time configuration for the parity oracle: what may appear in a parity
* record, and the sizes the kernel would otherwise hard-code.
*
*   targets:  test/test_parity_common.h -> D_INTERNAL_TEST_PARITY_*
*             test/test_parity.h        -> D_INTERNAL_TEST_PARITY_*
*             test/test_parity.hpp      -> D_INTERNAL_TEST_PARITY_*
*   requires: cfg_common.h (helpers, D_CFG_TESTING)
*
*   MOST OF THESE KNOBS NARROW WHAT MAY BE RECORDED, and that is the unusual
* thing about this file. Configuration normally adds capability; here the
* valuable direction is subtraction, because a parity record is only as useful
* as its determinism. A record that varies between two correct runs produces
* failures that are not bugs, the suite gets muted, and a muted oracle catches
* nothing for the rest of the project. Every knob below that turns something OFF
* exists so a project can reach a green, meaningful diff sooner rather than
* living with an amber one.
*
*   The grammar knobs (delimiter, escape, version) are here because they are a
* WIRE FORMAT. Two streams compared by `diff` must have been produced by builds
* that agree about all three, so they belong somewhere a build can pin them
* rather than in a header a reader might casually edit.
*
*
* path:      /inc/djinterp/config/test/cfg_test_parity.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.09.20
*******************************************************************************/

#ifndef DJINTERP_CONFIG_TEST_CFG_TEST_PARITY_H
#define DJINTERP_CONFIG_TEST_CFG_TEST_PARITY_H 1

// djinterp
// (0) root first.
#include "../cfg_common.h"

/*
TABLE OF CONTENTS
=================
0.    PARITY ORACLE CONFIGURATION
      ---------------------------
      1.    Master switch              (D_CFG_TEST_PARITY)
      2.    What may be recorded       (D_CFG_TEST_PARITY_RECORD_*)
      3.    Sizes                      (D_CFG_TEST_PARITY_PATH_MAX)
      4.    The wire grammar           (D_CFG_TEST_PARITY_DELIM, _ESCAPE, _VERSION)
      5.    Strictness                 (D_CFG_TEST_PARITY_STRICT)
      6.    Validation of the knobs
      7.    Derived values             (D_INTERNAL_TEST_PARITY_*)
*/


// ===========================================================================
// 0.   PARITY ORACLE CONFIGURATION
// ===========================================================================

// --- 0.1  Master switch ---

// D_CFG_TEST_PARITY
//   brief: compile the parity oracle at all. Defaults to D_CFG_TESTING, so a
// release build carries none of it and a test build carries all of it.
//   Set 1 explicitly to ship the oracle in a release build -- which is
// reasonable, since the record is also a diagnostic: a bug report that includes
// a parity dump says what the reporter's build actually computed.
#ifndef D_CFG_TEST_PARITY
#   define D_CFG_TEST_PARITY D_CFG_TESTING
#endif


// --- 0.2  What may be recorded ---
//   Each of these gates one observation KIND. Turning one off does not remove
// the rows -- it replaces them with `absent`, so both forks still emit the same
// row set and only the values differ. That distinction is what keeps a
// narrowed suite comparable to a full one.

// D_CFG_TEST_PARITY_RECORD_FLOAT
//   brief: record floating-point observations (1, the default).
//   Set 0 while the framework's float-parity question is open. `framework_goals`
// §13 does not yet settle whether the two forks must produce bit-identical
// floats, and until it does, a suite that records them is asserting an answer
// nobody has agreed to. The oracle writes floats in hexadecimal (`%a`) so that
// a failure means the ARITHMETIC differed and never that the printing did --
// which makes the question answerable, not answered.
//   With this off, float observations become `absent` rows carrying the reason,
// so the suite stays green and the gap stays visible.
#ifndef D_CFG_TEST_PARITY_RECORD_FLOAT
#   define D_CFG_TEST_PARITY_RECORD_FLOAT 1
#endif

// D_CFG_TEST_PARITY_RECORD_SIZE_WIDTH
//   brief: record the WIDTH of size_t alongside each size value (1, default).
//   ON is what lets one record be compared across data models: without the
// width, an LP64 and an ILP32 run differ at every large number for a reason
// that is not a bug, or -- worse -- agree at every small one while the type
// underneath silently differs. Turn it off only when both forks are known to
// share a data model and the extra column is noise.
#ifndef D_CFG_TEST_PARITY_RECORD_SIZE_WIDTH
#   define D_CFG_TEST_PARITY_RECORD_SIZE_WIDTH 1
#endif

// D_CFG_TEST_PARITY_RECORD_LAYOUT
//   brief: record sizeof / alignof / offsetof observations (1, the default).
//   These are the Layout law mechanised: asserting size and offset separately
// in each dialect proves each side self-consistent, and only a recorded
// comparison proves they AGREE. Turn it off for a suite that compares two
// builds of the SAME language, where layout cannot differ and the rows are
// noise.
#ifndef D_CFG_TEST_PARITY_RECORD_LAYOUT
#   define D_CFG_TEST_PARITY_RECORD_LAYOUT 1
#endif

// D_CFG_TEST_PARITY_RECORD_BYTES
//   brief: record byte-buffer observations as hex (1, the default).
//   These are the largest rows in a record by a wide margin -- a 4 KiB buffer
// becomes an 8 KiB line. Turn it off for a smoke suite that wants shapes and
// statuses rather than payloads; the byte rows become `absent` and the row set
// is unchanged.
#ifndef D_CFG_TEST_PARITY_RECORD_BYTES
#   define D_CFG_TEST_PARITY_RECORD_BYTES 1
#endif


// --- 0.3  Sizes ---

// D_CFG_TEST_PARITY_PATH_MAX
//   brief: the longest fully-qualified observation path a recorder will build,
// in bytes. Default 256.
//   Fixed rather than grown, because the oracle allocates nothing -- see the
// kernel's banner. A path that overflows is truncated AND MARKED rather than
// silently shortened, so raising this is a response to seeing the mark, not a
// precaution against it.
#ifndef D_CFG_TEST_PARITY_PATH_MAX
#   define D_CFG_TEST_PARITY_PATH_MAX 256
#endif


// --- 0.4  The wire grammar ---
//   Two streams compared by `diff` must agree about all three of these. They
// are configurable because a project embedding the record in another format
// may need a different delimiter -- not because they are a matter of taste.

// D_CFG_TEST_PARITY_DELIM
//   brief: the field delimiter, as a character literal. Default '|'.
//   It must be a byte the escape rule can quote, and it must not be a decimal
// digit, a hex digit, or '-' -- all of which appear unescaped inside encoded
// values.
#ifndef D_CFG_TEST_PARITY_DELIM
#   define D_CFG_TEST_PARITY_DELIM '|'
#endif

// D_CFG_TEST_PARITY_ESCAPE
//   brief: the escape byte, as a character literal. Default '\\'.
//   Exactly three sequences exist -- the delimiter, a newline, and the escape
// itself -- so the encoding is total and its inverse is obvious even though
// nothing in the framework computes it.
#ifndef D_CFG_TEST_PARITY_ESCAPE
#   define D_CFG_TEST_PARITY_ESCAPE '\\'
#endif

// D_CFG_TEST_PARITY_VERSION
//   brief: the grammar version, emitted as the first row of every stream.
// Default 1.
//   Two streams with different versions are not comparable, and emitting it
// first means `diff` says so on line 1 rather than producing a screenful of
// spurious differences. Bump it when the grammar changes, never for a content
// change.
#ifndef D_CFG_TEST_PARITY_VERSION
#   define D_CFG_TEST_PARITY_VERSION 1
#endif


// --- 0.5  Strictness ---

// D_CFG_TEST_PARITY_STRICT
//   brief: treat a truncated path or a refused sink as a hard failure of the
// run (1, the default) rather than a marked row (0).
//   ON is right for CI: a record that silently lost rows compares equal to
// another record that lost the same rows, so a truncation that affects both
// forks is invisible to `diff`. The recorder's own failure flag is what catches
// it, and STRICT is what makes that flag fail the build.
//   OFF is for interactive use, where a partial record is more useful than no
// record.
#ifndef D_CFG_TEST_PARITY_STRICT
#   define D_CFG_TEST_PARITY_STRICT 1
#endif


// --- 0.6  Validation of the knobs ---

#if !D_CFG_IS_BOOL(D_CFG_TEST_PARITY)
#   error "D_CFG_TEST_PARITY must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_TEST_PARITY_RECORD_FLOAT)
#   error "D_CFG_TEST_PARITY_RECORD_FLOAT must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_TEST_PARITY_RECORD_SIZE_WIDTH)
#   error "D_CFG_TEST_PARITY_RECORD_SIZE_WIDTH must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_TEST_PARITY_RECORD_LAYOUT)
#   error "D_CFG_TEST_PARITY_RECORD_LAYOUT must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_TEST_PARITY_RECORD_BYTES)
#   error "D_CFG_TEST_PARITY_RECORD_BYTES must be 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_TEST_PARITY_STRICT)
#   error "D_CFG_TEST_PARITY_STRICT must be 0 or 1"
#endif

#if ( D_CFG_NORM(D_CFG_TEST_PARITY_PATH_MAX) < 64 )
#   error "D_CFG_TEST_PARITY_PATH_MAX must be at least 64 bytes"
#endif
#if ( D_CFG_NORM(D_CFG_TEST_PARITY_VERSION) < 1 )
#   error "D_CFG_TEST_PARITY_VERSION must be at least 1"
#endif


// --- 0.7  Derived values (D_INTERNAL_*) ---

// D_INTERNAL_TEST_PARITY
//   brief: 1 when the oracle is compiled. Everything else in this file is
// meaningless when it is 0, and the kernel's declarations are gated on it.
#if D_CFG_IS_ON(D_CFG_TEST_PARITY)
#   define D_INTERNAL_TEST_PARITY 1
#else
#   define D_INTERNAL_TEST_PARITY 0
#endif

// D_INTERNAL_TEST_PARITY_FLOAT / _SIZE_WIDTH / _LAYOUT / _BYTES
//   brief: the four recording gates, strict 0/1 for use in `#if`. Each is
// ANDed with the master switch so a module tests one symbol.
#if ( D_INTERNAL_TEST_PARITY &&                                                \
      D_CFG_IS_ON(D_CFG_TEST_PARITY_RECORD_FLOAT) )
#   define D_INTERNAL_TEST_PARITY_FLOAT 1
#else
#   define D_INTERNAL_TEST_PARITY_FLOAT 0
#endif

#if ( D_INTERNAL_TEST_PARITY &&                                                \
      D_CFG_IS_ON(D_CFG_TEST_PARITY_RECORD_SIZE_WIDTH) )
#   define D_INTERNAL_TEST_PARITY_SIZE_WIDTH 1
#else
#   define D_INTERNAL_TEST_PARITY_SIZE_WIDTH 0
#endif

#if ( D_INTERNAL_TEST_PARITY &&                                                \
      D_CFG_IS_ON(D_CFG_TEST_PARITY_RECORD_LAYOUT) )
#   define D_INTERNAL_TEST_PARITY_LAYOUT 1
#else
#   define D_INTERNAL_TEST_PARITY_LAYOUT 0
#endif

#if ( D_INTERNAL_TEST_PARITY &&                                                \
      D_CFG_IS_ON(D_CFG_TEST_PARITY_RECORD_BYTES) )
#   define D_INTERNAL_TEST_PARITY_BYTES 1
#else
#   define D_INTERNAL_TEST_PARITY_BYTES 0
#endif

#if D_CFG_IS_ON(D_CFG_TEST_PARITY_STRICT)
#   define D_INTERNAL_TEST_PARITY_STRICT 1
#else
#   define D_INTERNAL_TEST_PARITY_STRICT 0
#endif

// D_INTERNAL_TEST_PARITY_PATH_MAX / _DELIM / _ESCAPE / _VERSION
//   brief: the sizes and grammar bytes, passed through after validation so the
// kernel reads one name apiece and carries no defaults of its own.
#define D_INTERNAL_TEST_PARITY_PATH_MAX D_CFG_TEST_PARITY_PATH_MAX
#define D_INTERNAL_TEST_PARITY_DELIM    D_CFG_TEST_PARITY_DELIM
#define D_INTERNAL_TEST_PARITY_ESCAPE   D_CFG_TEST_PARITY_ESCAPE
#define D_INTERNAL_TEST_PARITY_VERSION  D_CFG_TEST_PARITY_VERSION


#endif  // DJINTERP_CONFIG_TEST_CFG_TEST_PARITY_H
