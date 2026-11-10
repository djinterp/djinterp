/*******************************************************************************
* djinterp [test]                                      result_tests_equality.cpp
*
* Equality-operator tests for result<T, E>.
*
*   test_equality -- result == / != result across ok/err combinations.
*
*
* path:      /tests/djinterp/cpp/functional/result_tests_equality.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <string>
// djinterp
#include "./result_tests.hpp"


NS_DJINTERP
NS_TESTING


using ri = result<int, std::string>;


bool test_equality()
{
    bool verdict = true;

    ri ok1  = ok<int, std::string>(1);
    ri ok1b = ok<int, std::string>(1);
    ri ok2  = ok<int, std::string>(2);
    ri eA   = err<int, std::string>("a");
    ri eAb  = err<int, std::string>("a");
    ri eB   = err<int, std::string>("b");

    // both ok, equal values
    record_assertion(verdict, ok1 == ok1b,
        "eq: ok with equal values compare equal");

    // both ok, differing values
    record_assertion(verdict, !(ok1 == ok2) && (ok1 != ok2),
        "eq: ok with differing values compare unequal");

    // both err, equal errors
    record_assertion(verdict, eA == eAb,
        "eq: err with equal errors compare equal");

    // both err, differing errors
    record_assertion(verdict, !(eA == eB) && (eA != eB),
        "eq: err with differing errors compare unequal");

    // ok vs err always unequal
    record_assertion(verdict, !(ok1 == eA) && (ok1 != eA),
        "eq: ok and err compare unequal");

    // operator!= mirrors operator==
    record_assertion(verdict, !(ok1 != ok1b) && !(eA != eAb),
        "eq: operator!= negates operator==");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
