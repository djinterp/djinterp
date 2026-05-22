/******************************************************************************
* djinterp [test]                                    dtuple_tests_modifiers.cpp
*
*   Unit tests for the type-modifier section of dtuple.hpp:
*     - wrap_all (and the empty / general / explicit-empty
*       specializations) plus wrap_all_t
*     - to_lvalue_reference
*     - to_rvalue_reference
*     - to_pointer
*     - to_type / to_type_t
*     - tuple_join (and its three-way join_helper specializations:
*       empty tail, tuple head, single-type head)
*
*   Notable edge cases:
*     - wrap_all<> with the empty pack must yield identity (matches the
*       explicit empty specialization, NOT a re-instantiation hazard).
*     - to_lvalue_reference / to_rvalue_reference both strip ANY existing
*       reference before re-adding their own.  An incoming rvalue should
*       end up as the new flavor, not stuck at the original.
*     - to_pointer normalizes to EXACTLY one pointer level (strip then
*       re-add).  Calling it on a double-pointer should leave a double-
*       pointer alone, NOT a triple.  Verified below.
*     - tuple_join treats a `std::tuple<>` element by flattening it to
*       no contribution (empty inside the merged pack), and treats a
*       non-tuple element by contributing exactly one type.
*
*
* path:      /inc/djinterp/test/dtuple_tests_modifiers.cpp
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.19
******************************************************************************/

#include "./dtuple_tests.hpp"


NS_DJINTERP
NS_TESTING

using namespace dtuple_test_types;


// =========================================================================
// I.   WRAP_ALL  (compile-time)
// =========================================================================

// empty modifier pack -- both the general empty-pack instantiation and
// the explicit `<>` specialization should resolve to identity.
static_assert(std::is_same<typename wrap_all<>::template type<int>, int>::value,
              "wrap_all<>::type<int> should be int (identity)");
static_assert(std::is_same<typename wrap_all<>::template type<alpha>, alpha>::value,
              "wrap_all<>::type<alpha> should be alpha");
static_assert(std::is_same<typename wrap_all<>::template type<std::tuple<int>>,
                           std::tuple<int>>::value,
              "wrap_all<>::type<std::tuple<int>> should be std::tuple<int>");

// single modifier -- one application
static_assert(std::is_same<typename wrap_all<std::add_const>::template type<int>,
                           const int>::value,
              "wrap_all<std::add_const>::type<int> should be const int");
static_assert(std::is_same<typename wrap_all<std::add_pointer>::template type<int>,
                           int*>::value,
              "wrap_all<std::add_pointer>::type<int> should be int*");
static_assert(std::is_same<typename wrap_all<std::add_volatile>::template type<int>,
                           volatile int>::value,
              "wrap_all<std::add_volatile>::type<int> should be volatile int");

// two modifiers -- composition is RIGHT-to-LEFT (rightmost is innermost)
static_assert(std::is_same<typename wrap_all<std::add_pointer,
                                             std::add_const>::template type<int>,
                           const int*>::value,
              "wrap_all<add_pointer, add_const>::type<int> should be const int* (const applied first, then pointer)");
static_assert(std::is_same<typename wrap_all<std::add_const,
                                             std::add_pointer>::template type<int>,
                           int* const>::value,
              "wrap_all<add_const, add_pointer>::type<int> should be int* const (pointer first, then const)");
static_assert(std::is_same<typename wrap_all<std::add_pointer,
                                             std::add_pointer>::template type<int>,
                           int**>::value,
              "wrap_all<add_pointer, add_pointer>::type<int> should be int** (pointer twice)");
static_assert(std::is_same<typename wrap_all<std::add_lvalue_reference,
                                             std::add_const>::template type<int>,
                           const int&>::value,
              "wrap_all<add_lvalue_reference, add_const>::type<int> should be const int&");

// three modifiers -- composition continues right-to-left
static_assert(std::is_same<typename wrap_all<std::add_lvalue_reference,
                                             std::add_pointer,
                                             std::add_const>::template type<int>,
                           const int*&>::value,
              "wrap_all<add_lvalue_reference, add_pointer, add_const>::type<int> should be const int*&");

// wrap_all_t alias
static_assert(std::is_same<wrap_all_t<int>, int>::value,
              "wrap_all_t<int> (no modifiers) should be int");
static_assert(std::is_same<wrap_all_t<int, std::add_const>, const int>::value,
              "wrap_all_t<int, std::add_const> should be const int");
static_assert(std::is_same<wrap_all_t<int, std::add_pointer, std::add_const>,
                           const int*>::value,
              "wrap_all_t<int, add_pointer, add_const> should be const int*");


// =========================================================================
// II.  TO_LVALUE_REFERENCE  (compile-time)
// =========================================================================

// plain type -- becomes an lvalue ref
static_assert(std::is_same<typename to_lvalue_reference::template type<int>, int&>::value,
              "to_lvalue_reference::type<int> should be int&");
static_assert(std::is_same<typename to_lvalue_reference::template type<alpha>, alpha&>::value,
              "to_lvalue_reference::type<alpha> should be alpha&");

// already an lvalue ref -- stays an lvalue ref
static_assert(std::is_same<typename to_lvalue_reference::template type<int&>, int&>::value,
              "to_lvalue_reference::type<int&> should be int&");

// rvalue ref -- stripped and re-added as lvalue
static_assert(std::is_same<typename to_lvalue_reference::template type<int&&>, int&>::value,
              "to_lvalue_reference::type<int&&> should be int& (strip rvalue, add lvalue)");

// cv qualifiers preserved
static_assert(std::is_same<typename to_lvalue_reference::template type<const int>, const int&>::value,
              "to_lvalue_reference::type<const int> should be const int&");
static_assert(std::is_same<typename to_lvalue_reference::template type<volatile int>, volatile int&>::value,
              "to_lvalue_reference::type<volatile int> should be volatile int&");

// pointer types -- the pointer survives, an ref is added
static_assert(std::is_same<typename to_lvalue_reference::template type<int*>, int*&>::value,
              "to_lvalue_reference::type<int*> should be int*&");


// =========================================================================
// III. TO_RVALUE_REFERENCE  (compile-time)
// =========================================================================

// plain type -- becomes an rvalue ref
static_assert(std::is_same<typename to_rvalue_reference::template type<int>, int&&>::value,
              "to_rvalue_reference::type<int> should be int&&");

// lvalue ref -- stripped and re-added as rvalue
static_assert(std::is_same<typename to_rvalue_reference::template type<int&>, int&&>::value,
              "to_rvalue_reference::type<int&> should be int&& (strip lvalue, add rvalue)");

// already an rvalue ref -- stays
static_assert(std::is_same<typename to_rvalue_reference::template type<int&&>, int&&>::value,
              "to_rvalue_reference::type<int&&> should be int&&");

// cv preserved
static_assert(std::is_same<typename to_rvalue_reference::template type<const int>, const int&&>::value,
              "to_rvalue_reference::type<const int> should be const int&&");


// =========================================================================
// IV.  TO_POINTER  (compile-time)
// =========================================================================

// plain type -- gets one pointer level
static_assert(std::is_same<typename to_pointer::template type<int>, int*>::value,
              "to_pointer::type<int> should be int*");
static_assert(std::is_same<typename to_pointer::template type<alpha>, alpha*>::value,
              "to_pointer::type<alpha> should be alpha*");

// already a pointer -- strip then re-add -> same single-pointer
static_assert(std::is_same<typename to_pointer::template type<int*>, int*>::value,
              "to_pointer::type<int*> should be int* (normalize, not stack)");

// double pointer -- strip one level then re-add -> double pointer
static_assert(std::is_same<typename to_pointer::template type<int**>, int**>::value,
              "to_pointer::type<int**> should be int** (one strip, one add)");

// cv-qualified type -- the cv survives (remove_pointer is a no-op,
// then add_pointer wraps)
static_assert(std::is_same<typename to_pointer::template type<const int>, const int*>::value,
              "to_pointer::type<const int> should be const int*");

// pointer-to-cv -- strip strips the cv pointer, add re-adds without cv
// (the cv was on the POINTER, not the pointee, so remove_pointer drops
// the pointer entirely; add_pointer makes a fresh non-cv pointer to int)
static_assert(std::is_same<typename to_pointer::template type<int* const>, int*>::value,
              "to_pointer::type<int* const> should be int* (cv was on pointer, dropped by remove)");


// =========================================================================
// V.   TO_TYPE  (compile-time)
// =========================================================================

// identity wrapper -- the type is exposed verbatim
static_assert(std::is_same<typename to_type<int>::type, int>::value,
              "to_type<int>::type should be int");
static_assert(std::is_same<typename to_type<const int&>::type, const int&>::value,
              "to_type<const int&>::type should preserve cv and ref");
static_assert(std::is_same<typename to_type<std::tuple<int, char>>::type,
                           std::tuple<int, char>>::value,
              "to_type<std::tuple<int, char>>::type should be std::tuple<int, char>");
static_assert(std::is_same<typename to_type<void>::type, void>::value,
              "to_type<void>::type should be void");

// alias consistency
static_assert(std::is_same<to_type_t<int>, int>::value,
              "to_type_t<int> should be int");
static_assert(std::is_same<to_type_t<volatile int*>, volatile int*>::value,
              "to_type_t<volatile int*> should be volatile int*");


// =========================================================================
// VI.  TUPLE_JOIN  (compile-time)
// =========================================================================

// empty pack of tuples -- the helper's empty-tail base case
static_assert(std::is_same<typename tuple_join<>::type, std::tuple<>>::value,
              "tuple_join<>::type should be std::tuple<>");

// single non-tuple arg -- the helper's single-type-head specialization
static_assert(std::is_same<typename tuple_join<int>::type, std::tuple<int>>::value,
              "tuple_join<int>::type should be std::tuple<int>");
static_assert(std::is_same<typename tuple_join<alpha>::type, std::tuple<alpha>>::value,
              "tuple_join<alpha>::type should be std::tuple<alpha>");

// single tuple arg -- the helper's tuple-head specialization
static_assert(std::is_same<typename tuple_join<std::tuple<>>::type, std::tuple<>>::value,
              "tuple_join<std::tuple<>>::type should be std::tuple<>");
static_assert(std::is_same<typename tuple_join<std::tuple<int>>::type, std::tuple<int>>::value,
              "tuple_join<std::tuple<int>>::type should be std::tuple<int>");
static_assert(std::is_same<typename tuple_join<std::tuple<int, char>>::type,
                           std::tuple<int, char>>::value,
              "tuple_join<std::tuple<int, char>>::type should be std::tuple<int, char>");

// multiple non-tuple args
static_assert(std::is_same<typename tuple_join<int, char>::type,
                           std::tuple<int, char>>::value,
              "tuple_join<int, char>::type should be std::tuple<int, char>");
static_assert(std::is_same<typename tuple_join<int, char, float>::type,
                           std::tuple<int, char, float>>::value,
              "tuple_join<int, char, float>::type should be std::tuple<int, char, float>");

// two tuples -- concatenated
static_assert(std::is_same<typename tuple_join<std::tuple<int>, std::tuple<char>>::type,
                           std::tuple<int, char>>::value,
              "tuple_join<std::tuple<int>, std::tuple<char>>::type should be std::tuple<int, char>");
static_assert(std::is_same<typename tuple_join<std::tuple<int, char>,
                                                std::tuple<float, double>>::type,
                           std::tuple<int, char, float, double>>::value,
              "tuple_join<std::tuple<int, char>, std::tuple<float, double>>::type should concat");

// three tuples
static_assert(std::is_same<typename tuple_join<std::tuple<int>,
                                                std::tuple<char>,
                                                std::tuple<float>>::type,
                           std::tuple<int, char, float>>::value,
              "tuple_join<std::tuple<int>, std::tuple<char>, std::tuple<float>>::type should concat all three");

// mixed tuple / non-tuple args -- non-tuples contribute one element
static_assert(std::is_same<typename tuple_join<std::tuple<int>, char>::type,
                           std::tuple<int, char>>::value,
              "tuple_join<std::tuple<int>, char>::type should append the non-tuple");
static_assert(std::is_same<typename tuple_join<int, std::tuple<char>>::type,
                           std::tuple<int, char>>::value,
              "tuple_join<int, std::tuple<char>>::type should prepend the non-tuple");
static_assert(std::is_same<typename tuple_join<int, std::tuple<char>, float>::type,
                           std::tuple<int, char, float>>::value,
              "tuple_join<int, std::tuple<char>, float>::type should preserve interleaving");

// empty tuples in the mix contribute nothing
static_assert(std::is_same<typename tuple_join<std::tuple<>, std::tuple<int>>::type,
                           std::tuple<int>>::value,
              "tuple_join<std::tuple<>, std::tuple<int>>::type should drop the empty tuple");
static_assert(std::is_same<typename tuple_join<std::tuple<int>, std::tuple<>>::type,
                           std::tuple<int>>::value,
              "tuple_join<std::tuple<int>, std::tuple<>>::type should drop the empty tuple");
static_assert(std::is_same<typename tuple_join<std::tuple<>, std::tuple<>>::type,
                           std::tuple<>>::value,
              "tuple_join<std::tuple<>, std::tuple<>>::type should be empty");

// deeply-nested tuples are NOT flattened recursively -- join goes one
// level deep only
static_assert(std::is_same<typename tuple_join<std::tuple<std::tuple<int>>>::type,
                           std::tuple<std::tuple<int>>>::value,
              "tuple_join<std::tuple<std::tuple<int>>>::type should keep inner nesting");


// =========================================================================
// VII. RUNTIME DRIVER
// =========================================================================

void
dtuple_tests_modifiers_all(
    test_handler& _test_handler
)
{
    // ---- wrap_all ----
    record_assertion(_test_handler, 
        std::is_same<typename wrap_all<>::template type<int>, int>::value,
        "wrap_all: empty pack -> identity");
    record_assertion(_test_handler, 
        std::is_same<typename wrap_all<std::add_const>::template type<int>,
                     const int>::value,
        "wrap_all: single add_const");
    record_assertion(_test_handler, 
        std::is_same<typename wrap_all<std::add_pointer, std::add_const>::template type<int>,
                     const int*>::value,
        "wrap_all: add_pointer/add_const composes right-to-left");
    record_assertion(_test_handler, 
        std::is_same<typename wrap_all<std::add_pointer, std::add_pointer>::template type<int>,
                     int**>::value,
        "wrap_all: add_pointer twice -> int**");
    record_assertion(_test_handler, 
        std::is_same<wrap_all_t<int, std::add_const>, const int>::value,
        "wrap_all_t: alias consistent");

    // ---- to_lvalue_reference ----
    record_assertion(_test_handler, 
        std::is_same<typename to_lvalue_reference::template type<int>, int&>::value,
        "to_lvalue_reference: int -> int&");
    record_assertion(_test_handler, 
        std::is_same<typename to_lvalue_reference::template type<int&&>, int&>::value,
        "to_lvalue_reference: rvalue -> lvalue");
    record_assertion(_test_handler, 
        std::is_same<typename to_lvalue_reference::template type<const int>, const int&>::value,
        "to_lvalue_reference: preserves const");

    // ---- to_rvalue_reference ----
    record_assertion(_test_handler, 
        std::is_same<typename to_rvalue_reference::template type<int>, int&&>::value,
        "to_rvalue_reference: int -> int&&");
    record_assertion(_test_handler, 
        std::is_same<typename to_rvalue_reference::template type<int&>, int&&>::value,
        "to_rvalue_reference: lvalue -> rvalue");
    record_assertion(_test_handler, 
        std::is_same<typename to_rvalue_reference::template type<int&&>, int&&>::value,
        "to_rvalue_reference: rvalue stays rvalue");

    // ---- to_pointer ----
    record_assertion(_test_handler, 
        std::is_same<typename to_pointer::template type<int>, int*>::value,
        "to_pointer: int -> int*");
    record_assertion(_test_handler, 
        std::is_same<typename to_pointer::template type<int*>, int*>::value,
        "to_pointer: int* normalized");
    record_assertion(_test_handler, 
        std::is_same<typename to_pointer::template type<int**>, int**>::value,
        "to_pointer: int** normalized to int**");

    // ---- to_type ----
    record_assertion(_test_handler, 
        std::is_same<typename to_type<int>::type, int>::value,
        "to_type: int -> int");
    record_assertion(_test_handler, 
        std::is_same<typename to_type<const int&>::type, const int&>::value,
        "to_type: preserves cv + ref");
    record_assertion(_test_handler, 
        std::is_same<to_type_t<volatile int*>, volatile int*>::value,
        "to_type_t: alias preserves cv + pointer");

    // ---- tuple_join ----
    record_assertion(_test_handler, 
        std::is_same<typename tuple_join<>::type, std::tuple<>>::value,
        "tuple_join: empty pack -> std::tuple<>");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_join<int>::type, std::tuple<int>>::value,
        "tuple_join: single non-tuple wraps");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_join<std::tuple<int>>::type, std::tuple<int>>::value,
        "tuple_join: single tuple unwrapped to itself");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_join<std::tuple<int>, std::tuple<char>>::type,
                     std::tuple<int, char>>::value,
        "tuple_join: two tuples concat");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_join<std::tuple<int>, char>::type,
                     std::tuple<int, char>>::value,
        "tuple_join: tuple + non-tuple appends");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_join<int, std::tuple<char>>::type,
                     std::tuple<int, char>>::value,
        "tuple_join: non-tuple + tuple prepends");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_join<std::tuple<>, std::tuple<int>>::type,
                     std::tuple<int>>::value,
        "tuple_join: empty tuple contributes nothing");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_join<std::tuple<int>, std::tuple<char>,
                                          std::tuple<float>>::type,
                     std::tuple<int, char, float>>::value,
        "tuple_join: three tuples concat");
    record_assertion(_test_handler, 
        std::is_same<typename tuple_join<std::tuple<std::tuple<int>>>::type,
                     std::tuple<std::tuple<int>>>::value,
        "tuple_join: nested tuple is NOT flattened recursively");

    return;
}


NS_END  // testing
NS_END  // djinterp
