/*******************************************************************************
* djinterp [cpp]                                  override_tests_combinators.cpp
*
*
* path:      /tests/djinterp/cpp/meta/override_tests_combinators.cpp
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
  DWithOnBoth: replace the overlap hook, inherit the other two.
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
bool tests_override_with_on_both()
{
    bool ok = true;

    using p = DWithOnBoth<keep_base, always_keep_right>;
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
    using swapped = DWithOnBoth<DKeepDelta, ov_swap_sides>;

    D_OV_CHECK(ok, std::is_same_v<
                       swapped::on_both<ov_base, ov_delta>,
                       ov_merged<ov_delta, ov_base>>);
    D_OV_CHECK(ok, !std::is_same_v<
                       swapped::on_both<ov_base, ov_delta>,
                       ov_merged<ov_base, ov_delta>>);

    // ... including one that merges two keyed entries
    using merging = DWithOnBoth<DKeepDelta, ov_merge_payloads>;

    D_OV_CHECK(ok, std::is_same_v<
                       merging::on_both<ov_entry<7, ov_base>,
                                        ov_entry<7, ov_delta>>,
                       ov_entry<7, ov_merged<ov_base, ov_delta>>>);

    // the two shipped binary metafns behave as their names claim
    D_OV_CHECK(ok, std::is_same_v<
                       DWithOnBoth<DKeepDelta,
                                    always_keep_left>::on_both<ov_base,
                                                               ov_delta>,
                       ov_base>);
    D_OV_CHECK(ok, std::is_same_v<
                       DWithOnBoth<keep_base,
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
  DWithOnBaseOnly: replace the base-only hook via a unary metafunction.
  Tests the following:
  - on_base_only becomes _F<_B>::type; on_both and on_delta_only are
    inherited unchanged
  - the replacement can drop (always_drop), can be the identity
    (identity_t), and can be an arbitrary rewrite (ov_to_third)
  - the trampoline pattern the header prescribes - a struct template
    wrapping an alias template - is accepted where a `template<typename>
    class` is required
*/
bool tests_override_with_on_base_only()
{
    bool ok = true;

    using p = DWithOnBaseOnly<DKeepDelta, always_drop>;
    using h = ov_hooks<p, ov_base, ov_delta>;

    // the replaced hook drops
    D_OV_CHECK(ok, std::is_same_v<h::base_only, dropped>);
    D_OV_CHECK(ok, is_dropped_v<h::base_only>);

    // the inherited hooks are untouched
    D_OV_CHECK(ok, std::is_same_v<h::both,       ov_delta>);
    D_OV_CHECK(ok, std::is_same_v<h::delta_only, ov_delta>);

    D_OV_CHECK(ok, std::is_base_of_v<DKeepDelta, p>);

    // the identity replacement changes nothing observable
    using ident = DWithOnBaseOnly<DKeepDelta, identity_t>;

    D_OV_CHECK(ok, ov_hooks_agree<ident, DKeepDelta>::value);
    D_OV_CHECK(ok, !std::is_same_v<ident, DKeepDelta>);

    // an arbitrary rewrite
    using third = DWithOnBaseOnly<keep_base, ov_to_third>;

    D_OV_CHECK(ok, std::is_same_v<third::on_base_only<ov_base>, ov_third>);
    D_OV_CHECK(ok, std::is_same_v<third::on_both<ov_base, ov_delta>,
                                  ov_base>);

    // the documented trampoline for an alias-shaped metafunction
    using tramp = DWithOnBaseOnly<DDropExtras, ov_identity_trampoline>;

    D_OV_CHECK(ok, std::is_same_v<tramp::on_base_only<ov_base>, ov_base>);
    D_OV_CHECK(ok, ov_hooks_agree<tramp, DDropExtras>::value);

    D_OV_CHECK(ok, std::is_empty_v<p>);
    D_OV_CHECK(ok, OverridePolicy<p>);

    return ok;
}

/*
tests_override_with_on_delta_only
  DWithOnDeltaOnly: replace the extension hook.
  Tests the following:
  - on_delta_only becomes _F<_D>::type; the other two are inherited
  - the header's stated use case in BOTH directions: making a lenient
    policy stricter (DKeepDelta + always_drop), and making a strict one
    lenient
  - the strict-to-lenient direction is the sharp case.  Shadowing a
    strict hook must not merely hide it but prevent its instantiation
    entirely: DWithOnDeltaOnly<DStrictSubset, identity_t> answers
    on_delta_only for a real element, and the fact that this function
    COMPILES is the proof that DStrictSubset's static_assert was never
    reached
*/
bool tests_override_with_on_delta_only()
{
    bool ok = true;

    using p = DWithOnDeltaOnly<DKeepDelta, always_drop>;
    using h = ov_hooks<p, ov_base, ov_delta>;

    // the replaced hook now drops
    D_OV_CHECK(ok, std::is_same_v<h::delta_only, dropped>);

    // the inherited hooks are untouched
    D_OV_CHECK(ok, std::is_same_v<h::base_only, ov_base>);
    D_OV_CHECK(ok, std::is_same_v<h::both,      ov_delta>);

    D_OV_CHECK(ok, std::is_base_of_v<DKeepDelta, p>);

    // lenient -> strict: that composition IS DDropExtras
    D_OV_CHECK(ok, ov_hooks_agree<p, DDropExtras>::value);

    // strict -> lenient: the assert is shadowed out of existence, not
    // merely hidden.  Reaching these lines at all is the claim.
    using relaxed = DWithOnDeltaOnly<DStrictSubset, identity_t>;

    D_OV_CHECK(ok, std::is_same_v<relaxed::on_delta_only<ov_delta>,
                                  ov_delta>);
    D_OV_CHECK(ok, std::is_same_v<relaxed::on_delta_only<int>, int>);
    D_OV_CHECK(ok, ov_hooks_agree<relaxed, DKeepDelta>::value);
    D_OV_CHECK(ok, std::is_base_of_v<DStrictSubset, relaxed>);

    // the base's own alias is still there, unshadowed, under its own
    // qualified name - the derived declaration hides, it does not remove
    D_OV_CHECK(ok, ov_binds_unary<DStrictSubset::on_delta_only>::value);
    D_OV_CHECK(ok, ov_binds_unary<relaxed::on_delta_only>::value);

    // a policy with no on_delta_only at all can be given one
    using given = DWithOnDeltaOnly<ov_lenient_pair, identity_t>;

    D_OV_CHECK(ok, std::is_same_v<given::on_delta_only<ov_delta>,
                                  ov_delta>);
    D_OV_CHECK(ok, ov_hooks_agree<given, DKeepDelta>::value);

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
bool tests_override_combinator_stacking()
{
    bool ok = true;

    // all three hooks replaced, one wrapper each
    using stacked =
        DWithOnDeltaOnly<
            DWithOnBaseOnly<
                DWithOnBoth<keep_base, always_keep_right>,
                ov_to_third>,
            identity_t>;

    using h = ov_hooks<stacked, ov_base, ov_delta>;

    D_OV_CHECK(ok, std::is_same_v<h::both,       ov_delta>);
    D_OV_CHECK(ok, std::is_same_v<h::base_only,  ov_third>);
    D_OV_CHECK(ok, std::is_same_v<h::delta_only, ov_delta>);

    // the chain is transitive
    D_OV_CHECK(ok, std::is_base_of_v<keep_base, stacked>);
    D_OV_CHECK(ok, std::is_base_of_v<
                       DWithOnBoth<keep_base, always_keep_right>,
                       stacked>);

    // re-wrapping one hook: the outermost wrapper wins
    using twice = DWithOnBoth<DWithOnBoth<keep_base, always_keep_left>,
                               always_keep_right>;

    D_OV_CHECK(ok, std::is_same_v<twice::on_both<ov_base, ov_delta>,
                                  ov_delta>);
    D_OV_CHECK(ok, std::is_same_v<
                       DWithOnBoth<twice, always_keep_left>::on_both<
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
  - keep_base, DDropExtras and DDropUnmatchedBase can each be
    reconstructed from DKeepDelta by replacing exactly one hook, and
    DKeepDelta can be reconstructed from DDropExtras.  Each equivalence
    is a statement about BOTH the primitive and the combinator, so a
    change to either side breaks it
  - a reconstruction is behaviourally equal but NOT type-equal, which is
    what a downstream engine dispatching on the policy TYPE must know
  - the two orders of applying two different combinators agree
  - DStrictSubset is reconstructible only up to its probed hooks, and is
    checked with the reader that does not name on_delta_only
*/
bool tests_override_combinator_equivalences()
{
    bool ok = true;

    // keep_base = DKeepDelta with the overlap hook taking the left operand
    D_OV_CHECK(ok, ov_hooks_agree<
                       DWithOnDeltaOnly<
                           DWithOnBoth<DKeepDelta, always_keep_left>,
                           always_drop>,
                       keep_base>::value);

    // DDropExtras = DKeepDelta that refuses to extend
    D_OV_CHECK(ok, ov_hooks_agree<
                       DWithOnDeltaOnly<DKeepDelta, always_drop>,
                       DDropExtras>::value);

    // DKeepDelta = DDropExtras that allows extension again
    D_OV_CHECK(ok, ov_hooks_agree<
                       DWithOnDeltaOnly<DDropExtras, identity_t>,
                       DKeepDelta>::value);

    // DDropUnmatchedBase = DKeepDelta that discards unmatched base
    D_OV_CHECK(ok, ov_hooks_agree<
                       DWithOnBaseOnly<DKeepDelta, always_drop>,
                       DDropUnmatchedBase>::value);

    // keep_base = DDropExtras with the overlap hook flipped
    D_OV_CHECK(ok, ov_hooks_agree<
                       DWithOnBoth<DDropExtras, always_keep_left>,
                       keep_base>::value);

    // DStrictSubset, up to the hooks that may be instantiated
    D_OV_CHECK(ok, ov_probed_hooks_agree<
                       DWithOnBoth<keep_base, always_keep_right>,
                       DStrictSubset>::value);
    D_OV_CHECK(ok, ov_probed_hooks_agree<DDropExtras,
                                         DStrictSubset>::value);

    // behavioural equality is not type equality
    D_OV_CHECK(ok, !std::is_same_v<
                       DWithOnDeltaOnly<DKeepDelta, always_drop>,
                       DDropExtras>);
    D_OV_CHECK(ok, !std::is_same_v<
                       DWithOnBaseOnly<DKeepDelta, always_drop>,
                       DDropUnmatchedBase>);

    // the two orders of two different combinators agree
    using ab = DWithOnBaseOnly<DWithOnBoth<DKeepDelta,
                                              always_keep_right>,
                                 always_drop>;
    using ba = DWithOnBoth<DWithOnBaseOnly<DKeepDelta, always_drop>,
                            always_keep_right>;

    D_OV_CHECK(ok, ov_hooks_agree<ab, ba>::value);
    D_OV_CHECK(ok, !std::is_same_v<ab, ba>);
    D_OV_CHECK(ok, ov_hooks_agree<ab, DDropUnmatchedBase>::value);

    // the SAME two combinators over keep_base instead land on a policy
    // the module does not ship: base dropped, extensions dropped,
    // overlap taking delta - an intersection.  Worth pinning, because it
    // shows the combinators span strictly more than the five named
    // primitives, and because it is the trap the equivalences above walk
    // past: which primitive you start from decides what the inherited
    // third hook contributes
    using intersect = DWithOnBaseOnly<DWithOnBoth<keep_base,
                                                     always_keep_right>,
                                        always_drop>;

    D_OV_CHECK(ok, is_dropped_v<intersect::on_base_only<ov_base>>);
    D_OV_CHECK(ok, is_dropped_v<intersect::on_delta_only<ov_delta>>);
    D_OV_CHECK(ok, std::is_same_v<intersect::on_both<ov_base, ov_delta>,
                                  ov_delta>);
    D_OV_CHECK(ok, OverridePolicy<intersect>);
    D_OV_CHECK(ok, !ov_hooks_agree<intersect, DDropUnmatchedBase>::value);
    D_OV_CHECK(ok, !ov_hooks_agree<intersect, DDropExtras>::value);
    D_OV_CHECK(ok, !ov_hooks_agree<intersect, keep_base>::value);

    // the identity replacement is the algebra's unit on every hook
    D_OV_CHECK(ok, ov_hooks_agree<
                       DWithOnBaseOnly<keep_base, identity_t>,
                       keep_base>::value);
    D_OV_CHECK(ok, ov_hooks_agree<
                       DWithOnBoth<DKeepDelta, always_keep_right>,
                       DKeepDelta>::value);
    D_OV_CHECK(ok, ov_hooks_agree<
                       DWithOnDeltaOnly<DDropUnmatchedBase, identity_t>,
                       DDropUnmatchedBase>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
