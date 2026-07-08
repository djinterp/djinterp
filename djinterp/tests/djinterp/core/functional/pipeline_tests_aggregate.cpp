/******************************************************************************
* djinterp [test]                                 pipeline_tests_aggregate.cpp
*
* Aggregating-operation tests (section ii / iii):
*   test_fold      -- left fold with an explicit init (incl. empty).
*   test_reduce    -- reduce over a non-empty pipeline.
*   test_group_by  -- group elements into a map keyed by a function.
*   test_partition -- split into (passing, failing) pipelines.
*   test_zip_with  -- elementwise binary combine of two pipelines.
******************************************************************************/

#include <vector>
#include <map>
#include <utility>
#include "./pipeline_tests.hpp"


NS_DJINTERP
NS_TESTING


using pi = function_pipeline<int>;


void test_fold(test::test_handler& _h)
{
    pi p = pi::from({ 1, 2, 3, 4 });

    // sum from 0
    test::record_assertion(_h, p.fold(0, fn_add{}) == 10,
        "fold: accumulates from the initial value");

    // init contributes
    test::record_assertion(_h, p.fold(100, fn_add{}) == 110,
        "fold: the initial value participates");

    // empty pipeline -> init unchanged
    test::record_assertion(_h,
        pi::from(std::vector<int>()).fold(7, fn_add{}) == 7,
        "fold: empty pipeline returns the initial value");

    return;
}


void test_reduce(test::test_handler& _h)
{
    // reduce requires a non-empty pipeline
    pi p = pi::from({ 1, 2, 3, 4 });
    test::record_assertion(_h, p.reduce(fn_add{}) == 10,
        "reduce: folds using the first element as the seed");

    // single element reduces to itself
    pi one = pi::from({ 42 });
    test::record_assertion(_h, one.reduce(fn_add{}) == 42,
        "reduce: a single element reduces to itself");

    return;
}


void test_group_by(test::test_handler& _h)
{
    pi p = pi::from({ 1, 2, 3, 4, 5 });

    std::map<int, std::vector<int>> groups = p.group_by(fn_parity{});

    bool ok = groups.size() == 2
           && groups[0] == std::vector<int>({ 2, 4 })
           && groups[1] == std::vector<int>({ 1, 3, 5 });
    test::record_assertion(_h, ok,
        "group_by: buckets elements by key, preserving order within a bucket");

    // group_by over empty -> empty map
    std::map<int, std::vector<int>> empty_groups =
        pi::from(std::vector<int>()).group_by(fn_parity{});
    test::record_assertion(_h, empty_groups.empty(),
        "group_by: empty pipeline yields an empty map");

    return;
}


void test_partition(test::test_handler& _h)
{
    pi p = pi::from({ 1, 2, 3, 4, 5, 6 });

    std::pair<pi, pi> parts = p.partition_pipe(fn_is_even{});

    test::record_assertion(_h,
        parts.first.to_vector() == std::vector<int>({ 2, 4, 6 }),
        "partition_pipe: first holds the passing elements");
    test::record_assertion(_h,
        parts.second.to_vector() == std::vector<int>({ 1, 3, 5 }),
        "partition_pipe: second holds the failing elements");

    return;
}


void test_zip_with(test::test_handler& _h)
{
    pi a = pi::from({ 1, 2, 3 });
    pi b = pi::from({ 10, 20, 30, 40 });   // deliberately longer

    // elementwise multiply, truncated to the shorter length
    function_pipeline<int> z = a.zip_with(b, fn_mul{});
    test::record_assertion(_h,
        z.to_vector() == std::vector<int>({ 10, 40, 90 }),
        "zip_with: combines elementwise and truncates to the shorter");

    // zipping with an empty pipeline -> empty
    function_pipeline<int> z_empty =
        a.zip_with(pi::from(std::vector<int>()), fn_mul{});
    test::record_assertion(_h, z_empty.empty(),
        "zip_with: an empty operand yields an empty result");

    return;
}


NS_END  // testing
NS_END  // djinterp
