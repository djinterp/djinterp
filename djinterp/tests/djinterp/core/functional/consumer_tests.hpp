/******************************************************************************
* djinterp [test]                                           consumer_tests.hpp
*
*   DTest declarations and shared helper types for the consumer.hpp unit
* test suite.  Each like-group semantic section of consumer.hpp maps to
* a single `void(test_handler&)` section function declared here and
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
* path:      /inc/djinterp/core/functional/tests/consumer_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.31
******************************************************************************/

#ifndef DJINTERP_FUNCTIONAL_CONSUMER_TESTS_
#define DJINTERP_FUNCTIONAL_CONSUMER_TESTS_ 1

// std
#include <cstddef>
#include <string>
#include <vector>
// djinterp
#include "../../core/djinterp.hpp"
#include "../../test/test_common.hpp"
#include "../../test/test_handler.hpp"
#include "../../test/test_defaults.hpp"
#include "../../test/test_runner.hpp"
#include "../consumer.hpp"


NS_DJINTERP
NS_TESTING


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
    bool operator()(int _x) const
    {
        return (_x % 2) == 0;
    }
};

// always_true
//   struct: predicate that always returns true.
struct always_true
{
    bool operator()(int) const
    {
        return true;
    }
};

// always_false
//   struct: predicate that always returns false.
struct always_false
{
    bool operator()(int) const
    {
        return false;
    }
};

// doubler
//   struct: transformer doubling an int (A -> A).
struct doubler
{
    int operator()(int _x) const
    {
        return _x * 2;
    }
};

// to_string_fn
//   struct: transformer mapping an int to its decimal string (A -> B).
struct to_string_fn
{
    std::string operator()(int _x) const
    {
        return std::to_string(_x);
    }
};

// void_returning
//   struct: a unary callable returning void.  Used as a negative case
// for is_predicate / is_transformer.
struct void_returning
{
    void operator()(int) const
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
inline void
free_int_sink(
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

// test_consumer_error
//   struct: the exception type thrown by throwing_sink, used to drive
// the fallback path.
struct test_consumer_error
{
};

// throwing_sink
//   class: a consumer whose call always throws test_consumer_error,
// used to drive the fallback path.  Records nothing.
class throwing_sink
{
public:
    void operator()(
        const int&
    ) const
    {
        throw test_consumer_error();
    }
};


///////////////////////////////////////////////////////////////////////////////
///                II.  SECTION FUNCTION DECLARATIONS                        ///
///////////////////////////////////////////////////////////////////////////////
//   One section function per like-group semantic section of
// consumer.hpp.  Each matches the framework's section signature
// `void(test::test_handler&)`.

void consumer_tests_traits(::djinterp::test::test_handler& _handler);
void consumer_tests_primitives(::djinterp::test::test_handler& _handler);
void consumer_tests_adapters(::djinterp::test::test_handler& _handler);
void consumer_tests_tee(::djinterp::test::test_handler& _handler);
void consumer_tests_flow(::djinterp::test::test_handler& _handler);
void consumer_tests_branching(::djinterp::test::test_handler& _handler);
void consumer_tests_erasure(::djinterp::test::test_handler& _handler);


///////////////////////////////////////////////////////////////////////////////
///                III. MODULE WIRING                                        ///
///////////////////////////////////////////////////////////////////////////////
//   The module identity constant and the run_all entry point that
// schedules every section in document order.  Defined in
// consumer_tests_runner.cpp.

// consumer_module_info
//   constant: per-module identity bound at the in-output banner site.
extern const ::djinterp::test::test_module_info consumer_module_info;

// consumer_module_run_all
//   function: schedules every consumer.hpp test section against the
// runner engine in document order.
void consumer_module_run_all(::djinterp::test::test_runner_ctx& _ctx);


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_FUNCTIONAL_CONSUMER_TESTS_
