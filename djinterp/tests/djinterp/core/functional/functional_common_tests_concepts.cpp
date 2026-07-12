// djinterp [test]  functional_traits_tests_concepts.cpp
//   Section IV -- the C++20 concept faces (Callable / Predicate).

// std
#include <string>
#include <type_traits>
#include <vector>
// djinterp
#include "functional_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_concept_callable_mirrors_trait
  Callable is the concept face of is_callable -- the same answer, in concept
  syntax.
  Tests the following:
  - the two agree across positives, negatives, the const-lvalue edge, the
    templated operator(), and every arity
*/
bool
tests_concept_callable_mirrors_trait()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(Callable<const_fn, int> == is_callable<const_fn, int>::value,
                  "mirrors, positive");
    static_assert(Callable<not_callable, int> ==
                  is_callable<not_callable, int>::value, "mirrors, negative");
    static_assert(Callable<nonconst_fn, int> ==
                  is_callable<nonconst_fn, int>::value, "mirrors, const edge");

    static_assert(Callable<const_fn, int>, "const functor");
    static_assert(Callable<generic_fn, std::string>, "templated operator()");
    static_assert(Callable<niladic_fn>, "zero args");
    static_assert(Callable<three_arg_fn, int, int, int>, "three args");

    static_assert(!Callable<not_callable, int>, "no operator()");
    static_assert(!Callable<nonconst_fn, int>, "not const-callable");
    static_assert(!Callable<two_arg_fn, int>, "wrong arity");

    ok = ok && (Callable<const_fn, int>);
    ok = ok && (!Callable<not_callable, int>);
#endif

    return ok;
}


/*
tests_concept_predicate_mirrors_trait
  Predicate is the concept face of is_predicate.
  Tests the following:
  - the two agree, including on the explicit-operator-bool edge
*/
bool
tests_concept_predicate_mirrors_trait()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(Predicate<pred_bool, const int&> ==
                  is_predicate<pred_bool, const int&>::value, "mirrors");
    static_assert(Predicate<pred_explicit, int> ==
                  is_predicate<pred_explicit, int>::value,
                  "mirrors, explicit-bool edge");

    static_assert(Predicate<pred_bool, const int&>, "bool result");
    static_assert(Predicate<pred_int, int>, "int result converts");
    static_assert(Predicate<generic_pred, double>, "templated predicate");

    static_assert(!Predicate<pred_str, int>, "string result");
    static_assert(!Predicate<void_fn, int>, "void result");
    static_assert(!Predicate<pred_explicit, int>, "explicit operator bool");
    static_assert(!Predicate<not_callable, int>, "not callable");

    ok = ok && (Predicate<pred_int, int>);
    ok = ok && (!Predicate<pred_str, int>);
#endif

    return ok;
}


/*
tests_concept_constrained_parameter
  Predicate is usable as a CONSTRAINED TEMPLATE PARAMETER -- partially applied to
  its argument type, as the header's usage shows: template<Predicate<const int&>
  P>.
  Tests the following:
  - the constrained template accepts a matching predicate and computes correctly
  - it accepts both a lambda and a functor
*/
bool
tests_concept_constrained_parameter()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    std::vector<int> v;
    v.push_back(-1);
    v.push_back(2);
    v.push_back(3);
    v.push_back(-4);
    v.push_back(5);

    auto positive = [](const int& _x){ return _x > 0; };
    ok = ok && (keep_if(v, positive) == 3);

    ok = ok && (keep_if(v, pred_bool()) == 3);
    ok = ok && (keep_if(v, generic_pred()) == 5);   // every non-zero int
#endif

    return ok;
}


/*
tests_concept_requires_clause
  Callable is usable in a REQUIRES-CLAUSE, as the header's usage shows:
  requires Callable<F, const T&>.
  Tests the following:
  - the constrained function applies a matching callable
  - it works for a functor, a lambda, and a templated operator()
*/
bool
tests_concept_requires_clause()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    ok = ok && (apply_to(5, const_fn()) == 10);

    auto plus_one = [](const int& _x){ return _x + 1; };
    ok = ok && (apply_to(5, plus_one) == 6);

    ok = ok && (apply_to(std::string("hi"), generic_fn()) == std::string("hi"));
#endif

    return ok;
}


/*
tests_concept_overload_gating
  The concepts really GATE overload resolution -- they are constraints, not just
  bools that happen to be true.
  Tests the following:
  - a constrained overload wins for a matching argument
  - it is excluded (and the fallback chosen) for a non-matching one, including
    the explicit-bool near-miss
*/
bool
tests_concept_overload_gating()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    // Predicate-constrained overload.
    ok = ok && (which_pred(pred_int()) == 1);
    ok = ok && (which_pred(pred_bool()) == 1);
    ok = ok && (which_pred(pred_str()) == 0);        // callable, not a predicate
    ok = ok && (which_pred(pred_explicit()) == 0);   // explicit-bool near-miss
    ok = ok && (which_pred(not_callable()) == 0);
    ok = ok && (which_pred(42) == 0);

    // Callable-constrained overload.
    ok = ok && (which_callable(const_fn()) == 1);
    ok = ok && (which_callable(pred_str()) == 1);    // callable, though no bool
    ok = ok && (which_callable(nonconst_fn()) == 0); // not const-callable
    ok = ok && (which_callable(not_callable()) == 0);
#endif

    return ok;
}


/*
tests_concept_callable_but_not_predicate
  The two faces are distinct: every predicate is callable, but not every callable
  is a predicate -- Predicate is the strictly stronger constraint.
  Tests the following:
  - a string-returning and a void-returning callable satisfy Callable but not
    Predicate
  - every predicate fixture satisfies both
*/
bool
tests_concept_callable_but_not_predicate()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    static_assert(Callable<pred_str, int> && !Predicate<pred_str, int>,
                  "callable, not a predicate");
    static_assert(Callable<void_fn, int> && !Predicate<void_fn, int>,
                  "void: callable, not a predicate");
    static_assert(Callable<pred_explicit, int> && !Predicate<pred_explicit, int>,
                  "explicit bool: callable, not a predicate");

    static_assert(Callable<pred_int, int> && Predicate<pred_int, int>,
                  "a predicate is also callable");
    static_assert(Callable<pred_bool, const int&> &&
                  Predicate<pred_bool, const int&>, "and so is this one");

    ok = ok && (Callable<pred_str, int> && !Predicate<pred_str, int>);
#endif

    return ok;
}


/*
tests_concepts_gating
  The concept faces exist exactly where the standard allows them: gated on
  concepts (C++20), with the traits as the always-present floor beneath.
  Tests the following:
  - the gate agrees with the language level
  - the traits answer identically whether or not the gate is open
*/
bool
tests_concepts_gating()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    // gate open: the faces must be usable and must agree with the traits.
    ok = ok && (Callable<const_fn, int> == is_callable<const_fn, int>::value);
    ok = ok && (Predicate<pred_int, int> == is_predicate<pred_int, int>::value);
    #if !D_ENV_LANG_IS_CPP20_OR_HIGHER
        ok = false;         // faces present on a standard that lacks concepts
    #endif
#else
    // gate closed: this must be a pre-C++20 build.
    #if D_ENV_LANG_IS_CPP20_OR_HIGHER
        ok = false;         // faces missing on a standard that has concepts
    #endif
#endif

    // the traits are the floor: available on every standard.
    ok = ok && (is_callable<const_fn, int>::value);
    ok = ok && (is_predicate<pred_int, int>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
