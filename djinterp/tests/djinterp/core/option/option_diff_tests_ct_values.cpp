/******************************************************************************
* djinterp [test]                              option_diff_tests_ct_values.cpp
*
*   PART A / A.III + A.IV - compile-time value-level diff traits
* (option_set_changed_keys, option_set_unchanged_keys) and the diff summary
* (option_set_diff_count, option_set_value_equal).  Every trait here takes a
* unary extractor; option_diff.hpp's `= extract_actual` default is undefined
* (see the header note in option_diff_tests.hpp), so all instantiations pass
* the explicit extract_tval, which reports value_present<v> for opt<key,v>
* and value_absent for a bare key_opt<key>.
*
*   Fixture values (a/b/c/d/e = alpha..epsilon):
*     base    a:1  b:2  c:3
*     derived a:1  b:99 d:4     -> common {a,b}; a unchanged, b changed
*
* path:      /tests/djinterp/core/option/option_diff_tests_ct_values.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.08
******************************************************************************/

#include "./option_diff_tests.hpp"


NS_DJINTERP
NS_TESTING


// valued fixtures
using v_abc  = oset<opt<diff_key::alpha, 1>,
                    opt<diff_key::beta,  2>,
                    opt<diff_key::gamma, 3>>;                 // a:1 b:2 c:3
using v_abd  = oset<opt<diff_key::alpha, 1>,
                    opt<diff_key::beta,  99>,
                    opt<diff_key::delta, 4>>;                 // a:1 b:99 d:4
using v_abcd = oset<opt<diff_key::alpha, 1>,
                    opt<diff_key::beta,  2>,
                    opt<diff_key::gamma, 3>,
                    opt<diff_key::delta, 4>>;                 // a:1 b:2 c:3 d:4
using v_abce = oset<opt<diff_key::alpha, 1>,
                    opt<diff_key::beta,  20>,
                    opt<diff_key::gamma, 30>,
                    opt<diff_key::epsilon, 5>>;               // a:1 b:20 c:30 e:5
using v_a1   = oset<opt<diff_key::alpha, 1>>;
using v_b1   = oset<opt<diff_key::beta,  1>>;                 // disjoint from v_a1
using a_abs  = oset<key_opt<diff_key::alpha>>;               // a: value-absent
using a_pres = oset<opt<diff_key::alpha, 7>>;               // a: 7
using v_empty = oset<>;

// key_list literals
template<auto... _Keys>
using kl = ::djinterp::key_list<_Keys...>;


::djinterp::test::block_spec
option_diff_ct_values_block()
{
    ::djinterp::test::block_spec block;
    block.name       = "A.III+IV  compile-time value-level diff";
    block.descriptor = "changed / unchanged keys + diff_count / value_equal";

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    // ---- option_set_changed_keys / option_set_unchanged_keys -------------

    block.tests.push_back({
        "changed_keys_basic",
        "common keys whose extracted value differs -> {beta}",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::option_set_changed_keys_t<v_abc, v_abd, extract_tval>,
                kl<diff_key::beta> >;
            static_assert(ok, "changed(abc, abd) == {beta}");
            return ok;
        } });

    block.tests.push_back({
        "unchanged_keys_basic",
        "common keys whose extracted value matches -> {alpha}",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::option_set_unchanged_keys_t<v_abc, v_abd, extract_tval>,
                kl<diff_key::alpha> >;
            static_assert(ok, "unchanged(abc, abd) == {alpha}");
            return ok;
        } });

    block.tests.push_back({
        "changed_unchanged_partition_common",
        "changed and unchanged partition the common-key set exactly",
        []() -> bool
        {
            // common(abc, abd) == {alpha, beta}; changed {beta} + unchanged {alpha}
            constexpr bool changed_ok = same_v<
                ::djinterp::option_set_changed_keys_t<v_abc, v_abd, extract_tval>,
                kl<diff_key::beta> >;
            constexpr bool unchanged_ok = same_v<
                ::djinterp::option_set_unchanged_keys_t<v_abc, v_abd, extract_tval>,
                kl<diff_key::alpha> >;
            static_assert(changed_ok && unchanged_ok, "partition of common");
            return (changed_ok && unchanged_ok);
        } });

    block.tests.push_back({
        "changed_keys_multi_base_order",
        "several changed values reported in BASE order -> {beta,gamma}",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::option_set_changed_keys_t<v_abcd, v_abce, extract_tval>,
                kl<diff_key::beta, diff_key::gamma> >;
            static_assert(ok, "changed(abcd, abce) == {beta,gamma}");
            return ok;
        } });

    block.tests.push_back({
        "unchanged_keys_multi",
        "the one matching common value -> {alpha}",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::option_set_unchanged_keys_t<v_abcd, v_abce, extract_tval>,
                kl<diff_key::alpha> >;
            static_assert(ok, "unchanged(abcd, abce) == {alpha}");
            return ok;
        } });

    block.tests.push_back({
        "changed_keys_identical",
        "identical sets have no changed keys",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::option_set_changed_keys_t<v_abc, v_abc, extract_tval>,
                kl<> >;
            static_assert(ok, "changed(abc, abc) == {}");
            return ok;
        } });

    block.tests.push_back({
        "unchanged_keys_identical",
        "identical sets: every common key is unchanged",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::option_set_unchanged_keys_t<v_abc, v_abc, extract_tval>,
                kl<diff_key::alpha, diff_key::beta, diff_key::gamma> >;
            static_assert(ok, "unchanged(abc, abc) == {alpha,beta,gamma}");
            return ok;
        } });

    block.tests.push_back({
        "changed_unchanged_ignore_key_only_diffs",
        "disjoint sets have no common keys -> both lists empty",
        []() -> bool
        {
            constexpr bool changed_ok = same_v<
                ::djinterp::option_set_changed_keys_t<v_a1, v_b1, extract_tval>,
                kl<> >;
            constexpr bool unchanged_ok = same_v<
                ::djinterp::option_set_unchanged_keys_t<v_a1, v_b1, extract_tval>,
                kl<> >;
            static_assert(changed_ok && unchanged_ok,
                          "changed/unchanged over disjoint == {}");
            return (changed_ok && unchanged_ok);
        } });

    // ---- extractor carrier semantics : value_absent branch ---------------

    block.tests.push_back({
        "absent_equals_absent_is_unchanged",
        "two value-absent carriers compare equal -> unchanged, not changed",
        []() -> bool
        {
            constexpr bool changed_ok = same_v<
                ::djinterp::option_set_changed_keys_t<a_abs, a_abs, extract_tval>,
                kl<> >;
            constexpr bool unchanged_ok = same_v<
                ::djinterp::option_set_unchanged_keys_t<a_abs, a_abs, extract_tval>,
                kl<diff_key::alpha> >;
            static_assert(changed_ok && unchanged_ok, "absent == absent");
            return (changed_ok && unchanged_ok);
        } });

    block.tests.push_back({
        "present_vs_absent_is_changed",
        "present carrier vs absent carrier at the same key -> changed",
        []() -> bool
        {
            constexpr bool changed_ok = same_v<
                ::djinterp::option_set_changed_keys_t<a_pres, a_abs, extract_tval>,
                kl<diff_key::alpha> >;
            constexpr bool unchanged_ok = same_v<
                ::djinterp::option_set_unchanged_keys_t<a_pres, a_abs, extract_tval>,
                kl<> >;
            static_assert(changed_ok && unchanged_ok, "present != absent");
            return (changed_ok && unchanged_ok);
        } });

    // ---- option_set_diff_count -------------------------------------------

    block.tests.push_back({
        "diff_count_basic",
        "changed(1) + added(1) + removed(1) == 3",
        []() -> bool
        {
            constexpr std::size_t n =
                ::djinterp::option_set_diff_count_v<v_abc, v_abd, extract_tval>;
            static_assert(n == 3u, "diff_count(abc, abd) == 3");
            return (n == 3u);
        } });

    block.tests.push_back({
        "diff_count_multi",
        "changed(2) + added(1) + removed(1) == 4",
        []() -> bool
        {
            constexpr std::size_t n =
                ::djinterp::option_set_diff_count_v<v_abcd, v_abce, extract_tval>;
            static_assert(n == 4u, "diff_count(abcd, abce) == 4");
            return (n == 4u);
        } });

    block.tests.push_back({
        "diff_count_counts_key_only_diffs",
        "disjoint sets: 0 changed + 1 added + 1 removed == 2",
        []() -> bool
        {
            // no common keys, so all difference is key-only, yet still counted
            constexpr std::size_t n =
                ::djinterp::option_set_diff_count_v<v_a1, v_b1, extract_tval>;
            static_assert(n == 2u, "diff_count(a, b) == 2");
            return (n == 2u);
        } });

    block.tests.push_back({
        "diff_count_identical_is_zero",
        "identical sets diff to zero (via the struct ::value spelling)",
        []() -> bool
        {
            constexpr std::size_t n =
                ::djinterp::option_set_diff_count<v_abc, v_abc, extract_tval>::value;
            static_assert(n == 0u, "diff_count(abc, abc) == 0");
            return (n == 0u);
        } });

    block.tests.push_back({
        "diff_count_to_empty_is_all_removed",
        "everything removed against the empty set -> 3",
        []() -> bool
        {
            constexpr std::size_t n =
                ::djinterp::option_set_diff_count_v<v_abc, v_empty, extract_tval>;
            static_assert(n == 3u, "diff_count(abc, {}) == 3");
            return (n == 3u);
        } });

    block.tests.push_back({
        "diff_count_empty_empty_is_zero",
        "empty vs empty diffs to zero",
        []() -> bool
        {
            constexpr std::size_t n =
                ::djinterp::option_set_diff_count_v<v_empty, v_empty, extract_tval>;
            static_assert(n == 0u, "diff_count({}, {}) == 0");
            return (n == 0u);
        } });

    // ---- option_set_value_equal ------------------------------------------

    block.tests.push_back({
        "value_equal_true_when_diff_empty",
        "value_equal is true exactly when the diff is empty",
        []() -> bool
        {
            constexpr bool eq =
                ::djinterp::option_set_value_equal_v<v_abc, v_abc, extract_tval>;
            static_assert(eq, "value_equal(abc, abc)");
            return eq;
        } });

    block.tests.push_back({
        "value_equal_false_when_diff_nonempty",
        "any difference makes value_equal false (struct ::value spelling)",
        []() -> bool
        {
            constexpr bool eq =
                ::djinterp::option_set_value_equal<v_abc, v_abd, extract_tval>::value;
            static_assert(!eq, "!value_equal(abc, abd)");
            return !eq;
        } });

    block.tests.push_back({
        "value_equal_empty_empty",
        "two empty sets are value-equal",
        []() -> bool
        {
            constexpr bool eq =
                ::djinterp::option_set_value_equal_v<v_empty, v_empty, extract_tval>;
            static_assert(eq, "value_equal({}, {})");
            return eq;
        } });

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

    return block;
}


NS_END  // testing
NS_END  // djinterp
