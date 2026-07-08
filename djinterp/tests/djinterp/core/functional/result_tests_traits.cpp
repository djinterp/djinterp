/******************************************************************************
* djinterp [test]                                      result_tests_traits.cpp
*
* SFINAE structural trait tests for result.hpp section II:
*   is_result<T>, result_value_type<R> / result_error_type<R>,
*   is_result_value_mapper<F, T>, is_result_error_mapper<F, E>, the
*   variable-template shorthands, and the C++20 concept parallels
*   (result_type / result_value_mapper_for / result_error_mapper_for).
*
*   Compile-time guarantees are asserted at file scope via static_assert; the
* runtime section mirrors them so the counts roll into the report and the
* gated variable-template / concept paths are exercised when available.
******************************************************************************/

#include <string>
#include <cstddef>
#include <type_traits>
#include <vector>
#include "./result_tests.hpp"


NS_DJINTERP
NS_TESTING


using ri = result<int, std::string>;


// ---- is_result (structural), always available ----
static_assert(is_result<ri>::value,
    "is_result: result<int,string> is a result");
static_assert(is_result<const ri&>::value,
    "is_result: cv/ref-qualified result is still a result");
static_assert(is_result<result<std::string, int>>::value,
    "is_result: order-swapped result is a result");
static_assert(!is_result<int>::value,
    "is_result: int is not a result");
static_assert(!is_result<std::string>::value,
    "is_result: string is not a result");

// ---- type extractors ----
static_assert(std::is_same<result_value_type_t<ri>, int>::value,
    "result_value_type_t: yields the success type");
static_assert(std::is_same<result_error_type_t<ri>, std::string>::value,
    "result_error_type_t: yields the error type");
static_assert(
    std::is_same<result_value_type_t<const ri&>, int>::value,
    "result_value_type_t: decays cv/ref before extracting");

// ---- mapper traits ----
static_assert(is_result_value_mapper<fn_double, int>::value,
    "is_result_value_mapper: int->int callable is a value mapper");
static_assert(is_result_value_mapper<fn_to_string, int>::value,
    "is_result_value_mapper: int->string callable is a value mapper");
static_assert(!is_result_value_mapper<fn_string_only, int>::value,
    "is_result_value_mapper: string-only callable is not callable with int");
static_assert(!is_result_value_mapper<int, int>::value,
    "is_result_value_mapper: a non-callable is not a value mapper");

static_assert(is_result_error_mapper<fn_err_len, std::string>::value,
    "is_result_error_mapper: string->size_t callable is an error mapper");
static_assert(is_result_error_mapper<fn_err_prefix, std::string>::value,
    "is_result_error_mapper: string->string callable is an error mapper");
static_assert(!is_result_error_mapper<fn_double, std::string>::value,
    "is_result_error_mapper: int-only callable is not callable with string");


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
static_assert(is_result_v<ri>,
    "is_result_v: positive");
static_assert(!is_result_v<int>,
    "is_result_v: negative");
static_assert(is_result_value_mapper_v<fn_double, int>,
    "is_result_value_mapper_v: positive");
static_assert(!is_result_value_mapper_v<fn_string_only, int>,
    "is_result_value_mapper_v: negative");
static_assert(is_result_error_mapper_v<fn_err_len, std::string>,
    "is_result_error_mapper_v: positive");
static_assert(!is_result_error_mapper_v<fn_double, std::string>,
    "is_result_error_mapper_v: negative");
#endif


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
static_assert(result_type<ri>,
    "result_type: positive");
static_assert(!result_type<int>,
    "result_type: negative");
static_assert(result_value_mapper_for<fn_double, int>,
    "result_value_mapper_for: positive");
static_assert(!result_value_mapper_for<fn_string_only, int>,
    "result_value_mapper_for: negative");
static_assert(result_error_mapper_for<fn_err_len, std::string>,
    "result_error_mapper_for: positive");
static_assert(!result_error_mapper_for<fn_double, std::string>,
    "result_error_mapper_for: negative");
#endif


void test_traits(test::test_handler& _h)
{
    // ---- is_result ----
    test::record_assertion(_h, is_result<ri>::value,
        "trait: is_result true for result<int,string>");
    test::record_assertion(_h, is_result<const ri&>::value,
        "trait: is_result strips cv/ref before matching");
    test::record_assertion(_h, !is_result<int>::value,
        "trait: is_result false for a plain value type");

    // ---- extractors ----
    test::record_assertion(_h,
        std::is_same<result_value_type_t<ri>, int>::value,
        "trait: result_value_type_t yields the success type");
    test::record_assertion(_h,
        std::is_same<result_error_type_t<ri>, std::string>::value,
        "trait: result_error_type_t yields the error type");

    // ---- mappers ----
    test::record_assertion(_h,
        is_result_value_mapper<fn_double, int>::value,
        "trait: is_result_value_mapper true for an int->int callable");
    test::record_assertion(_h,
        !is_result_value_mapper<fn_string_only, int>::value,
        "trait: is_result_value_mapper false when not callable with int");
    test::record_assertion(_h,
        is_result_error_mapper<fn_err_len, std::string>::value,
        "trait: is_result_error_mapper true for a string->size_t callable");
    test::record_assertion(_h,
        !is_result_error_mapper<fn_double, std::string>::value,
        "trait: is_result_error_mapper false when not callable with string");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    test::record_assertion(_h,
        is_result_v<ri> && !is_result_v<int>,
        "trait: is_result_v matches the struct form");
    test::record_assertion(_h,
        is_result_value_mapper_v<fn_double, int> &&
        !is_result_value_mapper_v<fn_string_only, int>,
        "trait: is_result_value_mapper_v matches the struct form");
    test::record_assertion(_h,
        is_result_error_mapper_v<fn_err_len, std::string> &&
        !is_result_error_mapper_v<fn_double, std::string>,
        "trait: is_result_error_mapper_v matches the struct form");
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    test::record_assertion(_h,
        result_type<ri> && !result_type<int>,
        "trait: result_type concept matches is_result");
    test::record_assertion(_h,
        result_value_mapper_for<fn_double, int> &&
        !result_value_mapper_for<fn_string_only, int>,
        "trait: result_value_mapper_for concept matches the trait");
    test::record_assertion(_h,
        result_error_mapper_for<fn_err_len, std::string> &&
        !result_error_mapper_for<fn_double, std::string>,
        "trait: result_error_mapper_for concept matches the trait");
#endif

    return;
}


NS_END  // testing
NS_END  // djinterp
