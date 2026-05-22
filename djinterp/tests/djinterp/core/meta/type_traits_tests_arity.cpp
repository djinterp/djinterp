/******************************************************************************
* djinterp [test]                                    type_traits_tests_arity.cpp
*
*   Unit tests for the arity / single-arg detection traits in Section
* III of type_traits.hpp:
*     - is_zero            (alias template; no _v companion)
*     - is_nonzero, _v
*     - is_single_arg,     _v
*     - is_single_type_arg, _v
*
*   Coverage:
*   - is_zero<N>: true iff N == 0
*   - is_nonzero<N>: true iff N != 0 (complementary)
*   - is_single_arg<...>: true iff pack has EXACTLY one type; for the
*     true case, ::type names the single type
*   - is_single_type_arg<_Type, _Types...>: true iff _Types has exactly
*     one type AND that type is _Type
*
*   KNOWN BUG: is_single_type_arg<_Type> (with NO pack, i.e. _Types
* empty) accesses is_single_arg<>::type. The empty-pack primary template
* of is_single_arg is std::false_type and has no `::type` member, so
* this is a HARD ERROR rather than a SFINAE-friendly false. We DO NOT
* exercise the empty-pack case for is_single_type_arg.
*
*
* path:      /inc/djinterp/test/type_traits_tests_arity.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/
#include "./type_traits_tests.hpp"


NS_DJINTERP
NS_TEST


// =========================================================================
// I.   is_zero  (compile-time)
// =========================================================================

static_assert(is_zero<0>::value == true,
              "is_zero<0> -> true");
static_assert(is_zero<1>::value == false,
              "is_zero<1> -> false");
static_assert(is_zero<5>::value == false,
              "is_zero<5> -> false");
static_assert(is_zero<100>::value == false,
              "is_zero<100> -> false");
static_assert(is_zero<SIZE_MAX>::value == false,
              "is_zero<SIZE_MAX> -> false");


// =========================================================================
// II.  is_nonzero  (compile-time)
// =========================================================================

static_assert(is_nonzero<0>::value == false,
              "is_nonzero<0> -> false");
static_assert(is_nonzero<1>::value == true,
              "is_nonzero<1> -> true");
static_assert(is_nonzero<5>::value == true,
              "is_nonzero<5> -> true");
static_assert(is_nonzero<SIZE_MAX>::value == true,
              "is_nonzero<SIZE_MAX> -> true");

// complementary
static_assert(is_zero<0>::value != is_nonzero<0>::value,
              "is_zero<0> and is_nonzero<0> are complementary");
static_assert(is_zero<7>::value != is_nonzero<7>::value,
              "is_zero<7> and is_nonzero<7> are complementary");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_nonzero_v<0> == false,
                  "is_nonzero_v<0> -> false");
    static_assert(is_nonzero_v<5> == true,
                  "is_nonzero_v<5> -> true");
#endif


// =========================================================================
// III. is_single_arg  (compile-time)
// =========================================================================

// empty pack -> false
static_assert(is_single_arg<>::value == false,
              "is_single_arg<> -> false (empty pack)");

// single arg -> true; ::type names the type
static_assert(is_single_arg<int>::value == true,
              "is_single_arg<int> -> true");
static_assert(std::is_same<typename is_single_arg<int>::type, int>::value,
              "is_single_arg<int>::type == int");
static_assert(is_single_arg<double>::value == true,
              "is_single_arg<double> -> true");
static_assert(std::is_same<typename is_single_arg<double>::type, double>::value,
              "is_single_arg<double>::type == double");
static_assert(is_single_arg<std::vector<int>>::value == true,
              "is_single_arg<vector<int>> -> true");

// multi-arg pack -> false
static_assert(is_single_arg<int, char>::value == false,
              "is_single_arg<int, char> -> false");
static_assert(is_single_arg<int, char, float>::value == false,
              "is_single_arg<int, char, float> -> false");
static_assert(is_single_arg<int, int>::value == false,
              "is_single_arg<int, int> -> false (two of same type still multi-arg)");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_single_arg_v<int> == true,
                  "is_single_arg_v<int> -> true");
    static_assert(is_single_arg_v<>     == false,
                  "is_single_arg_v<> -> false");
    static_assert(is_single_arg_v<int, char> == false,
                  "is_single_arg_v<int, char> -> false");
#endif


// =========================================================================
// IV.  is_single_type_arg  (compile-time)
// =========================================================================
//   `is_single_type_arg<_Probe, _Types...>` -> true iff _Types has
// exactly one element AND that element == _Probe.

// positive case -- exact match
static_assert(is_single_type_arg<int, int>::value == true,
              "is_single_type_arg<int, int> -> true");
static_assert(is_single_type_arg<double, double>::value == true,
              "is_single_type_arg<double, double> -> true");

// negative case -- single arg but wrong type
static_assert(is_single_type_arg<int, char>::value == false,
              "is_single_type_arg<int, char> -> false (type mismatch)");
static_assert(is_single_type_arg<long, int>::value == false,
              "is_single_type_arg<long, int> -> false (long != int)");

// negative case -- wrong arity (2 instead of 1)
static_assert(is_single_type_arg<int, int, int>::value == false,
              "is_single_type_arg<int, int, int> -> false (arity)");

// NOTE: is_single_type_arg<int> (no pack -- empty _Types...) is NOT
// tested because it hard-errors: see file header.

// _v alias -- note its signature is `<_Types...>` not `<_Type, _Types...>`,
// so the first type given to it acts as the probe and the rest as the
// candidates.
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_single_type_arg_v<int, int> == true,
                  "is_single_type_arg_v<int, int> -> true (matches struct)");
    static_assert(is_single_type_arg_v<int, char> == false,
                  "is_single_type_arg_v<int, char> -> false (matches struct)");
#endif


// =========================================================================
// V.   RUNTIME DRIVER
// =========================================================================

void
type_traits_tests_arity(
    test_handler& _test_handler
)
{
    // ---- is_zero ----
    record_assertion(_test_handler, 
        is_zero<0>::value == true,
        "is_zero<0>");
    record_assertion(_test_handler, 
        is_zero<1>::value == false,
        "is_zero<1>");
    record_assertion(_test_handler, 
        is_zero<5>::value == false,
        "is_zero<5>");
    record_assertion(_test_handler, 
        is_zero<SIZE_MAX>::value == false,
        "is_zero<SIZE_MAX>");

    // ---- is_nonzero ----
    record_assertion(_test_handler, 
        is_nonzero<0>::value == false,
        "is_nonzero<0>");
    record_assertion(_test_handler, 
        is_nonzero<1>::value == true,
        "is_nonzero<1>");
    record_assertion(_test_handler, 
        is_nonzero<SIZE_MAX>::value == true,
        "is_nonzero<SIZE_MAX>");

    // complementarity
    record_assertion(_test_handler, 
        is_zero<0>::value != is_nonzero<0>::value,
        "is_zero/is_nonzero complementary at 0");
    record_assertion(_test_handler, 
        is_zero<7>::value != is_nonzero<7>::value,
        "is_zero/is_nonzero complementary at 7");

    // ---- is_single_arg ----
    record_assertion(_test_handler, 
        is_single_arg<>::value == false,
        "is_single_arg<> -> false");
    record_assertion(_test_handler, 
        is_single_arg<int>::value == true,
        "is_single_arg<int> -> true");
    record_assertion(_test_handler, 
        std::is_same<typename is_single_arg<int>::type, int>::value,
        "is_single_arg<int>::type == int");
    record_assertion(_test_handler, 
        is_single_arg<int, char>::value == false,
        "is_single_arg<int, char> -> false");
    record_assertion(_test_handler, 
        is_single_arg<int, char, float>::value == false,
        "is_single_arg<int, char, float> -> false");
    record_assertion(_test_handler, 
        is_single_arg<int, int>::value == false,
        "is_single_arg<int, int> -> false (duplicates still multi-arg)");

    // ---- is_single_type_arg ----
    record_assertion(_test_handler, 
        is_single_type_arg<int, int>::value == true,
        "is_single_type_arg<int, int>");
    record_assertion(_test_handler, 
        is_single_type_arg<double, double>::value == true,
        "is_single_type_arg<double, double>");
    record_assertion(_test_handler, 
        is_single_type_arg<int, char>::value == false,
        "is_single_type_arg<int, char> (mismatch)");
    record_assertion(_test_handler, 
        is_single_type_arg<int, int, int>::value == false,
        "is_single_type_arg<int, int, int> (arity)");

    return;
}


NS_END  // test
NS_END  // djinterp
