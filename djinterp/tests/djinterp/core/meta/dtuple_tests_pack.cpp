/******************************************************************************
* djinterp [test]                                         dtuple_tests_pack.cpp
*
*   Unit tests for the parameter-pack-utility and tuple-detection sections
* of dtuple.hpp:
*     - first_arg / first_arg_t
*     - is_tuple_single_arg / is_tuple_single_arg_v
*     - is_tuple / is_tuple_v
*     - is_single_tuple_arg / is_single_tuple_arg_v
*
*   Every compile-time predicate is enforced via a file-scope
* `static_assert` -- the test_handler / sink calls at runtime simply
* mirror those assertions for reporting.  Edge cases under scrutiny
* include:
*     - empty parameter packs (where first_arg is intentionally
*       undefined -- only is_tuple_single_arg is exercised here)
*     - cv- and reference-qualified tuple types (the `is_tuple`
*       partial specialization deliberately matches only the bare
*       `std::tuple<...>` shape, so `is_tuple<const std::tuple<int>>`
*       resolves to false_type -- exercised below)
*     - single-tuple-arg detection for empty tuples and nested tuples
*
*
* path:      /inc/djinterp/test/dtuple_tests_pack.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.19
******************************************************************************/

#include "./dtuple_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace dtuple_test_types;


// =========================================================================
// I.   FIRST_ARG (compile-time)
// =========================================================================

// single argument
static_assert(std::is_same<typename first_arg<int>::type, int>::value,
              "first_arg<int>::type should be int");
static_assert(std::is_same<typename first_arg<alpha>::type, alpha>::value,
              "first_arg<alpha>::type should be alpha");
static_assert(std::is_same<typename first_arg<void>::type, void>::value,
              "first_arg<void>::type should be void");

// two arguments -- only the first is taken
static_assert(std::is_same<typename first_arg<int, char>::type, int>::value,
              "first_arg<int, char>::type should be int");
static_assert(std::is_same<typename first_arg<alpha, bravo>::type, alpha>::value,
              "first_arg<alpha, bravo>::type should be alpha");

// many arguments -- only the first is taken
static_assert(std::is_same<typename first_arg<int, char, float, double>::type, int>::value,
              "first_arg<int, char, float, double>::type should be int");
static_assert(std::is_same<typename first_arg<alpha, bravo, charlie, delta>::type, alpha>::value,
              "first_arg<alpha, bravo, charlie, delta>::type should be alpha");

// cv-/ref-qualification must be preserved by first_arg
static_assert(std::is_same<typename first_arg<const int>::type, const int>::value,
              "first_arg<const int>::type should preserve const");
static_assert(std::is_same<typename first_arg<int&>::type, int&>::value,
              "first_arg<int&>::type should preserve lvalue ref");
static_assert(std::is_same<typename first_arg<int&&, char>::type, int&&>::value,
              "first_arg<int&&, char>::type should preserve rvalue ref");
static_assert(std::is_same<typename first_arg<const volatile int*, char>::type,
                           const volatile int*>::value,
              "first_arg<const volatile int*, char>::type should preserve cv and pointer");

// first_arg accepts tuple types as one element of the pack
static_assert(std::is_same<typename first_arg<std::tuple<int>>::type,
                           std::tuple<int>>::value,
              "first_arg<std::tuple<int>>::type should be std::tuple<int>");
static_assert(std::is_same<typename first_arg<std::tuple<int, char>, alpha>::type,
                           std::tuple<int, char>>::value,
              "first_arg<std::tuple<int, char>, alpha>::type should be std::tuple<int, char>");

// alias consistency: first_arg_t<Ts...> == first_arg<Ts...>::type
static_assert(std::is_same<first_arg_t<int>,
                           typename first_arg<int>::type>::value,
              "first_arg_t<int> should equal first_arg<int>::type");
static_assert(std::is_same<first_arg_t<int, char, float>,
                           typename first_arg<int, char, float>::type>::value,
              "first_arg_t<int, char, float> should equal first_arg<int, char, float>::type");


// =========================================================================
// II.  IS_TUPLE_SINGLE_ARG (compile-time)
// =========================================================================

// empty pack -- primary template, false
static_assert(is_tuple_single_arg<>::value == false,
              "is_tuple_single_arg<>::value should be false (empty pack)");

// exactly one element -- partial specialization, true
static_assert(is_tuple_single_arg<int>::value == true,
              "is_tuple_single_arg<int>::value should be true");
static_assert(is_tuple_single_arg<void>::value == true,
              "is_tuple_single_arg<void>::value should be true");
static_assert(is_tuple_single_arg<std::tuple<>>::value == true,
              "is_tuple_single_arg<std::tuple<>>::value should be true (still a single arg)");
static_assert(is_tuple_single_arg<std::tuple<int, char>>::value == true,
              "is_tuple_single_arg<std::tuple<int, char>>::value should be true");
static_assert(is_tuple_single_arg<alpha>::value == true,
              "is_tuple_single_arg<alpha>::value should be true");

// two or more elements -- primary template, false
static_assert(is_tuple_single_arg<int, char>::value == false,
              "is_tuple_single_arg<int, char>::value should be false");
static_assert(is_tuple_single_arg<int, char, float>::value == false,
              "is_tuple_single_arg<int, char, float>::value should be false");
static_assert(is_tuple_single_arg<std::tuple<int>, std::tuple<char>>::value == false,
              "is_tuple_single_arg<std::tuple<int>, std::tuple<char>>::value should be false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // _v alias consistency
    static_assert(is_tuple_single_arg_v<>          == false,
                  "is_tuple_single_arg_v<> should be false");
    static_assert(is_tuple_single_arg_v<int>       == true,
                  "is_tuple_single_arg_v<int> should be true");
    static_assert(is_tuple_single_arg_v<int, char> == false,
                  "is_tuple_single_arg_v<int, char> should be false");
#endif


// =========================================================================
// III. IS_TUPLE (compile-time)
// =========================================================================

// positive cases -- the bare std::tuple<...> shape
static_assert(is_tuple<std::tuple<>>::value             == true,
              "is_tuple<std::tuple<>>::value should be true (empty tuple)");
static_assert(is_tuple<std::tuple<int>>::value          == true,
              "is_tuple<std::tuple<int>>::value should be true");
static_assert(is_tuple<std::tuple<int, char>>::value    == true,
              "is_tuple<std::tuple<int, char>>::value should be true");
static_assert(is_tuple<std::tuple<int, char, float>>::value == true,
              "is_tuple<std::tuple<int, char, float>>::value should be true");
static_assert(is_tuple<std::tuple<alpha, bravo>>::value == true,
              "is_tuple<std::tuple<alpha, bravo>>::value should be true");
static_assert(is_tuple<std::tuple<std::tuple<int>>>::value == true,
              "is_tuple<std::tuple<std::tuple<int>>>::value should be true (nested tuple is still a tuple)");

// negative cases -- non-tuple types
static_assert(is_tuple<int>::value      == false,
              "is_tuple<int>::value should be false");
static_assert(is_tuple<void>::value     == false,
              "is_tuple<void>::value should be false");
static_assert(is_tuple<alpha>::value    == false,
              "is_tuple<alpha>::value should be false");
static_assert(is_tuple<int*>::value     == false,
              "is_tuple<int*>::value should be false");
static_assert(is_tuple<std::pair<int, int>>::value == false,
              "is_tuple<std::pair<int, int>>::value should be false");

// the partial specialization matches the EXACT std::tuple<...> shape -- cv
// or ref qualifiers on the outer type fall through to the primary template
static_assert(is_tuple<const std::tuple<int>>::value     == false,
              "is_tuple<const std::tuple<int>>::value should be false (cv-qualified, outside the partial spec)");
static_assert(is_tuple<std::tuple<int>&>::value          == false,
              "is_tuple<std::tuple<int>&>::value should be false (lvalue ref)");
static_assert(is_tuple<std::tuple<int>&&>::value         == false,
              "is_tuple<std::tuple<int>&&>::value should be false (rvalue ref)");
static_assert(is_tuple<const volatile std::tuple<int>>::value == false,
              "is_tuple<const volatile std::tuple<int>>::value should be false");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_tuple_v<std::tuple<int>> == true,
                  "is_tuple_v<std::tuple<int>> should be true");
    static_assert(is_tuple_v<int>             == false,
                  "is_tuple_v<int> should be false");
    static_assert(is_tuple_v<std::tuple<>>    == true,
                  "is_tuple_v<std::tuple<>> should be true");
#endif


// =========================================================================
// IV.  IS_SINGLE_TUPLE_ARG (compile-time)
// =========================================================================

// empty pack -- not a single arg, hence false_type
static_assert(is_single_tuple_arg<>::value == false,
              "is_single_tuple_arg<> should be false (zero args)");

// single tuple arg -- positive case
static_assert(is_single_tuple_arg<std::tuple<>>::value          == true,
              "is_single_tuple_arg<std::tuple<>> should be true (empty tuple is still a tuple)");
static_assert(is_single_tuple_arg<std::tuple<int>>::value       == true,
              "is_single_tuple_arg<std::tuple<int>> should be true");
static_assert(is_single_tuple_arg<std::tuple<int, char>>::value == true,
              "is_single_tuple_arg<std::tuple<int, char>> should be true");
static_assert(is_single_tuple_arg<std::tuple<alpha, bravo>>::value == true,
              "is_single_tuple_arg<std::tuple<alpha, bravo>> should be true");

// single non-tuple arg -- the count is 1 but the type is not a tuple
static_assert(is_single_tuple_arg<int>::value   == false,
              "is_single_tuple_arg<int> should be false (single but not tuple)");
static_assert(is_single_tuple_arg<alpha>::value == false,
              "is_single_tuple_arg<alpha> should be false (single but not tuple)");
static_assert(is_single_tuple_arg<void>::value  == false,
              "is_single_tuple_arg<void> should be false (single but not tuple)");

// multiple args, regardless of whether any are tuples
static_assert(is_single_tuple_arg<std::tuple<int>, std::tuple<char>>::value == false,
              "is_single_tuple_arg<std::tuple<int>, std::tuple<char>> should be false (two args)");
static_assert(is_single_tuple_arg<std::tuple<int>, int>::value == false,
              "is_single_tuple_arg<std::tuple<int>, int> should be false (two args)");
static_assert(is_single_tuple_arg<int, std::tuple<int>>::value == false,
              "is_single_tuple_arg<int, std::tuple<int>> should be false (two args)");
static_assert(is_single_tuple_arg<int, char, float>::value == false,
              "is_single_tuple_arg<int, char, float> should be false (three args)");

// outer cv- / ref-qualified single arg -- mirrors is_tuple behavior, since
// is_single_tuple_arg defers to is_tuple on the lone element
static_assert(is_single_tuple_arg<const std::tuple<int>>::value == false,
              "is_single_tuple_arg<const std::tuple<int>> should be false (is_tuple sees cv-qualified)");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_single_tuple_arg_v<std::tuple<int>>      == true,
                  "is_single_tuple_arg_v<std::tuple<int>> should be true");
    static_assert(is_single_tuple_arg_v<int>                  == false,
                  "is_single_tuple_arg_v<int> should be false");
    static_assert(is_single_tuple_arg_v<int, std::tuple<int>> == false,
                  "is_single_tuple_arg_v<int, std::tuple<int>> should be false");
    static_assert(is_single_tuple_arg_v<>                     == false,
                  "is_single_tuple_arg_v<> should be false");
#endif


// =========================================================================
// V.   RUNTIME DRIVER
// =========================================================================
//   Every static_assert above already enforces the property at compile
// time; if this translation unit compiles, those checks have already
// succeeded.  The runtime calls below mirror them into the framework's
// sink so the suite report exposes them as named, passed assertions.

void
dtuple_tests_pack_all(
    test_handler& _test_handler
)
{
    // ---- first_arg ----
    record_assertion(_test_handler, 
        std::is_same<typename first_arg<int>::type, int>::value,
        "first_arg: single arg int");
    record_assertion(_test_handler, 
        std::is_same<typename first_arg<int, char>::type, int>::value,
        "first_arg: two args yields first");
    record_assertion(_test_handler, 
        std::is_same<typename first_arg<int, char, float, double>::type, int>::value,
        "first_arg: four args yields first");
    record_assertion(_test_handler, 
        std::is_same<typename first_arg<const int>::type, const int>::value,
        "first_arg: preserves const qualifier");
    record_assertion(_test_handler, 
        std::is_same<typename first_arg<int&>::type, int&>::value,
        "first_arg: preserves lvalue reference");
    record_assertion(_test_handler, 
        std::is_same<typename first_arg<int&&, char>::type, int&&>::value,
        "first_arg: preserves rvalue reference");
    record_assertion(_test_handler, 
        std::is_same<typename first_arg<std::tuple<int, char>, alpha>::type,
                     std::tuple<int, char>>::value,
        "first_arg: tuple as first element");
    record_assertion(_test_handler, 
        std::is_same<first_arg_t<int, char>,
                     typename first_arg<int, char>::type>::value,
        "first_arg_t: alias consistent with first_arg::type");

    // ---- is_tuple_single_arg ----
    record_assertion(_test_handler, 
        is_tuple_single_arg<>::value == false,
        "is_tuple_single_arg: empty pack -> false");
    record_assertion(_test_handler, 
        is_tuple_single_arg<int>::value == true,
        "is_tuple_single_arg: one arg -> true");
    record_assertion(_test_handler, 
        is_tuple_single_arg<std::tuple<>>::value == true,
        "is_tuple_single_arg: one tuple arg -> true");
    record_assertion(_test_handler, 
        is_tuple_single_arg<int, char>::value == false,
        "is_tuple_single_arg: two args -> false");
    record_assertion(_test_handler, 
        is_tuple_single_arg<int, char, float>::value == false,
        "is_tuple_single_arg: three args -> false");

    // ---- is_tuple ----
    record_assertion(_test_handler, 
        is_tuple<std::tuple<>>::value == true,
        "is_tuple: empty tuple -> true");
    record_assertion(_test_handler, 
        is_tuple<std::tuple<int>>::value == true,
        "is_tuple: single-element tuple -> true");
    record_assertion(_test_handler, 
        is_tuple<std::tuple<int, char, float>>::value == true,
        "is_tuple: multi-element tuple -> true");
    record_assertion(_test_handler, 
        is_tuple<std::tuple<std::tuple<int>>>::value == true,
        "is_tuple: nested tuple -> true");
    record_assertion(_test_handler, 
        is_tuple<int>::value == false,
        "is_tuple: int -> false");
    record_assertion(_test_handler, 
        is_tuple<void>::value == false,
        "is_tuple: void -> false");
    record_assertion(_test_handler, 
        is_tuple<std::pair<int, int>>::value == false,
        "is_tuple: std::pair -> false");
    record_assertion(_test_handler, 
        is_tuple<const std::tuple<int>>::value == false,
        "is_tuple: const-qualified tuple -> false (outside partial spec)");
    record_assertion(_test_handler, 
        is_tuple<std::tuple<int>&>::value == false,
        "is_tuple: tuple lvalue ref -> false (outside partial spec)");

    // ---- is_single_tuple_arg ----
    record_assertion(_test_handler, 
        is_single_tuple_arg<>::value == false,
        "is_single_tuple_arg: empty pack -> false");
    record_assertion(_test_handler, 
        is_single_tuple_arg<std::tuple<>>::value == true,
        "is_single_tuple_arg: single empty tuple -> true");
    record_assertion(_test_handler, 
        is_single_tuple_arg<std::tuple<int, char>>::value == true,
        "is_single_tuple_arg: single non-empty tuple -> true");
    record_assertion(_test_handler, 
        is_single_tuple_arg<int>::value == false,
        "is_single_tuple_arg: single non-tuple -> false");
    record_assertion(_test_handler, 
        is_single_tuple_arg<std::tuple<int>, std::tuple<char>>::value == false,
        "is_single_tuple_arg: two tuples -> false");
    record_assertion(_test_handler, 
        is_single_tuple_arg<std::tuple<int>, int>::value == false,
        "is_single_tuple_arg: tuple plus non-tuple -> false");
    record_assertion(_test_handler, 
        is_single_tuple_arg<int, std::tuple<int>>::value == false,
        "is_single_tuple_arg: non-tuple plus tuple -> false");
    record_assertion(_test_handler, 
        is_single_tuple_arg<const std::tuple<int>>::value == false,
        "is_single_tuple_arg: const-qualified tuple -> false");

    return;
}


NS_END  // testing
NS_END  // djinterp
