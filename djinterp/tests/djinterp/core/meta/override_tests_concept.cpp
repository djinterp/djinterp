/*******************************************************************************
* djinterp [core]                                     override_tests_concept.cpp
*
*
* path:      /tests/djinterp/core/meta/override_tests_concept.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "override_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_override_concept_primitives
  Every shape the module itself ships must satisfy its own concept.
  Tests the following:
  - all five atomic primitives satisfy OverridePolicy
  - every combinator applied to every primitive still satisfies it, so
    composing a policy can never silently leave the contract
  - a hand-rolled type exposing just the two probed hooks satisfies it,
    which is the module's documented minimum
*/
bool
tests_override_concept_primitives()
{
    bool ok = true;

    // the five primitives
    D_OV_CHECK(ok, OverridePolicy<keep_base>);
    D_OV_CHECK(ok, OverridePolicy<keep_delta>);
    D_OV_CHECK(ok, OverridePolicy<drop_extras>);
    D_OV_CHECK(ok, OverridePolicy<strict_subset>);
    D_OV_CHECK(ok, OverridePolicy<drop_unmatched_base>);

    // the documented minimum: two probed hooks, no on_delta_only
    D_OV_CHECK(ok, OverridePolicy<ov_lenient_pair>);

    // combinators preserve the contract
    D_OV_CHECK(ok, OverridePolicy<with_on_both<keep_base,
                                               always_keep_right>>);
    D_OV_CHECK(ok, OverridePolicy<with_on_base_only<keep_delta,
                                                    always_drop>>);
    D_OV_CHECK(ok, OverridePolicy<with_on_delta_only<strict_subset,
                                                     identity_t>>);
    D_OV_CHECK(ok, OverridePolicy<with_on_both<drop_unmatched_base,
                                               ov_merge_payloads>>);

    // and so does stacking them
    D_OV_CHECK(ok, OverridePolicy<
                       with_on_delta_only<
                           with_on_base_only<
                               with_on_both<keep_base, always_keep_left>,
                               always_drop>,
                           identity_t>>);

    return ok;
}

/*
tests_override_concept_negatives
  What the concept must REJECT.
  Tests the following:
  - a type with no hooks, and each half of the pair on its own
  - a type whose hook NAMES exist but whose probe instantiation is
    ill-formed, in either position; this is what proves the concept forms
    its probe types rather than merely checking that the names resolve
  - non-class types, including an incomplete class type: every one of
    them reads false instead of diagnosing, so a downstream engine may
    constrain on this concept without first proving its argument is a
    complete class
*/
bool
tests_override_concept_negatives()
{
    bool ok = true;

    // nothing, and each half alone
    D_OV_CHECK(ok, !OverridePolicy<ov_no_hooks>);
    D_OV_CHECK(ok, !OverridePolicy<ov_base_only_hook>);
    D_OV_CHECK(ok, !OverridePolicy<ov_both_only_hook>);

    // present but ill-formed under the probe, in either position
    D_OV_CHECK(ok, !OverridePolicy<ov_ill_formed_both>);
    D_OV_CHECK(ok, !OverridePolicy<ov_ill_formed_base_only>);

    // element types are not policies
    D_OV_CHECK(ok, !OverridePolicy<ov_base>);
    D_OV_CHECK(ok, !OverridePolicy<dropped>);

    // non-class types answer false rather than diagnosing
    D_OV_CHECK(ok, !OverridePolicy<int>);
    D_OV_CHECK(ok, !OverridePolicy<void>);
    D_OV_CHECK(ok, !OverridePolicy<int&>);
    D_OV_CHECK(ok, !OverridePolicy<int[3]>);
    D_OV_CHECK(ok, !OverridePolicy<int(int)>);

    // the whole adversarial zoo at once.  count_holds has no
    // short-circuit, so every cell - the incomplete type included - is
    // instantiated; a concept that hard-errored on any of them would
    // break the build here rather than reading false
    D_OV_CHECK(ok, ::djinterp::test::holds_for_none<
                       ov_is_policy, D_TEST_HOSTILE_TYPES>::value);

    return ok;
}

/*
tests_override_concept_laziness
  The concept's deliberate blind spots - the header's central design claim.
  Tests the following:
  - on_delta_only is NEVER probed: a policy whose on_delta_only is
    ill-formed for every argument still satisfies the concept.  This is
    the exact guarantee strict_subset's static_assert depends on, stated
    here without a static_assert in play so that the mechanism, not the
    message, is what is under test
  - strict_subset itself therefore satisfies the concept, and merely
    naming it (or binding its hooks) never fires its assert
  - the probe types are int / int, so the concept is a SHAPE contract
    only: a policy well-formed for int and ill-formed for class types
    still satisfies it
*/
bool
tests_override_concept_laziness()
{
    bool ok = true;

    // an ill-formed on_delta_only does not disqualify a policy
    D_OV_CHECK(ok, OverridePolicy<ov_hostile_delta>);

    // the same guarantee, for the primitive that ships with it
    D_OV_CHECK(ok, OverridePolicy<strict_subset>);

    // the strict hooks can be NAMED - bound as template template
    // arguments - without being instantiated
    D_OV_CHECK(ok, ov_binds_unary<strict_subset::on_delta_only>::value);
    D_OV_CHECK(ok, ov_binds_unary<strict_subset::assert_extension>::value);
    D_OV_CHECK(ok, ov_binds_unary<ov_hostile_delta::on_delta_only>::value);

    // the concept checks SHAPE, not semantics: probe types are int / int
    D_OV_CHECK(ok, OverridePolicy<ov_int_only>);
    D_OV_CHECK(ok, std::is_same_v<ov_int_only::on_base_only<int>, int>);
    D_OV_CHECK(ok, std::is_same_v<ov_int_only::on_both<int, int>, int>);

    // a lazy engine wrapper honours the same contract: with _Drop true
    // the policy alias is not named at all, so a strict policy survives
    D_OV_CHECK(ok, std::is_same_v<
                       ov_lazy_delta_only<true, strict_subset,
                                          ov_delta>::type,
                       dropped>);
    D_OV_CHECK(ok, std::is_same_v<
                       ov_lazy_delta_only<true, ov_hostile_delta,
                                          ov_delta>::type,
                       dropped>);

    // and with _Drop false it really does consult the policy
    D_OV_CHECK(ok, std::is_same_v<
                       ov_lazy_delta_only<false, keep_delta,
                                          ov_delta>::type,
                       ov_delta>);
    D_OV_CHECK(ok, std::is_same_v<
                       ov_lazy_delta_only<false, keep_base,
                                          ov_delta>::type,
                       dropped>);

    return ok;
}

/*
tests_override_concept_as_constraint
  The concept used the way a downstream engine uses it.
  Tests the following:
  - a partial specialization differing from its primary ONLY by the
    constraint is selected for satisfying types and not for others
  - an overload pair differing only by the constraint resolves to the
    constrained member, so OverridePolicy participates in partial
    ordering rather than merely evaluating to a bool
  - the trait-shaped wrapper agrees with the concept everywhere, which is
    what licenses running it through the type-set quantifiers above
*/
bool
tests_override_concept_as_constraint()
{
    bool ok = true;

    // constrained partial specialization
    D_OV_CHECK(ok, ov_constrained<keep_base>::value);
    D_OV_CHECK(ok, ov_constrained<keep_delta>::value);
    D_OV_CHECK(ok, ov_constrained<strict_subset>::value);
    D_OV_CHECK(ok, ov_constrained<with_on_both<keep_base,
                                               always_keep_right>>::value);
    D_OV_CHECK(ok, !ov_constrained<ov_no_hooks>::value);
    D_OV_CHECK(ok, !ov_constrained<ov_ill_formed_both>::value);
    D_OV_CHECK(ok, !ov_constrained<int>::value);

    // constrained overload resolution
    D_OV_CHECK(ok, ov_constrained_call<keep_base>() == 1);
    D_OV_CHECK(ok, ov_constrained_call<drop_unmatched_base>() == 1);
    D_OV_CHECK(ok, ov_constrained_call<ov_lenient_pair>() == 1);
    D_OV_CHECK(ok, ov_constrained_call<ov_no_hooks>() == 0);
    D_OV_CHECK(ok, ov_constrained_call<ov_both_only_hook>() == 0);

    // the trait wrapper is faithful in both directions
    D_OV_CHECK(ok, ov_is_policy<keep_delta>::value ==
                   OverridePolicy<keep_delta>);
    D_OV_CHECK(ok, ov_is_policy<ov_no_hooks>::value ==
                   OverridePolicy<ov_no_hooks>);
    D_OV_CHECK(ok, ::djinterp::test::holds_for_all<
                       ov_is_policy,
                       keep_base, keep_delta, drop_extras,
                       strict_subset, drop_unmatched_base>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
