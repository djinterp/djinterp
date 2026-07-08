#include "extractor_tests.hpp"

// std
#include <type_traits>


NS_DJINTERP
NS_TESTING


// not_ext
//   struct: negative fixture. No call operator, so it models no extractor
// shape.
struct not_ext
{
    int x;
};


// ---- compile-time fixtures ----
//   Concrete helper types produced by the factories. Naming them lets the
// static_asserts read as plain type predicates. Every fixture is built from
// named functors (never lambdas) so decltype in this unevaluated context is
// well-formed from C++11 upward.
typedef decltype(extractors::from_member(&person::age))            age_ext_t;
typedef decltype(extractors::from_function(get_dept()))            dept_ext_t;
typedef decltype(extractors::identity<person>())                   identity_t;
typedef decltype(extractors::filtered(get_age(), is_adult()))      filtered_t;
typedef decltype(extractors::guarded(get_age(), dept_nonzero()))   guarded_t;
typedef decltype(extractors::try_extract(checked_age()))           try_t;
typedef decltype(extractors::mapped(get_age(), times_two()))       mapped_t;


///////////////////////////////////////////////////////////////////////////////
///                COMPILE-TIME ASSERTIONS                                   ///
///////////////////////////////////////////////////////////////////////////////
//   The traits are a compile-time contract, so the primary verification is a
// battery of static_asserts. The runtime predicates further below re-check
// the same facts through ::value so the section participates in the normal
// pass/fail tally as well.

// is_extractor: factory / combinator outputs and raw functors model it over
// person; non-callables and scalars do not.
static_assert(
    is_extractor<age_ext_t, person>::value,
    "from_member(age) is an extractor over person");
static_assert(
    is_extractor<get_age, person>::value,
    "a raw key functor is an extractor over person");
static_assert(
    is_extractor<filtered_t, person>::value,
    "filtered(...) is an extractor over person");
static_assert(
    !is_extractor<not_ext, person>::value,
    "a non-callable aggregate is not an extractor");
static_assert(
    !is_extractor<int, person>::value,
    "a scalar is not an extractor");

// cv / ref qualifiers are stripped before inspection.
static_assert(
    is_extractor<const age_ext_t&, person>::value,
    "is_extractor sees through const-ref");

// extractor_result_t: recovers the (decayed) extracted type.
static_assert(
    std::is_same<extractor_result_t<age_ext_t, person>, int>::value,
    "age extractor yields int");
static_assert(
    std::is_same<extractor_result_t<mapped_t, person>, int>::value,
    "mapped(age, times_two) yields int");
static_assert(
    std::is_same<extractor_result_t<filtered_t, person>,
                 maybe<int> >::value,
    "filtered(age, is_adult) yields maybe<int>");

// is_maybe: recognises a maybe specialization.
static_assert(
    is_maybe<maybe<int> >::value,
    "maybe<int> is a maybe");
static_assert(
    is_maybe<maybe<person> >::value,
    "maybe<person> is a maybe");
static_assert(
    !is_maybe<int>::value,
    "int is not a maybe");
static_assert(
    !is_maybe<person>::value,
    "person is not a maybe");

// is_maybe_extractor: the partial / safe extractors model it; total ones do
// not.
static_assert(
    is_maybe_extractor<filtered_t, person>::value,
    "filtered is a maybe-extractor");
static_assert(
    is_maybe_extractor<guarded_t, person>::value,
    "guarded is a maybe-extractor");
static_assert(
    is_maybe_extractor<try_t, person>::value,
    "try_extract is a maybe-extractor");
static_assert(
    !is_maybe_extractor<age_ext_t, person>::value,
    "from_member is total, not a maybe-extractor");
static_assert(
    !is_maybe_extractor<mapped_t, person>::value,
    "mapped(age, times_two) is total, not a maybe-extractor");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
// _v shorthands agree with the ::value form.
static_assert(
    is_extractor_v<age_ext_t, person>,
    "is_extractor_v agrees with ::value");
static_assert(
    !is_extractor_v<not_ext, person>,
    "is_extractor_v rejects a non-callable");
static_assert(
    is_maybe_v<maybe<int> >,
    "is_maybe_v agrees with ::value");
static_assert(
    is_maybe_extractor_v<filtered_t, person>,
    "is_maybe_extractor_v agrees with ::value");
static_assert(
    !is_maybe_extractor_v<age_ext_t, person>,
    "is_maybe_extractor_v rejects a total extractor");
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
// concepts mirror the traits.
static_assert(
    extractor_c<age_ext_t, person>,
    "from_member satisfies extractor_c");
static_assert(
    extractor_c<get_age, person>,
    "a raw key functor satisfies extractor_c");
static_assert(
    !extractor_c<not_ext, person>,
    "a non-callable fails extractor_c");
static_assert(
    maybe_extractor_c<filtered_t, person>,
    "filtered satisfies maybe_extractor_c");
static_assert(
    maybe_extractor_c<try_t, person>,
    "try_extract satisfies maybe_extractor_c");
static_assert(
    !maybe_extractor_c<age_ext_t, person>,
    "a total extractor fails maybe_extractor_c");
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///                RUNTIME PREDICATES                                        ///
///////////////////////////////////////////////////////////////////////////////

/*
test_traits_is_extractor
  Exercises the is_extractor trait at run time.
  Tests the following:
  - factory / combinator outputs and raw functors are accepted over person
  - non-callables and scalars are rejected
  - cv/ref qualifiers are stripped before inspection
*/
bool
test_traits_is_extractor(
)
{
    D_INTERNAL_EXT_CHECK((is_extractor<age_ext_t, person>::value));
    D_INTERNAL_EXT_CHECK((is_extractor<dept_ext_t, person>::value));
    D_INTERNAL_EXT_CHECK((is_extractor<identity_t, person>::value));
    D_INTERNAL_EXT_CHECK((is_extractor<get_age, person>::value));
    D_INTERNAL_EXT_CHECK((is_extractor<filtered_t, person>::value));
    D_INTERNAL_EXT_CHECK((!is_extractor<not_ext, person>::value));
    D_INTERNAL_EXT_CHECK((!is_extractor<int, person>::value));
    D_INTERNAL_EXT_CHECK((is_extractor<const age_ext_t&, person>::value));

    return true;
}


/*
test_traits_result_type
  Exercises the extractor_result_t alias at run time.
  Tests the following:
  - total extractors report their decayed value type (int)
  - a maybe-producing extractor reports maybe<int>
*/
bool
test_traits_result_type(
)
{
    D_INTERNAL_EXT_CHECK(
        (std::is_same<extractor_result_t<age_ext_t, person>, int>::value));
    D_INTERNAL_EXT_CHECK(
        (std::is_same<extractor_result_t<mapped_t, person>, int>::value));
    D_INTERNAL_EXT_CHECK(
        (std::is_same<extractor_result_t<filtered_t, person>,
                      maybe<int> >::value));

    return true;
}


/*
test_traits_is_maybe
  Exercises the is_maybe trait at run time.
  Tests the following:
  - maybe specializations are recognised
  - plain value types are rejected
*/
bool
test_traits_is_maybe(
)
{
    D_INTERNAL_EXT_CHECK((is_maybe<maybe<int> >::value));
    D_INTERNAL_EXT_CHECK((is_maybe<maybe<person> >::value));
    D_INTERNAL_EXT_CHECK((!is_maybe<int>::value));
    D_INTERNAL_EXT_CHECK((!is_maybe<person>::value));

    return true;
}


/*
test_traits_is_maybe_extractor
  Exercises the is_maybe_extractor trait at run time.
  Tests the following:
  - filtered / guarded / try_extract are accepted
  - total extractors (from_member, mapped) are rejected
*/
bool
test_traits_is_maybe_extractor(
)
{
    D_INTERNAL_EXT_CHECK((is_maybe_extractor<filtered_t, person>::value));
    D_INTERNAL_EXT_CHECK((is_maybe_extractor<guarded_t, person>::value));
    D_INTERNAL_EXT_CHECK((is_maybe_extractor<try_t, person>::value));
    D_INTERNAL_EXT_CHECK((!is_maybe_extractor<age_ext_t, person>::value));
    D_INTERNAL_EXT_CHECK((!is_maybe_extractor<mapped_t, person>::value));

    return true;
}


/*
test_traits_value_aliases
  Exercises the C++14 *_v variable-template shorthands.
  Tests the following:
  - each _v alias agrees with its ::value counterpart
  Absent before C++14, so the body is a no-op that still reports success.
*/
bool
test_traits_value_aliases(
)
{
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    D_INTERNAL_EXT_CHECK((is_extractor_v<age_ext_t, person>));
    D_INTERNAL_EXT_CHECK((!is_extractor_v<not_ext, person>));
    D_INTERNAL_EXT_CHECK((is_maybe_v<maybe<int> >));
    D_INTERNAL_EXT_CHECK((!is_maybe_v<int>));
    D_INTERNAL_EXT_CHECK((is_maybe_extractor_v<filtered_t, person>));
    D_INTERNAL_EXT_CHECK((!is_maybe_extractor_v<age_ext_t, person>));
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    return true;
}


/*
test_traits_concepts
  Exercises the C++20 extractor concepts.
  Tests the following:
  - extractors and raw functors satisfy extractor_c; non-callables fail it
  - the maybe-producing extractors satisfy maybe_extractor_c; total ones fail
  Absent before C++20, so the body is a no-op that still reports success.
*/
bool
test_traits_concepts(
)
{
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    D_INTERNAL_EXT_CHECK((extractor_c<age_ext_t, person>));
    D_INTERNAL_EXT_CHECK((extractor_c<get_age, person>));
    D_INTERNAL_EXT_CHECK((!extractor_c<not_ext, person>));
    D_INTERNAL_EXT_CHECK((maybe_extractor_c<filtered_t, person>));
    D_INTERNAL_EXT_CHECK((maybe_extractor_c<try_t, person>));
    D_INTERNAL_EXT_CHECK((!maybe_extractor_c<age_ext_t, person>));
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

    return true;
}


/*
run_traits_tests
  Aggregates every traits-section test.
  Tests the following:
  - all is_extractor / result_type / is_maybe / is_maybe_extractor /
    value-alias / concept tests pass
*/
bool
run_traits_tests(
)
{
    return ( test_traits_is_extractor()        &&
             test_traits_result_type()          &&
             test_traits_is_maybe()             &&
             test_traits_is_maybe_extractor()   &&
             test_traits_value_aliases()        &&
             test_traits_concepts() );
}


NS_END  // testing
NS_END  // djinterp
