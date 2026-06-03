/******************************************************************************
* djinterp [functional]                                     producer_tests.hpp
*
*   Unit-test header for producer.hpp. Provides a lightweight, self-contained
* test harness in `djinterp::testing`, shared helper producer/functor types,
* and the declarations of the per-section test entry points (one per
* like-group semantic section of producer.hpp).
*
*   HARNESS:
*   The harness is intentionally minimal so it carries no dependency on the
* full DTest session/tree machinery. A `test_registry` collects pass/fail
* tallies; each section function registers its checks against a registry
* reference and returns the number of failures it observed. Runtime checks
* use `D_TESTING_CHECK`; purely compile-time facts (traits / concepts) are
* asserted with `static_assert` directly in the .cpp files.
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


#ifndef D_KEYWORD_TESTING
#  define D_KEYWORD_TESTING testing
#endif  // D_KEYWORD_TESTING

#ifndef NS_TESTING
#  define NS_TESTING D_NAMESPACE(D_KEYWORD_TESTING)
#endif  // NS_TESTING


NS_DJINTERP
NS_TESTING


///////////////////////////////////////////////////////////////////////////////
///                I.   LIGHTWEIGHT TEST HARNESS                            ///
///////////////////////////////////////////////////////////////////////////////

// test_registry
//   class: minimal pass/fail tally for producer unit tests. Each runtime
// check increments either the pass or fail counter and records the first
// failure's location for reporting. Deliberately decoupled from the DTest
// session/tree machinery so these tests stay small and buildable alone.
class test_registry
{
public:
    test_registry()
        : m_checks(0),
          m_failures(0),
          m_first_fail_file(0),
          m_first_fail_line(0),
          m_first_fail_expr(0)
    {}

    // record
    //   function: record the outcome of a single runtime check.
    void
    record(
        bool        _ok,
        const char* _expr,
        const char* _file,
        int         _line
    )
    {
        ++m_checks;

        // remember the location of the first observed failure
        if (!_ok)
        {
            if (m_failures == 0)
            {
                m_first_fail_file = _file;
                m_first_fail_line = _line;
                m_first_fail_expr = _expr;
            }

            ++m_failures;
        }

        return;
    }

    std::size_t checks()   const { return m_checks; }
    std::size_t failures() const { return m_failures; }
    bool        passed()   const { return (m_failures == 0); }

    const char* first_fail_file() const { return m_first_fail_file; }
    int         first_fail_line() const { return m_first_fail_line; }
    const char* first_fail_expr() const { return m_first_fail_expr; }

private:
    std::size_t m_checks;
    std::size_t m_failures;
    const char* m_first_fail_file;
    int         m_first_fail_line;
    const char* m_first_fail_expr;
};


// D_TESTING_CHECK
//   macro: record a single boolean check against a test_registry named
// `_reg`. Evaluates `_expr` exactly once. Wrap expressions containing a
// top-level comma (e.g. multi-arg template-ids) in an extra set of parens
// so the preprocessor does not split them.
#define D_TESTING_CHECK(_reg, _expr)                                          \
    (_reg).record((_expr), #_expr, __FILE__, __LINE__)


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
// Each function registers its checks against the supplied registry and
// returns the number of failures it observed (0 == all passed).

// producer_step<T>, make_step, no_step
std::size_t test_producer_step(test_registry& _reg);

// iterate / unfold / range / iota / repeat / repeat_n / cycle /
// generate / empty / single / from_container
std::size_t test_producer_generators(test_registry& _reg);

// take_n / drop_n / concat / interleave / transform / filter + collect()
std::size_t test_producer_adapters(test_registry& _reg);

// free collect / for_each / fold
std::size_t test_producer_terminals(test_registry& _reg);

// is_producer_step / is_producer / producer_value_type + concepts
std::size_t test_producer_traits(test_registry& _reg);


// run_all_producer_tests
//   function: drives every section against the supplied registry and
// returns the total number of failures observed (0 == the suite passed).
std::size_t run_all_producer_tests(test_registry& _reg);


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_PRODUCER_TESTS_
