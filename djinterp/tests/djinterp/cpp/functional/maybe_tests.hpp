/*******************************************************************************
* djinterp [functional]                                          maybe_tests.hpp
*
*   DTest declarations and shared fixtures for the maybe.hpp unit-test
* suite.  Declares one section function per like-group semantic section of
* maybe.hpp (lifetime / value access, monadic methods, equality operators,
* predicate & structural traits, factories, combinator pipeline, the
* monad_traits specialization, and the free-function helpers) plus the
* module identity / run-all entry points consumed by the session runner.
* Definitions live in the per-section .cpp files; this header carries only
* declarations and the helper types the sections share.
*
*   Each section function is a `bool()` unit test: it records its findings
* on a local verdict with this header's record_assertion and returns it.
* The spec provider lists the functions for run_module, so the suite runs
* and reports like every other DTest module.
*
*   The maybe.hpp monad protocol combinators (bind_with, map_with, ...) are
* exercised by the monad module's own suite; here we test only the surface
* that maybe.hpp itself defines, including its monad_traits<maybe<_T>>
* specialization's direct members.
*
*
* path:      /tests/djinterp/cpp/functional/maybe_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.31
*                                                            revised: 2026.11.09
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SHARED HELPER TYPES & CALLABLES
      -------------------------------

II.   SECTION FUNCTION DECLARATIONS
      -----------------------------

III.  MODULE ENTRY POINTS
      -------------------
*/

#ifndef DJINTERP_TESTING_FUNCTIONAL_MAYBE_
#define DJINTERP_TESTING_FUNCTIONAL_MAYBE_ 1

// std
#include <cstdio>
#include <cstddef>
#include <string>
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/functional/maybe.hpp"
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

// D_MAYBE_CHECK
//   macro: one check of a converted body, its text the message.
#define D_MAYBE_CHECK(...) \
    record_assertion(verdict, (__VA_ARGS__), #__VA_ARGS__)



///////////////////////////////////////////////////////////////////////////////
///                I.   SHARED HELPER TYPES & CALLABLES                      ///
///////////////////////////////////////////////////////////////////////////////
//   Named function objects (rather than lambdas) so the same types can be used
// in both runtime invocations and unevaluated trait/concept contexts at every
// supported language standard.  Each carries a fixed, explicit shape so the
// SFINAE-driven trait detection resolves deterministically.

// pred_is_even
//   helper: unary predicate over int; a valid maybe filter predicate.
struct pred_is_even
{
    bool operator()(
        const int& _v
    ) const
    {
        return (_v % 2) == 0;
    }
};

// pred_is_positive
//   helper: unary predicate over int; used for filter / from_predicate.
struct pred_is_positive
{
    bool operator()(
        const int& _v
    ) const
    {
        return _v > 0;
    }
};

// DFnReturnsVoid
//   helper: callable with const int& but returning void; NOT a predicate
// (its result is not convertible to bool).
struct DFnReturnsVoid
{
    void operator()(
        const int&
    ) const
    {
        return;
    }
};

// DFnStringOnly
//   helper: callable only with std::string; NOT callable with int, so it is
// not a predicate over int.
struct DFnStringOnly
{
    bool operator()(
        const std::string&
    ) const
    {
        return true;
    }
};

// DFnDouble
//   helper: int -> int mapping function for map().
struct DFnDouble
{
    int operator()(
        const int& _v
    ) const
    {
        return _v * 2;
    }
};

// fn_to_string
//   helper: int -> std::string mapping (type-changing map / match).
struct fn_to_string
{
    std::string operator()(
        const int& _v
    ) const
    {
        return std::to_string(_v);
    }
};

// DFnNothingLabel
//   helper: nullary callable returning the std::string "none"; the
// nothing-branch of match().
struct DFnNothingLabel
{
    std::string operator()() const
    {
        return std::string("none");
    }
};

// DFnHalfIfEven
//   helper: int -> maybe<int>; just(v/2) when even, nothing otherwise.
// A bind/and_then function (returns a maybe).
struct DFnHalfIfEven
{
    maybe<int> operator()(
        const int& _v
    ) const
    {
        if ((_v % 2) == 0)
        {
            return just(_v / 2);
        }

        return nothing<int>();
    }
};

// DFnMakeJust
//   helper: nullary callable returning just(99); the or_else fallback.
struct DFnMakeJust
{
    maybe<int> operator()() const
    {
        return just(99);
    }
};

// DFnMakeNothing
//   helper: nullary callable returning an empty maybe<int>.
struct DFnMakeNothing
{
    maybe<int> operator()() const
    {
        return nothing<int>();
    }
};

// DFnAdd
//   helper: binary int sum for zip_with.
struct DFnAdd
{
    int operator()(
        const int& _a,
        const int& _b
    ) const
    {
        return _a + _b;
    }
};

// tracked
//   helper: instance-counting value type used to verify that maybe manages
// construction and destruction in balance across copy, move, assignment,
// reset, emplace, and scope exit.  The static counters are defined once in
// maybe_tests_lifetime.cpp.
struct tracked
{
    static int live;        // currently-alive instances
    static int constructed; // lifetime count of constructions
    static int destroyed;   // lifetime count of destructions

    int id;

    explicit tracked(int _id = 0)
        : id(_id)
    {
        ++live;
        ++constructed;
    }

    tracked(const tracked& _other)
        : id(_other.id)
    {
        ++live;
        ++constructed;
    }

    tracked(tracked&& _other) noexcept
        : id(_other.id)
    {
        ++live;
        ++constructed;
    }

    tracked& operator=(const tracked& _other)
    {
        id = _other.id;
        return *this;
    }

    tracked& operator=(tracked&& _other) noexcept
    {
        id = _other.id;
        return *this;
    }

    ~tracked()
    {
        --live;
        ++destroyed;
    }

    bool operator==(
        const tracked& _other
    ) const
    {
        return id == _other.id;
    }

    static
    void reset_counters()
    {
        live        = 0;
        constructed = 0;
        destroyed   = 0;
    }
};
#endif  // !DTEST_SPEC_MODE  (fixtures)



///////////////////////////////////////////////////////////////////////////////
///                II.  SECTION FUNCTION DECLARATIONS                        ///
///////////////////////////////////////////////////////////////////////////////

bool test_construction();
bool test_assignment();
bool test_observers();
bool test_modifiers();
bool test_lifetime();

bool test_map();
bool test_and_then();
bool test_or_else();
bool test_filter();
bool test_match();

bool test_equality();

bool test_traits();

bool test_factories();

bool test_combinators();

bool test_monad_traits();

bool test_zip_with();
bool test_flatten();
bool test_collect();


///////////////////////////////////////////////////////////////////////////////
///                III. MODULE ENTRY POINTS                                  ///
///////////////////////////////////////////////////////////////////////////////


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// maybe_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
maybe_spec()
{
    return dt::module_spec{
        "maybe",
        "maybe<T> -- a monadic optional value type (C++)",
        {
            dt::block_spec{
                "lifetime",
                "Lifetime",
                {
                    { "test_construction",
                      "construction",
                      &test_construction },
                    { "test_assignment",
                      "assignment",
                      &test_assignment },
                    { "test_observers",
                      "observers",
                      &test_observers },
                    { "test_modifiers",
                      "modifiers",
                      &test_modifiers },
                    { "test_lifetime",
                      "lifetime",
                      &test_lifetime },
                }
            },
            dt::block_spec{
                "monadic",
                "Monadic",
                {
                    { "test_map",
                      "map",
                      &test_map },
                    { "test_and_then",
                      "and then",
                      &test_and_then },
                    { "test_or_else",
                      "or else",
                      &test_or_else },
                    { "test_filter",
                      "filter",
                      &test_filter },
                    { "test_match",
                      "match",
                      &test_match },
                }
            },
            dt::block_spec{
                "equality",
                "Equality",
                {
                    { "test_equality",
                      "equality",
                      &test_equality },
                }
            },
            dt::block_spec{
                "traits",
                "Traits",
                {
                    { "test_traits",
                      "traits",
                      &test_traits },
                }
            },
            dt::block_spec{
                "factories",
                "Factories",
                {
                    { "test_factories",
                      "factories",
                      &test_factories },
                }
            },
            dt::block_spec{
                "combinators",
                "Combinators",
                {
                    { "test_combinators",
                      "combinators",
                      &test_combinators },
                }
            },
            dt::block_spec{
                "monad_traits",
                "Monad traits",
                {
                    { "test_monad_traits",
                      "monad traits",
                      &test_monad_traits },
                }
            },
            dt::block_spec{
                "free",
                "Free",
                {
                    { "test_zip_with",
                      "zip with",
                      &test_zip_with },
                    { "test_flatten",
                      "flatten",
                      &test_flatten },
                    { "test_collect",
                      "collect",
                      &test_collect },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_FUNCTIONAL_MAYBE_
