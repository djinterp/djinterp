/******************************************************************************
* djinterp [test]                               option_diff_tests_rt_merge.cpp
*
*   PART B / B.III - runtime merge (option_merge) over test_option_map.
* Exercises all three modes and, crucially, their two DIFFERENT count
* semantics: overwrite upserts every source entry (count == source size),
* while add_new_only / keep_existing insert only keys absent from the target
* (count == absent count) and leave existing values untouched.  Each case
* checks both the returned count and the mutated target's resulting state.
* Ungated.
*
*   Fixtures (a/b/c = alpha/beta/gamma):
*     target  a:1 b:2      source  b:99 c:3      -> b shared, c new
*
* path:      /tests/djinterp/core/option/option_diff_tests_rt_merge.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.08
******************************************************************************/

#include "./option_diff_tests.hpp"


NS_DJINTERP
NS_TESTING


using imap = test_option_map<diff_key, int>;
using dk   = diff_key;


::djinterp::test::block_spec
option_diff_rt_merge_block()
{
    ::djinterp::test::block_spec block;
    block.name       = "B.III  runtime merge";
    block.descriptor = "option_merge across overwrite / add_new_only / keep_existing";

    // ---- overwrite : upsert every source entry ---------------------------

    block.tests.push_back({
        "overwrite_count_is_source_size",
        "overwrite touches every source entry -> count == source size",
        []() -> bool
        {
            imap target; target.with(dk::alpha, 1).with(dk::beta, 2);
            imap source; source.with(dk::beta, 99).with(dk::gamma, 3);
            std::size_t n =
                ::djinterp::option_merge(target, source, merge_mode::overwrite);
            return (n == 2u);
        } });

    block.tests.push_back({
        "overwrite_upserts_values",
        "overwrite updates the shared key, appends the new one, keeps the rest",
        []() -> bool
        {
            imap target; target.with(dk::alpha, 1).with(dk::beta, 2);
            imap source; source.with(dk::beta, 99).with(dk::gamma, 3);
            ::djinterp::option_merge(target, source, merge_mode::overwrite);
            return ( target.size() == 3u                    &&
                     target.find(dk::alpha)->value == 1     &&   // untouched
                     target.find(dk::beta)->value  == 99    &&   // overwritten
                     target.find(dk::gamma)->value == 3 );       // inserted
        } });

    // ---- add_new_only : insert only absent keys --------------------------

    block.tests.push_back({
        "add_new_only_count_is_absent_count",
        "add_new_only inserts only absent keys -> count == absent count",
        []() -> bool
        {
            imap target; target.with(dk::alpha, 1).with(dk::beta, 2);
            imap source; source.with(dk::beta, 99).with(dk::gamma, 3);
            // b already present, only c is absent
            std::size_t n =
                ::djinterp::option_merge(target, source, merge_mode::add_new_only);
            return (n == 1u);
        } });

    block.tests.push_back({
        "add_new_only_keeps_existing_values",
        "add_new_only leaves the shared key's value intact, appends the new one",
        []() -> bool
        {
            imap target; target.with(dk::alpha, 1).with(dk::beta, 2);
            imap source; source.with(dk::beta, 99).with(dk::gamma, 3);
            ::djinterp::option_merge(target, source, merge_mode::add_new_only);
            return ( target.size() == 3u                    &&
                     target.find(dk::alpha)->value == 1     &&   // untouched
                     target.find(dk::beta)->value  == 2     &&   // NOT overwritten
                     target.find(dk::gamma)->value == 3 );       // inserted
        } });

    // ---- keep_existing : alias of add_new_only ---------------------------

    block.tests.push_back({
        "keep_existing_matches_add_new_only",
        "keep_existing has the same count and effect as add_new_only",
        []() -> bool
        {
            imap target; target.with(dk::alpha, 1).with(dk::beta, 2);
            imap source; source.with(dk::beta, 99).with(dk::gamma, 3);
            std::size_t n =
                ::djinterp::option_merge(target, source, merge_mode::keep_existing);
            return ( n == 1u                              &&
                     target.find(dk::beta)->value  == 2   &&     // kept
                     target.find(dk::gamma)->value == 3 );        // inserted
        } });

    // ---- default mode ----------------------------------------------------

    block.tests.push_back({
        "default_mode_is_overwrite",
        "omitting the mode behaves as overwrite",
        []() -> bool
        {
            imap target; target.with(dk::alpha, 1).with(dk::beta, 2);
            imap source; source.with(dk::beta, 99).with(dk::gamma, 3);
            std::size_t n = ::djinterp::option_merge(target, source);
            return ( n == 2u &&
                     target.find(dk::beta)->value == 99 );
        } });

    // ---- edge cases ------------------------------------------------------

    block.tests.push_back({
        "empty_source_is_noop",
        "merging an empty source changes nothing and returns 0",
        []() -> bool
        {
            imap target; target.with(dk::alpha, 1).with(dk::beta, 2);
            imap source;
            std::size_t n =
                ::djinterp::option_merge(target, source, merge_mode::overwrite);
            return ( n == 0u                             &&
                     target.size() == 2u                 &&
                     target.find(dk::alpha)->value == 1  &&
                     target.find(dk::beta)->value  == 2 );
        } });

    block.tests.push_back({
        "into_empty_target",
        "merging into an empty target inserts everything, either mode",
        []() -> bool
        {
            imap ov_target;
            imap ov_source; ov_source.with(dk::beta, 99).with(dk::gamma, 3);
            std::size_t ov_n =
                ::djinterp::option_merge(ov_target, ov_source, merge_mode::overwrite);

            imap an_target;
            imap an_source; an_source.with(dk::beta, 99).with(dk::gamma, 3);
            std::size_t an_n =
                ::djinterp::option_merge(an_target, an_source, merge_mode::add_new_only);

            return ( ov_n == 2u && ov_target.size() == 2u &&
                     an_n == 2u && an_target.size() == 2u &&
                     ov_target.find(dk::beta)->value == 99 &&
                     an_target.find(dk::gamma)->value == 3 );
        } });

    block.tests.push_back({
        "disjoint_source_inserts_all",
        "no shared keys -> every source key is inserted, count == source size",
        []() -> bool
        {
            imap ov_target; ov_target.with(dk::alpha, 1);
            imap ov_source; ov_source.with(dk::beta, 2).with(dk::gamma, 3);
            std::size_t ov_n =
                ::djinterp::option_merge(ov_target, ov_source, merge_mode::overwrite);

            imap an_target; an_target.with(dk::alpha, 1);
            imap an_source; an_source.with(dk::beta, 2).with(dk::gamma, 3);
            std::size_t an_n =
                ::djinterp::option_merge(an_target, an_source, merge_mode::add_new_only);

            // no conflicts, so both modes agree: 2 inserted, target grows to 3
            return ( ov_n == 2u && ov_target.size() == 3u &&
                     an_n == 2u && an_target.size() == 3u );
        } });

    block.tests.push_back({
        "all_existing_source_diverges_by_mode",
        "when every source key exists: overwrite reassigns all, add_new_only is a noop",
        []() -> bool
        {
            imap ov_target; ov_target.with(dk::alpha, 1).with(dk::beta, 2);
            imap ov_source; ov_source.with(dk::alpha, 9).with(dk::beta, 8);
            std::size_t ov_n =
                ::djinterp::option_merge(ov_target, ov_source, merge_mode::overwrite);

            imap an_target; an_target.with(dk::alpha, 1).with(dk::beta, 2);
            imap an_source; an_source.with(dk::alpha, 9).with(dk::beta, 8);
            std::size_t an_n =
                ::djinterp::option_merge(an_target, an_source, merge_mode::add_new_only);

            return ( ov_n == 2u                           &&   // both reassigned
                     ov_target.find(dk::alpha)->value == 9 &&
                     ov_target.find(dk::beta)->value  == 8 &&
                     an_n == 0u                           &&   // nothing inserted
                     an_target.find(dk::alpha)->value == 1 &&  // originals kept
                     an_target.find(dk::beta)->value  == 2 );
        } });

    return block;
}


NS_END  // testing
NS_END  // djinterp
