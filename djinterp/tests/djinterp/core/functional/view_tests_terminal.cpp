/******************************************************************************
* djinterp [test]                                      view_tests_terminal.cpp
*
* Terminal operator tests (section VII): the drains that force a pipeline.
*
*   test_to_vector    -- collect into a std::vector<value_type>.
*   test_to_container -- collect into an explicitly-typed container.
*   test_count        -- element count of a (possibly bounded) view.
*   test_fold         -- left fold with an explicit init.
*   test_for_each     -- visit every element for side effects.
*   test_any_all_none -- short-circuiting predicate drains, incl. empty views.
*
*   NOTE: the table of contents also lists reduce / find_if / min_element /
* max_element, but those terminals are not implemented in view.hpp, so there
* is nothing to exercise for them here.
******************************************************************************/

#include <vector>
#include <deque>
#include "./view_tests.hpp"


NS_DJINTERP
NS_TESTING


void test_to_vector(test::test_handler& _h)
{
    std::vector<int> data = { 3, 1, 2 };

    // preserves order and element type
    test::record_assertion(_h,
        (data | to_vector()) == std::vector<int>({ 3, 1, 2 }),
        "to_vector: drains a view into a vector in order");

    // empty view -> empty vector
    test::record_assertion(_h,
        (views::empty<int>() | to_vector()).empty(),
        "to_vector: an empty view drains to an empty vector");

    return;
}


void test_to_container(test::test_handler& _h)
{
    std::vector<int> data = { 1, 2, 3 };

    // collect into a std::deque
    std::deque<int> d = data | views::transform(fn_double{})
                             | to<std::deque<int>>();
    bool ok = d.size() == 3 && d[0] == 2 && d[1] == 4 && d[2] == 6;
    test::record_assertion(_h, ok,
        "to<C>: drains a view into an explicitly-typed container");

    return;
}


void test_count(test::test_handler& _h)
{
    std::vector<int> data = { 1, 2, 3, 4, 5 };

    test::record_assertion(_h, (data | count()) == 5u,
        "count: reports the number of elements");

    // count after a filter
    test::record_assertion(_h,
        (data | views::filter(fn_is_even{}) | count()) == 2u,
        "count: counts the filtered elements");

    // count of an empty view
    test::record_assertion(_h, (views::empty<int>() | count()) == 0u,
        "count: an empty view counts zero");

    return;
}


void test_fold(test::test_handler& _h)
{
    std::vector<int> data = { 1, 2, 3, 4 };

    // sum via fold(0, +)
    test::record_assertion(_h, (data | fold(0, fn_add{})) == 10,
        "fold: accumulates from the initial value");

    // fold over an empty view returns the init unchanged
    test::record_assertion(_h,
        (views::empty<int>() | fold(7, fn_add{})) == 7,
        "fold: an empty view returns the initial value");

    // init participates in the result
    test::record_assertion(_h, (data | fold(100, fn_add{})) == 110,
        "fold: the initial value contributes to the accumulation");

    return;
}


void test_for_each(test::test_handler& _h)
{
    std::vector<int> data = { 1, 2, 3, 4 };

    long sum = 0;
    data | for_each(accumulate_into(&sum));
    test::record_assertion(_h, sum == 10,
        "for_each: visits every element for its side effect");

    // for_each over empty -> no side effects
    long untouched = 5;
    views::empty<int>() | for_each(accumulate_into(&untouched));
    test::record_assertion(_h, untouched == 5,
        "for_each: an empty view triggers no side effects");

    return;
}


void test_any_all_none(test::test_handler& _h)
{
    std::vector<int> data = { 2, 4, 6 };
    std::vector<int> mixed = { 1, 2, 3 };
    std::vector<int> none;

    // any_of
    test::record_assertion(_h, (mixed | any_of(fn_is_even{})) == true,
        "any_of: true when at least one element matches");
    test::record_assertion(_h, (data | any_of(fn_is_positive{})) == true,
        "any_of: true when all match");
    test::record_assertion(_h, (none | any_of(fn_is_even{})) == false,
        "any_of: false for an empty view");

    // all_of
    test::record_assertion(_h, (data | all_of(fn_is_even{})) == true,
        "all_of: true when every element matches");
    test::record_assertion(_h, (mixed | all_of(fn_is_even{})) == false,
        "all_of: false when some element fails");
    test::record_assertion(_h, (none | all_of(fn_is_even{})) == true,
        "all_of: vacuously true for an empty view");

    // none_of
    std::vector<int> odds = { 1, 3, 5 };
    test::record_assertion(_h, (odds | none_of(fn_is_even{})) == true,
        "none_of: true when no element matches");
    test::record_assertion(_h, (mixed | none_of(fn_is_even{})) == false,
        "none_of: false when some element matches");
    test::record_assertion(_h, (none | none_of(fn_is_even{})) == true,
        "none_of: vacuously true for an empty view");

    return;
}


NS_END  // testing
NS_END  // djinterp
