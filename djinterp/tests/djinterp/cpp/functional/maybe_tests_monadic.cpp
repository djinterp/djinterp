/*******************************************************************************
* djinterp [test]                                        maybe_tests_monadic.cpp
*
* Monadic / fluent method tests for maybe<T> (section I methods that thread or
* gate the contained value).
*
*   test_map       -- value-preserving and type-changing map over present /
*                     absent maybes.
*   test_and_then   -- monadic bind: function invoked only when present;
*                     empty short-circuits to an empty result.
*   test_or_else    -- present returns *this; empty invokes the fallback.
*   test_filter     -- predicate pass keeps the value, fail / empty -> nothing.
*   test_match      -- pattern-match dispatch across both branches.
*
*
* path:      /tests/djinterp/cpp/functional/maybe_tests_monadic.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.01
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <string>
// djinterp
#include "./maybe_tests.hpp"


NS_DJINTERP
NS_TESTING


bool test_map()
{
    bool verdict = true;

    // map over a present value
    maybe<int> v(5);
    maybe<int> doubled = v.map(DFnDouble());
    record_assertion(verdict, doubled.has_value() && doubled.value() == 10,
        "map: applies function to a present value");

    // map over empty yields an empty mapped maybe
    maybe<int> e;
    maybe<int> mapped_empty = e.map(DFnDouble());
    record_assertion(verdict, !mapped_empty.has_value(),
        "map: empty maps to empty");

    // type-changing map: int -> std::string
    maybe<std::string> str = v.map(fn_to_string());
    record_assertion(verdict, str.has_value() && str.value() == "5",
        "map: changes the contained type");

    return verdict;
}


bool test_and_then()
{
    bool verdict = true;

    // and_then on a value that satisfies the inner function
    maybe<int> even(8);
    maybe<int> r1 = even.and_then(DFnHalfIfEven());
    record_assertion(verdict, r1.has_value() && r1.value() == 4,
        "and_then: present value threaded through function");

    // and_then where the inner function itself yields nothing
    maybe<int> odd(7);
    maybe<int> r2 = odd.and_then(DFnHalfIfEven());
    record_assertion(verdict, !r2.has_value(),
        "and_then: inner nothing propagates");

    // and_then on empty short-circuits without invoking the function
    maybe<int> e;
    maybe<int> r3 = e.and_then(DFnHalfIfEven());
    record_assertion(verdict, !r3.has_value(),
        "and_then: empty short-circuits to empty");

    return verdict;
}


bool test_or_else()
{
    bool verdict = true;

    // present value returns itself, fallback not consulted
    maybe<int> v(3);
    maybe<int> r1 = v.or_else(DFnMakeJust());
    record_assertion(verdict, r1.has_value() && r1.value() == 3,
        "or_else: present value is returned unchanged");

    // empty invokes the fallback (which yields a value)
    maybe<int> e;
    maybe<int> r2 = e.or_else(DFnMakeJust());
    record_assertion(verdict, r2.has_value() && r2.value() == 99,
        "or_else: empty falls back to the function result");

    // empty with a fallback that also yields nothing
    maybe<int> r3 = e.or_else(DFnMakeNothing());
    record_assertion(verdict, !r3.has_value(),
        "or_else: empty fallback stays empty");

    return verdict;
}


bool test_filter()
{
    bool verdict = true;

    // predicate satisfied -> value retained
    maybe<int> v(4);
    maybe<int> kept = v.filter(pred_is_even());
    record_assertion(verdict, kept.has_value() && kept.value() == 4,
        "filter: passing predicate retains the value");

    // predicate failed -> nothing
    maybe<int> odd(5);
    maybe<int> dropped = odd.filter(pred_is_even());
    record_assertion(verdict, !dropped.has_value(),
        "filter: failing predicate yields nothing");

    // empty -> nothing (predicate never invoked)
    maybe<int> e;
    maybe<int> filtered_empty = e.filter(pred_is_even());
    record_assertion(verdict, !filtered_empty.has_value(),
        "filter: empty stays empty");

    return verdict;
}


bool test_match()
{
    bool verdict = true;

    // just-branch
    maybe<int>  v(42);
    std::string a = v.match(fn_to_string(), DFnNothingLabel());
    record_assertion(verdict, a == "42",
        "match: present value dispatches to the just branch");

    // nothing-branch
    maybe<int>  e;
    std::string b = e.match(fn_to_string(), DFnNothingLabel());
    record_assertion(verdict, b == "none",
        "match: empty dispatches to the nothing branch");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
