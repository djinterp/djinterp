/******************************************************************************
* djinterp [test]                                     maybe_tests_equality.cpp
*
* Equality-operator tests for maybe<T>.
*
*   test_equality -- maybe == / != maybe across present/absent combinations,
*                    plus the two maybe/nothing_t overloads.
*
*   NOTE on the maybe/nothing_t overloads: as implemented in maybe.hpp these
* two are ASYMMETRIC.  `nothing_v == m` returns `!m.has_value()`, but
* `m == nothing_v` returns a hard-coded `false` for every maybe (including an
* empty one).  The assertions below pin the behavior AS WRITTEN so a future
* change is caught; the asymmetry is very likely a defect in maybe.hpp worth a
* separate fix (the left-hand overload should mirror the right-hand one).
******************************************************************************/

#include "./maybe_tests.hpp"


NS_DJINTERP
NS_TESTING


void test_equality(test::test_handler& _h)
{
    maybe<int> a(1);
    maybe<int> a2(1);
    maybe<int> b(2);
    maybe<int> e1;
    maybe<int> e2;

    // both present, equal values
    test::record_assertion(_h, a == a2,
        "eq: equal present values compare equal");

    // both present, differing values
    test::record_assertion(_h, !(a == b) && (a != b),
        "eq: differing present values compare unequal");

    // both empty
    test::record_assertion(_h, e1 == e2,
        "eq: two empties compare equal");

    // one present, one empty
    test::record_assertion(_h, !(a == e1) && (a != e1),
        "eq: present vs empty compare unequal");

    // operator!= mirrors operator== for the maybe/maybe overload
    test::record_assertion(_h, !(a != a2) && (e1 != b),
        "eq: operator!= negates operator== for maybe/maybe");

    // nothing_t == maybe : true iff the maybe is empty
    test::record_assertion(_h, (nothing_v == e1),
        "eq: nothing_v == empty is true");
    test::record_assertion(_h, !(nothing_v == a),
        "eq: nothing_v == present is false");

    // maybe == nothing_t : hard-coded false (documented asymmetry)
    test::record_assertion(_h, !(a == nothing_v),
        "eq: present == nothing_v is false");
    test::record_assertion(_h, !(e1 == nothing_v),
        "eq: empty == nothing_v is false (asymmetric, as implemented)");

    return;
}


NS_END  // testing
NS_END  // djinterp
