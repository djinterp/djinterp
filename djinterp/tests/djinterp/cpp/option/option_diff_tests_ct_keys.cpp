/*******************************************************************************
* djinterp [test]                                  option_diff_tests_ct_keys.cpp
*
*   PART A / A.II - compile-time key-level diff traits (option_set_added_keys,
* option_set_removed_keys, option_set_common_keys) plus direct coverage of the
* A.I key-list set primitives (key_list_append, key_list_filter_present,
* key_list_filter_dispatch) they are built on.  Keys only - values are the
* province of ct_values - so fixtures use the value-absent DKeyOpt<> shape.
*
*
* path:      /tests/djinterp/cpp/option/option_diff_tests_ct_keys.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./option_diff_tests.hpp"


NS_DJINTERP
NS_TESTING


// key-set fixtures (names encode their keys; a/b/c/d/e = alpha..epsilon)
using DKEmpty = DOset<>;
using DKA     = DOset<DKeyOpt<diff_key::alpha>>;
using DKB     = DOset<DKeyOpt<diff_key::beta>>;
using DKAb    = DOset<DKeyOpt<diff_key::alpha>, DKeyOpt<diff_key::beta>>;
using DKAbc   = DOset<DKeyOpt<diff_key::alpha>,
                     DKeyOpt<diff_key::beta>,
                     DKeyOpt<diff_key::gamma>>;
using DKAbd   = DOset<DKeyOpt<diff_key::alpha>,
                     DKeyOpt<diff_key::beta>,
                     DKeyOpt<diff_key::delta>>;
using DKBcd   = DOset<DKeyOpt<diff_key::beta>,
                     DKeyOpt<diff_key::gamma>,
                     DKeyOpt<diff_key::delta>>;
// deliberately out of natural order, to prove result order tracks the source
using DKCab   = DOset<DKeyOpt<diff_key::gamma>,
                     DKeyOpt<diff_key::alpha>,
                     DKeyOpt<diff_key::beta>>;
// base {a,b} vs derived {c,b}: added / removed / common each draw a different
// order, exercising all three orderings in one comparison
using DKCb    = DOset<DKeyOpt<diff_key::gamma>, DKeyOpt<diff_key::beta>>;

// convenience: DKeyList literals
template<auto... _Keys>
using kl = ::djinterp::DKeyList<_Keys...>;


::djinterp::test::block_spec option_diff_ct_keys_block()
{
    ::djinterp::test::block_spec block;
    block.name       = "A.II  compile-time key-level diff";
    block.descriptor = "added / removed / common keys + A.I key-list primitives";

#if D_ENV_LANG_IS_CPP20_OR_HIGHER

    // ---- option_set_added_keys : derived-minus-base, DERIVED order --------

    block.tests.push_back({
        "added_keys_basic",
        "keys in derived but not base -> the derived-only key",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::option_set_added_keys_t<DKAbc, DKAbd>,
                kl<diff_key::delta> >;
            static_assert(ok, "added(abc, abd) == {delta}");
            return ok;
        } });

    block.tests.push_back({
        "added_keys_multi_order",
        "multiple additions preserve DERIVED key order",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::option_set_added_keys_t<DKA, DKBcd>,
                kl<diff_key::beta, diff_key::gamma, diff_key::delta> >;
            static_assert(ok, "added(a, bcd) == {beta,gamma,delta}");
            return ok;
        } });

    block.tests.push_back({
        "added_keys_none_when_subset",
        "derived subset of base -> no additions",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::option_set_added_keys_t<DKAbc, DKAb>,
                kl<> >;
            static_assert(ok, "added(abc, ab) == {}");
            return ok;
        } });

    block.tests.push_back({
        "added_keys_empty_base",
        "empty base -> every derived key is added",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::option_set_added_keys_t<DKEmpty, DKAb>,
                kl<diff_key::alpha, diff_key::beta> >;
            static_assert(ok, "added({}, ab) == {alpha,beta}");
            return ok;
        } });

    block.tests.push_back({
        "added_keys_empty_derived",
        "empty derived -> nothing added",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::option_set_added_keys_t<DKAb, DKEmpty>,
                kl<> >;
            static_assert(ok, "added(ab, {}) == {}");
            return ok;
        } });

    // ---- option_set_removed_keys : base-minus-derived, BASE order ---------

    block.tests.push_back({
        "removed_keys_basic",
        "keys in base but not derived -> the base-only key",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::option_set_removed_keys_t<DKAbc, DKAbd>,
                kl<diff_key::gamma> >;
            static_assert(ok, "removed(abc, abd) == {gamma}");
            return ok;
        } });

    block.tests.push_back({
        "removed_keys_all_order",
        "empty derived -> all base keys removed, in BASE order",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::option_set_removed_keys_t<DKAbc, DKEmpty>,
                kl<diff_key::alpha, diff_key::beta, diff_key::gamma> >;
            static_assert(ok, "removed(abc, {}) == {alpha,beta,gamma}");
            return ok;
        } });

    block.tests.push_back({
        "removed_keys_none_when_superset",
        "base subset of derived -> nothing removed",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::option_set_removed_keys_t<DKAb, DKAbc>,
                kl<> >;
            static_assert(ok, "removed(ab, abc) == {}");
            return ok;
        } });

    block.tests.push_back({
        "removed_keys_empty_base",
        "empty base -> nothing to remove",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::option_set_removed_keys_t<DKEmpty, DKAbc>,
                kl<> >;
            static_assert(ok, "removed({}, abc) == {}");
            return ok;
        } });

    // ---- option_set_common_keys : intersection, FIRST-arg order ----------

    block.tests.push_back({
        "common_keys_basic",
        "keys in both sets, drawn in first-arg order",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::option_set_common_keys_t<DKAbc, DKAbd>,
                kl<diff_key::alpha, diff_key::beta> >;
            static_assert(ok, "common(abc, abd) == {alpha,beta}");
            return ok;
        } });

    block.tests.push_back({
        "common_keys_identical",
        "identical key sets -> the whole set is common",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::option_set_common_keys_t<DKAbc, DKAbc>,
                kl<diff_key::alpha, diff_key::beta, diff_key::gamma> >;
            static_assert(ok, "common(abc, abc) == {alpha,beta,gamma}");
            return ok;
        } });

    block.tests.push_back({
        "common_keys_disjoint",
        "disjoint sets -> empty intersection",
        []() -> bool
        {
            constexpr bool ok = same_v<
                ::djinterp::option_set_common_keys_t<DKA, DKB>,
                kl<> >;
            static_assert(ok, "common(a, b) == {}");
            return ok;
        } });

    block.tests.push_back({
        "common_keys_empty",
        "common with the empty set is empty (either side)",
        []() -> bool
        {
            constexpr bool lhs = same_v<
                ::djinterp::option_set_common_keys_t<DKEmpty, DKAbc>, kl<> >;
            constexpr bool rhs = same_v<
                ::djinterp::option_set_common_keys_t<DKAbc, DKEmpty>, kl<> >;
            static_assert(lhs && rhs, "common with {} == {}");
            return (lhs && rhs);
        } });

    block.tests.push_back({
        "common_keys_order_follows_first",
        "intersection order tracks the FIRST operand, not the second",
        []() -> bool
        {
            // DKCab keys = {gamma, alpha, beta}; intersect with DKAbc
            constexpr bool ok = same_v<
                ::djinterp::option_set_common_keys_t<DKCab, DKAbc>,
                kl<diff_key::gamma, diff_key::alpha, diff_key::beta> >;
            static_assert(ok, "common(cab, abc) == {gamma,alpha,beta}");
            return ok;
        } });

    // ---- the three orderings, side by side -------------------------------

    block.tests.push_back({
        "diff_orderings_are_independent",
        "added=derived order, removed=base order, common=base order",
        []() -> bool
        {
            // base {alpha,beta}, derived {gamma,beta}
            constexpr bool added_ok = same_v<
                ::djinterp::option_set_added_keys_t<DKAb, DKCb>,
                kl<diff_key::gamma> >;                 // derived-only, derived order
            constexpr bool removed_ok = same_v<
                ::djinterp::option_set_removed_keys_t<DKAb, DKCb>,
                kl<diff_key::alpha> >;                 // base-only, base order
            constexpr bool common_ok = same_v<
                ::djinterp::option_set_common_keys_t<DKAb, DKCb>,
                kl<diff_key::beta> >;                  // shared, base order
            static_assert(added_ok && removed_ok && common_ok,
                          "added/removed/common orderings");
            return (added_ok && removed_ok && common_ok);
        } });

    // ---- A.I internals : key_list_append ---------------------------------

    block.tests.push_back({
        "internal_append_nonempty",
        "key_list_append onto a populated list appends at the tail",
        []() -> bool
        {
            constexpr bool ok = same_v<
                typename ::djinterp::internal::key_list_append<
                    kl<diff_key::alpha, diff_key::beta>, diff_key::gamma>::type,
                kl<diff_key::alpha, diff_key::beta, diff_key::gamma> >;
            static_assert(ok, "append(<a,b>, c) == <a,b,c>");
            return ok;
        } });

    block.tests.push_back({
        "internal_append_empty",
        "key_list_append onto the empty list yields a singleton",
        []() -> bool
        {
            constexpr bool ok = same_v<
                typename ::djinterp::internal::key_list_append<
                    kl<>, diff_key::alpha>::type,
                kl<diff_key::alpha> >;
            static_assert(ok, "append(<>, a) == <a>");
            return ok;
        } });

    // ---- A.I internals : key_list_filter_dispatch (both _Want branches) --

    block.tests.push_back({
        "internal_filter_keep_present",
        "filter_dispatch _Want=true keeps source keys present in Other",
        []() -> bool
        {
            // source keys {a,b,c}, Other = DKAb -> keep those in Other -> {a,b}
            constexpr bool ok = same_v<
                typename ::djinterp::internal::key_list_filter_dispatch<
                    kl<diff_key::alpha, diff_key::beta, diff_key::gamma>,
                    DKAb, true>::type,
                kl<diff_key::alpha, diff_key::beta> >;
            static_assert(ok, "filter present(<a,b,c>, {a,b}) == <a,b>");
            return ok;
        } });

    block.tests.push_back({
        "internal_filter_keep_absent",
        "filter_dispatch _Want=false keeps source keys absent from Other",
        []() -> bool
        {
            // source keys {a,b,c}, Other = DKAb -> keep those NOT in Other -> {c}
            constexpr bool ok = same_v<
                typename ::djinterp::internal::key_list_filter_dispatch<
                    kl<diff_key::alpha, diff_key::beta, diff_key::gamma>,
                    DKAb, false>::type,
                kl<diff_key::gamma> >;
            static_assert(ok, "filter absent(<a,b,c>, {a,b}) == <c>");
            return ok;
        } });

    block.tests.push_back({
        "internal_filter_empty_source",
        "filter_dispatch over an empty source is empty (base case)",
        []() -> bool
        {
            constexpr bool keep = same_v<
                typename ::djinterp::internal::key_list_filter_dispatch<
                    kl<>, DKAb, true>::type, kl<> >;
            constexpr bool drop = same_v<
                typename ::djinterp::internal::key_list_filter_dispatch<
                    kl<>, DKAb, false>::type, kl<> >;
            static_assert(keep && drop, "filter(<>, ...) == <>");
            return (keep && drop);
        } });

#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

    return block;
}


NS_END  // testing
NS_END  // djinterp
