#include "comparator_tests.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
// std
#include <type_traits>
#endif


NS_DJINTERP
NS_TESTING


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// not_cmp
//   struct: negative fixture. Has no call operator at all, so it models none
// of the comparator/predicate shapes.
struct not_cmp
{
    int x;
};


// ---- compile-time fixtures ----
//   Concrete helper types produced by the factories. Naming them lets the
// static_asserts below read as plain type predicates. decltype on a factory
// call is well-formed in an unevaluated context on every C++11+ compiler.
typedef decltype(comparators::natural<int>())                  natural_t;
typedef decltype(comparators::by_key(get_rank()))              by_key_t;
typedef decltype(comparators::reversed(comparators::natural<int>()))
                                                               reversed_t;
typedef decltype(comparators::less_than(comparators::natural<int>(), 0))
                                                               less_than_t;
typedef decltype(comparators::greater_than(comparators::natural<int>(), 0))
                                                               greater_than_t;
typedef decltype(comparators::equal_under(comparators::natural<int>()))
                                                               equal_under_t;


///////////////////////////////////////////////////////////////////////////////
///                COMPILE-TIME ASSERTIONS                                   ///
///////////////////////////////////////////////////////////////////////////////
//   The traits are a compile-time contract, so the primary verification is a
// battery of static_asserts. The runtime predicates further below re-check
// the same facts through ::value so the section participates in the normal
// pass/fail tally as well.

// is_comparator: every factory / combinator output models it over its
// operand type; non-callables and unary predicates do not.
static_assert(
    is_comparator<natural_t, int>::value,
    "natural() must model is_comparator<.,int>");
static_assert(
    is_comparator<by_key_t, record>::value,
    "by_key(get_rank) must model is_comparator<.,record>");
static_assert(
    is_comparator<reversed_t, int>::value,
    "reversed(natural) must model is_comparator<.,int>");
static_assert(
    is_comparator<asc_int, int>::value,
    "a raw binary functor must model is_comparator");
static_assert(
    !is_comparator<not_cmp, int>::value,
    "a non-callable aggregate must NOT model is_comparator");
static_assert(
    !is_comparator<int, int>::value,
    "a scalar must NOT model is_comparator");
static_assert(
    !is_comparator<less_than_t, int>::value,
    "a unary predicate must NOT model is_comparator (wrong arity)");

// cv / ref qualifiers are stripped before inspection.
static_assert(
    is_comparator<const natural_t&, int>::value,
    "is_comparator must see through const-ref");

// is_binary_predicate: comparators and equal_under satisfy it; unary
// predicates and non-callables do not.
static_assert(
    is_binary_predicate<natural_t, int, int>::value,
    "natural() is a binary predicate over (int,int)");
static_assert(
    is_binary_predicate<equal_under_t, int, int>::value,
    "equal_under(natural) is a binary predicate over (int,int)");
static_assert(
    !is_binary_predicate<less_than_t, int, int>::value,
    "less_than result is unary, not a binary predicate");
static_assert(
    !is_binary_predicate<not_cmp, int, int>::value,
    "a non-callable is not a binary predicate");

// is_unary_predicate: the one-sided binders satisfy it; comparators do not.
static_assert(
    is_unary_predicate<less_than_t, int>::value,
    "less_than result is a unary predicate over int");
static_assert(
    is_unary_predicate<greater_than_t, int>::value,
    "greater_than result is a unary predicate over int");
static_assert(
    !is_unary_predicate<natural_t, int>::value,
    "a comparator is not a unary predicate (wrong arity)");
static_assert(
    !is_unary_predicate<not_cmp, int>::value,
    "a non-callable is not a unary predicate");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// _v shorthands agree with the ::value form.
static_assert(
    is_comparator_v<natural_t, int>,
    "is_comparator_v must agree with ::value");
static_assert(
    !is_comparator_v<not_cmp, int>,
    "is_comparator_v must reject a non-callable");
static_assert(
    is_binary_predicate_v<natural_t, int, int>,
    "is_binary_predicate_v must agree with ::value");
static_assert(
    is_unary_predicate_v<less_than_t, int>,
    "is_unary_predicate_v must agree with ::value");
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
// concepts mirror the traits.
static_assert(
    is_comparator_c<natural_t, int>,
    "natural() satisfies is_comparator_c");
static_assert(
    is_comparator_c<asc_int, int>,
    "a raw binary functor satisfies is_comparator_c");
static_assert(
    !is_comparator_c<not_cmp, int>,
    "a non-callable fails is_comparator_c");
static_assert(
    !is_comparator_c<less_than_t, int>,
    "a unary predicate fails is_comparator_c");
static_assert(
    is_binary_predicate_c<equal_under_t, int, int>,
    "equal_under satisfies is_binary_predicate_c");
static_assert(
    is_unary_predicate_c<greater_than_t, int>,
    "greater_than satisfies is_unary_predicate_c");
static_assert(
    !is_unary_predicate_c<natural_t, int>,
    "a comparator fails is_unary_predicate_c");
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///                RUNTIME PREDICATES                                        ///
///////////////////////////////////////////////////////////////////////////////

/*
test_traits_is_comparator
  Exercises the is_comparator trait at run time.
  Tests the following:
  - factory / combinator outputs and raw binary functors are accepted
  - non-callables, scalars, and unary predicates are rejected
  - cv/ref qualifiers are stripped before inspection
  Traits are C++11+; on the C++98 path the body is a no-op reporting success.
*/
bool
test_traits_is_comparator(
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_INTERNAL_CMP_CHECK((is_comparator<natural_t, int>::value));
    D_INTERNAL_CMP_CHECK((is_comparator<by_key_t, record>::value));
    D_INTERNAL_CMP_CHECK((is_comparator<reversed_t, int>::value));
    D_INTERNAL_CMP_CHECK((is_comparator<asc_int, int>::value));
    D_INTERNAL_CMP_CHECK((!is_comparator<not_cmp, int>::value));
    D_INTERNAL_CMP_CHECK((!is_comparator<int, int>::value));
    D_INTERNAL_CMP_CHECK((!is_comparator<less_than_t, int>::value));
    D_INTERNAL_CMP_CHECK((is_comparator<const natural_t&, int>::value));
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    return true;
}


/*
test_traits_is_binary_predicate
  Exercises the is_binary_predicate trait at run time.
  Tests the following:
  - comparators and equal_under results are accepted
  - unary predicates and non-callables are rejected
  C++11+; C++98 body is a no-op reporting success.
*/
bool
test_traits_is_binary_predicate(
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_INTERNAL_CMP_CHECK((is_binary_predicate<natural_t, int, int>::value));
    D_INTERNAL_CMP_CHECK((is_binary_predicate<equal_under_t, int, int>::value));
    D_INTERNAL_CMP_CHECK((!is_binary_predicate<less_than_t, int, int>::value));
    D_INTERNAL_CMP_CHECK((!is_binary_predicate<not_cmp, int, int>::value));
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    return true;
}


/*
test_traits_is_unary_predicate
  Exercises the is_unary_predicate trait at run time.
  Tests the following:
  - the one-sided binders are accepted
  - comparators (wrong arity) and non-callables are rejected
  C++11+; C++98 body is a no-op reporting success.
*/
bool
test_traits_is_unary_predicate(
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_INTERNAL_CMP_CHECK((is_unary_predicate<less_than_t, int>::value));
    D_INTERNAL_CMP_CHECK((is_unary_predicate<greater_than_t, int>::value));
    D_INTERNAL_CMP_CHECK((!is_unary_predicate<natural_t, int>::value));
    D_INTERNAL_CMP_CHECK((!is_unary_predicate<not_cmp, int>::value));
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    return true;
}


/*
test_traits_has_result_type
  Exercises the has_result_type structural hint.
  Tests the following:
  - a type without a result_type typedef is reported absent
  The C++11 comparator helpers are transparently callable and intentionally
  carry no result_type, so this trait is only a hint; the negative case is
  the portable, stable assertion.
  C++11+; C++98 body is a no-op reporting success.
*/
bool
test_traits_has_result_type(
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    D_INTERNAL_CMP_CHECK((!has_result_type<not_cmp>::value));
    D_INTERNAL_CMP_CHECK((!has_result_type<int>::value));
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    return true;
}


/*
test_traits_value_aliases
  Exercises the C++14 *_v variable-template shorthands.
  Tests the following:
  - each _v alias agrees with its ::value counterpart
  Absent before C++14 (and entirely on the C++98 path), so the body is a
  no-op that still reports success.
*/
bool
test_traits_value_aliases(
)
{
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    D_INTERNAL_CMP_CHECK((is_comparator_v<natural_t, int>));
    D_INTERNAL_CMP_CHECK((!is_comparator_v<not_cmp, int>));
    D_INTERNAL_CMP_CHECK((is_binary_predicate_v<natural_t, int, int>));
    D_INTERNAL_CMP_CHECK((is_unary_predicate_v<less_than_t, int>));
    D_INTERNAL_CMP_CHECK((!has_result_type_v<not_cmp>));
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    return true;
}


/*
test_traits_concepts
  Exercises the C++20 comparator concepts.
  Tests the following:
  - comparators and raw binary functors satisfy is_comparator_c
  - non-callables and unary predicates fail it
  - the predicate-shaped concepts agree with their trait counterparts
  Absent before C++20 (and on the C++98 path), so the body is a no-op that
  still reports success.
*/
bool
test_traits_concepts(
)
{
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    D_INTERNAL_CMP_CHECK((is_comparator_c<natural_t, int>));
    D_INTERNAL_CMP_CHECK((is_comparator_c<asc_int, int>));
    D_INTERNAL_CMP_CHECK((!is_comparator_c<not_cmp, int>));
    D_INTERNAL_CMP_CHECK((!is_comparator_c<less_than_t, int>));
    D_INTERNAL_CMP_CHECK((is_binary_predicate_c<equal_under_t, int, int>));
    D_INTERNAL_CMP_CHECK((is_unary_predicate_c<greater_than_t, int>));
    D_INTERNAL_CMP_CHECK((!is_unary_predicate_c<natural_t, int>));
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

    return true;
}


/*
run_traits_tests
  Aggregates every traits-section test.
  Tests the following:
  - all is_comparator / is_*_predicate / has_result_type / value-alias /
    concept tests pass
*/
bool
run_traits_tests(
)
{
    return ( test_traits_is_comparator()       &&
             test_traits_is_binary_predicate()  &&
             test_traits_is_unary_predicate()   &&
             test_traits_has_result_type()      &&
             test_traits_value_aliases()        &&
             test_traits_concepts() );
}


NS_END  // testing
NS_END  // djinterp
