/******************************************************************************
* djinterp [test]                               dtuple_tests_transformation.cpp
*
*   Unit tests for the tuple-transformation section of dtuple.hpp:
*     - tuple_apply_all / tuple_apply_all_t
*
*   tuple_apply_all takes a unary metafunction and a pack (or a single
* tuple wrapping a pack) and yields a tuple of the transformed types.
* It internally normalizes its input through `to_tuple`, so the
* tuple-vs-pack distinction collapses early -- both call shapes must
* produce identical results for the same logical element list.
*
*   Edge cases:
*     - empty input (both as `tuple_apply_all<Trait>` with no pack and as
*       `tuple_apply_all<Trait, std::tuple<>>`) must yield `std::tuple<>`.
*     - the trait is applied positionally, NOT by type identity, so
*       repeated input types are transformed once per occurrence.
*     - the trait must be a one-argument template; we exercise this with
*       std::add_const, std::add_pointer, the identity helper from
*       dtuple_test_types, and a custom add_array_2 that produces a
*       different output type than its input.
*
*
* path:      /inc/djinterp/test/dtuple_tests_transformation.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.19
******************************************************************************/

#include "./dtuple_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace dtuple_test_types;


// =========================================================================
// I.   TUPLE_APPLY_ALL  (compile-time)
// =========================================================================

// empty pack -- to_tuple<>::type collapses to std::tuple<>, helper base
// case yields the same empty tuple
static_assert(std::is_same<typename tuple_apply_all<std::add_const>::type,
                           std::tuple<>>::value,
              "tuple_apply_all<add_const> (empty pack) should be std::tuple<>");

// single tuple holding the empty pack -- same answer via the unwrapping
// route
static_assert(std::is_same<typename tuple_apply_all<std::add_const_t,
                                                     std::tuple<>>::type,
                           std::tuple<>>::value,
              "tuple_apply_all<add_const_t, std::tuple<>>::type should be std::tuple<>");

// single-element transformations
static_assert(std::is_same<typename tuple_apply_all<std::add_const_t, int>::type,
                           std::tuple<const int>>::value,
              "tuple_apply_all<add_const_t, int>::type should be std::tuple<const int>");
static_assert(std::is_same<typename tuple_apply_all<std::add_pointer_t, int>::type,
                           std::tuple<int*>>::value,
              "tuple_apply_all<add_pointer_t, int>::type should be std::tuple<int*>");

// multi-element pack-shape
static_assert(std::is_same<typename tuple_apply_all<std::add_const_t, int, char, float>::type,
                           std::tuple<const int, const char, const float>>::value,
              "tuple_apply_all<add_const_t, int, char, float>::type should add const to each");
static_assert(std::is_same<typename tuple_apply_all<std::add_pointer_t,
                                                     int, char, double>::type,
                           std::tuple<int*, char*, double*>>::value,
              "tuple_apply_all<add_pointer_t, int, char, double>::type should add pointer to each");
static_assert(std::is_same<typename tuple_apply_all<std::add_volatile_t,
                                                     int, char>::type,
                           std::tuple<volatile int, volatile char>>::value,
              "tuple_apply_all<add_volatile_t, int, char>::type should add volatile to each");

// multi-element tuple-shape -- to_tuple unwraps it before iterating
static_assert(std::is_same<typename tuple_apply_all<std::add_const_t,
                                                     std::tuple<int, char, float>>::type,
                           std::tuple<const int, const char, const float>>::value,
              "tuple_apply_all<add_const_t, std::tuple<int, char, float>>::type should match pack-shape result");
static_assert(std::is_same<typename tuple_apply_all<std::add_pointer_t,
                                                     std::tuple<alpha, bravo>>::type,
                           std::tuple<alpha*, bravo*>>::value,
              "tuple_apply_all<add_pointer_t, std::tuple<alpha, bravo>>::type should add pointer to each");

// identity trait -- result should equal the input tuple shape
static_assert(std::is_same<typename tuple_apply_all<identity, int, char>::type,
                           std::tuple<int, char>>::value,
              "tuple_apply_all<identity, int, char>::type should be std::tuple<int, char>");
static_assert(std::is_same<typename tuple_apply_all<identity,
                                                     std::tuple<alpha, bravo, charlie>>::type,
                           std::tuple<alpha, bravo, charlie>>::value,
              "tuple_apply_all<identity, std::tuple<alpha, bravo, charlie>>::type should preserve the tuple");

// trait that REPLACES the type entirely (different output type) -- proves
// the transformation is independent of the input type identity
static_assert(std::is_same<typename tuple_apply_all<add_array_2, int, char>::type,
                           std::tuple<std::array<int, 2>, std::array<char, 2>>>::value,
              "tuple_apply_all<add_array_2, int, char>::type should wrap each in std::array<_,2>");

// repeated input types are transformed once per occurrence (positional)
static_assert(std::is_same<typename tuple_apply_all<std::add_const_t,
                                                     int, int, int>::type,
                           std::tuple<const int, const int, const int>>::value,
              "tuple_apply_all<add_const_t, int, int, int>::type should add const three times");

// alias consistency
static_assert(std::is_same<tuple_apply_all_t<std::add_const_t, int, char>,
                           typename tuple_apply_all<std::add_const_t, int, char>::type>::value,
              "tuple_apply_all_t: alias should match ::type");
static_assert(std::is_same<tuple_apply_all_t<std::add_pointer_t,
                                              std::tuple<int, char, float>>,
                           std::tuple<int*, char*, float*>>::value,
              "tuple_apply_all_t<add_pointer_t, std::tuple<int, char, float>> should be std::tuple<int*, char*, float*>");


// =========================================================================
// II.  RUNTIME DRIVER
// =========================================================================

void
dtuple_tests_transformation_all(
    test_handler& _test_handler
)
{
    record_assertion(_test_handler, 
        std::is_same<typename tuple_apply_all<std::add_const>::type,
                     std::tuple<>>::value,
        "tuple_apply_all: empty pack -> std::tuple<>");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_apply_all<std::add_const_t,
                                               std::tuple<>>::type,
                     std::tuple<>>::value,
        "tuple_apply_all: single empty tuple unwraps to std::tuple<>");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_apply_all<std::add_const_t, int>::type,
                     std::tuple<const int>>::value,
        "tuple_apply_all: single element add_const");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_apply_all<std::add_const_t, int, char, float>::type,
                     std::tuple<const int, const char, const float>>::value,
        "tuple_apply_all: pack-shape add_const");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_apply_all<std::add_const_t,
                                               std::tuple<int, char, float>>::type,
                     std::tuple<const int, const char, const float>>::value,
        "tuple_apply_all: tuple-shape add_const matches pack-shape");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_apply_all<std::add_pointer_t,
                                               int, char, double>::type,
                     std::tuple<int*, char*, double*>>::value,
        "tuple_apply_all: add_pointer over three types");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_apply_all<identity,
                                               std::tuple<alpha, bravo, charlie>>::type,
                     std::tuple<alpha, bravo, charlie>>::value,
        "tuple_apply_all: identity preserves the tuple");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_apply_all<add_array_2, int, char>::type,
                     std::tuple<std::array<int, 2>, std::array<char, 2>>>::value,
        "tuple_apply_all: trait that replaces type (add_array_2)");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_apply_all<std::add_const_t,
                                               int, int, int>::type,
                     std::tuple<const int, const int, const int>>::value,
        "tuple_apply_all: repeated types transformed positionally");
    record_assertion(_test_handler, 
        std::is_same<tuple_apply_all_t<std::add_pointer_t,
                                        std::tuple<int, char, float>>,
                     std::tuple<int*, char*, float*>>::value,
        "tuple_apply_all_t: alias matches ::type");

    return;
}


NS_END  // testing
NS_END  // djinterp
