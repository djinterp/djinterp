/******************************************************************************
* djinterp [testing]                                   dtuple_tests_flatten.cpp
*
*   Flatten-group test definitions for the dtuple test suite:
* tuple_flatten_types (collapse a 2D tuple into its concatenated
* row contents) and normalize_tuple (strip cv- and reference-
* qualifiers from every element).
*
*   These two operations are tested together because they share the
* common theme of "reshape a tuple by applying a uniform operation
* element-wise".  flatten compresses two dimensions into one;
* normalize rewrites every element with its clean_t form.
*
*
* path:      /tests/djinterp/core/meta/dtuple_tests_flatten.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.19
******************************************************************************/

#include "./dtuple_tests.hpp"


NS_DJINTERP
NS_TESTING


// =========================================================================
// I.   FLATTEN AND NORMALIZE
// =========================================================================

/*
tests_dtuple_flatten_and_normalize
  Verifies tuple_flatten_types collapses a 2D tuple into its
  concatenated row contents, and that normalize_tuple strips
  cv- and reference-qualifiers from every element.
  Tests the following:
  - flattening the empty 2D tuple yields the empty tuple
  - flattening a single-row 2D tuple yields its row
  - flattening multiple rows yields their concatenation
  - flattening rows that include empty rows yields the right result
  - normalize strips const, volatile, references, and combinations
  - normalize of an empty tuple is an empty tuple (degenerate case)
  - normalize is idempotent
*/
bool
tests_dtuple_flatten_and_normalize(
    test_handler& _handler
)
{
    // flatten edge cases
    D_TEST_TYPE_EQ(tuple_flatten_types_t<std::tuple<>>,
                   std::tuple<>);
    D_TEST_TYPE_EQ(
        tuple_flatten_types_t<std::tuple<std::tuple<int>>>,
        std::tuple<int>);

    // flatten multi-row
    D_TEST_TYPE_EQ(
        tuple_flatten_types_t<
            std::tuple<std::tuple<int, char>,
                       std::tuple<long, double>>>,
        std::tuple<int, char, long, double>);

    // flatten with an empty row among non-empty rows
    D_TEST_TYPE_EQ(
        tuple_flatten_types_t<
            std::tuple<std::tuple<int>,
                       std::tuple<>,
                       std::tuple<char>>>,
        std::tuple<int, char>);

    // flatten all-empty rows
    D_TEST_TYPE_EQ(
        tuple_flatten_types_t<
            std::tuple<std::tuple<>,
                       std::tuple<>,
                       std::tuple<>>>,
        std::tuple<>);

    // normalize on a pure cv-qualified tuple
    D_TEST_TYPE_EQ(
        normalize_tuple_t<std::tuple<const int,
                                      volatile char>>,
        std::tuple<int, char>);

    // normalize on reference-qualified elements
    D_TEST_TYPE_EQ(
        normalize_tuple_t<std::tuple<int&, const char&&>>,
        std::tuple<int, char>);

    // normalize on combined cv + reference elements.
    // NOTE: clean_t on `char* const` removes the top-level const
    // qualifying the POINTER (it becomes `char*`); the pointee
    // type (`char`) is unaffected.  A test that wants to
    // preserve pointee-qualifications should pass a pointer-to-
    // const instead, e.g. `const char*`.
    D_TEST_TYPE_EQ(
        normalize_tuple_t<std::tuple<const volatile int&,
                                      char* const>>,
        std::tuple<int, char*>);

    // normalize on an empty tuple (degenerate)
    D_TEST_TYPE_EQ(normalize_tuple_t<std::tuple<>>,
                   std::tuple<>);

    // idempotence: normalize(normalize(x)) == normalize(x)
    typedef normalize_tuple_t<std::tuple<const int&, char>>
        norm_once;
    typedef normalize_tuple_t<norm_once>
        norm_twice;
    D_TEST_TYPE_EQ(norm_once, norm_twice);

    D_TEST_FIRE(_handler,
                on_compile_check,
                "flatten_and_normalize/all-checks");

    return true;
}


NS_END  // testing
NS_END  // djinterp
