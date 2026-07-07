/******************************************************************************
* djinterp [functional]                                      compose_tests.hpp
*
*   DTest declarations and shared helper types for the compose.hpp unit
* test suite.  Each like-group semantic section of compose.hpp maps to a
* single `void(test_handler&)` section function declared here and defined
* in its own .cpp translation unit:
*
*     section in compose.hpp            test .cpp file
*     -------------------------------   ------------------------------
*     0.  predicate traits / concepts   compose_tests_traits.cpp
*     II. compose / pipe / transformer  compose_tests_composition.cpp
*     III.variadic compose_all/pipe_all compose_tests_variadic.cpp
*     IV. partial_back                  compose_tests_partial.cpp
*     V.  tap                           compose_tests_tap.cpp
*     VI. memoize                       compose_tests_memoize.cpp
*     VII.fix                           compose_tests_fix.cpp
*
*   The compile-time trait/concept checks (section 0) live almost
* entirely in static_asserts within compose_tests_traits.cpp; the
* section function there records one runtime roll-up assertion so the
* report shows a row for the compile-time suite.
*
*   The shared helper callables below are intentionally small, header-
* only, and free of state so every section can include them without ODR
* trouble (all are types or `inline` functions).
*
*   PORTABILITY:
*   C++11 minimum.  compose.hpp's C++11+ primary path is assumed; the
* C++98 fallback is exercised separately by its own (out-of-scope here)
* suite because variadic / fix / traits are RED in C++98.
*
* path:      /tests/djinterp/core/functional/compose_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.31
******************************************************************************/

#ifndef DJINTERP_TESTING_FUNCTIONAL_COMPOSE_
#define DJINTERP_TESTING_FUNCTIONAL_COMPOSE_ 1

// std
#include <cstddef>
#include <string>
// djinterp
#include "../../core/djinterp.hpp"
#include "../../test/test_common.hpp"
#include "../../test/test_handler.hpp"
#include "../../test/test_defaults.hpp"
#include "../../test/test_runner.hpp"
#include "../compose.hpp"


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                I.   SHARED HELPER CALLABLES                              ///
///////////////////////////////////////////////////////////////////////////////
//   Small stateless function objects and free functions used across the
// section translation units.  Kept header-only: every entity is either a
// type or an `inline` function, so multiple .cpp files may include this
// header without violating the ODR.

// add_one
//   struct: unary transformer adding one to an int.
struct add_one
{
    D_CONSTEXPR int operator()(int _x) const
    {
        return _x + 1;
    }
};

// doubler
//   struct: unary transformer doubling an int.
struct doubler
{
    D_CONSTEXPR int operator()(int _x) const
    {
        return _x * 2;
    }
};

// negate_int
//   struct: unary transformer negating an int.
struct negate_int
{
    D_CONSTEXPR int operator()(int _x) const
    {
        return -_x;
    }
};

// to_string_fn
//   struct: unary transformer mapping an int to its decimal string.
struct to_string_fn
{
    std::string operator()(int _x) const
    {
        return std::to_string(_x);
    }
};

// length_of
//   struct: unary transformer yielding the size of a string.
struct length_of
{
    std::size_t operator()(const std::string& _s) const
    {
        return _s.size();
    }
};

// not_callable
//   struct: a plain aggregate with no call operator, used as a
// negative case for invocability detection.
struct not_callable
{
    int m_value;
};

// sink_void
//   struct: a unary callable returning void, used as a negative case
// for is_unary_transformer (which requires a non-void result).
struct sink_void
{
    void operator()(int) const
    {
        return;
    }
};

// subtract_two
//   struct: binary transformer computing _a - _b, used by the
// partial-application section.
struct subtract_two
{
    D_CONSTEXPR int operator()(int _a, int _b) const
    {
        return _a - _b;
    }
};

// subtract_three
//   struct: ternary transformer computing _a - _b - _c, used by the
// partial-application section.
struct subtract_three
{
    D_CONSTEXPR int operator()(int _a, int _b, int _c) const
    {
        return _a - _b - _c;
    }
};

// free_add_one
//   function: free-function unary transformer over const int&; the
// const-reference parameter matches memoize's keyed call shape.
inline int
free_add_one(
    const int& _x
)
{
    return _x + 1;
}


///////////////////////////////////////////////////////////////////////////////
///                II.  SECTION FUNCTION DECLARATIONS                        ///
///////////////////////////////////////////////////////////////////////////////
//   One section function per like-group semantic section of compose.hpp.
// Each is defined in its own .cpp and matches the framework's section
// signature `void(test::test_handler&)`.

void compose_tests_traits(::djinterp::test::test_handler& _handler);
void compose_tests_composition(::djinterp::test::test_handler& _handler);
void compose_tests_variadic(::djinterp::test::test_handler& _handler);
void compose_tests_partial(::djinterp::test::test_handler& _handler);
void compose_tests_tap(::djinterp::test::test_handler& _handler);
void compose_tests_memoize(::djinterp::test::test_handler& _handler);
void compose_tests_fix(::djinterp::test::test_handler& _handler);


///////////////////////////////////////////////////////////////////////////////
///                III. MODULE WIRING                                        ///
///////////////////////////////////////////////////////////////////////////////
//   The module identity constant and the run_all entry point that
// schedules every section in document order.  Defined in
// compose_tests_runner.cpp.

// compose_module_info
//   constant: per-module identity bound at the in-output banner site.
extern const ::djinterp::test::test_module_info compose_module_info;

// compose_module_run_all
//   function: schedules every compose.hpp test section against the
// runner engine in document order.
void compose_module_run_all(::djinterp::test::test_runner_ctx& _ctx);


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_FUNCTIONAL_COMPOSE_
