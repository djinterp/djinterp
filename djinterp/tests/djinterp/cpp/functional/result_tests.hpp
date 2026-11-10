/*******************************************************************************
* djinterp [test]                                               result_tests.hpp
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
*   Each section function is a `bool()` unit test: it records its findings
* on a local verdict with this header's record_assertion and returns it.
* The spec provider lists the functions for run_module, so the suite runs
* and reports like every other DTest module.
*
*   The generic monad protocol (monad_bind / map_with / ... in monad.hpp) is
* exercised by the monad module's own suite; here we test only the surface
* result.hpp itself defines, including its monad_traits<result<_T, _E>>
* specialization's direct members.
*
*
* path:      /tests/djinterp/cpp/functional/result_tests.hpp
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

#ifndef DJINTERP_TEST_RESULT_TESTS_
#define DJINTERP_TEST_RESULT_TESTS_ 1

// std
#include <cstdio>
#include <cstddef>
#include <string>
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/functional/result.hpp"
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

// D_RESULT_CHECK
//   macro: one check of a converted body, its text the message.
#define D_RESULT_CHECK(...) \
    record_assertion(verdict, (__VA_ARGS__), #__VA_ARGS__)



///////////////////////////////////////////////////////////////////////////////
///                I.   SHARED HELPER TYPES & CALLABLES                      ///
///////////////////////////////////////////////////////////////////////////////
//   Named function objects (rather than lambdas) so the same types can be used
// in both runtime invocations and unevaluated trait/concept contexts at every
// supported language standard.  result<int, std::string> is the canonical
// instantiation under test; the helpers below are shaped to its value (int)
// and error (std::string) sides.

// DFnDouble
//   helper: int -> int value mapper (result::map).
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
//   helper: int -> std::string, a type-changing value mapper.
struct fn_to_string
{
    std::string operator()(
        const int& _v
    ) const
    {
        return std::to_string(_v);
    }
};

// DFnErrLen
//   helper: std::string -> std::size_t, a type-changing error mapper
// (result::map_err).
struct DFnErrLen
{
    std::size_t operator()(
        const std::string& _e
    ) const
    {
        return _e.size();
    }
};

// DFnErrPrefix
//   helper: std::string -> std::string error mapper that tags the error.
struct DFnErrPrefix
{
    std::string operator()(
        const std::string& _e
    ) const
    {
        return std::string("E:") + _e;
    }
};

// DFnSafeHalve
//   helper: int -> result<int, std::string>; ok(v/2) when even, err("odd")
// otherwise. An and_then / bind arrow on the success side.
struct DFnSafeHalve
{
    result<int, std::string> operator()(
        const int& _v
    ) const
    {
        if ((_v % 2) == 0)
        {
            return ok<int, std::string>(_v / 2);
        }

        return err<int, std::string>("odd");
    }
};

// DFnRecoverZero
//   helper: std::string -> result<int, std::string>; recovers any error to
// ok(0). An or_else recovery that succeeds.
struct DFnRecoverZero
{
    result<int, std::string> operator()(
        const std::string&
    ) const
    {
        return ok<int, std::string>(0);
    }
};

// DFnRewrapErr
//   helper: std::string -> result<int, std::string>; an or_else recovery
// that maps to a different error.
struct DFnRewrapErr
{
    result<int, std::string> operator()(
        const std::string& _e
    ) const
    {
        return err<int, std::string>(std::string("wrapped:") + _e);
    }
};

// DFnOkShow
//   helper: int -> std::string, the ok arm of result::match.
struct DFnOkShow
{
    std::string operator()(
        const int& _v
    ) const
    {
        return std::string("ok:") + std::to_string(_v);
    }
};

// DFnErrShow
//   helper: std::string -> std::string, the err arm of result::match.
struct DFnErrShow
{
    std::string operator()(
        const std::string& _e
    ) const
    {
        return std::string("err:") + _e;
    }
};

// DFnAdd
//   helper: binary int sum for combine().
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

// DFnStringOnly
//   helper: callable only with std::string; NOT callable with int. Used as a
// negative case for is_result_value_mapper<_, int>.
struct DFnStringOnly
{
    bool operator()(
        const std::string&
    ) const
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
bool test_lifetime();

bool test_map();
bool test_map_err();
bool test_and_then();
bool test_or_else();
bool test_match();

bool test_equality();

bool test_traits();

bool test_factories();

bool test_combinators();

bool test_conversions();

bool test_monad_traits();

bool test_collect();
bool test_combine();


///////////////////////////////////////////////////////////////////////////////
///                III. MODULE ENTRY POINTS                                  ///
///////////////////////////////////////////////////////////////////////////////


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// result_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
result_spec()
{
    return dt::module_spec{
        "result",
        "result<T, E> -- a monadic success-or-error type (C++)",
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
                    { "test_lifetime",
                      "lifetime",
                      &test_lifetime },
                }
            },
            dt::block_spec{
                "transform",
                "Transform",
                {
                    { "test_map",
                      "map",
                      &test_map },
                    { "test_map_err",
                      "map err",
                      &test_map_err },
                    { "test_and_then",
                      "and then",
                      &test_and_then },
                    { "test_or_else",
                      "or else",
                      &test_or_else },
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
                "conversions",
                "Conversions",
                {
                    { "test_conversions",
                      "conversions",
                      &test_conversions },
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
                    { "test_collect",
                      "collect",
                      &test_collect },
                    { "test_combine",
                      "combine",
                      &test_combine },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_RESULT_TESTS_
