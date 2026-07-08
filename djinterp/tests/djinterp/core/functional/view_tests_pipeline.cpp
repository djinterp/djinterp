/******************************************************************************
* djinterp [test]                                      view_tests_pipeline.cpp
*
* Pipeline operator tests (section VI): operator| for view|adapter and
* container|adapter, plus multi-stage chaining.
*
*   test_pipeline_view_adapter  -- a view on the left of | feeds an adapter.
*   test_pipeline_container_lift -- a bare container is implicitly lifted to a
*                                   ref_view before the adapter applies.
*   test_pipeline_chain         -- several adapters compose left-to-right and
*                                   stay lazy until a terminal drains them.
******************************************************************************/

#include <vector>
#include "./view_tests.hpp"


NS_DJINTERP
NS_TESTING


void test_pipeline_view_adapter(test::test_handler& _h)
{
    // an explicit view (single_view) piped into an adapter
    test::record_assertion(_h,
        (views::single(5) | views::transform(fn_double{}) | to_vector())
            == std::vector<int>({ 10 }),
        "pipeline: a view feeds an adapter via operator|");

    // a source view into a chain of one adapter
    test::record_assertion(_h,
        (views::iota(0, 4) | views::filter(fn_is_even{}) | to_vector())
            == std::vector<int>({ 0, 2 }),
        "pipeline: a source view feeds an adapter");

    return;
}


void test_pipeline_container_lift(test::test_handler& _h)
{
    std::vector<int> data = { 1, 2, 3, 4 };

    // a bare container on the left is lifted to a ref_view automatically
    test::record_assertion(_h,
        (data | views::transform(fn_double{}) | to_vector())
            == std::vector<int>({ 2, 4, 6, 8 }),
        "pipeline: a container is implicitly lifted to a ref_view");

    // a bare container piped straight into a terminal
    test::record_assertion(_h, (data | count()) == 4u,
        "pipeline: a container pipes directly into a terminal");

    return;
}


void test_pipeline_chain(test::test_handler& _h)
{
    std::vector<int> data = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

    // filter evens, square them, take the first 3
    auto result = data
                | views::filter(fn_is_even{})
                | views::transform(fn_double{})
                | views::take(3)
                | to_vector();
    test::record_assertion(_h,
        result == std::vector<int>({ 4, 8, 12 }),
        "pipeline: filter|transform|take compose left-to-right");

    // an infinite source made finite mid-chain
    auto bounded = views::iota(1)
                 | views::transform(fn_double{})
                 | views::take(4)
                 | to_vector();
    test::record_assertion(_h,
        bounded == std::vector<int>({ 2, 4, 6, 8 }),
        "pipeline: an infinite source is bounded by a downstream take");

    return;
}


NS_END  // testing
NS_END  // djinterp
