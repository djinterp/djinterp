/******************************************************************************
* djinterp [functional]                            predicate_tests_variadic.cpp
*
*   Tests for the variadic fold factories all_of, any_of, and none_of, and
* the internal fold helpers (all_of_fold / any_of_fold) that compute the
* left-associated combinator type for an arbitrary-arity pack.
*
*   These features are C++11+ only; under C++98 this entry point compiles to
* a no-op pass.
*
* path:      /src/functional/predicate_tests_variadic.cpp
******************************************************************************/

#include "./predicate_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
test_predicate_variadic
  Tests all_of / any_of / none_of.
  Tests the following:
  - single-predicate base case folds to the predicate itself (decayed)
  - two-predicate case matches predicate_and / predicate_or exactly
  - three-or-more-arity folds (the case the helper-struct rewrite fixed)
  - left-to-right short-circuit across the whole chain
  - none_of(p...) == not(any_of(p...)) and == all_of(not p...)
  - result-type folding: all_of of two yields a predicate_and_combinator,
    confirmed via the structural traits
  - mixed predicate types in one pack (functors of distinct types)
*/
std::size_t
test_predicate_variadic(
    test_registry& _reg
)
{
    std::size_t before;

    before = _reg.failures();

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // ---- single-predicate base case (folds to the predicate itself) ----
    {
        D_TESTING_CHECK(_reg, all_of(is_positive())(5)  == true);
        D_TESTING_CHECK(_reg, all_of(is_positive())(-5) == false);
        D_TESTING_CHECK(_reg, any_of(is_even())(2)      == true);
        D_TESTING_CHECK(_reg, any_of(is_even())(3)      == false);
    }

    // ---- two-predicate all_of == predicate_and ----
    {
        is_positive pos;
        is_even     even;

        for (int v = -4; v <= 4; ++v)
        {
            D_TESTING_CHECK(_reg,
                all_of(pos, even)(v) == predicate_and(pos, even)(v));
        }
    }

    // ---- two-predicate any_of == predicate_or ----
    {
        is_positive pos;
        is_even     even;

        for (int v = -4; v <= 4; ++v)
        {
            D_TESTING_CHECK(_reg,
                any_of(pos, even)(v) == predicate_or(pos, even)(v));
        }
    }

    // ---- three-arity all_of: pos AND even AND (not neg) ----
    {
        is_positive pos;
        is_even     even;

        // not_neg is true for v >= 0
        auto combo = all_of(pos, even, predicate_not(is_negative()));

        D_TESTING_CHECK(_reg, combo(4)  == true);   // pos, even, !neg
        D_TESTING_CHECK(_reg, combo(2)  == true);
        D_TESTING_CHECK(_reg, combo(3)  == false);  // !even
        D_TESTING_CHECK(_reg, combo(-2) == false);  // !pos (and neg)
    }

    // ---- four-arity all_of (exercises deeper fold recursion) ----
    {
        auto combo = all_of(always_true(), always_true(),
                            always_true(), is_positive());

        D_TESTING_CHECK(_reg, combo(5)  == true);
        D_TESTING_CHECK(_reg, combo(-5) == false);  // last fails
    }

    // ---- three-arity any_of: neg OR even OR (always_false) ----
    {
        is_even     even;
        is_negative neg;

        auto combo = any_of(neg, even, always_false());

        D_TESTING_CHECK(_reg, combo(-3) == true);   // neg
        D_TESTING_CHECK(_reg, combo(2)  == true);   // even
        D_TESTING_CHECK(_reg, combo(3)  == false);  // none
    }

    // ---- none_of truth table (== !any_of) ----
    {
        is_positive pos;
        is_even     even;

        // none_of(pos, even) true only when neither holds
        D_TESTING_CHECK(_reg, none_of(pos, even)(-3) == true);   // !pos,!even
        D_TESTING_CHECK(_reg, none_of(pos, even)(4)  == false);  // both
        D_TESTING_CHECK(_reg, none_of(pos, even)(3)  == false);  // pos
        D_TESTING_CHECK(_reg, none_of(pos, even)(-2) == false);  // even
    }

    // ---- none_of(p...) == not(any_of(p...)) over a sweep ----
    {
        is_positive pos;
        is_even     even;
        is_negative neg;

        for (int v = -4; v <= 4; ++v)
        {
            bool none = none_of(pos, even, neg)(v);
            bool not_any =
                predicate_not(any_of(pos, even, neg))(v);

            D_TESTING_CHECK(_reg, none == not_any);
        }
    }

    // ---- short-circuit across a 3-chain all_of (false in the middle) ----
    {
        int c1 = 0;
        int c2 = 0;
        int c3 = 0;

        counting_predicate p1(&c1, true);
        counting_predicate p2(&c2, false);  // stops the chain here
        counting_predicate p3(&c3, true);

        bool result = all_of(p1, p2, p3)(0);

        D_TESTING_CHECK(_reg, result == false);
        D_TESTING_CHECK(_reg, c1 == 1);
        D_TESTING_CHECK(_reg, c2 == 1);
        D_TESTING_CHECK(_reg, c3 == 0);  // never reached
    }

    // ---- short-circuit across a 3-chain any_of (true in the middle) ----
    {
        int c1 = 0;
        int c2 = 0;
        int c3 = 0;

        counting_predicate p1(&c1, false);
        counting_predicate p2(&c2, true);   // satisfies the chain here
        counting_predicate p3(&c3, false);

        bool result = any_of(p1, p2, p3)(0);

        D_TESTING_CHECK(_reg, result == true);
        D_TESTING_CHECK(_reg, c1 == 1);
        D_TESTING_CHECK(_reg, c2 == 1);
        D_TESTING_CHECK(_reg, c3 == 0);  // never reached
    }

    // ---- result-type folding: all_of of two is a predicate_and ----
    {
        is_positive pos;
        is_even     even;

        typedef decltype(all_of(pos, even)) all2_type;
        typedef decltype(any_of(pos, even)) any2_type;

        D_TESTING_CHECK(_reg, is_predicate_and<all2_type>::value);
        D_TESTING_CHECK(_reg, is_predicate_or<any2_type>::value);

        // single-arg fold decays to the bare predicate type
        typedef decltype(all_of(pos)) all1_type;
        D_TESTING_CHECK(_reg,
            (internal::is_predicate_and_helper<all1_type>::value == false));
    }
#else
    (void)_reg;  // C++98: variadic folds unavailable; nothing to test here
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    return (_reg.failures() - before);
}


NS_END  // testing
NS_END  // djinterp
