/******************************************************************************
* djinterp [functional]                                     transducer_tests.hpp
*
* Unit-test declarations for transducer.hpp.
*   This header declares the full battery of transducer tests and the helpers
* shared between the per-section translation units. Each semantic section of
* transducer.hpp maps to one .cpp file and one section runner declared here:
*
*   transducer_tests_state.cpp        -> run_state_tests        (I)
*   transducer_tests_core.cpp         -> run_core_tests         (II: map..drop)
*   transducer_tests_core2.cpp        -> run_core2_tests        (II: take_while..)
*   transducer_tests_composition.cpp  -> run_composition_tests  (III)
*   transducer_tests_drivers.cpp      -> run_drivers_tests      (IV)
*   transducer_tests_traits.cpp       -> run_traits_tests       (V)
*
*   Every test is a parameterless predicate returning true on success and
* false on the first failed check. Tests live in djinterp::testing; the
* transducer factories under test are reached through djinterp::transducers,
* and the core types (reducing_state, the drivers, the traits) are flat
* djinterp types.
*
*   transducer.hpp REQUIRES C++14 (generic lambdas + deduced returns) and is
* suppressed below it. Accordingly this whole suite is wrapped in the same
* feature guard: under C++11 every test is a trivially-passing stub, so the
* suite links and reports success without exercising the (absent) module.
*
*   FIXTURES use named functor / struct types rather than lambdas where a
* type name is needed (the traits section takes decltype of transducers built
* from them, and a lambda in an unevaluated context is only legal from
* C++20). Named functors keep the suite uniform; runtime-only lambdas would
* be fine from C++14 but are avoided for consistency.
*
* path:      /test/functional/transducer_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.05.25
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    HELPERS
      1.  D_INTERNAL_TRD_CHECK            (early-return assertion)
      2.  feature guard                   (mirror transducer.hpp's C++14 gate)
      3.  transforms / predicates         (square / is_even / under ...)
      4.  side effects                    (counter / collector)
      5.  expand                           (flat_map one-to-many)
      6.  int_sum_acc                      (accumulator-protocol fixture)
      7.  range_producer                   (producer-protocol fixture)
      8.  int_reducer / not_a_reducer      (reducer fixtures for traits)
II.   PER-SECTION TEST DECLARATIONS
III.  SECTION RUNNERS + TOP-LEVEL AGGREGATE
*/

#ifndef DJINTERP_TEST_FUNCTIONAL_TRANSDUCER_
#define DJINTERP_TEST_FUNCTIONAL_TRANSDUCER_ 1

// std
#include <cstddef>
#include <vector>
// djinterp
#include "../../inc/functional/transducer.hpp"

#ifdef DTEST_SPEC_MODE
    // The enriched runner defines DTEST_SPEC_MODE and consumes transducer_spec()
    // (below). Pull in the DTest authoring surface: module_spec / block_spec /
    // test_spec, build_enriched_tree, run_module, and the test_option_set the
    // runner configures. Resolved via the djinterp include root, the same root
    // the runners' <djinterp/core/djinterp.hpp> resolves against. Gated so the
    // plain sectional builds - which never define DTEST_SPEC_MODE - do not pull
    // in the framework. (The runner is C++14+, so DJINTERP_TEST_TRANSDUCER_ENABLED
    // is always true wherever the spec is compiled.)
#   include <djinterp/test/test_defaults.hpp>
#endif


// D_INTERNAL_TRD_CHECK
//   macro: returns false from the enclosing test the moment a condition
// fails. Wrapped in a do/while so it is a single statement.
#define D_INTERNAL_TRD_CHECK(_cond)                                           \
    do                                                                        \
    {                                                                         \
        if (!(_cond))                                                         \
        {                                                                     \
            return false;                                                     \
        }                                                                     \
    } while (0)


// DJINTERP_TEST_TRANSDUCER_ENABLED
//   The module compiles only on C++14+ (generic lambdas + deduced returns).
// Mirror that gate here so the suite degrades to trivially-passing stubs
// under C++11 rather than failing to compile against an absent module.
#if ( D_ENV_CPP_FEATURE_LANG_GENERIC_LAMBDAS  &&                              \
      D_ENV_CPP_FEATURE_LANG_DECLTYPE_AUTO )
    #define DJINTERP_TEST_TRANSDUCER_ENABLED 1
#else
    #define DJINTERP_TEST_TRANSDUCER_ENABLED 0
#endif


NS_DJINTERP
NS_TESTING


#if DJINTERP_TEST_TRANSDUCER_ENABLED

///////////////////////////////////////////////////////////////////////////////
///                I.   HELPERS                                              ///
///////////////////////////////////////////////////////////////////////////////

// square
//   struct: transform doubling-as-squaring an int (for map / flat-shape).
struct square
{
    int operator()(
        int _x
    ) const
    {
        return (_x * _x);
    }
};


// plus_one
//   struct: transform incrementing an int (for chained map composition).
struct plus_one
{
    int operator()(
        int _x
    ) const
    {
        return (_x + 1);
    }
};


// is_even
//   struct: predicate, true for even ints.
struct is_even
{
    bool operator()(
        int _x
    ) const
    {
        return (_x % 2 == 0);
    }
};


// under
//   struct: predicate, true while an int is strictly below a stored bound
// (for take_while / drop_while).
struct under
{
    int bound;

    explicit under(
        int _b
    )
        : bound(_b)
    {}

    bool operator()(
        int _x
    ) const
    {
        return (_x < bound);
    }
};


// counter
//   struct: side effect counting invocations through a shared count (for
// tap). Holds a pointer so copies of the closure share one tally.
struct counter
{
    int* hits;

    explicit counter(
        int* _hits
    )
        : hits(_hits)
    {}

    void operator()(
        int
    ) const
    {
        ++(*hits);
    }
};


// expand
//   struct: one-to-many expansion for flat_map: maps n to the vector
// {n, n} (each value duplicated). Returns an iterable as required.
struct expand
{
    std::vector<int> operator()(
        int _n
    ) const
    {
        std::vector<int> out;
        out.push_back(_n);
        out.push_back(_n);
        return out;
    }
};


// int_sum_acc
//   struct: minimal accumulator-protocol fixture for
// transduce_into_accumulator. Exposes input_type, step(value), and
// finalize(), summing the values it receives.
struct int_sum_acc
{
    typedef int input_type;

    int total;

    int_sum_acc()
        : total(0)
    {}

    void step(
        int _v
    )
    {
        total += _v;
    }

    int finalize() const
    {
        return total;
    }
};


// range_producer
//   struct: minimal producer-protocol fixture for
// transduce_producer_to_consumer. Pulls 0, 1, ..., count-1 one at a time;
// each operator() yields a step whose has_value flags exhaustion.
struct range_producer
{
    typedef int value_type;

    struct step_type
    {
        bool has_value;
        int  value;
    };

    int count;
    int pos;

    explicit range_producer(
        int _count
    )
        : count(_count)
        , pos(0)
    {}

    step_type operator()()
    {
        step_type s;

        if (pos < count)
        {
            s.has_value = true;
            s.value     = pos;
            ++pos;
        }
        else
        {
            s.has_value = false;
            s.value     = 0;
        }

        return s;
    }
};


// int_reducer
//   struct: a reducer over (reducing_state<int>&, const int&) that sums into
// the accumulator. Named so the traits section can take its type.
struct int_reducer
{
    void operator()(
        reducing_state<int>& _state,
        const int&           _value
    ) const
    {
        _state.accumulator() += _value;
    }
};


// not_a_reducer
//   struct: negative fixture for the reducer / transducer traits. No usable
// call operator of the reducer shape.
struct not_a_reducer
{
    int x;
};


///////////////////////////////////////////////////////////////////////////////
///                II.  PER-SECTION TEST DECLARATIONS                        ///
///////////////////////////////////////////////////////////////////////////////

// I. reduced + reducing_state
//////////////////////////////////////////
bool test_state_reducing_basic();
bool test_state_mark_done();
bool test_state_reduced_wrapper();
bool test_state_reduced_move();

// II. core transducers (map / filter / filter_not / take / drop)
//////////////////////////////////////////
bool test_core_map();
bool test_core_filter();
bool test_core_filter_not();
bool test_core_take();
bool test_core_take_zero();
bool test_core_take_more_than_size();
bool test_core_drop();
bool test_core_drop_all();

// II. core transducers (take_while / drop_while / distinct / tap / flat_map)
//////////////////////////////////////////
bool test_core2_take_while();
bool test_core2_take_while_none();
bool test_core2_drop_while();
bool test_core2_drop_while_all();
bool test_core2_distinct();
bool test_core2_tap();
bool test_core2_flat_map();
bool test_core2_flat_map_short_circuit();

// III. composition
//////////////////////////////////////////
bool test_composition_compose2();
bool test_composition_compose_variadic();
bool test_composition_pipe_operator();
bool test_composition_pipe_lvalue();
bool test_composition_order();
bool test_composition_into_reducer();

// IV. drivers
//////////////////////////////////////////
bool test_drivers_transduce();
bool test_drivers_into_vector();
bool test_drivers_into_vector_empty();
bool test_drivers_into_accumulator();
bool test_drivers_producer_to_consumer();
bool test_drivers_short_circuit_stops_source();

// V. traits & concepts
//////////////////////////////////////////
bool test_traits_is_reducing_state();
bool test_traits_is_reducer();
bool test_traits_is_transducer();
bool test_traits_transduces_reducer();
bool test_traits_result_type();
bool test_traits_value_aliases();
bool test_traits_concepts();

#endif  // DJINTERP_TEST_TRANSDUCER_ENABLED


///////////////////////////////////////////////////////////////////////////////
///                III. SECTION RUNNERS + TOP-LEVEL AGGREGATE                ///
///////////////////////////////////////////////////////////////////////////////

bool run_state_tests();
bool run_core_tests();
bool run_core2_tests();
bool run_composition_tests();
bool run_drivers_tests();
bool run_traits_tests();


// run_all_transducer_tests
//   function: drives every section runner. Returns true only when all
// sections pass. Under C++11 each runner is a trivially-passing stub (the
// module is absent), so this still returns true.
inline bool
run_all_transducer_tests()
{
    return ( run_state_tests()       &&
             run_core_tests()         &&
             run_core2_tests()        &&
             run_composition_tests()  &&
             run_drivers_tests()      &&
             run_traits_tests() );
}


///////////////////////////////////////////////////////////////////////////////
///                IV.  SPEC PROVIDER  (DTEST_SPEC_MODE)                     ///
///////////////////////////////////////////////////////////////////////////////

#if defined(DTEST_SPEC_MODE) && DJINTERP_TEST_TRANSDUCER_ENABLED

// transducer_spec
//   function: the suite as plain data for the enriched runner. Each of the six
// sections becomes one block_spec; each predicate declared above becomes one
// test_spec carrying a name (the predicate's identifier minus the test_ prefix)
// and the one-line descriptor lifted verbatim from its section .cpp. Block
// order matches the section runners' document order. run_module() lowers this
// into the six-kind tree and drives the report / PDF from the same data,
// running each predicate exactly once for its leaf verdict. Gated on the C++14
// feature guard as well as DTEST_SPEC_MODE, since it references the guarded
// predicates (the runner is C++14+, so both always hold where it is compiled).
namespace dt = ::djinterp::test;

inline dt::module_spec
transducer_spec()
{
    return dt::module_spec{
        "transducer.hpp",
        "reducing_state / reduced, the core and extended transducers, "
        "composition, the drivers, and structural traits for transducer.hpp",
        {
            dt::block_spec{ "state",
                "reducing_state / reduced",
                {
                    { "state_reducing_basic",  "Verifies reducing_state exposes a mutable accumulator and starts not-done.", &test_state_reducing_basic },
                    { "state_mark_done",       "Verifies mark_done flips the termination flag.",                            &test_state_mark_done },
                    { "state_reduced_wrapper", "Verifies the reduced<_Acc> wrapper stores and returns its value.",          &test_state_reduced_wrapper },
                    { "state_reduced_move",    "Verifies the rvalue value() overload moves the wrapped accumulator out.",   &test_state_reduced_move }
                }
            },
            dt::block_spec{ "core",
                "map / filter / filter_not / take / drop",
                {
                    { "core_map",                 "Verifies map transforms each value before forwarding.",                  &test_core_map },
                    { "core_filter",              "Verifies filter forwards only values satisfying the predicate.",         &test_core_filter },
                    { "core_filter_not",          "Verifies filter_not forwards only values failing the predicate.",        &test_core_filter_not },
                    { "core_take",                "Verifies take forwards at most n values then short-circuits.",           &test_core_take },
                    { "core_take_zero",           "Verifies take(0) forwards nothing and immediately signals done.",        &test_core_take_zero },
                    { "core_take_more_than_size", "Verifies take(n) past the input size forwards everything cleanly.",      &test_core_take_more_than_size },
                    { "core_drop",                "Verifies drop skips the first n values and forwards the rest.",          &test_core_drop },
                    { "core_drop_all",            "Verifies drop(n) with n >= size forwards nothing.",                      &test_core_drop_all }
                }
            },
            dt::block_spec{ "core2",
                "take_while / drop_while / distinct / tap / flat_map",
                {
                    { "core2_take_while",              "Verifies take_while forwards values while the predicate holds, then stops.", &test_core2_take_while },
                    { "core2_take_while_none",         "Verifies take_while yields nothing when the first value already fails.",   &test_core2_take_while_none },
                    { "core2_drop_while",              "Verifies drop_while drops the leading run, then forwards the rest.",        &test_core2_drop_while },
                    { "core2_drop_while_all",          "Verifies drop_while drops everything when the predicate never fails.",     &test_core2_drop_while_all },
                    { "core2_distinct",                "Verifies distinct<int>() forwards each value only the first time seen.",   &test_core2_distinct },
                    { "core2_tap",                     "Verifies tap forwards every value unchanged while invoking the side effect.", &test_core2_tap },
                    { "core2_flat_map",                "Verifies flat_map expands each value into a sequence and forwards each.",  &test_core2_flat_map },
                    { "core2_flat_map_short_circuit",  "Verifies flat_map honours mid-expansion termination.",                     &test_core2_flat_map_short_circuit }
                }
            },
            dt::block_spec{ "composition",
                "compose / operator| / into_reducer",
                {
                    { "composition_compose2",         "Verifies compose(t1, t2) applies t1 first, then t2 (t1 outer).",       &test_composition_compose2 },
                    { "composition_compose_variadic", "Verifies the variadic compose folds left across three or more stages.", &test_composition_compose_variadic },
                    { "composition_pipe_operator",    "Verifies operator| composes two transducers equivalently to compose.", &test_composition_pipe_operator },
                    { "composition_pipe_lvalue",      "Verifies operator| accepts named (lvalue) transducer operands.",       &test_composition_pipe_lvalue },
                    { "composition_order",            "Verifies the documented ordering: a | b means a sees values first.",   &test_composition_order },
                    { "composition_into_reducer",     "Verifies into_reducer applies a transducer to a downstream reducer.",  &test_composition_into_reducer }
                }
            },
            dt::block_spec{ "drivers",
                "transduce / into_vector / into_accumulator / producer_to_consumer",
                {
                    { "drivers_transduce",                "Verifies the general transduce driver applies a transducer to a reducer.", &test_drivers_transduce },
                    { "drivers_into_vector",              "Verifies transduce_into_vector collects survivors into a vector.",        &test_drivers_into_vector },
                    { "drivers_into_vector_empty",        "Verifies transduce_into_vector over an empty source yields an empty vector.", &test_drivers_into_vector_empty },
                    { "drivers_into_accumulator",         "Verifies transduce_into_accumulator adapts an accumulator-protocol sink.", &test_drivers_into_accumulator },
                    { "drivers_producer_to_consumer",     "Verifies transduce_producer_to_consumer pulls a producer into a consumer.", &test_drivers_producer_to_consumer },
                    { "drivers_short_circuit_stops_source", "Verifies a short-circuiting transducer stops the driver pulling.",       &test_drivers_short_circuit_stops_source }
                }
            },
            dt::block_spec{ "traits",
                "is_reducing_state / is_reducer / is_transducer / concepts",
                {
                    { "traits_is_reducing_state",   "Verifies is_reducing_state recognises reducing_state specializations.",  &test_traits_is_reducing_state },
                    { "traits_is_reducer",          "Verifies is_reducer detects the (reducing_state<Acc>&, const Value&) step.", &test_traits_is_reducer },
                    { "traits_is_transducer",       "Verifies the marker-based is_transducer recognises the module's helpers.", &test_traits_is_transducer },
                    { "traits_transduces_reducer",  "Verifies the behavioural transduces_reducer trait.",                     &test_traits_transduces_reducer },
                    { "traits_result_type",         "Verifies transducer_result_t yields a concrete reducer type.",           &test_traits_result_type },
                    { "traits_value_aliases",       "Verifies the variable-template _v shorthands agree with their traits.",  &test_traits_value_aliases },
                    { "traits_concepts",            "Verifies the C++20 concepts accept the right shapes and reject wrong ones.", &test_traits_concepts }
                }
            }
        }
    };
}

#endif  // DTEST_SPEC_MODE && DJINTERP_TEST_TRANSDUCER_ENABLED


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TEST_FUNCTIONAL_TRANSDUCER_
