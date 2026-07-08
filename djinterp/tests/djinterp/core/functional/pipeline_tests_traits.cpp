/******************************************************************************
* djinterp [test]                                    pipeline_tests_traits.cpp
*
* SFINAE structural trait & concept tests for pipeline.hpp section III:
*   is_pipeline, pipeline_value_type, is_pipeline_mapper, is_pipeline_predicate,
*   the variable-template shorthands, and the C++20 concept parallels
*   (pipeline_type / pipeline_mapper_for / pipeline_predicate_for).
*
*   Compile-time guarantees are asserted at file scope via static_assert; the
* runtime section mirrors them so the counts roll into the report and the
* gated variable-template / concept paths are exercised when available.
******************************************************************************/

#include <vector>
#include <string>
#include <type_traits>
#include "./pipeline_tests.hpp"


NS_DJINTERP
NS_TESTING


using pi = function_pipeline<int>;


// ---- is_pipeline (structural) ----
static_assert(is_pipeline<pi>::value,
    "is_pipeline: function_pipeline<int> is a pipeline");
static_assert(is_pipeline<const pi&>::value,
    "is_pipeline: cv/ref-qualified pipeline is still a pipeline");
static_assert(is_pipeline<function_pipeline<std::string>>::value,
    "is_pipeline: pipeline over another element type is a pipeline");
static_assert(!is_pipeline<int>::value,
    "is_pipeline: int is not a pipeline");
static_assert(!is_pipeline<std::vector<int>>::value,
    "is_pipeline: a vector is not a pipeline");

// ---- pipeline_value_type ----
static_assert(std::is_same<pipeline_value_type_t<pi>, int>::value,
    "pipeline_value_type_t: yields the element type");
static_assert(
    std::is_same<pipeline_value_type_t<const pi&>, int>::value,
    "pipeline_value_type_t: decays cv/ref before extracting");
static_assert(
    std::is_same<
        pipeline_value_type_t<function_pipeline<std::string>>,
        std::string>::value,
    "pipeline_value_type_t: tracks the element type");

// ---- is_pipeline_mapper ----
static_assert(is_pipeline_mapper<fn_double, int>::value,
    "is_pipeline_mapper: int->int callable is a mapper over int");
static_assert(is_pipeline_mapper<fn_to_string, int>::value,
    "is_pipeline_mapper: int->string callable is a mapper over int");
static_assert(!is_pipeline_mapper<fn_string_only, int>::value,
    "is_pipeline_mapper: string-only callable is not a mapper over int");
static_assert(!is_pipeline_mapper<int, int>::value,
    "is_pipeline_mapper: a non-callable is not a mapper");

// ---- is_pipeline_predicate ----
static_assert(is_pipeline_predicate<fn_is_even, int>::value,
    "is_pipeline_predicate: bool-returning callable is a predicate");
static_assert(!is_pipeline_predicate<fn_to_string, int>::value,
    "is_pipeline_predicate: non-bool-convertible result is not a predicate");
static_assert(!is_pipeline_predicate<fn_string_only, int>::value,
    "is_pipeline_predicate: not callable with int is not a predicate");


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
static_assert(is_pipeline_v<pi> && !is_pipeline_v<int>,
    "is_pipeline_v: matches the struct form");
static_assert(is_pipeline_mapper_v<fn_double, int>
              && !is_pipeline_mapper_v<fn_string_only, int>,
    "is_pipeline_mapper_v: matches the struct form");
static_assert(is_pipeline_predicate_v<fn_is_even, int>
              && !is_pipeline_predicate_v<fn_to_string, int>,
    "is_pipeline_predicate_v: matches the struct form");
#endif


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
static_assert(pipeline_type<pi> && !pipeline_type<int>,
    "pipeline_type: concept matches is_pipeline");
static_assert(pipeline_mapper_for<fn_double, int>
              && !pipeline_mapper_for<fn_string_only, int>,
    "pipeline_mapper_for: concept matches is_pipeline_mapper");
static_assert(pipeline_predicate_for<fn_is_even, int>
              && !pipeline_predicate_for<fn_to_string, int>,
    "pipeline_predicate_for: concept matches is_pipeline_predicate");
#endif


void test_traits(test::test_handler& _h)
{
    // is_pipeline
    test::record_assertion(_h,
        is_pipeline<pi>::value && !is_pipeline<int>::value,
        "trait: is_pipeline distinguishes pipelines from scalars");
    test::record_assertion(_h, is_pipeline<const pi&>::value,
        "trait: is_pipeline strips cv/ref before matching");

    // pipeline_value_type
    test::record_assertion(_h,
        std::is_same<pipeline_value_type_t<pi>, int>::value,
        "trait: pipeline_value_type_t extracts the element type");

    // mapper
    test::record_assertion(_h,
        is_pipeline_mapper<fn_double, int>::value
        && !is_pipeline_mapper<fn_string_only, int>::value,
        "trait: is_pipeline_mapper detects valid mappers");

    // predicate
    test::record_assertion(_h,
        is_pipeline_predicate<fn_is_even, int>::value
        && !is_pipeline_predicate<fn_to_string, int>::value,
        "trait: is_pipeline_predicate requires a bool-convertible result");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    test::record_assertion(_h,
        is_pipeline_v<pi> && !is_pipeline_v<double>,
        "trait: is_pipeline_v matches the struct form");
    test::record_assertion(_h,
        is_pipeline_predicate_v<fn_is_even, int>
        && !is_pipeline_predicate_v<fn_string_only, int>,
        "trait: is_pipeline_predicate_v matches the struct form");
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    test::record_assertion(_h,
        pipeline_type<pi> && !pipeline_type<int>,
        "trait: pipeline_type concept matches is_pipeline");
    test::record_assertion(_h,
        pipeline_mapper_for<fn_double, int>
        && pipeline_predicate_for<fn_is_even, int>,
        "trait: mapper/predicate concepts match their traits");
#endif

    return;
}


NS_END  // testing
NS_END  // djinterp
