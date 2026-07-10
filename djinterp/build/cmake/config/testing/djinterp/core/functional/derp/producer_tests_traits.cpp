/******************************************************************************
* djinterp [functional]                            producer_tests_traits.cpp
*
*   Tests for the producer detection traits: is_producer_step, is_producer,
* producer_value_type, and the C++20 concepts producer_step_type / producer.
* Also defines the suite aggregator run_all_producer_tests().
*
*   The detection is compile-time, so the bulk of these checks are
* static_assert (which fire at build time). A representative subset is also
* re-checked at runtime so the harness registry reflects this section.
*
* path:      /src/functional/producer_tests_traits.cpp
******************************************************************************/

#include "./producer_tests.hpp"

#include <vector>
#include <type_traits>


NS_DJINTERP
NS_TESTING


// Helper types used in the negative trait checks below.

// step_returning_no_value_type
//   a const-nullary callable returning a producer_step but lacking the
// nested value_type member -> not a producer.
struct step_returning_no_value_type
{
    producer_step<int> operator()() const { return no_step<int>(); }
};

// wrong_return_producer
//   has value_type and is const-nullary, but returns a non-step -> not a
// producer.
struct wrong_return_producer
{
    typedef int value_type;
    int operator()() const { return 0; }
};

// needs_argument_producer
//   has value_type and returns a step, but requires an argument (not
// nullary) -> not a producer.
struct needs_argument_producer
{
    typedef int value_type;
    producer_step<int> operator()(int) const { return no_step<int>(); }
};

// non_const_call_producer
//   has value_type and returns a step from a nullary call operator, but the
// operator is non-const; the protocol pulls through a const reference, so
// this must NOT qualify.
struct non_const_call_producer
{
    typedef int value_type;
    producer_step<int> operator()() { return no_step<int>(); }
};


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// Concrete real producer types.
typedef decltype(range(1, 4))                            range_t;
typedef decltype(single(0))                              single_t;
typedef decltype(repeat(0))                              repeat_t;
typedef decltype(empty<int>())                           empty_t;
typedef decltype(take_n(repeat(0), 1))                   take_n_t;
typedef decltype(transform(range(1, 2), times_two()))    transform_t;

// ---- is_producer_step ----
static_assert(is_producer_step<producer_step<int> >::value,
              "producer_step<int> is a step");
static_assert(is_producer_step<producer_step<std::vector<int> > >::value,
              "producer_step of a vector is a step");
static_assert(is_producer_step<const producer_step<int>& >::value,
              "cv/ref forms decay to the same answer");
static_assert(!is_producer_step<int>::value, "int is not a step");
static_assert(!is_producer_step<range_t>::value,
              "a producer is not itself a step");

// ---- is_producer (positive) ----
static_assert(is_producer<range_t>::value,     "range is a producer");
static_assert(is_producer<single_t>::value,    "single is a producer");
static_assert(is_producer<repeat_t>::value,    "repeat is a producer");
static_assert(is_producer<empty_t>::value,     "empty is a producer");
static_assert(is_producer<take_n_t>::value,    "take_n is a producer");
static_assert(is_producer<transform_t>::value, "transform is a producer");
static_assert(is_producer<const range_t&>::value,
              "cv/ref forms decay to the same answer");

// ---- is_producer (negative) ----
static_assert(!is_producer<int>::value, "int is not a producer");
static_assert(!is_producer<producer_step<int> >::value,
              "a step is not a producer");
static_assert(!is_producer<step_returning_no_value_type>::value,
              "missing value_type -> not a producer");
static_assert(!is_producer<wrong_return_producer>::value,
              "non-step return -> not a producer");
static_assert(!is_producer<needs_argument_producer>::value,
              "non-nullary -> not a producer");
static_assert(!is_producer<non_const_call_producer>::value,
              "non-const call operator -> not a producer");

// ---- producer_value_type ----
static_assert(std::is_same<producer_value_type<range_t>::type, int>::value,
              "range emits int");
static_assert(
    std::is_same<producer_value_type<transform_t>::type, int>::value,
    "transform of times_two over int emits int");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
static_assert(is_producer_v<range_t>,                 "is_producer_v range");
static_assert(!is_producer_v<int>,                    "is_producer_v int");
static_assert(is_producer_step_v<producer_step<int> >, "is_producer_step_v");
static_assert(!is_producer_step_v<range_t>,           "is_producer_step_v neg");
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
// ---- C++20 concepts mirror the traits ----
static_assert(producer<range_t>,            "producer concept: range");
static_assert(!producer<int>,               "producer concept: int rejected");
static_assert(!producer<producer_step<int> >,
              "producer concept: a step is not a producer");
static_assert(producer_step_type<producer_step<int> >,
              "producer_step_type concept");
static_assert(!producer_step_type<range_t>,
              "producer_step_type concept: producer rejected");

// concept usable as a constraint
template<producer _P>
std::size_t length_of(_P _p)
{
    std::size_t n = 0;

    while (_p().has_value)
    {
        ++n;
    }

    return n;
}
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


/*
test_producer_traits
  Tests the detection traits and concepts (a runtime mirror of the
  compile-time static_asserts above, so the harness registry reflects this
  section).
  Tests the following:
  - is_producer_step recognizes producer_step specializations and rejects
    other types
  - is_producer accepts every real producer type and rejects non-producers,
    including the four structural near-misses (missing value_type, wrong
    return, non-nullary, non-const call)
  - producer_value_type extracts the emitted element type
  - the _v aliases agree with ::value (C++14+)
  - the producer / producer_step_type concepts mirror the traits and are
    usable as constraints (C++20+)
*/
void
test_producer_traits(
    test::test_handler& _h
)
{

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    // is_producer_step
    D_TEST_CHECK(_h, (is_producer_step<producer_step<int> >::value));
    D_TEST_CHECK(_h, (is_producer_step<int>::value == false));
    D_TEST_CHECK(_h, (is_producer_step<range_t>::value == false));

    // is_producer (positive)
    D_TEST_CHECK(_h, is_producer<range_t>::value);
    D_TEST_CHECK(_h, is_producer<single_t>::value);
    D_TEST_CHECK(_h, is_producer<empty_t>::value);
    D_TEST_CHECK(_h, is_producer<take_n_t>::value);
    D_TEST_CHECK(_h, is_producer<transform_t>::value);
    D_TEST_CHECK(_h, (is_producer<const range_t&>::value));

    // is_producer (negative)
    D_TEST_CHECK(_h, (is_producer<int>::value == false));
    D_TEST_CHECK(_h,
        (is_producer<producer_step<int> >::value == false));
    D_TEST_CHECK(_h,
        (is_producer<step_returning_no_value_type>::value == false));
    D_TEST_CHECK(_h,
        (is_producer<wrong_return_producer>::value == false));
    D_TEST_CHECK(_h,
        (is_producer<needs_argument_producer>::value == false));
    D_TEST_CHECK(_h,
        (is_producer<non_const_call_producer>::value == false));

    // producer_value_type
    D_TEST_CHECK(_h,
        (std::is_same<producer_value_type<range_t>::type, int>::value));

#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    // concept-constrained call compiles and runs
    {
        auto r = range(0, 5);
        D_TEST_CHECK(_h, length_of(r) == 5);
    }
#endif  // D_ENV_LANG_IS_CPP20_OR_HIGHER
#else
    (void)_h;  // producer.hpp requires C++11+; nothing to test under C++98
#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER

    return;
}


NS_END  // testing
NS_END  // djinterp
