/******************************************************************************
* djinterp [test]                                        test_metadata.h
*
*   The metadata container: a flat, borrowed, back-insertable row sequence with
* linear-scan lookup.  Extracted from test_object.h (revision.md §6).
*
*   WHY THIS IS ITS OWN MODULE.  The node used to hold the container BY VALUE,
* which made metadata mandatory: every translation unit that knew what a node
* was also had to know what a row was, and the node's LAYOUT moved when the
* container's did.  The node now holds a POINTER that may be null, so the
* C-side dependency breaks completely -- test_object.h names this module only
* through an incomplete type and test_object.c calls nothing in it.  That is
* the pointer's real payoff and it is invisible in a diff, which is why it is
* written here.
*
*   DIRECTION OF DEPENDENCY.  This header includes test_object.h, not
* the other way round, and that is deliberate: the OPTIONAL module may depend
* on the MANDATORY one, never the reverse.  What it needs from there is
* enum d_test_object_result, which stays with the node (revision.md §5) so the
* ten-plus consumers of D_TEST_OBJECT_OK and friends are untouched.  There is
* no cycle, because the node reaches back only through `struct d_test_metadata;`.
*
* path:      /inc/djinterp/c/test/test_metadata.h
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.08.10
******************************************************************************/

#ifndef DJINTERP_C_TEST_METADATA
#define DJINTERP_C_TEST_METADATA 1

// c
#include <stddef.h>
#include <stdint.h>
// djinterp
#include "../djinterp.h"
#include "../../config/c/test/cfg_test_metadata.h"  // D_CFG/D_INTERNAL_TEST_METADATA_*
#include "./test_object.h"           // enum d_test_object_result


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

//   THE WHOLE MODULE IS OPTIONAL, AND THIS IS THE GATE.  With
// D_CFG_TEST_METADATA off, this header declares nothing: no struct, no
// functions, no macros. The knob was defined and normalised in cfg_test.h from
// the start and read by nothing, so setting it to 0 compiled clean, produced
// byte-identical object code, and reported as supported. It is wired here.
//
//   struct d_test_object's LAYOUT DOES NOT CHANGE EITHER WAY.  Its metadata
// member is a POINTER, and a pointer to an incomplete type is legal whether or
// not this header declared one -- so the node is the same size and shape in
// both configurations. That is why revision.md 6 chose a pointer over an
// embedded member: optionality that alters a shared struct is the one-layout
// hazard of dconfig 6.2, and two TUs disagreeing about sizeof diagnose as
// nothing at all.
#if D_INTERNAL_TEST_METADATA

D_EXTERN_C_BEGIN


// =============================================================================
// I.   THE ROW AND THE CONTAINER
// =============================================================================

// d_test_kv
//   struct: one metadata row.  Both halves are BORROWED -- the key is
// normally a literal and the value usually is too.  This is the C form of
// kv_pair<_Key, _Value> at the framework's default instantiation.
// d_test_key_id
//   type: an interned metadata key. Width from D_CFG_TEST_KEY_ID_TYPE.
typedef D_CFG_TEST_KEY_ID_TYPE d_test_key_id;

D_STATIC_ASSERT(((d_test_key_id)-1) > 0,
                "d_test_key_id: D_CFG_TEST_KEY_ID_TYPE must name an "
                "unsigned integer type");
D_STATIC_ASSERT((sizeof(d_test_key_id) == 1) || (sizeof(d_test_key_id) == 2) ||
                (sizeof(d_test_key_id) == 4) || (sizeof(d_test_key_id) == 8),
                "d_test_key_id: width must be 1, 2, 4 or 8 bytes");

// D_TEST_NO_KEY
//   constant: the reserved zero key. A row never carries it and set() refuses
// it, so it doubles as "absent" without a second sentinel. Same convention as
// D_TEST_NO_CALLABLE.
#define D_TEST_NO_KEY   ((d_test_key_id)0)

// d_test_kv
//   struct: one interned key and one BORROWED, UNTYPED value. The container
// stores the pointer and never dereferences, copies or frees it -- what it
// points at, how long it lives and what type it really is are all the caller's
// business.
struct d_test_kv
{
    d_test_key_id key;
    void*           value;      // borrowed; may be null
};

// d_test_metadata
//   struct: a flat, back-insertable row sequence with linear-scan lookup.
// Borrowed storage, never owned, never grown -- set() on a full container
// returns FULL rather than reallocating.
//
//   ORDER IS INSERTION ORDER and that is a contract, not an accident: a
// projection that walks metadata must produce the same sequence in both forks,
// and a container that sorted or hashed would make the walk's output a
// property of the hash rather than of the suite.
struct d_test_metadata
{
    struct d_test_kv*   rows;       // borrowed
    size_t              count;
    size_t              capacity;
};

// D_TEST_METADATA_INIT
//   macro: an empty, unbound metadata container.
#define D_TEST_METADATA_INIT                                                   \
{                                                                              \
    (struct d_test_kv*)0, (size_t)0, (size_t)0                                 \
}

// D_TEST_METADATA_MISS
//   constant: what get() returns for a key that is not present.
#define D_TEST_METADATA_MISS    ((void*)0)

//   THE NEVER-NULL CONTRACT IS GONE, AND IT COULD NOT SURVIVE THIS CHANGE.
// MISS was "" -- an empty string, matching what a value-initialised
// std::string is, so the C++ face's container and this one agreed about a miss
// without converting, and get() could be printed without a null test. A void*
// has no empty value. Null is the only miss a pointer can express.
//
//   SO get() CAN NOW RETURN NULL, and two different situations return it: the
// key is absent, and the key is present with a null value. THAT IS WHAT
// contains() IS FOR -- it was already the documented way to tell those apart
// back when the distinction was rarer. It is now the only way, and every caller
// that reads a value it did not itself store needs it.
//
//   This supersedes a decision taken two revisions ago. get()-on-null-container
// was changed to return MISS specifically to preserve the never-null contract.
// That contract no longer exists, so the reasoning behind it is void; the
// BEHAVIOUR is unchanged, because MISS is now null and a null container still
// reads as empty.


// =============================================================================
// II.  LAYOUT ASSERTION
// =============================================================================
//   Moved here with the struct it describes.  The container is pointer-bearing
// and is asserted for ORDER only.

D_STATIC_ASSERT(offsetof(struct d_test_metadata, count) >
                offsetof(struct d_test_metadata, rows),
                "d_test_metadata: count must follow rows");


// =============================================================================
// III. OPERATIONS
// =============================================================================

//   set() replaces an existing key in place rather than appending a second
// row, so lookup stays unambiguous under a linear scan; the previous value is
// simply overwritten, and REPLACED says so.
//
//   THE NULL CONTAINER.  Every one of these accepts a null container, because
// the node's member is now a pointer and a node that was never pointed at one
// is the ordinary case rather than an error.  A null container READS AS EMPTY
// and REFUSES WRITES: count 0, contains 0, row null, set INVALID.
//
//   get() FOLLOWS THE SAME RULE: a null container returns D_TEST_METADATA_MISS,
// which is what an empty-but-bound container returns and what the delivered
// kernel returned.  Nothing get() can return is ever null, so no caller needs a
// null test.
//
//   A WRONG TURN, RECORDED BECAUSE IT COST A ROUND TRIP.  revision.md 6's table
// briefly specified a null return here.  It was wrong on three counts and the
// rev02 session was right to flag it rather than implement it quietly: it
// contradicted the summary sentence directly above it ("a null container reads
// as empty" -- and an EMPTY container's get returns MISS); it broke
// D_TEST_METADATA_MISS' standing contract ("Never null: a caller printing the
// result would then have to test it, and half of them would not"); and it was a
// silent change to EXISTING behaviour, since the delivered get() already
// accepted a null container.  The table was written without reading the
// delivered function.  It is reverted at the source; injection G is the pin.

enum d_test_object_result d_test_metadata_init(struct d_test_metadata* _metadata);
enum d_test_object_result d_test_metadata_bind(struct d_test_metadata* _metadata,
                                               struct d_test_kv*       _storage,
                                               size_t                  _capacity);
enum d_test_object_result d_test_metadata_set(struct d_test_metadata* _metadata,
                                              d_test_key_id           _key,
                                              void*                   _value);
void* d_test_metadata_get(const struct d_test_metadata* _metadata,
                          d_test_key_id                 _key);
int d_test_metadata_contains(const struct d_test_metadata* _metadata,
                             d_test_key_id                 _key);
size_t d_test_metadata_count(const struct d_test_metadata* _metadata);
const struct d_test_kv* d_test_metadata_row(const struct d_test_metadata* _metadata,
                                            size_t                        _index);


D_EXTERN_C_END

#else   // D_INTERNAL_TEST_METADATA

//   OFF.  struct d_test_metadata is left INCOMPLETE rather than undeclared, so
// test_object.h's pointer member still names a type and every consumer that
// only stores or forwards the pointer keeps compiling. A consumer that tries to
// DEREFERENCE it gets an incomplete-type error naming this file, which is the
// diagnostic you want -- not an unknown-identifier error three headers away.
struct d_test_metadata;

#endif  // D_INTERNAL_TEST_METADATA


#endif  // DJINTERP_C_TEST_METADATA
