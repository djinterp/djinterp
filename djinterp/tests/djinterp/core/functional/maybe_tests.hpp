/******************************************************************************
* djinterp [functional]                                        maybe_tests.hpp
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
*   Each section function has the framework's leaf signature
* `void(test::test_handler&)` and records its findings with
* test::record_assertion, so the totals roll up through the runner exactly
* like every other DTest module.
*
*   The maybe.hpp monad protocol combinators (bind_with, map_with, ...) are
* exercised by the monad module's own suite; here we test only the surface
* that maybe.hpp itself defines, including its monad_traits<maybe<_T>>
* specialization's direct members.
*
*
* TABLE OF CONTENTS
* =================
* I.    SHARED HELPER TYPES & CALLABLES
* II.   SECTION FUNCTION DECLARATIONS
* III.  MODULE ENTRY POINTS
*
*
* path:      /tests/djinterp/core/functional/maybe_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.31
******************************************************************************/

#ifndef DJINTERP_TESTING_FUNCTIONAL_MAYBE_
#define DJINTERP_TESTING_FUNCTIONAL_MAYBE_ 1

// std
#include <cstddef>
#include <string>
// djinterp
#include "../core/functional/maybe.hpp"
#include "./test_common.hpp"
#include "./test_handler.hpp"
#include "./test_defaults.hpp"
#include "./test_runner.hpp"


NS_DJINTERP
NS_TESTING


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
    bool operator()(const int& _v) const
    {
        return (_v % 2) == 0;
    }
};

// pred_is_positive
//   helper: unary predicate over int; used for filter / from_predicate.
struct pred_is_positive
{
    bool operator()(const int& _v) const
    {
        return _v > 0;
    }
};

// fn_returns_void
//   helper: callable with const int& but returning void; NOT a predicate
// (its result is not convertible to bool).
struct fn_returns_void
{
    void operator()(const int&) const
    {
        return;
    }
};

// fn_string_only
//   helper: callable only with std::string; NOT callable with int, so it is
// not a predicate over int.
struct fn_string_only
{
    bool operator()(const std::string&) const
    {
        return true;
    }
};

// fn_double
//   helper: int -> int mapping function for map().
struct fn_double
{
    int operator()(const int& _v) const
    {
        return _v * 2;
    }
};

// fn_to_string
//   helper: int -> std::string mapping (type-changing map / match).
struct fn_to_string
{
    std::string operator()(const int& _v) const
    {
        return std::to_string(_v);
    }
};

// fn_nothing_label
//   helper: nullary callable returning the std::string "none"; the
// nothing-branch of match().
struct fn_nothing_label
{
    std::string operator()() const
    {
        return std::string("none");
    }
};

// fn_half_if_even
//   helper: int -> maybe<int>; just(v/2) when even, nothing otherwise.
// A bind/and_then function (returns a maybe).
struct fn_half_if_even
{
    maybe<int> operator()(const int& _v) const
    {
        if ((_v % 2) == 0)
        {
            return just(_v / 2);
        }

        return nothing<int>();
    }
};

// fn_make_just
//   helper: nullary callable returning just(99); the or_else fallback.
struct fn_make_just
{
    maybe<int> operator()() const
    {
        return just(99);
    }
};

// fn_make_nothing
//   helper: nullary callable returning an empty maybe<int>.
struct fn_make_nothing
{
    maybe<int> operator()() const
    {
        return nothing<int>();
    }
};

// fn_add
//   helper: binary int sum for zip_with.
struct fn_add
{
    int operator()(const int& _a, const int& _b) const
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

    bool operator==(const tracked& _other) const
    {
        return id == _other.id;
    }

    static void reset_counters()
    {
        live        = 0;
        constructed = 0;
        destroyed   = 0;
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  SECTION FUNCTION DECLARATIONS                        ///
///////////////////////////////////////////////////////////////////////////////

void test_construction(test::test_handler&);
void test_assignment(test::test_handler&);
void test_observers(test::test_handler&);
void test_modifiers(test::test_handler&);
void test_lifetime(test::test_handler&);

void test_map(test::test_handler&);
void test_and_then(test::test_handler&);
void test_or_else(test::test_handler&);
void test_filter(test::test_handler&);
void test_match(test::test_handler&);

void test_equality(test::test_handler&);

void test_traits(test::test_handler&);

void test_factories(test::test_handler&);

void test_combinators(test::test_handler&);

void test_monad_traits(test::test_handler&);

void test_zip_with(test::test_handler&);
void test_flatten(test::test_handler&);
void test_collect(test::test_handler&);


///////////////////////////////////////////////////////////////////////////////
///                III. MODULE ENTRY POINTS                                  ///
///////////////////////////////////////////////////////////////////////////////

extern const test::test_module_info maybe_module_info;

void maybe_module_run_all(test::test_runner_ctx&);


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_FUNCTIONAL_MAYBE_
