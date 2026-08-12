/******************************************************************************
* djinterp [test]                                         test_object.h
*
*   The shared test NODE: type identity, result, status, a deferred-callable
* handle, and a metadata container.  Compiled by BOTH C and C++.  Step E of
* the relay -- the last leaf before the tree.
*
*
* WHAT A NODE IS, AND WHAT IT DELIBERATELY IS NOT
* ===============================================
*   test_object.hpp is unusually explicit about its own negative space, and
* every one of those decisions survives coalescence unchanged because they are
* semantic rather than notational:
*
*     NO UNIQUE ID AND NO DEPTH.  Both are facts of a node's POSITION in the
*     owning tree, not properties of the node.  Structural identity is the
*     node's address -- its path from the root -- and depth is that path's
*     length; the handler computes and tracks them during the walk.  A node is
*     anonymous.  This is why the parity body for a tree can record a node's
*     path without the node having ever carried one.
*
*     NO CLASSIFICATION.  Leaf-vs-interior is resolved through a test_kind set
*     held by the tree.  In isolation the type_id acts as the rank.
*
*     NO INLINE CLOSURE.  A node carries a callable ID -- a 1-based index into
*     an out-of-line table -- and never the thunk.  That is what keeps the node
*     flat, trivially copyable and memcpy-able, and it is the same reason the
*     framework's payload bytes use offsets rather than pointers.
*
*
* THE METADATA CONTAINER IS BORROWED ROWS
* =======================================
*   This is the one real decision in the step, and it is the substrate wall
* arriving three steps early.
*
*   The C++ default container is basic_metadata<> -- a std::string-keyed,
* std::vector-backed key/value store.  Its own header specifies the contract:
* a back-insertable sequence of kv_pair rows, LOOKUP BY LINEAR SCAN, keys
* compared with operator==, and get() returning a value-initialised mapped
* type on a miss.  Nothing in that contract requires ownership, ordering, or
* an allocator; it requires a sequence and a scan.
*
*   So the kernel takes the rows as CALLER-PROVIDED STORAGE -- the pattern
* step C fixed for the child arrays, and which d_pack_sink, d_archive_extract
* and d_counter already use -- with borrowed const char* keys and values.  A
* miss returns the empty string rather than null, which is what a
* value-initialised std::string is, so the two languages agree about the miss
* case without either one special-casing it.
*
*   WHAT IS LOST IS OWNERSHIP, AND IT IS LOST ON PURPOSE.  A node cannot own a
* name it computed at runtime until d_string exists at step J.  Every name in
* a suite is a literal in the source, because a suite is a spec and a spec is
* written down -- so the bound is knowable, exactly as the child-array bound
* is.  When d_string lands, an owning container becomes one more way to supply
* the rows and nothing in this header changes.
*
*   THE ALTERNATIVE WAS TO BLOCK THIS STEP ON J, and it was rejected for the
* reason the roadmap gives for the spine generally: it would strand test_tree,
* test_handler and test_session behind three thousand lines none of them need.
*
*
* STATUS IS CLAIMED TWICE, AGAIN
* ==============================
*   test_object.hpp declares its own status constants -- status_passed = 0
* through status_error = 4 -- as static members of the template, independent
* of test_common's D_TEST_STATUS_* macros.  They agree today.  Nothing made
* them agree, and nothing would have reported it if they stopped: the node's
* constants are read at the node's call sites and the macros at everyone
* else's, so a drift would surface as a status that reported wrong in exactly
* one of the two vocabularies.
*
*   The static assertions below tie them, and the parity body at step F must
* record both sides.  This is the third instance of roadmap §3's corollary in
* four modules, which is less a coincidence than a characteristic of the
* codebase: it states things twice, in good faith, and never checks.
*
*
* path:      /inc/djinterp/c/test/test_object.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.31
******************************************************************************/

#ifndef DJINTERP_C_TEST_OBJECT
#define DJINTERP_C_TEST_OBJECT 1

// c
#include <stddef.h>
#include <stdint.h>
// djinterp
#include "../djinterp.h"
#include "../../config/c/test/cfg_test_object.h"   // D_INTERNAL_TEST_OBJECT_*
#include "./test_common.h"             // status, type_id, callable_id


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


// =============================================================================
// I.   RESULT
// =============================================================================


// d_test_object_result
//   enum: the outcome of a node or metadata operation.  Pinned; recorded by
// name.
enum d_test_object_result
{
    D_TEST_OBJECT_OK        = 0,
    D_TEST_OBJECT_FULL      = 1,    // no room in the caller's row array
    D_TEST_OBJECT_INVALID   = 2,    // null argument, or a null key
    D_TEST_OBJECT_REPLACED  = 3     // set() overwrote an existing key
};

// D_TEST_OBJECT_RESULT_COUNT
//   constant: the number of enumerators in d_test_object_result.
#define D_TEST_OBJECT_RESULT_COUNT  4

//   REPLACED is reported rather than folded into OK because a suite that
// sets the same metadata key twice is usually a builder bug, and a caller
// counting them needs the distinction.  It is not an error.


// =============================================================================
// II.  THE NODE
// =============================================================================

// d_test_object
//   struct: one test element.  Plain data, trivially copyable, identical
// under both languages.
//
//   `result` is int32_t rather than a bool member, for the reason
// test_common.h gives about enum members: _Bool and bool have the same
// size on every target this framework has met, but neither standard requires
// it, and this struct's layout is recorded and compared.  Widening it costs
// three bytes and removes a question.
//
//   THE NODE HOLDS NO POINTERS AT ALL, and that is the point of it.  Four
// scalars: memcpy-safe, memcmp-comparable, no borrowed storage, no lifetime
// coupled to anything, and copying one cannot alias.
//
//   METADATA IS NOT HERE AND THIS HEADER KNOWS NOTHING ABOUT IT.  It was an
// embedded member, then a borrowed pointer (revision.md 6, so the layout would
// stop depending on the container's), and it is now an ASSOCIATION the owning
// tree keeps -- beside the unique id and the depth this node already declines
// to carry, for the same reason: those are facts of position, not properties
// of the node.
//
//   THE TWO FORKS HAD DRIFTED PAST REPAIR, which is what settled it.  C stored
// uint32 -> void*, borrowed and never freed. C++ stored std::string ->
// std::string, owning and allocating. Same field name, different data
// structure, opposite ownership -- and no parity relation left between them
// once the C key became an interned id. Decoupling is what lets the C++ tree
// use RAII properly instead of mirroring a borrowed pointer it gains nothing
// from.
struct d_test_object
{
    bool                    result;         // the verdict, true or false
    int32_t                 status;         // a d_test_status value
    d_test_type_id        type_id;        // int32_t
    d_test_callable_id    callable_id;    // uint32_t; 0 == none
};

// D_TEST_OBJECT_INIT
//   macro: a pending, untyped node with no deferred work and no metadata.
// Equal in every member to what d_test_object_init produces.
#define D_TEST_OBJECT_INIT                                                     \
{                                                                              \
    false, D_TEST_STATUS_PENDING, (d_test_type_id)0,                             \
    (d_test_callable_id)0                                                    \
}


// =============================================================================
// III. LAYOUT AND VALUE ASSERTIONS
// =============================================================================
//   The four scalars are fixed-width and are asserted exactly.  The metadata
// container is pointer-bearing and is asserted for ORDER only.

//   `result` IS A bool, AND THAT NEEDS AN ASSERTION THE OTHERS DID NOT.
// C's _Bool and C++'s bool are distinct types with IMPLEMENTATION-DEFINED
// size. Every mainstream ABI makes both one byte, but the standard does not,
// and this struct is the one thing the two forks must agree on byte for byte.
// The status field is int32_t rather than the enum for exactly this reason --
// the same argument reaches bool, so it gets the same treatment: a platform
// where the assumption fails must fail HERE, not in a parity row.
D_STATIC_ASSERT(sizeof(bool) == 1,
                "d_test_object: bool must be one byte for the two forks to "
                "agree on this layout");

//   The offsets were written as multiples of sizeof(int32_t) when every field
// in the block was one. They are stated outright now: with a one-byte result
// followed by three bytes of padding the arithmetic still happened to come
// out, which is the kind of accident that survives until it does not.
D_STATIC_ASSERT(offsetof(struct d_test_object, result) == 0,
                "d_test_object: field drift at result");
D_STATIC_ASSERT(offsetof(struct d_test_object, status) == 4,
                "d_test_object: field drift at status");
D_STATIC_ASSERT(offsetof(struct d_test_object, type_id) == 8,
                "d_test_object: field drift at type_id");
D_STATIC_ASSERT(offsetof(struct d_test_object, callable_id) == 12,
                "d_test_object: field drift at callable_id");
D_STATIC_ASSERT(sizeof(struct d_test_object) == 16,
                "d_test_object: the node is four scalars and nothing else");
//   The sixth assertion that stood here -- count must follow rows -- moved to
// test_metadata.h with the struct it describes.  Five of the six hold
// unchanged; this header can no longer state that one, because it no longer
// knows what a d_test_metadata is.

//   THE PAIR test_object.hpp NEVER CHECKED.  Its per-instantiation status
// constants against test_common's macro set.  Both are written by hand in
// different files; until now nothing compared them.
D_STATIC_ASSERT(D_TEST_STATUS_PASSED  == 0, "test_object: passed must be 0");
D_STATIC_ASSERT(D_TEST_STATUS_FAILED  == 1, "test_object: failed must be 1");
D_STATIC_ASSERT(D_TEST_STATUS_SKIPPED == 2, "test_object: skipped must be 2");
D_STATIC_ASSERT(D_TEST_STATUS_PENDING == 3, "test_object: pending must be 3");
D_STATIC_ASSERT(D_TEST_STATUS_ERROR   == 4, "test_object: error must be 4");


// =============================================================================
// IV.  OPERATIONS
// =============================================================================

//   THE SEVEN d_test_metadata_* OPERATIONS ARE DECLARED IN
// test_metadata.h, not here.  A caller that wants both includes both.

//   THE NODE.  evaluate() writes the boolean verdict and derives the status
// from it -- passed or failed, never skipped or pending, because those two are
// states a verdict cannot express and are set explicitly.

enum d_test_object_result d_test_object_init(struct d_test_object* _object);
enum d_test_object_result d_test_object_init_typed(struct d_test_object* _object,
                                                   d_test_type_id        _type_id);
enum d_test_object_result d_test_object_evaluate(struct d_test_object* _object,
                                                 int                   _result);
enum d_test_object_result d_test_object_set_status(struct d_test_object* _object,
                                                   int32_t               _status);
enum d_test_object_result d_test_object_set_type_id(struct d_test_object* _object,
                                                    d_test_type_id        _type_id);
enum d_test_object_result d_test_object_set_callable_id(struct d_test_object* _object,
                                                        d_test_callable_id    _id);

bool d_test_object_result_of(const struct d_test_object* _object);
int32_t d_test_object_status(const struct d_test_object* _object);
int d_test_object_passed(const struct d_test_object* _object);
int d_test_object_is_deferred(const struct d_test_object* _object);
const char* d_test_object_result_name(enum d_test_object_result _result);

//   CONSTRUCTION HELPERS.  The C forms of make_test and make_interior.  They
// return by value because the node is flat and trivially copyable, which is
// the property the whole design is arranged around.

struct d_test_object d_test_make(d_test_type_id _type_id,
                                 int            _result);
struct d_test_object d_test_make_interior(d_test_type_id _type_id);


D_EXTERN_C_END


#endif  // DJINTERP_C_TEST_OBJECT
