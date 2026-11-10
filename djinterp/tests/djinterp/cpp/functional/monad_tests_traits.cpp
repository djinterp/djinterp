/*******************************************************************************
* djinterp [test]                                         monad_tests_traits.cpp
*
*   Tests for monad.hpp Section 0: the predicate SFINAE structural
* traits and the C++20 concept mirrors.  Almost all coverage here is
* compile-time: each property reduces to a static_assert, so a
* regression halts the build with a descriptive message rather than
* failing silently at runtime.  The section function records a single
* runtime roll-up assertion so the report carries a visible row for the
* compile-time suite.
*
*
* path:      /tests/djinterp/cpp/functional/monad_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.08
*                                                            revised: 2026.11.09
*******************************************************************************/
// djinterp
#include "./monad_tests.hpp"


NS_DJINTERP
NS_TESTING



namespace {

    using maybe_int = DTestMaybe<int>;
    using DMaybeStr = DTestMaybe<std::string>;

    // ---- monad_value_type_t ----------------------------------------------
    D_TEST_STATIC(std::is_same< monad_value_type_t<maybe_int>, int >::value);
    D_TEST_STATIC(std::is_same< monad_value_type_t<DMaybeStr>, std::string >::value);

    // ---- monad_rebind_t ---------------------------------------------------
    D_TEST_STATIC(std::is_same< monad_rebind_t<maybe_int, double>, DTestMaybe<double> >::value);
    D_TEST_STATIC(std::is_same< monad_rebind_t<maybe_int, std::string>, DMaybeStr >::value);

    // ---- is_monadic_function ---------------------------------------------
    // positive: a Kleisli arrow int -> DTestMaybe<U> over maybe_int.
    D_TEST_STATIC(is_monadic_function< arrow_inc, maybe_int >::value);
    D_TEST_STATIC(is_monadic_function< DArrowToString, maybe_int >::value);
    D_TEST_STATIC(is_monadic_function< DArrowToNone, maybe_int >::value);
    // negative: a plain transform returns a non-monad; a non-callable
    // is not an arrow; a non-monad second arg is rejected.
    D_TEST_STATIC(!is_monadic_function< plain_double, maybe_int >::value);
    D_TEST_STATIC(!is_monadic_function< int, maybe_int >::value);
    D_TEST_STATIC(!is_monadic_function< arrow_inc, not_a_monad >::value);

    // ---- is_bindable ------------------------------------------------------
    // positive: monad_bind(maybe_int, arrow) is well-formed.
    D_TEST_STATIC(is_bindable< maybe_int, arrow_inc >::value);
    D_TEST_STATIC(is_bindable< maybe_int, DArrowToString >::value);
    // negative: a non-function second arg; a non-monad first arg.
    D_TEST_STATIC(!is_bindable< maybe_int, int >::value);
    D_TEST_STATIC(!is_bindable< not_a_monad, arrow_inc >::value);

    // ---- is_mappable ------------------------------------------------------
    // positive: monad_map takes an ordinary T -> U transform.
    D_TEST_STATIC(is_mappable< maybe_int, plain_double >::value);
    D_TEST_STATIC(is_mappable< maybe_int, DPlainToString >::value);
    // negative: a non-callable; a non-monad first arg.
    D_TEST_STATIC(!is_mappable< maybe_int, int >::value);
    D_TEST_STATIC(!is_mappable< not_a_monad, plain_double >::value);

    // ---- is_monad_combinator ---------------------------------------------
    using bind_combinator_t = decltype(::djinterp::bind_with(arrow_inc()));
    using map_combinator_t  = decltype(::djinterp::map_with(plain_double()));
    using then_combinator_t = decltype(
        ::djinterp::then_with(DTestMaybe<int>(0)));

    D_TEST_STATIC(is_monad_combinator< bind_combinator_t, maybe_int >::value);
    D_TEST_STATIC(is_monad_combinator< map_combinator_t, maybe_int >::value);
    D_TEST_STATIC(is_monad_combinator< then_combinator_t, maybe_int >::value);
    // negative: a plain type has no apply(monad) member.
    D_TEST_STATIC(!is_monad_combinator< not_a_monad, maybe_int >::value);
    D_TEST_STATIC(!is_monad_combinator< int, maybe_int >::value);

    // ---- variable-template `_v` aliases (C++14+) -------------------------
#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    static_assert(is_monad_v<maybe_int>,
                  "is_monad_v should hold for the test monad");
    static_assert(!is_monad_v<not_a_monad>,
                  "is_monad_v negative case");
    static_assert(is_monadic_function_v<arrow_inc, maybe_int>,
                  "is_monadic_function_v positive case");
    static_assert(!is_monadic_function_v<plain_double, maybe_int>,
                  "is_monadic_function_v negative case");
    static_assert(is_bindable_v<maybe_int, arrow_inc>,
                  "is_bindable_v positive case");
    static_assert(!is_bindable_v<maybe_int, int>,
                  "is_bindable_v negative case");
    static_assert(is_mappable_v<maybe_int, plain_double>,
                  "is_mappable_v positive case");
    static_assert(is_monad_combinator_v<bind_combinator_t, maybe_int>,
                  "is_monad_combinator_v positive case");
    static_assert(!is_monad_combinator_v<not_a_monad, maybe_int>,
                  "is_monad_combinator_v negative case");
#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    // ---- concept mirrors (C++20) -----------------------------------------
#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    D_TEST_STATIC(monad< maybe_int >);
    D_TEST_STATIC(!monad< not_a_monad >);
    D_TEST_STATIC(monadic_function_for< arrow_inc, maybe_int >);
    D_TEST_STATIC(!monadic_function_for< plain_double, maybe_int >);
    D_TEST_STATIC(bindable_with< maybe_int, arrow_inc >);
    D_TEST_STATIC(!bindable_with< maybe_int, int >);
    D_TEST_STATIC(mappable_with< maybe_int, plain_double >);
    D_TEST_STATIC(monad_combinator_for< bind_combinator_t, maybe_int >);
    D_TEST_STATIC(!monad_combinator_for< not_a_monad, maybe_int >);
#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS

}  // namespace


/*
monad_tests_traits
  Exercises monad.hpp Section 0 (predicate SFINAE traits + concepts).
  Tests the following:
  - monad_value_type_t: extracts the inner type of the monad
  - monad_rebind_t: re-parameterizes the monad over a new inner type
  - is_monadic_function: recognizes Kleisli arrows T -> M<U>; rejects
    plain transforms, non-callables, and non-monad targets
  - is_bindable: monad_bind(M, F) well-formedness, positive and negative
  - is_mappable: monad_map(M, F) well-formedness, positive and negative
  - is_monad_combinator: the apply(monad) shape of all three combinators
  - `_v` variable-template aliases agree with their trait structs (C++14+)
  - concept mirrors track their traits (C++20)
  All validated at compile time via static_assert; the single recorded
  assertion below is the runtime roll-up.  If this translation unit
  compiled, every compile-time property held.
*/
bool monad_tests_traits()
{
    bool verdict = true;

    DUnitTestTally tally;

    run_unit_test(verdict,
        tally,
        "monad traits + concepts (compile-time suite)",
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
