#include "accumulator_tests.hpp"

// std
#include <type_traits>
#include <vector>


NS_DJINTERP
NS_TESTING


// not_acc
//   struct: negative fixture. A plain aggregate that exposes none of the
// accumulator typedefs or interface; every accumulator trait must reject it.
struct not_acc
{
    int x;
};


// ---- compile-time fixtures ----
//   Concrete accumulator types produced by the pre-built factories. Naming
// them here lets the static_asserts below read as plain type predicates.
typedef decltype(sum<int>())   sum_acc_type;
typedef decltype(mean<int>())  mean_acc_type;


///////////////////////////////////////////////////////////////////////////////
///                COMPILE-TIME ASSERTIONS                                   ///
///////////////////////////////////////////////////////////////////////////////
//   The traits are a compile-time contract, so the primary verification is a
// battery of static_asserts. The runtime predicates further below re-check
// the same facts through ::value so the section participates in the normal
// pass/fail tally as well.

// is_accumulator: accepts an unboxed accumulator, rejects boxed + plain types.
static_assert(
    is_accumulator<sum_acc_type>::value,
    "sum() must model is_accumulator");
static_assert(
    is_accumulator<mean_acc_type>::value,
    "mean() must model is_accumulator");
static_assert(
    !is_accumulator<boxed_accumulator<int, int> >::value,
    "boxed_accumulator must NOT model is_accumulator (no state_type)");
static_assert(
    !is_accumulator<not_acc>::value,
    "a plain aggregate must NOT model is_accumulator");
static_assert(
    !is_accumulator<int>::value,
    "a scalar must NOT model is_accumulator");

// cv / ref qualifiers are stripped before inspection.
static_assert(
    is_accumulator<const sum_acc_type&>::value,
    "is_accumulator must see through const-ref");
static_assert(
    is_accumulator<sum_acc_type&&>::value,
    "is_accumulator must see through rvalue-ref");

// is_boxed_accumulator: accepts the erased form, rejects the unboxed one.
static_assert(
    is_boxed_accumulator<boxed_accumulator<int, int> >::value,
    "boxed_accumulator must model is_boxed_accumulator");
static_assert(
    is_boxed_accumulator<boxed_accumulator<double, double> >::value,
    "boxed_accumulator<double,double> must model is_boxed_accumulator");
static_assert(
    !is_boxed_accumulator<sum_acc_type>::value,
    "an unboxed accumulator must NOT model is_boxed_accumulator");
static_assert(
    !is_boxed_accumulator<not_acc>::value,
    "a plain aggregate must NOT model is_boxed_accumulator");

// composite building blocks.
static_assert(
    has_accumulator_typedefs<sum_acc_type>::value,
    "sum() must expose all five typedefs");
static_assert(
    !has_accumulator_typedefs<boxed_accumulator<int, int> >::value,
    "boxed_accumulator lacks the full typedef set");
static_assert(
    has_accumulator_interface<sum_acc_type>::value,
    "sum() must expose step + finalize");
static_assert(
    has_accumulator_interface<boxed_accumulator<int, int> >::value,
    "boxed_accumulator must expose step + finalize");

// individual member detectors.
static_assert(
    has_state_type<sum_acc_type>::value,
    "sum() exposes state_type");
static_assert(
    !has_state_type<boxed_accumulator<int, int> >::value,
    "boxed_accumulator hides state_type");
static_assert(
    has_run_method<boxed_accumulator<int, int> >::value,
    "boxed_accumulator exposes run()");
static_assert(
    has_state_method<sum_acc_type>::value,
    "sum() exposes state()");
static_assert(
    !has_state_method<boxed_accumulator<int, int> >::value,
    "boxed_accumulator hides state()");

// type extraction yields the inner typedefs.
static_assert(
    std::is_same<accumulator_state_t<sum_acc_type>, int>::value,
    "sum() state_type is int");
static_assert(
    std::is_same<accumulator_input_t<sum_acc_type>, int>::value,
    "sum() input_type is int");
static_assert(
    std::is_same<accumulator_output_t<sum_acc_type>, int>::value,
    "sum() output_type is int");
static_assert(
    std::is_same<accumulator_output_t<mean_acc_type>, double>::value,
    "mean() output_type is double");
static_assert(
    std::is_same<accumulator_state_t<not_acc>, internal::nonesuch>::value,
    "extraction on a non-accumulator yields nonesuch");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// _v shorthands agree with the ::value form.
static_assert(
    is_accumulator_v<sum_acc_type>,
    "is_accumulator_v must agree with ::value");
static_assert(
    !is_accumulator_v<boxed_accumulator<int, int> >,
    "is_accumulator_v must reject boxed");
static_assert(
    is_boxed_accumulator_v<boxed_accumulator<int, int> >,
    "is_boxed_accumulator_v must agree with ::value");
static_assert(
    has_accumulator_typedefs_v<sum_acc_type>,
    "has_accumulator_typedefs_v must agree with ::value");
static_assert(
    has_accumulator_interface_v<sum_acc_type>,
    "has_accumulator_interface_v must agree with ::value");
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
// concepts mirror the traits.
static_assert(
    accumulator_typedefs<sum_acc_type>,
    "sum() satisfies accumulator_typedefs concept");
static_assert(
    accumulator_steppable<sum_acc_type>,
    "sum() satisfies accumulator_steppable concept");
static_assert(
    accumulator_finalizable<sum_acc_type>,
    "sum() satisfies accumulator_finalizable concept");
static_assert(
    accumulator_like<sum_acc_type>,
    "sum() satisfies accumulator_like concept");
static_assert(
    accumulator_like<mean_acc_type>,
    "mean() satisfies accumulator_like concept");
static_assert(
    !accumulator_typedefs<not_acc>,
    "a plain aggregate fails accumulator_typedefs concept");
static_assert(
    !accumulator_like<not_acc>,
    "a plain aggregate fails accumulator_like concept");
static_assert(
    !accumulator_like<boxed_accumulator<int, int> >,
    "boxed_accumulator fails accumulator_like (no typedefs)");
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///                RUNTIME PREDICATES                                        ///
///////////////////////////////////////////////////////////////////////////////

/*
test_traits_is_accumulator
  Exercises the is_accumulator composite trait at run time.
  Tests the following:
  - a pre-built unboxed accumulator is accepted
  - a boxed accumulator is rejected (it hides state_type)
  - a plain aggregate and a scalar are rejected
  - cv/ref qualifiers are stripped before inspection
*/
bool
test_traits_is_accumulator(
)
{
    D_INTERNAL_ACC_CHECK(is_accumulator<sum_acc_type>::value);
    D_INTERNAL_ACC_CHECK(is_accumulator<mean_acc_type>::value);
    D_INTERNAL_ACC_CHECK((!is_accumulator<boxed_accumulator<int, int> >::value));
    D_INTERNAL_ACC_CHECK(!is_accumulator<not_acc>::value);
    D_INTERNAL_ACC_CHECK(!is_accumulator<int>::value);
    D_INTERNAL_ACC_CHECK(is_accumulator<const sum_acc_type&>::value);
    D_INTERNAL_ACC_CHECK(is_accumulator<sum_acc_type&&>::value);

    return true;
}


/*
test_traits_is_boxed_accumulator
  Exercises the is_boxed_accumulator composite trait at run time.
  Tests the following:
  - a boxed accumulator is accepted
  - an unboxed accumulator is rejected (it exposes state_type)
  - a plain aggregate is rejected
*/
bool
test_traits_is_boxed_accumulator(
)
{
    D_INTERNAL_ACC_CHECK((is_boxed_accumulator<boxed_accumulator<int, int> >::value));
    D_INTERNAL_ACC_CHECK(
        (is_boxed_accumulator<boxed_accumulator<double, double> >::value));
    D_INTERNAL_ACC_CHECK((!is_boxed_accumulator<sum_acc_type>::value));
    D_INTERNAL_ACC_CHECK((!is_boxed_accumulator<not_acc>::value));

    return true;
}


/*
test_traits_member_detection
  Exercises the individual member/typedef detectors and the two composite
  helpers they feed.
  Tests the following:
  - typedef detectors split unboxed (has state_type) from boxed (does not)
  - method detectors see run() on boxed and state() on unboxed only
  - has_accumulator_typedefs / has_accumulator_interface compose correctly
*/
bool
test_traits_member_detection(
)
{
    D_INTERNAL_ACC_CHECK(has_state_type<sum_acc_type>::value);
    D_INTERNAL_ACC_CHECK(has_input_type<sum_acc_type>::value);
    D_INTERNAL_ACC_CHECK(has_output_type<sum_acc_type>::value);
    D_INTERNAL_ACC_CHECK(has_step_type<sum_acc_type>::value);
    D_INTERNAL_ACC_CHECK(has_final_type<sum_acc_type>::value);

    D_INTERNAL_ACC_CHECK((!has_state_type<boxed_accumulator<int, int> >::value));
    D_INTERNAL_ACC_CHECK((has_input_type<boxed_accumulator<int, int> >::value));
    D_INTERNAL_ACC_CHECK((has_output_type<boxed_accumulator<int, int> >::value));

    D_INTERNAL_ACC_CHECK(has_step_method<sum_acc_type>::value);
    D_INTERNAL_ACC_CHECK(has_finalize_method<sum_acc_type>::value);
    D_INTERNAL_ACC_CHECK(has_state_method<sum_acc_type>::value);
    D_INTERNAL_ACC_CHECK(has_step_fn_method<sum_acc_type>::value);
    D_INTERNAL_ACC_CHECK(has_finalize_fn_method<sum_acc_type>::value);

    D_INTERNAL_ACC_CHECK((has_run_method<boxed_accumulator<int, int> >::value));
    D_INTERNAL_ACC_CHECK((!has_state_method<boxed_accumulator<int, int> >::value));

    D_INTERNAL_ACC_CHECK(has_accumulator_typedefs<sum_acc_type>::value);
    D_INTERNAL_ACC_CHECK(
        (!has_accumulator_typedefs<boxed_accumulator<int, int> >::value));
    D_INTERNAL_ACC_CHECK(has_accumulator_interface<sum_acc_type>::value);
    D_INTERNAL_ACC_CHECK(
        (has_accumulator_interface<boxed_accumulator<int, int> >::value));

    return true;
}


/*
test_traits_type_extraction
  Exercises the accumulator_state_t / input_t / output_t aliases.
  Tests the following:
  - extraction recovers the inner typedefs for an unboxed accumulator
  - a double-producing accumulator reports a double output_type
  - extraction on a non-accumulator collapses to nonesuch
*/
bool
test_traits_type_extraction(
)
{
    D_INTERNAL_ACC_CHECK(
        (std::is_same<accumulator_state_t<sum_acc_type>, int>::value));
    D_INTERNAL_ACC_CHECK(
        (std::is_same<accumulator_input_t<sum_acc_type>, int>::value));
    D_INTERNAL_ACC_CHECK(
        (std::is_same<accumulator_output_t<sum_acc_type>, int>::value));
    D_INTERNAL_ACC_CHECK(
        (std::is_same<accumulator_output_t<mean_acc_type>, double>::value));
    D_INTERNAL_ACC_CHECK(
        (std::is_same<accumulator_state_t<not_acc>,
                      internal::nonesuch>::value));

    return true;
}


/*
test_traits_value_aliases
  Exercises the C++14 *_v variable-template shorthands.
  Tests the following:
  - each _v alias agrees with its ::value counterpart
  Under pre-C++14 the aliases are absent, so the body is a no-op that still
  reports success.
*/
bool
test_traits_value_aliases(
)
{
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    D_INTERNAL_ACC_CHECK(is_accumulator_v<sum_acc_type>);
    D_INTERNAL_ACC_CHECK((!is_accumulator_v<boxed_accumulator<int, int> >));
    D_INTERNAL_ACC_CHECK((is_boxed_accumulator_v<boxed_accumulator<int, int> >));
    D_INTERNAL_ACC_CHECK(!is_boxed_accumulator_v<sum_acc_type>);
    D_INTERNAL_ACC_CHECK(has_accumulator_typedefs_v<sum_acc_type>);
    D_INTERNAL_ACC_CHECK(has_accumulator_interface_v<sum_acc_type>);
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return true;
}


/*
test_traits_concepts
  Exercises the C++20 accumulator concepts.
  Tests the following:
  - an unboxed accumulator satisfies every concept layer
  - a plain aggregate and a boxed accumulator fail accumulator_like
  Under pre-C++20 the concepts are absent, so the body is a no-op that still
  reports success.
*/
bool
test_traits_concepts(
)
{
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    D_INTERNAL_ACC_CHECK(accumulator_typedefs<sum_acc_type>);
    D_INTERNAL_ACC_CHECK(accumulator_steppable<sum_acc_type>);
    D_INTERNAL_ACC_CHECK(accumulator_finalizable<sum_acc_type>);
    D_INTERNAL_ACC_CHECK(accumulator_like<sum_acc_type>);
    D_INTERNAL_ACC_CHECK(accumulator_like<mean_acc_type>);
    D_INTERNAL_ACC_CHECK(!accumulator_typedefs<not_acc>);
    D_INTERNAL_ACC_CHECK(!accumulator_like<not_acc>);
    D_INTERNAL_ACC_CHECK((!accumulator_like<boxed_accumulator<int, int> >));
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

    return true;
}


/*
run_traits_tests
  Aggregates every traits-section test.
  Tests the following:
  - all is_accumulator / is_boxed_accumulator / detector / extraction /
    value-alias / concept tests pass
*/
bool
run_traits_tests(
)
{
    return ( test_traits_is_accumulator()       &&
             test_traits_is_boxed_accumulator() &&
             test_traits_member_detection()     &&
             test_traits_type_extraction()      &&
             test_traits_value_aliases()        &&
             test_traits_concepts() );
}


NS_END  // testing
NS_END  // djinterp
