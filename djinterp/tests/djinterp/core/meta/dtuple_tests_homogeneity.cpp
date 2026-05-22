/******************************************************************************
* djinterp [test]                                  dtuple_tests_homogeneity.cpp
*
*   Unit tests for the tuple-homogeneity section of dtuple.hpp:
*     - is_tuple_homogeneous / is_tuple_homogeneous_v   (compile-time)
*     - is_homogeneous                                   (runtime)
*
*   is_tuple_homogeneous has three specialization paths:
*     1. primary template (false, used for anything that isn't a
*        std::tuple OR is a std::tuple of size 0)
*     2. <std::tuple<_Type>>: single-element tuple, trivially true
*     3. <std::tuple<_Type, _Type2, _Types...>>: recursive comparison
*        of head pair plus recursion on the tail
*
*   Notable edge case (verified below):
*     std::tuple<>  has NO matching specialization beyond the primary
*     template, so `is_tuple_homogeneous<std::tuple<>>::value` evaluates
*     to FALSE.  This is the documented behaviour we lock in.
*
*   is_homogeneous is a tiny runtime wrapper that defers to the
*   compile-time trait via the type of its argument; the runtime checks
*   confirm the value matches the trait for representative inputs.
*
*
* path:      /inc/djinterp/test/dtuple_tests_homogeneity.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.19
******************************************************************************/
#include "./dtuple_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace dtuple_test_types;


// =========================================================================
// I.   IS_TUPLE_HOMOGENEOUS  (compile-time)
// =========================================================================

// non-tuple types -- primary template, false
static_assert(is_tuple_homogeneous<int>::value == false,
              "is_tuple_homogeneous<int>::value should be false (not a tuple)");
static_assert(is_tuple_homogeneous<alpha>::value == false,
              "is_tuple_homogeneous<alpha>::value should be false (not a tuple)");
static_assert(is_tuple_homogeneous<void>::value == false,
              "is_tuple_homogeneous<void>::value should be false");
static_assert(is_tuple_homogeneous<std::pair<int, int>>::value == false,
              "is_tuple_homogeneous<std::pair<int, int>>::value should be false (not a tuple)");

// empty tuple -- intentionally false because the partial specializations
// require at least one element
static_assert(is_tuple_homogeneous<std::tuple<>>::value == false,
              "is_tuple_homogeneous<std::tuple<>>::value should be false (no element-shape spec covers empty)");

// single-element tuples -- trivially true
static_assert(is_tuple_homogeneous<std::tuple<int>>::value == true,
              "is_tuple_homogeneous<std::tuple<int>>::value should be true");
static_assert(is_tuple_homogeneous<std::tuple<alpha>>::value == true,
              "is_tuple_homogeneous<std::tuple<alpha>>::value should be true");
static_assert(is_tuple_homogeneous<std::tuple<std::tuple<int>>>::value == true,
              "is_tuple_homogeneous<std::tuple<std::tuple<int>>>::value should be true");

// two-element tuples
static_assert(is_tuple_homogeneous<std::tuple<int, int>>::value == true,
              "is_tuple_homogeneous<std::tuple<int, int>>::value should be true");
static_assert(is_tuple_homogeneous<std::tuple<int, char>>::value == false,
              "is_tuple_homogeneous<std::tuple<int, char>>::value should be false");
static_assert(is_tuple_homogeneous<std::tuple<alpha, alpha>>::value == true,
              "is_tuple_homogeneous<std::tuple<alpha, alpha>>::value should be true");
static_assert(is_tuple_homogeneous<std::tuple<alpha, bravo>>::value == false,
              "is_tuple_homogeneous<std::tuple<alpha, bravo>>::value should be false");

// multi-element tuples
static_assert(is_tuple_homogeneous<std::tuple<int, int, int>>::value == true,
              "is_tuple_homogeneous<std::tuple<int, int, int>>::value should be true");
static_assert(is_tuple_homogeneous<std::tuple<int, int, int, int>>::value == true,
              "is_tuple_homogeneous<std::tuple<4 ints>>::value should be true");
static_assert(is_tuple_homogeneous<std::tuple<int, int, int, char>>::value == false,
              "is_tuple_homogeneous<std::tuple<3 ints, char>>::value should be false (last differs)");
static_assert(is_tuple_homogeneous<std::tuple<char, int, int, int>>::value == false,
              "is_tuple_homogeneous<std::tuple<char, 3 ints>>::value should be false (first differs)");
static_assert(is_tuple_homogeneous<std::tuple<int, char, int>>::value == false,
              "is_tuple_homogeneous<std::tuple<int, char, int>>::value should be false (middle differs)");

// strict type identity -- cv- and ref-qualified variants count as
// different types
static_assert(is_tuple_homogeneous<std::tuple<int, const int>>::value == false,
              "is_tuple_homogeneous<int, const int> should be false (cv qualifies distinct)");
static_assert(is_tuple_homogeneous<std::tuple<int, int&>>::value == false,
              "is_tuple_homogeneous<int, int&> should be false (ref qualifies distinct)");
static_assert(is_tuple_homogeneous<std::tuple<const int, const int>>::value == true,
              "is_tuple_homogeneous<const int, const int> should be true");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_tuple_homogeneous_v<std::tuple<int, int>>     == true,
                  "is_tuple_homogeneous_v<tuple<int, int>> should be true");
    static_assert(is_tuple_homogeneous_v<std::tuple<int, char>>    == false,
                  "is_tuple_homogeneous_v<tuple<int, char>> should be false");
    static_assert(is_tuple_homogeneous_v<std::tuple<>>             == false,
                  "is_tuple_homogeneous_v<tuple<>> should be false");
    static_assert(is_tuple_homogeneous_v<int>                      == false,
                  "is_tuple_homogeneous_v<int> should be false");
#endif


// =========================================================================
// II.  RUNTIME DRIVER
// =========================================================================

void
dtuple_tests_homogeneity_all(
    test_handler& _test_handler
)
{
    // ---- is_tuple_homogeneous (mirrored compile-time) ----
    record_assertion(_test_handler,
        is_tuple_homogeneous<int>::value == false,
        "is_tuple_homogeneous: non-tuple type -> false");
    record_assertion(_test_handler,
        is_tuple_homogeneous<std::tuple<>>::value == false,
        "is_tuple_homogeneous: empty tuple -> false");
    record_assertion(_test_handler,
        is_tuple_homogeneous<std::tuple<int>>::value == true,
        "is_tuple_homogeneous: single-element tuple -> true");
    record_assertion(_test_handler,
        is_tuple_homogeneous<std::tuple<int, int>>::value == true,
        "is_tuple_homogeneous: two equal elements -> true");
    record_assertion(_test_handler,
        is_tuple_homogeneous<std::tuple<int, char>>::value == false,
        "is_tuple_homogeneous: two distinct elements -> false");
    record_assertion(_test_handler,
        is_tuple_homogeneous<std::tuple<int, int, int>>::value == true,
        "is_tuple_homogeneous: three equal elements -> true");
    record_assertion(_test_handler,
        is_tuple_homogeneous<std::tuple<int, int, char>>::value == false,
        "is_tuple_homogeneous: three elements, last differs -> false");
    record_assertion(_test_handler,
        is_tuple_homogeneous<std::tuple<int, char, int>>::value == false,
        "is_tuple_homogeneous: three elements, middle differs -> false");
    record_assertion(_test_handler,
        is_tuple_homogeneous<std::tuple<int, const int>>::value == false,
        "is_tuple_homogeneous: cv qualifies distinct types");

    // ---- is_homogeneous (runtime function over a tuple value) ----
    {
        std::tuple<int> single{1};

        record_assertion(_test_handler,
            is_homogeneous(single) == true,
            "is_homogeneous: single-element tuple -> true");
    }
    {
        std::tuple<int, int> two_equal{1, 2};

        record_assertion(_test_handler,
            is_homogeneous(two_equal) == true,
            "is_homogeneous: two-equal tuple -> true");
    }
    {
        std::tuple<int, char> two_distinct{1, 'a'};

        record_assertion(_test_handler,
            is_homogeneous(two_distinct) == false,
            "is_homogeneous: two-distinct tuple -> false");
    }
    {
        std::tuple<int, int, int, int> four_equal{1, 2, 3, 4};

        record_assertion(_test_handler,
            is_homogeneous(four_equal) == true,
            "is_homogeneous: four-equal tuple -> true");
    }
    {
        std::tuple<int, int, char> mixed{1, 2, 'q'};

        record_assertion(_test_handler,
            is_homogeneous(mixed) == false,
            "is_homogeneous: three-element mixed tuple -> false");
    }
    {
        std::tuple<> empty{};

        record_assertion(_test_handler,
            is_homogeneous(empty) == false,
            "is_homogeneous: empty tuple -> false (matches the trait)");
    }
    // const-qualified tuple input -- the function takes `const tuple&`,
    // so this just hits the same trait via the same path
    {
        const std::tuple<int, int> const_t{1, 2};

        record_assertion(_test_handler,
            is_homogeneous(const_t) == true,
            "is_homogeneous: const tuple input -> true (trait matches via tuple-of-T template arg)");
    }

    return;
}


NS_END  // testing
NS_END  // djinterp
