/*******************************************************************************
* djinterp [test]                                 option_diff_tests_rt_merge.cpp
*
*   PART B / B.III - runtime merge (option_merge) over DTestOptionMap.
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
*
* path:      /tests/djinterp/cpp/option/option_diff_tests_rt_merge.cpp
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


::djinterp::test::block_spec option_diff_rt_merge_block()
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
            DImap target; target.with(DDk::alpha, 1).with(DDk::beta, 2);
            DImap source; source.with(DDk::beta, 99).with(DDk::gamma, 3);
            std::size_t n =
                ::djinterp::option_merge(target, source, DMergeMode::overwrite);
            return (n == 2u);
        } });

    block.tests.push_back({
        "overwrite_upserts_values",
        "overwrite updates the shared key, appends the new one, keeps the rest",
        []() -> bool
        {
            DImap target; target.with(DDk::alpha, 1).with(DDk::beta, 2);
            DImap source; source.with(DDk::beta, 99).with(DDk::gamma, 3);
            ::djinterp::option_merge(target, source, DMergeMode::overwrite);
            return ( target.size() == 3u                    &&
                     target.find(DDk::alpha)->value == 1     &&   // untouched
                     target.find(DDk::beta)->value  == 99    &&   // overwritten
                     target.find(DDk::gamma)->value == 3 );       // inserted
        } });

    // ---- add_new_only : insert only absent keys --------------------------

    block.tests.push_back({
        "add_new_only_count_is_absent_count",
        "add_new_only inserts only absent keys -> count == absent count",
        []() -> bool
        {
            DImap target; target.with(DDk::alpha, 1).with(DDk::beta, 2);
            DImap source; source.with(DDk::beta, 99).with(DDk::gamma, 3);
            // b already present, only c is absent
            std::size_t n =
                ::djinterp::option_merge(target, source, DMergeMode::add_new_only);
            return (n == 1u);
        } });

    block.tests.push_back({
        "add_new_only_keeps_existing_values",
        "add_new_only leaves the shared key's value intact, appends the new one",
        []() -> bool
        {
            DImap target; target.with(DDk::alpha, 1).with(DDk::beta, 2);
            DImap source; source.with(DDk::beta, 99).with(DDk::gamma, 3);
            ::djinterp::option_merge(target, source, DMergeMode::add_new_only);
            return ( target.size() == 3u                    &&
                     target.find(DDk::alpha)->value == 1     &&   // untouched
                     target.find(DDk::beta)->value  == 2     &&   // NOT overwritten
                     target.find(DDk::gamma)->value == 3 );       // inserted
        } });

    // ---- keep_existing : alias of add_new_only ---------------------------

    block.tests.push_back({
        "keep_existing_matches_add_new_only",
        "keep_existing has the same count and effect as add_new_only",
        []() -> bool
        {
            DImap target; target.with(DDk::alpha, 1).with(DDk::beta, 2);
            DImap source; source.with(DDk::beta, 99).with(DDk::gamma, 3);
            std::size_t n =
                ::djinterp::option_merge(target, source, DMergeMode::keep_existing);
            return ( n == 1u                              &&
                     target.find(DDk::beta)->value  == 2   &&     // kept
                     target.find(DDk::gamma)->value == 3 );        // inserted
        } });

    // ---- default mode ----------------------------------------------------

    block.tests.push_back({
        "default_mode_is_overwrite",
        "omitting the mode behaves as overwrite",
        []() -> bool
        {
            DImap target; target.with(DDk::alpha, 1).with(DDk::beta, 2);
            DImap source; source.with(DDk::beta, 99).with(DDk::gamma, 3);
            std::size_t n = ::djinterp::option_merge(target, source);
            return ( n == 2u &&
                     target.find(DDk::beta)->value == 99 );
        } });

    // ---- edge cases ------------------------------------------------------

    block.tests.push_back({
        "empty_source_is_noop",
        "merging an empty source changes nothing and returns 0",
        []() -> bool
        {
            DImap target; target.with(DDk::alpha, 1).with(DDk::beta, 2);
            DImap source;
            std::size_t n =
                ::djinterp::option_merge(target, source, DMergeMode::overwrite);
            return ( n == 0u                             &&
                     target.size() == 2u                 &&
                     target.find(DDk::alpha)->value == 1  &&
                     target.find(DDk::beta)->value  == 2 );
        } });

    block.tests.push_back({
        "into_empty_target",
        "merging into an empty target inserts everything, either mode",
        []() -> bool
        {
            DImap ov_target;
            DImap ov_source; ov_source.with(DDk::beta, 99).with(DDk::gamma, 3);
            std::size_t ov_n =
                ::djinterp::option_merge(ov_target, ov_source, DMergeMode::overwrite);

            DImap an_target;
            DImap an_source; an_source.with(DDk::beta, 99).with(DDk::gamma, 3);
            std::size_t an_n =
                ::djinterp::option_merge(an_target, an_source, DMergeMode::add_new_only);

            return ( ov_n == 2u && ov_target.size() == 2u &&
                     an_n == 2u && an_target.size() == 2u &&
                     ov_target.find(DDk::beta)->value == 99 &&
                     an_target.find(DDk::gamma)->value == 3 );
        } });

    block.tests.push_back({
        "disjoint_source_inserts_all",
        "no shared keys -> every source key is inserted, count == source size",
        []() -> bool
        {
            DImap ov_target; ov_target.with(DDk::alpha, 1);
            DImap ov_source; ov_source.with(DDk::beta, 2).with(DDk::gamma, 3);
            std::size_t ov_n =
                ::djinterp::option_merge(ov_target, ov_source, DMergeMode::overwrite);

            DImap an_target; an_target.with(DDk::alpha, 1);
            DImap an_source; an_source.with(DDk::beta, 2).with(DDk::gamma, 3);
            std::size_t an_n =
                ::djinterp::option_merge(an_target, an_source, DMergeMode::add_new_only);

            // no conflicts, so both modes agree: 2 inserted, target grows to 3
            return ( ov_n == 2u && ov_target.size() == 3u &&
                     an_n == 2u && an_target.size() == 3u );
        } });

    block.tests.push_back({
        "all_existing_source_diverges_by_mode",
        "when every source key exists: overwrite reassigns all, add_new_only is a noop",
        []() -> bool
        {
            DImap ov_target; ov_target.with(DDk::alpha, 1).with(DDk::beta, 2);
            DImap ov_source; ov_source.with(DDk::alpha, 9).with(DDk::beta, 8);
            std::size_t ov_n =
                ::djinterp::option_merge(ov_target, ov_source, DMergeMode::overwrite);

            DImap an_target; an_target.with(DDk::alpha, 1).with(DDk::beta, 2);
            DImap an_source; an_source.with(DDk::alpha, 9).with(DDk::beta, 8);
            std::size_t an_n =
                ::djinterp::option_merge(an_target, an_source, DMergeMode::add_new_only);

            return ( ov_n == 2u                           &&   // both reassigned
                     ov_target.find(DDk::alpha)->value == 9 &&
                     ov_target.find(DDk::beta)->value  == 8 &&
                     an_n == 0u                           &&   // nothing inserted
                     an_target.find(DDk::alpha)->value == 1 &&  // originals kept
                     an_target.find(DDk::beta)->value  == 2 );
        } });

    return block;
}


NS_END  // testing
NS_END  // djinterp
