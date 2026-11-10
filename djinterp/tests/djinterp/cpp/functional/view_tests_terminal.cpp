/*******************************************************************************
* djinterp [test]                                        view_tests_terminal.cpp
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
*
*
* path:      /tests/djinterp/cpp/functional/view_tests_terminal.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <deque>
#include <vector>
// djinterp
#include "./view_tests.hpp"


NS_DJINTERP
NS_TESTING


bool test_to_vector()
{
    bool verdict = true;

    std::vector<int> data = { 3, 1, 2 };

    // preserves order and element type
    record_assertion(verdict,
        (data | to_vector()) == std::vector<int>({ 3, 1, 2 }),
        "to_vector: drains a view into a vector in order");

    // empty view -> empty vector
    record_assertion(verdict,
        (djinterp::empty<int>() | to_vector()).empty(),
        "to_vector: an empty view drains to an empty vector");

    return verdict;
}


bool test_to_container()
{
    bool verdict = true;

    std::vector<int> data = { 1, 2, 3 };

    // collect into a std::deque
    std::deque<int> d = data | djinterp::transform(DFnDouble{})
                             | to<std::deque<int>>();
    bool ok = d.size() == 3 && d[0] == 2 && d[1] == 4 && d[2] == 6;
    record_assertion(verdict, ok,
        "to<C>: drains a view into an explicitly-typed container");

    return verdict;
}


bool test_count()
{
    bool verdict = true;

    std::vector<int> data = { 1, 2, 3, 4, 5 };

    record_assertion(verdict, (data | count()) == 5u,
        "count: reports the number of elements");

    // count after a filter
    record_assertion(verdict,
        (data | djinterp::filter(fn_is_even{}) | count()) == 2u,
        "count: counts the filtered elements");

    // count of an empty view
    record_assertion(verdict, (djinterp::empty<int>() | count()) == 0u,
        "count: an empty view counts zero");

    return verdict;
}


bool test_fold()
{
    bool verdict = true;

    std::vector<int> data = { 1, 2, 3, 4 };

    // sum via fold(0, +)
    record_assertion(verdict, (data | fold(0, DFnAdd{})) == 10,
        "fold: accumulates from the initial value");

    // fold over an empty view returns the init unchanged
    record_assertion(verdict,
        (djinterp::empty<int>() | fold(7, DFnAdd{})) == 7,
        "fold: an empty view returns the initial value");

    // init participates in the result
    record_assertion(verdict, (data | fold(100, DFnAdd{})) == 110,
        "fold: the initial value contributes to the accumulation");

    return verdict;
}


bool test_for_each()
{
    bool verdict = true;

    std::vector<int> data = { 1, 2, 3, 4 };

    long sum = 0;
    data | for_each(accumulate_into(&sum));
    record_assertion(verdict, sum == 10,
        "for_each: visits every element for its side effect");

    // for_each over empty -> no side effects
    long untouched = 5;
    djinterp::empty<int>() | for_each(accumulate_into(&untouched));
    record_assertion(verdict, untouched == 5,
        "for_each: an empty view triggers no side effects");

    return verdict;
}


bool test_any_all_none()
{
    bool verdict = true;

    std::vector<int> data = { 2, 4, 6 };
    std::vector<int> mixed = { 1, 2, 3 };
    std::vector<int> none;

    // any_of
    record_assertion(verdict, (mixed | any_of(fn_is_even{})) == true,
        "any_of: true when at least one element matches");
    record_assertion(verdict, (data | any_of(fn_is_positive{})) == true,
        "any_of: true when all match");
    record_assertion(verdict, (none | any_of(fn_is_even{})) == false,
        "any_of: false for an empty view");

    // all_of
    record_assertion(verdict, (data | all_of(fn_is_even{})) == true,
        "all_of: true when every element matches");
    record_assertion(verdict, (mixed | all_of(fn_is_even{})) == false,
        "all_of: false when some element fails");
    record_assertion(verdict, (none | all_of(fn_is_even{})) == true,
        "all_of: vacuously true for an empty view");

    // none_of
    std::vector<int> odds = { 1, 3, 5 };
    record_assertion(verdict, (odds | none_of(fn_is_even{})) == true,
        "none_of: true when no element matches");
    record_assertion(verdict, (mixed | none_of(fn_is_even{})) == false,
        "none_of: false when some element matches");
    record_assertion(verdict, (none | none_of(fn_is_even{})) == true,
        "none_of: vacuously true for an empty view");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
