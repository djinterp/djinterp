/******************************************************************************
* djinterp [test]                              option_diff_tests_rt_values.cpp
*
*   PART B / B.II - runtime value-level diff (option_changed_keys,
* option_unchanged_keys, option_diff_count, option_sets_equal) over
* test_option_map.  These compare mapped values, so besides the int-valued
* cases there is an rgb value type (a user struct with operator==) proving the
* only requirement on mapped_type is equality-comparability.  Ungated.
*
*   Fixtures (a/b/c/d/e = alpha..epsilon):
*     base  a:1 b:2 c:3      derived  a:1 b:20 d:4
*       -> common {a,b}; a unchanged, b changed; c removed; d added
*
* path:      /tests/djinterp/core/option/option_diff_tests_rt_values.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.08
******************************************************************************/

#include "./option_diff_tests.hpp"


NS_DJINTERP
NS_TESTING


using imap = test_option_map<diff_key, int>;
using dk   = diff_key;

// rgb
//   struct: a user value type with operator==, standing in for an arbitrary
// equality-comparable mapped_type.
struct rgb
{
    int r;
    int g;
    int b;
};

inline bool
operator==(const rgb& _lhs, const rgb& _rhs)
{
    return ( (_lhs.r == _rhs.r) &&
             (_lhs.g == _rhs.g) &&
             (_lhs.b == _rhs.b) );
}

using cmap = test_option_map<diff_key, rgb>;


::djinterp::test::block_spec
option_diff_rt_values_block()
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
            imap base;  base.with(dk::alpha, 1).with(dk::beta, 2).with(dk::gamma, 3);
            imap deriv; deriv.with(dk::alpha, 1).with(dk::beta, 20).with(dk::delta, 4);
            return ( ::djinterp::option_changed_keys(base, deriv) ==
                     std::vector<dk>{ dk::beta } );
        } });

    block.tests.push_back({
        "unchanged_basic",
        "common keys whose value matches -> {alpha}",
        []() -> bool
        {
            imap base;  base.with(dk::alpha, 1).with(dk::beta, 2).with(dk::gamma, 3);
            imap deriv; deriv.with(dk::alpha, 1).with(dk::beta, 20).with(dk::delta, 4);
            return ( ::djinterp::option_unchanged_keys(base, deriv) ==
                     std::vector<dk>{ dk::alpha } );
        } });

    block.tests.push_back({
        "changed_multi_base_order",
        "several changed values reported in base order -> {beta,gamma}",
        []() -> bool
        {
            imap base;  base.with(dk::alpha, 1).with(dk::beta, 2).with(dk::gamma, 3).with(dk::delta, 4);
            imap deriv; deriv.with(dk::alpha, 1).with(dk::beta, 20).with(dk::gamma, 30).with(dk::epsilon, 5);
            return ( ::djinterp::option_changed_keys(base, deriv) ==
                     std::vector<dk>{ dk::beta, dk::gamma } );
        } });

    block.tests.push_back({
        "unchanged_multi",
        "the single matching common value -> {alpha}",
        []() -> bool
        {
            imap base;  base.with(dk::alpha, 1).with(dk::beta, 2).with(dk::gamma, 3).with(dk::delta, 4);
            imap deriv; deriv.with(dk::alpha, 1).with(dk::beta, 20).with(dk::gamma, 30).with(dk::epsilon, 5);
            return ( ::djinterp::option_unchanged_keys(base, deriv) ==
                     std::vector<dk>{ dk::alpha } );
        } });

    block.tests.push_back({
        "changed_identical_empty",
        "identical maps have no changed keys",
        []() -> bool
        {
            imap base;  base.with(dk::alpha, 1).with(dk::beta, 2).with(dk::gamma, 3);
            imap copy;  copy.with(dk::alpha, 1).with(dk::beta, 2).with(dk::gamma, 3);
            return ::djinterp::option_changed_keys(base, copy).empty();
        } });

    block.tests.push_back({
        "unchanged_identical_all",
        "identical maps: every common key is unchanged",
        []() -> bool
        {
            imap base;  base.with(dk::alpha, 1).with(dk::beta, 2).with(dk::gamma, 3);
            imap copy;  copy.with(dk::alpha, 1).with(dk::beta, 2).with(dk::gamma, 3);
            return ( ::djinterp::option_unchanged_keys(base, copy) ==
                     std::vector<dk>{ dk::alpha, dk::beta, dk::gamma } );
        } });

    block.tests.push_back({
        "changed_unchanged_ignore_key_only",
        "disjoint maps have no common keys -> both empty",
        []() -> bool
        {
            imap base;  base.with(dk::alpha, 1);
            imap deriv; deriv.with(dk::beta, 1);
            return ( ::djinterp::option_changed_keys(base, deriv).empty() &&
                     ::djinterp::option_unchanged_keys(base, deriv).empty() );
        } });

    // ---- option_diff_count -----------------------------------------------

    block.tests.push_back({
        "diff_count_basic",
        "changed(1) + added(1) + removed(1) == 3",
        []() -> bool
        {
            imap base;  base.with(dk::alpha, 1).with(dk::beta, 2).with(dk::gamma, 3);
            imap deriv; deriv.with(dk::alpha, 1).with(dk::beta, 20).with(dk::delta, 4);
            return (::djinterp::option_diff_count(base, deriv) == 3u);
        } });

    block.tests.push_back({
        "diff_count_multi",
        "changed(2) + added(1) + removed(1) == 4",
        []() -> bool
        {
            imap base;  base.with(dk::alpha, 1).with(dk::beta, 2).with(dk::gamma, 3).with(dk::delta, 4);
            imap deriv; deriv.with(dk::alpha, 1).with(dk::beta, 20).with(dk::gamma, 30).with(dk::epsilon, 5);
            return (::djinterp::option_diff_count(base, deriv) == 4u);
        } });

    block.tests.push_back({
        "diff_count_identical_zero",
        "identical maps diff to zero",
        []() -> bool
        {
            imap base;  base.with(dk::alpha, 1).with(dk::beta, 2).with(dk::gamma, 3);
            imap copy;  copy.with(dk::alpha, 1).with(dk::beta, 2).with(dk::gamma, 3);
            return (::djinterp::option_diff_count(base, copy) == 0u);
        } });

    block.tests.push_back({
        "diff_count_key_only",
        "disjoint maps: 0 changed + 1 added + 1 removed == 2",
        []() -> bool
        {
            imap base;  base.with(dk::alpha, 1);
            imap deriv; deriv.with(dk::beta, 1);
            return (::djinterp::option_diff_count(base, deriv) == 2u);
        } });

    block.tests.push_back({
        "diff_count_to_empty",
        "all keys removed against an empty map -> 3",
        []() -> bool
        {
            imap base;  base.with(dk::alpha, 1).with(dk::beta, 2).with(dk::gamma, 3);
            imap empty;
            return (::djinterp::option_diff_count(base, empty) == 3u);
        } });

    block.tests.push_back({
        "diff_count_empty_empty",
        "empty vs empty diffs to zero",
        []() -> bool
        {
            imap a;
            imap b;
            return (::djinterp::option_diff_count(a, b) == 0u);
        } });

    // ---- option_sets_equal -----------------------------------------------

    block.tests.push_back({
        "sets_equal_true",
        "sets_equal is true exactly when the diff is empty",
        []() -> bool
        {
            imap base;  base.with(dk::alpha, 1).with(dk::beta, 2).with(dk::gamma, 3);
            imap copy;  copy.with(dk::alpha, 1).with(dk::beta, 2).with(dk::gamma, 3);
            return ::djinterp::option_sets_equal(base, copy);
        } });

    block.tests.push_back({
        "sets_equal_false",
        "any difference makes sets_equal false",
        []() -> bool
        {
            imap base;  base.with(dk::alpha, 1).with(dk::beta, 2).with(dk::gamma, 3);
            imap deriv; deriv.with(dk::alpha, 1).with(dk::beta, 20).with(dk::delta, 4);
            return !::djinterp::option_sets_equal(base, deriv);
        } });

    block.tests.push_back({
        "sets_equal_empty",
        "two empty maps are equal",
        []() -> bool
        {
            imap a;
            imap b;
            return ::djinterp::option_sets_equal(a, b);
        } });

    // ---- arbitrary equality-comparable mapped_type (rgb) -----------------

    block.tests.push_back({
        "custom_value_changed_unchanged",
        "value diffs of a user struct are detected via its operator==",
        []() -> bool
        {
            cmap base;
            base.with(dk::alpha, rgb{ 1, 2, 3 }).with(dk::beta, rgb{ 0, 0, 0 });
            cmap deriv;
            deriv.with(dk::alpha, rgb{ 1, 2, 3 }).with(dk::beta, rgb{ 9, 9, 9 });
            return ( ::djinterp::option_changed_keys(base, deriv)   == std::vector<dk>{ dk::beta } &&
                     ::djinterp::option_unchanged_keys(base, deriv) == std::vector<dk>{ dk::alpha } );
        } });

    block.tests.push_back({
        "custom_value_count_and_equal",
        "diff_count / sets_equal work over the rgb mapped_type",
        []() -> bool
        {
            cmap base;
            base.with(dk::alpha, rgb{ 1, 1, 1 }).with(dk::beta, rgb{ 2, 2, 2 });
            cmap copy;
            copy.with(dk::alpha, rgb{ 1, 1, 1 }).with(dk::beta, rgb{ 2, 2, 2 });
            cmap diff;
            diff.with(dk::alpha, rgb{ 1, 1, 1 }).with(dk::beta, rgb{ 5, 5, 5 });
            return ( ::djinterp::option_sets_equal(base, copy)      &&
                     ::djinterp::option_diff_count(base, copy) == 0u &&
                     ::djinterp::option_diff_count(base, diff) == 1u &&
                     !::djinterp::option_sets_equal(base, diff) );
        } });

    return block;
}


NS_END  // testing
NS_END  // djinterp
