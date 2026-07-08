/******************************************************************************
* djinterp [test]                                    pipeline_tests_errors.cpp
*
* Error-propagation tests. An errored pipeline (from error(code)) carries its
* state through the chainable operations and short-circuits the queries.
*
*   test_error_propagation -- map/filter/take/slice/partition/zip_with carry
*                             the error and code; fold returns init; for_each
*                             no-ops; any/all/none/count and group_by take
*                             their documented errored-pipeline values. Note
*                             in particular that all() on an errored pipeline
*                             is false (not vacuously true).
******************************************************************************/

#include <vector>
#include <map>
#include <utility>
#include "./pipeline_tests.hpp"


NS_DJINTERP
NS_TESTING


using pi = function_pipeline<int>;


void test_error_propagation(test::test_handler& _h)
{
    const int code = 99;

    // map propagates the error and code into the new element type
    function_pipeline<int> mapped = pi::error(code).map(fn_double{});
    test::record_assertion(_h,
        mapped.has_error() && mapped.error_code() == code,
        "error: map propagates the error and code");

    // filter propagates
    pi filtered = pi::error(code).filter(fn_is_even{});
    test::record_assertion(_h,
        filtered.has_error() && filtered.error_code() == code,
        "error: filter propagates the error");

    // take propagates
    pi taken = pi::error(code).take(3);
    test::record_assertion(_h,
        taken.has_error() && taken.error_code() == code,
        "error: take propagates the error");

    // reversed / sorted / distinct propagate
    test::record_assertion(_h,
        pi::error(code).reversed().has_error()
        && pi::error(code).sorted().has_error()
        && pi::error(code).distinct().has_error(),
        "error: reversed / sorted / distinct propagate the error");

    // fold returns the init unchanged
    test::record_assertion(_h, pi::error(code).fold(5, fn_add{}) == 5,
        "error: fold returns the initial value");

    // for_each is a no-op and returns the same (errored) pipeline
    long sink = 0;
    pi errored = pi::error(code);
    const pi& same = errored.for_each(accumulate_into(&sink));
    test::record_assertion(_h, sink == 0 && &same == &errored,
        "error: for_each performs no work on an errored pipeline");

    // query short-circuits: any=false, all=false (NOT vacuously true),
    // none=true, count=0
    test::record_assertion(_h,
        pi::error(code).any(fn_is_even{}) == false
        && pi::error(code).all(fn_is_even{}) == false
        && pi::error(code).none(fn_is_even{}) == true
        && pi::error(code).count(fn_is_even{}) == 0u,
        "error: any/all/none/count take their errored values (all is false)");

    // group_by yields an empty map
    std::map<int, std::vector<int>> g = pi::error(code).group_by(fn_parity{});
    test::record_assertion(_h, g.empty(),
        "error: group_by yields an empty map");

    // partition_pipe yields a pair of errored pipelines
    std::pair<pi, pi> parts = pi::error(code).partition_pipe(fn_is_even{});
    test::record_assertion(_h,
        parts.first.has_error() && parts.first.error_code() == code
        && parts.second.has_error() && parts.second.error_code() == code,
        "error: partition_pipe yields two errored pipelines");

    // zip_with: a healthy pipeline zipped against an errored one inherits the
    // errored operand's code
    function_pipeline<int> zipped =
        pi::from({ 1, 2 }).zip_with(pi::error(code), fn_mul{});
    test::record_assertion(_h,
        zipped.has_error() && zipped.error_code() == code,
        "error: zip_with propagates an errored operand's code");

    return;
}


NS_END  // testing
NS_END  // djinterp
