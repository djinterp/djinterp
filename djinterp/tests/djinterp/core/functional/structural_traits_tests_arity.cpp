// djinterp [test]  structural_traits_tests_arity.cpp
//   Section II -- the arity trilogy: is_nullary_callable, is_unary_callable,
//   is_binary_callable.

// std
#include <string>
#include <type_traits>
// djinterp
#include "structural_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_is_nullary_callable_positive
  A nullary callable -- the "next" step that drives a pull-based source -- is
  detected.
  Tests the following:
  - a const nullary, a stateful (non-const) nullary, and a lambda
*/
bool
tests_is_nullary_callable_positive()
{
    bool ok = true;

    ok = ok && (is_nullary_callable<nil_const>::value);
    ok = ok && (is_nullary_callable<nil_mut>::value);
    ok = ok && (is_nullary_callable<src_pull>::value);

    int  captured = 3;
    auto lam = [captured](){ return captured; };
    ok = ok && (is_nullary_callable<decltype(lam)>::value);

    return ok;
}


/*
tests_is_nullary_callable_negative
  Rejected cleanly (SFINAE, not a hard error).
  Tests the following:
  - a type with no operator(); a scalar
  - a callable of the wrong arity
*/
bool
tests_is_nullary_callable_negative()
{
    bool ok = true;

    ok = ok && (!is_nullary_callable<not_callable>::value);
    ok = ok && (!is_nullary_callable<int>::value);
    ok = ok && (!is_nullary_callable<std::string>::value);

    ok = ok && (!is_nullary_callable<const_step>::value);     // unary
    ok = ok && (!is_nullary_callable<sum_reducer>::value);    // binary

    return ok;
}


/*
tests_is_unary_callable_positive
  A unary callable -- the transform and predicate shape -- is detected.
  Tests the following:
  - a const step, a stateful step, a lambda, a templated operator()
*/
bool
tests_is_unary_callable_positive()
{
    bool ok = true;

    ok = ok && (is_unary_callable<const_step, int>::value);
    ok = ok && (is_unary_callable<mut_step, int>::value);
    ok = ok && (is_unary_callable<generic_step, int>::value);
    ok = ok && (is_unary_callable<generic_step, std::string>::value);

    auto lam = [](int _x){ return _x + 1; };
    ok = ok && (is_unary_callable<decltype(lam), int>::value);

    return ok;
}


/*
tests_is_unary_callable_negative
  Tests the following:
  - not callable at all; the wrong arity in both directions
  - an argument the callable cannot take
*/
bool
tests_is_unary_callable_negative()
{
    bool ok = true;

    ok = ok && (!is_unary_callable<not_callable, int>::value);
    ok = ok && (!is_unary_callable<int, int>::value);

    ok = ok && (!is_unary_callable<nil_const, int>::value);      // nullary
    ok = ok && (!is_unary_callable<sum_reducer, int>::value);    // binary

    ok = ok && (!is_unary_callable<const_step, std::string>::value);

    return ok;
}


/*
tests_is_binary_callable_positive
  A binary callable -- the reducer step (acc, x) -> acc -- is detected. This is
  the structural floor the Reducer concept builds on.
  Tests the following:
  - a pure reducer and a STATEFUL (non-const) one, as an accumulating reducer
    must be
  - a lambda, and mixed argument types
*/
bool
tests_is_binary_callable_positive()
{
    bool ok = true;

    ok = ok && (is_binary_callable<sum_reducer, int, int>::value);
    ok = ok && (is_binary_callable<tally_reducer, int, int>::value);
    ok = ok && (is_binary_callable<void_reducer, int, int>::value);

    auto lam = [](int _a, int _b){ return _a * _b; };
    ok = ok && (is_binary_callable<decltype(lam), int, int>::value);

    return ok;
}


/*
tests_is_binary_callable_negative
  Tests the following:
  - not callable; the wrong arity in both directions
  - arguments the reducer cannot take
*/
bool
tests_is_binary_callable_negative()
{
    bool ok = true;

    ok = ok && (!is_binary_callable<not_callable, int, int>::value);
    ok = ok && (!is_binary_callable<int, int, int>::value);

    ok = ok && (!is_binary_callable<const_step, int, int>::value);   // unary
    ok = ok && (!is_binary_callable<nil_const, int, int>::value);    // nullary

    ok = ok && (!is_binary_callable<sum_reducer, std::string,
                                    std::string>::value);

    return ok;
}


/*
tests_arity_mutable_lvalue_contract
  The defining contract of all three: the probe is declval<_Type&>() -- a MUTABLE
  LVALUE. Four fixtures determine that expression uniquely and admit no other
  reading:
      const operator()   accepted  -> a const callable is fine on an lvalue
      NON-const one      accepted  -> so the probe is NOT const _Type&
      &-qualified one    accepted  -> so the probe IS an lvalue
      &&-qualified one   REJECTED  -> so the probe is NOT _Type&&
  The mutable half is load-bearing: a pull source advances on every call and a
  tallying reducer accumulates, so both need a non-const operator(). (Note this
  is the opposite of functional_traits' is_callable, which probes a const lvalue,
  and of has_find_method above.)
  Tests the following:
  - each of the four, at arity zero and at arity one
  - the mutating fixtures really do mutate when called
*/
bool
tests_arity_mutable_lvalue_contract()
{
    bool ok = true;

    // nullary.
    ok = ok && (is_nullary_callable<nil_const>::value);   // const:       yes
    ok = ok && (is_nullary_callable<nil_mut>::value);     // non-const:   yes
    ok = ok && (is_nullary_callable<nil_lref>::value);    // &-qualified: yes
    ok = ok && (!is_nullary_callable<nil_rref>::value);   // &&-qualified: NO

    // unary.
    ok = ok && (is_unary_callable<const_step, int>::value);
    ok = ok && (is_unary_callable<mut_step, int>::value);
    ok = ok && (is_unary_callable<lref_step, int>::value);
    ok = ok && (!is_unary_callable<rref_step, int>::value);

    // binary, with a stateful reducer.
    ok = ok && (is_binary_callable<tally_reducer, int, int>::value);

    // the mutating fixtures are genuinely mutating.
    mut_step m;
    ok = ok && (m(5) == 10);
    ok = ok && (m.calls == 1);

    tally_reducer t;
    ok = ok && (t(0, 4) == 4);
    ok = ok && (t.seen == 1);

    return ok;
}


/*
tests_arity_argument_decay
  clean_t is applied to _Arg, so every cv-ref spelling of an argument collapses to
  the same query -- and the probe then offers an RVALUE. Two consequences, both
  pinned here: the spellings are interchangeable, and a step taking a mutable
  reference is therefore undetectable, since an rvalue will not bind to int&.
  Tests the following:
  - int, int&, const int&, and int&& all give the same answer
  - a callable taking int& is NOT detected, for any spelling of the argument,
    though it plainly is callable on an int&
*/
bool
tests_arity_argument_decay()
{
    bool ok = true;

    // all four spellings collapse to the same query.
    ok = ok && (is_unary_callable<const_step, int>::value);
    ok = ok && (is_unary_callable<const_step, int&>::value);
    ok = ok && (is_unary_callable<const_step, const int&>::value);
    ok = ok && (is_unary_callable<const_step, int&&>::value);

    // the by-reference step: undetectable, whichever way the argument is spelt.
    ok = ok && (!is_unary_callable<in_place, int>::value);
    ok = ok && (!is_unary_callable<in_place, int&>::value);
    ok = ok && (!is_unary_callable<in_place, const int&>::value);

    // ...yet it is plainly callable on an int&.
    int      n = 21;
    in_place f;
    f(n);
    ok = ok && (n == 42);

    // the same decay applies to the binary form's arguments.
    ok = ok && (is_binary_callable<sum_reducer, const int&, int&>::value);

    return ok;
}


/*
tests_arity_type_decay
  clean_t is also applied to _Type, so cv-ref spellings of the CALLABLE collapse
  too. A consequence worth pinning: the const is stripped before the probe, so a
  const-qualified stateful source still reads as callable -- the trait answers
  about the underlying type, not about the const you asked with.
  Tests the following:
  - const / reference / rvalue spellings of the callable all agree
  - is_nullary_callable<const src_pull> is TRUE, though a const src_pull could
    not in fact be called
*/
bool
tests_arity_type_decay()
{
    bool ok = true;

    ok = ok && (is_unary_callable<const_step&, int>::value);
    ok = ok && (is_unary_callable<const const_step&, int>::value);
    ok = ok && (is_unary_callable<const_step&&, int>::value);

    // the const is stripped: a const stateful source still reads as callable.
    ok = ok && (is_nullary_callable<src_pull>::value);
    ok = ok && (is_nullary_callable<const src_pull>::value);
    ok = ok && (is_nullary_callable<const src_pull&>::value);

    // a negative stays negative through the same decay.
    ok = ok && (!is_nullary_callable<const not_callable&>::value);

    return ok;
}


/*
tests_arity_trilogy_exclusive
  The three traits partition by arity: each accepts its own and refuses the
  others, so the source / transform / reducer roles cannot be confused.
  Tests the following:
  - a nullary, a unary, and a binary callable are each detected by exactly one
*/
bool
tests_arity_trilogy_exclusive()
{
    bool ok = true;

    // nullary.
    ok = ok && (is_nullary_callable<nil_const>::value);
    ok = ok && (!is_unary_callable<nil_const, int>::value);
    ok = ok && (!is_binary_callable<nil_const, int, int>::value);

    // unary.
    ok = ok && (!is_nullary_callable<const_step>::value);
    ok = ok && (is_unary_callable<const_step, int>::value);
    ok = ok && (!is_binary_callable<const_step, int, int>::value);

    // binary.
    ok = ok && (!is_nullary_callable<sum_reducer>::value);
    ok = ok && (!is_unary_callable<sum_reducer, int>::value);
    ok = ok && (is_binary_callable<sum_reducer, int, int>::value);

    // a templated operator() answers at whatever arity it accepts -- here, one.
    ok = ok && (is_unary_callable<generic_step, int>::value);
    ok = ok && (!is_nullary_callable<generic_step>::value);
    ok = ok && (!is_binary_callable<generic_step, int, int>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
