/*******************************************************************************
* djinterp [test]                                       compose_tests_traits.cpp
*
*   Tests for compose.hpp Section 0: the predicate SFINAE structural
* traits and the C++20 concept mirrors.  Almost all coverage here is
* compile-time: each property reduces to a static_assert via the
* DTest trait/concept/type macros, so a regression halts the build
* with a descriptive message rather than failing silently at runtime.
* The section function records a single runtime roll-up assertion so
* the report carries a visible row for the compile-time suite.
*
*
* path:      /tests/djinterp/cpp/functional/compose_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.31
*                                                            revised: 2026.11.09
*******************************************************************************/

// djinterp
#include "compose_tests.hpp"


NS_DJINTERP
NS_TESTING



namespace {

    // ---- is_invocable -----------------------------------------------------
    // positive: each helper callable accepts its intended argument.
    D_TEST_STATIC(is_invocable< add_one, int >::value);
    D_TEST_STATIC(is_invocable< doubler, int >::value);
    D_TEST_STATIC(is_invocable< to_string_fn, int >::value);
    D_TEST_STATIC(is_invocable< length_of, std::string >::value);
    D_TEST_STATIC(is_invocable< DSinkVoid, int >::value);
    // negative: wrong argument type, or a non-callable type.
    D_TEST_STATIC(!is_invocable< add_one, std::string >::value);
    D_TEST_STATIC(!is_invocable< length_of, int >::value);
    D_TEST_STATIC(!is_invocable< not_callable, int >::value);
    D_TEST_STATIC(!is_invocable< int, int >::value);

    // ---- callable_result_t ------------------------------------------------
    D_TEST_STATIC(std::is_same< callable_result_t<add_one, int>, int >::value);
    D_TEST_STATIC(std::is_same< callable_result_t<to_string_fn, int>, std::string >::value);
    D_TEST_STATIC(std::is_same< callable_result_t<length_of, std::string>, std::size_t >::value);

    // ---- is_invocable_r ---------------------------------------------------
    // exact result type, and a widening conversion, both satisfy.
    D_TEST_STATIC(is_invocable_r< int, add_one, int >::value);
    D_TEST_STATIC(is_invocable_r< long, add_one, int >::value);
    D_TEST_STATIC(is_invocable_r< std::string, to_string_fn, int >::value);
    // not convertible: int result is not convertible to std::string;
    // and an uninvocable pairing is false regardless of result type.
    D_TEST_STATIC(!is_invocable_r< std::string, add_one, int >::value);
    D_TEST_STATIC(!is_invocable_r< int, add_one, std::string >::value);

    // ---- is_unary_transformer --------------------------------------------
    // non-void unary callables qualify; the void-returning sink does not.
    D_TEST_STATIC(is_unary_transformer< add_one, int >::value);
    D_TEST_STATIC(is_unary_transformer< to_string_fn, int >::value);
    D_TEST_STATIC(!is_unary_transformer< DSinkVoid, int >::value);
    D_TEST_STATIC(!is_unary_transformer< add_one, std::string >::value);

    // ---- is_composable ----------------------------------------------------
    // outer(inner(input)) well-formed in both same-type and changing-type
    // chains; ill-formed when the outer cannot accept the inner result.
    D_TEST_STATIC(is_composable< add_one, doubler, int >::value);
    D_TEST_STATIC(is_composable< to_string_fn, add_one, int >::value);
    D_TEST_STATIC(is_composable< length_of, to_string_fn, int >::value);
    D_TEST_STATIC(!is_composable< add_one, to_string_fn, int >::value);
    D_TEST_STATIC(!is_composable< length_of, add_one, int >::value);

    // ---- composition_result_t --------------------------------------------
    D_TEST_STATIC(std::is_same< composition_result_t<add_one, doubler, int>, int >::value);
    D_TEST_STATIC(std::is_same< composition_result_t<to_string_fn, add_one, int>, std::string >::value);
    D_TEST_STATIC(std::is_same< composition_result_t<length_of, to_string_fn, int>, std::size_t >::value);

    // ---- is_composed_transformer (structural) ----------------------------
    // the three factories all yield the same composed_transformer_helper
    // surface, so all three are detected; raw callables are not.
    using compose_t   = decltype(compose(add_one(), doubler()));
    using pipe_t      = decltype(pipe(add_one(), doubler()));
    using transform_t = decltype(compose_transformer(add_one(), doubler()));

    D_TEST_STATIC(is_composed_transformer< compose_t >::value);
    D_TEST_STATIC(is_composed_transformer< pipe_t >::value);
    D_TEST_STATIC(is_composed_transformer< transform_t >::value);
    D_TEST_STATIC(!is_composed_transformer< add_one >::value);
    D_TEST_STATIC(!is_composed_transformer< int >::value);

    // ---- is_memoized (structural) ----------------------------------------
    using memoize_t = decltype(
        ::djinterp::memoize<int (*)(const int&), int>(&free_add_one));

    D_TEST_STATIC(is_memoized< memoize_t >::value);
    D_TEST_STATIC(!is_memoized< add_one >::value);
    D_TEST_STATIC(!is_memoized< compose_t >::value);

    // ---- variable-template `_v` aliases (C++14+) -------------------------
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_invocable_v<add_one, int>,
                  "is_invocable_v should agree with is_invocable");
    static_assert(!is_invocable_v<add_one, std::string>,
                  "is_invocable_v negative case");
    static_assert(is_invocable_r_v<long, add_one, int>,
                  "is_invocable_r_v widening case");
    static_assert(is_unary_transformer_v<to_string_fn, int>,
                  "is_unary_transformer_v positive case");
    static_assert(is_composable_v<length_of, to_string_fn, int>,
                  "is_composable_v positive case");
    static_assert(!is_composable_v<add_one, to_string_fn, int>,
                  "is_composable_v negative case");
    static_assert(is_composed_transformer_v<compose_t>,
                  "is_composed_transformer_v positive case");
    static_assert(!is_composed_transformer_v<add_one>,
                  "is_composed_transformer_v negative case");
    static_assert(is_memoized_v<memoize_t>,
                  "is_memoized_v positive case");
    static_assert(!is_memoized_v<add_one>,
                  "is_memoized_v negative case");
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // ---- concept mirrors (C++20) -----------------------------------------
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    D_TEST_STATIC(invocable_with< add_one, int >);
    D_TEST_STATIC(!invocable_with< add_one, std::string >);
    D_TEST_STATIC(unary_transformer< to_string_fn, int >);
    D_TEST_STATIC(!unary_transformer< DSinkVoid, int >);
    D_TEST_STATIC(composable< length_of, to_string_fn, int >);
    D_TEST_STATIC(!composable< add_one, to_string_fn, int >);
    D_TEST_STATIC(composed_transformer_like< compose_t >);
    D_TEST_STATIC(!composed_transformer_like< add_one >);
    D_TEST_STATIC(memoized_like< memoize_t >);
    D_TEST_STATIC(!memoized_like< add_one >);
#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

}  // namespace


/*
compose_tests_traits
  Exercises compose.hpp Section 0 (predicate SFINAE traits + concepts).
  Tests the following:
  - is_invocable: positive/negative across callables and a non-callable
  - callable_result_t: result-type extraction for several callables
  - is_invocable_r: exact, widening, and non-convertible result cases
  - is_unary_transformer: non-void unary acceptance, void rejection
  - is_composable: well-formed and ill-formed outer(inner(input)) chains
  - composition_result_t: result type of the composed chain
  - is_composed_transformer: structural detection of the factory surface
  - is_memoized: structural detection of the memoize surface
  - `_v` variable-template aliases agree with their trait structs (C++14+)
  - concept mirrors track their traits (C++20)
  All of the above are validated at compile time via static_assert; the
  single recorded assertion below is the runtime roll-up so the suite
  appears as a row in the report.  If this translation unit compiled,
  every compile-time property held.
*/
bool compose_tests_traits()
{
    bool verdict = true;

    DUnitTestTally tally;

    // every static_assert above is part of THIS translation unit; if it
    // compiled, the compile-time suite passed in full.  Record a single
    // green roll-up so the report shows a row for it.
    run_unit_test(verdict,
        tally,
        "compose traits + concepts (compile-time suite)",
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
