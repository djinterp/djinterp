/*******************************************************************************
* djinterp [testing]                                          override_tests.hpp
*
*   Unit-test surface for the container-agnostic override-policy module
* (core/paradigm/override.hpp).  This header carries ONLY declarations and
* the fixtures the section TUs share; every `tests_override_*` function is
* defined in its own section .cpp and reported by override_tests_runner.cpp.
*
*   WHAT IS TESTED, AND HOW:
*   override.hpp is pure metaprogramming - its whole observable surface is a
* set of nested template aliases plus one sentinel type.  Every assertion is
* therefore a constant expression, handed to D_OV_CHECK so that a regression
* paints a red line in the run rather than stopping the build.  Two things
* deliberately are NOT stated that way:
*
*     1. `strict_subset::on_delta_only<_D>` cannot be instantiated at all.
*        Its static_assert is unconditional for every type a C++ program can
*        name (no type has sizeof 0), and a failing static_assert is a hard
*        error rather than a substitution failure - it can be neither caught
*        nor probed.  What IS testable is the contract AROUND it: that the
*        OverridePolicy concept never touches it, that a lazy engine never
*        touches it for a key base already carries, and that the alias and
*        its assert_extension helper exist and can be NAMED (bound to a
*        template template parameter) without being instantiated.  Section
*        III states all three; `assert_extension::type` is the one line in
*        the module that is unreachable BY DESIGN.
*
*     2. Out-of-contract policies (a missing on_both, an ill-formed hook)
*        are stated as concept negatives, since the concept is the only
*        place the module makes a checkable claim about them.
*
*   THE MINIATURE ENGINE (section V of this header):
*   override.hpp defines policies but no engine - by design, the engine
* belongs to whichever module owns a container shape.  Reading a policy's
* aliases back therefore only half-tests it; the other half is that the
* three hooks compose into a coherent override when a real walk drives
* them, including the `dropped` filtering and the LAZY on_delta_only the
* header's contract requires of any engine.  ov_apply is that walk, over a
* deliberately trivial keyed list, and it is what lets the engine section
* state end-to-end results such as "drop_unmatched_base filters base down
* to delta's keys".  It is modelled on the real engine in
* option_override.hpp so that the laziness contract is exercised the same
* way a production consumer exercises it.
*
*
* path:      /tests/djinterp/core/meta/override_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.20
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    check plumbing            (D_OV_CHECK)
      --------------------------------------

II.   element fixtures          (ov_base, ov_delta, ov_entry, ...)
      ------------------------------------------------------------

III.  policy fixtures           (concept negatives, hostile hooks, trampolines)
      -------------------------------------------------------------------------

IV.   hook readers              (ov_hooks, ov_probed_hooks, ov_binds_*)
      -----------------------------------------------------------------

V.    the miniature engine      (ov_list, ov_apply)
      ---------------------------------------------

VI.   test declarations
      -----------------
*/

#ifndef DJINTERP_TESTING_OVERRIDE_TESTS_
#define DJINTERP_TESTING_OVERRIDE_TESTS_ 1

// std
#include <cstdio>
#include <type_traits>
// djinterp
#include "../../../../inc/djinterp/core/meta/override.hpp"                    // the module under test
#include "djinterp/test/test_traits.hpp"   // hostile type zoo + quantifiers


NS_DJINTERP
NS_TESTING


// ===========================================================================
// I.   check plumbing
// ===========================================================================

// override_check
//   function: records one assertion.  Clears _all on failure and emits a
// line naming the failing expression and its source line, so a red run
// says WHICH claim broke without a debugger.  Returns the outcome so a
// caller may also branch on it.
D_INLINE bool
override_check(
    bool&       _all,
    bool        _ok,
    const char* _expression,
    int         _line
)
{
    // report the failure; the section keeps running so one break does
    // not mask the state of every claim after it
    if (!_ok)
    {
        std::printf("      ! line %d: %s\n", _line, _expression);
        _all = false;
    }

    return _ok;
}

// D_OV_CHECK
//   macro: states one claim about override.hpp.  The condition is passed
// as __VA_ARGS__ so that a template argument list may contain commas
// (`std::is_same<a, b>::value` is one argument here, not two).
#define D_OV_CHECK(_all, ...)                                                 \
    ::djinterp::testing::override_check((_all),                               \
                                        (__VA_ARGS__),                        \
                                        #__VA_ARGS__,                         \
                                        __LINE__)


// ===========================================================================
// II.  element fixtures
// ===========================================================================

// ov_base
//   type: stand-in for an element sourced from the BASE side.
struct ov_base
{};

// ov_delta
//   type: stand-in for an element sourced from the DELTA side.
struct ov_delta
{};

// ov_third
//   type: a third element, distinct from both sides.  Used where a check
// must fail for "returned the wrong operand" AND for "returned some
// unrelated type".
struct ov_third
{};

// ov_merged
//   type: payload marker for an element produced by a CUSTOM merge, so a
// combinator-built result is distinguishable from either operand.
template<typename _Left,
         typename _Right>
struct ov_merged
{};

// ov_entry
//   type: the minimum shape a keyed override needs - an integer key and a
// payload.  Two entries sharing a key model "the same slot on the base and
// on the delta side"; the payload is what tells them apart in a result.
template<int      _Key,
         typename _Payload>
struct ov_entry
{
    static constexpr int key = _Key;

    using payload = _Payload;
};


// ===========================================================================
// III. policy fixtures
// ===========================================================================

// ov_no_hooks
//   struct: a policy-shaped type carrying no hooks at all - the baseline
// negative for the OverridePolicy concept.
struct ov_no_hooks
{};

// ov_base_only_hook
//   struct: exposes on_base_only but not on_both.  Half a policy, and the
// concept must say so.
struct ov_base_only_hook
{
    template<typename _B>
    using on_base_only = _B;
};

// ov_both_only_hook
//   struct: exposes on_both but not on_base_only - the other half.
struct ov_both_only_hook
{
    template<typename _B, typename _D>
    using on_both = _D;
};

// ov_lenient_pair
//   struct: exactly the two hooks the concept probes and NO on_delta_only.
// The header documents a missing on_delta_only as legal ("the engine treats
// a missing alias as drop"), so this must satisfy OverridePolicy.
struct ov_lenient_pair
{
    template<typename _B>
    using on_base_only = _B;

    template<typename _B, typename _D>
    using on_both = _D;
};

// ov_ill_formed_both
//   struct: on_base_only is fine, but on_both names a nested ::type on its
// operand, so the concept's int / int probe is ill-formed.  The negative
// that proves the concept actually FORMS its probe types rather than merely
// checking that the names exist.
struct ov_ill_formed_both
{
    template<typename _B>
    using on_base_only = _B;

    template<typename _B, typename _D>
    using on_both = typename _D::type;
};

// ov_ill_formed_base_only
//   struct: the mirror - on_both is fine, on_base_only is ill-formed for
// the probe type.
struct ov_ill_formed_base_only
{
    template<typename _B>
    using on_base_only = typename _B::type;

    template<typename _B, typename _D>
    using on_both = _D;
};

// ov_hostile_delta
//   struct: a well-formed pair of PROBED hooks plus an on_delta_only that
// is ill-formed for every argument.  If the concept ever probed
// on_delta_only this type would be rejected, so `OverridePolicy` holding
// here is the direct statement that it does not - the same guarantee
// strict_subset depends on, but stated without a static_assert in play.
struct ov_hostile_delta
{
    template<typename _B>
    using on_base_only = _B;

    template<typename _D>
    using on_delta_only = typename _D::no_such_member;

    template<typename _B, typename _D>
    using on_both = _D;
};

// ov_int_only
//   struct: hooks that are well-formed for the concept's int / int probe
// and ill-formed for every class type.  OverridePolicy is a SHAPE contract,
// not a semantic one, so this MUST satisfy it - which is the honest
// statement of how much the concept guarantees a downstream engine.
struct ov_int_only
{
    template<typename _B>
    using on_base_only = std::enable_if_t<std::is_integral_v<_B>, _B>;

    template<typename _B, typename _D>
    using on_both = std::enable_if_t<std::is_integral_v<_D>, _D>;
};

// ov_swap_sides
//   metafn: binary metafunction yielding a marker built from BOTH operands
// in reverse order.  Feeds with_on_both where a check needs a result that
// is neither operand and is order-sensitive.
template<typename _Left,
         typename _Right>
struct ov_swap_sides
{
    using type = ov_merged<_Right, _Left>;
};

// ov_to_third
//   metafn: unary metafunction yielding ov_third regardless of argument.
// Feeds with_on_base_only / with_on_delta_only where a check needs a
// replacement result distinct from `dropped` and from either operand.
template<typename>
struct ov_to_third
{
    using type = ov_third;
};

// ov_identity_trampoline
//   metafn: the trampoline the header's with_on_both note calls for -
// a STRUCT template wrapping what would otherwise be an alias template,
// so it can be passed where a `template<typename> class` is required.
template<typename _Type>
struct ov_identity_trampoline
{
    using type = typename identity_t<_Type>::type;
};

// ov_merge_payloads
//   metafn: binary metafunction for with_on_both - yields an entry carrying
// the shared key and a payload naming both sides.  The primary template
// yields `dropped` rather than hard-erroring so that a policy built from it
// still satisfies OverridePolicy, whose probe forms on_both<int, int>; this
// mirrors the documented reason merge_args_union in option_override.hpp has
// a neutral primary.
template<typename _B,
         typename _D>
struct ov_merge_payloads
{
    using type = dropped;
};

// ov_merge_payloads (matching pair)
//   metafn: the real case - two entries sharing a key.
template<int      _Key,
         typename _BPayload,
         typename _DPayload>
struct ov_merge_payloads<ov_entry<_Key, _BPayload>,
                         ov_entry<_Key, _DPayload>>
{
    using type = ov_entry<_Key, ov_merged<_BPayload, _DPayload>>;
};


// ===========================================================================
// IV.  hook readers
// ===========================================================================

// ov_hooks
//   trait: reads a policy's three hooks for a chosen pair of element types.
// Parameterizing the ELEMENTS is what lets the primitive section re-state
// every policy with the operands swapped, which is the check that a hook
// answers by POSITION and not by type.
//
//   Instantiates on_delta_only, so it must not be used on a strict policy;
// use ov_probed_hooks there.
template<typename _Policy,
         typename _B,
         typename _D>
struct ov_hooks
{
    using base_only  = typename _Policy::template on_base_only<_B>;
    using delta_only = typename _Policy::template on_delta_only<_D>;
    using both       = typename _Policy::template on_both<_B, _D>;
};

// ov_probed_hooks
//   trait: only the two hooks the OverridePolicy concept probes, read
// WITHOUT ever naming on_delta_only.  The only hook reader a strict policy
// survives.
template<typename _Policy,
         typename _B,
         typename _D>
struct ov_probed_hooks
{
    using base_only = typename _Policy::template on_base_only<_B>;
    using both      = typename _Policy::template on_both<_B, _D>;
};

// ov_hooks_agree
//   trait: true iff two policies map ALL THREE hooks identically for the
// standard element pair.  This is what states a behavioural equivalence
// between a primitive and a combinator-built policy in one line - the
// combinator algebra is checked by equating policies, not by restating
// every hook of every composition by hand.
template<typename _Left,
         typename _Right>
struct ov_hooks_agree
{
private:
    using l = ov_hooks<_Left,  ov_base, ov_delta>;
    using r = ov_hooks<_Right, ov_base, ov_delta>;

public:
    static constexpr bool value =
        ( std::is_same<typename l::base_only,
                       typename r::base_only>::value    &&
          std::is_same<typename l::delta_only,
                       typename r::delta_only>::value   &&
          std::is_same<typename l::both,
                       typename r::both>::value );
};

// ov_probed_hooks_agree
//   trait: the same equivalence over only the two hooks a strict policy
// permits to be instantiated.
template<typename _Left,
         typename _Right>
struct ov_probed_hooks_agree
{
private:
    using l = ov_probed_hooks<_Left,  ov_base, ov_delta>;
    using r = ov_probed_hooks<_Right, ov_base, ov_delta>;

public:
    static constexpr bool value =
        ( std::is_same<typename l::base_only,
                       typename r::base_only>::value &&
          std::is_same<typename l::both,
                       typename r::both>::value );
};

// ov_hook_binder
//   trait: binds a policy so that "what does this hook do to _Type?"
// becomes the ONE-parameter trait the type-set quantifiers take.  The
// module's headline claim is that it assumes nothing about the elements it
// handles; running a hook over void, function, array, reference and
// incomplete types is what states that claim as a test rather than as
// prose.  Both operands of on_both are the same type here, which is the
// one pairing every non-dropping policy must answer identically.
template<typename _Policy>
struct ov_hook_binder
{
    // base_only_is_identity
    //   trait: on_base_only<_Type> is _Type.
    template<typename _Type>
    using base_only_is_identity =
        std::is_same<typename _Policy::template on_base_only<_Type>, _Type>;

    // base_only_is_dropped
    //   trait: on_base_only<_Type> is the sentinel.
    template<typename _Type>
    using base_only_is_dropped =
        is_dropped<typename _Policy::template on_base_only<_Type>>;

    // delta_only_is_identity
    //   trait: on_delta_only<_Type> is _Type.
    template<typename _Type>
    using delta_only_is_identity =
        std::is_same<typename _Policy::template on_delta_only<_Type>, _Type>;

    // delta_only_is_dropped
    //   trait: on_delta_only<_Type> is the sentinel.
    template<typename _Type>
    using delta_only_is_dropped =
        is_dropped<typename _Policy::template on_delta_only<_Type>>;

    // both_is_identity
    //   trait: on_both<_Type, _Type> is _Type.
    template<typename _Type>
    using both_is_identity =
        std::is_same<typename _Policy::template on_both<_Type, _Type>,
                     _Type>;
};

// ---------------------------------------------------------------------------
//  probes for the misc metafunction helpers
// ---------------------------------------------------------------------------
//   Each restates one helper's whole contract as a ONE-parameter trait, so
// the quantifiers can assert it over the adversarial zoo in a single line.
// These helpers are the slots the combinators take, so "works for every
// type" is the property that matters about them.

// ov_identity_holds
//   trait: identity_t<_Type>::type is _Type.
template<typename _Type>
struct ov_identity_holds
    : std::is_same<typename identity_t<_Type>::type, _Type>
{};

// ov_identity_matches_std
//   trait: identity_t agrees with std::type_identity on _Type.
template<typename _Type>
struct ov_identity_matches_std
    : std::is_same<typename identity_t<_Type>::type,
                   typename std::type_identity<_Type>::type>
{};

// ov_always_drop_holds
//   trait: always_drop<_Type>::type is the sentinel.
template<typename _Type>
struct ov_always_drop_holds
    : is_dropped<typename always_drop<_Type>::type>
{};

// ov_keep_left_holds
//   trait: always_keep_left yields _Type from the LEFT position.
template<typename _Type>
struct ov_keep_left_holds
    : std::is_same<typename always_keep_left<_Type, ov_third>::type, _Type>
{};

// ov_keep_left_ignores_right
//   trait: always_keep_left ignores whatever sits on the right.
template<typename _Type>
struct ov_keep_left_ignores_right
    : std::is_same<typename always_keep_left<ov_third, _Type>::type,
                   ov_third>
{};

// ov_keep_right_holds
//   trait: always_keep_right yields _Type from the RIGHT position.
template<typename _Type>
struct ov_keep_right_holds
    : std::is_same<typename always_keep_right<ov_third, _Type>::type, _Type>
{};

// ov_keep_right_ignores_left
//   trait: always_keep_right ignores whatever sits on the left.
template<typename _Type>
struct ov_keep_right_ignores_left
    : std::is_same<typename always_keep_right<_Type, ov_third>::type,
                   ov_third>
{};

// ov_binds_unary
//   trait: true when _Op can be BOUND as a unary type metafunction.  Naming
// an alias template or a class template as a template template argument
// does not instantiate it, so this reports presence and arity for a hook
// that must never be instantiated.
template<template<typename> class _Op>
struct ov_binds_unary : std::true_type
{};

// ov_binds_binary
//   trait: the binary counterpart of ov_binds_unary.
template<template<typename, typename> class _Op>
struct ov_binds_binary : std::true_type
{};

// ov_is_policy
//   trait: the OverridePolicy concept in TRAIT shape, so the type-set
// quantifiers can run it over a whole battery at once.
template<typename _Policy>
struct ov_is_policy : std::bool_constant<OverridePolicy<_Policy>>
{};

// ov_constrained_call
//   function: an overload pair separated ONLY by the OverridePolicy
// constraint.  The constrained overload wins by partial ordering, which is
// the property a downstream engine leans on when it declares
// `template<OverridePolicy _Policy> struct engine`.
template<typename _Policy>
D_CONSTEXPR int
ov_constrained_call()
{
    return 0;
}

// ov_constrained_call (constrained)
//   function: the more-constrained overload.
template<OverridePolicy _Policy>
D_CONSTEXPR int
ov_constrained_call()
{
    return 1;
}

// ov_constrained
//   trait: false unless _Policy satisfies OverridePolicy.  The partial
// specialization below differs from the primary ONLY by its constraint, so
// selecting it exercises the concept as a genuine CONSTRAINT rather than as
// a boolean the test happens to read.
template<typename _Policy>
struct ov_constrained : std::false_type
{};

// ov_constrained (satisfied case)
//   trait: the more-constrained partial specialization.
template<OverridePolicy _Policy>
struct ov_constrained<_Policy> : std::true_type
{};

// ov_lazy_delta_only
//   trait: lazy access to a policy's on_delta_only<_D>.  When _Drop is true
// the policy alias is NEVER named - which is exactly what keeps a strict
// policy's static_assert from firing on a key base already carries.  The
// engine below routes every delta-only decision through this, as the
// header's contract requires of any engine.
template<bool     _Drop,
         typename _Policy,
         typename _D>
struct ov_lazy_delta_only
{
    using type = typename _Policy::template on_delta_only<_D>;
};

// ov_lazy_delta_only (dropped case)
//   trait: the short circuit - the policy is not consulted at all.
template<typename _Policy,
         typename _D>
struct ov_lazy_delta_only<true, _Policy, _D>
{
    using type = dropped;
};


// ===========================================================================
// V.   the miniature engine
// ===========================================================================

// ov_list
//   type: a list of ov_entry, standing in for a keyed container.  The
// engine's input and output shape.
template<typename... _Entries>
struct ov_list
{};

// ov_list_has_key
//   trait: whether any entry in _List carries _Key.
template<int      _Key,
         typename _List>
struct ov_list_has_key;

// ov_list_has_key (list case)
//   trait: the fold over the list's entries.
template<int         _Key,
         typename... _Entries>
struct ov_list_has_key<_Key, ov_list<_Entries...>>
    : std::bool_constant<( (_Entries::key == _Key) || ... )>
{};

// ov_list_find
//   trait: the entry in _List carrying _Key.  Only ever reached when
// ov_list_has_key already answered true; the primary is left undefined so
// that a misuse is a diagnosed error rather than a silent wrong answer.
template<int      _Key,
         typename _List>
struct ov_list_find;

// ov_list_find (list case)
//   trait: head match, else recurse into the tail.
template<int         _Key,
         typename    _Head,
         typename... _Tail>
struct ov_list_find<_Key, ov_list<_Head, _Tail...>>
{
    using type = typename std::conditional<
        (_Head::key == _Key),
        std::type_identity<_Head>,
        ov_list_find<_Key, ov_list<_Tail...>>>::type::type;
};

// ov_append_kept
//   trait: appends _Type to a list unless it is the `dropped` sentinel.
// This is the filtering the header makes the engine responsible for.
template<typename _List,
         typename _Type>
struct ov_append_kept;

// ov_append_kept (keep case)
//   trait: an ordinary element joins the accumulator.
template<typename    _Type,
         typename... _Kept>
struct ov_append_kept<ov_list<_Kept...>, _Type>
{
    using type = ov_list<_Kept..., _Type>;
};

// ov_append_kept (drop case)
//   trait: `dropped` leaves the accumulator untouched.
template<typename... _Kept>
struct ov_append_kept<ov_list<_Kept...>, dropped>
{
    using type = ov_list<_Kept...>;
};

// ov_resolve_base
//   trait: which hook a BASE entry goes through - on_both when delta
// carries the same key, on_base_only otherwise.
template<bool     _Matched,
         typename _Policy,
         typename _B,
         typename _Delta>
struct ov_resolve_base
{
    using type = typename _Policy::template on_base_only<_B>;
};

// ov_resolve_base (matched case)
//   trait: the key exists on both sides.
template<typename _Policy,
         typename _B,
         typename _Delta>
struct ov_resolve_base<true, _Policy, _B, _Delta>
{
    using type = typename _Policy::template on_both<
        _B,
        typename ov_list_find<_B::key, _Delta>::type>;
};

// ov_base_pass
//   trait: folds every base entry through on_both / on_base_only, dropping
// what the policy discards.  Base order is preserved.
template<typename    _Policy,
         typename    _Delta,
         typename    _Acc,
         typename... _BaseEntries>
struct ov_base_pass
{
    using type = _Acc;
};

// ov_base_pass (step)
//   trait: resolve the head, accumulate, recurse.
template<typename    _Policy,
         typename    _Delta,
         typename    _Acc,
         typename    _Head,
         typename... _Tail>
struct ov_base_pass<_Policy, _Delta, _Acc, _Head, _Tail...>
{
private:
    static constexpr bool matched =
        ov_list_has_key<_Head::key, _Delta>::value;

    using resolved =
        typename ov_resolve_base<matched, _Policy, _Head, _Delta>::type;

public:
    using type = typename ov_base_pass<
        _Policy,
        _Delta,
        typename ov_append_kept<_Acc, resolved>::type,
        _Tail...>::type;
};

// ov_delta_pass
//   trait: folds every DELTA-ONLY entry through on_delta_only, lazily.
// Keys base already carries never reach the policy at all.  Delta order is
// preserved, and delta-only entries land after the base pass's output.
template<typename    _Policy,
         typename    _Base,
         typename    _Acc,
         typename... _DeltaEntries>
struct ov_delta_pass
{
    using type = _Acc;
};

// ov_delta_pass (step)
//   trait: resolve the head through the lazy wrapper, accumulate, recurse.
template<typename    _Policy,
         typename    _Base,
         typename    _Acc,
         typename    _Head,
         typename... _Tail>
struct ov_delta_pass<_Policy, _Base, _Acc, _Head, _Tail...>
{
private:
    static constexpr bool in_base =
        ov_list_has_key<_Head::key, _Base>::value;

    using resolved =
        typename ov_lazy_delta_only<in_base, _Policy, _Head>::type;

public:
    using type = typename ov_delta_pass<
        _Policy,
        _Base,
        typename ov_append_kept<_Acc, resolved>::type,
        _Tail...>::type;
};

// ov_apply
//   trait: THE miniature engine.  Applies _Policy to a base list and a
// delta list and yields the surviving entries: the base pass first (in base
// order), then delta's extensions (in delta order).
template<typename _Policy,
         typename _Base,
         typename _Delta>
struct ov_apply;

// ov_apply (list case)
//   trait: the two-pass walk.
template<typename    _Policy,
         typename... _B,
         typename... _D>
struct ov_apply<_Policy, ov_list<_B...>, ov_list<_D...>>
{
private:
    using after_base = typename ov_base_pass<_Policy,
                                             ov_list<_D...>,
                                             ov_list<>,
                                             _B...>::type;

public:
    using type = typename ov_delta_pass<_Policy,
                                        ov_list<_B...>,
                                        after_base,
                                        _D...>::type;
};

// ov_apply_t
//   type: convenience alias for ov_apply<...>::type.
template<typename _Policy,
         typename _Base,
         typename _Delta>
using ov_apply_t = typename ov_apply<_Policy, _Base, _Delta>::type;


// ===========================================================================
// VI.  test declarations
// ===========================================================================

// section I - the dropped sentinel (override_tests_dropped.cpp)
bool tests_override_dropped_shape();
bool tests_override_is_dropped_trait();
bool tests_override_is_dropped_cvref();
bool tests_override_is_dropped_hostile();

// section II - the OverridePolicy concept (override_tests_concept.cpp)
bool tests_override_concept_primitives();
bool tests_override_concept_negatives();
bool tests_override_concept_laziness();
bool tests_override_concept_as_constraint();

// section III - the atomic primitives (override_tests_primitives.cpp)
bool tests_override_keep_base();
bool tests_override_keep_delta();
bool tests_override_drop_extras();
bool tests_override_strict_subset();
bool tests_override_drop_unmatched_base();
bool tests_override_primitive_shapes();

// section IV - the combinators (override_tests_combinators.cpp)
bool tests_override_with_on_both();
bool tests_override_with_on_base_only();
bool tests_override_with_on_delta_only();
bool tests_override_combinator_stacking();
bool tests_override_combinator_equivalences();

// section V - the misc helpers (override_tests_helpers.cpp)
bool tests_override_identity_t();
bool tests_override_always_drop();
bool tests_override_always_keep_left();
bool tests_override_always_keep_right();

// integration - policies driven by a real walk (override_tests_engine.cpp)
bool tests_override_engine_primitives();
bool tests_override_engine_edge_cases();
bool tests_override_engine_custom_policies();


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_OVERRIDE_TESTS_
