/******************************************************************************
* djinterp [functional]                          predicate_tests_behavioral.cpp
*
*   Tests for the behavioral detection trait is_predicate (+ is_predicate_v)
* and the C++20 concepts predicate_combinator / predicate. Also defines the
* suite aggregator run_all_predicate_tests().
*
*   The behavioral trait asks whether an arbitrary callable is invocable with
* a given argument pack and yields a bool-convertible result. C++11+ only;
* under C++98 the trait checks compile to a no-op pass (the aggregator still
* runs every other section).
*
* path:      /src/functional/predicate_tests_behavioral.cpp
******************************************************************************/

#include "./predicate_tests.hpp"


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// ---- positive: plain functors with bool / bool-convertible results ----
static_assert(is_predicate<is_positive, int>::value,
              "is_positive is a predicate over int");
static_assert(is_predicate<is_even, int>::value,
              "is_even is a predicate over int");
static_assert(is_predicate<less_than, int, int>::value,
              "less_than is a predicate over (int,int)");
static_assert(is_predicate<returns_int_predicate, int>::value,
              "int result is bool-convertible -> predicate");

// ---- positive: the header's own combinators model the trait ----
static_assert(
    is_predicate<internal::predicate_and_combinator<is_positive, is_even>,
                 int>::value,
    "and combinator is a predicate over int");
static_assert(
    is_predicate<internal::predicate_not_combinator<is_positive>, int>::value,
    "not combinator is a predicate over int");

// ---- negative: not callable at all ----
static_assert(!is_predicate<not_a_predicate, int>::value,
              "non-callable is not a predicate");

// ---- negative: void result is not bool-convertible ----
static_assert(!is_predicate<returns_void_predicate, int>::value,
              "void result is not a predicate");

// ---- negative: wrong arity ----
static_assert(!is_predicate<is_positive, int, int>::value,
              "unary functor is not a predicate over (int,int)");
static_assert(!is_predicate<less_than, int>::value,
              "binary functor is not a predicate over a single int");

// ---- negative: a non-callable scalar type ----
static_assert(!is_predicate<int, int>::value, "int is not a predicate");

#if D_ENV_LANG_IS_CPP14_OR_HIGHER
// ---- _v alias agrees with ::value ----
static_assert(is_predicate_v<is_positive, int>,  "is_predicate_v positive");
static_assert(!is_predicate_v<not_a_predicate, int>,
              "is_predicate_v negative");
static_assert(is_predicate_v<less_than, int, int>, "is_predicate_v binary");
#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
// ---- C++20 concepts mirror the traits ----
static_assert(predicate<is_positive, int>,
              "predicate concept: positive case");
static_assert(!predicate<not_a_predicate, int>,
              "predicate concept: non-callable rejected");
static_assert(!predicate<returns_void_predicate, int>,
              "predicate concept: void result rejected");

static_assert(
    predicate_combinator<
        internal::predicate_and_combinator<is_positive, is_even> >,
    "predicate_combinator concept: and accepted");
static_assert(!predicate_combinator<is_positive>,
              "predicate_combinator concept: bare functor rejected");

// concept usable as a constraint in an abbreviated function template
template<typename _P>
    requires predicate<_P, int>
D_CONSTEXPR bool invoke_pred(const _P& _p, int _v)
{
    return _p(_v);
}
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


/*
test_predicate_behavioral
  Tests the behavioral is_predicate trait and the C++20 concepts.
  Tests the following:
  - functors callable with the arg pack and yielding a bool-convertible
    result satisfy the trait (bool and int results both accepted)
  - the header's own combinators satisfy the trait
  - non-callable types, void-returning callables, and wrong-arity calls are
    all rejected
  - the _v alias agrees with the ::value form (C++14+)
  - the predicate / predicate_combinator concepts mirror the traits and are
    usable as constraints (C++20+)
*/
std::size_t
test_predicate_behavioral(
    test_registry& _reg
)
{
    std::size_t before;

    before = _reg.failures();

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // positive
    D_TESTING_CHECK(_reg, (is_predicate<is_positive, int>::value));
    D_TESTING_CHECK(_reg, (is_predicate<less_than, int, int>::value));
    D_TESTING_CHECK(_reg, (is_predicate<returns_int_predicate, int>::value));
    D_TESTING_CHECK(_reg,
        (is_predicate<
            internal::predicate_and_combinator<is_positive, is_even>,
            int>::value));

    // negative
    D_TESTING_CHECK(_reg,
        (is_predicate<not_a_predicate, int>::value == false));
    D_TESTING_CHECK(_reg,
        (is_predicate<returns_void_predicate, int>::value == false));
    D_TESTING_CHECK(_reg,
        (is_predicate<is_positive, int, int>::value == false));
    D_TESTING_CHECK(_reg, (is_predicate<int, int>::value == false));

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    // concept-constrained call compiles and runs
    D_TESTING_CHECK(_reg, invoke_pred(is_positive(), 5)  == true);
    D_TESTING_CHECK(_reg, invoke_pred(is_positive(), -5) == false);
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER
#else
    (void)_reg;  // C++98: trait/concepts unavailable
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    return (_reg.failures() - before);
}


/*
run_all_predicate_tests
  Drives every predicate test section against the supplied registry and
  returns the total number of failures observed across all sections.
*/
std::size_t
run_all_predicate_tests(
    test_registry& _reg
)
{
    // run each section; the registry accumulates checks and failures
    test_predicate_binary(_reg);
    test_predicate_not(_reg);
    test_predicate_nand_nor(_reg);
    test_predicate_variadic(_reg);
    test_predicate_traits(_reg);
    test_predicate_behavioral(_reg);

    return _reg.failures();
}


NS_END  // testing
NS_END  // djinterp
