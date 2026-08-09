/******************************************************************************
* djinterp [test]                                         test_common_common.h
*
*   The DTest vocabulary: the ids, the status, and the event record that eleven
* other modules speak.  Compiled by BOTH C and C++.  Step B of the relay.
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
* WIDTHS ARE PINNED, EXCEPT ONE
* =============================
*   test_type_id is int32_t and test_callable_id is uint32_t: both fixed, both
* already correct in the C++ header.  test_event_id is std::size_t, which is
* NOT fixed -- it is 8 bytes on LP64 and 4 on ILP32.
*
*   It is left as size_t rather than pinned, because pinning it would change
* the existing surface for a benefit the parity oracle already provides:
* d_test_observe_size records a size WITH ITS WIDTH, so a data-model difference
* shows up as a visible diff rather than as silent agreement on small numbers.
* The hazard is handled by making it observable, not by hiding it.
*
*
* WHY message STAYS A const char*
* ===============================
*   The archive layer uses d_pack_text -- pointer plus length -- because a ZIP
* comment may contain a NUL.  An event message may not: it is a literal or a
* static string, it is never sliced, and every one of the eleven consumers
* treats it as a C string today.  Changing it would be a surface change with no
* failure mode behind it.  The convention is not universal, and this is the
* place to say which side of it this module sits on.
*
*
* path:      /inc/djinterp/test/test_common_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.30
******************************************************************************/

#ifndef DJINTERP_TEST_COMMON_
#define DJINTERP_TEST_COMMON_ 1

// c
#include <stddef.h>
#include <stdint.h>
// djinterp
#include "../c/djinterp.h"
#include "../config/test/cfg_test_common.h"     // D_INTERNAL_TEST_COMMON_*

#if !defined(D_EXTERN_C_BEGIN)
    #error "`test_common_common.h` needs `D_EXTERN_C_BEGIN`; define it or set \
            `D_CFG_DEFINE_EXTERN_C to 1"
#endif


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

#if defined(__cplusplus)

//   C++ keeps the scoped enum, so `test_status::passed` at eleven call sites
// is untouched.  The underlying type is fixed at int32_t so the two languages
// agree about width -- an unfixed enum class has an implementation-defined
// underlying type, which is a §4 determinacy break at a boundary this value
// crosses.
namespace djinterp
{
    enum class test_status : int32_t
    {
        passed  = D_TEST_STATUS_PASSED,
        failed  = D_TEST_STATUS_FAILED,
        skipped = D_TEST_STATUS_SKIPPED,
        pending = D_TEST_STATUS_PENDING,
        error   = D_TEST_STATUS_ERROR
    };
}

#endif  // __cplusplus


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

D_STATIC_ASSERT( (D_TEST_STATUS_ERROR + 1) == D_TEST_STATUS_COUNT,
                "d_test_status: the value set is not dense");


// =============================================================================
// II.  IDENTIFIERS
// =============================================================================

// d_test_type_id
//   type: a node's STRUCTURAL type identity -- the key into its test_kind
// definition.  Not a per-instance unique id: a node carries neither a unique id
// nor a depth, because both are facts of position that the owning tree confers
// during the walk.
typedef int32_t d_test_type_id;

// d_test_callable_id
//   type: an opaque, non-owning handle into a callable table.  Keeps a test
// node trivially copyable while still letting a leaf carry deferred work.
//   Zero is reserved: a node carrying 0 is fully evaluated and the value
// already in it is authoritative; a node carrying non-zero is deferred and the
// handler invokes the callable before firing per-test events.
typedef uint32_t d_test_callable_id;

// D_TEST_NO_CALLABLE
//   constant: the reserved zero handle. Equivalent to writing 0, and clearer.
#define D_TEST_NO_CALLABLE  ((d_test_callable_id)0)

#ifdef __cplusplus
namespace djinterp
{
    //   THE CALLABLE ID, in the C++ spelling the faces use.
    //
    //   AN ALIAS AND NOT A SECOND TYPE.  `d_test_callable_id` is the type;
    // this is its name on the C++ side, so a value crosses the boundary with
    // no conversion and `test_callable.hpp` keeps the spelling its call sites
    // already use.  Both lived in the un-coalesced `test_common.hpp`, which
    // this kernel replaced -- and their absence is why `test_callable.hpp`
    // did not compile against the coalesced tree.
    //
    //   DECLARED HERE, AFTER the typedef and the macro, rather than beside
    // `test_status` above: an alias cannot name a type declared later in the
    // same file, and the C++ block above runs first.
    typedef d_test_callable_id test_callable_id;

    //   `static const` rather than `constexpr` so the declaration is legal at
    // the C++11 floor with no qualifier gate; it is a compile-time constant
    // either way.
    static const test_callable_id k_no_callable = D_TEST_NO_CALLABLE;
}
#endif  // __cplusplus

// d_test_event_id
//   type: a lightweight numeric identifier for a lifecycle event.  size_t, so
// its width varies by data model -- see the banner for why that is observed
// rather than pinned.
typedef size_t d_test_event_id;


// =============================================================================
// III. THE EVENT RECORD
// =============================================================================

// d_test_event
//   struct: what happened, in what state, with an optional message.  Plain
// data, trivially copyable, identical under both languages.
struct d_test_event
{
    d_test_event_id event;
    int32_t         status;     // a d_test_status / test_status value
    const char*     message;    // borrowed; may be null
};

// D_TEST_EVENT_INIT
//   macro: an empty event -- id 0, pending, no message.
#define D_TEST_EVENT_INIT                                                      \
{                                                                              \
    (d_test_event_id)0, D_TEST_STATUS_PENDING, (const char*)0                  \
}

//   `status` is int32_t rather than the enum, for the reason compress_common.h
// gives at length: a struct member declared with an enum type has an
// implementation-defined size, and C and C++ may resolve it differently for one
// declaration.  Assigning `test_status::passed` to it is an ordinary
// conversion, so no call site changes.

D_STATIC_ASSERT(offsetof(struct d_test_event, event) == 0,
                "d_test_event: field drift at event");
D_STATIC_ASSERT(offsetof(struct d_test_event, status) >= sizeof(size_t),
                "d_test_event: status must follow event");
D_STATIC_ASSERT(sizeof(((struct d_test_event*)0)->status) == 4,
                "d_test_event: status must be exactly 32 bits");


// =============================================================================
// IV.  OPERATIONS
// =============================================================================

const char*     d_test_status_name(int32_t _status);
int             d_test_status_is_terminal(int32_t _status);
int             d_test_status_is_valid(int32_t _status);
int32_t         d_test_status_worse_of(int32_t _a, int32_t _b);
struct d_test_event
                d_test_event_make(d_test_event_id _event,
                                  int32_t         _status,
                                  const char*     _message);


D_EXTERN_C_END


#endif  // DJINTERP_TEST_COMMON_