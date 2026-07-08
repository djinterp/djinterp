/******************************************************************************
* djinterp [test]                                             result_tests.hpp
*
*   DTest declarations and shared fixtures for the result.hpp unit-test
* suite.  Declares one section function per like-group semantic section of
* result.hpp (lifetime / value+error access, the transform methods, equality
* operators, the SFINAE structural traits, factories, combinator pipeline,
* the maybe conversions, the monad_traits specialization, and the free
* helpers) plus the module identity / run-all entry points consumed by the
* session runner.  Definitions live in the per-section .cpp files; this
* header carries only declarations and the helper types the sections share.
*
*   Each section function has the framework's leaf signature
* `void(test::test_handler&)` and records its findings with
* test::record_assertion, so the totals roll up through the runner exactly
* like every other DTest module.
*
*   The generic monad protocol (monad_bind / map_with / ... in monad.hpp) is
* exercised by the monad module's own suite; here we test only the surface
* result.hpp itself defines, including its monad_traits<result<_T, _E>>
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
* path:      /inc/djinterp/test/result_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.31
******************************************************************************/

#ifndef DJINTERP_TEST_RESULT_TESTS_
#define DJINTERP_TEST_RESULT_TESTS_ 1

// std
#include <cstddef>
#include <string>
// djinterp
#include "../core/functional/result.hpp"
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
// supported language standard.  result<int, std::string> is the canonical
// instantiation under test; the helpers below are shaped to its value (int)
// and error (std::string) sides.

// fn_double
//   helper: int -> int value mapper (result::map).
struct fn_double
{
    int operator()(const int& _v) const
    {
        return _v * 2;
    }
};

// fn_to_string
//   helper: int -> std::string, a type-changing value mapper.
struct fn_to_string
{
    std::string operator()(const int& _v) const
    {
        return std::to_string(_v);
    }
};

// fn_err_len
//   helper: std::string -> std::size_t, a type-changing error mapper
// (result::map_err).
struct fn_err_len
{
    std::size_t operator()(const std::string& _e) const
    {
        return _e.size();
    }
};

// fn_err_prefix
//   helper: std::string -> std::string error mapper that tags the error.
struct fn_err_prefix
{
    std::string operator()(const std::string& _e) const
    {
        return std::string("E:") + _e;
    }
};

// fn_safe_halve
//   helper: int -> result<int, std::string>; ok(v/2) when even, err("odd")
// otherwise. An and_then / bind arrow on the success side.
struct fn_safe_halve
{
    result<int, std::string> operator()(const int& _v) const
    {
        if ((_v % 2) == 0)
        {
            return ok<int, std::string>(_v / 2);
        }

        return err<int, std::string>("odd");
    }
};

// fn_recover_zero
//   helper: std::string -> result<int, std::string>; recovers any error to
// ok(0). An or_else recovery that succeeds.
struct fn_recover_zero
{
    result<int, std::string> operator()(const std::string&) const
    {
        return ok<int, std::string>(0);
    }
};

// fn_rewrap_err
//   helper: std::string -> result<int, std::string>; an or_else recovery
// that maps to a different error.
struct fn_rewrap_err
{
    result<int, std::string> operator()(const std::string& _e) const
    {
        return err<int, std::string>(std::string("wrapped:") + _e);
    }
};

// fn_ok_show
//   helper: int -> std::string, the ok arm of result::match.
struct fn_ok_show
{
    std::string operator()(const int& _v) const
    {
        return std::string("ok:") + std::to_string(_v);
    }
};

// fn_err_show
//   helper: std::string -> std::string, the err arm of result::match.
struct fn_err_show
{
    std::string operator()(const std::string& _e) const
    {
        return std::string("err:") + _e;
    }
};

// fn_add
//   helper: binary int sum for combine().
struct fn_add
{
    int operator()(const int& _a, const int& _b) const
    {
        return _a + _b;
    }
};

// fn_string_only
//   helper: callable only with std::string; NOT callable with int. Used as a
// negative case for is_result_value_mapper<_, int>.
struct fn_string_only
{
    bool operator()(const std::string&) const
    {
        return true;
    }
};

// tracked
//   helper: instance-counting payload used to verify that result manages
// construction and destruction in balance across copy, move, branch-changing
// assignment, and scope exit (for both the value and error branches). The
// static counters are defined once in result_tests_lifetime.cpp.
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
void test_lifetime(test::test_handler&);

void test_map(test::test_handler&);
void test_map_err(test::test_handler&);
void test_and_then(test::test_handler&);
void test_or_else(test::test_handler&);
void test_match(test::test_handler&);

void test_equality(test::test_handler&);

void test_traits(test::test_handler&);

void test_factories(test::test_handler&);

void test_combinators(test::test_handler&);

void test_conversions(test::test_handler&);

void test_monad_traits(test::test_handler&);

void test_collect(test::test_handler&);
void test_combine(test::test_handler&);


///////////////////////////////////////////////////////////////////////////////
///                III. MODULE ENTRY POINTS                                  ///
///////////////////////////////////////////////////////////////////////////////

extern const test::test_module_info result_module_info;

void result_module_run_all(test::test_runner_ctx&);


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_RESULT_TESTS_
