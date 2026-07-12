// djinterp [test]  functional_traits_tests_callable.cpp
//   Section I -- is_callable and callable_result_t.

// std
#include <string>
#include <type_traits>
// djinterp
#include "functional_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_is_callable_positive
  The ordinary callable shapes the combinators take are all recognised.
  Tests the following:
  - a const functor, a lambda, a function pointer, and a function reference
  - a niladic callable and a multi-argument one
*/
bool
tests_is_callable_positive()
{
    bool ok = true;

    ok = ok && (is_callable<const_fn, int>::value);

    // a lambda -- the shape callers actually write.
    int  captured = 3;
    auto lam = [captured](int _x){ return _x + captured; };
    ok = ok && (is_callable<decltype(lam), int>::value);

    // function pointer and function reference.
    ok = ok && (is_callable<int(*)(int), int>::value);
    ok = ok && (is_callable<int(&)(int), int>::value);
    ok = ok && (is_callable<decltype(&free_fn), int>::value);

    ok = ok && (is_callable<niladic_fn>::value);
    ok = ok && (is_callable<two_arg_fn, int, double>::value);

    return ok;
}


/*
tests_is_callable_negative
  Non-callables and wrong call shapes are rejected, cleanly (SFINAE, not a hard
  error).
  Tests the following:
  - a struct with no operator(), and a scalar
  - too few and too many arguments
  - an argument the callable cannot accept
*/
bool
tests_is_callable_negative()
{
    bool ok = true;

    ok = ok && (!is_callable<not_callable, int>::value);
    ok = ok && (!is_callable<int, int>::value);
    ok = ok && (!is_callable<std::string, int>::value);

    ok = ok && (!is_callable<two_arg_fn, int>::value);              // too few
    ok = ok && (!is_callable<const_fn, int, int>::value);           // too many
    ok = ok && (!is_callable<niladic_fn, int>::value);              // too many

    ok = ok && (!is_callable<str_pred, int>::value);                // wrong type

    return ok;
}


/*
tests_is_callable_const_lvalue
  The defining contract: the probe is a CONST-LVALUE call, so a callable that
  cannot be invoked on a const lvalue is not callable -- however happily it would
  answer a non-const call.
  Tests the following:
  - a non-const operator() is rejected
  - an &&-qualified operator() is rejected
  - the const-qualified counterpart is accepted (the contrast)
*/
bool
tests_is_callable_const_lvalue()
{
    bool ok = true;

    ok = ok && (!is_callable<nonconst_fn, int>::value);
    ok = ok && (!is_callable<rvalue_fn, int>::value);

    ok = ok && (is_callable<const_fn, int>::value);

    // the rejected fixtures really are callable -- just not on a const lvalue.
    nonconst_fn nc;
    ok = ok && (nc(5) == 10);
    ok = ok && (rvalue_fn()(5) == 5);

    return ok;
}


/*
tests_is_callable_templated_operator
  The trait probes an expression, so it succeeds on generic lambdas and other
  templated operator() callables -- the header's headline claim, and the shape a
  declared-signature probe would miss.
  Tests the following:
  - a templated operator() is callable at several unrelated argument types
*/
bool
tests_is_callable_templated_operator()
{
    bool ok = true;

    ok = ok && (is_callable<generic_fn, int>::value);
    ok = ok && (is_callable<generic_fn, double>::value);
    ok = ok && (is_callable<generic_fn, std::string>::value);

    // still arity-sensitive.
    ok = ok && (!is_callable<generic_fn, int, int>::value);
    ok = ok && (!is_callable<generic_fn>::value);

    return ok;
}


/*
tests_is_callable_overload_set
  For an overload set the trait resolves against _Args, accepting exactly the
  arguments some overload takes.
  Tests the following:
  - both overloads' argument types are callable
  - an argument matching neither is not
*/
bool
tests_is_callable_overload_set()
{
    bool ok = true;

    ok = ok && (is_callable<overloaded_fn, int>::value);
    ok = ok && (is_callable<overloaded_fn, std::string>::value);

    ok = ok && (!is_callable<overloaded_fn, int, int>::value);

    return ok;
}


/*
tests_is_callable_arg_categories
  _Args are probed exactly as written, so value category and constness matter.
  Tests the following:
  - a callable taking int& accepts int&, but not const int& and not a prvalue
*/
bool
tests_is_callable_arg_categories()
{
    bool ok = true;

    ok = ok && (is_callable<ref_fn, int&>::value);
    ok = ok && (!is_callable<ref_fn, const int&>::value);
    ok = ok && (!is_callable<ref_fn, int>::value);          // prvalue -> int&&

    // a by-value callable accepts all three.
    ok = ok && (is_callable<const_fn, int>::value);
    ok = ok && (is_callable<const_fn, int&>::value);
    ok = ok && (is_callable<const_fn, const int&>::value);

    return ok;
}


/*
tests_is_callable_arity_spread
  _Args is a pack, so the trait works at any arity.
  Tests the following:
  - zero, one, two, and three arguments
  - each callable rejects the other arities
*/
bool
tests_is_callable_arity_spread()
{
    bool ok = true;

    ok = ok && (is_callable<niladic_fn>::value);
    ok = ok && (is_callable<const_fn, int>::value);
    ok = ok && (is_callable<two_arg_fn, int, double>::value);
    ok = ok && (is_callable<three_arg_fn, int, int, int>::value);

    ok = ok && (!is_callable<three_arg_fn, int, int>::value);
    ok = ok && (!is_callable<niladic_fn, int>::value);

    return ok;
}


/*
tests_is_callable_integral_constant
  is_callable is a std::integral_constant, so it carries the whole
  bool-constant surface.
  Tests the following:
  - value, value_type, and type are as expected
  - it converts implicitly to bool, and derives from true_type / false_type
*/
bool
tests_is_callable_integral_constant()
{
    static_assert(is_callable<const_fn, int>::value, "value");
    static_assert(std::is_same<is_callable<const_fn, int>::value_type,
                               bool>::value, "value_type");
    static_assert(std::is_same<is_callable<const_fn, int>::type,
                               std::true_type>::value, "type = true_type");
    static_assert(std::is_same<is_callable<not_callable, int>::type,
                               std::false_type>::value, "type = false_type");

    static_assert(std::is_base_of<std::true_type,
                                  is_callable<const_fn, int> >::value,
                  "derives from true_type");
    static_assert(std::is_base_of<std::false_type,
                                  is_callable<not_callable, int> >::value,
                  "derives from false_type");

    // implicit conversion to bool.
    const bool b = is_callable<const_fn, int>();
    return b;
}


/*
tests_is_callable_reuses_invocable_with
  is_callable is the functional-facing NAME for function_traits.hpp's
  is_invocable_with -- a thin reuse, not a reimplementation.
  Tests the following:
  - is_callable derives from is_invocable_with, positively and negatively
  - the two agree on every fixture
*/
bool
tests_is_callable_reuses_invocable_with()
{
    static_assert(std::is_base_of<is_invocable_with<const_fn, int>,
                                  is_callable<const_fn, int> >::value,
                  "derives from is_invocable_with");
    static_assert(std::is_base_of<is_invocable_with<not_callable, int>,
                                  is_callable<not_callable, int> >::value,
                  "derives, negative case too");

    bool ok = true;
    ok = ok && (is_callable<const_fn, int>::value ==
                is_invocable_with<const_fn, int>::value);
    ok = ok && (is_callable<nonconst_fn, int>::value ==
                is_invocable_with<nonconst_fn, int>::value);
    ok = ok && (is_callable<generic_fn, std::string>::value ==
                is_invocable_with<generic_fn, std::string>::value);

    return ok;
}


/*
tests_callable_result_basic
  callable_result_t is the type the call yields.
  Tests the following:
  - int, void, std::string, and a templated operator()'s deduced result
*/
bool
tests_callable_result_basic()
{
    static_assert(std::is_same<callable_result_t<const_fn, int>, int>::value,
                  "int result");
    static_assert(std::is_same<callable_result_t<void_fn, int>, void>::value,
                  "void result");
    static_assert(std::is_same<callable_result_t<overloaded_fn, std::string>,
                               std::string>::value, "overload result");
    static_assert(std::is_same<callable_result_t<overloaded_fn, int>,
                               int>::value, "other overload result");
    static_assert(std::is_same<callable_result_t<generic_fn, double>,
                               double>::value, "templated result follows arg");
    static_assert(std::is_same<callable_result_t<niladic_fn>, int>::value,
                  "niladic result");
    static_assert(std::is_same<callable_result_t<two_arg_fn, int, double>,
                               int>::value, "two-arg result");

    return true;
}


/*
tests_callable_result_nonesuch
  When the call is ill-formed the alias is internal::call_nonesuch rather than a
  hard error -- so callers may name it before gating on is_callable.
  Tests the following:
  - a non-callable, a wrong arity, and a const-lvalue-rejected callable all
    yield call_nonesuch
  - a well-formed call does not
*/
bool
tests_callable_result_nonesuch()
{
    static_assert(std::is_same<callable_result_t<not_callable, int>,
                               internal::call_nonesuch>::value,
                  "non-callable -> nonesuch");
    static_assert(std::is_same<callable_result_t<const_fn, int, int>,
                               internal::call_nonesuch>::value,
                  "wrong arity -> nonesuch");
    static_assert(std::is_same<callable_result_t<nonconst_fn, int>,
                               internal::call_nonesuch>::value,
                  "non-const operator() -> nonesuch");
    static_assert(std::is_same<callable_result_t<ref_fn, const int&>,
                               internal::call_nonesuch>::value,
                  "wrong arg category -> nonesuch");

    static_assert(!std::is_same<callable_result_t<const_fn, int>,
                                internal::call_nonesuch>::value,
                  "well-formed call is not nonesuch");

    return true;
}


/*
tests_callable_result_preserves_reference
  The alias reports the call's DECLARED result type -- references are preserved,
  not decayed away.
  Tests the following:
  - a callable returning int& yields int&, distinct from int
*/
bool
tests_callable_result_preserves_reference()
{
    static_assert(std::is_same<callable_result_t<ref_fn, int&>, int&>::value,
                  "reference preserved");
    static_assert(!std::is_same<callable_result_t<ref_fn, int&>, int>::value,
                  "not decayed to a value");
    static_assert(std::is_reference<callable_result_t<ref_fn, int&> >::value,
                  "is a reference");

    // and the call really does alias the caller's object.
    int  n = 1;
    ref_fn f;
    f(n) = 42;
    return (n == 42);
}


/*
tests_callable_result_is_alias
  callable_result_t is a thin alias of function_traits.hpp's call_result_t --
  the same type, not a parallel computation.
  Tests the following:
  - the two agree for a well-formed call and for an ill-formed one
*/
bool
tests_callable_result_is_alias()
{
    static_assert(std::is_same<callable_result_t<const_fn, int>,
                               call_result_t<const_fn, int> >::value,
                  "alias, well-formed");
    static_assert(std::is_same<callable_result_t<not_callable, int>,
                               call_result_t<not_callable, int> >::value,
                  "alias, ill-formed");
    static_assert(std::is_same<callable_result_t<generic_fn, std::string>,
                               call_result_t<generic_fn, std::string> >::value,
                  "alias, templated");

    return true;
}


/*
tests_reexports_function_traits
  The header re-exports function_traits.hpp, so a single include carries both the
  declared-shape introspection and this can-I-call-it vocabulary.
  Tests the following:
  - is_invocable_with, is_invocable_r_with, and call_result_t are all visible
    through this header alone
*/
bool
tests_reexports_function_traits()
{
    bool ok = true;

    ok = ok && (is_invocable_with<const_fn, int>::value);
    ok = ok && (!is_invocable_with<not_callable, int>::value);

    ok = ok && (is_invocable_r_with<bool, pred_bool, const int&>::value);
    ok = ok && (!is_invocable_r_with<bool, pred_str, int>::value);

    static_assert(std::is_same<call_result_t<const_fn, int>, int>::value,
                  "call_result_t visible");

    return ok;
}


NS_END  // testing
NS_END  // djinterp
