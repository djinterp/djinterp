/******************************************************************************
* djinterp [test]                                   fn_builder_tests_traits.cpp
*
* SFINAE structural trait & concept tests for fn_builder.hpp section V:
*   is_fn_builder, is_boxed_fn_builder, fn_builder_input_type,
*   fn_builder_current_type, is_fn_builder_mapper, is_fn_builder_predicate,
*   the variable-template shorthands, and the C++20 concept parallels.
*
*   Compile-time guarantees are asserted at file scope via static_assert; the
* runtime section mirrors them so the counts roll into the report and the
* gated variable-template / concept paths are exercised when available.
******************************************************************************/

#include <vector>
#include <string>
#include <type_traits>
#include "./fn_builder_tests.hpp"


NS_DJINTERP
NS_TESTING


// representative builder types (unevaluated decltype of factory expressions)
using seed_t      = decltype(fn_builder<int>::create());
using mapped_t    = decltype(fn_builder<int>::create().map(fn_double{}));
using restyled_t  = decltype(fn_builder<int>::create().map(fn_to_string{}));
using boxed_t     = boxed_fn_builder<int, int>;


// ---- is_fn_builder ----
static_assert(is_fn_builder<seed_t>::value,
    "is_fn_builder: a seeded builder is a builder");
static_assert(is_fn_builder<mapped_t>::value,
    "is_fn_builder: a composed builder is a builder");
static_assert(is_fn_builder<const seed_t&>::value,
    "is_fn_builder: cv/ref-qualified builder is still a builder");
static_assert(!is_fn_builder<int>::value,
    "is_fn_builder: int is not a builder");
static_assert(!is_fn_builder<boxed_t>::value,
    "is_fn_builder: a boxed builder is not a typed fn_builder");

// ---- is_boxed_fn_builder ----
static_assert(is_boxed_fn_builder<boxed_t>::value,
    "is_boxed_fn_builder: a boxed builder is detected");
static_assert(!is_boxed_fn_builder<seed_t>::value,
    "is_boxed_fn_builder: a typed builder is not boxed");
static_assert(!is_boxed_fn_builder<int>::value,
    "is_boxed_fn_builder: int is not boxed");

// ---- input / current type extractors ----
static_assert(std::is_same<fn_builder_input_type_t<seed_t>, int>::value,
    "fn_builder_input_type_t: seed input type is int");
static_assert(std::is_same<fn_builder_current_type_t<seed_t>, int>::value,
    "fn_builder_current_type_t: seed current type is int");
static_assert(
    std::is_same<fn_builder_input_type_t<restyled_t>, int>::value,
    "fn_builder_input_type_t: a type-changing chain keeps the input type");
static_assert(
    std::is_same<fn_builder_current_type_t<restyled_t>, std::string>::value,
    "fn_builder_current_type_t: tracks the current (output) element type");

// ---- mapper / predicate ----
static_assert(is_fn_builder_mapper<fn_double, int>::value,
    "is_fn_builder_mapper: int->int callable is a mapper over int");
static_assert(is_fn_builder_mapper<fn_to_string, int>::value,
    "is_fn_builder_mapper: int->string callable is a mapper over int");
static_assert(!is_fn_builder_mapper<fn_string_only, int>::value,
    "is_fn_builder_mapper: string-only callable is not a mapper over int");
static_assert(!is_fn_builder_mapper<int, int>::value,
    "is_fn_builder_mapper: a non-callable is not a mapper");

static_assert(is_fn_builder_predicate<fn_is_even, int>::value,
    "is_fn_builder_predicate: bool-returning callable is a predicate");
static_assert(!is_fn_builder_predicate<fn_to_string, int>::value,
    "is_fn_builder_predicate: non-bool-convertible result is not a predicate");
static_assert(!is_fn_builder_predicate<fn_string_only, int>::value,
    "is_fn_builder_predicate: not callable with int is not a predicate");


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
static_assert(is_fn_builder_v<seed_t> && !is_fn_builder_v<int>,
    "is_fn_builder_v: matches the struct form");
static_assert(is_boxed_fn_builder_v<boxed_t> && !is_boxed_fn_builder_v<seed_t>,
    "is_boxed_fn_builder_v: matches the struct form");
static_assert(is_fn_builder_mapper_v<fn_double, int>
              && !is_fn_builder_mapper_v<fn_string_only, int>,
    "is_fn_builder_mapper_v: matches the struct form");
static_assert(is_fn_builder_predicate_v<fn_is_even, int>
              && !is_fn_builder_predicate_v<fn_to_string, int>,
    "is_fn_builder_predicate_v: matches the struct form");
#endif


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
static_assert(fn_builder_type<seed_t> && !fn_builder_type<int>,
    "fn_builder_type: concept matches is_fn_builder");
static_assert(boxed_fn_builder_type<boxed_t> && !boxed_fn_builder_type<seed_t>,
    "boxed_fn_builder_type: concept matches is_boxed_fn_builder");
static_assert(fn_builder_mapper_for<fn_double, int>
              && !fn_builder_mapper_for<fn_string_only, int>,
    "fn_builder_mapper_for: concept matches is_fn_builder_mapper");
static_assert(fn_builder_predicate_for<fn_is_even, int>
              && !fn_builder_predicate_for<fn_to_string, int>,
    "fn_builder_predicate_for: concept matches is_fn_builder_predicate");
#endif


void test_traits(test::test_handler& _h)
{
    // structural detectors
    test::record_assertion(_h,
        is_fn_builder<seed_t>::value && !is_fn_builder<int>::value,
        "trait: is_fn_builder distinguishes builders from scalars");
    test::record_assertion(_h,
        is_boxed_fn_builder<boxed_t>::value
        && !is_boxed_fn_builder<seed_t>::value,
        "trait: is_boxed_fn_builder distinguishes boxed from typed builders");

    // extractors
    test::record_assertion(_h,
        std::is_same<fn_builder_input_type_t<restyled_t>, int>::value
        && std::is_same<
               fn_builder_current_type_t<restyled_t>, std::string>::value,
        "trait: input/current type extractors track a type-changing chain");

    // mapper / predicate
    test::record_assertion(_h,
        is_fn_builder_mapper<fn_double, int>::value
        && !is_fn_builder_mapper<fn_string_only, int>::value,
        "trait: is_fn_builder_mapper detects valid mappers");
    test::record_assertion(_h,
        is_fn_builder_predicate<fn_is_even, int>::value
        && !is_fn_builder_predicate<fn_to_string, int>::value,
        "trait: is_fn_builder_predicate requires a bool-convertible result");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    test::record_assertion(_h,
        is_fn_builder_v<mapped_t> && !is_fn_builder_v<double>,
        "trait: is_fn_builder_v matches the struct form");
    test::record_assertion(_h,
        is_fn_builder_predicate_v<fn_is_even, int>
        && !is_fn_builder_predicate_v<fn_string_only, int>,
        "trait: is_fn_builder_predicate_v matches the struct form");
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    test::record_assertion(_h,
        fn_builder_type<seed_t> && !fn_builder_type<int>,
        "trait: fn_builder_type concept matches is_fn_builder");
    test::record_assertion(_h,
        fn_builder_mapper_for<fn_double, int>
        && fn_builder_predicate_for<fn_is_even, int>,
        "trait: mapper/predicate concepts match their traits");
#endif

    return;
}


NS_END  // testing
NS_END  // djinterp
