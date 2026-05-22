/******************************************************************************
* djinterp [test]                                     dtuple_tests_counting.cpp
*
*   Unit tests for the type-counting-and-filtering section of dtuple.hpp:
*     - tuple_count_type / tuple_count_type_v
*     - tuple_count_and_remove / tuple_count_and_remove_t /
*       tuple_count_and_remove_v
*     - tuple_consolidate_types / tuple_consolidate_types_t
*
*   The counting helpers each have multiple internal partial
* specializations that this file exercises directly:
*
*   tuple_count_type_helper:
*     - empty (base case, returns 0)
*     - single-element tuple (special-cased one-step decision)
*     - recursive head + tail
*
*   tuple_count_and_remove_helper:
*     - empty (returns count and the accumulated filtered tuple)
*     - non-empty (branches on whether head matches the target type,
*       producing two distinct recursive shapes)
*
*   tuple_consolidate_types_helper:
*     - empty (returns the accumulated result tuple)
*     - non-empty (folds equal types into a single std::array entry of
*       the right length, leaves a unique type as itself)
*
*   Order of remaining-element preservation is verified for
* tuple_count_and_remove (the unmatched elements should appear in their
* original relative order in the resulting tuple).
*
*
* path:      /inc/djinterp/test/dtuple_tests_counting.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.19
******************************************************************************/

#include "./dtuple_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace dtuple_test_types;


// =========================================================================
// I.   TUPLE_COUNT_TYPE  (compile-time)
// =========================================================================

// empty input -- helper's base case
static_assert(tuple_count_type<int, std::tuple<>>::value == 0,
              "tuple_count_type<int, std::tuple<>>::value should be 0");

// single-element tuple where head matches
static_assert(tuple_count_type<int, std::tuple<int>>::value == 1,
              "tuple_count_type<int, std::tuple<int>>::value should be 1");

// single-element tuple where head does NOT match
static_assert(tuple_count_type<char, std::tuple<int>>::value == 0,
              "tuple_count_type<char, std::tuple<int>>::value should be 0");

// multi-element pack-shape -- exercises the recursive case
static_assert(tuple_count_type<int, int, char, int>::value == 2,
              "tuple_count_type<int, int, char, int>::value should be 2");
static_assert(tuple_count_type<int, int, int, int>::value == 3,
              "tuple_count_type<int, int, int, int>::value should be 3");
static_assert(tuple_count_type<float, int, char>::value == 0,
              "tuple_count_type<float, int, char>::value should be 0");

// multi-element tuple-shape
static_assert(tuple_count_type<int, std::tuple<int, char, int>>::value == 2,
              "tuple_count_type<int, std::tuple<int, char, int>>::value should be 2");
static_assert(tuple_count_type<int, std::tuple<int, int, int, int, int>>::value == 5,
              "tuple_count_type<int, std::tuple<5 ints>>::value should be 5");
static_assert(tuple_count_type<float, std::tuple<int, char, double>>::value == 0,
              "tuple_count_type<float, std::tuple<int, char, double>>::value should be 0");

// type identity is strict: cv and ref qualifiers DO change identity
static_assert(tuple_count_type<int, std::tuple<int, const int, int&>>::value == 1,
              "tuple_count_type counts only the bare int (cv/ref differ)");
static_assert(tuple_count_type<const int, std::tuple<int, const int, int>>::value == 1,
              "tuple_count_type counts only the const int");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(tuple_count_type_v<int, std::tuple<int, char, int>> == 2,
                  "tuple_count_type_v: matches ::value");
    static_assert(tuple_count_type_v<int, std::tuple<>> == 0,
                  "tuple_count_type_v: empty -> 0");
#endif


// =========================================================================
// II.  TUPLE_COUNT_AND_REMOVE  (compile-time)
// =========================================================================

// empty input -- count 0, filtered tuple empty
static_assert(tuple_count_and_remove<int, std::tuple<>>::value == 0,
              "tuple_count_and_remove<int, std::tuple<>>::value should be 0");
static_assert(std::is_same<typename tuple_count_and_remove<int, std::tuple<>>::type,
                           std::tuple<>>::value,
              "tuple_count_and_remove<int, std::tuple<>>::type should be std::tuple<>");

// no matches in a non-empty input
static_assert(tuple_count_and_remove<float, int, char>::value == 0,
              "tuple_count_and_remove<float, int, char>::value should be 0");
static_assert(std::is_same<typename tuple_count_and_remove<float, int, char>::type,
                           std::tuple<int, char>>::value,
              "tuple_count_and_remove<float, int, char>::type should preserve order int, char");

// all match
static_assert(tuple_count_and_remove<int, int, int, int>::value == 3,
              "tuple_count_and_remove<int, int, int, int>::value should be 3");
static_assert(std::is_same<typename tuple_count_and_remove<int, int, int, int>::type,
                           std::tuple<>>::value,
              "tuple_count_and_remove<int, int, int, int>::type should be empty");

// mixed -- counted matches, kept the rest IN ORDER
static_assert(tuple_count_and_remove<int, int, char, int>::value == 2,
              "tuple_count_and_remove<int, int, char, int>::value should be 2");
static_assert(std::is_same<typename tuple_count_and_remove<int, int, char, int>::type,
                           std::tuple<char>>::value,
              "tuple_count_and_remove<int, int, char, int>::type should be std::tuple<char>");
static_assert(tuple_count_and_remove<int, char, int, char, int, float>::value == 2,
              "tuple_count_and_remove<int, char, int, char, int, float>::value should be 2");
static_assert(std::is_same<typename tuple_count_and_remove<int,
                                                             char, int, char, int, float>::type,
                           std::tuple<char, char, float>>::value,
              "tuple_count_and_remove<int, char, int, char, int, float>::type should preserve relative order");

// tuple-shape inputs work identically
static_assert(tuple_count_and_remove<int, std::tuple<int, char, int>>::value == 2,
              "tuple_count_and_remove tuple-shape value");
static_assert(std::is_same<typename tuple_count_and_remove<int, std::tuple<int, char, int>>::type,
                           std::tuple<char>>::value,
              "tuple_count_and_remove tuple-shape type");

// strict type-identity matching -- cv- and ref-qualified variants are
// NOT the same as the bare type
static_assert(tuple_count_and_remove<int, int, const int, int&>::value == 1,
              "tuple_count_and_remove<int, ...>: cv/ref variants not counted");
static_assert(std::is_same<typename tuple_count_and_remove<int, int, const int, int&>::type,
                           std::tuple<const int, int&>>::value,
              "tuple_count_and_remove<int, ...>: cv/ref variants kept in remainder");

// alias and value-helper consistency
static_assert(std::is_same<tuple_count_and_remove_t<int, std::tuple<int, char, int>>,
                           std::tuple<char>>::value,
              "tuple_count_and_remove_t: alias");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(tuple_count_and_remove_v<int, std::tuple<int, char, int>> == 2,
                  "tuple_count_and_remove_v: matches ::value");
#endif


// =========================================================================
// III. TUPLE_CONSOLIDATE_TYPES  (compile-time)
// =========================================================================

// empty input -- helper's empty case
static_assert(std::is_same<typename tuple_consolidate_types<>::type, std::tuple<>>::value,
              "tuple_consolidate_types<>::type should be std::tuple<>");
static_assert(std::is_same<typename tuple_consolidate_types<std::tuple<>>::type,
                           std::tuple<>>::value,
              "tuple_consolidate_types<std::tuple<>>::type should be std::tuple<>");

// all-unique input -- everything stays as its own element
static_assert(std::is_same<typename tuple_consolidate_types<int, char, float>::type,
                           std::tuple<int, char, float>>::value,
              "tuple_consolidate_types<int, char, float>::type should be std::tuple<int, char, float>");
static_assert(std::is_same<typename tuple_consolidate_types<std::tuple<int, char, float>>::type,
                           std::tuple<int, char, float>>::value,
              "tuple_consolidate_types<tuple<int, char, float>>::type should be std::tuple<int, char, float>");

// single duplicate group -- becomes std::array<T, N>
static_assert(std::is_same<typename tuple_consolidate_types<int, int>::type,
                           std::tuple<std::array<int, 2>>>::value,
              "tuple_consolidate_types<int, int>::type should be std::tuple<std::array<int, 2>>");
static_assert(std::is_same<typename tuple_consolidate_types<int, int, int>::type,
                           std::tuple<std::array<int, 3>>>::value,
              "tuple_consolidate_types<int, int, int>::type should be std::tuple<std::array<int, 3>>");

// multiple duplicate groups
static_assert(std::is_same<typename tuple_consolidate_types<int, char, int, char>::type,
                           std::tuple<std::array<int, 2>, std::array<char, 2>>>::value,
              "tuple_consolidate_types<int, char, int, char>::type should consolidate both groups");
static_assert(std::is_same<typename tuple_consolidate_types<int, char, int, char, int>::type,
                           std::tuple<std::array<int, 3>, std::array<char, 2>>>::value,
              "tuple_consolidate_types<int, char, int, char, int>::type should yield std::array<int, 3>, std::array<char, 2>");

// mixed unique + duplicate -- order based on first appearance
static_assert(std::is_same<typename tuple_consolidate_types<int, char, int, float>::type,
                           std::tuple<std::array<int, 2>, char, float>>::value,
              "tuple_consolidate_types<int, char, int, float>::type should be std::tuple<std::array<int, 2>, char, float>");
static_assert(std::is_same<typename tuple_consolidate_types<float, int, int, char, int>::type,
                           std::tuple<float, std::array<int, 3>, char>>::value,
              "tuple_consolidate_types<float, int, int, char, int>::type should be std::tuple<float, std::array<int, 3>, char>");

// alias consistency
static_assert(std::is_same<tuple_consolidate_types_t<int, int, char>,
                           std::tuple<std::array<int, 2>, char>>::value,
              "tuple_consolidate_types_t: alias");


// =========================================================================
// IV.  RUNTIME DRIVER
// =========================================================================

void
dtuple_tests_counting_all(
    test_handler& _test_handler
)
{
    // ---- tuple_count_type ----
    record_assertion(_test_handler, 
        tuple_count_type<int, std::tuple<>>::value == 0,
        "tuple_count_type: empty input -> 0");
    record_assertion(_test_handler, 
        tuple_count_type<int, std::tuple<int>>::value == 1,
        "tuple_count_type: single matching element -> 1");
    record_assertion(_test_handler, 
        tuple_count_type<char, std::tuple<int>>::value == 0,
        "tuple_count_type: single non-matching element -> 0");
    record_assertion(_test_handler, 
        tuple_count_type<int, int, char, int>::value == 2,
        "tuple_count_type: pack-shape, two matches");
    record_assertion(_test_handler, 
        tuple_count_type<int, std::tuple<int, char, int>>::value == 2,
        "tuple_count_type: tuple-shape, two matches");
    record_assertion(_test_handler, 
        tuple_count_type<int, std::tuple<int, int, int, int, int>>::value == 5,
        "tuple_count_type: five matches");
    record_assertion(_test_handler, 
        tuple_count_type<int, std::tuple<int, const int, int&>>::value == 1,
        "tuple_count_type: cv/ref variants are distinct types");

    // ---- tuple_count_and_remove ----
    record_assertion(_test_handler, 
        tuple_count_and_remove<int, std::tuple<>>::value == 0,
        "tuple_count_and_remove: empty input count -> 0");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_count_and_remove<int, std::tuple<>>::type,
                     std::tuple<>>::value,
        "tuple_count_and_remove: empty input type -> empty");
    record_assertion(_test_handler, 
        tuple_count_and_remove<int, int, char, int>::value == 2,
        "tuple_count_and_remove: count is 2");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_count_and_remove<int, int, char, int>::type,
                     std::tuple<char>>::value,
        "tuple_count_and_remove: remaining element is char");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_count_and_remove<int,
                                                      char, int, char, int, float>::type,
                     std::tuple<char, char, float>>::value,
        "tuple_count_and_remove: preserves relative order of remainder");
    record_assertion(_test_handler, 
        tuple_count_and_remove<int, int, const int, int&>::value == 1,
        "tuple_count_and_remove: cv/ref distinct -> count 1");
    record_assertion(_test_handler, 
        std::is_same<tuple_count_and_remove_t<int, std::tuple<int, char, int>>,
                     std::tuple<char>>::value,
        "tuple_count_and_remove_t: alias");

    // ---- tuple_consolidate_types ----
    record_assertion(_test_handler, 
        std::is_same<typename tuple_consolidate_types<>::type, std::tuple<>>::value,
        "tuple_consolidate_types: empty pack -> empty tuple");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_consolidate_types<int, char, float>::type,
                     std::tuple<int, char, float>>::value,
        "tuple_consolidate_types: all-unique stays untouched");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_consolidate_types<int, int>::type,
                     std::tuple<std::array<int, 2>>>::value,
        "tuple_consolidate_types: two equal types -> std::array<_, 2>");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_consolidate_types<int, int, int>::type,
                     std::tuple<std::array<int, 3>>>::value,
        "tuple_consolidate_types: three equal types -> std::array<_, 3>");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_consolidate_types<int, char, int, char, int>::type,
                     std::tuple<std::array<int, 3>, std::array<char, 2>>>::value,
        "tuple_consolidate_types: two groups consolidated");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_consolidate_types<float, int, int, char, int>::type,
                     std::tuple<float, std::array<int, 3>, char>>::value,
        "tuple_consolidate_types: order is first-appearance order");
    record_assertion(_test_handler, 
        std::is_same<tuple_consolidate_types_t<int, int, char>,
                     std::tuple<std::array<int, 2>, char>>::value,
        "tuple_consolidate_types_t: alias");

    return;
}


NS_END  // testing
NS_END  // djinterp
