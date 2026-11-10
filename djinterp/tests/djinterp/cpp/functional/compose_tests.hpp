/*******************************************************************************
* djinterp [functional]                                        compose_tests.hpp
*
*   DTest declarations and shared helper types for the compose.hpp unit
* test suite.  Each like-group semantic section of compose.hpp maps to a
* single `bool()` section function declared here and defined
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
*
* path:      /tests/djinterp/cpp/functional/compose_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.31
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTING_FUNCTIONAL_COMPOSE_
#define DJINTERP_TESTING_FUNCTIONAL_COMPOSE_ 1

// std
#include <cstdio>
#include <cstddef>
#include <string>
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/functional/compose.hpp"
#include "../../../../inc/djinterp/cpp/modules/test/test_traits.hpp"  // D_TEST_STATIC
#endif
#ifdef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec, run_module
#endif


// the name ::djinterp::test exists in both faces for the dt alias below
// (test_defaults.hpp declares it in the spec face; this reopens it)
namespace djinterp { namespace test {} }

NS_DJINTERP
NS_TESTING

// dt names DTest; unconditional, the spec provider needs it
namespace dt = ::djinterp::test;

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only

// record_assertion
//   function: one check of a converted body: on failure it prints what was
// expected and clears the body's verdict.  (The bodies used DTest's retired
// handler API, record_assertion(handler, condition, what).)
inline
void record_assertion(
    bool&       _ok,
    bool        _condition,
    const char* _what
)
{
    if (!_condition)
    {
        std::printf("    ! %s\n", _what);
        _ok = false;
    }

    return;
}

// record_assertion (with detail)
//   function: the five-argument form the bodies also used: the detail and
// the extra text are printed with a failure.
inline
void record_assertion(
    bool&       _ok,
    bool        _condition,
    const char* _what,
    const char* _detail,
    const char* _extra = nullptr
)
{
    if (!_condition)
    {
        std::printf("    ! %s (%s%s%s)\n",
                    _what,
                    _detail ? _detail : "",
                    _extra ? "; " : "",
                    _extra ? _extra : "");
        _ok = false;
    }

    return;
}


// DUnitTestTally
//   struct: kept for the converted bodies' run_unit_test calls; a body's
// verdict is its bool now, so the tally holds nothing.
struct DUnitTestTally
{
};

// run_unit_test
//   function: runs one named unit inside a converted body, and prints its
// name when it was the unit that first failed.
template<typename Fn>
void run_unit_test(
    bool&           _ok,
    DUnitTestTally& _tally,
    const char*     _name,
    Fn              _unit
)
{
    const bool before = _ok;

    (void)_tally;
    _unit();

    if ( (before) &&
         (!_ok) )
    {
        std::printf("    [FAIL] %s\n", _name);
    }

    return;
}

// D_COMPOSE_CHECK
//   macro: one check of a converted body, its text the message.
#define D_COMPOSE_CHECK(...) \
    record_assertion(verdict, (__VA_ARGS__), #__VA_ARGS__)



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
    D_CONSTEXPR
    int operator()(
        int _x
    ) const
    {
        return _x + 1;
    }
};

// doubler
//   struct: unary transformer doubling an int.
struct doubler
{
    D_CONSTEXPR
    int operator()(
        int _x
    ) const
    {
        return _x * 2;
    }
};

// negate_int
//   struct: unary transformer negating an int.
struct negate_int
{
    D_CONSTEXPR
    int operator()(
        int _x
    ) const
    {
        return -_x;
    }
};

// to_string_fn
//   struct: unary transformer mapping an int to its decimal string.
struct to_string_fn
{
    std::string operator()(
        int _x
    ) const
    {
        return std::to_string(_x);
    }
};

// length_of
//   struct: unary transformer yielding the size of a string.
struct length_of
{
    std::size_t operator()(
        const std::string& _s
    ) const
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

// DSinkVoid
//   struct: a unary callable returning void, used as a negative case
// for is_unary_transformer (which requires a non-void result).
struct DSinkVoid
{
    void operator()(
        int
    ) const
    {
        return;
    }
};

// DSubtractTwo
//   struct: binary transformer computing _a - _b, used by the
// partial-application section.
struct DSubtractTwo
{
    D_CONSTEXPR
    int operator()(
        int _a,
        int _b
    ) const
    {
        return _a - _b;
    }
};

// DSubtractThree
//   struct: ternary transformer computing _a - _b - _c, used by the
// partial-application section.
struct DSubtractThree
{
    D_CONSTEXPR
    int operator()(
        int _a,
        int _b,
        int _c
    ) const
    {
        return _a - _b - _c;
    }
};

// free_add_one
//   function: free-function unary transformer over const int&; the
// const-reference parameter matches memoize's keyed call shape.
inline
int free_add_one(
    const int& _x
)
{
    return _x + 1;
}
#endif  // !DTEST_SPEC_MODE  (fixtures)



///////////////////////////////////////////////////////////////////////////////
///                II.  SECTION FUNCTION DECLARATIONS                        ///
///////////////////////////////////////////////////////////////////////////////
//   One section function per like-group semantic section of compose.hpp.
// Each is defined in its own .cpp and matches the framework's section
// signature `void(test::test_handler&)`.

bool compose_tests_traits();
bool compose_tests_composition();
bool compose_tests_variadic();
bool compose_tests_partial();
bool compose_tests_tap();
bool compose_tests_memoize();
bool compose_tests_fix();


///////////////////////////////////////////////////////////////////////////////
///                III. MODULE WIRING                                        ///
///////////////////////////////////////////////////////////////////////////////
//   The module identity constant and the run_all entry point that
// schedules every section in document order.  Defined in
// compose_tests_runner.cpp.

// compose_module_info
//   constant: per-module identity bound at the in-output banner site.

// compose_module_run_all
//   function: schedules every compose.hpp test section against the
// runner engine in document order.


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// compose_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
compose_spec()
{
    return dt::module_spec{
        "compose",
        "template function composition and partial application (C++)",
        {
            dt::block_spec{
                "traits",
                "Traits",
                {
                    { "compose_tests_traits",
                      "Exercises compose.hpp Section 0 (predicate SFINAE "
                      "traits + concepts)",
                      &compose_tests_traits },
                }
            },
            dt::block_spec{
                "composition",
                "Composition",
                {
                    { "compose_tests_composition",
                      "Exercises compose.hpp Section II (compose / pipe / "
                      "compose_transformer)",
                      &compose_tests_composition },
                }
            },
            dt::block_spec{
                "variadic",
                "Variadic",
                {
                    { "compose_tests_variadic",
                      "Exercises compose.hpp Section III (compose_all / "
                      "pipe_all)",
                      &compose_tests_variadic },
                }
            },
            dt::block_spec{
                "partial",
                "Partial",
                {
                    { "compose_tests_partial",
                      "Exercises compose.hpp Section IV (partial_back)",
                      &compose_tests_partial },
                }
            },
            dt::block_spec{
                "tap",
                "Tap",
                {
                    { "compose_tests_tap",
                      "Exercises compose.hpp Section V (tap)",
                      &compose_tests_tap },
                }
            },
            dt::block_spec{
                "memoize",
                "Memoize",
                {
                    { "compose_tests_memoize",
                      "Exercises compose.hpp Section VI (memoize)",
                      &compose_tests_memoize },
                }
            },
            dt::block_spec{
                "fix",
                "Fix",
                {
                    { "compose_tests_fix",
                      "Exercises compose.hpp Section VII (fix)",
                      &compose_tests_fix },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_FUNCTIONAL_COMPOSE_
