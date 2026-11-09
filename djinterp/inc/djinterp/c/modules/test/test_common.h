/*******************************************************************************
* djinterp [test][c]                                               test_common.h
*
*   This module defines the DTest vocabulary common to both the C and C++ 
* implementations. Specifies the status (`dj_test_status`) and two identifier 
* types (`dj_test_type_id` and `dj_test_callable_id`), as defined by the 
* `D_CFG_*` variables in `cfg_test_object.h`. Compiles in both C and/or C++ (or
* both, in the case of the latter.
* 
* Notes
* =====
*  1. `dj_test_status` is a C++ SCOPED enum (`test_status::passed`) but C has no
*     equivalent. 
*     - Therefore, the VALUES are written once, as macros.  Each language then 
*       declares the enum in the form it can express; both declarations read 
*       the same macros. 
*  2. event-agnostic by design
*     - included by `test_event.h` but not the other way around.
*  3. widths come from the config (not from this file) 
*     
*     AN EARLIER BANNER ARGUED THE OPPOSITE for the event id, and it is worth
* saying why it was superseded rather than letting it look like an oversight.
* It read: left as size_t rather than pinned, "because pinning it would change
* the existing surface for a benefit the parity oracle already provides" --
* d_test_observe_size records a width, so a data-model difference shows up as a
* visible diff. True, but it makes the parity harness responsible for catching
* a layout that dconfig 6.2 says must not vary in the first place. Observing a
* hazard is weaker than not having one. The event id now lives in test_event.h
* and takes its type from D_CFG_TEST_EVENT_ID_TYPE.
*
* WHY THE MESSAGE FIELD STAYS A const char*
* =========================================
*   The archive layer uses dj_pack_text - pointer plus length - because a ZIP
* comment may contain a NUL.  An event message may not: it is a literal or a
* static string, it is never sliced, and every one of the eleven consumers
* treats it as a C string today.  Changing it would be a surface change with no
* failure mode behind it.  The convention is not universal, and this is the
* place to say which side of it this module sits on.
*
* Configurability
* ===============
* All configuration variables are defined in `cfg_test_object.h`.
*  1. The following types are set by the following values:
*     +----------------------+-------------------------------+-----------------+
*     |      type            |       config variable         |     default     |
*     +----------------------+-------------------------------+-----------------+ 
*     | dj_test_status       |  D_CFG_TEST_STATUS_TYPE       |     int32_t     |
*     | dj_test_type_id      |  D_CFG_TEST_TYPE_ID_TYPE      |     int32_t     |
*     | dj_test_callable_id  |  D_CFG_TEST_CALLABLE_ID_TYPE  |     int32_t     |
*     +----------------------+-------------------------------+-----------------+
*     Each typedef asserts what the knob must satisfy:
*     - signedness
*     - a width of 1, 2, 4 or 8 bytes.
* 
*   IMPORTANT:
*   ----------
*   Code names the typedef, NEVER the underlying type:
*   - A knob that names a type (instead of a typedef) cannot be checked against 
      a declared bit count the way a width knob can
      - Therefore: the assertions check the PROPERTIES the code depends on 
        instead. 
*   - Writing `int32_t` where `dj_test_type_id` is meant compiles today and
*     breaks silently the moment the width is reconfigured. That is the one
*     thing this mechanism exists to prevent.
*   - As per dconfig 6.1: misconfiguration results in a build-time failure.
*   
* 
* path:      /inc/djinterp/c/modules/test/test_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.11.08
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  STATUS VALUES
    -------------
    1.  value set
         1.  D_TEST_STATUS_PASSED
         2.  D_TEST_STATUS_FAILED
         3.  D_TEST_STATUS_SKIPPED
         4.  D_TEST_STATUS_PENDING
         5.  D_TEST_STATUS_ERROR
         6.  D_TEST_STATUS_COUNT
    2.  C enum
         1.  dj_test_status_flag
2.  IDENTIFIERS
    -----------
    1.  Status storage
         1.  dj_test_status
    2.  Type identity
         1.  dj_test_type_id
    3.  Callable handle
         1.  dj_test_callable_id
         2.  D_TEST_NO_CALLABLE
3.  OPERATIONS
    ----------
    1.  Classification
    2.  Naming and ordering
*/

#ifndef DJINTERP_C_TEST_COMMON_H
#define DJINTERP_C_TEST_COMMON_H 1

// std
#include <stddef.h>  // size_t
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // int32_t, int64_t, uint32_t
// djinterp
#include "../../djinterp.h"                                  // framework root
#include "../../../config/c/modules/test/cfg_test_object.h"  // D_CFG_TEST_*


//==============================================================================
// 1.  STATUS VALUES
//==============================================================================
// The value set, written once, here.  
// Both enum declarations below read these, so the two notations cannot drift.  
// Values are pinned, because a status is recorded in a parity stream and passed 
// across the DTest bridge.

// 1.1    value set
//------------------------------------------------------------------------------

// 1.1.1
// D_TEST_STATUS_PASSED
//   constant: this status value indicates that the test both ran AND every 
// check held.
#define D_TEST_STATUS_PASSED    0

// 1.1.2
// D_TEST_STATUS_FAILED
//   constant: this status value indicates that the test ran, but at least one
// check failed.
#define D_TEST_STATUS_FAILED    1

// 1.1.3
// D_TEST_STATUS_SKIPPED
//   constant: this status value indicates that the test was not run, by user
// decision.
#define D_TEST_STATUS_SKIPPED   2

// 1.1.4
// D_TEST_STATUS_PENDING
//   constant: this status value indicates that the test has not been evaluated
// yet.
#define D_TEST_STATUS_PENDING   3

// 1.1.5
// D_TEST_STATUS_ERROR
//   constant: this status value indicates that the test run was attempted, but
// the test could not be run.
#define D_TEST_STATUS_ERROR     4

// 1.1.6
// D_TEST_STATUS_COUNT
//   constant: how many statuses exist.  A switch over the set that forgets one
// is caught by asserting this against the highest enumerator.
#define D_TEST_STATUS_COUNT     5

// 1.2    C enum
//------------------------------------------------------------------------------

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
// asymmetry itself - "`extern "C" { ... }` is a linkage block, `extern "C"
// decl;` is a linkage specification on one declaration."  Swapping only the
// opener leaves `extern "C"` applied to the next declaration and an unmatched
// `}` at the end of the file.  The guard removal - which is what §8 is
// actually for - is done; the token swap is not.  See the log.

D_EXTERN_C_BEGIN

// 1.2.1
// dj_test_status_flag
//   enum: the C notation over the value set.
//   C gets the unscoped form.  It is the SAME value set - both declarations
// expand the same five macros - and it is what the shared kernel's functions
// take, so a C++ caller converts once at the boundary rather than the kernel
// carrying two signatures.
//   THE TAG IS `dj_test_status_flag`, AND THE BARE NAME BELONGS TO THE TYPEDEF.
// C keeps enum tags in their own namespace, so `enum dj_test_status` and a
// `dj_test_status` typedef coexist here; C++ does not, and this header is
// compiled by both.  Spelling the tag apart from the storage type is what lets
// the two forks read one file.  The enumerators keep their `_E` suffix.
enum dj_test_status_flag
{
    D_TEST_STATUS_PASSED_E  = D_TEST_STATUS_PASSED,
    D_TEST_STATUS_FAILED_E  = D_TEST_STATUS_FAILED,
    D_TEST_STATUS_SKIPPED_E = D_TEST_STATUS_SKIPPED,
    D_TEST_STATUS_PENDING_E = D_TEST_STATUS_PENDING,
    D_TEST_STATUS_ERROR_E   = D_TEST_STATUS_ERROR
};

D_STATIC_ASSERT((D_TEST_STATUS_ERROR + 1) == D_TEST_STATUS_COUNT,
                "dj_test_status_flag: the value set is not dense");


//==============================================================================
// 2.  IDENTIFIERS
//==============================================================================

// 2.1    Status storage
//------------------------------------------------------------------------------
// 2.1.1
// dj_test_status
//   type: the storage a status VALUE is held in - what a node's `status`
// member, and every function that takes or returns one, is spelled as.
//   Distinct from `enum dj_test_status_flag` above, deliberately: the enum is
// the C notation over the value set, and this is the type the value is stored
// and compared in.  A member declared as the enum would have
// implementation-defined size and could be resolved differently by the two
// forks, which is the goals §4 determinacy break the whole layout is arranged
// to avoid - so the enum names the values and this names the storage.
typedef D_CFG_TEST_STATUS_TYPE dj_test_status;

D_STATIC_ASSERT(((dj_test_status)-1) < 0,
                "dj_test_status: D_CFG_TEST_STATUS_TYPE must name a SIGNED "
                "integer type - d_test_status_is_valid rejects on `< 0`");
D_STATIC_ASSERT(( (sizeof(dj_test_status) == 1) ||
                  (sizeof(dj_test_status) == 2) ||
                  (sizeof(dj_test_status) == 4) ||
                  (sizeof(dj_test_status) == 8) ),
                "dj_test_status: width must be 1, 2, 4 or 8 bytes");

//   NO RANGE ASSERTION, AND THAT IS DELIBERATE RATHER THAN AN OMISSION.  The
// obvious third check - that the chosen type can hold D_TEST_STATUS_ERROR --
// cannot fail: the two assertions above already force a SIGNED type of 1, 2, 4
// or 8 bytes, and the narrowest of those still reaches 127 against a value set
// that stops at 4.  An assertion that cannot fire reads like a guarantee and
// provides none, so the reasoning is written here instead.

// 2.2    Type identity
//------------------------------------------------------------------------------
// 2.2.1
// dj_test_type_id
//   type: a node's STRUCTURAL type identity - the key into its test_kind
// definition.  Not a per-instance unique id: a node carries neither a unique id
// nor a depth, because both are facts of position that the owning tree confers
// during the walk.
typedef D_CFG_TEST_TYPE_ID_TYPE dj_test_type_id;

D_STATIC_ASSERT(((dj_test_type_id)-1) < 0,
                "dj_test_type_id: D_CFG_TEST_TYPE_ID_TYPE must name a "
                "SIGNED integer type");
D_STATIC_ASSERT(( (sizeof(dj_test_type_id) == 1) ||
                  (sizeof(dj_test_type_id) == 2) ||
                  (sizeof(dj_test_type_id) == 4) ||
                  (sizeof(dj_test_type_id) == 8) ),
                "dj_test_type_id: width must be 1, 2, 4 or 8 bytes");

// 2.3    Callable handle
//------------------------------------------------------------------------------
// 2.3.1
// dj_test_callable_id
//   type: an opaque, non-owning handle into a callable table.  Keeps a test
// node trivially copyable while still letting a leaf carry deferred work.
//   Zero is reserved: a node carrying 0 is fully evaluated and the value
// already in it is authoritative; a node carrying non-zero is deferred and the
// handler invokes the callable before firing per-test events.
typedef D_CFG_TEST_CALLABLE_ID_TYPE dj_test_callable_id;

D_STATIC_ASSERT(((dj_test_callable_id)-1) > 0,
                "dj_test_callable_id: D_CFG_TEST_CALLABLE_ID_TYPE must "
                "name an unsigned integer type");
D_STATIC_ASSERT(( (sizeof(dj_test_callable_id) == 1) ||
                  (sizeof(dj_test_callable_id) == 2) ||
                  (sizeof(dj_test_callable_id) == 4) ||
                  (sizeof(dj_test_callable_id) == 8) ),
                "dj_test_callable_id: width must be 1, 2, 4 or 8 bytes");

// 2.3.2
// D_TEST_NO_CALLABLE
//   constant: the reserved zero handle. Equivalent to writing 0, and clearer.
#define D_TEST_NO_CALLABLE  ((dj_test_callable_id)0)

//   THE C++ ALIAS `DTestCallableId` AND `k_no_callable` are in
// test_common.hpp, not here - same split as the enum above.


//==============================================================================
// 3.  OPERATIONS
//==============================================================================
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
// recorded in revision.md §3, not an oversight - all four of these are two to
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
//
//   THE ARGUMENTS ARE int64_t, THE ONE PLACE THE RULE ABOVE BENDS. What these
// take is a value that may not be a status at all - test_event.h's record
// carries one as int32_t - and a parameter of dj_test_status would narrow it
// before it is checked: at int8_t, 257 would arrive as 1 and pass. int64_t is
// the widest width the assertions in section 2 allow, so no configured width or
// caller's type is narrowed on the way in. What they RETURN is a status, so
// that is dj_test_status.


// 3.1    Classification
//------------------------------------------------------------------------------
/**
 * @brief Whether the value names one of the five statuses.
 *
 * @param[in] _status  the value, which need not be a status.
 * @return 1 when it names a status, 0 otherwise.
 */
D_STATIC_INLINE int
d_test_status_is_valid(
    int64_t _status
)
{
    return ( (_status >= 0) &&
             (_status < D_TEST_STATUS_COUNT) ) ? 1 : 0;
}

/**
 * @brief The value if it names a status, `error` otherwise.
 *
 * @param[in] _status  the value, which need not be a status.
 * @return the status, or D_TEST_STATUS_ERROR.
 *
 * THE COERCION RULE, AND IT LIVES HERE BECAUSE IT IS A PROPERTY OF THE TYPE,
 * not of any one module that stores one.  Three modules apply it - the event
 * record, the event status-change arguments, and the node's set_status - and
 * each used to spell it out at its own call site with its own comment.  Two
 * records of "what happened to this node" that disagreed about what an
 * out-of-range status means would be a divergence between two of DTest's own
 * modules, which the oracle cannot see because both are on the same side of it.
 * One inline function in the module that owns dj_test_status is the cheapest
 * place for them not to disagree.
 */
D_STATIC_INLINE dj_test_status
d_test_status_or_error(
    int64_t _status
)
{
    return d_test_status_is_valid(_status)
        ? (dj_test_status)_status
        : (dj_test_status)D_TEST_STATUS_ERROR;
}

/**
 * @brief Whether a status is an outcome rather than a state on the way to
 *        one.
 *
 * @param[in] _status  the status to classify.
 * @return 1 when the status is an outcome, 0 for pending or an invalid value.
 *
 * `pending` is the only non-terminal value: a node in it has not been
 * evaluated. `skipped` IS terminal - the decision not to run is itself an
 * outcome, and treating it as pending would make a skipped suite look
 * unfinished.
 */
D_STATIC_INLINE int
d_test_status_is_terminal(
    int64_t _status
)
{
    if (!d_test_status_is_valid(_status))
    {
        return 0;
    }

    return (_status != D_TEST_STATUS_PENDING) ? 1 : 0;
}

// 3.2    Naming and ordering
//------------------------------------------------------------------------------

/**
 * @brief The status's own spelling.
 *
 * @param[in] _status  the status to name.
 * @return a static, NUL-terminated name, or "unknown".
 *
 * A parity record carries the NAME, never the integer, so a renumbering cannot
 * pass unnoticed on one side.
 */
D_STATIC_INLINE const char*
d_test_status_name(
    int64_t _status
)
{
    static const char* const k_status[D_TEST_STATUS_COUNT] =
    {
        "passed",
        "failed",
        "skipped",
        "pending",
        "error"
    };

    return ( (_status < 0) ||
             (_status >= D_TEST_STATUS_COUNT) )
        ? "unknown"
        : k_status[_status];
}

/**
 * @brief Combines two statuses into the one a parent should carry.
 *
 * @param[in] _a  the first status.
 * @param[in] _b  the second.
 * @return the more severe of the two. An invalid input yields `error`, because
 *         a status nobody can classify is exactly the case error exists for.
 *
 * THIS IS WHERE A TREE'S RESULT COMES FROM, so the ordering is a semantic
 * decision rather than a convenience. Severity runs:
 *
 *     error > failed > skipped > pending > passed
 *
 * `error` outranks `failed` because the two mean different things: a failure
 * is a test that ran and disagreed, an error is a test that could not run. A
 * suite reporting "3 failed" when one of them never executed is telling the
 * reader something untrue.
 *
 * `pending` outranks `passed` so that a partly-evaluated tree never rolls up
 * as passed. A parent whose children are half unevaluated is pending, not
 * green - which is the whole reason this function is not just a max over the
 * enum values.
 *
 * `skipped` sits below `failed`, always. This is FIXED, not configurable; see
 * the section banner above for why the knob that used to move it could not
 * survive this function becoming inline.
 */
D_STATIC_INLINE dj_test_status
d_test_status_worse_of(
    int64_t _a,
    int64_t _b
)
{
    // severity rank, indexed by status value; higher is worse
    static const int k_rank[D_TEST_STATUS_COUNT] =
    {
        0,      // passed
        3,      // failed
        2,      // skipped
        1,      // pending
        4       // error
    };

    if ( (!d_test_status_is_valid(_a)) ||
         (!d_test_status_is_valid(_b)) )
    {
        return (dj_test_status)D_TEST_STATUS_ERROR;
    }

    return (dj_test_status)( (k_rank[_a] >= k_rank[_b]) ? _a : _b );
}


D_EXTERN_C_END


#endif  // DJINTERP_C_TEST_COMMON_H
