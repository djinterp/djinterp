/******************************************************************************
* djinterp [test]                                    maybe_tests_factories.cpp
*
* Factory-function tests for maybe.hpp section III:
*   test_factories -- just, nothing<T>, from_pointer (null / non-null),
*                     from_predicate (satisfied / not satisfied).
******************************************************************************/

#include <string>
#include "./maybe_tests.hpp"


NS_DJINTERP
NS_TESTING


void test_factories(test::test_handler& _h)
{
    // just deduces the decayed value type and wraps the value
    maybe<int> j = just(5);
    test::record_assertion(_h, j.has_value() && j.value() == 5,
        "factory: just wraps a value");

    // just from an lvalue (decays reference/const away)
    const std::string s = "hi";
    maybe<std::string> js = just(s);
    test::record_assertion(_h, js.has_value() && js.value() == "hi",
        "factory: just decays lvalue argument types");

    // nothing<T> builds an empty maybe of the requested type
    maybe<int> n = nothing<int>();
    test::record_assertion(_h, !n.has_value(),
        "factory: nothing<T> is empty");

    // from_pointer: non-null copies the pointee
    int        value = 99;
    maybe<int> fp = from_pointer(&value);
    test::record_assertion(_h, fp.has_value() && fp.value() == 99,
        "factory: from_pointer copies a non-null pointee");

    // the pointer is not stored: mutating the source does not affect the maybe
    value = 7;
    test::record_assertion(_h, fp.value() == 99,
        "factory: from_pointer stores a copy, not the pointer");

    // from_pointer: null yields nothing
    const int* null_ptr = nullptr;
    maybe<int> fpn = from_pointer(null_ptr);
    test::record_assertion(_h, !fpn.has_value(),
        "factory: from_pointer(nullptr) is empty");

    // from_predicate: satisfied -> just
    maybe<int> good = from_predicate(10, pred_is_positive());
    test::record_assertion(_h, good.has_value() && good.value() == 10,
        "factory: from_predicate keeps a value that satisfies the predicate");

    // from_predicate: not satisfied -> nothing
    maybe<int> bad = from_predicate(-3, pred_is_positive());
    test::record_assertion(_h, !bad.has_value(),
        "factory: from_predicate drops a value that fails the predicate");

    return;
}


NS_END  // testing
NS_END  // djinterp
