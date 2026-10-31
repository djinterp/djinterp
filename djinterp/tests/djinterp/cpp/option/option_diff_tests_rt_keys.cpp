/*******************************************************************************
* djinterp [test]                                  option_diff_tests_rt_keys.cpp
*
*   PART B / B.I - runtime key-level diff (option_keys_in, option_added_keys,
* option_removed_keys, option_common_keys) exercised over DTestOptionMap, the
* stand-in for the duck-typed option_set_map surface.  Returned key vectors
* preserve source order, so each case asserts on an exact std::vector<key_type>.
* Standard-agnostic: ungated.
*
*   Fixtures (a/b/c/d = alpha..delta):
*     base  a:1 b:2 c:3      derived  a:1 b:20 d:4
*
*
* path:      /tests/djinterp/cpp/option/option_diff_tests_rt_keys.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "./option_diff_tests.hpp"


NS_DJINTERP
NS_TESTING


using DImap = DTestOptionMap<diff_key, int>;
using DDk   = diff_key;


::djinterp::test::block_spec option_diff_rt_keys_block()
{
    ::djinterp::test::block_spec block;
    block.name       = "B.I  runtime key-level diff";
    block.descriptor = "keys_in / added / removed / common over the map surface";

    // ---- option_keys_in : full key list, insertion order -----------------

    block.tests.push_back({
        "keys_in_preserves_order",
        "keys_in returns every key in insertion order",
        []() -> bool
        {
            DImap base;
            base.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3);
            return ( ::djinterp::option_keys_in(base) ==
                     std::vector<DDk>{ DDk::alpha, DDk::beta, DDk::gamma } );
        } });

    block.tests.push_back({
        "keys_in_empty",
        "keys_in of an empty map is empty",
        []() -> bool
        {
            DImap empty;
            return ::djinterp::option_keys_in(empty).empty();
        } });

    // ---- option_added_keys : derived-minus-base, DERIVED order -----------

    block.tests.push_back({
        "added_basic",
        "keys in derived but not base -> the derived-only key",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3);
            DImap deriv; deriv.with(DDk::alpha, 1).with(DDk::beta, 20).with(DDk::delta, 4);
            return ( ::djinterp::option_added_keys(base, deriv) ==
                     std::vector<DDk>{ DDk::delta } );
        } });

    block.tests.push_back({
        "added_multi_derived_order",
        "multiple additions keep derived order",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1);
            DImap deriv; deriv.with(DDk::beta, 1).with(DDk::gamma, 1).with(DDk::delta, 1);
            return ( ::djinterp::option_added_keys(base, deriv) ==
                     std::vector<DDk>{ DDk::beta, DDk::gamma, DDk::delta } );
        } });

    block.tests.push_back({
        "added_none_when_subset",
        "derived subset of base -> nothing added",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3);
            DImap deriv; deriv.with(DDk::alpha, 1).with(DDk::beta, 2);
            return ::djinterp::option_added_keys(base, deriv).empty();
        } });

    block.tests.push_back({
        "added_empty_base",
        "empty base -> all derived keys added",
        []() -> bool
        {
            DImap base;
            DImap deriv; deriv.with(DDk::alpha, 1).with(DDk::beta, 2);
            return ( ::djinterp::option_added_keys(base, deriv) ==
                     std::vector<DDk>{ DDk::alpha, DDk::beta } );
        } });

    block.tests.push_back({
        "added_empty_derived",
        "empty derived -> nothing added",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1).with(DDk::beta, 2);
            DImap deriv;
            return ::djinterp::option_added_keys(base, deriv).empty();
        } });

    // ---- option_removed_keys : base-minus-derived, BASE order ------------

    block.tests.push_back({
        "removed_basic",
        "keys in base but not derived -> the base-only key",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3);
            DImap deriv; deriv.with(DDk::alpha, 1).with(DDk::beta, 20).with(DDk::delta, 4);
            return ( ::djinterp::option_removed_keys(base, deriv) ==
                     std::vector<DDk>{ DDk::gamma } );
        } });

    block.tests.push_back({
        "removed_all_base_order",
        "empty derived -> all base keys removed, base order",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3);
            DImap deriv;
            return ( ::djinterp::option_removed_keys(base, deriv) ==
                     std::vector<DDk>{ DDk::alpha, DDk::beta, DDk::gamma } );
        } });

    block.tests.push_back({
        "removed_none_when_superset",
        "base subset of derived -> nothing removed",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1).with(DDk::beta, 2);
            DImap deriv; deriv.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3);
            return ::djinterp::option_removed_keys(base, deriv).empty();
        } });

    block.tests.push_back({
        "removed_empty_base",
        "empty base -> nothing to remove",
        []() -> bool
        {
            DImap base;
            DImap deriv; deriv.with(DDk::alpha, 1);
            return ::djinterp::option_removed_keys(base, deriv).empty();
        } });

    // ---- option_common_keys : intersection, FIRST-arg order --------------

    block.tests.push_back({
        "common_basic",
        "keys present in both, first-arg order",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3);
            DImap deriv; deriv.with(DDk::alpha, 1).with(DDk::beta, 20).with(DDk::delta, 4);
            return ( ::djinterp::option_common_keys(base, deriv) ==
                     std::vector<DDk>{ DDk::alpha, DDk::beta } );
        } });

    block.tests.push_back({
        "common_identical",
        "identical key sets -> the whole set",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3);
            DImap deriv; deriv.with(DDk::alpha, 9).with(DDk::beta, 9).with(DDk::gamma, 9);
            return ( ::djinterp::option_common_keys(base, deriv) ==
                     std::vector<DDk>{ DDk::alpha, DDk::beta, DDk::gamma } );
        } });

    block.tests.push_back({
        "common_disjoint",
        "no shared keys -> empty",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1);
            DImap deriv; deriv.with(DDk::beta, 1);
            return ::djinterp::option_common_keys(base, deriv).empty();
        } });

    block.tests.push_back({
        "common_empty",
        "intersection with an empty map is empty",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1).with(DDk::beta, 2);
            DImap empty;
            return ( ::djinterp::option_common_keys(base, empty).empty() &&
                     ::djinterp::option_common_keys(empty, base).empty() );
        } });

    block.tests.push_back({
        "common_order_follows_first",
        "intersection order tracks the first operand",
        []() -> bool
        {
            DImap base;  base.with(DDk::gamma, 1).with(DDk::alpha, 1).with(DDk::beta, 1);
            DImap deriv; deriv.with(DDk::alpha, 1).with(DDk::beta, 1).with(DDk::gamma, 1);
            return ( ::djinterp::option_common_keys(base, deriv) ==
                     std::vector<DDk>{ DDk::gamma, DDk::alpha, DDk::beta } );
        } });

    // ---- the three orderings, side by side -------------------------------

    block.tests.push_back({
        "orderings_are_independent",
        "added=derived order, removed=base order, common=base order",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1).with(DDk::beta, 2);
            DImap deriv; deriv.with(DDk::gamma, 3).with(DDk::beta, 9);
            return ( ::djinterp::option_added_keys(base, deriv)   == std::vector<DDk>{ DDk::gamma } &&
                     ::djinterp::option_removed_keys(base, deriv) == std::vector<DDk>{ DDk::alpha } &&
                     ::djinterp::option_common_keys(base, deriv)  == std::vector<DDk>{ DDk::beta } );
        } });

    return block;
}


NS_END  // testing
NS_END  // djinterp
