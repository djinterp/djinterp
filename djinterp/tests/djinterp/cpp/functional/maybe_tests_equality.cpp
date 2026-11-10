/*******************************************************************************
* djinterp [test]                                       maybe_tests_equality.cpp
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
*
*
* path:      /tests/djinterp/cpp/functional/maybe_tests_equality.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.01
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./maybe_tests.hpp"


NS_DJINTERP
NS_TESTING


bool test_equality()
{
    bool verdict = true;

    maybe<int> a(1);
    maybe<int> a2(1);
    maybe<int> b(2);
    maybe<int> e1;
    maybe<int> e2;

    // both present, equal values
    record_assertion(verdict, a == a2,
        "eq: equal present values compare equal");

    // both present, differing values
    record_assertion(verdict, !(a == b) && (a != b),
        "eq: differing present values compare unequal");

    // both empty
    record_assertion(verdict, e1 == e2,
        "eq: two empties compare equal");

    // one present, one empty
    record_assertion(verdict, !(a == e1) && (a != e1),
        "eq: present vs empty compare unequal");

    // operator!= mirrors operator== for the maybe/maybe overload
    record_assertion(verdict, !(a != a2) && (e1 != b),
        "eq: operator!= negates operator== for maybe/maybe");

    // nothing_t == maybe : true iff the maybe is empty
    record_assertion(verdict, (nothing_v == e1),
        "eq: nothing_v == empty is true");
    record_assertion(verdict, !(nothing_v == a),
        "eq: nothing_v == present is false");

    // maybe == nothing_t : hard-coded false (documented asymmetry)
    record_assertion(verdict, !(a == nothing_v),
        "eq: present == nothing_v is false");
    record_assertion(verdict, !(e1 == nothing_v),
        "eq: empty == nothing_v is false (asymmetric, as implemented)");

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
