/*******************************************************************************
* djinterp [test]                                      maybe_tests_factories.cpp
*
* Factory-function tests for maybe.hpp section III:
*   test_factories -- just, nothing<T>, from_pointer (null / non-null),
*                     from_predicate (satisfied / not satisfied).
*
*
* path:      /tests/djinterp/cpp/functional/maybe_tests_factories.cpp
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


bool test_factories()
{
    bool verdict = true;

    // just deduces the decayed value type and wraps the value
    maybe<int> j = just(5);
    record_assertion(verdict, j.has_value() && j.value() == 5,
        "factory: just wraps a value");

    // just from an lvalue (decays reference/const away)
    const std::string s = "hi";
    maybe<std::string> js = just(s);
    record_assertion(verdict, js.has_value() && js.value() == "hi",
        "factory: just decays lvalue argument types");

    // nothing<T> builds an empty maybe of the requested type
    maybe<int> n = nothing<int>();
    record_assertion(verdict, !n.has_value(),
        "factory: nothing<T> is empty");

    // from_pointer: non-null copies the pointee
    int        value = 99;
    maybe<int> fp = from_pointer(&value);
    record_assertion(verdict, fp.has_value() && fp.value() == 99,
        "factory: from_pointer copies a non-null pointee");

    // the pointer is not stored: mutating the source does not affect the maybe
    value = 7;
    record_assertion(verdict, fp.value() == 99,
        "factory: from_pointer stores a copy, not the pointer");

    // from_pointer: null yields nothing
    const int* null_ptr = nullptr;
    maybe<int> fpn = from_pointer(null_ptr);
    record_assertion(verdict, !fpn.has_value(),
        "factory: from_pointer(nullptr) is empty");

    // from_predicate: satisfied -> just
    maybe<int> good = from_predicate(10, pred_is_positive());
    record_assertion(verdict, good.has_value() && good.value() == 10,
        "factory: from_predicate keeps a value that satisfies the predicate");

    // from_predicate: not satisfied -> nothing
    maybe<int> bad = from_predicate(-3, pred_is_positive());
    record_assertion(verdict, !bad.has_value(),
        "factory: from_predicate drops a value that fails the predicate");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
