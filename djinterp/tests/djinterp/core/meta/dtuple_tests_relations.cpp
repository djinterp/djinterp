/******************************************************************************
* djinterp [test]                                    dtuple_tests_relations.cpp
*
*   Unit tests for the tuple-of-tuples type-relation section of
* dtuple.hpp:
*     - normalize_tuple / normalize_tuple_t
*     - tuple_all_elements_same_as
*     - all_inner_tuple_elements_one_type /
*       all_inner_tuple_elements_one_type_v
*
*   normalize_tuple applies `clean_t` to every element of a tuple,
* stripping cv- and reference qualifiers.  The `normalize_tuple_t`
* alias additionally cleans the OUTER type first, so a
* `const std::tuple<int>&` normalizes to `std::tuple<int>`.
*
*   tuple_all_elements_same_as checks that every element of a tuple
* (after clean_t) is the same as the supplied probe type.  Empty
* tuple is vacuously true.  cv- and ref-qualified element variants
* count as the SAME type for the probe because clean_t is applied
* first.
*
*   all_inner_tuple_elements_one_type is a four-way classifier:
*     - non-tuple outer: false (primary template)
*     - empty outer: true (vacuous)
*     - first inner is empty: every other inner must ALSO be empty
*     - first inner is non-empty: every inner must be non-empty AND
*       every element across all inners must be clean-equal to the
*       first element's clean type
*
*   The tests exercise all four paths, including the asymmetric
* mix-with-empty case (first non-empty, later empty -> false), the
* symmetric mix-with-empty (first empty, later non-empty -> false),
* and uniform tuples-of-cv-qualified-elements that should still
* return true via clean_t.
*
*
* path:      /inc/djinterp/test/dtuple_tests_relations.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.19
******************************************************************************/

#include "./dtuple_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace dtuple_test_types;


// =========================================================================
// I.   NORMALIZE_TUPLE  (compile-time)
// =========================================================================

// empty tuple -- no elements to clean
static_assert(std::is_same<typename normalize_tuple<std::tuple<>>::type,
                           std::tuple<>>::value,
              "normalize_tuple<std::tuple<>>::type should be std::tuple<>");

// already-clean elements -- result equal to input
static_assert(std::is_same<typename normalize_tuple<std::tuple<int>>::type,
                           std::tuple<int>>::value,
              "normalize_tuple<std::tuple<int>>::type should be std::tuple<int>");
static_assert(std::is_same<typename normalize_tuple<std::tuple<int, char, float>>::type,
                           std::tuple<int, char, float>>::value,
              "normalize_tuple<std::tuple<int, char, float>>::type should be unchanged");

// cv qualifiers stripped
static_assert(std::is_same<typename normalize_tuple<std::tuple<const int>>::type,
                           std::tuple<int>>::value,
              "normalize_tuple<std::tuple<const int>>::type should strip const");
static_assert(std::is_same<typename normalize_tuple<std::tuple<volatile int, const char>>::type,
                           std::tuple<int, char>>::value,
              "normalize_tuple<std::tuple<volatile int, const char>>::type should strip cv");
static_assert(std::is_same<typename normalize_tuple<std::tuple<const volatile int>>::type,
                           std::tuple<int>>::value,
              "normalize_tuple<std::tuple<const volatile int>>::type should strip cv");

// references stripped
static_assert(std::is_same<typename normalize_tuple<std::tuple<int&>>::type,
                           std::tuple<int>>::value,
              "normalize_tuple<std::tuple<int&>>::type should strip lvalue ref");
static_assert(std::is_same<typename normalize_tuple<std::tuple<int&&>>::type,
                           std::tuple<int>>::value,
              "normalize_tuple<std::tuple<int&&>>::type should strip rvalue ref");

// mixed cv + ref stripped
static_assert(std::is_same<typename normalize_tuple<std::tuple<const int&, volatile char>>::type,
                           std::tuple<int, char>>::value,
              "normalize_tuple<std::tuple<const int&, volatile char>>::type should strip cv + ref");

// normalize_tuple_t -- also cleans the OUTER type first
static_assert(std::is_same<normalize_tuple_t<std::tuple<int>>,
                           std::tuple<int>>::value,
              "normalize_tuple_t<std::tuple<int>> should be std::tuple<int>");
static_assert(std::is_same<normalize_tuple_t<const std::tuple<int>>,
                           std::tuple<int>>::value,
              "normalize_tuple_t<const std::tuple<int>> should strip outer const before normalizing");
static_assert(std::is_same<normalize_tuple_t<std::tuple<const int>&>,
                           std::tuple<int>>::value,
              "normalize_tuple_t<std::tuple<const int>&> should strip outer ref and inner const");
static_assert(std::is_same<normalize_tuple_t<const std::tuple<const int&, volatile char>&>,
                           std::tuple<int, char>>::value,
              "normalize_tuple_t deep cleaning: outer const+ref and inner cv+ref");


// =========================================================================
// II.  TUPLE_ALL_ELEMENTS_SAME_AS  (compile-time)
// =========================================================================

// empty tuple -- vacuously true for any probe
static_assert(tuple_all_elements_same_as<std::tuple<>, int>::value == true,
              "tuple_all_elements_same_as<std::tuple<>, int>::value should be true (vacuous)");
static_assert(tuple_all_elements_same_as<std::tuple<>, alpha>::value == true,
              "tuple_all_elements_same_as<std::tuple<>, alpha>::value should be true");

// single-element tuple, match
static_assert(tuple_all_elements_same_as<std::tuple<int>, int>::value == true,
              "tuple_all_elements_same_as<std::tuple<int>, int>::value should be true");

// single-element tuple, mismatch
static_assert(tuple_all_elements_same_as<std::tuple<int>, char>::value == false,
              "tuple_all_elements_same_as<std::tuple<int>, char>::value should be false");

// multi-element tuple, all match
static_assert(tuple_all_elements_same_as<std::tuple<int, int, int>, int>::value == true,
              "tuple_all_elements_same_as<std::tuple<int, int, int>, int>::value should be true");

// multi-element tuple, partial match
static_assert(tuple_all_elements_same_as<std::tuple<int, char, int>, int>::value == false,
              "tuple_all_elements_same_as<std::tuple<int, char, int>, int>::value should be false");
static_assert(tuple_all_elements_same_as<std::tuple<int, int, char>, int>::value == false,
              "tuple_all_elements_same_as<std::tuple<int, int, char>, int>::value should be false");
static_assert(tuple_all_elements_same_as<std::tuple<char, int, int>, int>::value == false,
              "tuple_all_elements_same_as<std::tuple<char, int, int>, int>::value should be false");

// cv- and ref-qualified elements -- clean_t is applied, so they match
// the bare type
static_assert(tuple_all_elements_same_as<std::tuple<const int>, int>::value == true,
              "tuple_all_elements_same_as<tuple<const int>, int>::value should be true (clean_t)");
static_assert(tuple_all_elements_same_as<std::tuple<int&>, int>::value == true,
              "tuple_all_elements_same_as<tuple<int&>, int>::value should be true (clean_t)");
static_assert(tuple_all_elements_same_as<std::tuple<int, const int, int&>, int>::value == true,
              "tuple_all_elements_same_as<tuple<int, const int, int&>, int>::value should be true (clean_t)");

// the probe type itself is NOT cleaned -- only the elements are.  So
// probing with a qualified type fails when elements are bare.
static_assert(tuple_all_elements_same_as<std::tuple<int>, const int>::value == false,
              "tuple_all_elements_same_as<tuple<int>, const int>::value should be false (probe NOT cleaned)");

// alpha-only tuple
static_assert(tuple_all_elements_same_as<std::tuple<alpha, alpha, alpha>, alpha>::value == true,
              "tuple_all_elements_same_as<tuple<alpha * 3>, alpha>::value should be true");


// =========================================================================
// III. ALL_INNER_TUPLE_ELEMENTS_ONE_TYPE  (compile-time)
// =========================================================================

// non-tuple outer -- primary template, false
static_assert(all_inner_tuple_elements_one_type<int>::value == false,
              "all_inner_tuple_elements_one_type<int>::value should be false");
static_assert(all_inner_tuple_elements_one_type<alpha>::value == false,
              "all_inner_tuple_elements_one_type<alpha>::value should be false");

// empty outer -- explicit specialization, true
static_assert(all_inner_tuple_elements_one_type<std::tuple<>>::value == true,
              "all_inner_tuple_elements_one_type<std::tuple<>>::value should be true (vacuous)");

// single empty inner -- all_inners_empty<> base case, true
static_assert(all_inner_tuple_elements_one_type<std::tuple<std::tuple<>>>::value == true,
              "all_inner_tuple_elements_one_type<tuple<tuple<>>>::value should be true");

// multiple empty inners -- all_inners_empty recursive case, true
static_assert(all_inner_tuple_elements_one_type<std::tuple<std::tuple<>,
                                                            std::tuple<>>>::value == true,
              "all_inner_tuple_elements_one_type<tuple<tuple<>, tuple<>>>::value should be true");
static_assert(all_inner_tuple_elements_one_type<std::tuple<std::tuple<>,
                                                            std::tuple<>,
                                                            std::tuple<>>>::value == true,
              "all_inner_tuple_elements_one_type<3 empty inners>::value should be true");

// first inner empty, later inner non-empty -- mismatch in shape, false
static_assert(all_inner_tuple_elements_one_type<std::tuple<std::tuple<>,
                                                            std::tuple<int>>>::value == false,
              "all_inner_tuple_elements_one_type<tuple<empty, non-empty>>::value should be false");
static_assert(all_inner_tuple_elements_one_type<std::tuple<std::tuple<>,
                                                            std::tuple<>,
                                                            std::tuple<int>>>::value == false,
              "all_inner_tuple_elements_one_type<tuple<empty, empty, non-empty>>::value should be false");

// first inner non-empty, all later inners uniform -- true
static_assert(all_inner_tuple_elements_one_type<std::tuple<std::tuple<int>>>::value == true,
              "all_inner_tuple_elements_one_type<tuple<tuple<int>>>::value should be true (one row, one element)");
static_assert(all_inner_tuple_elements_one_type<std::tuple<std::tuple<int, int, int>>>::value == true,
              "all_inner_tuple_elements_one_type<tuple<tuple<int, int, int>>>::value should be true (one row, all int)");
static_assert(all_inner_tuple_elements_one_type<std::tuple<std::tuple<int>,
                                                            std::tuple<int>>>::value == true,
              "all_inner_tuple_elements_one_type<tuple<tuple<int>, tuple<int>>>::value should be true");
static_assert(all_inner_tuple_elements_one_type<std::tuple<std::tuple<int>,
                                                            std::tuple<int, int>>>::value == true,
              "all_inner_tuple_elements_one_type<tuple<tuple<int>, tuple<int, int>>>::value should be true (jagged sizes OK)");
static_assert(all_inner_tuple_elements_one_type<std::tuple<std::tuple<int, int>,
                                                            std::tuple<int>,
                                                            std::tuple<int, int, int>>>::value == true,
              "all_inner_tuple_elements_one_type: jagged with all int across all rows -> true");

// first inner non-empty but contains a mixed-type element list -- false
static_assert(all_inner_tuple_elements_one_type<std::tuple<std::tuple<int, char>>>::value == false,
              "all_inner_tuple_elements_one_type<tuple<tuple<int, char>>>::value should be false (first row has mixed elements)");

// first inner non-empty, later inner has different type -- false
static_assert(all_inner_tuple_elements_one_type<std::tuple<std::tuple<int>,
                                                            std::tuple<char>>>::value == false,
              "all_inner_tuple_elements_one_type<tuple<tuple<int>, tuple<char>>>::value should be false");
static_assert(all_inner_tuple_elements_one_type<std::tuple<std::tuple<int>,
                                                            std::tuple<int, char>>>::value == false,
              "all_inner_tuple_elements_one_type<tuple<tuple<int>, tuple<int, char>>>::value should be false");

// first inner non-empty, later inner empty -- the
// all_inners_nonempty_all_elements_same path requires every inner to
// be non-empty, so this is false
static_assert(all_inner_tuple_elements_one_type<std::tuple<std::tuple<int>,
                                                            std::tuple<>>>::value == false,
              "all_inner_tuple_elements_one_type<tuple<tuple<int>, tuple<>>>::value should be false");
static_assert(all_inner_tuple_elements_one_type<std::tuple<std::tuple<int>,
                                                            std::tuple<int>,
                                                            std::tuple<>>>::value == false,
              "all_inner_tuple_elements_one_type<tuple<tuple<int>, tuple<int>, tuple<>>>::value should be false");

// cv-/ref-qualified element types -- clean_t is applied, so they match
static_assert(all_inner_tuple_elements_one_type<std::tuple<std::tuple<const int>,
                                                            std::tuple<int&>>>::value == true,
              "all_inner_tuple_elements_one_type<tuple<tuple<const int>, tuple<int&>>>::value should be true (clean_t)");
static_assert(all_inner_tuple_elements_one_type<std::tuple<std::tuple<const int, int&>,
                                                            std::tuple<int>>>::value == true,
              "all_inner_tuple_elements_one_type<tuple<tuple<const int, int&>, tuple<int>>>::value should be true (clean_t)");

// alpha-only inner tuples
static_assert(all_inner_tuple_elements_one_type<std::tuple<std::tuple<alpha>,
                                                            std::tuple<alpha, alpha>>>::value == true,
              "all_inner_tuple_elements_one_type<tuple<tuple<alpha>, tuple<alpha, alpha>>>::value should be true");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(all_inner_tuple_elements_one_type_v<std::tuple<>>                          == true,
                  "all_inner_tuple_elements_one_type_v: empty outer -> true");
    static_assert(all_inner_tuple_elements_one_type_v<std::tuple<std::tuple<int>,
                                                                  std::tuple<int>>>          == true,
                  "all_inner_tuple_elements_one_type_v: all int -> true");
    static_assert(all_inner_tuple_elements_one_type_v<std::tuple<std::tuple<int>,
                                                                  std::tuple<char>>>         == false,
                  "all_inner_tuple_elements_one_type_v: int vs char -> false");
    static_assert(all_inner_tuple_elements_one_type_v<int>                                   == false,
                  "all_inner_tuple_elements_one_type_v<int> should be false");
#endif


// =========================================================================
// IV.  RUNTIME DRIVER
// =========================================================================

void
dtuple_tests_relations_all(
    test_handler& _test_handler
)
{
    // ---- normalize_tuple ----
    record_assertion(_test_handler, 
        std::is_same<typename normalize_tuple<std::tuple<>>::type,
                     std::tuple<>>::value,
        "normalize_tuple: empty tuple -> empty");
    record_assertion(_test_handler, 
        std::is_same<typename normalize_tuple<std::tuple<int, char>>::type,
                     std::tuple<int, char>>::value,
        "normalize_tuple: already-clean tuple unchanged");
    record_assertion(_test_handler, 
        std::is_same<typename normalize_tuple<std::tuple<const int>>::type,
                     std::tuple<int>>::value,
        "normalize_tuple: strips const from element");
    record_assertion(_test_handler, 
        std::is_same<typename normalize_tuple<std::tuple<int&&>>::type,
                     std::tuple<int>>::value,
        "normalize_tuple: strips rvalue ref from element");
    record_assertion(_test_handler, 
        std::is_same<typename normalize_tuple<std::tuple<const int&, volatile char>>::type,
                     std::tuple<int, char>>::value,
        "normalize_tuple: strips mixed cv + ref");
    record_assertion(_test_handler, 
        std::is_same<normalize_tuple_t<const std::tuple<int>>, std::tuple<int>>::value,
        "normalize_tuple_t: cleans outer const before normalizing");
    record_assertion(_test_handler, 
        std::is_same<normalize_tuple_t<const std::tuple<const int&, volatile char>&>,
                     std::tuple<int, char>>::value,
        "normalize_tuple_t: deep clean (outer + inner)");

    // ---- tuple_all_elements_same_as ----
    record_assertion(_test_handler, 
        tuple_all_elements_same_as<std::tuple<>, int>::value == true,
        "tuple_all_elements_same_as: empty tuple vacuously true");
    record_assertion(_test_handler, 
        tuple_all_elements_same_as<std::tuple<int>, int>::value == true,
        "tuple_all_elements_same_as: single match");
    record_assertion(_test_handler, 
        tuple_all_elements_same_as<std::tuple<int>, char>::value == false,
        "tuple_all_elements_same_as: single mismatch");
    record_assertion(_test_handler, 
        tuple_all_elements_same_as<std::tuple<int, int, int>, int>::value == true,
        "tuple_all_elements_same_as: all match");
    record_assertion(_test_handler, 
        tuple_all_elements_same_as<std::tuple<int, char, int>, int>::value == false,
        "tuple_all_elements_same_as: partial match -> false");
    record_assertion(_test_handler, 
        tuple_all_elements_same_as<std::tuple<int, const int, int&>, int>::value == true,
        "tuple_all_elements_same_as: cv/ref variants match via clean_t");
    record_assertion(_test_handler, 
        tuple_all_elements_same_as<std::tuple<int>, const int>::value == false,
        "tuple_all_elements_same_as: probe is NOT cleaned (asymmetric)");

    // ---- all_inner_tuple_elements_one_type ----
    record_assertion(_test_handler, 
        all_inner_tuple_elements_one_type<int>::value == false,
        "all_inner_tuple_elements_one_type: non-tuple -> false");
    record_assertion(_test_handler, 
        all_inner_tuple_elements_one_type<std::tuple<>>::value == true,
        "all_inner_tuple_elements_one_type: empty outer -> true");
    record_assertion(_test_handler, 
        all_inner_tuple_elements_one_type<std::tuple<std::tuple<>>>::value == true,
        "all_inner_tuple_elements_one_type: single empty inner -> true");
    record_assertion(_test_handler, 
        all_inner_tuple_elements_one_type<std::tuple<std::tuple<>, std::tuple<>>>::value == true,
        "all_inner_tuple_elements_one_type: all empty inners -> true");
    record_assertion(_test_handler, 
        all_inner_tuple_elements_one_type<std::tuple<std::tuple<>, std::tuple<int>>>::value == false,
        "all_inner_tuple_elements_one_type: empty then non-empty -> false");
    record_assertion(_test_handler, 
        all_inner_tuple_elements_one_type<std::tuple<std::tuple<int>>>::value == true,
        "all_inner_tuple_elements_one_type: single inner of int -> true");
    record_assertion(_test_handler, 
        all_inner_tuple_elements_one_type<std::tuple<std::tuple<int, int, int>>>::value == true,
        "all_inner_tuple_elements_one_type: single inner of 3 ints -> true");
    record_assertion(_test_handler, 
        all_inner_tuple_elements_one_type<std::tuple<std::tuple<int>,
                                                      std::tuple<int>>>::value == true,
        "all_inner_tuple_elements_one_type: two inners of int -> true");
    record_assertion(_test_handler, 
        all_inner_tuple_elements_one_type<std::tuple<std::tuple<int>,
                                                      std::tuple<int, int>>>::value == true,
        "all_inner_tuple_elements_one_type: jagged sizes but uniform type -> true");
    record_assertion(_test_handler, 
        all_inner_tuple_elements_one_type<std::tuple<std::tuple<int, char>>>::value == false,
        "all_inner_tuple_elements_one_type: single inner with mixed elements -> false");
    record_assertion(_test_handler, 
        all_inner_tuple_elements_one_type<std::tuple<std::tuple<int>,
                                                      std::tuple<char>>>::value == false,
        "all_inner_tuple_elements_one_type: int row vs char row -> false");
    record_assertion(_test_handler, 
        all_inner_tuple_elements_one_type<std::tuple<std::tuple<int>,
                                                      std::tuple<>>>::value == false,
        "all_inner_tuple_elements_one_type: non-empty first, empty later -> false");
    record_assertion(_test_handler, 
        all_inner_tuple_elements_one_type<std::tuple<std::tuple<const int>,
                                                      std::tuple<int&>>>::value == true,
        "all_inner_tuple_elements_one_type: cv/ref normalized via clean_t");

    return;
}


NS_END  // testing
NS_END  // djinterp
