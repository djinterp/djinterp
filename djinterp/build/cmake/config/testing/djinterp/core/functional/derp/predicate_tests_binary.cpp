/******************************************************************************
* djinterp [functional]                              predicate_tests_binary.cpp
*
*   Tests for the binary predicate combinators (AND, OR, XOR) and their
* factory functions (predicate_and, predicate_or, predicate_xor).
*
* path:      /src/functional/predicate_tests_binary.cpp
******************************************************************************/

#include "./predicate_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
test_predicate_binary
  Tests the and / or / xor combinators and their factories.
  Tests the following:
  - truth tables for AND, OR, XOR across all input combinations
  - the arity-2 (binary relation) operator() overloads
  - left-to-right short-circuit evaluation for AND (second skipped on
    false-left) and OR (second skipped on true-left)
  - XOR evaluates both operands unconditionally (no short-circuit)
  - the first()/second() introspection accessors return the stored predicates
  - factories decay their arguments (cv / ref stripped in the result type)
  - bool-convertible (non-bool) predicate results are coerced correctly
  - combinators are nestable (a combinator used as an operand of another)
*/
void
test_predicate_binary(
    test::test_handler& _h
)
{

    // ---- AND truth table (is_positive AND is_even over selected ints) ----
    {
        is_positive pos;
        is_even     even;

        internal::predicate_and_combinator<is_positive, is_even> a(pos, even);

        D_TEST_CHECK(_h, a(4)  == true);   // pos && even
        D_TEST_CHECK(_h, a(3)  == false);  // pos && !even
        D_TEST_CHECK(_h, a(-2) == false);  // !pos && even
        D_TEST_CHECK(_h, a(-3) == false);  // !pos && !even
    }

    // ---- OR truth table ----
    {
        is_positive pos;
        is_even     even;

        internal::predicate_or_combinator<is_positive, is_even> o(pos, even);

        D_TEST_CHECK(_h, o(4)  == true);   // pos || even
        D_TEST_CHECK(_h, o(3)  == true);   // pos || !even
        D_TEST_CHECK(_h, o(-2) == true);   // !pos || even
        D_TEST_CHECK(_h, o(-3) == false);  // !pos || !even
    }

    // ---- XOR truth table ----
    {
        is_positive pos;
        is_even     even;

        internal::predicate_xor_combinator<is_positive, is_even> x(pos, even);

        D_TEST_CHECK(_h, x(4)  == false);  // pos == even  -> false
        D_TEST_CHECK(_h, x(3)  == true);   // pos != even  -> true
        D_TEST_CHECK(_h, x(-2) == true);   // !pos != even -> true
        D_TEST_CHECK(_h, x(-3) == false);  // !pos == !even -> false
    }

    // ---- factory functions produce equivalent results ----
    {
        is_positive pos;
        is_even     even;

        D_TEST_CHECK(_h, predicate_and(pos, even)(4) == true);
        D_TEST_CHECK(_h, predicate_and(pos, even)(3) == false);
        D_TEST_CHECK(_h, predicate_or(pos, even)(-2) == true);
        D_TEST_CHECK(_h, predicate_or(pos, even)(-3) == false);
        D_TEST_CHECK(_h, predicate_xor(pos, even)(3) == true);
        D_TEST_CHECK(_h, predicate_xor(pos, even)(4) == false);
    }

    // ---- arity-2 (binary relation) operator() overloads ----
    {
        less_than lt;

        // AND of two identical relations is just the relation
        internal::predicate_and_combinator<less_than, less_than> a2(lt, lt);
        D_TEST_CHECK(_h, a2(1, 2) == true);
        D_TEST_CHECK(_h, a2(2, 1) == false);

        internal::predicate_or_combinator<less_than, less_than> o2(lt, lt);
        D_TEST_CHECK(_h, o2(1, 2) == true);
        D_TEST_CHECK(_h, o2(2, 1) == false);

        internal::predicate_xor_combinator<less_than, less_than> x2(lt, lt);
        // identical relations -> equal verdicts -> XOR false
        D_TEST_CHECK(_h, x2(1, 2) == false);
        D_TEST_CHECK(_h, x2(2, 1) == false);
    }

    // ---- introspection accessors ----
    {
        is_positive pos;
        is_even     even;

        internal::predicate_and_combinator<is_positive, is_even> a(pos, even);

        // first()/second() return the stored predicates; verify by invoking
        D_TEST_CHECK(_h, a.first()(5)  == true);
        D_TEST_CHECK(_h, a.first()(-5) == false);
        D_TEST_CHECK(_h, a.second()(2) == true);
        D_TEST_CHECK(_h, a.second()(3) == false);
    }

    // ---- bool-convertible (int-returning) predicate is coerced ----
    {
        returns_int_predicate ri;
        is_positive           pos;

        // ri(x) is x (int); under AND it is coerced to bool
        internal::predicate_and_combinator<returns_int_predicate, is_positive>
            a(ri, pos);

        D_TEST_CHECK(_h, a(5)  == true);   // 5 -> true,  pos true
        D_TEST_CHECK(_h, a(0)  == false);  // 0 -> false
        D_TEST_CHECK(_h, a(-5) == false);  // -5 -> true(int), pos false
    }

    // ---- nesting: combinator as an operand of another combinator ----
    {
        is_positive pos;
        is_even     even;
        is_negative neg;

        // (pos AND even) OR neg
        internal::predicate_and_combinator<is_positive, is_even>
            inner(pos, even);
        internal::predicate_or_combinator<
            internal::predicate_and_combinator<is_positive, is_even>,
            is_negative> outer(inner, neg);

        D_TEST_CHECK(_h, outer(4)  == true);   // pos&&even
        D_TEST_CHECK(_h, outer(-3) == true);   // neg
        D_TEST_CHECK(_h, outer(3)  == false);  // !even, !neg
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // ---- short-circuit: AND must not evaluate the second on false-left ----
    {
        int left_calls  = 0;
        int right_calls = 0;

        counting_predicate left(&left_calls, false);   // left is false
        counting_predicate right(&right_calls, true);

        internal::predicate_and_combinator<counting_predicate,
                                           counting_predicate> a(left, right);

        bool result = a(0);

        D_TEST_CHECK(_h, result == false);
        D_TEST_CHECK(_h, left_calls  == 1);  // left evaluated
        D_TEST_CHECK(_h, right_calls == 0);  // right short-circuited
    }

    // ---- short-circuit: OR must not evaluate the second on true-left ----
    {
        int left_calls  = 0;
        int right_calls = 0;

        counting_predicate left(&left_calls, true);    // left is true
        counting_predicate right(&right_calls, false);

        internal::predicate_or_combinator<counting_predicate,
                                          counting_predicate> o(left, right);

        bool result = o(0);

        D_TEST_CHECK(_h, result == true);
        D_TEST_CHECK(_h, left_calls  == 1);
        D_TEST_CHECK(_h, right_calls == 0);  // short-circuited
    }

    // ---- XOR evaluates BOTH operands (no short-circuit) ----
    {
        int left_calls  = 0;
        int right_calls = 0;

        counting_predicate left(&left_calls, true);
        counting_predicate right(&right_calls, true);

        internal::predicate_xor_combinator<counting_predicate,
                                           counting_predicate> x(left, right);

        bool result = x(0);

        D_TEST_CHECK(_h, result == false);   // true != true -> false
        D_TEST_CHECK(_h, left_calls  == 1);
        D_TEST_CHECK(_h, right_calls == 1);  // both evaluated
    }

    // ---- AND that does reach the second operand evaluates it once ----
    {
        int left_calls  = 0;
        int right_calls = 0;

        counting_predicate left(&left_calls, true);    // left true -> proceed
        counting_predicate right(&right_calls, true);

        internal::predicate_and_combinator<counting_predicate,
                                           counting_predicate> a(left, right);

        bool result = a(0);

        D_TEST_CHECK(_h, result == true);
        D_TEST_CHECK(_h, left_calls  == 1);
        D_TEST_CHECK(_h, right_calls == 1);
    }
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    return;
}


NS_END  // testing
NS_END  // djinterp
