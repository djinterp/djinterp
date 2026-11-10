/*******************************************************************************
* djinterp [test]                                   pipeline_tests_aggregate.cpp
*
* Aggregating-operation tests (section ii / iii):
*   test_fold      -- left fold with an explicit init (incl. empty).
*   test_reduce    -- reduce over a non-empty pipeline.
*   test_group_by  -- group elements into a map keyed by a function.
*   test_partition -- split into (passing, failing) pipelines.
*   test_zip_with  -- elementwise binary combine of two pipelines.
*
*
* path:      /tests/djinterp/cpp/functional/pipeline_tests_aggregate.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <map>
#include <utility>
#include <vector>
// djinterp
#include "./pipeline_tests.hpp"


NS_DJINTERP
NS_TESTING


using pi = DFunctionPipeline<int>;


bool test_fold()
{
    bool verdict = true;

    pi p = pi::from({ 1, 2, 3, 4 });

    // sum from 0
    record_assertion(verdict, p.fold(0, DFnAdd{}) == 10,
        "fold: accumulates from the initial value");

    // init contributes
    record_assertion(verdict, p.fold(100, DFnAdd{}) == 110,
        "fold: the initial value participates");

    // empty pipeline -> init unchanged
    record_assertion(verdict,
        pi::from(std::vector<int>()).fold(7, DFnAdd{}) == 7,
        "fold: empty pipeline returns the initial value");

    return verdict;
}


bool test_reduce()
{
    bool verdict = true;

    // reduce requires a non-empty pipeline
    pi p = pi::from({ 1, 2, 3, 4 });
    record_assertion(verdict, p.reduce(DFnAdd{}) == 10,
        "reduce: folds using the first element as the seed");

    // single element reduces to itself
    pi one = pi::from({ 42 });
    record_assertion(verdict, one.reduce(DFnAdd{}) == 42,
        "reduce: a single element reduces to itself");

    return verdict;
}


bool test_group_by()
{
    bool verdict = true;

    pi p = pi::from({ 1, 2, 3, 4, 5 });

    std::map<int, std::vector<int>> groups = p.group_by(DFnParity{});

    bool ok = groups.size() == 2
           && groups[0] == std::vector<int>({ 2, 4 })
           && groups[1] == std::vector<int>({ 1, 3, 5 });
    record_assertion(verdict, ok,
        "group_by: buckets elements by key, preserving order within a bucket");

    // group_by over empty -> empty map
    std::map<int, std::vector<int>> empty_groups =
        pi::from(std::vector<int>()).group_by(DFnParity{});
    record_assertion(verdict, empty_groups.empty(),
        "group_by: empty pipeline yields an empty map");

    return verdict;
}


bool test_partition()
{
    bool verdict = true;

    pi p = pi::from({ 1, 2, 3, 4, 5, 6 });

    std::pair<pi, pi> parts = p.partition_pipe(fn_is_even{});

    record_assertion(verdict,
        parts.first.to_vector() == std::vector<int>({ 2, 4, 6 }),
        "partition_pipe: first holds the passing elements");
    record_assertion(verdict,
        parts.second.to_vector() == std::vector<int>({ 1, 3, 5 }),
        "partition_pipe: second holds the failing elements");

    return verdict;
}


bool test_zip_with()
{
    bool verdict = true;

    pi a = pi::from({ 1, 2, 3 });
    pi b = pi::from({ 10, 20, 30, 40 });   // deliberately longer

    // elementwise multiply, truncated to the shorter length
    DFunctionPipeline<int> z = a.zip_with(b, DFnMul{});
    record_assertion(verdict,
        z.to_vector() == std::vector<int>({ 10, 40, 90 }),
        "zip_with: combines elementwise and truncates to the shorter");

    // zipping with an empty pipeline -> empty
    DFunctionPipeline<int> z_empty =
        a.zip_with(pi::from(std::vector<int>()), DFnMul{});
    record_assertion(verdict, z_empty.empty(),
        "zip_with: an empty operand yields an empty result");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
