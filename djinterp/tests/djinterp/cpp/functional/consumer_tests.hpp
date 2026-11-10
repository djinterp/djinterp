/*******************************************************************************
* djinterp [functional]                                       consumer_tests.hpp
*
*   DTest declarations and shared helper types for the consumer.hpp unit
* test suite.  Each like-group semantic section of consumer.hpp maps to
* a single `bool()` section function declared here and
* defined in its own .cpp translation unit:
*
*     section in consumer.hpp           test .cpp file
*     -------------------------------   ------------------------------
*     0.  predicate traits / concepts   consumer_tests_traits.cpp
*     II. primitives (print/write/...)  consumer_tests_primitives.cpp
*     II. adapters (filtered/mapped)    consumer_tests_adapters.cpp
*     II. tee (broadcast)               consumer_tests_tee.cpp
*     II. flow (batched/take/drop)      consumer_tests_flow.cpp
*     II. branching (conditional/...)   consumer_tests_branching.cpp
*     III.type erasure (boxed/box)      consumer_tests_erasure.cpp
*
*   The compile-time trait/concept checks (section 0) live in
* static_asserts within consumer_tests_traits.cpp; the section function
* there records one runtime roll-up assertion so the report shows a row
* for the compile-time suite.
*
*   The shared helper callables and probes below are header-only (types
* or `inline` functions) so every section may include this header
* without ODR trouble.
*
*   PORTABILITY:
*   C++11 minimum.  Several consumer helpers use D_CONSTEXPR14 bodies
* (loops / assignments), which are runtime on C++11 and constexpr on
* C++14+; the tests exercise them at runtime on every standard.
*
*
* path:      /tests/djinterp/cpp/functional/consumer_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.31
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_TESTING_FUNCTIONAL_CONSUMER_
#define DJINTERP_TESTING_FUNCTIONAL_CONSUMER_ 1

// std
#include <cstdio>
#include <cstddef>
#include <string>
#include <vector>
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/functional/consumer.hpp"
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

// D_CONSUMER_CHECK
//   macro: one check of a converted body, its text the message.
#define D_CONSUMER_CHECK(...) \
    record_assertion(verdict, (__VA_ARGS__), #__VA_ARGS__)



///////////////////////////////////////////////////////////////////////////////
///                I.   SHARED HELPER CALLABLES & PROBES                     ///
///////////////////////////////////////////////////////////////////////////////
//   Small header-only function objects and probe sinks used across the
// section translation units.  Every entity is a type or an `inline`
// function so multiple .cpp files may include this header without
// violating the ODR.

// is_even
//   struct: predicate returning true for even ints.
struct is_even
{
    bool operator()(
        int _x
    ) const
    {
        return (_x % 2) == 0;
    }
};

// always_true
//   struct: predicate that always returns true.
struct always_true
{
    bool operator()(
        int
    ) const
    {
        return true;
    }
};

// always_false
//   struct: predicate that always returns false.
struct always_false
{
    bool operator()(
        int
    ) const
    {
        return false;
    }
};

// doubler
//   struct: transformer doubling an int (A -> A).
struct doubler
{
    int operator()(
        int _x
    ) const
    {
        return _x * 2;
    }
};

// to_string_fn
//   struct: transformer mapping an int to its decimal string (A -> B).
struct to_string_fn
{
    std::string operator()(
        int _x
    ) const
    {
        return std::to_string(_x);
    }
};

// void_returning
//   struct: a unary callable returning void.  Used as a negative case
// for is_predicate / is_transformer.
struct void_returning
{
    void operator()(
        int
    ) const
    {
        return;
    }
};

// not_callable
//   struct: a plain aggregate with no call operator.  Negative case for
// every callable-shaped trait.
struct not_callable
{
    int m_value;
};

// free_int_sink
//   function: a free-function consumer of int (void(const int&)).
inline
void free_int_sink(
    const int&
)
{
    return;
}

// probe_sink
//   class: a copyable consumer that records each received int into an
// externally-owned vector (held by pointer), so the log survives the
// by-value copies that the combinators make of their inner consumers.
class probe_sink
{
public:
    explicit probe_sink(
        std::vector<int>& _log
    )
        : m_log(&_log)
    {}

    void operator()(
        const int& _value
    ) const
    {
        m_log->push_back(_value);

        return;
    }

private:
    std::vector<int>* m_log;
};

// DTestConsumerError
//   struct: the exception type thrown by throwing_sink, used to drive
// the fallback path.
struct DTestConsumerError
{
};

// throwing_sink
//   class: a consumer whose call always throws DTestConsumerError,
// used to drive the fallback path.  Records nothing.
class throwing_sink
{
public:
    void operator()(
        const int&
    ) const
    {
        throw DTestConsumerError();
    }
};
#endif  // !DTEST_SPEC_MODE  (fixtures)



///////////////////////////////////////////////////////////////////////////////
///                II.  SECTION FUNCTION DECLARATIONS                        ///
///////////////////////////////////////////////////////////////////////////////
//   One section function per like-group semantic section of
// consumer.hpp.  Each matches the framework's section signature
// `void(test::test_handler&)`.

bool consumer_tests_traits();
bool consumer_tests_primitives();
bool consumer_tests_adapters();
bool consumer_tests_tee();
bool consumer_tests_flow();
bool consumer_tests_branching();
bool consumer_tests_erasure();


///////////////////////////////////////////////////////////////////////////////
///                III. MODULE WIRING                                        ///
///////////////////////////////////////////////////////////////////////////////
//   The module identity constant and the run_all entry point that
// schedules every section in document order.  Defined in
// consumer_tests_runner.cpp.

// consumer_module_info
//   constant: per-module identity bound at the in-output banner site.

// consumer_module_run_all
//   function: schedules every consumer.hpp test section against the
// runner engine in document order.


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// consumer_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
consumer_spec()
{
    return dt::module_spec{
        "consumer",
        "first-class consumers (sinks) for functional dataflow (C++)",
        {
            dt::block_spec{
                "traits",
                "Traits",
                {
                    { "consumer_tests_traits",
                      "Exercises consumer.hpp Section 0 (predicate SFINAE "
                      "traits + concepts)",
                      &consumer_tests_traits },
                }
            },
            dt::block_spec{
                "primitives",
                "Primitives",
                {
                    { "consumer_tests_primitives",
                      "Exercises consumer.hpp Section II primitives",
                      &consumer_tests_primitives },
                }
            },
            dt::block_spec{
                "adapters",
                "Adapters",
                {
                    { "consumer_tests_adapters",
                      "Exercises consumer.hpp Section II adapters (filtered, "
                      "mapped)",
                      &consumer_tests_adapters },
                }
            },
            dt::block_spec{
                "tee",
                "Tee",
                {
                    { "consumer_tests_tee",
                      "Exercises consumer.hpp Section II tee (broadcast)",
                      &consumer_tests_tee },
                }
            },
            dt::block_spec{
                "flow",
                "Flow",
                {
                    { "consumer_tests_flow",
                      "Exercises consumer.hpp Section II flow combinators "
                      "(batched/take/drop)",
                      &consumer_tests_flow },
                }
            },
            dt::block_spec{
                "branching",
                "Branching",
                {
                    { "consumer_tests_branching",
                      "Exercises consumer.hpp Section II branching "
                      "(conditional, fallback)",
                      &consumer_tests_branching },
                }
            },
            dt::block_spec{
                "erasure",
                "Erasure",
                {
                    { "consumer_tests_erasure",
                      "Exercises consumer.hpp Section III (DBoxedConsumer, "
                      "box)",
                      &consumer_tests_erasure },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_FUNCTIONAL_CONSUMER_
