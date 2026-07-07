/******************************************************************************
* djinterp [test]                           option_override_tests_policies.cpp
*
*   Section V of the option_override.hpp suite: the ready-made policy aliases.
*
*     override_replace = keep_delta            (standard override)
*     override_keep    = keep_base             (base wins; delta extras dropped)
*     override_subset  = drop_extras           (delta wins overlap; no new keys)
*     override_strict  = strict_subset         (delta extension = hard error)
*     override_filter  = drop_unmatched_base   (keep only delta's keys)
*     arg_union_delta  = with_on_both<keep_delta, merge_args_union>
*
*   The first five are exercised END-TO-END through option_set_override against
* a shared scenario:
*     base  {a:ov_p, b:ov_q}     delta {b:ov_r, c:ov_s}
* so each policy's on_base_only / on_both / on_delta_only decision is visible
* in one result type per policy.  override_strict is checked on a delta that is
* a pure SUBSET of base (no extension); a delta that DID introduce a new key
* would be a hard compile error by design (strict_subset's static_assert), so
* that path is intentionally not a runtime test.
*
*   arg_union_delta is verified BOTH through the engine and at the policy-hook
* level.  The engine path relies on merge_args_union having a defined primary
* template (so arg_union_delta satisfies the container-agnostic OverridePolicy
* concept, whose probe forms on_both<int,int> == merge_args_union<int,int>);
* the corrected option_override.hpp provides that primary.  If a build uses a
* merge_args_union with only the option<> partial specializations (the original
* forward-declared-only primary), arg_union_delta will not satisfy
* OverridePolicy and only the hook-level test below is applicable - drop
* policy_arg_union_delta_engine in that case.  The hook-level test pins the
* merge semantics directly and holds either way.
*
*
* path:      /tests/djinterp/core/option/option_override_tests_policies.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

// std
#include <type_traits>
// djinterp
#include "option_override_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

namespace  // the shared base/delta scenario, local to this TU
{
    using scenario_base  = option_set<option<ov_key::a, ov_p>,
                                       option<ov_key::b, ov_q>>;
    using scenario_delta = option_set<option<ov_key::b, ov_r>,
                                       option<ov_key::c, ov_s>>;
}

// policy_override_replace
//   keep_delta: base-only a kept; shared b -> delta; delta-only c kept.
bool
policy_override_replace()
{
    constexpr bool ok =
        std::is_same<
            option_set_override_t<scenario_base, scenario_delta, override_replace>,
            option_set<option<ov_key::a, ov_p>,
                       option<ov_key::b, ov_r>,
                       option<ov_key::c, ov_s>>>::value;

    static_assert(ok, "override_replace: keep a, replace b with delta, add c");
    return ok;
}

// policy_override_keep
//   keep_base: base-only a kept; shared b -> base wins; delta-only c dropped.
bool
policy_override_keep()
{
    constexpr bool ok =
        std::is_same<
            option_set_override_t<scenario_base, scenario_delta, override_keep>,
            option_set<option<ov_key::a, ov_p>,
                       option<ov_key::b, ov_q>>>::value;

    static_assert(ok, "override_keep: base wins on b, delta-only c dropped");
    return ok;
}

// policy_override_subset
//   drop_extras: base-only a kept; shared b -> delta wins; delta-only c
// dropped (delta may refine but not extend).
bool
policy_override_subset()
{
    constexpr bool ok =
        std::is_same<
            option_set_override_t<scenario_base, scenario_delta, override_subset>,
            option_set<option<ov_key::a, ov_p>,
                       option<ov_key::b, ov_r>>>::value;

    static_assert(ok, "override_subset: delta wins on b, delta-only c dropped");
    return ok;
}

// policy_override_filter
//   drop_unmatched_base: base-only a dropped; shared b -> delta; delta-only c
// kept.  Only delta's keys survive (result ordered b then c).
bool
policy_override_filter()
{
    constexpr bool ok =
        std::is_same<
            option_set_override_t<scenario_base, scenario_delta, override_filter>,
            option_set<option<ov_key::b, ov_r>,
                       option<ov_key::c, ov_s>>>::value;

    static_assert(ok, "override_filter: keep only delta's keys (b then c)");
    return ok;
}

// policy_override_strict_accepts_subset
//   strict_subset: a delta that is a pure subset of base (no new key) is
// accepted; shared b -> delta wins, base-only a kept.  (A delta that
// introduced a new key would be a hard compile error by design.)
bool
policy_override_strict_accepts_subset()
{
    using subset_delta = option_set<option<ov_key::b, ov_r>>;  // subset of base's keys

    constexpr bool ok =
        std::is_same<
            option_set_override_t<scenario_base, subset_delta, override_strict>,
            option_set<option<ov_key::a, ov_p>,
                       option<ov_key::b, ov_r>>>::value;

    static_assert(ok, "override_strict: subset delta accepted; b replaced, a kept");
    return ok;
}

// policy_arg_union_delta_hooks
//   arg_union_delta at the hook level (see the file header for why not through
// the engine): on_both merges args delta-first then base; on_base_only and
// on_delta_only are inherited from keep_delta (identity on base / delta); the
// unary on_both variants collapse to the present side's args.
bool
policy_arg_union_delta_hooks()
{
    constexpr bool ok =
        // on_both: option<K,base...> + option<K,delta...> -> option<K,delta...,base...>
        std::is_same<
            typename arg_union_delta::template on_both<
                option<ov_key::b, ov_q>, option<ov_key::b, ov_r>>,
            option<ov_key::b, ov_r, ov_q>>::value                               &&
        // on_base_only: inherited keep_delta -> the base option unchanged
        std::is_same<
            typename arg_union_delta::template on_base_only<option<ov_key::a, ov_p>>,
            option<ov_key::a, ov_p>>::value                                      &&
        // on_delta_only: inherited keep_delta -> the delta option unchanged
        std::is_same<
            typename arg_union_delta::template on_delta_only<option<ov_key::c, ov_s>>,
            option<ov_key::c, ov_s>>::value                                      &&
        // unary base: merge yields the delta args
        std::is_same<
            typename arg_union_delta::template on_both<
                option<ov_key::b>, option<ov_key::b, ov_r>>,
            option<ov_key::b, ov_r>>::value                                      &&
        // unary delta: merge yields the base args
        std::is_same<
            typename arg_union_delta::template on_both<
                option<ov_key::b, ov_q>, option<ov_key::b>>,
            option<ov_key::b, ov_q>>::value;

    static_assert(ok, "arg_union_delta hooks: on_both merges (delta-first); base/delta hooks identity");
    return ok;
}

// policy_arg_union_delta_engine
//   arg_union_delta END-TO-END through option_set_override (requires the
// corrected merge_args_union primary; see the file header).  On the shared
// scenario: base-only a is kept as-is (its args are NOT merged against a
// missing delta - the engine's lazy branch guards that); shared b MERGES args
// delta-first then base (option<b, ov_r, ov_q>); delta-only c is kept.
bool
policy_arg_union_delta_engine()
{
    constexpr bool ok =
        OverridePolicy<arg_union_delta>                                         &&
        std::is_same<
            option_set_override_t<scenario_base, scenario_delta, arg_union_delta>,
            option_set<option<ov_key::a, ov_p>,
                       option<ov_key::b, ov_r, ov_q>,
                       option<ov_key::c, ov_s>>>::value;

    static_assert(ok, "arg_union_delta via engine: keep a, merge b (delta-first), add c");
    return ok;
}

#endif  // C++20 concepts available


// ---------------------------------------------------------------------------
// block provider  (empty below C++20)
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_override_policies_block()
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
