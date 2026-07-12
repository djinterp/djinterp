// djinterp [test]  functional_traits_tests_predicate.cpp
//   Section II -- is_predicate.

// std
#include <string>
#include <type_traits>
// djinterp
#include "functional_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_is_predicate_positive
  The predicate shapes the combinators accept (filter, take_while, partition,
  ...) are recognised.
  Tests the following:
  - a bool-returning functor, a bool-returning lambda, and a function pointer
*/
bool
tests_is_predicate_positive()
{
    bool ok = true;

    ok = ok && (is_predicate<pred_bool, const int&>::value);

    auto lam = [](const int& _x){ return _x > 0; };
    ok = ok && (is_predicate<decltype(lam), const int&>::value);

    ok = ok && (is_predicate<bool(*)(int), int>::value);
    ok = ok && (is_predicate<decltype(&free_pred), int>::value);

    ok = ok && (is_predicate<str_pred, const std::string&>::value);

    return ok;
}


/*
tests_is_predicate_negative
  A predicate must be BOTH callable and bool-convertible; failing either is not a
  predicate.
  Tests the following:
  - a callable whose result does not convert to bool (std::string)
  - a callable that yields void
  - something that is not callable at all
*/
bool
tests_is_predicate_negative()
{
    bool ok = true;

    ok = ok && (!is_predicate<pred_str, int>::value);      // string result
    ok = ok && (!is_predicate<void_fn, int>::value);       // void result
    ok = ok && (!is_predicate<not_callable, int>::value);  // not callable
    ok = ok && (!is_predicate<int, int>::value);           // not callable

    // but pred_str IS callable -- it fails only the bool-convertibility half.
    ok = ok && (is_callable<pred_str, int>::value);
    ok = ok && (is_callable<void_fn, int>::value);

    return ok;
}


/*
tests_is_predicate_convertible_results
  The requirement is convertibility to bool, not a bool result -- so the ordinary
  standard conversions qualify.
  Tests the following:
  - int, pointer, and a class with an implicit operator bool all qualify
  - the results are not literally bool (the point of the test)
*/
bool
tests_is_predicate_convertible_results()
{
    bool ok = true;

    ok = ok && (is_predicate<pred_int, int>::value);        // int -> bool
    ok = ok && (is_predicate<pred_ptr, int>::value);        // pointer -> bool
    ok = ok && (is_predicate<pred_implicit, int>::value);   // operator bool

    static_assert(!std::is_same<callable_result_t<pred_int, int>, bool>::value,
                  "result is int, not bool");
    static_assert(!std::is_same<callable_result_t<pred_ptr, int>, bool>::value,
                  "result is a pointer, not bool");

    return ok;
}


/*
tests_is_predicate_explicit_operator_bool
  The conversion must be IMPLICIT: a result whose operator bool is explicit is
  not convertible in the trait's sense, so such a callable is not a predicate --
  even though `if (p(x))` would compile via contextual conversion.
  Tests the following:
  - an explicit operator bool result is rejected
  - the implicit counterpart is accepted (the contrast)
  - both are callable; only the conversion differs
*/
bool
tests_is_predicate_explicit_operator_bool()
{
    bool ok = true;

    ok = ok && (!is_predicate<pred_explicit, int>::value);
    ok = ok && (is_predicate<pred_implicit, int>::value);

    // both are perfectly callable; the difference is the conversion.
    ok = ok && (is_callable<pred_explicit, int>::value);
    ok = ok && (is_callable<pred_implicit, int>::value);

    static_assert(!std::is_convertible<explicit_bool, bool>::value,
                  "explicit_bool is not implicitly convertible");
    static_assert(std::is_convertible<implicit_bool, bool>::value,
                  "implicit_bool is");

    return ok;
}


/*
tests_is_predicate_const_lvalue
  is_predicate inherits the const-lvalue call contract from the call detection it
  is built on.
  Tests the following:
  - a non-const operator() is not a predicate, even returning bool
  - an &&-qualified one is not either
*/
bool
tests_is_predicate_const_lvalue()
{
    bool ok = true;

    ok = ok && (!is_predicate<nonconst_fn, int>::value);
    ok = ok && (!is_predicate<rvalue_fn, int>::value);

    ok = ok && (is_predicate<pred_int, int>::value);

    return ok;
}


/*
tests_is_predicate_arg_type
  A predicate is a predicate OVER an argument type -- the same callable can be a
  predicate for one _Arg and not for another.
  Tests the following:
  - a string predicate is one over std::string, not over int
  - an int predicate is one over int, int&, and const int&
*/
bool
tests_is_predicate_arg_type()
{
    bool ok = true;

    ok = ok && (is_predicate<str_pred, const std::string&>::value);
    ok = ok && (is_predicate<str_pred, std::string>::value);
    ok = ok && (!is_predicate<str_pred, int>::value);

    ok = ok && (is_predicate<pred_bool, const int&>::value);
    ok = ok && (is_predicate<pred_bool, int&>::value);
    ok = ok && (is_predicate<pred_bool, int>::value);
    ok = ok && (!is_predicate<pred_bool, std::string>::value);

    return ok;
}


/*
tests_is_predicate_templated_operator
  Like is_callable, is_predicate probes an expression, so a templated operator()
  is a predicate at every argument type it accepts with a bool-convertible
  result.
  Tests the following:
  - a generic predicate qualifies over int, bool, and a pointer
  - a generic non-predicate (identity, returning its argument) qualifies only
    where the result happens to convert to bool
*/
bool
tests_is_predicate_templated_operator()
{
    bool ok = true;

    ok = ok && (is_predicate<generic_pred, int>::value);
    ok = ok && (is_predicate<generic_pred, bool>::value);
    ok = ok && (is_predicate<generic_pred, double>::value);

    // generic_fn returns its argument: a predicate over int (int -> bool),
    // but not over std::string (no conversion to bool).
    ok = ok && (is_predicate<generic_fn, int>::value);
    ok = ok && (!is_predicate<generic_fn, std::string>::value);
    ok = ok && (is_callable<generic_fn, std::string>::value);

    return ok;
}


/*
tests_is_predicate_single_argument
  is_predicate takes exactly one _Arg -- it is the one-argument, bool-convertible
  case, not a variadic trait.
  Tests the following:
  - a two-argument callable is not a predicate over one argument
  - a niladic callable is not a predicate over one argument
*/
bool
tests_is_predicate_single_argument()
{
    bool ok = true;

    ok = ok && (!is_predicate<two_arg_fn, int>::value);
    ok = ok && (!is_predicate<niladic_fn, int>::value);

    // they are callable at their own arity -- is_callable takes the pack.
    ok = ok && (is_callable<two_arg_fn, int, double>::value);
    ok = ok && (is_callable<niladic_fn>::value);

    return ok;
}


/*
tests_is_predicate_reuses_invocable_r_with
  is_predicate is defined as the bool case of function_traits.hpp's
  is_invocable_r_with -- a thin reuse, not a reimplementation.
  Tests the following:
  - is_predicate derives from is_invocable_r_with<bool, ...>
  - the two agree on the fixtures, including the explicit-bool edge
*/
bool
tests_is_predicate_reuses_invocable_r_with()
{
    static_assert(std::is_base_of<is_invocable_r_with<bool, pred_int, int>,
                                  is_predicate<pred_int, int> >::value,
                  "derives from is_invocable_r_with");
    static_assert(std::is_base_of<is_invocable_r_with<bool, pred_str, int>,
                                  is_predicate<pred_str, int> >::value,
                  "derives, negative case too");

    bool ok = true;
    ok = ok && (is_predicate<pred_bool, const int&>::value ==
                is_invocable_r_with<bool, pred_bool, const int&>::value);
    ok = ok && (is_predicate<pred_explicit, int>::value ==
                is_invocable_r_with<bool, pred_explicit, int>::value);

    return ok;
}


/*
tests_is_predicate_integral_constant
  is_predicate carries the same bool-constant surface as is_callable.
  Tests the following:
  - value, value_type, and type
  - it derives from true_type / false_type and converts to bool
*/
bool
tests_is_predicate_integral_constant()
{
    static_assert(is_predicate<pred_int, int>::value, "value");
    static_assert(std::is_same<is_predicate<pred_int, int>::value_type,
                               bool>::value, "value_type");
    static_assert(std::is_same<is_predicate<pred_int, int>::type,
                               std::true_type>::value, "type = true_type");
    static_assert(std::is_same<is_predicate<pred_str, int>::type,
                               std::false_type>::value, "type = false_type");
    static_assert(std::is_base_of<std::false_type,
                                  is_predicate<pred_str, int> >::value,
                  "derives from false_type");

    const bool b = is_predicate<pred_int, int>();
    return b;
}


NS_END  // testing
NS_END  // djinterp
