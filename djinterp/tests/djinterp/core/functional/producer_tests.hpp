/******************************************************************************
* djinterp [functional]                                     producer_tests.hpp
*
*   Unit-test header for producer.hpp. Provides a lightweight, self-contained
* test harness in `djinterp::testing`, shared helper producer/functor types,
* and the declarations of the per-section test entry points (one per
* like-group semantic section of producer.hpp).
*
*   HARNESS (DTest run_session):
*   Converted from the original self-contained test_registry harness onto the
* DTest run_session model, so this suite reads and reports like every other
* functional module. Each section has the framework's leaf signature
* `void(test::test_handler&)` and records its findings through D_TEST_CHECK --
* the thin bridge over test::record_assertion that replaced D_TESTING_CHECK (it
* stringifies the checked expression as the assertion label). Purely
* compile-time facts (traits / concepts) remain `static_assert` in the .cpp
* files. The module identity / run-all entry points are consumed by the session
* runner (producer_tests_runner.cpp).
*
*   SECTION MAP (one .cpp per section):
*     producer_tests_step.cpp       -- producer_step<T>, make_step, no_step
*     producer_tests_generators.cpp -- iterate / unfold / range / iota /
*                                      repeat / repeat_n / cycle / generate /
*                                      empty / single / from_container
*     producer_tests_adapters.cpp   -- take_n / drop_n / concat / interleave /
*                                      transform / filter, and the CRTP
*                                      producer_base::collect terminal
*     producer_tests_terminals.cpp  -- free collect / for_each / fold
*     producer_tests_traits.cpp     -- is_producer_step / is_producer /
*                                      producer_value_type + C++20 concepts
*
*   PORTABILITY:
*   Compiles and passes under C++11 through C++20. (producer.hpp itself
* requires C++11+ for its rvalue-reference / variadic machinery.)
*
*
* path:      /inc/functional/producer_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.30
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_PRODUCER_TESTS_
#define DJINTERP_FUNCTIONAL_PRODUCER_TESTS_ 1

// std
#include <cstddef>
#include <vector>
// djinterp
#include "../djinterp.hpp"
#include "./producer.hpp"
// DTest framework (run_session model: handler + record_assertion + session).
// Same include convention as the example suites (view_tests.hpp, ...); adjust
// the prefix if this header sits elsewhere relative to the test framework.
#include "./test_common.hpp"
#include "./test_handler.hpp"
#include "./test_defaults.hpp"
#include "./test_runner.hpp"


#ifndef D_KEYWORD_TESTING
#  define D_KEYWORD_TESTING testing
#endif  // D_KEYWORD_TESTING

#ifndef NS_TESTING
#  define NS_TESTING D_NAMESPACE(D_KEYWORD_TESTING)
#endif  // NS_TESTING


NS_DJINTERP
NS_TESTING


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
    int
    operator()(int _value) const
    {
        return (_value + 1);
    }
};

// times_two
//   struct: unary mapping, doubles its argument. Used with transform.
struct times_two
{
    int
    operator()(int _value) const
    {
        return (_value * 2);
    }
};

// is_even_pred
//   struct: unary predicate, true when its argument is divisible by two.
// Used with filter.
struct is_even_pred
{
    bool
    operator()(int _value) const
    {
        return ((_value % 2) == 0);
    }
};

// constant_source
//   struct: nullary callable returning a fixed value. Drives generate(...).
struct constant_source
{
    int value;

    constant_source()
        : value(0)
    {}

    explicit constant_source(int _value)
        : value(_value)
    {}

    int
    operator()() const
    {
        return value;
    }
};

// counting_source
//   struct: nullary callable returning a strictly increasing counter,
// starting at zero. Each call yields the next integer. Used to give
// generate(...) a distinguishable, ordered output stream.
struct counting_source
{
    mutable int next;

    counting_source()
        : next(0)
    {}

    int
    operator()() const
    {
        return next++;
    }
};

// sum_step
//   struct: binary accumulator step (acc, value) -> acc + value, for the
// fold terminal.
struct sum_step
{
    int
    operator()(int _acc, int _value) const
    {
        return (_acc + _value);
    }
};

// push_consumer
//   struct: a consumer that appends each value it receives to a vector it
// references. Used to exercise for_each. The target is held by pointer so
// the consumer remains copyable (for_each takes its consumer by value).
struct push_consumer
{
    std::vector<int>* sink;

    push_consumer()
        : sink(0)
    {}

    explicit push_consumer(std::vector<int>* _sink)
        : sink(_sink)
    {}

    void
    operator()(int _value) const
    {
        if (sink)
        {
            sink->push_back(_value);
        }

        return;
    }
};

// unfold_countdown
//   struct: an unfold step of signature
//   producer_step<pair<int,int>>(int). Emits the current state as the
// value and decrements the state, stopping when the state reaches zero.
// Produces the sequence n, n-1, ..., 1 for an initial state of n.
struct unfold_countdown
{
    producer_step<std::pair<int, int> >
    operator()(int _state) const
    {
        if (_state <= 0)
        {
            return no_step<std::pair<int, int> >();
        }

        // value = current state; next state = state - 1
        return make_step(std::make_pair(_state, _state - 1));
    }
};


///////////////////////////////////////////////////////////////////////////////
///                III. PER-SECTION TEST ENTRY POINTS                       ///
///////////////////////////////////////////////////////////////////////////////
// Each section has the framework's leaf signature void(test::test_handler&) and
// records its findings with D_TEST_CHECK (test::record_assertion), so the totals
// roll up through the runner exactly like every other run_session module.

// producer_step<T>, make_step, no_step
void test_producer_step(test::test_handler&);

// iterate / unfold / range / iota / repeat / repeat_n / cycle /
// generate / empty / single / from_container
void test_producer_generators(test::test_handler&);

// take_n / drop_n / concat / interleave / transform / filter + collect()
void test_producer_adapters(test::test_handler&);

// free collect / for_each / fold
void test_producer_terminals(test::test_handler&);

// is_producer_step / is_producer / producer_value_type + concepts
void test_producer_traits(test::test_handler&);


// producer_module_info / producer_module_run_all
//   The run_session wiring: module_info carries the module identity and
// producer_module_run_all schedules every section against the shared engine.
// Both are DEFINED in producer_tests_runner.cpp.
extern const test::test_module_info producer_module_info;

void producer_module_run_all(test::test_runner_ctx&);


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_PRODUCER_TESTS_
