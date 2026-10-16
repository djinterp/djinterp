/*******************************************************************************
* djinterp [core]                                      override_tests_engine.cpp
*
*
* path:      /tests/djinterp/core/meta/override_tests_engine.cpp
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "override_tests.hpp"


NS_DJINTERP
NS_TESTING


// ---------------------------------------------------------------------------
//  the standard scenario
// ---------------------------------------------------------------------------
//   Base carries keys 1, 2 and 4; delta carries 2 and 3.  That gives one
// base-only key (1 and 4), one overlapping key (2) and one delta-only key
// (3), so a single pair of lists exercises all three hooks at once and the
// four results below differ from one another in every interesting way.

// ov_std_base
//   type: the base side of the standard scenario.
using ov_std_base = ov_list<ov_entry<1, ov_base>,
                            ov_entry<2, ov_base>,
                            ov_entry<4, ov_base>>;

// ov_std_delta
//   type: the delta side - one overlap, one extension.
using ov_std_delta = ov_list<ov_entry<2, ov_delta>,
                             ov_entry<3, ov_delta>>;

// ov_std_subset
//   type: a delta that is a strict SUBSET of base's keys, so no policy is
// ever asked about an extension.
using ov_std_subset = ov_list<ov_entry<2, ov_delta>>;


/*
tests_override_engine_primitives
  The five primitives driven end to end by a real walk.
  Tests the following:
  - each primitive produces exactly the documented result list, with the
    `dropped` positions filtered out by the engine rather than appearing
    in the output
  - result ORDER is base order followed by delta's extensions in delta
    order, which is what makes the results comparable at all
  - strict_subset and a policy whose on_delta_only is outright ill-formed
    both survive a delta that introduces no new key: the engine's lazy
    lookup never consults them, and this function compiling is that claim
*/
bool
tests_override_engine_primitives()
{
    bool ok = true;

    // keep_base: base wins on the overlap, delta's extension discarded
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<keep_base, ov_std_base, ov_std_delta>,
                       ov_list<ov_entry<1, ov_base>,
                               ov_entry<2, ov_base>,
                               ov_entry<4, ov_base>>>);

    // keep_delta: delta wins on the overlap and may extend.  Note the
    // extension lands last, after the whole base pass
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<keep_delta, ov_std_base, ov_std_delta>,
                       ov_list<ov_entry<1, ov_base>,
                               ov_entry<2, ov_delta>,
                               ov_entry<4, ov_base>,
                               ov_entry<3, ov_delta>>>);

    // drop_extras: keep_delta's overlap, keep_base's refusal to extend
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<drop_extras, ov_std_base, ov_std_delta>,
                       ov_list<ov_entry<1, ov_base>,
                               ov_entry<2, ov_delta>,
                               ov_entry<4, ov_base>>>);

    // drop_unmatched_base: filter to delta - only delta's keys survive,
    // and they carry delta's payloads
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<drop_unmatched_base,
                                  ov_std_base, ov_std_delta>,
                       ov_list<ov_entry<2, ov_delta>,
                               ov_entry<3, ov_delta>>>);

    // strict_subset over a genuine subset: same answer as drop_extras,
    // and the assert is never reached
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<strict_subset,
                                  ov_std_base, ov_std_subset>,
                       ov_list<ov_entry<1, ov_base>,
                               ov_entry<2, ov_delta>,
                               ov_entry<4, ov_base>>>);
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<strict_subset,
                                  ov_std_base, ov_std_subset>,
                       ov_apply_t<drop_extras,
                                  ov_std_base, ov_std_subset>>);

    // the same laziness, stated without a static_assert in play: this
    // policy's on_delta_only is ill-formed for EVERY argument, so the
    // walk touching it even once would fail the build
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<ov_hostile_delta,
                                  ov_std_base, ov_std_subset>,
                       ov_list<ov_entry<1, ov_base>,
                               ov_entry<2, ov_delta>,
                               ov_entry<4, ov_base>>>);

    // no two primitives agree on this scenario except where documented
    D_OV_CHECK(ok, !std::is_same_v<
                       ov_apply_t<keep_base, ov_std_base, ov_std_delta>,
                       ov_apply_t<keep_delta, ov_std_base, ov_std_delta>>);
    D_OV_CHECK(ok, !std::is_same_v<
                       ov_apply_t<drop_extras, ov_std_base, ov_std_delta>,
                       ov_apply_t<keep_delta, ov_std_base, ov_std_delta>>);

    return ok;
}

/*
tests_override_engine_edge_cases
  The degenerate shapes a real container will eventually hand an engine.
  Tests the following:
  - both sides empty, base empty, delta empty
  - fully disjoint keys and fully overlapping keys
  - with an empty delta, every non-filtering policy is the identity on
    base, and drop_unmatched_base annihilates it
  - with an empty base, only the extension-allowing policies produce
    anything at all
  - applying keep_delta twice with the same delta is idempotent, which is
    the property that makes a chain of overrides well behaved
*/
bool
tests_override_engine_edge_cases()
{
    bool ok = true;

    using empty  = ov_list<>;
    using only_1 = ov_list<ov_entry<1, ov_base>>;
    using only_2 = ov_list<ov_entry<2, ov_delta>>;
    using dup_1  = ov_list<ov_entry<1, ov_delta>>;

    // both sides empty - every policy yields nothing
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<keep_delta, empty, empty>, ov_list<>>);
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<keep_base, empty, empty>, ov_list<>>);
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<strict_subset, empty, empty>, ov_list<>>);
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<drop_unmatched_base, empty, empty>,
                       ov_list<>>);

    // empty delta: the identity on base, except for the filter
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<keep_delta, ov_std_base, empty>,
                       ov_std_base>);
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<keep_base, ov_std_base, empty>,
                       ov_std_base>);
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<drop_extras, ov_std_base, empty>,
                       ov_std_base>);
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<strict_subset, ov_std_base, empty>,
                       ov_std_base>);
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<drop_unmatched_base, ov_std_base, empty>,
                       ov_list<>>);

    // empty base: only the extension-allowing policies produce anything
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<keep_delta, empty, only_2>, only_2>);
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<drop_unmatched_base, empty, only_2>,
                       only_2>);
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<keep_base, empty, only_2>, ov_list<>>);
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<drop_extras, empty, only_2>, ov_list<>>);

    // fully disjoint keys - the overlap hook is never used
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<keep_delta, only_1, only_2>,
                       ov_list<ov_entry<1, ov_base>,
                               ov_entry<2, ov_delta>>>);
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<drop_extras, only_1, only_2>, only_1>);
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<drop_unmatched_base, only_1, only_2>,
                       only_2>);

    // fully overlapping keys - no hook but on_both is ever used, so even
    // strict_subset is safe
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<keep_delta, only_1, dup_1>, dup_1>);
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<keep_base, only_1, dup_1>, only_1>);
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<strict_subset, only_1, dup_1>, dup_1>);
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<drop_unmatched_base, only_1, dup_1>,
                       dup_1>);

    // applying the same delta twice changes nothing the second time
    using once  = ov_apply_t<keep_delta, ov_std_base, ov_std_delta>;
    using twice = ov_apply_t<keep_delta, once, ov_std_delta>;

    D_OV_CHECK(ok, std::is_same_v<once, twice>);

    // and the filtering policy is idempotent for the same reason
    using filtered = ov_apply_t<drop_unmatched_base,
                                ov_std_base, ov_std_delta>;

    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<drop_unmatched_base, filtered,
                                  ov_std_delta>,
                       filtered>);

    return ok;
}

/*
tests_override_engine_custom_policies
  Combinator-built policies driven by the same walk.
  Tests the following:
  - a custom binary merge reaches the engine intact and produces entries
    that are neither operand
  - policies proved behaviourally equal at the hook level really do
    produce identical engine output, which is the statement that the
    hooks are the WHOLE of a policy's observable behaviour
  - relaxing strict_subset with with_on_delta_only lets a delta-only key
    through: here the engine genuinely DOES invoke on_delta_only, so this
    is the case that would fire the base policy's assert if the derived
    alias merely hid it instead of replacing it
*/
bool
tests_override_engine_custom_policies()
{
    bool ok = true;

    // a custom merge on the overlap, extensions still allowed
    using merging = with_on_both<keep_delta, ov_merge_payloads>;

    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<merging, ov_std_base, ov_std_delta>,
                       ov_list<ov_entry<1, ov_base>,
                               ov_entry<2, ov_merged<ov_base, ov_delta>>,
                               ov_entry<4, ov_base>,
                               ov_entry<3, ov_delta>>>);

    // equal at the hooks implies equal through the engine
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<with_on_delta_only<keep_delta,
                                                     always_drop>,
                                  ov_std_base, ov_std_delta>,
                       ov_apply_t<drop_extras, ov_std_base,
                                  ov_std_delta>>);
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<with_on_base_only<keep_delta,
                                                    always_drop>,
                                  ov_std_base, ov_std_delta>,
                       ov_apply_t<drop_unmatched_base, ov_std_base,
                                  ov_std_delta>>);
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<with_on_delta_only<
                                      with_on_both<keep_delta,
                                                   always_keep_left>,
                                      always_drop>,
                                  ov_std_base, ov_std_delta>,
                       ov_apply_t<keep_base, ov_std_base, ov_std_delta>>);

    // ... and replacing only the OVERLAP hook is not enough to get
    // there: that composition still inherits keep_delta's lenient
    // extension hook, so its result differs from keep_base's by exactly
    // delta's new key.  This isolates which hook contributes which part
    // of the walk's output
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<with_on_both<keep_delta,
                                               always_keep_left>,
                                  ov_std_base, ov_std_delta>,
                       ov_list<ov_entry<1, ov_base>,
                               ov_entry<2, ov_base>,
                               ov_entry<4, ov_base>,
                               ov_entry<3, ov_delta>>>);
    D_OV_CHECK(ok, !std::is_same_v<
                       ov_apply_t<with_on_both<keep_delta,
                                               always_keep_left>,
                                  ov_std_base, ov_std_delta>,
                       ov_apply_t<keep_base, ov_std_base, ov_std_delta>>);

    // the sharp case: a relaxed strict_subset over a delta that DOES
    // extend.  The engine's lazy lookup resolves to false here, so
    // on_delta_only is really invoked - and it must reach the derived
    // alias, not the base's assert
    using relaxed = with_on_delta_only<strict_subset, identity_t>;

    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<relaxed, ov_std_base, ov_std_delta>,
                       ov_list<ov_entry<1, ov_base>,
                               ov_entry<2, ov_delta>,
                               ov_entry<4, ov_base>,
                               ov_entry<3, ov_delta>>>);
    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<relaxed, ov_std_base, ov_std_delta>,
                       ov_apply_t<keep_delta, ov_std_base, ov_std_delta>>);

    // a fully stacked policy still walks correctly
    using stacked =
        with_on_delta_only<
            with_on_base_only<
                with_on_both<keep_base, always_keep_right>,
                always_drop>,
            identity_t>;

    D_OV_CHECK(ok, std::is_same_v<
                       ov_apply_t<stacked, ov_std_base, ov_std_delta>,
                       ov_apply_t<drop_unmatched_base, ov_std_base,
                                  ov_std_delta>>);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
