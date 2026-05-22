/******************************************************************************
* djinterp [test]                                  type_traits_tests_logical.cpp
*
*   Unit tests for the logical metafunctions in Section I.1 and the
* portable logical-metafunction macros in Section II of type_traits.hpp:
*     - bool_constant
*     - conjunction        + conjunction_v
*     - disjunction        + disjunction_v
*     - negation           + negation_v
*     - D_CONJUNCTION      (macro)
*     - D_DISJUNCTION      (macro)
*     - D_NEGATION         (macro)
*
*   On C++17+ these are imported from std::, so the tests double as a
* smoke test of the using-declarations.  On C++11/14 they are djinterp's
* own implementations, and the tests below probe the corner cases of
* those:
*
*   - conjunction:
*       * empty pack -> std::true_type
*       * single arg -> inherits from that arg directly (i.e. the trait
*         is the truthiness of that one trait, not just bool)
*       * left-to-right scan stops at the first false; inherits from the
*         first false trait
*       * if all true, inherits from the LAST trait
*   - disjunction: symmetric (empty pack -> std::false_type)
*   - negation: flips the boolean of any unary trait
*   - D_CONJUNCTION/D_DISJUNCTION/D_NEGATION: macros must resolve to a
*     functioning logical metafunction in the active language mode --
*     we use them in the same shape as the bare names and confirm equal
*     answers.
*
*
* path:      /inc/djinterp/test/type_traits_tests_logical.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/
#include "./type_traits_tests.hpp"


NS_DJINTERP
NS_TEST


// =========================================================================
// I.   bool_constant  (compile-time)
// =========================================================================

static_assert(std::is_same<bool_constant<true>, std::true_type>::value,
              "bool_constant<true> == std::true_type");
static_assert(std::is_same<bool_constant<false>, std::false_type>::value,
              "bool_constant<false> == std::false_type");
static_assert(bool_constant<true>::value  == true,
              "bool_constant<true>::value == true");
static_assert(bool_constant<false>::value == false,
              "bool_constant<false>::value == false");


// =========================================================================
// II.  conjunction  (compile-time)
// =========================================================================

// empty pack -> true
static_assert(conjunction<>::value == true,
              "conjunction<>: empty pack -> true (identity element)");

// single arg -> inherits from that trait
static_assert(conjunction<std::true_type>::value  == true,
              "conjunction<true_type> -> true");
static_assert(conjunction<std::false_type>::value == false,
              "conjunction<false_type> -> false");

// all true -> last trait wins
static_assert(conjunction<std::true_type, std::true_type, std::true_type>::value == true,
              "conjunction<all true> -> true");

// any false -> false; returns the FIRST false
static_assert(conjunction<std::true_type, std::false_type, std::true_type>::value == false,
              "conjunction<true, false, true> -> false");
static_assert(conjunction<std::false_type, std::true_type>::value == false,
              "conjunction<false, true> -> false");

// nested in another type-list -- using is_integral, is_pointer
static_assert(conjunction<std::is_integral<int>,
                          std::is_integral<char>>::value == true,
              "conjunction<is_integral<int>, is_integral<char>> -> true");
static_assert(conjunction<std::is_integral<int>,
                          std::is_pointer<int>>::value == false,
              "conjunction<is_integral<int>, is_pointer<int>> -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(conjunction_v<std::true_type, std::true_type>  == true,
                  "conjunction_v: all true");
    static_assert(conjunction_v<std::true_type, std::false_type> == false,
                  "conjunction_v: any false");
#endif


// =========================================================================
// III. disjunction  (compile-time)
// =========================================================================

// empty pack -> false (identity for OR)
static_assert(disjunction<>::value == false,
              "disjunction<>: empty pack -> false (identity element)");

// single arg -> inherits
static_assert(disjunction<std::true_type>::value  == true,
              "disjunction<true_type> -> true");
static_assert(disjunction<std::false_type>::value == false,
              "disjunction<false_type> -> false");

// all false -> last trait wins (false)
static_assert(disjunction<std::false_type, std::false_type, std::false_type>::value == false,
              "disjunction<all false> -> false");

// any true -> returns the FIRST true
static_assert(disjunction<std::false_type, std::true_type, std::false_type>::value == true,
              "disjunction<false, true, false> -> true");
static_assert(disjunction<std::true_type, std::false_type>::value == true,
              "disjunction<true, false> -> true");

// real traits
static_assert(disjunction<std::is_pointer<int>,
                          std::is_integral<int>>::value == true,
              "disjunction<is_pointer<int>, is_integral<int>> -> true");
static_assert(disjunction<std::is_pointer<int>,
                          std::is_floating_point<int>>::value == false,
              "disjunction<is_pointer<int>, is_floating_point<int>> -> false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(disjunction_v<std::false_type, std::true_type>  == true,
                  "disjunction_v: any true");
    static_assert(disjunction_v<std::false_type, std::false_type> == false,
                  "disjunction_v: all false");
#endif


// =========================================================================
// IV.  negation  (compile-time)
// =========================================================================

static_assert(negation<std::true_type>::value  == false,
              "negation<true_type> -> false");
static_assert(negation<std::false_type>::value == true,
              "negation<false_type> -> true");

static_assert(negation<std::is_integral<int>>::value   == false,
              "negation<is_integral<int>> -> false");
static_assert(negation<std::is_pointer<int>>::value    == true,
              "negation<is_pointer<int>> -> true");

// double negation
static_assert(negation<negation<std::true_type>>::value == true,
              "negation<negation<true_type>> -> true (double negation)");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(negation_v<std::true_type>  == false,
                  "negation_v<true_type> -> false");
    static_assert(negation_v<std::false_type> == true,
                  "negation_v<false_type> -> true");
#endif


// =========================================================================
// V.   D_CONJUNCTION / D_DISJUNCTION / D_NEGATION  (compile-time)
// =========================================================================
//   The macros must resolve to functioning logical metafunctions and
// must agree with the bare-name versions.

static_assert(D_CONJUNCTION<std::true_type, std::true_type>::value == true,
              "D_CONJUNCTION: all true -> true");
static_assert(D_CONJUNCTION<std::true_type, std::false_type>::value == false,
              "D_CONJUNCTION: any false -> false");
static_assert(D_CONJUNCTION<>::value == true,
              "D_CONJUNCTION: empty -> true");

static_assert(D_DISJUNCTION<std::false_type, std::true_type>::value == true,
              "D_DISJUNCTION: any true -> true");
static_assert(D_DISJUNCTION<std::false_type, std::false_type>::value == false,
              "D_DISJUNCTION: all false -> false");
static_assert(D_DISJUNCTION<>::value == false,
              "D_DISJUNCTION: empty -> false");

static_assert(D_NEGATION<std::true_type>::value  == false,
              "D_NEGATION: true -> false");
static_assert(D_NEGATION<std::false_type>::value == true,
              "D_NEGATION: false -> true");

// macros agree with bare names
static_assert(D_CONJUNCTION<std::true_type, std::true_type>::value ==
              conjunction<std::true_type, std::true_type>::value,
              "D_CONJUNCTION agrees with conjunction");
static_assert(D_DISJUNCTION<std::false_type, std::true_type>::value ==
              disjunction<std::false_type, std::true_type>::value,
              "D_DISJUNCTION agrees with disjunction");
static_assert(D_NEGATION<std::true_type>::value ==
              negation<std::true_type>::value,
              "D_NEGATION agrees with negation");


// =========================================================================
// VI.  RUNTIME DRIVER
// =========================================================================

void
type_traits_tests_logical(
    test_handler& _test_handler
)
{
    // ---- bool_constant ----
    record_assertion(_test_handler, 
        std::is_same<bool_constant<true>, std::true_type>::value,
        "bool_constant<true> == std::true_type");
    record_assertion(_test_handler, 
        std::is_same<bool_constant<false>, std::false_type>::value,
        "bool_constant<false> == std::false_type");

    // ---- conjunction ----
    record_assertion(_test_handler, 
        conjunction<>::value == true,
        "conjunction<> -> true (empty)");
    record_assertion(_test_handler, 
        conjunction<std::true_type>::value == true,
        "conjunction<true>");
    record_assertion(_test_handler, 
        conjunction<std::true_type, std::true_type, std::true_type>::value == true,
        "conjunction<all true>");
    record_assertion(_test_handler, 
        conjunction<std::true_type, std::false_type, std::true_type>::value == false,
        "conjunction<any false>");
    record_assertion(_test_handler, 
        conjunction<std::is_integral<int>, std::is_integral<char>>::value == true,
        "conjunction<is_integral<int>, is_integral<char>>");
    record_assertion(_test_handler, 
        conjunction<std::is_integral<int>, std::is_pointer<int>>::value == false,
        "conjunction<is_integral<int>, is_pointer<int>>");

    // ---- disjunction ----
    record_assertion(_test_handler, 
        disjunction<>::value == false,
        "disjunction<> -> false (empty)");
    record_assertion(_test_handler, 
        disjunction<std::true_type>::value == true,
        "disjunction<true>");
    record_assertion(_test_handler, 
        disjunction<std::false_type, std::false_type, std::false_type>::value == false,
        "disjunction<all false>");
    record_assertion(_test_handler, 
        disjunction<std::false_type, std::true_type, std::false_type>::value == true,
        "disjunction<any true>");
    record_assertion(_test_handler, 
        disjunction<std::is_pointer<int>, std::is_integral<int>>::value == true,
        "disjunction<is_pointer<int>, is_integral<int>>");

    // ---- negation ----
    record_assertion(_test_handler, 
        negation<std::true_type>::value  == false,
        "negation<true>");
    record_assertion(_test_handler, 
        negation<std::false_type>::value == true,
        "negation<false>");
    record_assertion(_test_handler, 
        negation<negation<std::true_type>>::value == true,
        "negation<negation<true>> (double)");

    // ---- D_CONJUNCTION / D_DISJUNCTION / D_NEGATION ----
    record_assertion(_test_handler, 
        D_CONJUNCTION<std::true_type, std::true_type>::value == true,
        "D_CONJUNCTION<true, true> -> true");
    record_assertion(_test_handler, 
        D_CONJUNCTION<std::true_type, std::false_type>::value == false,
        "D_CONJUNCTION<true, false> -> false");
    record_assertion(_test_handler, 
        D_DISJUNCTION<std::false_type, std::true_type>::value == true,
        "D_DISJUNCTION<false, true> -> true");
    record_assertion(_test_handler, 
        D_DISJUNCTION<std::false_type, std::false_type>::value == false,
        "D_DISJUNCTION<false, false> -> false");
    record_assertion(_test_handler, 
        D_NEGATION<std::true_type>::value == false,
        "D_NEGATION<true>");
    record_assertion(_test_handler, 
        D_NEGATION<std::false_type>::value == true,
        "D_NEGATION<false>");

    return;
}


NS_END  // test
NS_END  // djinterp
