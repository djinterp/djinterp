/*******************************************************************************
* djinterp [c]                                          option_override_common.h
*
* The merge engine: two sets in, one set out, under a policy. Tier 0 -- the
* shared core both faces compile.
*
* THE WALK IS THE SAME WALK IN BOTH LANGUAGES.
*   `option_override.hpp` states the engine in three lines, and they are
* language-neutral:
*
*     for each option in A:
*         if its key is in B  ->  policy.on_both(A_opt, B_opt)
*         else                ->  policy.on_base_only(A_opt)
*     for each option in B whose key is NOT in A:
*                             ->  policy.on_delta_only(B_opt)
*
*   That is a fold over two flat sequences with a policy parameter. C++ runs it
* at translation time over types; C runs it at run time over cells. Phase is
* not semantics -- the answer is the same set either way, and the differential
* test is over the answer.
*
* A POLICY IS A DICTIONARY.
*   The C++ policy is a type with three nested template aliases. Its C image is
* the standard dictionary encoding: a struct of function pointers plus a
* context. This is exactly what functional_types.tex prescribes for a protocol
* at kind `*`, and it is mechanical rather than clever -- there is no higher-
* kinded structure here to lose.
*
* THE LAZINESS IS FREE IN C, AND THAT IS THE POINT.
*   The C++ engine carries a `lazy_delta_only` SFINAE wrapper whose only job is
* to stop `on_delta_only` being INSTANTIATED for keys that are not actually
* delta-only -- otherwise `strict`'s static_assert fires for every key, and the
* policy concept probe fires it again. None of that machinery has a C analogue,
* because a C loop calls a function pointer only when it reaches the branch
* that calls it. The elaborate half of the C++ header is notation for an
* eagerness problem C does not have. Recording this is worth more than the code
* it saves: it is the clearest small example in the subframework of the
* framework's central claim, and a reader who does not know it will look for
* the missing laziness and conclude the C side is incomplete.
*
* WHAT HAS NO C FORM, AND CORRECTLY SO.
*   `arg_union_delta` merges two options' ARG PACKS -- it concatenates types.
* Args are opaque types that carry no runtime bytes, so they do not survive
* lowering and there is nothing in C for the policy to concatenate. Per goal
* 11 this is declared C++-only rather than approximated: an approximation would
* be a second implementation of an object the core does not contain. The same
* reasoning retires `merge_args_union` from the shared surface.
*   What DOES survive lowering is the value, and `d_option_policy_replace`
* takes the delta's value. That is the operation a C caller wanted.
*
* THE STRICT POLICY ERRORS RATHER THAN ASSERTING.
*   C++ `strict_subset` fires a static_assert on an extension, which is a
// `-D_OPTION_STATUS_POLICY_REJECTED`, and the engine stops and reports
// it. Keeping verdicts non-negative and errors negative means one
// comparison distinguishes them, and no verdict can collide with a status.
* formal, so it lands in the formal status range and a caller can tell it from
* a full buffer. Same invariant, different enforcement tier, per goal 2.
*
*
* path:      /inc/djinterp/c/option/option_override_common.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.30
*                                                            revised: 2026.10.03
*******************************************************************************/

#ifndef DJINTERP_C_OPTION_OPTION_OVERRIDE_COMMON_H
#define DJINTERP_C_OPTION_OPTION_OVERRIDE_COMMON_H 1

// std
#include <stddef.h>
// djinterp
#include "../djinterp.h"
#include "./option_common.h"
#include "./option_set_common.h"
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // int32_t, uint32_t

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)

D_EXTERN_C_BEGIN


///////////////////////////////////////////////////////////////////////////////
///             I.    THE VERDICT                                           ///
///////////////////////////////////////////////////////////////////////////////

// D_OPTION_KEEP
//   constant: the policy produced an option; append it to the result. The hook
// has written `_out` and, when the option carries a value, `_out_value`.
#define D_OPTION_KEEP       ((int32_t)0)

// D_OPTION_DROP
//   constant: the policy filtered this position out. The C++ image is the
// `dropped` sentinel, and it means the same thing: no error, no output.
#define D_OPTION_DROP       ((int32_t)1)

//   Any NEGATIVE return is a status from option_common.h section I, negated.
// A policy that refuses an input -- `strict` meeting an extension -- returns
// `-D_OPTION_STATUS_POLICY_REJECTED`, and the engine stops and reports it. Keeping
// verdicts non-negative and errors negative means one comparison distinguishes
// them, and no verdict can ever collide with a status.
#define D_OPTION_VERDICT_IS_ERROR(v)    ((int32_t)(v) < (int32_t)0)
#define D_OPTION_VERDICT_STATUS(v)      ((int32_t)(-(int32_t)(v)))


///////////////////////////////////////////////////////////////////////////////
///             II.   THE POLICY DICTIONARY                                 ///
///////////////////////////////////////////////////////////////////////////////

// fn_option_on_both
//   function pointer: the key appears in BOTH sets. Decide what the result
// holds at that key.
//
//   THE HOOK DOES NOT MOVE BYTES. It writes the winning CELL into `_out` and
// points `*_out_bytes` at that cell's value in its own set; the engine then
// allocates the destination slot and copies. An earlier contract had the hook
// write the bytes into a buffer, which cannot work on a fixed-storage tier:
// the buffer would have to be at least as wide as the widest slot in the set,
// which is not known until the walk is over, and the engine has no allocator
// to grow one. Borrowing a pointer needs no storage at all.
//
//   A policy that SYNTHESISES a value rather than electing one keeps the bytes
// in its own `context` and points `*_out_bytes` there. None of the five named
// policies does; all five elect.
//
//   `*_out_bytes` may be left NULL for a unary cell, and is ignored when the
// verdict is D_OPTION_DROP.
// Note: `_context` may be NULL.
typedef int32_t (*fn_option_on_both)(const struct d_option* _base,
                                     const unsigned char*   _base_values,
                                     const struct d_option* _delta,
                                     const unsigned char*   _delta_values,
                                     struct d_option*       _out,
                                     const void**           _out_bytes,
                                     void*                  _context);

// fn_option_on_base_only
//   function pointer: the key is in the base and not in the delta.
// Note: `_context` may be NULL.
typedef int32_t (*fn_option_on_base_only)(const struct d_option* _base,
                                          const unsigned char*   _base_values,
                                          struct d_option*       _out,
                                          const void**           _out_bytes,
                                          void*                  _context);

// fn_option_on_delta_only
//   function pointer: the key is in the delta and not in the base -- an
// EXTENSION. Called only for keys that genuinely are one, which is where the
// C++ side needs its SFINAE wrapper and C needs nothing.
// Note: `_context` may be NULL.
typedef int32_t (*fn_option_on_delta_only)(const struct d_option* _delta,
                                           const unsigned char*   _delta_values,
                                           struct d_option*       _out,
                                           const void**           _out_bytes,
                                           void*                  _context);

// d_option_policy
//   struct: the three hooks plus their shared context -- the C image of a C++
// override policy. A NULL hook means "drop", so a policy that only cares about
// one branch is three words and two zeroes rather than three stubs.
struct d_option_policy
{
    fn_option_on_both       on_both;
    fn_option_on_base_only  on_base_only;
    fn_option_on_delta_only on_delta_only;
    void*                   context;
};

// d_option_policy_is_valid
//   function: a policy is usable if at least one hook is non-NULL. An all-NULL
// policy drops everything, which is almost certainly a mistake rather than an
// intention, so it is rejected here rather than silently returning empty.
bool     d_option_policy_is_valid(const struct d_option_policy* _policy);


///////////////////////////////////////////////////////////////////////////////
///             III.  THE NAMED POLICIES                                    ///
///////////////////////////////////////////////////////////////////////////////
//
//   The five policies that lower cleanly, each the exact image of the C++
// alias named beside it. Returned by value from a function rather than exposed
// as a mutable global, so no translation unit can reach in and edit one.

// d_option_policy_replace
//   policy: the delta wins at shared keys; base-only kept; extensions added.
// C++ `override_replace` (= keep_delta). The default, because "add this
// option" most naturally means "this option now holds for this key".
struct d_option_policy d_option_policy_replace(void);

// d_option_policy_keep
//   policy: the base wins at shared keys; extensions still added.
// C++ `override_keep` (= keep_base).
struct d_option_policy d_option_policy_keep(void);

// d_option_policy_subset
//   policy: the delta wins at shared keys; extensions DROPPED.
// C++ `override_subset` (= drop_extras).
struct d_option_policy d_option_policy_subset(void);

// d_option_policy_strict
//   policy: the delta wins at shared keys; an extension is an ERROR.
// C++ `override_strict` (= strict_subset), where it is a static_assert. Here
// it is D_OPTION_STATUS_POLICY_REJECTED, which is formal -- so a caller inspecting
// the range learns the merge was ill-formed rather than under-provisioned.
struct d_option_policy d_option_policy_strict(void);

// d_option_policy_filter
//   policy: keep only keys the delta also has -- base-only positions dropped,
// extensions added. C++ `override_filter` (= drop_unmatched_base).
struct d_option_policy d_option_policy_filter(void);


///////////////////////////////////////////////////////////////////////////////
///             IV.   MERGE MODES                                           ///
///////////////////////////////////////////////////////////////////////////////
//
//   The coarse vocabulary the runtime half of option_diff.hpp speaks. It lives
// here rather than with the diff because it selects a POLICY, and a mode that
// named a policy from a different header would be the one place the two
// vocabularies could drift apart.

#define D_OPTION_MERGE_OVERWRITE        ((int32_t)0)
#define D_OPTION_MERGE_ADD_NEW_ONLY     ((int32_t)1)
#define D_OPTION_MERGE_KEEP_EXISTING    ((int32_t)1)

// d_option_merge_mode_policy
//   function: the policy a merge mode names. OVERWRITE selects replace;
// ADD_NEW_ONLY and KEEP_EXISTING both select keep -- they are one mode with
// two spellings, and share a value here so the two halves cannot disagree
// about whether they are the same thing. In every mode extensions are added,
// matching the runtime merge, which always inserts new keys.
struct d_option_policy d_option_merge_mode_policy(int32_t _mode);


///////////////////////////////////////////////////////////////////////////////
///             V.    THE ALGEBRAIC LAWS                                    ///
///////////////////////////////////////////////////////////////////////////////
//
//   body-options.tex proves three things about the precedence-resolved union
// `(+)`, and the framework RELIES on all three while recording none of them.
// Goal 1's Conformance law says every claim a .tex makes has a test, so each
// is stated here with the test it owes.
//
//   1. MONOID. "(+) is associative with identity {}, so (option sets, (+), {})
//      form a monoid -- non-commutative in general."
//
//          (A (+) B) (+) C  ==  A (+) (B (+) C)
//          {} (+) A  ==  A  ==  A (+) {}
//
//      This is not decoration. `d_option_set_compose` -- and the C++ face's
//      `compose_options_t` -- is a LEFT FOLD, and a left fold over a binary
//      operator is only well defined because the operator is associative. The
//      property is being leaned on at every call site and was written down
//      nowhere.
//
//   2. COMMUTATIVITY IFF AGREEMENT. "A (+) B == B (+) A if and only if A ~ B."
//
//      The proof is short and worth keeping in view: off the overlap each side
//      is forced to the unique present option, so order cannot matter there;
//      on the overlap A (+) B yields B[k] while B (+) A yields A[k], and those
//      agree for every shared k exactly when A ~ B.
//
//      The practical reading is that `(+)` only ever "elects a winner" on
//      keys where the sets conflict -- which is the same sentence as
//      "`u` is the agreement-restricted total of `(+)`" from
//      option_set_common.h section VI, seen from the other side.
//
//   3. NON-COMMUTATIVITY IS A STATED NON-PROPERTY. "non-commutative in
//      general, by the proposition." So the conformance suite must exhibit a
//      pair for which A (+) B != B (+) A, for the same reason it must exhibit
//      the non-transitivity of agreement: to stop a later maintainer
//      "simplifying" the operator into a commutative one and finding that
//      every test still passes.
//
//   These are declared rather than merely commented, so the suite calls a
// function instead of open-coding the property and drifting from it.

// d_option_set_is_identity
//   function: whether a set is the monoid identity -- the empty set. Trivial,
// and named so the identity law can be asserted against the document's notion
// rather than against `count == 0`, which is an implementation fact that
// happens to coincide today.
bool     d_option_set_is_identity(const struct d_option_set* _set);

// d_option_set_commutes
//   function: whether `(+)` commutes on this pair -- which, by the .tex's
// proposition, is exactly whether they agree. Implemented AS the agreement
// test rather than by computing both folds and comparing: the proposition says
// the two questions are the same question, so answering it twice would create
// the possibility of two answers.
bool     d_option_set_commutes(const struct d_option_set* _lhs,
                               const struct d_option_set* _rhs,
                               fn_option_project          _project,
                               fn_binary_predicate        _compare,
                               void*                      _context);


///////////////////////////////////////////////////////////////////////////////
///             VI.   THE ENGINE                                            ///
///////////////////////////////////////////////////////////////////////////////

// d_option_set_override
//   function: `_out` becomes `_base` overridden by `_delta` under `_policy`.
//
//   Under `d_option_policy_replace` this is EXACTLY the .tex's `(+)`:
//
//       K(A (+) B) = K(A) u K(B)
//       (A (+) B)[k] = B[k] if k in K(B), else A[k]
//
//   The other policies are not `(+)`. They are engine configurations that
// answer a different question -- keep the base, drop extensions, reject them --
// and none of them is claimed to satisfy the laws in section V, which are
// stated for `(+)` alone. A conformance test must therefore instantiate the
// monoid laws against `d_option_policy_replace` and not against the engine in
// general, or it will fail for reasons that are correct.
//
//   ORDER IS REPRODUCIBLE, NOT SEMANTIC. The result preserves the base's order
// for keys the base has, with policy-permitted extensions appended in the
// delta's order -- so two runs over the same inputs produce the same
// representation, which is what a line-by-line diff needs.
//   It is NOT part of the answer. An option set is a set; a permutation of
// this result denotes the same object and the .tex's equality says so. Where a
// comparison must not see the representation, canonicalise first --
// `d_option_set_canon` is the parity law's `canon` and exists for exactly
// this.
//
//   `_out` must be distinct from both inputs -- merging in place would have
// the walk reading cells it has already rewritten. Aliasing is rejected with
// D_OPTION_STATUS_INVALID_ARGUMENT rather than diagnosed by the caller later.
//
//   On success the result carries the number of options written. A policy
// error is returned as-is, so `strict` meeting an extension surfaces as
// D_OPTION_STATUS_POLICY_REJECTED and not as a generic failure.
struct d_option_result d_option_set_override(struct d_option_set*          _out,
                                             const struct d_option_set*    _base,
                                             const struct d_option_set*    _delta,
                                             const struct d_option_policy* _policy);

// d_option_set_merge
//   function: fold `_delta` into `_target` in place under a merge MODE.
//   This is the runtime shape the C++ Part B `option_merge` had, and it is
// deliberately kept alongside the engine rather than derived from it: an
// in-place merge over a mode needs no output set and no second buffer, which
// is what makes it usable on a fixed-storage tier. Returns the number of
// entries inserted or modified.
struct d_option_result d_option_set_merge(struct d_option_set*       _target,
                                          const struct d_option_set* _delta,
                                          int32_t                    _mode);

// d_option_set_compose
//   function: left fold of `_count` deltas into `_base` under one policy --
// the C++ `with_options_t` / `compose_options_t` family lowered.
//   Composition IS repeated override, so this is a loop and not a second
// engine. It needs one scratch set because each step's output is the next
// step's base; `_scratch` must have the same capacity as `_out` and the two
// are swapped between rounds.
struct d_option_result d_option_set_compose(struct d_option_set*          _out,
                                            struct d_option_set*          _scratch,
                                            const struct d_option_set*    _base,
                                            const struct d_option_set**   _deltas,
                                            uint32_t                      _count,
                                            const struct d_option_policy* _policy);


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_OPTION_OPTION_OVERRIDE_COMMON_H
