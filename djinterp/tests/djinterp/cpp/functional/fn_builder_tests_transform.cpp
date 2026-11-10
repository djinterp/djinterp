/*******************************************************************************
* djinterp [test]                                 fn_builder_tests_transform.cpp
*
* Transforming fluent-operation tests (section ii, part 1):
*   test_map      -- map (incl. type-changing) and and_then (its alias).
*   test_filter   -- filter and where (its alias).
*   test_flat_map -- map each element to a container then flatten.
*
*
* path:      /tests/djinterp/cpp/functional/fn_builder_tests_transform.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <string>
#include <vector>
// djinterp
#include "./fn_builder_tests.hpp"


NS_DJINTERP
NS_TESTING


bool test_map()
{
    bool verdict = true;

    std::vector<int> in = { 1, 2, 3 };

    // same-type map
    record_assertion(verdict,
        DFnBuilder<int>::create().map(DFnDouble{}).execute(in)
            == std::vector<int>({ 2, 4, 6 }),
        "map: applies the function to each element");

    // type-changing map (int -> std::string)
    record_assertion(verdict,
        DFnBuilder<int>::create().map(fn_to_string{}).execute(in)
            == std::vector<std::string>({ "1", "2", "3" }),
        "map: can change the element type");

    // and_then is an alias for map
    record_assertion(verdict,
        DFnBuilder<int>::create().and_then(DFnAddOne{}).execute(in)
            == std::vector<int>({ 2, 3, 4 }),
        "and_then: behaves as an alias for map");

    // chained maps compose
    record_assertion(verdict,
        DFnBuilder<int>::create()
            .map(DFnDouble{})
            .map(DFnAddOne{})
            .execute(in)
            == std::vector<int>({ 3, 5, 7 }),
        "map: successive maps compose left-to-right");

    return verdict;
}


bool test_filter()
{
    bool verdict = true;

    std::vector<int> in = { 1, 2, 3, 4, 5, 6 };

    // filter keeps matching
    record_assertion(verdict,
        DFnBuilder<int>::create().filter(fn_is_even{}).execute(in)
            == std::vector<int>({ 2, 4, 6 }),
        "filter: keeps elements satisfying the predicate");

    // where is an alias for filter
    record_assertion(verdict,
        DFnBuilder<int>::create().where(fn_is_even{}).execute(in)
            == std::vector<int>({ 2, 4, 6 }),
        "where: behaves as an alias for filter");

    // filter then map composes
    record_assertion(verdict,
        DFnBuilder<int>::create()
            .filter(fn_is_even{})
            .map(DFnDouble{})
            .execute(in)
            == std::vector<int>({ 4, 8, 12 }),
        "filter: composes with a following map");

    return verdict;
}


bool test_flat_map()
{
    bool verdict = true;

    std::vector<int> in = { 1, 2 };

    // each element explodes to { x, x + 100 }, then flattens
    record_assertion(verdict,
        DFnBuilder<int>::create().flat_map(DFnExplode{}).execute(in)
            == std::vector<int>({ 1, 101, 2, 102 }),
        "flat_map: maps each element to a container and flattens");

    // flat_map over empty -> empty
    record_assertion(verdict,
        DFnBuilder<int>::create()
            .flat_map(DFnExplode{})
            .execute(std::vector<int>())
            .empty(),
        "flat_map: empty input yields empty output");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
