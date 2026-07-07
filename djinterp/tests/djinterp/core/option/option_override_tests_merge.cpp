/******************************************************************************
* djinterp [test]                              option_override_tests_merge.cpp
*
*   Section II of the option_override.hpp suite: the option-aware merge metafn
* merge_args_union<_B, _D>.
*
*   Semantics (from the header): given a base option and a delta option that
* share a key, produce one option whose args are the DELTA's args first, then
* the BASE's args - option<_Key, _DArgs..., _BArgs...>.  Because the option
* arg-query helpers return the FIRST match, "delta first" means delta wins any
* tag-role lookup without merge_args_union having to know any predicate, and
* it deliberately does NOT deduplicate (duplicate roles are skipped downstream
* by design).  Four partial specializations cover every unary/args combination
* of the two operands; the undefined primary template is never a valid target
* here because both operands are always same-keyed options.
*
*   These tests exercise merge_args_union directly at the type level (its
* natural interface).  Note it powers the arg_union_delta policy in section V,
* whose hook-level behavior is checked there.
*
*
* path:      /tests/djinterp/core/option/option_override_tests_merge.cpp
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

// merge_both_have_args
//   both operands carry args -> delta's args precede base's args, single key.
bool
merge_both_have_args()
{
    constexpr bool ok =
        std::is_same<
            typename merge_args_union<option<ov_key::a, ov_p, ov_q>,
                                      option<ov_key::a, ov_r>>::type,
            option<ov_key::a, ov_r, ov_p, ov_q>>::value;

    static_assert(ok, "merge_args_union: option<K,B...> + option<K,D...> -> option<K,D...,B...>");
    return ok;
}

// merge_base_unary
//   base is unary (no args): the result is exactly the delta's args.
bool
merge_base_unary()
{
    constexpr bool ok =
        std::is_same<
            typename merge_args_union<option<ov_key::a>,
                                      option<ov_key::a, ov_r, ov_s>>::type,
            option<ov_key::a, ov_r, ov_s>>::value;

    static_assert(ok, "merge_args_union: option<K> + option<K,D...> -> option<K,D...>");
    return ok;
}

// merge_delta_unary
//   delta is unary (no args): the result is exactly the base's args.
bool
merge_delta_unary()
{
    constexpr bool ok =
        std::is_same<
            typename merge_args_union<option<ov_key::a, ov_p, ov_q>,
                                      option<ov_key::a>>::type,
            option<ov_key::a, ov_p, ov_q>>::value;

    static_assert(ok, "merge_args_union: option<K,B...> + option<K> -> option<K,B...>");
    return ok;
}

// merge_both_unary
//   both unary: the result is the bare key option.
bool
merge_both_unary()
{
    constexpr bool ok =
        std::is_same<
            typename merge_args_union<option<ov_key::a>, option<ov_key::a>>::type,
            option<ov_key::a>>::value;

    static_assert(ok, "merge_args_union: option<K> + option<K> -> option<K>");
    return ok;
}

// merge_delta_wins_order
//   with one arg each, the delta arg is placed BEFORE the base arg - the
// ordering that gives "delta wins" under first-match arg queries.
bool
merge_delta_wins_order()
{
    constexpr bool ok =
        std::is_same<
            typename merge_args_union<option<ov_key::a, ov_p>,
                                      option<ov_key::a, ov_r>>::type,
            option<ov_key::a, ov_r, ov_p>>::value;

    static_assert(ok, "merge_args_union: delta arg precedes base arg (delta-wins order)");
    return ok;
}

// merge_no_dedupe
//   identical args on both sides are BOTH kept (delta's copy first) - the
// metafn performs no deduplication.
bool
merge_no_dedupe()
{
    constexpr bool ok =
        std::is_same<
            typename merge_args_union<option<ov_key::a, ov_p>,
                                      option<ov_key::a, ov_p>>::type,
            option<ov_key::a, ov_p, ov_p>>::value;

    static_assert(ok, "merge_args_union: duplicate args are preserved, not deduplicated");
    return ok;
}

// merge_preserves_arg_qualifiers
//   arg types are concatenated verbatim, including cv / reference qualifiers,
// in delta-then-base order.
bool
merge_preserves_arg_qualifiers()
{
    constexpr bool ok =
        std::is_same<
            typename merge_args_union<option<ov_key::a, const int>,
                                      option<ov_key::a, int&>>::type,
            option<ov_key::a, int&, const int>>::value;

    static_assert(ok, "merge_args_union: cv/ref-qualified args concatenated verbatim (D then B)");
    return ok;
}

#endif  // C++20 concepts available


// ---------------------------------------------------------------------------
// block provider  (empty below C++20)
// ---------------------------------------------------------------------------
::djinterp::test::block_spec
option_override_merge_block()
{
    ::djinterp::test::block_spec b;
    b.name       = "II. merge_args_union";
    b.descriptor = "option-aware arg union: delta args first, base args second, no dedupe";

#if D_ENV_LANG_IS_CPP20_OR_HIGHER && D_ENV_CPP_FEATURE_LANG_CONCEPTS
    b.tests = {
        { "merge_both_have_args",
          "option<K,B...> + option<K,D...> -> option<K,D...,B...>",
          &merge_both_have_args },
        { "merge_base_unary",
          "option<K> + option<K,D...> -> option<K,D...>",
          &merge_base_unary },
        { "merge_delta_unary",
          "option<K,B...> + option<K> -> option<K,B...>",
          &merge_delta_unary },
        { "merge_both_unary",
          "option<K> + option<K> -> option<K>",
          &merge_both_unary },
        { "merge_delta_wins_order",
          "delta arg precedes base arg (delta-wins ordering)",
          &merge_delta_wins_order },
        { "merge_no_dedupe",
          "duplicate args preserved (no deduplication)",
          &merge_no_dedupe },
        { "merge_preserves_arg_qualifiers",
          "cv/ref-qualified args concatenated verbatim (D then B)",
          &merge_preserves_arg_qualifiers },
    };
#else
    b.descriptor = "II. merge_args_union (skipped: option_override.hpp requires C++20)";
#endif

    return b;
}


NS_END  // testing
NS_END  // djinterp
