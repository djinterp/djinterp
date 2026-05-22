/******************************************************************************
* djinterp [test]                                 type_traits_tests_evaluate.cpp
*
*   Unit tests for the trait-evaluation utilities in Section III of
* type_traits.hpp:
*     - evaluate_types_for_trait      + evaluate_types_for_trait_v
*     - are_all_nonvoid               + are_all_nonvoid_v
*     - exclusive_disjunction         + exclusive_disjunction_v
*
*   KNOWN BUGS in evaluate_types_for_trait (documented for the user, not
* worked around in the macro; tests demonstrate the misbehaviour):
*
*   B1. evaluate_all_for_trait_helper<std::tuple<_Type>>:
*       The single-element-tuple base specialization inherits from
*       std::true_type unconditionally, regardless of the supplied
*       trait. So evaluate_types_for_trait<std::tuple<int>, std::is_void>
*       reports true even though int is plainly not void.
*
*   B2. evaluate_all_for_trait_helper has no specialization for
*       std::tuple<>: instantiating evaluate_types_for_trait<std::tuple<>,
*       Trait> hits the undefined primary -> "incomplete type" hard
*       error.  We DO NOT exercise that case (it cannot compile).
*
*   B3. evaluate_types_for_trait_v ignores the user's _Evaluator argument
*       and hardcodes std::conjunction.  The test below documents this
*       by demonstrating that the _v alias and the trait class form give
*       DIFFERENT answers when the evaluator is std::disjunction.
*
*   exclusive_disjunction is well-formed; we cover:
*   - empty pack -> std::false_type (identity for XOR)
*   - single arg -> inherits from that one trait
*   - two args  -> XOR
*   - three+ args -> note the implementation uses "(B1 != B2) && rec(...)",
*     so multi-arg XOR isn't true logical-XOR over the pack -- we verify
*     the implemented behaviour rather than the textbook XOR semantics
*
*
* path:      /inc/djinterp/test/type_traits_tests_evaluate.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.20
******************************************************************************/
#include "./type_traits_tests.hpp"


NS_DJINTERP
NS_TEST



// =========================================================================
// I.   evaluate_types_for_trait  (compile-time)
// =========================================================================

// default evaluator (conjunction) -- ALL types must satisfy the trait
static_assert(evaluate_types_for_trait<std::tuple<int, char, long>,
                                       std::is_integral>::value == true,
              "evaluate_types_for_trait<conjunction>: all integral -> true");
static_assert(evaluate_types_for_trait<std::tuple<int, char, float>,
                                       std::is_integral>::value == false,
              "evaluate_types_for_trait<conjunction>: one non-integral -> false");
static_assert(evaluate_types_for_trait<std::tuple<float, double, int>,
                                       std::is_floating_point>::value == false,
              "evaluate_types_for_trait<conjunction>: int among floats -> false");

// disjunction evaluator -- AT LEAST ONE type must satisfy the trait
static_assert(evaluate_types_for_trait<std::tuple<int, float>,
                                       std::is_integral,
                                       std::disjunction>::value == true,
              "evaluate_types_for_trait<disjunction>: at least one integral -> true");
static_assert(evaluate_types_for_trait<std::tuple<float, double>,
                                       std::is_integral,
                                       std::disjunction>::value == false,
              "evaluate_types_for_trait<disjunction>: no integral -> false");

// NOTE on bug B1: single-element tuple ALWAYS returns true. So
// evaluate_types_for_trait<std::tuple<int>, std::is_void> wrongly
// reports true. We document the broken behaviour:
static_assert(evaluate_types_for_trait<std::tuple<int>, std::is_void>::value == true,
              "[BUG B1] evaluate_types_for_trait<std::tuple<int>, std::is_void> wrongly true");
static_assert(evaluate_types_for_trait<std::tuple<int>, std::is_integral>::value == true,
              "evaluate_types_for_trait<std::tuple<int>, std::is_integral>: true (coincidentally correct)");

// Also note: bug B2 — we cannot test evaluate_types_for_trait<std::tuple<>, ...>
// because the implementation has no empty-pack specialization. Omitted.


// =========================================================================
// II.  evaluate_types_for_trait_v  (compile-time -- bug B3 demo)
// =========================================================================
//   The _v alias ignores the supplied evaluator and forces conjunction.
// This is observable by comparing the _v alias's answer with the struct
// form's answer on a case that differs between conjunction and
// disjunction.

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // case: {int, float}, trait = is_integral
    //   - conjunction -> false (float not integral)
    //   - disjunction -> true  (int IS integral)
    // The _v alias should match whatever the user requested; if it
    // hardcodes conjunction, it'll disagree with the struct form when
    // the user requested disjunction.

    static_assert(evaluate_types_for_trait<std::tuple<int, float>,
                                           std::is_integral,
                                           std::disjunction>::value == true,
                  "[reference] struct-form disjunction: int|float has integral -> true");

    static_assert(evaluate_types_for_trait_v<std::tuple<int, float>,
                                              std::is_integral,
                                              std::disjunction> == false,
                  "[BUG B3] _v alias hardcodes conjunction; reports false (struct returns true)");
#endif


// =========================================================================
// III. are_all_nonvoid  (compile-time)
// =========================================================================
//   are_all_nonvoid<T1, T2, ...> -> true iff none of the supplied types
// are void.
//
//   KNOWN BUG B4: are_all_nonvoid has a single-arg specialization that
// directly inherits from evaluate_types_for_trait<tuple<_Type>, is_void,
// disjunction>.  That helper hits bug B1's single-element base, so it
// returns true regardless of _Type, and the specialization does NOT
// apply std::negation the way the variadic primary does. Result:
// are_all_nonvoid<void>::value is TRUE (wrong).

// positive cases (no voids)
static_assert(are_all_nonvoid<int>::value == true,
              "are_all_nonvoid<int> -> true");
static_assert(are_all_nonvoid<int, char, double>::value == true,
              "are_all_nonvoid<int, char, double> -> true");
static_assert(are_all_nonvoid<std::vector<int>, char*, int>::value == true,
              "are_all_nonvoid<complex types, none void> -> true");

// negative cases that work (multi-arg primary applies negation properly)
static_assert(are_all_nonvoid<void, int>::value == false,
              "are_all_nonvoid<void, int> -> false");
static_assert(are_all_nonvoid<int, void>::value == false,
              "are_all_nonvoid<int, void> -> false");
static_assert(are_all_nonvoid<int, char, void, double>::value == false,
              "are_all_nonvoid<one void in middle> -> false");

// BUG B4 demonstration: single-arg are_all_nonvoid<void> wrongly true
static_assert(are_all_nonvoid<void>::value == true,
              "[BUG B4] are_all_nonvoid<void> wrongly returns true (single-arg spec missing negation)");

// _v alias
static_assert(are_all_nonvoid_v<int, char, double> == true,
              "are_all_nonvoid_v: matches struct form (true)");
static_assert(are_all_nonvoid_v<int, void, double> == false,
              "are_all_nonvoid_v: matches struct form (false)");


// =========================================================================
// IV.  exclusive_disjunction  (compile-time)
// =========================================================================

// empty pack -> false (identity for XOR)
static_assert(exclusive_disjunction<>::value == false,
              "exclusive_disjunction<>: empty pack -> false");

// single arg -> inherits from that trait
static_assert(exclusive_disjunction<std::true_type>::value  == true,
              "exclusive_disjunction<true> -> true");
static_assert(exclusive_disjunction<std::false_type>::value == false,
              "exclusive_disjunction<false> -> false");

// two args -- proper XOR
static_assert(exclusive_disjunction<std::true_type, std::true_type>::value   == false,
              "exclusive_disjunction<T, T> -> false (T XOR T = false)");
static_assert(exclusive_disjunction<std::true_type, std::false_type>::value  == true,
              "exclusive_disjunction<T, F> -> true (T XOR F = true)");
static_assert(exclusive_disjunction<std::false_type, std::true_type>::value  == true,
              "exclusive_disjunction<F, T> -> true (F XOR T = true)");
static_assert(exclusive_disjunction<std::false_type, std::false_type>::value == false,
              "exclusive_disjunction<F, F> -> false (F XOR F = false)");

// three args -- implementation uses "(B1 != B2) && rec(...)"; verify
// what the implementation actually computes (not textbook XOR over pack)
//   - <T, F, F>:   B1!=B2 = true,  rec<F> = false -> false
//   - <F, T, F>:   B1!=B2 = true,  rec<F> = false -> false (NOT textbook XOR=T)
//   - <T, T, T>:   B1!=B2 = false                  -> false
//   - <T, F, T>:   B1!=B2 = true,  rec<T> = true  -> true
//   - <F, T, T>:   B1!=B2 = true,  rec<T> = true  -> true
static_assert(exclusive_disjunction<std::true_type,
                                     std::false_type,
                                     std::false_type>::value == false,
              "exclusive_disjunction<T,F,F>: rec<F>=false -> false");
static_assert(exclusive_disjunction<std::true_type,
                                     std::true_type,
                                     std::true_type>::value == false,
              "exclusive_disjunction<T,T,T>: B1!=B2 false -> false");
static_assert(exclusive_disjunction<std::true_type,
                                     std::false_type,
                                     std::true_type>::value == true,
              "exclusive_disjunction<T,F,T>: B1!=B2 true AND rec<T>=true -> true");

// _v alias
static_assert(exclusive_disjunction_v<std::true_type, std::false_type> == true,
              "exclusive_disjunction_v: T XOR F -> true");
static_assert(exclusive_disjunction_v<std::false_type, std::false_type> == false,
              "exclusive_disjunction_v: F XOR F -> false");


// =========================================================================
// V.   RUNTIME DRIVER
// =========================================================================

void
type_traits_tests_evaluate(
    test_handler& _test_handler
)
{
    // ---- evaluate_types_for_trait ----
    record_assertion(_test_handler, 
        evaluate_types_for_trait<std::tuple<int, char, long>,
                                 std::is_integral>::value == true,
        "evaluate<conjunction>: all integral");
    record_assertion(_test_handler, 
        evaluate_types_for_trait<std::tuple<int, char, float>,
                                 std::is_integral>::value == false,
        "evaluate<conjunction>: one non-integral -> false");
    record_assertion(_test_handler, 
        evaluate_types_for_trait<std::tuple<int, float>,
                                 std::is_integral,
                                 std::disjunction>::value == true,
        "evaluate<disjunction>: at least one integral -> true");
    record_assertion(_test_handler, 
        evaluate_types_for_trait<std::tuple<float, double>,
                                 std::is_integral,
                                 std::disjunction>::value == false,
        "evaluate<disjunction>: no integral -> false");

    // bug B1
    record_assertion(_test_handler, 
        evaluate_types_for_trait<std::tuple<int>, std::is_void>::value == true,
        "[BUG B1] single-elem tuple wrongly reports true (is_void on int)");

    // bug B3
    record_assertion(_test_handler, 
        evaluate_types_for_trait_v<std::tuple<int, float>,
                                    std::is_integral,
                                    std::disjunction> == false,
        "[BUG B3] _v alias hardcodes conjunction -> disagrees with struct form");

    // ---- are_all_nonvoid ----
    record_assertion(_test_handler, 
        are_all_nonvoid<int>::value == true,
        "are_all_nonvoid<int>");
    record_assertion(_test_handler, 
        are_all_nonvoid<int, char, double>::value == true,
        "are_all_nonvoid<int, char, double>");
    record_assertion(_test_handler, 
        are_all_nonvoid<void>::value == true,
        "[BUG B4] are_all_nonvoid<void> wrongly true (single-arg spec missing negation)");
    record_assertion(_test_handler, 
        are_all_nonvoid<void, int>::value == false,
        "are_all_nonvoid<void, int>");
    record_assertion(_test_handler, 
        are_all_nonvoid<int, void>::value == false,
        "are_all_nonvoid<int, void>");
    record_assertion(_test_handler, 
        are_all_nonvoid<int, char, void, double>::value == false,
        "are_all_nonvoid<one void in middle>");
    record_assertion(_test_handler, 
        are_all_nonvoid_v<int, char> == true,
        "are_all_nonvoid_v matches struct (true)");
    record_assertion(_test_handler, 
        are_all_nonvoid_v<int, void> == false,
        "are_all_nonvoid_v matches struct (false)");

    // ---- exclusive_disjunction ----
    record_assertion(_test_handler, 
        exclusive_disjunction<>::value == false,
        "exclusive_disjunction<> -> false (identity)");
    record_assertion(_test_handler, 
        exclusive_disjunction<std::true_type>::value == true,
        "exclusive_disjunction<true>");
    record_assertion(_test_handler, 
        exclusive_disjunction<std::false_type>::value == false,
        "exclusive_disjunction<false>");
    record_assertion(_test_handler, 
        exclusive_disjunction<std::true_type, std::false_type>::value == true,
        "exclusive_disjunction<T, F> -> true");
    record_assertion(_test_handler, 
        exclusive_disjunction<std::true_type, std::true_type>::value == false,
        "exclusive_disjunction<T, T> -> false");
    record_assertion(_test_handler, 
        exclusive_disjunction<std::false_type, std::false_type>::value == false,
        "exclusive_disjunction<F, F> -> false");
    record_assertion(_test_handler, 
        exclusive_disjunction<std::true_type, std::false_type,
                              std::true_type>::value == true,
        "exclusive_disjunction<T, F, T> -> true (per implementation)");
    record_assertion(_test_handler, 
        exclusive_disjunction<std::true_type, std::true_type,
                              std::true_type>::value == false,
        "exclusive_disjunction<T, T, T> -> false (B1!=B2 false)");
    record_assertion(_test_handler, 
        exclusive_disjunction_v<std::true_type, std::false_type> == true,
        "exclusive_disjunction_v matches struct (true)");
    record_assertion(_test_handler, 
        exclusive_disjunction_v<std::false_type, std::false_type> == false,
        "exclusive_disjunction_v matches struct (false)");

    return;
}


NS_END  // test
NS_END  // djinterp
