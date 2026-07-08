#include "filter_tests.hpp"

// std
#include <cstddef>
#include <type_traits>
#include <vector>


NS_DJINTERP
NS_TESTING


// ---- compile-time fixtures ----
//   Concrete types produced by the module, named so the static_asserts read
// as plain predicates. The typed-filter fixture is built from a named functor
// (is_even), never a lambda, so decltype is well-formed from C++11 upward.
typedef filter_op_fn<int>                                op_fn_t;
typedef filter_chain<int>                                chain_t;
typedef filter_builder<int>                              builder_t;
typedef filter_result<int>                               result_t;
typedef decltype(make_typed_filter<int>())               typed_id_t;
typedef decltype(make_typed_filter<int>().where(is_even())) typed_where_t;


///////////////////////////////////////////////////////////////////////////////
///                COMPILE-TIME ASSERTIONS                                   ///
///////////////////////////////////////////////////////////////////////////////

// is_filter_operation: the filter_op_fn protocol over int.
static_assert( is_filter_operation<op_fn_t, int>::value,
               "filter_op_fn models the op protocol");
static_assert( is_filter_operation<raw_op, int>::value,
               "a raw vector->indices functor models the op protocol");
static_assert( is_filter_operation<typed_id_t::chain_type, int>::value,
               "the typed identity functor models the op protocol");
static_assert( is_filter_operation<typed_where_t::chain_type, int>::value,
               "a composed typed functor models the op protocol");
static_assert( !is_filter_operation<not_a_filter, int>::value,
               "a non-callable struct is not an op");
static_assert( !is_filter_operation<int, int>::value,
               "a scalar is not an op");

// cv/ref are stripped on the callable.
static_assert( is_filter_operation<const raw_op&, int>::value,
               "is_filter_operation sees through const-ref");

// is_filter_applicable: chain / builder / typed_filter expose .apply(vector).
static_assert( is_filter_applicable<chain_t, int>::value,
               "filter_chain is applicable");
static_assert( is_filter_applicable<builder_t, int>::value,
               "filter_builder is applicable");
static_assert( is_filter_applicable<typed_id_t, int>::value,
               "typed_filter is applicable");
static_assert( !is_filter_applicable<not_a_filter, int>::value,
               "a non-applicable struct is rejected");
static_assert( !is_filter_applicable<result_t, int>::value,
               "filter_result has no apply, so it is not applicable");

// is_filter_result: the result inspection surface.
static_assert( is_filter_result<result_t>::value,
               "filter_result models the result surface");
static_assert( !is_filter_result<not_a_filter>::value,
               "a plain struct is not a result");
static_assert( !is_filter_result<chain_t>::value,
               "filter_chain lacks ok/elements, so it is not a result");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// _v shorthands agree with the ::value form.
static_assert( is_filter_operation_v<op_fn_t, int>,
               "is_filter_operation_v agrees");
static_assert( !is_filter_operation_v<not_a_filter, int>,
               "is_filter_operation_v rejects a non-op");
static_assert( is_filter_applicable_v<chain_t, int>,
               "is_filter_applicable_v agrees");
static_assert( is_filter_result_v<result_t>,
               "is_filter_result_v agrees");
static_assert( is_filterable_v<std::vector<int> >,
               "is_filterable_v agrees (folded trait)");
static_assert( is_iterable_v<std::vector<int> >,
               "is_iterable_v agrees (folded trait)");
static_assert( !has_filter_method_v<std::vector<int> >,
               "has_filter_method_v agrees (folded trait)");
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
// concepts mirror the traits.
static_assert( filter_operation_c<op_fn_t, int>,
               "filter_operation_c on op_fn");
static_assert( !filter_operation_c<not_a_filter, int>,
               "filter_operation_c rejects a non-op");
static_assert( filter_applicable_c<chain_t, int>,
               "filter_applicable_c on chain");
static_assert( !filter_applicable_c<result_t, int>,
               "filter_applicable_c rejects a result");
static_assert( filter_result_c<result_t>,
               "filter_result_c on result");
static_assert( !filter_result_c<not_a_filter>,
               "filter_result_c rejects a plain struct");
static_assert( filterable_c<std::vector<int> >,
               "filterable_c on vector (folded trait)");
static_assert( !filterable_c<int>,
               "filterable_c rejects a scalar");
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///                RUNTIME PREDICATES                                        ///
///////////////////////////////////////////////////////////////////////////////

/*
test_traits_is_filter_operation
  Exercises is_filter_operation at run time.
  Tests the following:
  - filter_op_fn, a raw vector->indices functor, and the typed chain functors
    are accepted over int
  - a non-callable struct and a scalar are rejected
  - cv/ref qualifiers are stripped on the callable
*/
bool
test_traits_is_filter_operation(
)
{
    D_INTERNAL_FLT_CHECK((is_filter_operation<op_fn_t, int>::value));
    D_INTERNAL_FLT_CHECK((is_filter_operation<raw_op, int>::value));
    D_INTERNAL_FLT_CHECK(
        (is_filter_operation<typed_id_t::chain_type, int>::value));
    D_INTERNAL_FLT_CHECK(
        (is_filter_operation<typed_where_t::chain_type, int>::value));
    D_INTERNAL_FLT_CHECK((!is_filter_operation<not_a_filter, int>::value));
    D_INTERNAL_FLT_CHECK((!is_filter_operation<int, int>::value));
    D_INTERNAL_FLT_CHECK((is_filter_operation<const raw_op&, int>::value));

    return true;
}


/*
test_traits_is_filter_applicable
  Exercises is_filter_applicable at run time.
  Tests the following:
  - filter_chain, filter_builder, and typed_filter are accepted over int
  - a non-applicable struct and filter_result are rejected
*/
bool
test_traits_is_filter_applicable(
)
{
    D_INTERNAL_FLT_CHECK((is_filter_applicable<chain_t, int>::value));
    D_INTERNAL_FLT_CHECK((is_filter_applicable<builder_t, int>::value));
    D_INTERNAL_FLT_CHECK((is_filter_applicable<typed_id_t, int>::value));
    D_INTERNAL_FLT_CHECK((!is_filter_applicable<not_a_filter, int>::value));
    D_INTERNAL_FLT_CHECK((!is_filter_applicable<result_t, int>::value));

    return true;
}


/*
test_traits_is_filter_result
  Exercises is_filter_result at run time.
  Tests the following:
  - filter_result is accepted
  - a plain struct and a filter_chain are rejected
*/
bool
test_traits_is_filter_result(
)
{
    D_INTERNAL_FLT_CHECK((is_filter_result<result_t>::value));
    D_INTERNAL_FLT_CHECK((!is_filter_result<not_a_filter>::value));
    D_INTERNAL_FLT_CHECK((!is_filter_result<chain_t>::value));

    return true;
}


/*
test_traits_value_aliases
  Exercises the C++14 *_v variable-template shorthands (new and folded).
  Tests the following:
  - each _v alias agrees with its ::value counterpart
  Absent before C++14, so the body is a no-op that still reports success.
*/
bool
test_traits_value_aliases(
)
{
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    D_INTERNAL_FLT_CHECK((is_filter_operation_v<op_fn_t, int>));
    D_INTERNAL_FLT_CHECK((!is_filter_operation_v<not_a_filter, int>));
    D_INTERNAL_FLT_CHECK((is_filter_applicable_v<chain_t, int>));
    D_INTERNAL_FLT_CHECK((is_filter_result_v<result_t>));
    D_INTERNAL_FLT_CHECK((is_filterable_v<std::vector<int> >));
    D_INTERNAL_FLT_CHECK((is_iterable_v<std::vector<int> >));
    D_INTERNAL_FLT_CHECK((!has_filter_method_v<std::vector<int> >));
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return true;
}


/*
test_traits_concepts
  Exercises the C++20 concepts (new and folded).
  Tests the following:
  - filter_operation_c / filter_applicable_c / filter_result_c / filterable_c
    accept and reject in line with their traits
  Absent before C++20, so the body is a no-op that still reports success.
*/
bool
test_traits_concepts(
)
{
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    D_INTERNAL_FLT_CHECK((filter_operation_c<op_fn_t, int>));
    D_INTERNAL_FLT_CHECK((!filter_operation_c<not_a_filter, int>));
    D_INTERNAL_FLT_CHECK((filter_applicable_c<chain_t, int>));
    D_INTERNAL_FLT_CHECK((!filter_applicable_c<result_t, int>));
    D_INTERNAL_FLT_CHECK((filter_result_c<result_t>));
    D_INTERNAL_FLT_CHECK((!filter_result_c<not_a_filter>));
    D_INTERNAL_FLT_CHECK((filterable_c<std::vector<int> >));
    D_INTERNAL_FLT_CHECK((!filterable_c<int>));
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

    return true;
}


/*
run_traits_tests
  Aggregates every structural-traits test.
  Tests the following:
  - all is_filter_operation / is_filter_applicable / is_filter_result /
    value-alias / concept tests pass
*/
bool
run_traits_tests(
)
{
    return ( test_traits_is_filter_operation()  &&
             test_traits_is_filter_applicable()  &&
             test_traits_is_filter_result()      &&
             test_traits_value_aliases()         &&
             test_traits_concepts() );
}


NS_END  // testing
NS_END  // djinterp
