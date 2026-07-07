/******************************************************************************
* djinterp [test]                             option_override_tests_engine.cpp
*
*   Section IV of the option_override.hpp suite: the option_set_override engine
* (and its internal tuple_to_option_set lift).
*
*   The engine walks two option_sets - A (base) and B (delta) - and produces a
* new option_set:
*     - for each option in A: on_both<A_opt, B_opt> if its key is in B, else
*       on_base_only<A_opt>;
*     - for each option in B whose key is NOT in A: on_delta_only<B_opt>;
*   with `dropped` results filtered out.  The result PRESERVES A's ordering for
* A's keys, then appends B-only keys in B's order.
*
*   This section fixes the POLICY at override_replace (keep_delta) and probes
* the ENGINE's structural behavior: the tuple<->option_set lift, empty-operand
* handling on both sides, disjoint-vs-overlapping ordering, whole-option
* replacement on an overlap, and the _t / ::type equivalence.  Per-policy
* decision tables live in section V.
*
*   Result verification is by exact type identity: option_set<Opts...> is a
* class template whose identity is its ordered entry pack, and the engine
* yields option_set<...> via tuple_to_option_set, so std::is_same against a
* hand-written option_set<...> is a precise check of both contents and order.
*
*
* path:      /tests/djinterp/core/option/option_override_tests_engine.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

// std
#include <tuple>
#include <type_traits>
// djinterp
#include "option_override_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS

// engine_tuple_to_option_set
//   internal::tuple_to_option_set lifts a std::tuple<options...> back to
// option_set<options...>, including the empty tuple -> option_set<>.
bool
engine_tuple_to_option_set()
{
    constexpr bool ok =
        std::is_same<
            typename ::djinterp::internal::tuple_to_option_set<
                std::tuple<option<ov_key::a>, option<ov_key::b, ov_p>>>::type,
            option_set<option<ov_key::a>, option<ov_key::b, ov_p>>>::value       &&
        std::is_same<
            typename ::djinterp::internal::tuple_to_option_set<std::tuple<>>::type,
            option_set<>>::value;

    static_assert(ok, "tuple_to_option_set: tuple<opts...> -> option_set<opts...> (incl. empty)");
    return ok;
}

// engine_result_is_option_set
//   whatever the operands, the engine yields an option_set<...> specialization.
bool
engine_result_is_option_set()
{
    using result = option_set_override_t<
        option_set<option<ov_key::a, ov_p>>,
        option_set<option<ov_key::a, ov_q>>,
        override_replace>;

    constexpr bool ok = is_option_set_v<result>;

    static_assert(ok, "option_set_override_t yields an option_set<...>");
    return ok;
}

// engine_empty_base
//   an empty base means every delta key is a B-only extension: under
// keep_delta they are all kept, in B's order.
bool
engine_empty_base()
{
    constexpr bool ok =
        std::is_same<
            option_set_override_t<option_set<>,
                                  option_set<option<ov_key::a, ov_p>,
                                             option<ov_key::b, ov_q>>,
                                  override_replace>,
            option_set<option<ov_key::a, ov_p>, option<ov_key::b, ov_q>>>::value;

    static_assert(ok, "empty base: all delta keys kept as B-only extensions (B order)");
    return ok;
}

// engine_empty_delta
//   an empty delta means every base key is base-only: under keep_delta they
// pass through unchanged, in A's order.
bool
engine_empty_delta()
{
    constexpr bool ok =
        std::is_same<
            option_set_override_t<option_set<option<ov_key::a>,
                                             option<ov_key::b, ov_p>>,
                                  option_set<>,
                                  override_replace>,
            option_set<option<ov_key::a>, option<ov_key::b, ov_p>>>::value;

    static_assert(ok, "empty delta: base passes through unchanged (A order)");
    return ok;
}

// engine_both_empty
//   two empty operands produce the empty option_set.
bool
engine_both_empty()
{
    constexpr bool ok =
        std::is_same<
            option_set_override_t<option_set<>, option_set<>, override_replace>,
            option_set<>>::value;

    static_assert(ok, "empty base + empty delta -> option_set<>");
    return ok;
}

// engine_disjoint_preserves_order
//   with no shared keys, the result is A's options (base-only) followed by
// B's options (delta-only) - A-block then B-block.
bool
engine_disjoint_preserves_order()
{
    constexpr bool ok =
        std::is_same<
            option_set_override_t<option_set<option<ov_key::a, ov_p>>,
                                  option_set<option<ov_key::c, ov_r>>,
                                  override_replace>,
            option_set<option<ov_key::a, ov_p>, option<ov_key::c, ov_r>>>::value;

    static_assert(ok, "disjoint keys: [A base-only...] then [B delta-only...]");
    return ok;
}

// engine_overlap_replaces_whole_option
//   on a shared key under keep_delta, on_both yields the WHOLE delta option -
// the base option's args are dropped, not merged (contrast arg_union_delta).
bool
engine_overlap_replaces_whole_option()
{
    constexpr bool ok =
        std::is_same<
            option_set_override_t<option_set<option<ov_key::b, ov_p, ov_q>>,
                                  option_set<option<ov_key::b, ov_r>>,
                                  override_replace>,
            option_set<option<ov_key::b, ov_r>>>::value;

    static_assert(ok, "overlap under keep_delta: whole delta option replaces base (args dropped)");
    return ok;
}

// engine_overlap_preserves_base_order
//   an overridden key stays in its ORIGINAL base position; only its value is
// replaced.  Base {a, b, c} overridden at b keeps order a, b(delta), c.
bool
engine_overlap_preserves_base_order()
{
    constexpr bool ok =
        std::is_same<
            option_set_override_t<
                option_set<option<ov_key::a, ov_p>,
                           option<ov_key::b, ov_q>,
                           option<ov_key::c, ov_r>>,
                option_set<option<ov_key::b, ov_s>>,
                override_replace>,
            option_set<option<ov_key::a, ov_p>,
                       option<ov_key::b, ov_s>,
                       option<ov_key::c, ov_r>>>::value;

    static_assert(ok, "overridden key keeps its base position; only its value changes");
    return ok;
}

// engine_t_alias_matches_struct
//   the _t alias is exactly option_set_override<...>::type.
bool
engine_t_alias_matches_struct()
{
    using a = option_set<option<ov_key::a, ov_p>, option<ov_key::b, ov_q>>;
    using b = option_set<option<ov_key::b, ov_r>, option<ov_key::c, ov_s>>;

    constexpr bool ok =
        std::is_same<
            option_set_override_t<a, b, override_replace>,
            typename option_set_override<a, b, override_replace>::type>::value;

    static_assert(ok, "option_set_override_t<A,B,P> == option_set_override<A,B,P>::type");
    return ok;
}

#endif  // C++20 concepts available


// ---------------------------------------------------------------------------
// block provider  (empty below C++20)
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_override_engine_block()
{
    ::djinterp::test::block_spec b;
    b.name       = "IV. option_set_override engine";
    b.descriptor = "engine structure under keep_delta: lift, empties, ordering, whole-option replace";

#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
    b.tests = {
        { "engine_tuple_to_option_set",
          "tuple_to_option_set: tuple<opts...> -> option_set<opts...> (incl. empty)",
          &engine_tuple_to_option_set },
        { "engine_result_is_option_set",
          "option_set_override_t yields an option_set<...>",
          &engine_result_is_option_set },
        { "engine_empty_base",
          "empty base: all delta keys kept as B-only extensions (B order)",
          &engine_empty_base },
        { "engine_empty_delta",
          "empty delta: base passes through unchanged (A order)",
          &engine_empty_delta },
        { "engine_both_empty",
          "empty + empty -> option_set<>",
          &engine_both_empty },
        { "engine_disjoint_preserves_order",
          "disjoint keys: [A base-only...] then [B delta-only...]",
          &engine_disjoint_preserves_order },
        { "engine_overlap_replaces_whole_option",
          "overlap under keep_delta: whole delta option replaces base",
          &engine_overlap_replaces_whole_option },
        { "engine_overlap_preserves_base_order",
          "overridden key keeps its base position; only its value changes",
          &engine_overlap_preserves_base_order },
        { "engine_t_alias_matches_struct",
          "option_set_override_t<A,B,P> == option_set_override<A,B,P>::type",
          &engine_t_alias_matches_struct },
    };
#else
    b.descriptor = "IV. option_set_override engine (skipped: requires C++20)";
#endif

    return b;
}


NS_END  // testing
NS_END  // djinterp
