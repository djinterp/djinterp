/******************************************************************************
* djinterp [test]                                              test_event.h
*
*   The DTest event module: the observation RECORD, the lifecycle ALPHABET,
* each summand's payload signature, and a dispatch table over both.  Compiled
* by BOTH C and C++.
*
*
* TWO THINGS IN ONE FILE, AND THE SEAM IS DELIBERATE
* ==================================================
*   This module is the merge of two files that shared a name and shared
* nothing else.  Both halves are kept whole and the join is named rather than
* blended, because a reader who needs one of them should be able to see where
* the other begins:
*
*     THE RECORD (§I) is `struct d_test_event` -- an {id, status, message}
*     observation, plain data, built by one constructor.  It was extracted
*     from test_common.h so that module could be event-agnostic, then had no
*     home for two revisions.  This is the home.
*
*     THE ALPHABET (§II onward) is the twelve lifecycle summands, their
*     payload signatures, and the dispatch table that fires them.  It was
*     test_event_common.h.
*
*   THEY DO NOT REFER TO EACH OTHER.  The record does not know a kind exists;
* dispatch does not carry a record.  The one thing they share is a rule -- an
* out-of-range status is stored as `error` -- and §VII.a is where that is now
* stated once instead of twice.
*
*
* WHAT WAS COALESCED, AND WHAT WAS DELIBERATELY NOT
* =================================================
*   test_event.hpp declares twelve lifecycle tags through the D_EVENT family
* from core/event/event_common.hpp.  In that subsystem an event is not an
* instance but a TYPE -- a tag carrying a nested `payload_type` (a std::tuple
* of value domains) and a `name()` -- and, in its own words, "the type IS the
* registration".
*
*   Two different things are wrapped up in that sentence and they coalesce
* differently:
*
*     THE ALPHABET is data.  Twelve summands, each with a name, an arity, and
*     an ordered list of payload domains.  That is a signature -- the F of
*     F_test -- and it is the thing every walker, handler and reporter in the
*     framework agrees about.  It lives here.
*
*     TAG-AS-TYPE is notation.  Registration by declaration, tuple payloads,
*     and bind-time compatibility checking are how C++ SPELLS the alphabet;
*     they are not what the alphabet MEANS.  Per goals §11 and the decision
*     log -- "notation is not semantics", "C needs the object, not the
*     notation" -- C gets the signature table and a switch, and the tags stay
*     in the C++ face, unchanged at every call site.
*
*   THE OPEN ALPHABET IS OPEN IN BOTH FORKS AND CHECKED IN NEITHER.  A user tag
* declared with D_TEST_EVENT needs no enrolment call in C++; in C it takes a
* kind id at or above D_TEST_EVENT_KIND_COUNT and a context its listeners
* understand.  An earlier revision claimed C recovered at runtime what C++
* rejects at bind time -- "enforcement differs between the languages;
* behaviour does not" -- via a signature table and a MISMATCH return.  That was
* not true even then: a user tag had no way to add a table row, so its payload
* was validated against nothing and its arguments were silently discarded.  The
* claim is withdrawn rather than restated.  What C++ still has is each tag's
* tuple size asserted against §IV's arity macro at compile time; what C has is
* the contract at §III.
*
*
* WHY THERE IS NO DEPENDENCY ON THE EVENT SUBFRAMEWORK
* ====================================================
*   core/event/event_common.hpp is ABSENT from the working set, and step C's
* handoff records event_dispatcher.hpp as absent too -- while noting that
* test_counter and test_timer were both written to accept a bridge that
* nothing yet supplies.
*
*   This module is that bridge's kernel half, and it is written so that the
* absence stays a gap rather than becoming a guess.  Nothing here reconstructs
* D_EVENT, event_traits, or verdict.  When event_common.hpp surfaces, the C++
* face's twelve tags re-derive from D_EVENT and gain a `kind` constant; every
* declaration in this file is untouched.  If instead the real subframework
* turns out to disagree with the signature table below, that is a divergence
* to report, and the parity body is what will find it.
*
*
* THE SIGNATURE IS CLAIMED TWICE, ON PURPOSE
* ==========================================
*   Every arity appears both as a macro (D_TEST_EVENT_ARITY_*, usable in #if
* and in a static assertion) and as a switch inside d_test_event_arity().
* That is redundant and it is the point: roadmap §3's corollary is that
* wherever two mechanisms answer one question, their agreement is itself a
* parity property, and it is the one an author is most likely to assume.  The
* .c ties them with D_STATIC_ASSERT; the parity body records both halves; and
* the C++ face additionally asserts each macro against
* std::tuple_size<payload_type>, so a tag whose tuple grows a domain without
* the table learning about it fails to compile.
*
*   TWO mechanisms now, not three.  A tuple that disagrees with the table is a
* compile error and a table that disagrees with itself is a static assertion in
* the .c.  The third -- a fire-site check that a payload's slots matched the
* signature -- went with the slots, and §III says what that cost.
*
*
* CALLER-PROVIDED STORAGE
* =======================
*   The listener array is borrowed, never owned, never grown -- the pattern
* step C made binding for this half of the relay, and which d_pack_sink,
* d_archive_extract and d_counter already use.  d_test_dispatch_on returns
* FULL rather than reallocating.  A suite's listener count is a property of
* the suite, which is written down, so the bound is knowable.
*
*
* DISPATCH ORDER IS BIND ORDER, AND IS RECORDED
* =============================================
*   Listeners for one kind fire in the order they were bound.  This is a
* contract rather than an implementation detail: an event stream is only
* comparable across two forks if its ORDER is a function of the inputs, and a
* handler that fired in table order would reorder the moment either fork's
* binding sequence changed.  Each listener carries the sequence number it was
* issued, so the order is recoverable from the table alone and a parity body
* can record it directly rather than inferring it from side effects.
*
*   IT IS NOT CONFIGURABLE, AND THE KNOB THAT SAID IT WAS IS GONE.
* D_CFG_TEST_EVENT_STABLE_ORDER was defined, validated and normalised into
* D_INTERNAL_TEST_EVENT_STABLE_ORDER, which nothing read -- so the off path
* compiled clean, produced identical object code, and read as supported.  The
* operation it offered to switch off is the one the parity law rests on.  It
* was deleted rather than wired, on the same argument rev08 makes for
* D_CFG_EVENT_TABLE_ORDERED_ITERATION.  Deleting it changed no behaviour,
* because it never had any.
*
*
* path:      /inc/djinterp/c/test/test_event.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.31
*                                                          merged:  2026.08.11
******************************************************************************/

#ifndef DJINTERP_C_TEST_EVENT
#define DJINTERP_C_TEST_EVENT 1

// c
#include <stddef.h>
#include <stdint.h>
// djinterp
#include "../djinterp.h"
#include "../../config/c/test/cfg_test_event.h"  // D_CFG_TEST_EVENT_*
#include "./test_common.h"                  // d_test_status, is_valid


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
//   ONE CONFIG, ONE MODULE.  This header includes cfg_test_event.h and not the
// cfg_test.h umbrella: the umbrella exists for consumers who want every DTest
// knob at once, and a module reaching for it would rebuild, one include at a
// time, the four-module config that rev13 split apart.

D_EXTERN_C_BEGIN


// =============================================================================
// 0.   TYPES FROM CONFIGURATION
// =============================================================================
//   FIRST THING IN THE FILE, and deliberately.  A knob originates in
// cfg_test_event.h with a default and a validation block; the typedef that
// turns it into a type belongs in the module that owns the type, so a reader
// looking for d_test_event_id finds it in the file that uses it rather than in
// a config header three directories away.
//
//   The D_TEST_EVENT_ID_TYPE roll-up alias that used to sit between these two
// is gone.  It renamed one macro to another and put the answer in a third
// file; nothing read it except this typedef.

// d_test_event_id
//   type: a lightweight numeric identifier for a lifecycle event.
//
//   THE WIDTH COMES FROM THE CONFIG.  This was size_t, so it followed the data
// model -- 8 bytes on LP64, 4 on ILP32.  That is the platform-varying layout
// dconfig 6.2 forbids, inside a struct two forks must agree on byte for byte.
// It is now D_CFG_TEST_EVENT_ID_TYPE, defaulting to uint64_t: identical on the
// common target, a stated choice on the others.  ON A 32-BIT TARGET THIS
// WIDENS THE FIELD; set uint32_t to keep the old size.
//
//   Code names d_test_event_id, never the underlying type.  Writing uint64_t
// where this is meant compiles today and breaks silently the moment the knob
// is changed, which is the one thing the mechanism exists to prevent.
typedef D_CFG_TEST_EVENT_ID_TYPE d_test_event_id;

D_STATIC_ASSERT(((d_test_event_id)-1) > 0,
                "d_test_event_id: D_CFG_TEST_EVENT_ID_TYPE must name an "
                "unsigned integer type");
D_STATIC_ASSERT((sizeof(d_test_event_id) == 1) ||
                (sizeof(d_test_event_id) == 2) ||
                (sizeof(d_test_event_id) == 4) ||
                (sizeof(d_test_event_id) == 8),
                "d_test_event_id: width must be 1, 2, 4 or 8 bytes");


// =============================================================================
// I.   THE OBSERVATION RECORD
// =============================================================================
//   Independent of everything below it.  A module that wants to record what
// happened to a node needs this and none of the alphabet; a module that
// dispatches needs the alphabet and none of this.

// d_test_event
//   struct: what happened, in what state, with an optional message.  Plain
// data, trivially copyable, identical under both languages.
//
//   NOT TO BE CONFUSED WITH struct d_test_event_payload BELOW.  This is an
// observation -- one id, one status, one message, recorded after the fact.
// That is a fired event's arguments, checked against a signature.  They share
// a prefix and nothing else, which is exactly why the two files that held them
// collided by name and not by content.
struct d_test_event
{
    d_test_event_id event;
    int32_t         status;     // a d_test_status value
    const char*     message;    // borrowed; may be null
    const void*     context;    // borrowed; may be null; see below
};

// D_TEST_EVENT_INIT
//   macro: an empty event -- id 0, pending, no message.
#define D_TEST_EVENT_INIT                                                      \
{                                                                              \
    (d_test_event_id)0, D_TEST_STATUS_PENDING,                                 \
    (const char*)0, (const void*)0                                             \
}

//   `status` is int32_t rather than the enum, for the reason compress_common.h
// gives at length: a struct member declared with an enum type has an
// implementation-defined size, and C and C++ may resolve it differently for
// one declaration.  Assigning `test_status::passed` to it is an ordinary
// conversion, so no call site changes.

//   `context` IS ARBITRARY AND ITS MEANING COMES FROM `event`.  It carries
// whatever the producer wants a receiver to have and the other three fields
// cannot hold.  There is no tag, no size, no descriptor: the receiver is
// expected to know the presence, absence, type, number and order of whatever
// is behind the pointer, from the event id alone.  §III's payload carries the
// same contract, stated separately because the two structs are independent.
//
//   THE COST, NAMED HERE RATHER THAN DISCOVERED LATER.  An address is on the
// parity kernel's closed list of unrecordable things, so a parity body cannot
// compare two forks' `context` -- only whether one was present.  Two forks
// differing ONLY in what they hang here are, to the oracle, identical.  That
// is fine for something that is the producer's private business; it is not
// fine for anything the differential is meant to catch, and putting the latter
// here hides it silently.
//
//   BORROWED, AND const.  The record does not own it, free it, dereference it,
// or write through it.  Lifetime is the producer's problem, and a record
// outliving what its context points at is this field's one failure mode.
//
//   NOTE THE UNRELATED `context` IN struct d_test_listener (§V).  That one is a
// LISTENER's own mutable state, handed back to it at every call.  This one is
// an EVENT's arguments.  Two different things share the word.
//
//   WHY THE MESSAGE FIELD STAYS A const char*.  The archive layer uses
// d_pack_text -- pointer plus length -- because a ZIP comment may contain a
// NUL.  An event message may not: it is a literal or a static string, it is
// never sliced, and every one of the eleven consumers treats it as a C string
// today.  Changing it would be a surface change with no failure mode behind
// it.  The convention is not universal, and this is the place to say which
// side of it this module sits on.


// =============================================================================
// II.  THE ALPHABET
// =============================================================================
//   The twelve built-in lifecycle summands, in the order test_event.hpp
// declares them.  Values are pinned: a kind is recorded in a parity stream and
// crosses the DTest bridge, so a renumbering is a wire change.
//
//   The names below are the C spelling; d_test_event_kind_name returns the
// C++ TAG spelling ("on_test_failed"), because that is the name the framework
// already prints, binds by, and reports in on_listener_threw's first payload
// slot.  Two spellings of one summand is a divergence waiting to happen, so
// there is exactly one function that produces the printable form and both
// languages call it.

enum d_test_event_kind
{
    D_TEST_EVENT_SESSION_START  = 0,
    D_TEST_EVENT_SESSION_END    = 1,
    D_TEST_EVENT_MODULE_START   = 2,
    D_TEST_EVENT_MODULE_END     = 3,
    D_TEST_EVENT_TEST_START     = 4,
    D_TEST_EVENT_TEST_END       = 5,
    D_TEST_EVENT_TEST_PASSED    = 6,
    D_TEST_EVENT_TEST_FAILED    = 7,
    D_TEST_EVENT_TEST_SKIPPED   = 8,
    D_TEST_EVENT_TEST_ERROR     = 9,
    D_TEST_EVENT_STATUS_CHANGE  = 10,
    D_TEST_EVENT_LISTENER_THREW = 11
};

//   WHAT EACH KIND'S `context` POINTS AT.  This table IS the contract of §III:
// a receiver knows a kind's arguments from the kind, and this is where it knows
// them from.  Nothing enforces it -- see §III for why that is deliberate and
// what it costs -- so it is the one comment in this header that must not be
// allowed to drift.  `arity` is the count in the same row, and §IV's macro half
// is asserted against the C++ tuples, so the NUMBER is checked even though the
// types are not.
//
//     kind                 arity   context points at
//     -------------------  -----   -----------------------------------------
//     session_start          0     null
//     session_end            2     { size_t passed; size_t failed; }
//     module_start           1     the node, directly
//     module_end             1     the node, directly
//     test_start             1     the node, directly
//     test_end               1     the node, directly
//     test_passed            1     the node, directly
//     test_failed            1     the node, directly
//     test_skipped           1     the node, directly
//     test_error             2     { const void* node; const char* text; }
//     status_change          3     { const void* node; int32_t before, after; }
//     listener_threw         2     { const char* event_name; const char* what; }
//
//   A `text` MAY BE NULL; the alphabet says on_test_error's message is
// optional.  The two status values in status_change are d_test_status values
// and SHOULD be passed through d_test_status_or_error, which is the rule
// d_test_event_make applies and the reason it lives in test_common.h rather
// than in either caller.

// D_TEST_EVENT_KIND_COUNT
//   constant: the number of BUILT-IN summands.  Also the first kind id
// available to a user tag: the alphabet is open, so this is a boundary rather
// than a maximum.
#define D_TEST_EVENT_KIND_COUNT     12

//   NOTE the two summands that are not node-scoped.  on_session_start and
// on_session_end bracket the run and carry no node, and on_listener_threw
// carries two strings and no node at all -- it reports a failure IN the
// dispatch machinery, so binding it to a node would suggest the node was at
// fault.  A signature table that assumed "every event carries a node" would
// be wrong about three of twelve, which is why the table is explicit.


// =============================================================================
// III. PAYLOAD
// =============================================================================

// d_test_event_payload
//   struct: a fired event -- its kind, and a pointer to its arguments.
// Trivially copyable and identical under both languages.
struct d_test_event_payload
{
    int32_t     kind;       // an enum d_test_event_kind value
    const void* context;    // borrowed; the kind says what is behind it
};

//   THE RECEIVER KNOWS.  There is no arity, no domain tag, no size, and no
// descriptor.  `kind` names the summand; the presence, absence, type, number
// and order of whatever `context` points at follow from that and are the
// receiver's to know.  A listener bound to on_status_change is bound to it
// precisely because it knows what on_status_change carries.
//
//   THIS REPLACED A THREE-SLOT TAGGED ARRAY, and the trade is worth stating
// because the thing removed looked like a safety net.  The old payload was 104
// bytes -- three 32-byte slots each holding a domain tag plus five members of
// which exactly one was ever live -- and it was validated at every fire against
// a signature table.  What that bought, measured rather than assumed:
//
//     THE CHECK WAS VACUOUS.  Every constructor stamped its domains FROM the
//     table, and the validator compared them AGAINST the table, so the two
//     agreed by construction.  It could only fail on a payload assembled by
//     hand, which was not an API path.
//
//     THE CONSTRUCTORS DROPPED ARGUMENTS SILENTLY.  Each stored a value only
//     `if` the slot's domain matched, so d_test_event_counts on a node-shaped
//     kind discarded both counts, returned a payload, and passed validation.
//     The machinery checked SHAPE and never checked that anything was STORED,
//     which is the mistake a caller actually makes.
//
//     THE OPEN ALPHABET DID NOT WORK.  A user tag was documented to "supply
//     its own signature row"; there was no API to supply one, the table was
//     static in the .c, and d_test_event_node on kind 12 returned arity 0 with
//     the pointer discarded -- and passed validation.
//
//   So the slots were paying 104 bytes and a silent-discard bug for a check
// that could not catch the realistic error and a generality that was not
// implemented.  A pointer the receiver understands is smaller, honest about
// having no runtime guarantee, and cannot drop what it is given.
//
//   WHAT WAS GENUINELY LOST, so it is not rediscovered as a surprise: a parity
// body could previously walk ANY payload generically -- "kind 10, slot1
// STATUS=1, slot2 STATUS=4" -- without knowing the kind.  It now needs a
// per-kind decoder.  The twelve-way switch did not disappear; it moved out of
// the table (data, auditable) and into the recorder (code).  The arity table
// at §IV survives partly to keep half of that claim checkable.

//   NO ARGUMENT STRUCTS ARE DECLARED HERE, AND THAT IS THE POINT OF THE
// CONTRACT.  Four were, briefly -- one per multi-argument shape -- and they
// bought nothing the contract does not already give away.  A struct with no
// tag, no size and nothing checking it is a comment that occupies a name: the
// compiler never compares a producer's struct to a consumer's cast, so the
// agreement was always by reading, and a declaration only made the reading feel
// enforced.  Four names in a shared namespace, for that.
//
//   THE SHAPES ARE WRITTEN DOWN AT §II INSTEAD, in the alphabet's own comment
// column, beside the arity each already carried.  One place says what a kind
// is, how many arguments it takes, and what they are -- rather than a comment
// pointing at a struct in another section that a caller must then match by
// hand anyway.
//
//   A CALLER DECLARES WHAT IT NEEDS, AT THE FIRE SITE:
//
//       struct { const void* node; int32_t before; int32_t after; } a;
//       a.node   = node;
//       a.before = d_test_status_or_error(before);
//       a.after  = d_test_status_or_error(after);
//
//       struct d_test_event_payload p =
//           d_test_event_payload_make(D_TEST_EVENT_STATUS_CHANGE, &a);
//       d_test_dispatch_fire(&dispatch, &p);
//
//   AND THE COST IS REAL, so it is stated rather than left to be found.  A
// producer and a consumer in different translation units now agree because both
// read §II correctly, not because both included one declaration.  Reorder the
// members in one of them and it is undefined behaviour with nothing to catch
// it -- no warning, no assertion, no MISMATCH.  That is a further step down the
// same road §III already took, taken deliberately: the enforcement was already
// gone, and what is removed here is the appearance of it.
//
//   EIGHT OF THE TWELVE NEED NOTHING AT ALL.  The node-scoped kinds point
// `context` straight at the node, and on_session_start carries null.


// =============================================================================
// IV.  THE SIGNATURE TABLE, CLAIMED AT COMPILE TIME
// =============================================================================
//   The macro half.  Usable in #if, in a static assertion, and -- in the C++
// face -- against std::tuple_size<payload_type>.  d_test_event_arity() is the
// runtime half of the same claim and the two are tied in the .c.

#define D_TEST_EVENT_ARITY_SESSION_START    0
#define D_TEST_EVENT_ARITY_SESSION_END      2
#define D_TEST_EVENT_ARITY_MODULE_START     1
#define D_TEST_EVENT_ARITY_MODULE_END       1
#define D_TEST_EVENT_ARITY_TEST_START       1
#define D_TEST_EVENT_ARITY_TEST_END         1
#define D_TEST_EVENT_ARITY_TEST_PASSED      1
#define D_TEST_EVENT_ARITY_TEST_FAILED      1
#define D_TEST_EVENT_ARITY_TEST_SKIPPED     1
#define D_TEST_EVENT_ARITY_TEST_ERROR       2
#define D_TEST_EVENT_ARITY_STATUS_CHANGE    3
#define D_TEST_EVENT_ARITY_LISTENER_THREW   2


// =============================================================================
// V.   DISPATCH
// =============================================================================

// -----------------------------------------------------------------------------
//      THE VERDICT SET P
// -----------------------------------------------------------------------------
//   ADOPTED FROM core/event/event_common.h RATHER THAN INVENTED.  That header
// declares `enum d_verdict { D_VERDICT_PASS = 0, D_VERDICT_CONSUME = 1 }` and
// documents the law: "consume is the left zero".  The VALUES are adopted; the
// NAME is DTest's, and that is a rev13 change with a consequence worth stating.
//
//   IT USED TO BE `d_verdict`, SPELLED IDENTICALLY TO THE SUBFRAMEWORK'S, under
// an `#if !defined(D_VERDICT_PASS)` guard so that whichever header was included
// first won and there was never a second definition.  With a distinct name
// there is nothing to collide with, so the guard would always be true and
// would document a hazard that no longer exists.  It is gone and the enum is
// declared unconditionally.
//
//   AND THE GUARD NEVER WORKED, WHICH IS WHY REMOVING IT COSTS NOTHING.
// D_VERDICT_PASS is an ENUMERATOR, not a macro, so `#if !defined(D_VERDICT_PASS)`
// was invisible to the preprocessor and always true: the enum was declared
// unconditionally the whole time, and had event_common.h ever been included
// the result would have been a redefinition error, not the orderly
// first-one-wins the banner described.  An inert guard reading as protection,
// which is rev08's finding in a third shape.
//
//   NO CROSS-CHECK IS WIRED HERE, AND THAT IS DELIBERATE.  Tying the two sets
// with a static assertion needs the preprocessor to know whether
// event_common.h was included, and an enumerator cannot tell it that.  The
// mechanism would be that header's include guard or a macro it defines --
// neither knowable from here, and guessing one is how a check ends up
// asserting nothing.  It is owed, not written; see the log.
//
//   THE COST, STATED: there are now two spellings of one two-point set when
// both headers are present.  They are pinned equal, the listener returns plain
// `int`, and the conversion at the bridge is a cast that changes no bits.
//
//   THIS WAS STEP D's ONE WRONG CALL, and it was mine.  Step D ruled the
// verdict "a face concern" and gave the listener a void return.  It is not a
// face concern: a two-point set with a left annihilator is a MONOID, and
// dispatch is a fold over it -- which is semantics, and belongs in the kernel
// by exactly the argument step D used to put the alphabet there.  With a void
// return the dispatcher could not halt propagation at all, so `consume` was
// unreachable and the subsystem's central law was unimplementable.
//
//   It is the same error I caught in F's tree decision at step G: a monoid
// assumed away, and the collapsed form silently keeps only one branch.  I made
// it two steps before I caught someone else making it.

enum d_test_verdict
{
    D_TEST_VERDICT_PASS    = 0,
    D_TEST_VERDICT_CONSUME = 1
};

D_STATIC_ASSERT(D_TEST_VERDICT_PASS == 0,
                "d_test_verdict: pass must be 0 -- it is the fold's identity");
D_STATIC_ASSERT(D_TEST_VERDICT_CONSUME == 1,
                "d_test_verdict: consume must be 1");

//   The values are pinned against the subframework's by DOCUMENTATION only,
// until the mechanism above exists.  Both are 0 and 1; a renumbering on either
// side is currently a silent reinterpretation at the bridge.

// D_TEST_VERDICT_CONSUMED
//   macro: true if a verdict halts propagation.  The C spelling of
// event_common.hpp's `consumed`, so a dispatch loop reads the same in both
// languages.
#define D_TEST_VERDICT_CONSUMED(_v)  ((int)(_v) == (int)D_TEST_VERDICT_CONSUME)

// D_TEST_VERDICT_COMBINE
//   macro: the monoid operation.  `consume` is the LEFT ZERO, so once a
// listener consumes, nothing after it can restore propagation.  Written out
// rather than inlined at the fold site because it is the law, and a law stated
// once is a law that can be recorded.
#define D_TEST_VERDICT_COMBINE(_acc, _next)                                         \
    (D_TEST_VERDICT_CONSUMED(_acc) ? (int)D_TEST_VERDICT_CONSUME : (int)(_next))


// d_test_listener_fn
//   function pointer: notified when a bound kind fires.  Takes the payload by
// const pointer rather than by value -- a payload is ~80 bytes and a listener
// has no reason to own a copy.
//
//   A listener RETURNS A VERDICT.  `pass` lets dispatch continue to the next
// bound listener; `consume` halts it.  See the note above for why this is not,
// as step D claimed, a face concern.
//
//   A listener that has no opinion returns D_TEST_VERDICT_PASS, which is the fold's
// identity, so the common case costs one `return 0`.
typedef int (*d_test_listener_fn)(void*                              _context,
                                  const struct d_test_event_payload* _payload);

// d_test_listener
//   struct: one binding.  `order` is the sequence number issued at bind time
// and is what makes dispatch order recoverable from the table -- see the
// banner.
struct d_test_listener
{
    d_test_listener_fn  notify;
    void*               context;
    int32_t             kind;
    int32_t             order;
};

// d_test_dispatch
//   struct: the listener table.  Borrowed storage, a bind counter, and the
// running totals a parity body reads.
struct d_test_dispatch
{
    struct d_test_listener* listeners;      // borrowed, never owned
    size_t                  count;
    size_t                  capacity;

    size_t                  fired;          // events accepted and dispatched
    size_t                  delivered;      // listener invocations
    size_t                  rejected;       // events refused by the signature
    size_t                  consumed;       // fires halted by a listener
    int32_t                 last_kind;      // -1 when nothing has fired
    int32_t                 next_order;
};

// d_test_dispatch_result
//   enum: the outcome of a dispatch operation.  Pinned; recorded by name.
enum d_test_dispatch_result
{
    D_TEST_DISPATCH_OK          = 0,
    D_TEST_DISPATCH_FULL        = 1,    // no room in the caller's array
    D_TEST_DISPATCH_INVALID     = 2     // null table, null callback, bad kind
};

// D_TEST_DISPATCH_RESULT_COUNT
//   constant: the number of enumerators in d_test_dispatch_result.
#define D_TEST_DISPATCH_RESULT_COUNT    3

//   MISMATCH IS GONE, AND SO IS D_CFG_TEST_EVENT_VALIDATE_PAYLOAD.  It named
// "payload disagrees with the signature", which nothing can now determine: an
// opaque context has no shape to disagree with.  Keeping the enumerator would
// have left a value no function returns, and keeping the knob would have left
// one gating a branch that no longer exists -- an inert knob reading as
// supported, which is what rev08 and rev11 each deleted rather than kept.
//
//   WHAT C GIVES UP IS NAMED, NOT PAPERED OVER.  The header used to argue that
// C++ rejects a mismatched listener at bind time and C recovers it at runtime,
// so "enforcement differs between the languages; behaviour does not."  That
// symmetry is now broken on purpose: C++ still asserts each tag's tuple size
// against §IV's arity macro at compile time, and C checks nothing about the
// context at all.  A C fire site that hands on_status_change a counts struct is
// undefined behaviour rather than a returned error.  That is the price of the
// contract at §III, and it is charged to the fire site, which is the only place
// with enough information to avoid it.

// D_TEST_DISPATCH_INIT
//   macro: an empty, unbound dispatch table, as a brace initialiser.  Equal
// in every member to what d_test_dispatch_init produces; the parity body
// records both and compares them, because two ways of spelling "empty" that
// drift apart is a defect neither one can see alone.
#define D_TEST_DISPATCH_INIT                                                   \
{                                                                              \
    (struct d_test_listener*)0, (size_t)0, (size_t)0,                          \
    (size_t)0, (size_t)0, (size_t)0, (size_t)0, -1, 0                          \
}


// =============================================================================
// VI.  LAYOUT ASSERTIONS
// =============================================================================
//   Fixed-width members are asserted exactly; everything pointer-bearing is
// asserted for ORDER only, since where the padding lands is a data-model
// property and nothing here reads it.

//   THE RECORD.  Its offsets are asserted AGAINST THE ID TYPE, not against
// size_t.  The assertion read `>= sizeof(size_t)` while the field WAS size_t;
// with the type configurable that form fails on a correct 32-bit-id build
// under LP64 -- the assertion wrong, not the struct.
D_STATIC_ASSERT(offsetof(struct d_test_event, event) == 0,
                "d_test_event: field drift at event");
D_STATIC_ASSERT(offsetof(struct d_test_event, status) >=
                    sizeof(d_test_event_id),
                "d_test_event: status must follow event");
D_STATIC_ASSERT(sizeof(((struct d_test_event*)0)->status) == 4,
                "d_test_event: status must be exactly 32 bits");
D_STATIC_ASSERT(offsetof(struct d_test_event, context) >
                offsetof(struct d_test_event, message),
                "d_test_event: context must follow message");

//   THE ALPHABET.
D_STATIC_ASSERT(offsetof(struct d_test_event_payload, kind) == 0,
                "d_test_event_payload: field drift at kind");
D_STATIC_ASSERT(offsetof(struct d_test_event_payload, context) >=
                sizeof(int32_t),
                "d_test_event_payload: context must follow kind");
//   NOTHING ASSERTS THE ARGUMENT SHAPES, because there are no argument types to
// assert about -- a caller's local struct is outside this header's reach.  What
// was lost with the four structs was three layout assertions that only ever
// checked the four declarations against themselves.
D_STATIC_ASSERT(offsetof(struct d_test_listener, kind) >
                offsetof(struct d_test_listener, context),
                "d_test_listener: kind must follow the callback pair");
D_STATIC_ASSERT(offsetof(struct d_test_dispatch, count) >
                offsetof(struct d_test_dispatch, listeners),
                "d_test_dispatch: count must follow listeners");


// =============================================================================
// VII. OPERATIONS
// =============================================================================

// -----------------------------------------------------------------------------
//   a.  THE RECORD
// -----------------------------------------------------------------------------
//   d_test_event_make builds a record, storing an out-of-range status as
// `error` rather than passing it through, so a malformed event cannot later be
// mistaken for a passing one.
//
//   IT IS OUT-OF-LINE NOW, AND THAT WAS THE STATED PLAN.  It was D_STATIC_INLINE
// in the extracted record with the reason written down: "this function belongs
// to the event record and moves to test_event when that module is done; it is
// here because that module is not."  That module is this one, and it is done,
// so the interim ends -- house style §10 restored, and it joins the other
// eighteen definitions in the .c instead of being the one that is not there.
//
//   THE COERCION RULE IS STATED HERE ONCE.  d_test_event_status_change (§VII.c)
// applies the same rule to the two status slots it fills, and its own comment
// used to be the second place it was written down.  Two records of "what
// happened to this node" that disagreed about what an out-of-range status
// means would be a divergence between two of DTest's own modules -- which the
// oracle cannot see, because both are on the same side of it.
//
//   ONE MODULE STILL DISAGREES, AND IT IS NOT THIS ONE.
// d_test_object_set_status applies the rule only when
// D_CFG_TEST_OBJECT_VALIDATE_STATUS is 1, so at 0 a node stores 9999 while an
// event built from it stores `error`.  That knob is out of this module's
// scope; see the log.

struct d_test_event d_test_event_make(d_test_event_id _event,
                                      int32_t         _status,
                                      const char*     _message,
                                      const void*     _context);

// -----------------------------------------------------------------------------
//   b.  THE ALPHABET
// -----------------------------------------------------------------------------
//   kind_name returns the C++ TAG spelling; a kind outside the built-in range
// returns "custom", because a user tag's name lives with the user tag and the
// kernel has nowhere to keep it.

const char* d_test_event_kind_name(enum d_test_event_kind _kind);
int d_test_event_arity(enum d_test_event_kind _kind);
int d_test_event_kind_is_builtin(int32_t _kind);

// -----------------------------------------------------------------------------
//   c.  PAYLOAD CONSTRUCTION
// -----------------------------------------------------------------------------
//   ONE CONSTRUCTOR, because there is one shape.  The six shape-specific
// constructors are gone with the slots they filled.
//
//   THE CALLER OWNS THE ARGUMENTS.  A payload borrows; it does not copy.  Fire
// is synchronous, so a stack temporary in the calling frame is sufficient and
// is the expected form:
//
//       struct { const void* node; int32_t before; int32_t after; } a;
//
//       a.node   = node;
//       a.before = d_test_status_or_error(before);
//       a.after  = d_test_status_or_error(after);
//
//       struct d_test_event_payload p =
//           d_test_event_payload_make(D_TEST_EVENT_STATUS_CHANGE, &a);
//       d_test_dispatch_fire(&dispatch, &p);
//
//   A listener that RETAINS the payload past the fire retains a pointer into a
// frame that has returned.  The old payload carried its scalars inline and so
// survived a copy; this one does not, and that is the one behavioural
// difference a caller can be bitten by.

struct d_test_event_payload d_test_event_payload_make(enum d_test_event_kind _kind,
                                                      const void*            _context);

//   ARGUMENT BUILDERS.  Present only where building the struct is not merely
// filling it in -- which is once.  The node-scoped kinds take the node pointer
// directly, and the other three shapes are brace-initialised at the fire site.

//   d_test_event_status_change IS GONE.  It built on_status_change's arguments
// and applied the coercion rule to both status values, and it existed as a
// function rather than a brace initialiser so that the rule was written in one
// place.  Its return type was one of the four structs, so it could not survive
// them.
//
//   THE RULE DID SURVIVE, AND MOVED SOMEWHERE BETTER: d_test_status_or_error in
// test_common.h.  A coercion policy for d_test_status belongs in the module
// that owns d_test_status, where all three modules that apply it can reach one
// definition -- which is more than the old arrangement managed, since
// d_test_object_set_status never shared it.
//
//   IT IS THE SECOND TIME THIS FUNCTION HAS BEEN DELETED and the difference is
// the whole point: the first time, no log recorded it and its contents went
// with it, so a later session spent a revision working out what it had been.
// This time the deletion is logged, the reason is here, and the behaviour it
// carried is named and reachable.

// -----------------------------------------------------------------------------
//   e.  DISPATCH
// -----------------------------------------------------------------------------

enum d_test_dispatch_result d_test_dispatch_init(struct d_test_dispatch* _dispatch);
enum d_test_dispatch_result d_test_dispatch_bind_listeners(struct d_test_dispatch* _dispatch,
                                                           struct d_test_listener* _storage,
                                                           size_t                  _capacity);
enum d_test_dispatch_result d_test_dispatch_on(struct d_test_dispatch* _dispatch,
                                               enum d_test_event_kind  _kind,
                                               d_test_listener_fn      _notify,
                                               void*                   _context);
enum d_test_dispatch_result d_test_dispatch_fire(struct d_test_dispatch*            _dispatch,
                                                 const struct d_test_event_payload* _payload);
size_t d_test_dispatch_count_for(const struct d_test_dispatch* _dispatch,
                                 enum d_test_event_kind        _kind);
const char* d_test_dispatch_result_name(enum d_test_dispatch_result _r);


D_EXTERN_C_END


#endif  // DJINTERP_C_TEST_EVENT
