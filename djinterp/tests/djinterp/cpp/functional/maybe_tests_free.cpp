/*******************************************************************************
* djinterp [test]                                           maybe_tests_free.cpp
*
* Free-function helper tests for maybe.hpp section VI:
*   test_zip_with -- combine two maybes via a binary function; nothing if
*                    either side is empty.
*   test_flatten  -- collapse maybe<maybe<T>> to maybe<T>.
*   test_collect  -- container of maybe<T> -> maybe<vector<T>>: just iff all
*                    elements are present.
*
*
* path:      /tests/djinterp/cpp/functional/maybe_tests_free.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.01
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <vector>
// djinterp
#include "./maybe_tests.hpp"


NS_DJINTERP
NS_TESTING


bool test_zip_with()
{
    bool verdict = true;

    maybe<int> a(3);
    maybe<int> b(4);
    maybe<int> e;

    // both present -> just(f(a, b))
    maybe<int> sum = zip_with(a, b, DFnAdd());
    record_assertion(verdict, sum.has_value() && sum.value() == 7,
        "zip_with: both present combines via the function");

    // left empty -> nothing
    maybe<int> z1 = zip_with(e, b, DFnAdd());
    record_assertion(verdict, !z1.has_value(),
        "zip_with: empty left operand yields nothing");

    // right empty -> nothing
    maybe<int> z2 = zip_with(a, e, DFnAdd());
    record_assertion(verdict, !z2.has_value(),
        "zip_with: empty right operand yields nothing");

    // both empty -> nothing
    maybe<int> z3 = zip_with(e, e, DFnAdd());
    record_assertion(verdict, !z3.has_value(),
        "zip_with: both empty yields nothing");

    return verdict;
}


bool test_flatten()
{
    bool verdict = true;

    // just(just(x)) -> just(x)
    maybe<maybe<int>> nested = just(just(5));
    maybe<int>        flat   = flatten(nested);
    record_assertion(verdict, flat.has_value() && flat.value() == 5,
        "flatten: just(just(x)) collapses to just(x)");

    // just(nothing) -> nothing
    maybe<maybe<int>> just_nothing = just(nothing<int>());
    maybe<int>        fn1          = flatten(just_nothing);
    record_assertion(verdict, !fn1.has_value(),
        "flatten: just(nothing) collapses to nothing");

    // outer nothing -> nothing
    maybe<maybe<int>> outer_empty;
    maybe<int>        fn2 = flatten(outer_empty);
    record_assertion(verdict, !fn2.has_value(),
        "flatten: outer nothing yields nothing");

    return verdict;
}


bool test_collect()
{
    bool verdict = true;

    // all-present -> just(vector) preserving order
    std::vector<maybe<int>> all_just;
    all_just.push_back(just(1));
    all_just.push_back(just(2));
    all_just.push_back(just(3));

    maybe<std::vector<int>> collected = collect(all_just);
    bool ok = collected.has_value()
           && collected.value().size() == 3
           && collected.value()[0] == 1
           && collected.value()[1] == 2
           && collected.value()[2] == 3;
    record_assertion(verdict, ok,
        "collect: all-present yields just(vector) in order");

    // one missing -> nothing
    std::vector<maybe<int>> with_gap;
    with_gap.push_back(just(1));
    with_gap.push_back(nothing<int>());
    with_gap.push_back(just(3));

    maybe<std::vector<int>> gapped = collect(with_gap);
    record_assertion(verdict, !gapped.has_value(),
        "collect: a single nothing collapses the whole result");

    // empty container -> just(empty vector) (vacuously all present)
    std::vector<maybe<int>> empty_container;
    maybe<std::vector<int>> empty_collected = collect(empty_container);
    record_assertion(verdict,
        empty_collected.has_value() && empty_collected.value().empty(),
        "collect: empty input yields just(empty vector)");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
