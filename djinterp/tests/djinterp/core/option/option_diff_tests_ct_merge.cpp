/******************************************************************************
* djinterp [test]                               option_diff_tests_ct_merge.cpp
*
*   PART A / A.V - the merge bridge.  Covers the merge_mode vocabulary, the
* internal merge_mode_policy map (overwrite -> override_replace;
* add_new_only / keep_existing -> override_keep), and diff_merge_t, which
* applies the mapped policy through the option_set_override engine.
*
*   The diff_merge_t assertions read the merged set back through
* option_set_find_t / option_set_contains_v and lean on option_override.hpp's
* documented whole-option policy semantics: keep_delta replaces on shared
* keys, keep_base retains, and BOTH always append delta-only keys.  This
* section rides the concept-constrained override engine and so is C++20-only.
*
*   Fixtures (a/b/c = alpha/beta/gamma):
*     base  a:1 b:2      delta  b:99 c:3      -> b is the contested key
*
* path:      /tests/djinterp/core/option/option_diff_tests_ct_merge.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.08
******************************************************************************/

#include "./option_diff_tests.hpp"


NS_DJINTERP
NS_TESTING


// merge fixtures
using m_base  = oset<opt<diff_key::alpha, 1>, opt<diff_key::beta, 2>>;   // a:1 b:2
using m_delta = oset<opt<diff_key::beta, 99>, opt<diff_key::gamma, 3>>;  // b:99 c:3
using m_empty = oset<>;
using m_d_only = oset<opt<diff_key::gamma, 3>>;                         // disjoint w/ a

// the individual options, for identity assertions on the merge result
using base_a  = opt<diff_key::alpha, 1>;
using base_b  = opt<diff_key::beta,  2>;
using delta_b = opt<diff_key::beta,  99>;
using delta_c = opt<diff_key::gamma, 3>;


::djinterp::test::block_spec
option_diff_ct_merge_block()
{
    ::djinterp::test::block_spec block;
    block.name       = "A.V  compile-time merge bridge";
    block.descriptor = "merge_mode -> policy map + diff_merge_t (override engine)";

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    // ---- merge_mode vocabulary -------------------------------------------

    block.tests.push_back({
        "merge_mode_enumerators_distinct",
        "add_new_only and keep_existing are distinct enumerators",
        []() -> bool
        {
            // they map to the same policy but remain separate names/values
            constexpr bool ok =
                ( static_cast<int>(merge_mode::add_new_only) !=
                  static_cast<int>(merge_mode::keep_existing) );
            static_assert(ok, "add_new_only != keep_existing (as values)");
            return ok;
        } });

    // ---- merge_mode_policy : the mode -> override-policy map --------------

    block.tests.push_back({
        "policy_overwrite_is_replace",
        "overwrite maps to override_replace (delta wins)",
        []() -> bool
        {
            constexpr bool ok = std::is_same_v<
                typename ::djinterp::internal::merge_mode_policy<
                    merge_mode::overwrite>::type,
                ::djinterp::override_replace >;
            static_assert(ok, "policy(overwrite) == override_replace");
            return ok;
        } });

    block.tests.push_back({
        "policy_add_new_only_is_keep",
        "add_new_only maps to override_keep (base wins)",
        []() -> bool
        {
            constexpr bool ok = std::is_same_v<
                typename ::djinterp::internal::merge_mode_policy<
                    merge_mode::add_new_only>::type,
                ::djinterp::override_keep >;
            static_assert(ok, "policy(add_new_only) == override_keep");
            return ok;
        } });

    block.tests.push_back({
        "policy_keep_existing_is_keep",
        "keep_existing maps to override_keep as well",
        []() -> bool
        {
            constexpr bool ok = std::is_same_v<
                typename ::djinterp::internal::merge_mode_policy<
                    merge_mode::keep_existing>::type,
                ::djinterp::override_keep >;
            static_assert(ok, "policy(keep_existing) == override_keep");
            return ok;
        } });

    // ---- diff_merge_t : default mode is overwrite ------------------------

    block.tests.push_back({
        "diff_merge_default_is_overwrite",
        "the defaulted mode equals an explicit overwrite",
        []() -> bool
        {
            constexpr bool ok = std::is_same_v<
                ::djinterp::diff_merge_t<m_base, m_delta>,
                ::djinterp::diff_merge_t<m_base, m_delta, merge_mode::overwrite> >;
            static_assert(ok, "diff_merge_t<> default == overwrite");
            return ok;
        } });

    // ---- diff_merge_t : overwrite (keep_delta) ---------------------------

    block.tests.push_back({
        "diff_merge_overwrite_delta_wins",
        "overwrite: contested key takes the delta's option",
        []() -> bool
        {
            using merged =
                ::djinterp::diff_merge_t<m_base, m_delta, merge_mode::overwrite>;
            constexpr bool b_is_delta = std::is_same_v<
                ::djinterp::option_set_find_t<merged, diff_key::beta>, delta_b >;
            constexpr bool a_is_base = std::is_same_v<
                ::djinterp::option_set_find_t<merged, diff_key::alpha>, base_a >;
            static_assert(b_is_delta && a_is_base,
                          "overwrite: b<-delta, a<-base");
            return (b_is_delta && a_is_base);
        } });

    block.tests.push_back({
        "diff_merge_overwrite_extends",
        "overwrite: delta-only keys are appended; all keys present",
        []() -> bool
        {
            using merged =
                ::djinterp::diff_merge_t<m_base, m_delta, merge_mode::overwrite>;
            constexpr bool c_is_delta = std::is_same_v<
                ::djinterp::option_set_find_t<merged, diff_key::gamma>, delta_c >;
            constexpr bool has_all =
                ( ::djinterp::option_set_contains_v<merged, diff_key::alpha> &&
                  ::djinterp::option_set_contains_v<merged, diff_key::beta>  &&
                  ::djinterp::option_set_contains_v<merged, diff_key::gamma> );
            static_assert(c_is_delta && has_all, "overwrite extends with c");
            return (c_is_delta && has_all);
        } });

    // ---- diff_merge_t : add_new_only / keep_existing (keep_base) ---------

    block.tests.push_back({
        "diff_merge_add_new_only_base_wins",
        "add_new_only: contested key keeps the base's option",
        []() -> bool
        {
            using merged =
                ::djinterp::diff_merge_t<m_base, m_delta, merge_mode::add_new_only>;
            constexpr bool b_is_base = std::is_same_v<
                ::djinterp::option_set_find_t<merged, diff_key::beta>, base_b >;
            constexpr bool c_added = std::is_same_v<
                ::djinterp::option_set_find_t<merged, diff_key::gamma>, delta_c >;
            static_assert(b_is_base && c_added,
                          "add_new_only: b kept, c still added");
            return (b_is_base && c_added);
        } });

    block.tests.push_back({
        "diff_merge_keep_existing_matches_add_new_only",
        "keep_existing produces the same set as add_new_only",
        []() -> bool
        {
            constexpr bool ok = std::is_same_v<
                ::djinterp::diff_merge_t<m_base, m_delta, merge_mode::keep_existing>,
                ::djinterp::diff_merge_t<m_base, m_delta, merge_mode::add_new_only> >;
            static_assert(ok, "keep_existing == add_new_only");
            return ok;
        } });

    // ---- diff_merge_t : edge cases ---------------------------------------

    block.tests.push_back({
        "diff_merge_disjoint",
        "no shared keys -> union of both, mode-independent",
        []() -> bool
        {
            using ov = ::djinterp::diff_merge_t<
                m_base, m_d_only, merge_mode::overwrite>;
            using an = ::djinterp::diff_merge_t<
                m_base, m_d_only, merge_mode::add_new_only>;
            constexpr bool same_result = std::is_same_v<ov, an>;
            constexpr bool has_all =
                ( ::djinterp::option_set_contains_v<ov, diff_key::alpha> &&
                  ::djinterp::option_set_contains_v<ov, diff_key::beta>  &&
                  ::djinterp::option_set_contains_v<ov, diff_key::gamma> );
            static_assert(same_result && has_all,
                          "disjoint merge is a union either way");
            return (same_result && has_all);
        } });

    block.tests.push_back({
        "diff_merge_into_empty_base",
        "empty base -> merged is exactly the delta",
        []() -> bool
        {
            using merged =
                ::djinterp::diff_merge_t<m_empty, m_delta, merge_mode::overwrite>;
            constexpr bool has_delta =
                ( ::djinterp::option_set_contains_v<merged, diff_key::beta> &&
                  ::djinterp::option_set_contains_v<merged, diff_key::gamma> );
            constexpr bool b_is_delta = std::is_same_v<
                ::djinterp::option_set_find_t<merged, diff_key::beta>, delta_b >;
            static_assert(has_delta && b_is_delta, "merge into {} == delta");
            return (has_delta && b_is_delta);
        } });

    block.tests.push_back({
        "diff_merge_empty_delta",
        "empty delta -> base is preserved unchanged",
        []() -> bool
        {
            using merged =
                ::djinterp::diff_merge_t<m_base, m_empty, merge_mode::overwrite>;
            constexpr bool a_kept = std::is_same_v<
                ::djinterp::option_set_find_t<merged, diff_key::alpha>, base_a >;
            constexpr bool b_kept = std::is_same_v<
                ::djinterp::option_set_find_t<merged, diff_key::beta>, base_b >;
            static_assert(a_kept && b_kept, "merge with {} delta == base");
            return (a_kept && b_kept);
        } });

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

    return block;
}


NS_END  // testing
NS_END  // djinterp
