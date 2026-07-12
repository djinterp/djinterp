// djinterp [test]  functional_traits_tests_aliases.cpp
//   Section III -- the _v variable-template shorthands (C++14+).

// std
#include <string>
#include <type_traits>
// djinterp
#include "functional_traits_tests.hpp"


NS_DJINTERP
NS_TESTING


/*
tests_is_callable_v_agrees
  is_callable_v is exactly is_callable<...>::value.
  Tests the following:
  - the two agree across positives, negatives, and the const-lvalue edge
*/
bool
tests_is_callable_v_agrees()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    ok = ok && (is_callable_v<const_fn, int> ==
                is_callable<const_fn, int>::value);
    ok = ok && (is_callable_v<not_callable, int> ==
                is_callable<not_callable, int>::value);
    ok = ok && (is_callable_v<nonconst_fn, int> ==
                is_callable<nonconst_fn, int>::value);
    ok = ok && (is_callable_v<generic_fn, std::string> ==
                is_callable<generic_fn, std::string>::value);

    static_assert(is_callable_v<const_fn, int>, "positive");
    static_assert(!is_callable_v<not_callable, int>, "negative");
    static_assert(!is_callable_v<nonconst_fn, int>, "const-lvalue edge");
#endif

    return ok;
}


/*
tests_is_predicate_v_agrees
  is_predicate_v is exactly is_predicate<...>::value.
  Tests the following:
  - the two agree across positives, negatives, and the explicit-bool edge
*/
bool
tests_is_predicate_v_agrees()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    ok = ok && (is_predicate_v<pred_bool, const int&> ==
                is_predicate<pred_bool, const int&>::value);
    ok = ok && (is_predicate_v<pred_str, int> ==
                is_predicate<pred_str, int>::value);
    ok = ok && (is_predicate_v<pred_explicit, int> ==
                is_predicate<pred_explicit, int>::value);

    static_assert(is_predicate_v<pred_int, int>, "positive");
    static_assert(!is_predicate_v<pred_str, int>, "negative");
    static_assert(!is_predicate_v<pred_explicit, int>, "explicit-bool edge");
#endif

    return ok;
}


/*
tests_aliases_are_constant_expressions
  The shorthands are constexpr bools, usable wherever a constant is required.
  Tests the following:
  - as a template argument, as an array bound, and in a constexpr initialiser
  - the constant is of type bool
*/
bool
tests_aliases_are_constant_expressions()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // as a template argument.
    typedef std::integral_constant<bool, is_callable_v<const_fn, int> > c1;
    static_assert(c1::value, "template argument");

    // as an array bound.
    int arr[is_callable_v<const_fn, int> ? 2 : 1];
    ok = ok && (sizeof(arr) / sizeof(arr[0]) == 2u);

    // in a constexpr initialiser, and typed bool.
    D_CONSTEXPR bool b = is_predicate_v<pred_int, int>;
    static_assert(b, "constexpr initialiser");
    static_assert(std::is_same<decltype(is_callable_v<const_fn, int>),
                               const bool>::value, "typed bool");
#endif

    return ok;
}


/*
tests_is_callable_v_variadic
  is_callable_v carries the same argument pack as the trait.
  Tests the following:
  - zero, one, two, and three arguments through the shorthand
*/
bool
tests_is_callable_v_variadic()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_callable_v<niladic_fn>, "zero args");
    static_assert(is_callable_v<const_fn, int>, "one arg");
    static_assert(is_callable_v<two_arg_fn, int, double>, "two args");
    static_assert(is_callable_v<three_arg_fn, int, int, int>, "three args");
    static_assert(!is_callable_v<three_arg_fn, int, int>, "wrong arity");

    ok = ok && (is_callable_v<two_arg_fn, int, double>);
#endif

    return ok;
}


/*
tests_aliases_gating
  The shorthands exist exactly where the standard allows them: they are gated on
  variable templates (C++14+), and below that callers use the ::value forms,
  which are always present.
  Tests the following:
  - the gate agrees with the language level
  - the ::value forms work regardless of the gate
*/
bool
tests_aliases_gating()
{
    bool ok = true;

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // gate open: the shorthands must be usable.
    ok = ok && (is_callable_v<const_fn, int>);
    ok = ok && (is_predicate_v<pred_int, int>);
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        ok = ok && true;    // open on C++14 and later, as advertised
    #endif
#else
    // gate closed: this must be a pre-C++14 build.
    #if D_ENV_LANG_IS_CPP14_OR_HIGHER
        ok = false;         // shorthands missing on a standard that has them
    #endif
#endif

    // the ::value forms are the floor, available on every standard.
    ok = ok && (is_callable<const_fn, int>::value);
    ok = ok && (is_predicate<pred_int, int>::value);

    return ok;
}


NS_END  // testing
NS_END  // djinterp
