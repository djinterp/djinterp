/*******************************************************************************
* djinterp [test]                                   pipeline_tests_transform.cpp
*
* Transforming-operation tests (section ii, part 1):
*   test_map      -- element-wise map, including a type-changing map.
*   test_filter   -- filter and filter_not.
*   test_distinct -- distinct() via operator== and distinct(eq) via a custom
*                    equality.
*   test_flat_map -- map-to-container then flatten.
*
*
* path:      /tests/djinterp/cpp/functional/pipeline_tests_transform.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <string>
#include <vector>
// djinterp
#include "./pipeline_tests.hpp"


NS_DJINTERP
NS_TESTING


using pi = DFunctionPipeline<int>;


bool test_map()
{
    bool verdict = true;

    pi p = pi::from({ 1, 2, 3 });

    // same-type map
    record_assertion(verdict,
        p.map(DFnDouble{}).to_vector() == std::vector<int>({ 2, 4, 6 }),
        "map: applies the function to each element");

    // type-changing map (int -> std::string)
    DFunctionPipeline<std::string> strs = p.map(fn_to_string{});
    record_assertion(verdict,
        strs.to_vector()
            == std::vector<std::string>({ "1", "2", "3" }),
        "map: can change the element type");

    // map over empty -> empty
    record_assertion(verdict,
        pi::from(std::vector<int>()).map(DFnDouble{}).empty(),
        "map: empty input yields empty output");

    return verdict;
}


bool test_filter()
{
    bool verdict = true;

    pi p = pi::from({ 1, 2, 3, 4, 5, 6 });

    // filter keeps matching
    record_assertion(verdict,
        p.filter(fn_is_even{}).to_vector() == std::vector<int>({ 2, 4, 6 }),
        "filter: keeps elements satisfying the predicate");

    // filter_not keeps failing
    record_assertion(verdict,
        p.filter_not(fn_is_even{}).to_vector()
            == std::vector<int>({ 1, 3, 5 }),
        "filter_not: keeps elements failing the predicate");

    // filter none-match -> empty
    record_assertion(verdict,
        pi::from({ 1, 3, 5 }).filter(fn_is_even{}).empty(),
        "filter: yields empty when nothing matches");

    return verdict;
}


bool test_distinct()
{
    bool verdict = true;

    // distinct() via operator==, preserving first-seen order
    pi p = pi::from({ 1, 2, 2, 3, 1, 3, 4 });
    record_assertion(verdict,
        p.distinct().to_vector() == std::vector<int>({ 1, 2, 3, 4 }),
        "distinct: removes duplicates, preserving first-seen order");

    // distinct(eq) via custom equality (mod 10): 1 and 11 collide
    pi q = pi::from({ 1, 11, 2, 22, 3 });
    record_assertion(verdict,
        q.distinct(DFnEqMod10{}).to_vector()
            == std::vector<int>({ 1, 2, 3 }),
        "distinct(eq): collapses elements equal under the custom relation");

    return verdict;
}


bool test_flat_map()
{
    bool verdict = true;

    pi p = pi::from({ 1, 2 });

    // each element explodes to { x, x + 100 }, then flattens
    record_assertion(verdict,
        p.flat_map(DFnExplode{}).to_vector()
            == std::vector<int>({ 1, 101, 2, 102 }),
        "flat_map: maps each element to a container and flattens");

    // flat_map over empty -> empty
    record_assertion(verdict,
        pi::from(std::vector<int>()).flat_map(DFnExplode{}).empty(),
        "flat_map: empty input yields empty output");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
