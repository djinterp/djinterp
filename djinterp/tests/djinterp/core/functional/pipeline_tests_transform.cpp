/******************************************************************************
* djinterp [test]                                 pipeline_tests_transform.cpp
*
* Transforming-operation tests (section ii, part 1):
*   test_map      -- element-wise map, including a type-changing map.
*   test_filter   -- filter and filter_not.
*   test_distinct -- distinct() via operator== and distinct(eq) via a custom
*                    equality.
*   test_flat_map -- map-to-container then flatten.
******************************************************************************/

#include <vector>
#include <string>
#include "./pipeline_tests.hpp"


NS_DJINTERP
NS_TESTING


using pi = function_pipeline<int>;


void test_map(test::test_handler& _h)
{
    pi p = pi::from({ 1, 2, 3 });

    // same-type map
    test::record_assertion(_h,
        p.map(fn_double{}).to_vector() == std::vector<int>({ 2, 4, 6 }),
        "map: applies the function to each element");

    // type-changing map (int -> std::string)
    function_pipeline<std::string> strs = p.map(fn_to_string{});
    test::record_assertion(_h,
        strs.to_vector()
            == std::vector<std::string>({ "1", "2", "3" }),
        "map: can change the element type");

    // map over empty -> empty
    test::record_assertion(_h,
        pi::from(std::vector<int>()).map(fn_double{}).empty(),
        "map: empty input yields empty output");

    return;
}


void test_filter(test::test_handler& _h)
{
    pi p = pi::from({ 1, 2, 3, 4, 5, 6 });

    // filter keeps matching
    test::record_assertion(_h,
        p.filter(fn_is_even{}).to_vector() == std::vector<int>({ 2, 4, 6 }),
        "filter: keeps elements satisfying the predicate");

    // filter_not keeps failing
    test::record_assertion(_h,
        p.filter_not(fn_is_even{}).to_vector()
            == std::vector<int>({ 1, 3, 5 }),
        "filter_not: keeps elements failing the predicate");

    // filter none-match -> empty
    test::record_assertion(_h,
        pi::from({ 1, 3, 5 }).filter(fn_is_even{}).empty(),
        "filter: yields empty when nothing matches");

    return;
}


void test_distinct(test::test_handler& _h)
{
    // distinct() via operator==, preserving first-seen order
    pi p = pi::from({ 1, 2, 2, 3, 1, 3, 4 });
    test::record_assertion(_h,
        p.distinct().to_vector() == std::vector<int>({ 1, 2, 3, 4 }),
        "distinct: removes duplicates, preserving first-seen order");

    // distinct(eq) via custom equality (mod 10): 1 and 11 collide
    pi q = pi::from({ 1, 11, 2, 22, 3 });
    test::record_assertion(_h,
        q.distinct(fn_eq_mod10{}).to_vector()
            == std::vector<int>({ 1, 2, 3 }),
        "distinct(eq): collapses elements equal under the custom relation");

    return;
}


void test_flat_map(test::test_handler& _h)
{
    pi p = pi::from({ 1, 2 });

    // each element explodes to { x, x + 100 }, then flattens
    test::record_assertion(_h,
        p.flat_map(fn_explode{}).to_vector()
            == std::vector<int>({ 1, 101, 2, 102 }),
        "flat_map: maps each element to a container and flattens");

    // flat_map over empty -> empty
    test::record_assertion(_h,
        pi::from(std::vector<int>()).flat_map(fn_explode{}).empty(),
        "flat_map: empty input yields empty output");

    return;
}


NS_END  // testing
NS_END  // djinterp
