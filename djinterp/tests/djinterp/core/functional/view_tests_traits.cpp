/******************************************************************************
* djinterp [test]                                        view_tests_traits.cpp
*
* SFINAE trait & concept tests for view.hpp (sections I and VIII):
*   is_view, has_begin_end, is_pipeable_to_view, view_value_type, is_adapter,
*   is_terminal, the variable-template shorthands, and the C++20 concept
*   parallels (view_type / view_adapter / view_terminal / pipeable_to_view).
*
*   Compile-time guarantees are asserted at file scope via static_assert; the
* runtime sections mirror them so the counts roll into the report and the
* gated variable-template / concept paths are exercised when available.
******************************************************************************/

#include <vector>
#include <type_traits>
#include "./view_tests.hpp"


NS_DJINTERP
NS_TESTING


// adapter / terminal instance types obtained from their factory functions
// (unevaluated -- decltype only).
using transform_adapter_t = decltype(views::transform(fn_double{}));
using take_adapter_t       = decltype(views::take(std::size_t(3)));
using to_vector_terminal_t = decltype(to_vector());
using count_terminal_t     = decltype(count());


// ---- is_view ----
static_assert(is_view<single_view<int>>::value,
    "is_view: single_view is a view");
static_assert(is_view<iota_view<int>>::value,
    "is_view: iota_view is a view");
static_assert(is_view<ref_view<std::vector<int>>>::value,
    "is_view: ref_view is a view");
static_assert(!is_view<int>::value,
    "is_view: int is not a view");
static_assert(!is_view<std::vector<int>>::value,
    "is_view: vector is not a view");

// ---- has_begin_end ----
static_assert(has_begin_end<std::vector<int>>::value,
    "has_begin_end: vector is container-like");
static_assert(!has_begin_end<int>::value,
    "has_begin_end: int is not container-like");

// ---- is_pipeable_to_view (view OR container) ----
static_assert(is_pipeable_to_view<std::vector<int>>::value,
    "is_pipeable_to_view: a container is a pipeline source");
static_assert(is_pipeable_to_view<single_view<int>>::value,
    "is_pipeable_to_view: a view is a pipeline source");
static_assert(!is_pipeable_to_view<int>::value,
    "is_pipeable_to_view: a scalar is not a pipeline source");

// ---- view_value_type ----
static_assert(std::is_same<view_value_type_t<single_view<int>>, int>::value,
    "view_value_type_t: single_view<int> yields int");
static_assert(std::is_same<view_value_type_t<iota_view<long>>, long>::value,
    "view_value_type_t: iota_view<long> yields long");
static_assert(
    std::is_same<view_value_type_t<ref_view<std::vector<int>>>, int>::value,
    "view_value_type_t: ref_view over vector<int> yields int");

// ---- is_adapter / is_terminal (mutually exclusive) ----
static_assert(is_adapter<transform_adapter_t>::value,
    "is_adapter: transform adapter is an adapter");
static_assert(is_adapter<take_adapter_t>::value,
    "is_adapter: take adapter is an adapter");
static_assert(!is_terminal<transform_adapter_t>::value,
    "is_terminal: an adapter is not a terminal");

static_assert(is_terminal<to_vector_terminal_t>::value,
    "is_terminal: to_vector is a terminal");
static_assert(is_terminal<count_terminal_t>::value,
    "is_terminal: count is a terminal");
static_assert(!is_adapter<to_vector_terminal_t>::value,
    "is_adapter: a terminal is not an adapter");


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
static_assert(is_view_v<single_view<int>> && !is_view_v<int>,
    "is_view_v: matches the struct form");
static_assert(has_begin_end_v<std::vector<int>> && !has_begin_end_v<int>,
    "has_begin_end_v: matches the struct form");
static_assert(is_adapter_v<transform_adapter_t>
              && !is_adapter_v<to_vector_terminal_t>,
    "is_adapter_v: matches the struct form");
static_assert(is_terminal_v<to_vector_terminal_t>
              && !is_terminal_v<transform_adapter_t>,
    "is_terminal_v: matches the struct form");
static_assert(is_pipeable_to_view_v<std::vector<int>>
              && !is_pipeable_to_view_v<int>,
    "is_pipeable_to_view_v: matches the struct form");
#endif


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
static_assert(view_type<single_view<int>> && !view_type<int>,
    "view_type: concept matches is_view");
static_assert(view_adapter<transform_adapter_t>
              && !view_adapter<to_vector_terminal_t>,
    "view_adapter: concept matches is_adapter");
static_assert(view_terminal<to_vector_terminal_t>
              && !view_terminal<transform_adapter_t>,
    "view_terminal: concept matches is_terminal");
static_assert(pipeable_to_view<std::vector<int>> && !pipeable_to_view<int>,
    "pipeable_to_view: concept matches is_pipeable_to_view");
#endif


void test_core_traits(test::test_handler& _h)
{
    // is_view
    test::record_assertion(_h,
        is_view<single_view<int>>::value && !is_view<int>::value,
        "trait: is_view distinguishes views from scalars");
    test::record_assertion(_h, !is_view<std::vector<int>>::value,
        "trait: is_view is false for a bare container");

    // has_begin_end
    test::record_assertion(_h,
        has_begin_end<std::vector<int>>::value && !has_begin_end<int>::value,
        "trait: has_begin_end detects container-like types");

    // is_pipeable_to_view
    test::record_assertion(_h,
        is_pipeable_to_view<std::vector<int>>::value
        && is_pipeable_to_view<single_view<int>>::value
        && !is_pipeable_to_view<int>::value,
        "trait: is_pipeable_to_view accepts views and containers only");

    // view_value_type
    test::record_assertion(_h,
        std::is_same<view_value_type_t<single_view<int>>, int>::value,
        "trait: view_value_type_t extracts the element type");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    test::record_assertion(_h,
        is_view_v<iota_view<int>> && !is_view_v<double>,
        "trait: is_view_v matches the struct form");
    test::record_assertion(_h,
        is_pipeable_to_view_v<single_view<int>>
        && !is_pipeable_to_view_v<int>,
        "trait: is_pipeable_to_view_v matches the struct form");
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    test::record_assertion(_h,
        view_type<single_view<int>> && !view_type<int>,
        "trait: view_type concept matches is_view");
    test::record_assertion(_h,
        pipeable_to_view<std::vector<int>> && !pipeable_to_view<int>,
        "trait: pipeable_to_view concept matches is_pipeable_to_view");
#endif

    return;
}


void test_adapter_terminal_traits(test::test_handler& _h)
{
    // adapter is an adapter, not a terminal
    test::record_assertion(_h,
        is_adapter<transform_adapter_t>::value
        && !is_terminal<transform_adapter_t>::value,
        "trait: an adapter is is_adapter and not is_terminal");

    // terminal is a terminal, not an adapter
    test::record_assertion(_h,
        is_terminal<to_vector_terminal_t>::value
        && !is_adapter<to_vector_terminal_t>::value,
        "trait: a terminal is is_terminal and not is_adapter");

    // a plain type is neither
    test::record_assertion(_h,
        !is_adapter<int>::value && !is_terminal<int>::value,
        "trait: a scalar is neither adapter nor terminal");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    test::record_assertion(_h,
        is_adapter_v<take_adapter_t> && is_terminal_v<count_terminal_t>,
        "trait: is_adapter_v / is_terminal_v match the struct forms");
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    test::record_assertion(_h,
        view_adapter<transform_adapter_t>
        && view_terminal<to_vector_terminal_t>,
        "trait: view_adapter / view_terminal concepts match the traits");
#endif

    return;
}


NS_END  // testing
NS_END  // djinterp
