/******************************************************************************
* djinterp [test]                                          maybe_tests_free.cpp
*
* Free-function helper tests for maybe.hpp section VI:
*   test_zip_with -- combine two maybes via a binary function; nothing if
*                    either side is empty.
*   test_flatten  -- collapse maybe<maybe<T>> to maybe<T>.
*   test_collect  -- container of maybe<T> -> maybe<vector<T>>: just iff all
*                    elements are present.
******************************************************************************/

#include <vector>
#include "./maybe_tests.hpp"


NS_DJINTERP
NS_TESTING


void test_zip_with(test::test_handler& _h)
{
    maybe<int> a(3);
    maybe<int> b(4);
    maybe<int> e;

    // both present -> just(f(a, b))
    maybe<int> sum = zip_with(a, b, fn_add());
    test::record_assertion(_h, sum.has_value() && sum.value() == 7,
        "zip_with: both present combines via the function");

    // left empty -> nothing
    maybe<int> z1 = zip_with(e, b, fn_add());
    test::record_assertion(_h, !z1.has_value(),
        "zip_with: empty left operand yields nothing");

    // right empty -> nothing
    maybe<int> z2 = zip_with(a, e, fn_add());
    test::record_assertion(_h, !z2.has_value(),
        "zip_with: empty right operand yields nothing");

    // both empty -> nothing
    maybe<int> z3 = zip_with(e, e, fn_add());
    test::record_assertion(_h, !z3.has_value(),
        "zip_with: both empty yields nothing");

    return;
}


void test_flatten(test::test_handler& _h)
{
    // just(just(x)) -> just(x)
    maybe<maybe<int>> nested = just(just(5));
    maybe<int>        flat   = flatten(nested);
    test::record_assertion(_h, flat.has_value() && flat.value() == 5,
        "flatten: just(just(x)) collapses to just(x)");

    // just(nothing) -> nothing
    maybe<maybe<int>> just_nothing = just(nothing<int>());
    maybe<int>        fn1          = flatten(just_nothing);
    test::record_assertion(_h, !fn1.has_value(),
        "flatten: just(nothing) collapses to nothing");

    // outer nothing -> nothing
    maybe<maybe<int>> outer_empty;
    maybe<int>        fn2 = flatten(outer_empty);
    test::record_assertion(_h, !fn2.has_value(),
        "flatten: outer nothing yields nothing");

    return;
}


void test_collect(test::test_handler& _h)
{
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
    test::record_assertion(_h, ok,
        "collect: all-present yields just(vector) in order");

    // one missing -> nothing
    std::vector<maybe<int>> with_gap;
    with_gap.push_back(just(1));
    with_gap.push_back(nothing<int>());
    with_gap.push_back(just(3));

    maybe<std::vector<int>> gapped = collect(with_gap);
    test::record_assertion(_h, !gapped.has_value(),
        "collect: a single nothing collapses the whole result");

    // empty container -> just(empty vector) (vacuously all present)
    std::vector<maybe<int>> empty_container;
    maybe<std::vector<int>> empty_collected = collect(empty_container);
    test::record_assertion(_h,
        empty_collected.has_value() && empty_collected.value().empty(),
        "collect: empty input yields just(empty vector)");

    return;
}


NS_END  // testing
NS_END  // djinterp
