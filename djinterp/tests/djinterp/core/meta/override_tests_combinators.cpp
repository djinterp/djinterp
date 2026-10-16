/*******************************************************************************
* djinterp [core]                                 override_tests_combinators.cpp
*
*
* path:      /tests/djinterp/core/meta/override_tests_combinators.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "override_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_override_with_on_both
  with_on_both: replace the overlap hook, inherit the other two.
  Tests the following:
  - on_both becomes _F<_B, _D>::type while on_base_only and on_delta_only
    come through from the base policy unchanged
  - the result derives publicly from the base policy, which is the
    mechanism the inheritance relies on
  - the replacement is a real metafunction call: a binary metafn that is
    neither projection (ov_swap_sides) produces a result that is neither
    operand, and an entry-merging metafn produces a merged entry
  - the combinator stays an empty, stateless tag
*/
bool
tests_override_with_on_both()
{
    bool ok = true;

    using p = with_on_both<keep_base, always_keep_right>;
    using h = ov_hooks<p, ov_base, ov_delta>;

    // the replaced hook
    D_OV_CHECK(ok, std::is_same_v<h::both, ov_delta>);

    // the inherited hooks are untouched
    D_OV_CHECK(ok, std::is_same_v<h::base_only,  ov_base>);
    D_OV_CHECK(ok, std::is_same_v<h::delta_only, dropped>);
    D_OV_CHECK(ok, std::is_same_v<h::delta_only,
                                  keep_base::on_delta_only<ov_delta>>);

    // inheritance is the mechanism
    D_OV_CHECK(ok, std::is_base_of_v<keep_base, p>);
    D_OV_CHECK(ok, std::is_convertible_v<p*, keep_base*>);

    // a genuine metafunction call, not a projection
    using swapped = with_on_both<keep_delta, ov_swap_sides>;

    D_OV_CHECK(ok, std::is_same_v<
                       swapped::on_both<ov_base, ov_delta>,
                       ov_merged<ov_delta, ov_base>>);
    D_OV_CHECK(ok, !std::is_same_v<
                       swapped::on_both<ov_base, ov_delta>,
                       ov_merged<ov_base, ov_delta>>);

    // ... including one that merges two keyed entries
    using merging = with_on_both<keep_delta, ov_merge_payloads>;

    D_OV_CHECK(ok, std::is_same_v<
                       merging::on_both<ov_entry<7, ov_base>,
                                        ov_entry<7, ov_delta>>,
                       ov_entry<7, ov_merged<ov_base, ov_delta>>>);

    // the two shipped binary metafns behave as their names claim
    D_OV_CHECK(ok, std::is_same_v<
                       with_on_both<keep_delta,
                                    always_keep_left>::on_both<ov_base,
                                                               ov_delta>,
                       ov_base>);
    D_OV_CHECK(ok, std::is_same_v<
                       with_on_both<keep_base,
                                    always_keep_right>::on_both<ov_base,
                                                                ov_delta>,
                       ov_delta>);

    // still a stateless tag, still a policy
    D_OV_CHECK(ok, std::is_empty_v<p>);
    D_OV_CHECK(ok, sizeof(p) == 1);
    D_OV_CHECK(ok, OverridePolicy<p>);

    return ok;
}

/*
tests_override_with_on_base_only
  with_on_base_only: replace the base-only hook via a unary metafunction.
  Tests the following:
  - on_base_only becomes _F<_B>::type; on_both and on_delta_only are
    inherited unchanged
  - the replacement can drop (always_drop), can be the identity
    (identity_t), and can be an arbitrary rewrite (ov_to_third)
  - the trampoline pattern the header prescribes - a struct template
    wrapping an alias template - is accepted where a `template<typename>
    class` is required
*/
bool
tests_override_with_on_base_only()
{
    bool ok = true;

    using p = with_on_base_only<keep_delta, always_drop>;
    using h = ov_hooks<p, ov_base, ov_delta>;

    // the replaced hook drops
    D_OV_CHECK(ok, std::is_same_v<h::base_only, dropped>);
    D_OV_CHECK(ok, is_dropped_v<h::base_only>);

    // the inherited hooks are untouched
    D_OV_CHECK(ok, std::is_same_v<h::both,       ov_delta>);
    D_OV_CHECK(ok, std::is_same_v<h::delta_only, ov_delta>);

    D_OV_CHECK(ok, std::is_base_of_v<keep_delta, p>);

    // the identity replacement changes nothing observable
    using ident = with_on_base_only<keep_delta, identity_t>;

    D_OV_CHECK(ok, ov_hooks_agree<ident, keep_delta>::value);
    D_OV_CHECK(ok, !std::is_same_v<ident, keep_delta>);

    // an arbitrary rewrite
    using third = with_on_base_only<keep_base, ov_to_third>;

    D_OV_CHECK(ok, std::is_same_v<third::on_base_only<ov_base>, ov_third>);
    D_OV_CHECK(ok, std::is_same_v<third::on_both<ov_base, ov_delta>,
                                  ov_base>);

    // the documented trampoline for an alias-shaped metafunction
    using tramp = with_on_base_only<drop_extras, ov_identity_trampoline>;

    D_OV_CHECK(ok, std::is_same_v<tramp::on_base_only<ov_base>, ov_base>);
    D_OV_CHECK(ok, ov_hooks_agree<tramp, drop_extras>::value);

    D_OV_CHECK(ok, std::is_empty_v<p>);
    D_OV_CHECK(ok, OverridePolicy<p>);

    return ok;
}

/*
tests_override_with_on_delta_only
  with_on_delta_only: replace the extension hook.
  Tests the following:
  - on_delta_only becomes _F<_D>::type; the other two are inherited
  - the header's stated use case in BOTH directions: making a lenient
    policy stricter (keep_delta + always_drop), and making a strict one
    lenient
  - the strict-to-lenient direction is the sharp case.  Shadowing a
    strict hook must not merely hide it but prevent its instantiation
    entirely: with_on_delta_only<strict_subset, identity_t> answers
    on_delta_only for a real element, and the fact that this function
    COMPILES is the proof that strict_subset's static_assert was never
    reached
*/
bool
tests_override_with_on_delta_only()
{
    bool ok = true;

    using p = with_on_delta_only<keep_delta, always_drop>;
    using h = ov_hooks<p, ov_base, ov_delta>;

    // the replaced hook now drops
    D_OV_CHECK(ok, std::is_same_v<h::delta_only, dropped>);

    // the inherited hooks are untouched
    D_OV_CHECK(ok, std::is_same_v<h::base_only, ov_base>);
    D_OV_CHECK(ok, std::is_same_v<h::both,      ov_delta>);

    D_OV_CHECK(ok, std::is_base_of_v<keep_delta, p>);

    // lenient -> strict: that composition IS drop_extras
    D_OV_CHECK(ok, ov_hooks_agree<p, drop_extras>::value);

    // strict -> lenient: the assert is shadowed out of existence, not
    // merely hidden.  Reaching these lines at all is the claim.
    using relaxed = with_on_delta_only<strict_subset, identity_t>;

    D_OV_CHECK(ok, std::is_same_v<relaxed::on_delta_only<ov_delta>,
                                  ov_delta>);
    D_OV_CHECK(ok, std::is_same_v<relaxed::on_delta_only<int>, int>);
    D_OV_CHECK(ok, ov_hooks_agree<relaxed, keep_delta>::value);
    D_OV_CHECK(ok, std::is_base_of_v<strict_subset, relaxed>);

    // the base's own alias is still there, unshadowed, under its own
    // qualified name - the derived declaration hides, it does not remove
    D_OV_CHECK(ok, ov_binds_unary<strict_subset::on_delta_only>::value);
    D_OV_CHECK(ok, ov_binds_unary<relaxed::on_delta_only>::value);

    // a policy with no on_delta_only at all can be given one
    using given = with_on_delta_only<ov_lenient_pair, identity_t>;

    D_OV_CHECK(ok, std::is_same_v<given::on_delta_only<ov_delta>,
                                  ov_delta>);
    D_OV_CHECK(ok, ov_hooks_agree<given, keep_delta>::value);

    D_OV_CHECK(ok, std::is_empty_v<p>);
    D_OV_CHECK(ok, OverridePolicy<p>);

    return ok;
}

/*
tests_override_combinator_stacking
  Combinators composed with one another.
  Tests the following:
  - all three may be stacked; each replaces its own hook and none
    disturbs the others
  - the inheritance chain is transitive, so the original primitive is
    still a base of the fully composed policy
  - re-wrapping the SAME hook is legal and the outermost wrapper wins -
    name hiding along a single-inheritance chain, with no ambiguity
  - the result is still a policy, and still stateless
*/
bool
tests_override_combinator_stacking()
{
    bool ok = true;

    // all three hooks replaced, one wrapper each
    using stacked =
        with_on_delta_only<
            with_on_base_only<
                with_on_both<keep_base, always_keep_right>,
                ov_to_third>,
            identity_t>;

    using h = ov_hooks<stacked, ov_base, ov_delta>;

    D_OV_CHECK(ok, std::is_same_v<h::both,       ov_delta>);
    D_OV_CHECK(ok, std::is_same_v<h::base_only,  ov_third>);
    D_OV_CHECK(ok, std::is_same_v<h::delta_only, ov_delta>);

    // the chain is transitive
    D_OV_CHECK(ok, std::is_base_of_v<keep_base, stacked>);
    D_OV_CHECK(ok, std::is_base_of_v<
                       with_on_both<keep_base, always_keep_right>,
                       stacked>);

    // re-wrapping one hook: the outermost wrapper wins
    using twice = with_on_both<with_on_both<keep_base, always_keep_left>,
                               always_keep_right>;

    D_OV_CHECK(ok, std::is_same_v<twice::on_both<ov_base, ov_delta>,
                                  ov_delta>);
    D_OV_CHECK(ok, std::is_same_v<
                       with_on_both<twice, always_keep_left>::on_both<
                           ov_base, ov_delta>,
                       ov_base>);

    // ... and the hooks it did not touch still resolve unambiguously
    D_OV_CHECK(ok, std::is_same_v<twice::on_base_only<ov_base>, ov_base>);
    D_OV_CHECK(ok, is_dropped_v<twice::on_delta_only<ov_delta>>);

    // stacking preserves the contract and the shape
    D_OV_CHECK(ok, OverridePolicy<stacked>);
    D_OV_CHECK(ok, OverridePolicy<twice>);
    D_OV_CHECK(ok, std::is_empty_v<stacked>);
    D_OV_CHECK(ok, sizeof(stacked) == 1);

    return ok;
}

/*
tests_override_combinator_equivalences
  The algebra: every primitive rebuilt out of another primitive plus one
  combinator.
  Tests the following:
  - keep_base, drop_extras and drop_unmatched_base can each be
    reconstructed from keep_delta by replacing exactly one hook, and
    keep_delta can be reconstructed from drop_extras.  Each equivalence
    is a statement about BOTH the primitive and the combinator, so a
    change to either side breaks it
  - a reconstruction is behaviourally equal but NOT type-equal, which is
    what a downstream engine dispatching on the policy TYPE must know
  - the two orders of applying two different combinators agree
  - strict_subset is reconstructible only up to its probed hooks, and is
    checked with the reader that does not name on_delta_only
*/
bool
tests_override_combinator_equivalences()
{
    bool ok = true;

    // keep_base = keep_delta with the overlap hook taking the left operand
    D_OV_CHECK(ok, ov_hooks_agree<
                       with_on_delta_only<
                           with_on_both<keep_delta, always_keep_left>,
                           always_drop>,
                       keep_base>::value);

    // drop_extras = keep_delta that refuses to extend
    D_OV_CHECK(ok, ov_hooks_agree<
                       with_on_delta_only<keep_delta, always_drop>,
                       drop_extras>::value);

    // keep_delta = drop_extras that allows extension again
    D_OV_CHECK(ok, ov_hooks_agree<
                       with_on_delta_only<drop_extras, identity_t>,
                       keep_delta>::value);

    // drop_unmatched_base = keep_delta that discards unmatched base
    D_OV_CHECK(ok, ov_hooks_agree<
                       with_on_base_only<keep_delta, always_drop>,
                       drop_unmatched_base>::value);

    // keep_base = drop_extras with the overlap hook flipped
    D_OV_CHECK(ok, ov_hooks_agree<
                       with_on_both<drop_extras, always_keep_left>,
                       keep_base>::value);

    // strict_subset, up to the hooks that may be instantiated
    D_OV_CHECK(ok, ov_probed_hooks_agree<
                       with_on_both<keep_base, always_keep_right>,
                       strict_subset>::value);
    D_OV_CHECK(ok, ov_probed_hooks_agree<drop_extras,
                                         strict_subset>::value);

    // behavioural equality is not type equality
    D_OV_CHECK(ok, !std::is_same_v<
                       with_on_delta_only<keep_delta, always_drop>,
                       drop_extras>);
    D_OV_CHECK(ok, !std::is_same_v<
                       with_on_base_only<keep_delta, always_drop>,
                       drop_unmatched_base>);

    // the two orders of two different combinators agree
    using ab = with_on_base_only<with_on_both<keep_delta,
                                              always_keep_right>,
                                 always_drop>;
    using ba = with_on_both<with_on_base_only<keep_delta, always_drop>,
                            always_keep_right>;

    D_OV_CHECK(ok, ov_hooks_agree<ab, ba>::value);
    D_OV_CHECK(ok, !std::is_same_v<ab, ba>);
    D_OV_CHECK(ok, ov_hooks_agree<ab, drop_unmatched_base>::value);

    // the SAME two combinators over keep_base instead land on a policy
    // the module does not ship: base dropped, extensions dropped,
    // overlap taking delta - an intersection.  Worth pinning, because it
    // shows the combinators span strictly more than the five named
    // primitives, and because it is the trap the equivalences above walk
    // past: which primitive you start from decides what the inherited
    // third hook contributes
    using intersect = with_on_base_only<with_on_both<keep_base,
                                                     always_keep_right>,
                                        always_drop>;

    D_OV_CHECK(ok, is_dropped_v<intersect::on_base_only<ov_base>>);
    D_OV_CHECK(ok, is_dropped_v<intersect::on_delta_only<ov_delta>>);
    D_OV_CHECK(ok, std::is_same_v<intersect::on_both<ov_base, ov_delta>,
                                  ov_delta>);
    D_OV_CHECK(ok, OverridePolicy<intersect>);
    D_OV_CHECK(ok, !ov_hooks_agree<intersect, drop_unmatched_base>::value);
    D_OV_CHECK(ok, !ov_hooks_agree<intersect, drop_extras>::value);
    D_OV_CHECK(ok, !ov_hooks_agree<intersect, keep_base>::value);

    // the identity replacement is the algebra's unit on every hook
    D_OV_CHECK(ok, ov_hooks_agree<
                       with_on_base_only<keep_base, identity_t>,
                       keep_base>::value);
    D_OV_CHECK(ok, ov_hooks_agree<
                       with_on_both<keep_delta, always_keep_right>,
                       keep_delta>::value);
    D_OV_CHECK(ok, ov_hooks_agree<
                       with_on_delta_only<drop_unmatched_base, identity_t>,
                       drop_unmatched_base>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
