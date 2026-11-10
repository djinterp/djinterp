/*******************************************************************************
* djinterp [functional]                                  predicate_tests_not.cpp
*
*   Tests for the unary negation combinator (predicate_not_combinator) and
* its factory function (predicate_not).
*
*
* path:      /tests/djinterp/cpp/functional/predicate_tests_not.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.30
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "./predicate_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
test_predicate_not
  Tests the not combinator and its factory.
  Tests the following:
  - negation truth table over a unary predicate
  - the arity-2 operator() overload negates a binary relation
  - the inner() accessor returns the stored (un-negated) predicate
  - double negation round-trips to the original verdict
  - the factory decays its argument
  - bool-convertible (int-returning) inner predicate is negated correctly
  - negation composes with binary combinators (e.g. NOT over an AND)
*/
bool test_predicate_not()
{
    bool verdict = true;


    // ---- basic negation truth table ----
    {
        is_positive pos;

        internal::predicate_not_combinator<is_positive> n(pos);

        D_PREDICATE_CHECK(n(5)  == false);  // !pos(5)
        D_PREDICATE_CHECK(n(-5) == true);   // !pos(-5)
        D_PREDICATE_CHECK(n(0)  == true);   // !pos(0)  (0 is not > 0)
    }

    // ---- factory equivalence ----
    {
        is_positive pos;

        D_PREDICATE_CHECK(predicate_not(pos)(5)  == false);
        D_PREDICATE_CHECK(predicate_not(pos)(-5) == true);
    }

    // ---- arity-2 negation of a binary relation ----
    {
        less_than lt;

        internal::predicate_not_combinator<less_than> n(lt);

        D_PREDICATE_CHECK(n(1, 2) == false);  // !(1 < 2)
        D_PREDICATE_CHECK(n(2, 1) == true);   // !(2 < 1)
        D_PREDICATE_CHECK(n(1, 1) == true);   // !(1 < 1)
    }

    // ---- inner() accessor returns the un-negated predicate ----
    {
        is_positive pos;

        internal::predicate_not_combinator<is_positive> n(pos);

        D_PREDICATE_CHECK(n.inner()(5)  == true);   // pos, not !pos
        D_PREDICATE_CHECK(n.inner()(-5) == false);
    }

    // ---- double negation round-trips ----
    {
        is_positive pos;

        D_PREDICATE_CHECK(predicate_not(predicate_not(pos))(5) == true);
        D_PREDICATE_CHECK(predicate_not(predicate_not(pos))(-5) == false);
    }

    // ---- bool-convertible inner predicate ----
    {
        DReturnsIntPredicate ri;

        internal::predicate_not_combinator<DReturnsIntPredicate> n(ri);

        D_PREDICATE_CHECK(n(5) == false);  // 5 -> true  -> !true
        D_PREDICATE_CHECK(n(0) == true);   // 0 -> false -> !false
    }

    // ---- negation over a binary combinator (NOT (pos AND even)) ----
    {
        is_positive pos;
        is_even     even;

        internal::predicate_and_combinator<is_positive, is_even>
            a(pos, even);
        internal::predicate_not_combinator<
            internal::predicate_and_combinator<is_positive, is_even> > n(a);

        D_PREDICATE_CHECK(n(4)  == false);  // !(pos&&even)
        D_PREDICATE_CHECK(n(3)  == true);   // !(pos&&!even)
        D_PREDICATE_CHECK(n(-2) == true);   // !(!pos&&even)
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // ---- negation evaluates the inner predicate exactly once ----
    {
        int calls = 0;

        counting_predicate inner(&calls, true);

        internal::predicate_not_combinator<counting_predicate> n(inner);

        bool result = n(0);

        D_PREDICATE_CHECK(result == false);  // !true
        D_PREDICATE_CHECK(calls   == 1);
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
