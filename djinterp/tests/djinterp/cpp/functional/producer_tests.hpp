/*******************************************************************************
* djinterp [functional]                                       producer_tests.hpp
*
*   Unit-test header for producer.hpp. Provides a lightweight, self-contained
* test harness in `djinterp::testing`, shared helper producer/functor types,
* and the declarations of the per-section test entry points (one per
* like-group semantic section of producer.hpp).
*
*   HARNESS (DTest spec idiom):
*   Converted from the original self-contained test_registry harness, then
* from DTest's retired session model, onto the spec idiom: each section is a
* `bool()` unit test that records its findings through D_PRODUCER_CHECK (a
* thin macro over this header's record_assertion that stringifies the
* checked expression as the label) and returns its verdict. Purely
* compile-time facts (traits / concepts) remain `static_assert` in the .cpp
* files. The spec provider, producer_spec(), lists the sections for
* run_module (producer_tests_runner.cpp).
*
*   SECTION MAP (one .cpp per section):
*     producer_tests_step.cpp       -- DProducerStep<T>, make_step, no_step
*     producer_tests_generators.cpp -- iterate / unfold / range / iota /
*                                      repeat / repeat_n / cycle / generate /
*                                      empty / single / from_container
*     producer_tests_adapters.cpp   -- take_n / drop_n / concat / interleave /
*                                      transform / filter, and the CRTP
*                                      DProducerBase::collect terminal
*     producer_tests_terminals.cpp  -- free collect / for_each / fold
*     producer_tests_traits.cpp     -- is_producer_step / is_producer /
*                                      producer_value_type + C++20 concepts
*
*   PORTABILITY:
*   Compiles and passes under C++11 through C++20. (producer.hpp itself
* requires C++11+ for its rvalue-reference / variadic machinery.)
*
*
* path:      /tests/djinterp/cpp/functional/producer_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.30
*                                                            revised: 2026.11.09
*******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_PRODUCER_TESTS_
#define DJINTERP_FUNCTIONAL_PRODUCER_TESTS_ 1

// std
#include <cstdio>
#include <cstddef>
#include <vector>
// djinterp
#include "../../../../inc/djinterp/cpp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/functional/producer.hpp"
#endif
#ifdef DTEST_SPEC_MODE
#include "../../../../inc/djinterp/cpp/modules/test/test_defaults.hpp"  // module_spec, run_module
#endif

#ifndef DTEST_SPEC_MODE  // fixtures - normal mode only
// DTest framework (run_session model: handler + record_assertion + session).
// Same include convention as the example suites (view_tests.hpp, ...); adjust
// the prefix if this header sits elsewhere relative to the test framework.


#ifndef D_KEYWORD_TESTING
#  define D_KEYWORD_TESTING testing
#endif  // D_KEYWORD_TESTING

#ifndef NS_TESTING
#  define NS_TESTING D_NAMESPACE(D_KEYWORD_TESTING)
#endif  // NS_TESTING


#endif  // !DTEST_SPEC_MODE
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

// D_PRODUCER_CHECK
//   macro: one check of a converted body, its text the message.
#define D_PRODUCER_CHECK(...) \
    record_assertion(verdict, (__VA_ARGS__), #__VA_ARGS__)



///////////////////////////////////////////////////////////////////////////////
///                I.   ASSERTION BRIDGE                                    ///
///////////////////////////////////////////////////////////////////////////////

// D_TEST_CHECK
//   macro: records one boolean check against the DTest handler `_h`, using
// the stringified expression as the assertion label, by forwarding to
// test::record_assertion. Section bodies stay flat check-lists while results
// flow through the framework's handler / tree / report like every other
// run_session suite. Variadic so a top-level comma inside a multi-arg
// template-id passes through as a single expression.
#define D_TEST_CHECK(_h, ...)                                                 \
    ::djinterp::test::record_assertion((_h), (__VA_ARGS__), #__VA_ARGS__)


///////////////////////////////////////////////////////////////////////////////
///                II.  SHARED HELPER TYPES                                 ///
///////////////////////////////////////////////////////////////////////////////
// Plain function objects (not lambdas) so they are nameable in trait checks
// and behave identically across C++11 through C++20.

// add_one
//   struct: unary step function, returns its argument plus one. Drives
// iterate(...) into an arithmetic progression.
struct add_one
{
    int operator()(
        int _value
    ) const
    {
        return (_value + 1);
    }
};

// times_two
//   struct: unary mapping, doubles its argument. Used with transform.
struct times_two
{
    int operator()(
        int _value
    ) const
    {
        return (_value * 2);
    }
};

// is_even_pred
//   struct: unary predicate, true when its argument is divisible by two.
// Used with filter.
struct is_even_pred
{
    bool operator()(
        int _value
    ) const
    {
        return ((_value % 2) == 0);
    }
};

// DConstantSource
//   struct: nullary callable returning a fixed value. Drives generate(...).
struct DConstantSource
{
    int value;

    DConstantSource()
        : value(0)
    {}

    explicit DConstantSource(int _value)
        : value(_value)
    {}

    int operator()() const
    {
        return value;
    }
};

// DCountingSource
//   struct: nullary callable returning a strictly increasing counter,
// starting at zero. Each call yields the next integer. Used to give
// generate(...) a distinguishable, ordered output stream.
struct DCountingSource
{
    mutable int next;

    DCountingSource()
        : next(0)
    {}

    int operator()() const
    {
        return next++;
    }
};

// DSumStep
//   struct: binary accumulator step (acc, value) -> acc + value, for the
// fold terminal.
struct DSumStep
{
    int operator()(
        int _acc,
        int _value
    ) const
    {
        return (_acc + _value);
    }
};

// DPushConsumer
//   struct: a consumer that appends each value it receives to a vector it
// references. Used to exercise for_each. The target is held by pointer so
// the consumer remains copyable (for_each takes its consumer by value).
struct DPushConsumer
{
    std::vector<int>* sink;

    DPushConsumer()
        : sink(0)
    {}

    explicit DPushConsumer(std::vector<int>* _sink)
        : sink(_sink)
    {}

    void operator()(
        int _value
    ) const
    {
        if (sink)
        {
            sink->push_back(_value);
        }

        return;
    }
};

// DUnfoldCountdown
//   struct: an unfold step of signature
//   DProducerStep<pair<int,int>>(int). Emits the current state as the
// value and decrements the state, stopping when the state reaches zero.
// Produces the sequence n, n-1, ..., 1 for an initial state of n.
struct DUnfoldCountdown
{
    DProducerStep<std::pair<int, int> > operator()(
        int _state
    ) const
    {
        if (_state <= 0)
        {
            return no_step<std::pair<int, int> >();
        }

        // value = current state; next state = state - 1
        return make_step(std::make_pair(_state, _state - 1));
    }
};
#endif  // !DTEST_SPEC_MODE  (fixtures)



///////////////////////////////////////////////////////////////////////////////
///                III. PER-SECTION TEST ENTRY POINTS                       ///
///////////////////////////////////////////////////////////////////////////////
// Each section has the framework's leaf signature void(test::test_handler&) and
// records its findings with D_TEST_CHECK (test::record_assertion), so the totals
// roll up through the runner exactly like every other run_session module.

// DProducerStep<T>, make_step, no_step
bool test_producer_step();

// iterate / unfold / range / iota / repeat / repeat_n / cycle /
// generate / empty / single / from_container
bool test_producer_generators();

// take_n / drop_n / concat / interleave / transform / filter + collect()
bool test_producer_adapters();

// free collect / for_each / fold
bool test_producer_terminals();

// is_producer_step / is_producer / producer_value_type + concepts
bool test_producer_traits();


// producer_module_info / producer_module_run_all
//   The run_session wiring: module_info carries the module identity and
// producer_module_run_all schedules every section against the shared engine.
// Both are DEFINED in producer_tests_runner.cpp.


// -- the spec provider: spec mode only ----------------------------------------
#ifdef DTEST_SPEC_MODE
// producer_spec
//   function: the suite as DTest's module spec, one block per section.
inline
dt::module_spec
producer_spec()
{
    return dt::module_spec{
        "producer",
        "first-class producers (sources) for functional dataflow (C++)",
        {
            dt::block_spec{
                "step",
                "Step",
                {
                    { "test_producer_step",
                      "Tests DProducerStep<T>, make_step, and no_step",
                      &test_producer_step },
                }
            },
            dt::block_spec{
                "generators",
                "Generators",
                {
                    { "test_producer_generators",
                      "Tests the source producers",
                      &test_producer_generators },
                }
            },
            dt::block_spec{
                "adapters",
                "Adapters",
                {
                    { "test_producer_adapters",
                      "Tests the wrapping producers and the inherited "
                      "collect() terminal",
                      &test_producer_adapters },
                }
            },
            dt::block_spec{
                "terminals",
                "Terminals",
                {
                    { "test_producer_terminals",
                      "Tests the free collect / for_each / fold terminals",
                      &test_producer_terminals },
                }
            },
            dt::block_spec{
                "traits",
                "Traits",
                {
                    { "test_producer_traits",
                      "Tests the detection traits and concepts (a runtime "
                      "mirror of the compile-time static_asserts above, so "
                      "the harness registry reflects this section)",
                      &test_producer_traits },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_PRODUCER_TESTS_
