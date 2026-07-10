/******************************************************************************
* djinterp [functional]                            predicate_tests_nand_nor.cpp
*
*   Tests for the NAND and NOR binary combinators (predicate_nand_combinator,
* predicate_nor_combinator) and their factories (predicate_nand,
* predicate_nor).
*
* path:      /src/functional/predicate_tests_nand_nor.cpp
******************************************************************************/

#include "./predicate_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
test_predicate_nand_nor
  Tests the nand / nor combinators and their factories.
  Tests the following:
  - NAND truth table: !(a && b) across all input combinations
  - NOR truth table:  !(a || b) across all input combinations
  - the arity-2 operator() overloads for both
  - De Morgan equivalences hold against the and/or/not combinators:
      nand(p,q)  ==  not(and(p,q))  ==  or(not p, not q)
      nor(p,q)   ==  not(or(p,q))   ==  and(not p, not q)
  - the factories decay their arguments
  - short-circuit behavior of the underlying && / || is preserved
*/
void
test_predicate_nand_nor(
    test::test_handler& _h
)
{

    // ---- NAND truth table: !(pos && even) ----
    {
        is_positive pos;
        is_even     even;

        internal::predicate_nand_combinator<is_positive, is_even>
            n(pos, even);

        D_TEST_CHECK(_h, n(4)  == false);  // !(T&&T)
        D_TEST_CHECK(_h, n(3)  == true);   // !(T&&F)
        D_TEST_CHECK(_h, n(-2) == true);   // !(F&&T)
        D_TEST_CHECK(_h, n(-3) == true);   // !(F&&F)
    }

    // ---- NOR truth table: !(pos || even) ----
    {
        is_positive pos;
        is_even     even;

        internal::predicate_nor_combinator<is_positive, is_even>
            n(pos, even);

        D_TEST_CHECK(_h, n(4)  == false);  // !(T||T)
        D_TEST_CHECK(_h, n(3)  == false);  // !(T||F)
        D_TEST_CHECK(_h, n(-2) == false);  // !(F||T)
        D_TEST_CHECK(_h, n(-3) == true);   // !(F||F)
    }

    // ---- factory equivalence ----
    {
        is_positive pos;
        is_even     even;

        D_TEST_CHECK(_h, predicate_nand(pos, even)(4)  == false);
        D_TEST_CHECK(_h, predicate_nand(pos, even)(3)  == true);
        D_TEST_CHECK(_h, predicate_nor(pos, even)(-3)  == true);
        D_TEST_CHECK(_h, predicate_nor(pos, even)(4)   == false);
    }

    // ---- arity-2 overloads ----
    {
        less_than lt;

        internal::predicate_nand_combinator<less_than, less_than>
            nand2(lt, lt);
        D_TEST_CHECK(_h, nand2(1, 2) == false);  // !(T&&T)
        D_TEST_CHECK(_h, nand2(2, 1) == true);   // !(F&&F)

        internal::predicate_nor_combinator<less_than, less_than>
            nor2(lt, lt);
        D_TEST_CHECK(_h, nor2(1, 2) == false);   // !(T||T)
        D_TEST_CHECK(_h, nor2(2, 1) == true);    // !(F||F)
    }

    // ---- De Morgan: nand(p,q) == not(and(p,q)) over a sweep ----
    {
        is_positive pos;
        is_even     even;

        for (int v = -4; v <= 4; ++v)
        {
            bool nand_result = predicate_nand(pos, even)(v);
            bool not_and     = predicate_not(predicate_and(pos, even))(v);

            D_TEST_CHECK(_h, nand_result == not_and);
        }
    }

    // ---- De Morgan: nor(p,q) == not(or(p,q)) over a sweep ----
    {
        is_positive pos;
        is_even     even;

        for (int v = -4; v <= 4; ++v)
        {
            bool nor_result = predicate_nor(pos, even)(v);
            bool not_or     = predicate_not(predicate_or(pos, even))(v);

            D_TEST_CHECK(_h, nor_result == not_or);
        }
    }

    // ---- De Morgan: nor(p,q) == and(not p, not q) over a sweep ----
    {
        is_positive pos;
        is_even     even;

        for (int v = -4; v <= 4; ++v)
        {
            bool nor_result = predicate_nor(pos, even)(v);
            bool and_nots   =
                predicate_and(predicate_not(pos), predicate_not(even))(v);

            D_TEST_CHECK(_h, nor_result == and_nots);
        }
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // ---- NAND preserves AND short-circuit: false-left skips right ----
    {
        int left_calls  = 0;
        int right_calls = 0;

        counting_predicate left(&left_calls, false);
        counting_predicate right(&right_calls, true);

        internal::predicate_nand_combinator<counting_predicate,
                                            counting_predicate> n(left, right);

        bool result = n(0);

        D_TEST_CHECK(_h, result == true);    // !(F&&?) -> true
        D_TEST_CHECK(_h, left_calls  == 1);
        D_TEST_CHECK(_h, right_calls == 0);  // short-circuited
    }

    // ---- NOR preserves OR short-circuit: true-left skips right ----
    {
        int left_calls  = 0;
        int right_calls = 0;

        counting_predicate left(&left_calls, true);
        counting_predicate right(&right_calls, false);

        internal::predicate_nor_combinator<counting_predicate,
                                           counting_predicate> n(left, right);

        bool result = n(0);

        D_TEST_CHECK(_h, result == false);   // !(T||?) -> false
        D_TEST_CHECK(_h, left_calls  == 1);
        D_TEST_CHECK(_h, right_calls == 0);  // short-circuited
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    return;
}


NS_END  // testing
NS_END  // djinterp
