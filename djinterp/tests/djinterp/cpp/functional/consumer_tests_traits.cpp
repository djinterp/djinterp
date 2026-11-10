/*******************************************************************************
* djinterp [test]                                      consumer_tests_traits.cpp
*
*   Tests for consumer.hpp Section 0: the predicate SFINAE structural
* traits and the C++20 concept mirrors.  Almost all coverage here is
* compile-time: each property reduces to a static_assert, so a
* regression halts the build with a descriptive message rather than
* failing silently at runtime.  The section function records a single
* runtime roll-up assertion so the report carries a visible row for the
* compile-time suite.
*
*
* path:      /tests/djinterp/cpp/functional/consumer_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.31
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "consumer_tests.hpp"


NS_DJINTERP
NS_TESTING



namespace {

    // concrete consumer types produced by the factories, named here so
    // the trait macros can pass them as single template arguments.
    using discard_t  = decltype(djinterp::discard());
    using free_sink_t = void (*)(const int&);

    // ---- is_consumer ------------------------------------------------------
    // positive: discard is a consumer of anything; a void(const int&)
    // function pointer is a consumer of int.
    D_TEST_STATIC(is_consumer< discard_t, int >::value);
    D_TEST_STATIC(is_consumer< discard_t, std::string >::value);
    D_TEST_STATIC(is_consumer< free_sink_t, int >::value);
    D_TEST_STATIC(is_consumer< void_returning, int >::value);
    // negative: a value-returning callable is not a consumer (result is
    // not void); a non-callable is not a consumer; wrong argument type.
    D_TEST_STATIC(!is_consumer< doubler, int >::value);
    D_TEST_STATIC(!is_consumer< is_even, int >::value);
    D_TEST_STATIC(!is_consumer< not_callable, int >::value);
    D_TEST_STATIC(!is_consumer< free_sink_t, std::string >::value);

    // ---- consumer_result_t ------------------------------------------------
    D_TEST_STATIC(std::is_same< consumer_result_t<discard_t, int>, void >::value);
    D_TEST_STATIC(std::is_same< consumer_result_t<doubler, int>, int >::value);
    D_TEST_STATIC(std::is_same< consumer_result_t<to_string_fn, int>, std::string >::value);

    // ---- is_predicate -----------------------------------------------------
    // positive: bool-returning unary callables.
    D_TEST_STATIC(is_predicate< is_even, int >::value);
    D_TEST_STATIC(is_predicate< always_true, int >::value);
    D_TEST_STATIC(is_predicate< always_false, int >::value);
    // negative: void result is not bool-convertible; non-callable.
    D_TEST_STATIC(!is_predicate< void_returning, int >::value);
    D_TEST_STATIC(!is_predicate< not_callable, int >::value);
    // a to_string transformer returns std::string, not bool-convertible
    D_TEST_STATIC(!is_predicate< to_string_fn, int >::value);

    // ---- is_transformer ---------------------------------------------------
    // positive: non-void unary callables, including same-type and
    // changing-type transforms.
    D_TEST_STATIC(is_transformer< doubler, int >::value);
    D_TEST_STATIC(is_transformer< to_string_fn, int >::value);
    D_TEST_STATIC(is_transformer< is_even, int >::value);   // bool is non-void
    // negative: void result; non-callable.
    D_TEST_STATIC(!is_transformer< void_returning, int >::value);
    D_TEST_STATIC(!is_transformer< not_callable, int >::value);

    // ---- contramap_input_t ------------------------------------------------
    // the value type a mapped consumer forwards to its inner consumer.
    D_TEST_STATIC(std::is_same< contramap_input_t<doubler, int>, int >::value);
    D_TEST_STATIC(std::is_same< contramap_input_t<to_string_fn, int>, std::string >::value);

    // ---- is_boxable -------------------------------------------------------
    // positive: a discard helper and a function pointer both convert to
    // std::function<void(const int&)>.
    D_TEST_STATIC(is_boxable< discard_t, int >::value);
    D_TEST_STATIC(is_boxable< free_sink_t, int >::value);
    // negative: a non-callable aggregate is not boxable.
    D_TEST_STATIC(!is_boxable< not_callable, int >::value);

    // ---- variable-template `_v` aliases (C++14+) -------------------------
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_consumer_v<discard_t, int>,
                  "is_consumer_v should agree with is_consumer");
    static_assert(!is_consumer_v<doubler, int>,
                  "is_consumer_v negative case");
    static_assert(is_predicate_v<is_even, int>,
                  "is_predicate_v positive case");
    static_assert(!is_predicate_v<void_returning, int>,
                  "is_predicate_v negative case");
    static_assert(is_transformer_v<to_string_fn, int>,
                  "is_transformer_v positive case");
    static_assert(!is_transformer_v<void_returning, int>,
                  "is_transformer_v negative case");
    static_assert(is_boxable_v<free_sink_t, int>,
                  "is_boxable_v positive case");
    static_assert(!is_boxable_v<not_callable, int>,
                  "is_boxable_v negative case");
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // ---- concept mirrors (C++20) -----------------------------------------
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    D_TEST_STATIC(consumes< discard_t, int >);
    D_TEST_STATIC(consumes< free_sink_t, int >);
    D_TEST_STATIC(!consumes< doubler, int >);
    D_TEST_STATIC(predicate_for< is_even, int >);
    D_TEST_STATIC(!predicate_for< void_returning, int >);
    D_TEST_STATIC(transformer_for< to_string_fn, int >);
    D_TEST_STATIC(!transformer_for< void_returning, int >);
    D_TEST_STATIC(boxable_as< free_sink_t, int >);
    D_TEST_STATIC(!boxable_as< not_callable, int >);
#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

}  // namespace


/*
consumer_tests_traits
  Exercises consumer.hpp Section 0 (predicate SFINAE traits + concepts).
  Tests the following:
  - is_consumer: void(const T&) acceptance; rejects value-returning,
    non-callable, and wrong-argument cases
  - consumer_result_t: result-type extraction for several callables
  - is_predicate: bool-convertible unary callables; rejects void and
    non-bool results
  - is_transformer: non-void unary callables; rejects void result
  - contramap_input_t: the inner element type a mapped consumer feeds
  - is_boxable: convertibility to std::function<void(const T&)>
  - `_v` variable-template aliases agree with their trait structs (C++14+)
  - concept mirrors track their traits (C++20)
  All validated at compile time via static_assert; the single recorded
  assertion below is the runtime roll-up.  If this translation unit
  compiled, every compile-time property held.
*/
bool consumer_tests_traits()
{
    bool verdict = true;

    DUnitTestTally tally;

    run_unit_test(verdict,
        tally,
        "consumer traits + concepts (compile-time suite)",
        [&]()
        {
            record_assertion(verdict,
                true,
                "Section 0 predicate traits and concepts compiled",
                "all static_assert checks held",
                nullptr);
        });

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
