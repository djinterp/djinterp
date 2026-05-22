/******************************************************************************
* djinterp [test]                                 dtuple_tests_construction.cpp
*
*   Unit tests for the tuple-construction section of dtuple.hpp:
*     - to_tuple / to_tuple_t  (including the empty-pack specialization)
*     - make_tuple_of / make_tuple_of_t
*
*   Edge cases under scrutiny include:
*     - the empty-pack `to_tuple<>` specialization that fixes the original
*       eager-`std::conditional` instantiation hazard.  This test
*       explicitly instantiates `to_tuple<>::type` -- if the fix were
*       absent the file would not compile.
*     - the single-tuple-arg path that unwraps `to_tuple<std::tuple<...>>`
*       to the tuple itself (as opposed to wrapping it again).
*     - the cv-qualified-tuple single-arg path, where the inner type is a
*       `const std::tuple<...>` -- since `is_tuple` matches only the bare
*       shape, this falls through to the wrapping branch.
*     - make_tuple_of at sizes 0, 1, and >= 2 (each specialization is
*       distinct), and with a tuple as the element type (so the result
*       is a tuple-of-tuples).
*
*
* path:      /inc/djinterp/test/dtuple_tests_construction.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.19
******************************************************************************/

#include "./dtuple_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace dtuple_test_types;


// =========================================================================
// I.   TO_TUPLE  (compile-time)
// =========================================================================

// empty-pack specialization -- the regression target.
//   The very act of naming `to_tuple<>::type` below forces the
// specialization to be instantiated; if the empty-pack specialization
// were absent this translation unit would fail to compile.
static_assert(std::is_same<typename to_tuple<>::type, std::tuple<>>::value,
              "to_tuple<>::type should be std::tuple<>");
static_assert(std::is_same<to_tuple_t<>, std::tuple<>>::value,
              "to_tuple_t<> should be std::tuple<>");

// single non-tuple type -- the partial specialization wraps in std::tuple
static_assert(std::is_same<typename to_tuple<int>::type, std::tuple<int>>::value,
              "to_tuple<int>::type should be std::tuple<int>");
static_assert(std::is_same<typename to_tuple<void>::type, std::tuple<void>>::value,
              "to_tuple<void>::type should be std::tuple<void>");
static_assert(std::is_same<typename to_tuple<alpha>::type, std::tuple<alpha>>::value,
              "to_tuple<alpha>::type should be std::tuple<alpha>");
static_assert(std::is_same<typename to_tuple<int*>::type, std::tuple<int*>>::value,
              "to_tuple<int*>::type should preserve pointer qualifier");

// single tuple type -- the partial specialization unwraps to the tuple
// itself, NOT a tuple-of-tuple
static_assert(std::is_same<typename to_tuple<std::tuple<>>::type, std::tuple<>>::value,
              "to_tuple<std::tuple<>>::type should be std::tuple<>");
static_assert(std::is_same<typename to_tuple<std::tuple<int>>::type, std::tuple<int>>::value,
              "to_tuple<std::tuple<int>>::type should be std::tuple<int>");
static_assert(std::is_same<typename to_tuple<std::tuple<int, char>>::type,
                           std::tuple<int, char>>::value,
              "to_tuple<std::tuple<int, char>>::type should be std::tuple<int, char>");
static_assert(std::is_same<typename to_tuple<std::tuple<alpha, bravo, charlie>>::type,
                           std::tuple<alpha, bravo, charlie>>::value,
              "to_tuple<std::tuple<alpha, bravo, charlie>>::type should be std::tuple<alpha, bravo, charlie>");

// cv-/ref-qualified single tuple -- is_tuple sees the bare shape only,
// so the qualified-tuple single-arg goes through the WRAPPING branch
static_assert(std::is_same<typename to_tuple<const std::tuple<int>>::type,
                           std::tuple<const std::tuple<int>>>::value,
              "to_tuple<const std::tuple<int>>::type should wrap (is_tuple sees bare shape only)");

// multiple non-tuple types -- general template, just builds the tuple
static_assert(std::is_same<typename to_tuple<int, char>::type,
                           std::tuple<int, char>>::value,
              "to_tuple<int, char>::type should be std::tuple<int, char>");
static_assert(std::is_same<typename to_tuple<int, char, float, double>::type,
                           std::tuple<int, char, float, double>>::value,
              "to_tuple<int, char, float, double>::type should be std::tuple<int, char, float, double>");
static_assert(std::is_same<typename to_tuple<alpha, bravo, charlie, delta>::type,
                           std::tuple<alpha, bravo, charlie, delta>>::value,
              "to_tuple<alpha, bravo, charlie, delta>::type should be the matching std::tuple");

// multiple args including tuples -- nested tuples remain nested
static_assert(std::is_same<typename to_tuple<std::tuple<int>, char>::type,
                           std::tuple<std::tuple<int>, char>>::value,
              "to_tuple<std::tuple<int>, char>::type should keep nested tuple as one element");
static_assert(std::is_same<typename to_tuple<int, std::tuple<char>>::type,
                           std::tuple<int, std::tuple<char>>>::value,
              "to_tuple<int, std::tuple<char>>::type should keep nested tuple as one element");
static_assert(std::is_same<typename to_tuple<std::tuple<int>, std::tuple<char>>::type,
                           std::tuple<std::tuple<int>, std::tuple<char>>>::value,
              "to_tuple<std::tuple<int>, std::tuple<char>>::type should NOT unwrap (two args)");

// to_tuple_t alias consistency on the principal cases
static_assert(std::is_same<to_tuple_t<int>, std::tuple<int>>::value,
              "to_tuple_t<int> should be std::tuple<int>");
static_assert(std::is_same<to_tuple_t<int, char>, std::tuple<int, char>>::value,
              "to_tuple_t<int, char> should be std::tuple<int, char>");
static_assert(std::is_same<to_tuple_t<std::tuple<int, char>>, std::tuple<int, char>>::value,
              "to_tuple_t<std::tuple<int, char>> should unwrap to std::tuple<int, char>");


// =========================================================================
// II.  MAKE_TUPLE_OF  (compile-time)
// =========================================================================

// count == 0 -- explicit specialization yielding std::tuple<>
static_assert(std::is_same<typename make_tuple_of<int,  0>::type, std::tuple<>>::value,
              "make_tuple_of<int,  0>::type should be std::tuple<>");
static_assert(std::is_same<typename make_tuple_of<alpha, 0>::type, std::tuple<>>::value,
              "make_tuple_of<alpha, 0>::type should be std::tuple<>");
static_assert(std::is_same<typename make_tuple_of<std::tuple<int>, 0>::type,
                           std::tuple<>>::value,
              "make_tuple_of<std::tuple<int>, 0>::type should be std::tuple<>");
static_assert(std::is_same<typename make_tuple_of<void, 0>::type, std::tuple<>>::value,
              "make_tuple_of<void, 0>::type should be std::tuple<>");

// count == 1 -- explicit specialization yielding to_tuple_t<_Type>;
// for non-tuple types this is std::tuple<_Type>; for tuple types it
// is the tuple itself (since to_tuple unwraps).
static_assert(std::is_same<typename make_tuple_of<int,  1>::type, std::tuple<int>>::value,
              "make_tuple_of<int, 1>::type should be std::tuple<int>");
static_assert(std::is_same<typename make_tuple_of<alpha, 1>::type, std::tuple<alpha>>::value,
              "make_tuple_of<alpha, 1>::type should be std::tuple<alpha>");
static_assert(std::is_same<typename make_tuple_of<std::tuple<int, char>, 1>::type,
                           std::tuple<int, char>>::value,
              "make_tuple_of<std::tuple<int, char>, 1>::type should unwrap to std::tuple<int, char> (to_tuple semantics)");

// count >= 2 -- general template delegating to repeat_t
static_assert(std::is_same<typename make_tuple_of<int, 2>::type,
                           std::tuple<int, int>>::value,
              "make_tuple_of<int, 2>::type should be std::tuple<int, int>");
static_assert(std::is_same<typename make_tuple_of<int, 3>::type,
                           std::tuple<int, int, int>>::value,
              "make_tuple_of<int, 3>::type should be std::tuple<int, int, int>");
static_assert(std::is_same<typename make_tuple_of<int, 5>::type,
                           std::tuple<int, int, int, int, int>>::value,
              "make_tuple_of<int, 5>::type should be std::tuple<int * 5>");
static_assert(std::is_same<typename make_tuple_of<char, 4>::type,
                           std::tuple<char, char, char, char>>::value,
              "make_tuple_of<char, 4>::type should be std::tuple<char * 4>");
static_assert(std::is_same<typename make_tuple_of<alpha, 3>::type,
                           std::tuple<alpha, alpha, alpha>>::value,
              "make_tuple_of<alpha, 3>::type should be std::tuple<alpha * 3>");

// make_tuple_of with a tuple element type -- general path keeps each
// row as the SAME nested tuple type (no unwrapping at this layer)
static_assert(std::is_same<typename make_tuple_of<std::tuple<int>, 2>::type,
                           std::tuple<std::tuple<int>, std::tuple<int>>>::value,
              "make_tuple_of<std::tuple<int>, 2>::type should keep nested tuples");
static_assert(std::is_same<typename make_tuple_of<std::tuple<int, char>, 3>::type,
                           std::tuple<std::tuple<int, char>,
                                      std::tuple<int, char>,
                                      std::tuple<int, char>>>::value,
              "make_tuple_of<std::tuple<int, char>, 3>::type should keep nested tuples");

// alias consistency: make_tuple_of_t mirrors the trait's ::type
static_assert(std::is_same<make_tuple_of_t<int, 0>,
                           typename make_tuple_of<int, 0>::type>::value,
              "make_tuple_of_t<int, 0> should alias make_tuple_of<int, 0>::type");
static_assert(std::is_same<make_tuple_of_t<int, 1>,
                           typename make_tuple_of<int, 1>::type>::value,
              "make_tuple_of_t<int, 1> should alias make_tuple_of<int, 1>::type");
static_assert(std::is_same<make_tuple_of_t<int, 4>,
                           typename make_tuple_of<int, 4>::type>::value,
              "make_tuple_of_t<int, 4> should alias make_tuple_of<int, 4>::type");


// =========================================================================
// III. RUNTIME DRIVER
// =========================================================================

void
dtuple_tests_construction_all(
    test_handler& _test_handler
)
{
    // ---- to_tuple ----
    record_assertion(_test_handler, 
        std::is_same<typename to_tuple<>::type, std::tuple<>>::value,
        "to_tuple: empty pack -> std::tuple<>");
    record_assertion(_test_handler, 
        std::is_same<typename to_tuple<int>::type, std::tuple<int>>::value,
        "to_tuple: single non-tuple wraps");
    record_assertion(_test_handler, 
        std::is_same<typename to_tuple<std::tuple<int, char>>::type,
                     std::tuple<int, char>>::value,
        "to_tuple: single tuple unwraps");
    record_assertion(_test_handler, 
        std::is_same<typename to_tuple<std::tuple<>>::type, std::tuple<>>::value,
        "to_tuple: single empty tuple stays empty");
    record_assertion(_test_handler, 
        std::is_same<typename to_tuple<int, char>::type, std::tuple<int, char>>::value,
        "to_tuple: two args build new tuple");
    record_assertion(_test_handler, 
        std::is_same<typename to_tuple<std::tuple<int>, char>::type,
                     std::tuple<std::tuple<int>, char>>::value,
        "to_tuple: two args with nested tuple stays nested");
    record_assertion(_test_handler, 
        std::is_same<typename to_tuple<const std::tuple<int>>::type,
                     std::tuple<const std::tuple<int>>>::value,
        "to_tuple: cv-qualified tuple single arg wraps");
    record_assertion(_test_handler, 
        std::is_same<to_tuple_t<int, char, float>,
                     std::tuple<int, char, float>>::value,
        "to_tuple_t: alias yields the same tuple");

    // ---- make_tuple_of ----
    record_assertion(_test_handler, 
        std::is_same<typename make_tuple_of<int, 0>::type, std::tuple<>>::value,
        "make_tuple_of: count 0 -> empty tuple");
    record_assertion(_test_handler, 
        std::is_same<typename make_tuple_of<int, 1>::type, std::tuple<int>>::value,
        "make_tuple_of: count 1 -> single-element tuple");
    record_assertion(_test_handler, 
        std::is_same<typename make_tuple_of<int, 2>::type, std::tuple<int, int>>::value,
        "make_tuple_of: count 2 -> two-element tuple");
    record_assertion(_test_handler, 
        std::is_same<typename make_tuple_of<int, 5>::type,
                     std::tuple<int, int, int, int, int>>::value,
        "make_tuple_of: count 5 -> five-element tuple");
    record_assertion(_test_handler, 
        std::is_same<typename make_tuple_of<std::tuple<int, char>, 1>::type,
                     std::tuple<int, char>>::value,
        "make_tuple_of: count 1 with tuple unwraps via to_tuple");
    record_assertion(_test_handler, 
        std::is_same<typename make_tuple_of<std::tuple<int>, 2>::type,
                     std::tuple<std::tuple<int>, std::tuple<int>>>::value,
        "make_tuple_of: count 2 with tuple keeps nesting");
    record_assertion(_test_handler, 
        std::is_same<make_tuple_of_t<char, 4>, std::tuple<char, char, char, char>>::value,
        "make_tuple_of_t: alias yields the same tuple");

    return;
}


NS_END  // testing
NS_END  // djinterp
