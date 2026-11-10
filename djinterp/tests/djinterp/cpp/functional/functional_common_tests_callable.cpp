/*******************************************************************************
* djinterp [cpp]                            functional_common_tests_callable.cpp
*
*
* path:      /tests/djinterp/cpp/functional/functional_common_tests_callable.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.12
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp [test]  functional_traits_tests_callable.cpp
//   Section I -- is_callable and callable_result_t.

// std
#include <string>
#include <type_traits>
// djinterp
#include "functional_common_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_is_callable_positive
  The ordinary callable shapes the combinators take are all recognised.
  Tests the following:
  - a const functor, a lambda, a function pointer, and a function reference
  - a niladic callable and a multi-argument one
*/
bool tests_is_callable_positive()
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

    ok = ok && (is_callable<DNiladicFn>::value);
    ok = ok && (is_callable<DTwoArgFn, int, double>::value);

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
bool tests_is_callable_negative()
{
    bool ok = true;

    ok = ok && (!is_callable<not_callable, int>::value);
    ok = ok && (!is_callable<int, int>::value);
    ok = ok && (!is_callable<std::string, int>::value);

    ok = ok && (!is_callable<DTwoArgFn, int>::value);              // too few
    ok = ok && (!is_callable<const_fn, int, int>::value);           // too many
    ok = ok && (!is_callable<DNiladicFn, int>::value);              // too many

    ok = ok && (!is_callable<DStrPred, int>::value);                // wrong type

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
bool tests_is_callable_const_lvalue()
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
bool tests_is_callable_templated_operator()
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
bool tests_is_callable_overload_set()
{
    bool ok = true;

    ok = ok && (is_callable<DOverloadedFn, int>::value);
    ok = ok && (is_callable<DOverloadedFn, std::string>::value);

    ok = ok && (!is_callable<DOverloadedFn, int, int>::value);

    return ok;
}


/*
tests_is_callable_arg_categories
  _Args are probed exactly as written, so value category and constness matter.
  Tests the following:
  - a callable taking int& accepts int&, but not const int& and not a prvalue
*/
bool tests_is_callable_arg_categories()
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
bool tests_is_callable_arity_spread()
{
    bool ok = true;

    ok = ok && (is_callable<DNiladicFn>::value);
    ok = ok && (is_callable<const_fn, int>::value);
    ok = ok && (is_callable<DTwoArgFn, int, double>::value);
    ok = ok && (is_callable<DThreeArgFn, int, int, int>::value);

    ok = ok && (!is_callable<DThreeArgFn, int, int>::value);
    ok = ok && (!is_callable<DNiladicFn, int>::value);

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
bool tests_is_callable_integral_constant()
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
bool tests_is_callable_reuses_invocable_with()
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
bool tests_callable_result_basic()
{
    static_assert(std::is_same<callable_result_t<const_fn, int>, int>::value,
                  "int result");
    static_assert(std::is_same<callable_result_t<void_fn, int>, void>::value,
                  "void result");
    static_assert(std::is_same<callable_result_t<DOverloadedFn, std::string>,
                               std::string>::value, "overload result");
    static_assert(std::is_same<callable_result_t<DOverloadedFn, int>,
                               int>::value, "other overload result");
    static_assert(std::is_same<callable_result_t<generic_fn, double>,
                               double>::value, "templated result follows arg");
    static_assert(std::is_same<callable_result_t<DNiladicFn>, int>::value,
                  "niladic result");
    static_assert(std::is_same<callable_result_t<DTwoArgFn, int, double>,
                               int>::value, "two-arg result");

    return true;
}


// DHasType
//   trait: true when Trait has a nested `type` -- how a caller sees whether
// callable_result found a well-formed call.
template<typename Trait,
         typename = void>
struct DHasType : std::false_type
{};

template<typename Trait>
struct DHasType<Trait, void_t<typename Trait::type> > : std::true_type
{};


/*
tests_callable_result_absent
  When the call is ill-formed, callable_result has no `type`, so
  callable_result_t is a soft (SFINAE) failure, as with std::invoke_result.
  (call_result_t is the form that yields internal::call_nonesuch instead.)
  Tests the following:
  - a non-callable, a wrong arity, and a wrong argument category leave
    callable_result without a `type`; a well-formed call has one
  - the call is made as std::invoke_result makes it, on an rvalue Function, so
    a non-const operator() is found; is_callable, which probes a const lvalue,
    rejects the same type
*/
bool tests_callable_result_absent()
{
    static_assert(!DHasType<callable_result<not_callable, int> >::value,
                  "non-callable: no type");
    static_assert(!DHasType<callable_result<const_fn, int, int> >::value,
                  "wrong arity: no type");
    static_assert(!DHasType<callable_result<ref_fn, const int&> >::value,
                  "wrong arg category: no type");
    static_assert(DHasType<callable_result<const_fn, int> >::value,
                  "well-formed call: type");

    static_assert(std::is_same<callable_result_t<nonconst_fn, int>,
                               int>::value,
                  "an rvalue call finds a non-const operator()");
    static_assert(!is_callable<nonconst_fn, int>::value,
                  "is_callable probes a const lvalue");

    return true;
}


/*
tests_callable_result_preserves_reference
  The alias reports the call's DECLARED result type -- references are preserved,
  not decayed away.
  Tests the following:
  - a callable returning int& yields int&, distinct from int
*/
bool tests_callable_result_preserves_reference()
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
tests_callable_result_agrees
  On a well-formed call callable_result_t and function_traits.hpp's
  call_result_t name the same type; on an ill-formed one only call_result_t
  names a type, internal::call_nonesuch.
  Tests the following:
  - the two agree for a plain and for a templated operator()
  - on an ill-formed call, call_result_t is call_nonesuch and callable_result
    has no type
*/
bool tests_callable_result_agrees()
{
    static_assert(std::is_same<callable_result_t<const_fn, int>,
                               call_result_t<const_fn, int> >::value,
                  "agree, well-formed");
    static_assert(std::is_same<callable_result_t<generic_fn, std::string>,
                               call_result_t<generic_fn, std::string> >::value,
                  "agree, templated");
    static_assert(std::is_same<call_result_t<not_callable, int>,
                               internal::call_nonesuch>::value,
                  "call_result_t: call_nonesuch");
    static_assert(!DHasType<callable_result<not_callable, int> >::value,
                  "callable_result: no type");

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
bool tests_reexports_function_traits()
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
