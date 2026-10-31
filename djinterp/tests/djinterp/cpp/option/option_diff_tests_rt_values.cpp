/*******************************************************************************
* djinterp [test]                                option_diff_tests_rt_values.cpp
*
*   PART B / B.II - runtime value-level diff (option_changed_keys,
* option_unchanged_keys, option_diff_count, option_sets_equal) over
* DTestOptionMap.  These compare mapped values, so besides the int-valued
* cases there is an rgb value type (a user struct with operator==) proving the
* only requirement on mapped_type is equality-comparability.  Ungated.
*
*   Fixtures (a/b/c/d/e = alpha..epsilon):
*     base  a:1 b:2 c:3      derived  a:1 b:20 d:4
*       -> common {a,b}; a unchanged, b changed; c removed; d added
*
*
* path:      /tests/djinterp/cpp/option/option_diff_tests_rt_values.cpp
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

// rgb
//   struct: a user value type with operator==, standing in for an arbitrary
// equality-comparable mapped_type.
struct rgb
{
    int r;
    int g;
    int b;
};

inline
bool operator==(
    const rgb& _lhs,
    const rgb& _rhs
)
{
    return ( (_lhs.r == _rhs.r) &&
             (_lhs.g == _rhs.g) &&
             (_lhs.b == _rhs.b) );
}

using DCmap = DTestOptionMap<diff_key, rgb>;


::djinterp::test::block_spec option_diff_rt_values_block()
{
    ::djinterp::test::block_spec block;
    block.name       = "B.II  runtime value-level diff";
    block.descriptor = "changed / unchanged / diff_count / sets_equal";

    // ---- option_changed_keys / option_unchanged_keys ---------------------

    block.tests.push_back({
        "changed_basic",
        "common keys whose value differs -> {beta}",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3);
            DImap deriv; deriv.with(DDk::alpha, 1).with(DDk::beta, 20).with(DDk::delta, 4);
            return ( ::djinterp::option_changed_keys(base, deriv) ==
                     std::vector<DDk>{ DDk::beta } );
        } });

    block.tests.push_back({
        "unchanged_basic",
        "common keys whose value matches -> {alpha}",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3);
            DImap deriv; deriv.with(DDk::alpha, 1).with(DDk::beta, 20).with(DDk::delta, 4);
            return ( ::djinterp::option_unchanged_keys(base, deriv) ==
                     std::vector<DDk>{ DDk::alpha } );
        } });

    block.tests.push_back({
        "changed_multi_base_order",
        "several changed values reported in base order -> {beta,gamma}",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3).with(DDk::delta, 4);
            DImap deriv; deriv.with(DDk::alpha, 1).with(DDk::beta, 20).with(DDk::gamma, 30).with(DDk::epsilon, 5);
            return ( ::djinterp::option_changed_keys(base, deriv) ==
                     std::vector<DDk>{ DDk::beta, DDk::gamma } );
        } });

    block.tests.push_back({
        "unchanged_multi",
        "the single matching common value -> {alpha}",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3).with(DDk::delta, 4);
            DImap deriv; deriv.with(DDk::alpha, 1).with(DDk::beta, 20).with(DDk::gamma, 30).with(DDk::epsilon, 5);
            return ( ::djinterp::option_unchanged_keys(base, deriv) ==
                     std::vector<DDk>{ DDk::alpha } );
        } });

    block.tests.push_back({
        "changed_identical_empty",
        "identical maps have no changed keys",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3);
            DImap copy;  copy.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3);
            return ::djinterp::option_changed_keys(base, copy).empty();
        } });

    block.tests.push_back({
        "unchanged_identical_all",
        "identical maps: every common key is unchanged",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3);
            DImap copy;  copy.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3);
            return ( ::djinterp::option_unchanged_keys(base, copy) ==
                     std::vector<DDk>{ DDk::alpha, DDk::beta, DDk::gamma } );
        } });

    block.tests.push_back({
        "changed_unchanged_ignore_key_only",
        "disjoint maps have no common keys -> both empty",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1);
            DImap deriv; deriv.with(DDk::beta, 1);
            return ( ::djinterp::option_changed_keys(base, deriv).empty() &&
                     ::djinterp::option_unchanged_keys(base, deriv).empty() );
        } });

    // ---- option_diff_count -----------------------------------------------

    block.tests.push_back({
        "diff_count_basic",
        "changed(1) + added(1) + removed(1) == 3",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3);
            DImap deriv; deriv.with(DDk::alpha, 1).with(DDk::beta, 20).with(DDk::delta, 4);
            return (::djinterp::option_diff_count(base, deriv) == 3u);
        } });

    block.tests.push_back({
        "diff_count_multi",
        "changed(2) + added(1) + removed(1) == 4",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3).with(DDk::delta, 4);
            DImap deriv; deriv.with(DDk::alpha, 1).with(DDk::beta, 20).with(DDk::gamma, 30).with(DDk::epsilon, 5);
            return (::djinterp::option_diff_count(base, deriv) == 4u);
        } });

    block.tests.push_back({
        "diff_count_identical_zero",
        "identical maps diff to zero",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3);
            DImap copy;  copy.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3);
            return (::djinterp::option_diff_count(base, copy) == 0u);
        } });

    block.tests.push_back({
        "diff_count_key_only",
        "disjoint maps: 0 changed + 1 added + 1 removed == 2",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1);
            DImap deriv; deriv.with(DDk::beta, 1);
            return (::djinterp::option_diff_count(base, deriv) == 2u);
        } });

    block.tests.push_back({
        "diff_count_to_empty",
        "all keys removed against an empty map -> 3",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3);
            DImap empty;
            return (::djinterp::option_diff_count(base, empty) == 3u);
        } });

    block.tests.push_back({
        "diff_count_empty_empty",
        "empty vs empty diffs to zero",
        []() -> bool
        {
            DImap a;
            DImap b;
            return (::djinterp::option_diff_count(a, b) == 0u);
        } });

    // ---- option_sets_equal -----------------------------------------------

    block.tests.push_back({
        "sets_equal_true",
        "sets_equal is true exactly when the diff is empty",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3);
            DImap copy;  copy.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3);
            return ::djinterp::option_sets_equal(base, copy);
        } });

    block.tests.push_back({
        "sets_equal_false",
        "any difference makes sets_equal false",
        []() -> bool
        {
            DImap base;  base.with(DDk::alpha, 1).with(DDk::beta, 2).with(DDk::gamma, 3);
            DImap deriv; deriv.with(DDk::alpha, 1).with(DDk::beta, 20).with(DDk::delta, 4);
            return !::djinterp::option_sets_equal(base, deriv);
        } });

    block.tests.push_back({
        "sets_equal_empty",
        "two empty maps are equal",
        []() -> bool
        {
            DImap a;
            DImap b;
            return ::djinterp::option_sets_equal(a, b);
        } });

    // ---- arbitrary equality-comparable mapped_type (rgb) -----------------

    block.tests.push_back({
        "custom_value_changed_unchanged",
        "value diffs of a user struct are detected via its operator==",
        []() -> bool
        {
            DCmap base;
            base.with(DDk::alpha, rgb{ 1, 2, 3 }).with(DDk::beta, rgb{ 0, 0, 0 });
            DCmap deriv;
            deriv.with(DDk::alpha, rgb{ 1, 2, 3 }).with(DDk::beta, rgb{ 9, 9, 9 });
            return ( ::djinterp::option_changed_keys(base, deriv)   == std::vector<DDk>{ DDk::beta } &&
                     ::djinterp::option_unchanged_keys(base, deriv) == std::vector<DDk>{ DDk::alpha } );
        } });

    block.tests.push_back({
        "custom_value_count_and_equal",
        "diff_count / sets_equal work over the rgb mapped_type",
        []() -> bool
        {
            DCmap base;
            base.with(DDk::alpha, rgb{ 1, 1, 1 }).with(DDk::beta, rgb{ 2, 2, 2 });
            DCmap copy;
            copy.with(DDk::alpha, rgb{ 1, 1, 1 }).with(DDk::beta, rgb{ 2, 2, 2 });
            DCmap diff;
            diff.with(DDk::alpha, rgb{ 1, 1, 1 }).with(DDk::beta, rgb{ 5, 5, 5 });
            return ( ::djinterp::option_sets_equal(base, copy)      &&
                     ::djinterp::option_diff_count(base, copy) == 0u &&
                     ::djinterp::option_diff_count(base, diff) == 1u &&
                     !::djinterp::option_sets_equal(base, diff) );
        } });

    return block;
}


NS_END  // testing
NS_END  // djinterp
