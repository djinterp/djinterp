/******************************************************************************
* djinterp [test]                                         test_common.h
*
*   The DTest vocabulary: the status and the two identifier types that the rest
* of the tier speaks.  Compiled by BOTH C and C++.
*
*   EVENT-AGNOSTIC BY DESIGN.  d_test_event, d_test_event_id,
* D_TEST_EVENT_INIT and d_test_event_make used to live here; they are now in
* c/test/test_event.h.  Nothing in this file mentions an event, and nothing
* added to it should: a module that wants the status vocabulary should not have
* to take the event record with it.  test_event.h includes THIS header, not the
* other way round.
*
*
* THE ONE HARD PROBLEM HERE IS `enum class`
* =========================================
*   test_status is a C++ SCOPED enum -- `test_status::passed` -- and C has no
* such thing.  Three ways to handle it, and only one of them is honest.
*
*   WRONG: give C a plain enum with the same enumerator names and call it done.
* Two independent declarations of one value set, free to drift the moment
* someone adds an enumerator to one side.
*
*   WRONG: demote the C++ side to a plain enum plus constants, as compress.hpp's
* `status` did.  That works there because `status_ok` was ALREADY the spelling;
* here it would rewrite `test_status::passed` at every call site in eleven
* modules, to make C's limitation the shared vocabulary.  Goals §11 is explicit
* that C does not get C++'s type-level expressiveness -- it does not say C++
* gives up its own.
*
*   RIGHT: the VALUES are written once, as macros.  Each language then declares
* the enum in the form it can express, and both declarations read the same
* macros.  There is one definition of the value set and two notations over it,
* which is precisely what "notation is not semantics" means.  Adding an
* enumerator means adding a macro, and a language that forgets to name it fails
* to compile rather than diverging.
*
*
* WIDTHS COME FROM THE CONFIG, NOT FROM THIS FILE
* ===============================================
*   d_test_type_id and d_test_callable_id are set by D_CFG_TEST_TYPE_ID_TYPE
* and D_CFG_TEST_CALLABLE_ID_TYPE, which NAME THE TYPE rather than a width, and
* default to int32_t and uint32_t -- what they were when written out longhand.
*
*   Each typedef asserts what the knob must satisfy: signedness, and a width of
* 1, 2, 4 or 8 bytes. A knob that names a type cannot be checked against a
* declared bit count the way a width knob can, so the assertions check the
* PROPERTIES the code depends on instead. dconfig 6.1 wants a build-time
* failure on misconfiguration; this is the form that gives one here.
*
*   CODE NAMES THE TYPEDEF, NEVER THE UNDERLYING TYPE.  Writing int32_t where
* d_test_type_id is meant compiles today and breaks silently the moment the
* width is reconfigured. That is the one thing the mechanism exists to prevent.
*
*   AN EARLIER BANNER ARGUED THE OPPOSITE for the event id, and it is worth
* saying why it was superseded rather than letting it look like an oversight.
* It read: left as size_t rather than pinned, "because pinning it would change
* the existing surface for a benefit the parity oracle already provides" --
* d_test_observe_size records a width, so a data-model difference shows up as a
* visible diff. True, but it makes the parity harness responsible for catching
* a layout that dconfig 6.2 says must not vary in the first place. Observing a
* hazard is weaker than not having one. The event id now lives in test_event.h
* and takes its type from D_CFG_TEST_EVENT_ID_TYPE.
*
*
* WHY THE MESSAGE FIELD STAYS A const char*
* ===============================
*   The archive layer uses d_pack_text -- pointer plus length -- because a ZIP
* comment may contain a NUL.  An event message may not: it is a literal or a
* static string, it is never sliced, and every one of the eleven consumers
* treats it as a C string today.  Changing it would be a surface change with no
* failure mode behind it.  The convention is not universal, and this is the
* place to say which side of it this module sits on.
*
*
* path:      /inc/djinterp/c/test/test_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.30
******************************************************************************/

#ifndef DJINTERP_C_TEST_COMMON
#define DJINTERP_C_TEST_COMMON 1

// c
#include <stddef.h>
#include <stdint.h>
// djinterp
#include "../djinterp.h"
#include "../../config/c/test/cfg_test_object.h"   // D_CFG_TEST_*_ID_TYPE
//   NO CONFIG INCLUDE, DELIBERATELY. D_CFG_TEST_COMMON_SKIP_IS_FAILURE was the
// only knob this module ever had, and dropping it (revision.md §3) left
// cfg_test_common.h with nothing to say, so the file is deleted rather than
// kept empty. This module is now unconfigurable, which is the point: a value
// set and a record are not things a build should be able to disagree about.



// =============================================================================
// I.   STATUS VALUES
// =============================================================================
//   The value set, written ONCE.  Both enum declarations below read these, so
// the two notations cannot drift.  Values are pinned because a status is
// recorded in a parity stream and passed across the DTest bridge.

#define D_TEST_STATUS_PASSED    0
#define D_TEST_STATUS_FAILED    1
#define D_TEST_STATUS_SKIPPED   2
#define D_TEST_STATUS_PENDING   3
#define D_TEST_STATUS_ERROR     4

// D_TEST_STATUS_COUNT
//   constant: how many statuses exist.  A switch over the set that forgets one
// is caught by asserting this against the highest enumerator.
#define D_TEST_STATUS_COUNT     5

//   THE SCOPED C++ ENUM IS NOT HERE.  It moved to test_common.hpp in
// revision.md §2's language split, along with the callable-id alias below.
// Both used to sit in this file behind `#ifdef __cplusplus`; C declarations
// belong under c/test/ and C++ declarations under test/, and a header that
// carried both was the rule broken in place.
//   The five macros above remain the ONE value set, and the C++ enum reads
// them from here, so the two notations still cannot drift.


//   THE INCLUDE ABOVE IS LOAD-BEARING, NOT CONVENTIONAL.  This header used to
// open with
//
//     #if !defined(D_EXTERN_C_BEGIN)
//     #   error "<this file> needs D_EXTERN_C_BEGIN; define it or set ..."
//     #endif
//
// and revision.md §8 removed it: the macro is assumed present-or-empty.  That
// trades a compile-time diagnostic naming this file for a link error naming
// nothing, so djinterp.h must stay MANDATORY and FIRST here.  Moving it, or
// letting it become conditional, is how a caller ends up debugging a missing
// symbol three headers away from the cause.
//
//   THE BLOCK FORM IS KEPT.  revision.md §8 also says to spell the opener
// D_EXTERN_C and leave D_EXTERN_C_END unchanged.  That pairing does not
// compile: djinterp.h defines D_EXTERN_C as a linkage specification on ONE
// declaration and D_EXTERN_C_BEGIN/_END as the block form, and flags the
// asymmetry itself -- "`extern "C" { ... }` is a linkage block, `extern "C"
// decl;` is a linkage specification on one declaration."  Swapping only the
// opener leaves `extern "C"` applied to the next declaration and an unmatched
// `}` at the end of the file.  The guard removal -- which is what §8 is
// actually for -- is done; the token swap is not.  See the log.

D_EXTERN_C_BEGIN

//   C gets the unscoped form.  It is the SAME value set -- both declarations
// expand the same five macros -- and it is what the shared kernel's functions
// take, so a C++ caller converts once at the boundary rather than the kernel
// carrying two signatures.
enum d_test_status
{
    D_TEST_STATUS_PASSED_E  = D_TEST_STATUS_PASSED,
    D_TEST_STATUS_FAILED_E  = D_TEST_STATUS_FAILED,
    D_TEST_STATUS_SKIPPED_E = D_TEST_STATUS_SKIPPED,
    D_TEST_STATUS_PENDING_E = D_TEST_STATUS_PENDING,
    D_TEST_STATUS_ERROR_E   = D_TEST_STATUS_ERROR
};

D_STATIC_ASSERT((D_TEST_STATUS_ERROR + 1) == D_TEST_STATUS_COUNT,
                "d_test_status: the value set is not dense");


// =============================================================================
// II.  IDENTIFIERS
// =============================================================================

// d_test_type_id
//   type: a node's STRUCTURAL type identity -- the key into its test_kind
// definition.  Not a per-instance unique id: a node carries neither a unique id
// nor a depth, because both are facts of position that the owning tree confers
// during the walk.
typedef D_CFG_TEST_TYPE_ID_TYPE d_test_type_id;

D_STATIC_ASSERT(((d_test_type_id)-1) < 0,
                "d_test_type_id: D_CFG_TEST_TYPE_ID_TYPE must name an "
                "integer type");
D_STATIC_ASSERT((sizeof(d_test_type_id) == 1) ||
                (sizeof(d_test_type_id) == 2) ||
                (sizeof(d_test_type_id) == 4) || (sizeof(d_test_type_id) == 8),
                "d_test_type_id: width must be 1, 2, 4 or 8 bytes");

// d_test_callable_id
//   type: an opaque, non-owning handle into a callable table.  Keeps a test
// node trivially copyable while still letting a leaf carry deferred work.
//   Zero is reserved: a node carrying 0 is fully evaluated and the value
// already in it is authoritative; a node carrying non-zero is deferred and the
// handler invokes the callable before firing per-test events.
typedef D_CFG_TEST_CALLABLE_ID_TYPE d_test_callable_id;

D_STATIC_ASSERT(((d_test_callable_id)-1) > 0,
                "d_test_callable_id: D_CFG_TEST_CALLABLE_ID_TYPE must "
                "name an unsigned integer type");
D_STATIC_ASSERT((sizeof(d_test_callable_id) == 1) ||
                (sizeof(d_test_callable_id) == 2) ||
                (sizeof(d_test_callable_id) == 4) ||
                (sizeof(d_test_callable_id) == 8),
                "d_test_callable_id: width must be 1, 2, 4 or 8 bytes");

// D_TEST_NO_CALLABLE
//   constant: the reserved zero handle. Equivalent to writing 0, and clearer.
#define D_TEST_NO_CALLABLE  ((d_test_callable_id)0)

//   THE C++ ALIAS `test_callable_id` AND `k_no_callable` are in
// test_common.hpp, not here -- same split as the enum above.

// =============================================================================
// III. OPERATIONS
// =============================================================================
//   SPELLED D_STATIC_INLINE, NOT `D_STATIC D_INLINE`.  revision.md §3 says the
// latter; in standard C that expands to `static static inline` and does not
// compile, because D_INLINE already carries the `static` a C header inline must
// have.  djinterp.h anticipates exactly this and provides D_STATIC_INLINE
// "composed as exactly one `static` plus the inline specifier ... no
// double-`static`".  Same intent, the spelling the macro kit actually supplies.
//
//   HEADER-ONLY, AND THIS INVERTS HOUSE STYLE §10 ON PURPOSE.  Normally the
// header carries the reasoning and the `.c` the arithmetic; there is no `.c`
// for this module any more, so both live here.  That is an accepted exception
// recorded in revision.md §3, not an oversight -- all four of these are two to
// four lines and the translation unit that held them did nothing else.
//
//   WHY THE KNOB HAD TO GO FIRST.  d_test_status_worse_of was the one function
// that could not safely inline while its rank table was chosen by the
// preprocessor: as `static inline` in a header, every translation unit compiles
// its own copy against whatever the config said IN THAT UNIT, and `static`
// means internal linkage, so there is no ODR violation to diagnose and no
// duplicate symbol to trip the linker. Two units would simply disagree, for
// good, about what a rolled-up result means. Dropping the knob removes the
// hazard instead of managing it.
//
//   BOTH TABLES ARE `static const` LOCALS rather than file-scope objects, so
// they do not sit at file scope in every translation unit that includes this
// header and cannot warn as unused in one that calls neither function.

// d_test_status_is_valid
//   Whether the value names one of the five statuses.
D_STATIC_INLINE int
d_test_status_is_valid(int32_t _status)
{
    return ( (_status >= 0) && (_status < D_TEST_STATUS_COUNT) ) ? 1 : 0;
}

// d_test_status_or_error
//   The value if it names a status, `error` otherwise.
//
//   THE COERCION RULE, AND IT LIVES HERE BECAUSE IT IS A PROPERTY OF THE TYPE,
// not of any one module that stores one.  Three modules apply it -- the event
// record, the event status-change arguments, and the node's set_status -- and
// each used to spell it out at its own call site with its own comment.  Two
// records of "what happened to this node" that disagreed about what an
// out-of-range status means would be a divergence between two of DTest's own
// modules, which the oracle cannot see because both are on the same side of it.
// One inline function in the module that owns d_test_status is the cheapest
// place for them not to disagree.
D_STATIC_INLINE int32_t
d_test_status_or_error(int32_t _status)
{
    return d_test_status_is_valid(_status) ? _status
                                           : (int32_t)D_TEST_STATUS_ERROR;
}

/*
d_test_status_is_terminal
  Whether a status is an outcome rather than a state on the way to one.

  `pending` is the only non-terminal value: a node in it has not been
evaluated. `skipped` IS terminal -- the decision not to run is itself an
outcome, and treating it as pending would make a skipped suite look unfinished.

Parameter(s):
  _status: the status to classify.
Return:
  1 when the status is an outcome, 0 for pending or an invalid value.
*/
D_STATIC_INLINE int
d_test_status_is_terminal(int32_t _status)
{
    if (!d_test_status_is_valid(_status))
    {
        return 0;
    }

    return (_status != D_TEST_STATUS_PENDING) ? 1 : 0;
}

/*
d_test_status_name
  The status's own spelling.  A parity record carries the NAME, never the
integer, so a renumbering cannot pass unnoticed on one side.

Parameter(s):
  _status: the status to name.
Return:
  A static, NUL-terminated name, or "unknown".
*/
D_STATIC_INLINE const char*
d_test_status_name(int32_t _status)
{
    static const char* const k_status[D_TEST_STATUS_COUNT] =
    { "passed", "failed", "skipped", "pending", "error" };

    return ( (_status < 0) || (_status >= D_TEST_STATUS_COUNT) )
         ? "unknown" : k_status[_status];
}

/*
d_test_status_worse_of
  Combine two statuses into the one a parent should carry.

  THIS IS WHERE A TREE'S RESULT COMES FROM, so the ordering is a semantic
decision rather than a convenience. Severity runs:

      error > failed > skipped > pending > passed

  `error` outranks `failed` because the two mean different things: a failure is
a test that ran and disagreed, an error is a test that could not run. A suite
reporting "3 failed" when one of them never executed is telling the reader
something untrue.

  `pending` outranks `passed` so that a partly-evaluated tree never rolls up as
passed. A parent whose children are half unevaluated is pending, not green --
which is the whole reason this function is not just a max over the enum values.

  `skipped` sits below `failed`, always. This is FIXED, not configurable; see
the section banner above for why the knob that used to move it could not
survive this function becoming inline.

Parameter(s):
  _a: the first status.
  _b: the second.
Return:
  The more severe of the two.  An invalid input yields `error`, because a
status nobody can classify is exactly the case error exists for.
*/
D_STATIC_INLINE int32_t
d_test_status_worse_of(int32_t _a, int32_t _b)
{
    /* severity rank, indexed by status value. higher is worse. */
    static const int k_rank[D_TEST_STATUS_COUNT] =
    {
        0,      /* passed  */
        3,      /* failed  */
        2,      /* skipped */
        1,      /* pending */
        4       /* error   */
    };

    if ( (!d_test_status_is_valid(_a)) ||
         (!d_test_status_is_valid(_b)) )
    {
        return D_TEST_STATUS_ERROR;
    }

    return (k_rank[_a] >= k_rank[_b]) ? _a : _b;
}



D_EXTERN_C_END


#endif  // DJINTERP_C_TEST_COMMON
