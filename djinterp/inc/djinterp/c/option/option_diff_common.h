/*******************************************************************************
* djinterp [c]                                              option_diff_common.h
*
* The difference algebra over option sets. Tier 0 -- the shared core both faces
* compile.
*
* THE .tex DEFINES THIS SECTION COMPLETELY, AND IN FOUR PARTS.
*
*     changed_pi(Oa, Ob) = { k in K(Oa) ^ K(Ob) | pi(Oa[k]) != pi(Ob[k]) }
*     added(Oa, Ob)      = K(Ob) \ K(Oa)
*     removed(Oa, Ob)    = K(Oa) \ K(Ob)
*     diff_pi(Oa, Ob)    = added u changed_pi u removed
*
* Every one of those is a KEY SET, and the fourth is the union of the other
* three. That last point was missing from the C++ tree and from the first draft
* of this header: both supplied the three components and a COUNT, but never the
* union itself, so `diff_pi` -- the operator the section is named for -- could
* not be obtained.
*
* THE COUNT IS ONLY CORRECT BECAUSE THE THREE PARTS ARE DISJOINT, and that is
* load-bearing rather than incidental. `added` and `removed` are drawn from
* complementary key sets and `changed` from their intersection, so no key can
* appear twice and the union's size is the sum of the parts. If a later edit
* made any part overlap another the count would silently over-report, so it is
* worth a conformance test: |diff| == |added| + |changed| + |removed|.
*
* THE TWO CHARACTERISATIONS, which are why this section connects back to the
* relation algebra rather than sitting beside it:
*
*     changed_pi empty  <=>  the sets AGREE under pi   (pi = id: Oa ~ Ob)
*     diff_pi    empty  <=>  the sets are EQUAL under pi
*
* So agreement and equality are not separate machinery bolted on next to the
* difference -- they ARE the difference, read at two different scopes.
* `d_option_set_agrees` and `d_option_set_equal` in option_set_common.h must
* give the same answers as these, and a conformance test should say so rather
* than trusting that two implementations of one relation happen to concur.
*
* PROJECTION AND EXEMPTION ARE SEPARATE AXES, AND THE .tex INSISTS.
*   pi decides WHAT is compared -- the declared value, a default, an effective
* value-or-default. epsilon decides WHICH keys are REPORTED. The document is
* explicit that exemption filters the RESULT rather than threading through the
* comparison, "leaving both independently composable", and that epsilon "need
* not be part of the difference operator itself".
*   So there is one difference operator and a filtered variant of it, not a
* family of operators each with its own idea of what to skip. An earlier draft
* had neither axis: it passed a bare equality comparator, which cannot express
* pi at all, and offered no epsilon.
*
* NO ALLOCATOR, BY THE TWO-CALL PROTOCOL.
*   Every enumerating function takes a destination and its capacity:
*
*     `_out == NULL`   ->  the count is returned, nothing written
*     short buffer     ->  D_OPTION_STATUS_BUFFER_TOO_SMALL, AND the count
*     enough room      ->  the keys are written, and the count
*
*   The required size travels WITH the failure, so one retry always suffices
* and a measure pass can never disagree with a produce pass -- they are the
* same pass. This is the discipline the compress core settled, for the same
* reason.
*
*   ONE DELIBERATE DIVERGENCE FROM THAT PRECEDENT. `compress_common.h` spells
* the protocol `(_out, _out_capacity, _out_size)` -- status returned, count
* written through an out-parameter. These functions return
* `struct d_option_result` instead, which carries the count in its success arm
* and the status in its error arm, so there is no out-parameter at all.
*   That is not a stylistic preference. Stage 3 of the testing roadmap required
* the pack facade to return the `result` carrier and it could not, because
* `result` did not exist in C at the time; the status enum was the documented
* stand-in and the migration was recorded as owed. `result` exists now, so this
* module takes the dependency the roadmap asked for rather than inheriting a
* workaround along with the discipline. The discipline -- a failure carrying
* its own remedy -- is unchanged.
*
*
* path:      /inc/djinterp/c/option/option_diff_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_C_OPTION_OPTION_DIFF_COMMON_H
#define DJINTERP_C_OPTION_OPTION_DIFF_COMMON_H 1

// std
#include <stddef.h>
// djinterp
#include "../djinterp.h"
#include "../functional/functional_common.h"
#include "./option_common.h"
#include "./option_set_common.h"
#include "./option_override_common.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // int32_t, uint32_t, uint64_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN

///////////////////////////////////////////////////////////////////////////////
///             I.    THE KEY-SET DELTA                                     ///
///////////////////////////////////////////////////////////////////////////////
//
//   `added` and `removed` need no projection and no comparator: they are
// defined by set difference on keys alone, and no value is read. Only
// `changed` compares anything.
//
//   Each writes keys in a STATED order, because two runs over the same inputs
// must produce the same sequence for the oracle's diff to mean anything. That
// order is a property of the ENUMERATION, not of the set -- see
// `d_option_set_canon` for the set's own canonical form.

// d_option_set_added_keys
//   function: K(Ob) \ K(Oa) -- keys the delta introduces. Delta's order.
struct d_option_result d_option_set_added_keys(const struct d_option_set* _base,
                                               const struct d_option_set* _delta,
                                               uint64_t*                  _out,
                                               uint32_t                   _capacity);

// d_option_set_removed_keys
//   function: K(Oa) \ K(Ob) -- keys the delta drops. Base's order.
struct d_option_result d_option_set_removed_keys(const struct d_option_set* _base,
                                                 const struct d_option_set* _delta,
                                                 uint64_t*                  _out,
                                                 uint32_t                   _capacity);

// d_option_set_keys
//   function: K(O) -- every key in a set, in its own order. Not one of the
// .tex's operators; it is the enumeration the others are expressed over.
struct d_option_result d_option_set_keys(const struct d_option_set* _set,
                                         uint64_t*                  _out,
                                         uint32_t                   _capacity);


///////////////////////////////////////////////////////////////////////////////
///             II.   CHANGED, AND ITS COMPLEMENT                           ///
///////////////////////////////////////////////////////////////////////////////
//
//   The only part of the difference that reads values, and therefore the only
// part that takes pi. Pass NULL for pi = id.

// d_option_set_changed_keys
//   function: changed_pi -- shared keys whose projections differ. Base's
// order.
//   Empty exactly when the sets AGREE under pi. That is the .tex's first
// characterisation, and it makes this function the cheapest correct
// implementation of agreement rather than a neighbour of one.
struct d_option_result d_option_set_changed_keys(const struct d_option_set* _base,
                                                 const struct d_option_set* _delta,
                                                 uint64_t*                  _out,
                                                 uint32_t                   _capacity,
                                                 fn_option_project          _project,
                                                 fn_binary_predicate        _compare,
                                                 void*                      _context);

// d_option_set_unchanged_keys
//   function: shared keys whose projections agree. Base's order.
//   The complement of `changed` within the key intersection, so
// `|changed| + |unchanged| == |key_intersection|` always -- a second
// disjointness property, worth asserting alongside the one in the file header.
//   Not named in the .tex, which is why it sits here rather than in section
// III: it is a convenience over the document's operators, not one of them.
struct d_option_result d_option_set_unchanged_keys(const struct d_option_set* _base,
                                                   const struct d_option_set* _delta,
                                                   uint64_t*                  _out,
                                                   uint32_t                   _capacity,
                                                   fn_option_project          _project,
                                                   fn_binary_predicate        _compare,
                                                   void*                      _context);


///////////////////////////////////////////////////////////////////////////////
///             III.  THE FULL DIFFERENCE                                   ///
///////////////////////////////////////////////////////////////////////////////

// d_option_set_diff_keys
//   function: diff_pi = added u changed_pi u removed. THE operator this file
// is named for, and the one both the C++ tree and this header's first draft
// omitted.
//
//   Emitted in a stated order -- removed and changed in the base's order, then
// added in the delta's -- so the result is reproducible. The three parts are
// disjoint, so no key appears twice and the count is their sum.
struct d_option_result d_option_set_diff_keys(const struct d_option_set* _base,
                                              const struct d_option_set* _delta,
                                              uint64_t*                  _out,
                                              uint32_t                   _capacity,
                                              fn_option_project          _project,
                                              fn_binary_predicate        _compare,
                                              void*                      _context);

// fn_option_exempt
//   function pointer: the exemption predicate epsilon. Returns true for a key
// that should be OMITTED from the reported difference.
//   It receives both options at that key, and EITHER MAY BE NULL: a key in
// `added` has no base option and one in `removed` has no delta option. So an
// exemption may consult Oa[k] and Ob[k] as the .tex allows, and must handle
// their absence, which is the same thing the difference itself has to do.
// Note: `_context` may be NULL.
typedef bool (*fn_option_exempt)(uint64_t               _key,
                                 const struct d_option* _base_option,
                                 const struct d_option* _delta_option,
                                 void*                  _context);

// d_option_set_diff_keys_exempt
//   function: diff_{pi,epsilon} = { k in diff_pi | !epsilon(k) }.
//
//   A FILTER OVER THE RESULT, not a modification of the comparison, exactly as
// the .tex prescribes: "keeping projection (what is compared) and exemption
// (which keys are reported) on separate axes leaves both independently
// composable". The difference is computed in full and then filtered, so
// changing epsilon cannot change what counts as changed and changing pi cannot
// change what counts as reportable.
//
//   `_exempt` NULL is the unfiltered difference, which makes
// `d_option_set_diff_keys` the epsilon = false instance of this function
// rather than a separate algorithm with its own opportunity to disagree.
struct d_option_result d_option_set_diff_keys_exempt(const struct d_option_set* _base,
                                                     const struct d_option_set* _delta,
                                                     uint64_t*                  _out,
                                                     uint32_t                   _capacity,
                                                     fn_option_project          _project,
                                                     fn_binary_predicate        _compare,
                                                     fn_option_exempt           _exempt,
                                                     void*                      _context);

// d_option_set_diff_count
//   function: |diff_pi|. One walk, no storage, and the measurement every
// enumeration above is sized by.
//   Equal to |added| + |changed| + |removed| by disjointness -- an identity
// that is a conformance test, not a comment.
struct d_option_result d_option_set_diff_count(const struct d_option_set* _base,
                                               const struct d_option_set* _delta,
                                               fn_option_project          _project,
                                               fn_binary_predicate        _compare,
                                               void*                      _context);


///////////////////////////////////////////////////////////////////////////////
///             IV.   THE DIFFERENCE AS DATA                                ///
///////////////////////////////////////////////////////////////////////////////
//
//   Sections I to III answer questions about a difference. This section IS
// one: a flat record of per-key verdicts, which is what a report projects and
// what the parity oracle's parseable row is a fold of.
//
//   It exists because "print the diff" written over the separate enumerations
// walks the sets three times and then has to re-associate the answers by key.
// Written over this record it walks once. The archive work found the same
// shape -- a fifty-field comparison re-derived by hand at every boundary,
// collapsed into one loop over a knob table -- and this is the same fix.
//
//   The verdict set is exactly the .tex's three parts plus the unchanged
// remainder, so every operator above is recovered by filtering these entries
// on `verdict`. That is the check that this record has not drifted from the
// document: if a verdict existed that no operator selects, or an operator that
// no verdict expresses, one of the two would be wrong.

#define D_OPTION_DIFF_UNCHANGED     ((int32_t)0)
#define D_OPTION_DIFF_CHANGED       ((int32_t)1)
#define D_OPTION_DIFF_ADDED         ((int32_t)2)
#define D_OPTION_DIFF_REMOVED       ((int32_t)3)

// d_option_diff_entry
//   struct: one key's verdict. Sixteen bytes, no padding, and the verdict is
// `int32_t` rather than an enum for the reason option_common.h gives -- an
// enum has implementation-defined size and this record reaches a wire format.
struct d_option_diff_entry
{
    uint64_t key;       // the key this verdict is about
    int32_t  verdict;   // D_OPTION_DIFF_*
    uint32_t reserved;  // written zero; keeps the entry 16 bytes and aligned
};

// d_option_set_diff
//   function: the full account as entries -- removed, changed and unchanged in
// the base's order, then added in the delta's. Same two-call protocol.
//
//   Note this reports UNCHANGED keys, which `diff_pi` does not. That is
// deliberate: the record is a complete account of both sets and the operator
// is recovered by dropping the unchanged rows. A report wants the complete
// account, the operator wants the filtered one, and computing the former once
// is cheaper than computing the latter and then going back for the rest.
struct d_option_result d_option_set_diff(const struct d_option_set*  _base,
                                         const struct d_option_set*  _delta,
                                         struct d_option_diff_entry* _out,
                                         uint32_t                    _capacity,
                                         fn_option_project           _project,
                                         fn_binary_predicate         _compare,
                                         void*                       _context);

// d_option_diff_verdict_name
//   function: the verdict's stable spelling, for reports and for the parity
// oracle. Never NULL.
const char* d_option_diff_verdict_name(int32_t _verdict);


///////////////////////////////////////////////////////////////////////////////
///             V.    LAYOUT ASSERTIONS                                     ///
///////////////////////////////////////////////////////////////////////////////

D_STATIC_ASSERT(sizeof(struct d_option_diff_entry) == 16,
                "d_option_diff_entry layout drift: expected 16 bytes");
D_STATIC_ASSERT(offsetof(struct d_option_diff_entry, verdict) == 8,
                "d_option_diff_entry layout drift: verdict");
D_STATIC_ASSERT(offsetof(struct d_option_diff_entry, reserved) == 12,
                "d_option_diff_entry layout drift: reserved");


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_OPTION_OPTION_DIFF_COMMON_H
