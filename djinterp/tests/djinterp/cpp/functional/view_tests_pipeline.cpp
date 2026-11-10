/*******************************************************************************
* djinterp [test]                                        view_tests_pipeline.cpp
*
* Pipeline operator tests (section VI): operator| for view|adapter and
* container|adapter, plus multi-stage chaining.
*
*   test_pipeline_view_adapter  -- a view on the left of | feeds an adapter.
*   test_pipeline_container_lift -- a bare container is implicitly lifted to a
*                                   DRefView before the adapter applies.
*   test_pipeline_chain         -- several adapters compose left-to-right and
*                                   stay lazy until a terminal drains them.
*
*
* path:      /tests/djinterp/cpp/functional/view_tests_pipeline.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <vector>
// djinterp
#include "./view_tests.hpp"


NS_DJINTERP
NS_TESTING


bool test_pipeline_view_adapter()
{
    bool verdict = true;

    // an explicit view (DSingleView) piped into an adapter
    record_assertion(verdict,
        (djinterp::single(5) | djinterp::transform(DFnDouble{}) | to_vector())
            == std::vector<int>({ 10 }),
        "pipeline: a view feeds an adapter via operator|");

    // a source view into a chain of one adapter
    record_assertion(verdict,
        (djinterp::iota(0, 4) | djinterp::filter(fn_is_even{}) | to_vector())
            == std::vector<int>({ 0, 2 }),
        "pipeline: a source view feeds an adapter");

    return verdict;
}


bool test_pipeline_container_lift()
{
    bool verdict = true;

    std::vector<int> data = { 1, 2, 3, 4 };

    // a bare container on the left is lifted to a DRefView automatically
    record_assertion(verdict,
        (data | djinterp::transform(DFnDouble{}) | to_vector())
            == std::vector<int>({ 2, 4, 6, 8 }),
        "pipeline: a container is implicitly lifted to a ref_view");

    // a bare container piped straight into a terminal
    record_assertion(verdict, (data | count()) == 4u,
        "pipeline: a container pipes directly into a terminal");

    return verdict;
}


bool test_pipeline_chain()
{
    bool verdict = true;

    std::vector<int> data = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

    // filter evens, square them, take the first 3
    auto result = data
                | djinterp::filter(fn_is_even{})
                | djinterp::transform(DFnDouble{})
                | djinterp::take(3)
                | to_vector();
    record_assertion(verdict,
        result == std::vector<int>({ 4, 8, 12 }),
        "pipeline: filter|transform|take compose left-to-right");

    // an infinite source made finite mid-chain
    auto bounded = djinterp::iota(1)
                 | djinterp::transform(DFnDouble{})
                 | djinterp::take(4)
                 | to_vector();
    record_assertion(verdict,
        bounded == std::vector<int>({ 2, 4, 6, 8 }),
        "pipeline: an infinite source is bounded by a downstream take");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
