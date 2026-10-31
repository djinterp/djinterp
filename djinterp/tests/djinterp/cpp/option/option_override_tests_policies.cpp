/*******************************************************************************
* djinterp [test]                             option_override_tests_policies.cpp
*
*   Section V of the option_override.hpp suite: the ready-made policy aliases.
*
*     DOverrideReplace = DKeepDelta            (standard override)
*     DOverrideKeep    = keep_base             (base wins; delta extras dropped)
*     DOverrideSubset  = DDropExtras           (delta wins overlap; no new keys)
*     DOverrideStrict  = DStrictSubset         (delta extension = hard error)
*     DOverrideFilter  = DDropUnmatchedBase   (keep only delta's keys)
*     DArgUnionDelta  = DWithOnBoth<DKeepDelta, merge_args_union>
*
*   The first five are exercised END-TO-END through option_set_override against
* a shared scenario:
*     base  {a:DOvP, b:DOvQ}     delta {b:DOvR, c:DOvS}
* so each policy's on_base_only / on_both / on_delta_only decision is visible
* in one result type per policy.  DOverrideStrict is checked on a delta that is
* a pure SUBSET of base (no extension); a delta that DID introduce a new key
* would be a hard compile error by design (DStrictSubset's static_assert), so
* that path is intentionally not a runtime test.
*
*   DArgUnionDelta is verified BOTH through the engine and at the policy-hook
* level.  The engine path relies on merge_args_union having a defined primary
* template (so DArgUnionDelta satisfies the container-agnostic OverridePolicy
* concept, whose probe forms on_both<int,int> == merge_args_union<int,int>);
* the corrected option_override.hpp provides that primary.  If a build uses a
* merge_args_union with only the option<> partial specializations (the original
* forward-declared-only primary), DArgUnionDelta will not satisfy
* OverridePolicy and only the hook-level test below is applicable - drop
* policy_arg_union_delta_engine in that case.  The hook-level test pins the
* merge semantics directly and holds either way.
*
*
* path:      /tests/djinterp/cpp/option/option_override_tests_policies.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.09.21
*******************************************************************************/

// std
#include <type_traits>
// djinterp
#include "./option_override_tests.hpp"


NS_DJINTERP
NS_TESTING


#if ( D_ENV_LANG_IS_CPP20_OR_HIGHER &&                                        \
      D_ENV_CPP_FEATURE_LANG_CONCEPTS )

namespace  // the shared base/delta scenario, local to this TU
{
    using DScenarioBase  = option_set<option<ov_key::a, DOvP>,
                                       option<ov_key::b, DOvQ>>;
    using DScenarioDelta = option_set<option<ov_key::b, DOvR>,
                                       option<ov_key::c, DOvS>>;
}

// policy_override_replace
//   DKeepDelta: base-only a kept; shared b -> delta; delta-only c kept.
bool policy_override_replace()
{
    constexpr bool ok =
        std::is_same<
            option_set_override_t<DScenarioBase, DScenarioDelta, DOverrideReplace>,
            option_set<option<ov_key::a, DOvP>,
                       option<ov_key::b, DOvR>,
                       option<ov_key::c, DOvS>>>::value;

    static_assert(ok, "override_replace: keep a, replace b with delta, add c");
    return ok;
}

// policy_override_keep
//   keep_base: base-only a kept; shared b -> base wins; delta-only c dropped.
bool policy_override_keep()
{
    constexpr bool ok =
        std::is_same<
            option_set_override_t<DScenarioBase, DScenarioDelta, DOverrideKeep>,
            option_set<option<ov_key::a, DOvP>,
                       option<ov_key::b, DOvQ>>>::value;

    static_assert(ok, "override_keep: base wins on b, delta-only c dropped");
    return ok;
}

// policy_override_subset
//   DDropExtras: base-only a kept; shared b -> delta wins; delta-only c
// dropped (delta may refine but not extend).
bool policy_override_subset()
{
    constexpr bool ok =
        std::is_same<
            option_set_override_t<DScenarioBase, DScenarioDelta, DOverrideSubset>,
            option_set<option<ov_key::a, DOvP>,
                       option<ov_key::b, DOvR>>>::value;

    static_assert(ok, "override_subset: delta wins on b, delta-only c dropped");
    return ok;
}

// policy_override_filter
//   DDropUnmatchedBase: base-only a dropped; shared b -> delta; delta-only c
// kept.  Only delta's keys survive (result ordered b then c).
bool policy_override_filter()
{
    constexpr bool ok =
        std::is_same<
            option_set_override_t<DScenarioBase, DScenarioDelta, DOverrideFilter>,
            option_set<option<ov_key::b, DOvR>,
                       option<ov_key::c, DOvS>>>::value;

    static_assert(ok, "override_filter: keep only delta's keys (b then c)");
    return ok;
}

// policy_override_strict_accepts_subset
//   DStrictSubset: a delta that is a pure subset of base (no new key) is
// accepted; shared b -> delta wins, base-only a kept.  (A delta that
// introduced a new key would be a hard compile error by design.)
bool policy_override_strict_accepts_subset()
{
    using subset_delta = option_set<option<ov_key::b, DOvR>>;  // subset of base's keys

    constexpr bool ok =
        std::is_same<
            option_set_override_t<DScenarioBase, subset_delta, DOverrideStrict>,
            option_set<option<ov_key::a, DOvP>,
                       option<ov_key::b, DOvR>>>::value;

    static_assert(ok, "override_strict: subset delta accepted; b replaced, a kept");
    return ok;
}

// policy_arg_union_delta_hooks
//   DArgUnionDelta at the hook level (see the file header for why not through
// the engine): on_both merges args delta-first then base; on_base_only and
// on_delta_only are inherited from DKeepDelta (identity on base / delta); the
// unary on_both variants collapse to the present side's args.
bool policy_arg_union_delta_hooks()
{
    constexpr bool ok =
        // on_both: option<K,base...> + option<K,delta...> -> option<K,delta...,base...>
        std::is_same<
            typename DArgUnionDelta::template on_both<
                option<ov_key::b, DOvQ>, option<ov_key::b, DOvR>>,
            option<ov_key::b, DOvR, DOvQ>>::value                               &&
        // on_base_only: inherited DKeepDelta -> the base option unchanged
        std::is_same<
            typename DArgUnionDelta::template on_base_only<option<ov_key::a, DOvP>>,
            option<ov_key::a, DOvP>>::value                                      &&
        // on_delta_only: inherited DKeepDelta -> the delta option unchanged
        std::is_same<
            typename DArgUnionDelta::template on_delta_only<option<ov_key::c, DOvS>>,
            option<ov_key::c, DOvS>>::value                                      &&
        // unary base: merge yields the delta args
        std::is_same<
            typename DArgUnionDelta::template on_both<
                option<ov_key::b>, option<ov_key::b, DOvR>>,
            option<ov_key::b, DOvR>>::value                                      &&
        // unary delta: merge yields the base args
        std::is_same<
            typename DArgUnionDelta::template on_both<
                option<ov_key::b, DOvQ>, option<ov_key::b>>,
            option<ov_key::b, DOvQ>>::value;

    static_assert(ok, "arg_union_delta hooks: on_both merges (delta-first); base/delta hooks identity");
    return ok;
}

// policy_arg_union_delta_engine
//   DArgUnionDelta END-TO-END through option_set_override (requires the
// corrected merge_args_union primary; see the file header).  On the shared
// scenario: base-only a is kept as-is (its args are NOT merged against a
// missing delta - the engine's lazy branch guards that); shared b MERGES args
// delta-first then base (option<b, DOvR, DOvQ>); delta-only c is kept.
bool policy_arg_union_delta_engine()
{
    constexpr bool ok =
        OverridePolicy<DArgUnionDelta>                                         &&
        std::is_same<
            option_set_override_t<DScenarioBase, DScenarioDelta, DArgUnionDelta>,
            option_set<option<ov_key::a, DOvP>,
                       option<ov_key::b, DOvR, DOvQ>,
                       option<ov_key::c, DOvS>>>::value;

    static_assert(ok, "arg_union_delta via engine: keep a, merge b (delta-first), add c");
    return ok;
}

#endif  // C++20 concepts available


// ---------------------------------------------------------------------------
// block provider  (empty below C++20)
// ---------------------------------------------------------------------------
::djinterp::test::block_spec option_override_policies_block()
{
    ::djinterp::test::block_spec b;
    b.name       = "V. ready-made policies";
    b.descriptor = "replace / keep / subset / strict / filter / arg_union_delta via engine (+ arg_union hooks)";

#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
    b.tests = {
        { "policy_override_replace",
          "keep_delta: keep a, replace b, add c",
          &policy_override_replace },
        { "policy_override_keep",
          "keep_base: base wins on b, delta-only c dropped",
          &policy_override_keep },
        { "policy_override_subset",
          "drop_extras: delta wins on b, delta-only c dropped",
          &policy_override_subset },
        { "policy_override_filter",
          "drop_unmatched_base: keep only delta's keys (b then c)",
          &policy_override_filter },
        { "policy_override_strict_accepts_subset",
          "strict_subset: subset delta accepted (extension would hard-error)",
          &policy_override_strict_accepts_subset },
        { "policy_arg_union_delta_hooks",
          "arg_union_delta hooks: on_both merges delta-first; base/delta identity",
          &policy_arg_union_delta_hooks },
        { "policy_arg_union_delta_engine",
          "arg_union_delta via engine: keep a, merge b (delta-first), add c",
          &policy_arg_union_delta_engine },
    };
#else
    b.descriptor = "V. ready-made policies (skipped: option_override.hpp requires C++20)";
#endif

    return b;
}


NS_END  // testing
NS_END  // djinterp
