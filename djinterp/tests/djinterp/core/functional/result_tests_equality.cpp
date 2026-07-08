/******************************************************************************
* djinterp [test]                                    result_tests_equality.cpp
*
* Equality-operator tests for result<T, E>.
*
*   test_equality -- result == / != result across ok/err combinations.
******************************************************************************/

#include <string>
#include "./result_tests.hpp"


NS_DJINTERP
NS_TESTING


using ri = result<int, std::string>;


void test_equality(test::test_handler& _h)
{
    ri ok1  = ok<int, std::string>(1);
    ri ok1b = ok<int, std::string>(1);
    ri ok2  = ok<int, std::string>(2);
    ri eA   = err<int, std::string>("a");
    ri eAb  = err<int, std::string>("a");
    ri eB   = err<int, std::string>("b");

    // both ok, equal values
    test::record_assertion(_h, ok1 == ok1b,
        "eq: ok with equal values compare equal");

    // both ok, differing values
    test::record_assertion(_h, !(ok1 == ok2) && (ok1 != ok2),
        "eq: ok with differing values compare unequal");

    // both err, equal errors
    test::record_assertion(_h, eA == eAb,
        "eq: err with equal errors compare equal");

    // both err, differing errors
    test::record_assertion(_h, !(eA == eB) && (eA != eB),
        "eq: err with differing errors compare unequal");

    // ok vs err always unequal
    test::record_assertion(_h, !(ok1 == eA) && (ok1 != eA),
        "eq: ok and err compare unequal");

    // operator!= mirrors operator==
    test::record_assertion(_h, !(ok1 != ok1b) && !(eA != eAb),
        "eq: operator!= negates operator==");

    return;
}


NS_END  // testing
NS_END  // djinterp
