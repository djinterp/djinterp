/******************************************************************************
* djinterp [test]                                fn_builder_tests_transform.cpp
*
* Transforming fluent-operation tests (section ii, part 1):
*   test_map      -- map (incl. type-changing) and and_then (its alias).
*   test_filter   -- filter and where (its alias).
*   test_flat_map -- map each element to a container then flatten.
******************************************************************************/

#include <vector>
#include <string>
#include "./fn_builder_tests.hpp"


NS_DJINTERP
NS_TESTING


void test_map(test::test_handler& _h)
{
    std::vector<int> in = { 1, 2, 3 };

    // same-type map
    test::record_assertion(_h,
        fn_builder<int>::create().map(fn_double{}).execute(in)
            == std::vector<int>({ 2, 4, 6 }),
        "map: applies the function to each element");

    // type-changing map (int -> std::string)
    test::record_assertion(_h,
        fn_builder<int>::create().map(fn_to_string{}).execute(in)
            == std::vector<std::string>({ "1", "2", "3" }),
        "map: can change the element type");

    // and_then is an alias for map
    test::record_assertion(_h,
        fn_builder<int>::create().and_then(fn_add_one{}).execute(in)
            == std::vector<int>({ 2, 3, 4 }),
        "and_then: behaves as an alias for map");

    // chained maps compose
    test::record_assertion(_h,
        fn_builder<int>::create()
            .map(fn_double{})
            .map(fn_add_one{})
            .execute(in)
            == std::vector<int>({ 3, 5, 7 }),
        "map: successive maps compose left-to-right");

    return;
}


void test_filter(test::test_handler& _h)
{
    std::vector<int> in = { 1, 2, 3, 4, 5, 6 };

    // filter keeps matching
    test::record_assertion(_h,
        fn_builder<int>::create().filter(fn_is_even{}).execute(in)
            == std::vector<int>({ 2, 4, 6 }),
        "filter: keeps elements satisfying the predicate");

    // where is an alias for filter
    test::record_assertion(_h,
        fn_builder<int>::create().where(fn_is_even{}).execute(in)
            == std::vector<int>({ 2, 4, 6 }),
        "where: behaves as an alias for filter");

    // filter then map composes
    test::record_assertion(_h,
        fn_builder<int>::create()
            .filter(fn_is_even{})
            .map(fn_double{})
            .execute(in)
            == std::vector<int>({ 4, 8, 12 }),
        "filter: composes with a following map");

    return;
}


void test_flat_map(test::test_handler& _h)
{
    std::vector<int> in = { 1, 2 };

    // each element explodes to { x, x + 100 }, then flattens
    test::record_assertion(_h,
        fn_builder<int>::create().flat_map(fn_explode{}).execute(in)
            == std::vector<int>({ 1, 101, 2, 102 }),
        "flat_map: maps each element to a container and flattens");

    // flat_map over empty -> empty
    test::record_assertion(_h,
        fn_builder<int>::create()
            .flat_map(fn_explode{})
            .execute(std::vector<int>())
            .empty(),
        "flat_map: empty input yields empty output");

    return;
}


NS_END  // testing
NS_END  // djinterp
