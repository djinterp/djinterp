/******************************************************************************
* djinterp [test]                                      monad_tests_traits.cpp
*
*   Tests for monad.hpp Section 0: the predicate SFINAE structural
* traits and the C++20 concept mirrors.  Almost all coverage here is
* compile-time: each property reduces to a static_assert, so a
* regression halts the build with a descriptive message rather than
* failing silently at runtime.  The section function records a single
* runtime roll-up assertion so the report carries a visible row for the
* compile-time suite.
******************************************************************************/
#include "./monad_tests.hpp"


NS_DJINTERP
NS_TESTING

using ::djinterp::test::test_handler;
using ::djinterp::test::unit_test_tally;
using ::djinterp::test::run_unit_test;
using ::djinterp::test::record_assertion;
using ::djinterp::test::type_equal;


namespace {

    using maybe_int = test_maybe<int>;
    using maybe_str = test_maybe<std::string>;

    // ---- monad_value_type_t ----------------------------------------------
    D_TEST_TYPE_EQ(monad_value_type_t<maybe_int>, int);
    D_TEST_TYPE_EQ(monad_value_type_t<maybe_str>, std::string);

    // ---- monad_rebind_t ---------------------------------------------------
    D_TEST_TYPE_EQ(monad_rebind_t<maybe_int, double>,      test_maybe<double>);
    D_TEST_TYPE_EQ(monad_rebind_t<maybe_int, std::string>, maybe_str);

    // ---- is_monadic_function ---------------------------------------------
    // positive: a Kleisli arrow int -> test_maybe<U> over maybe_int.
    D_TEST_TRAIT_TRUE(is_monadic_function, arrow_inc,       maybe_int);
    D_TEST_TRAIT_TRUE(is_monadic_function, arrow_to_string, maybe_int);
    D_TEST_TRAIT_TRUE(is_monadic_function, arrow_to_none,   maybe_int);
    // negative: a plain transform returns a non-monad; a non-callable
    // is not an arrow; a non-monad second arg is rejected.
    D_TEST_TRAIT_FALSE(is_monadic_function, plain_double,    maybe_int);
    D_TEST_TRAIT_FALSE(is_monadic_function, int,             maybe_int);
    D_TEST_TRAIT_FALSE(is_monadic_function, arrow_inc,       not_a_monad);

    // ---- is_bindable ------------------------------------------------------
    // positive: monad_bind(maybe_int, arrow) is well-formed.
    D_TEST_TRAIT_TRUE (is_bindable, maybe_int, arrow_inc);
    D_TEST_TRAIT_TRUE (is_bindable, maybe_int, arrow_to_string);
    // negative: a non-function second arg; a non-monad first arg.
    D_TEST_TRAIT_FALSE(is_bindable, maybe_int, int);
    D_TEST_TRAIT_FALSE(is_bindable, not_a_monad, arrow_inc);

    // ---- is_mappable ------------------------------------------------------
    // positive: monad_map takes an ordinary T -> U transform.
    D_TEST_TRAIT_TRUE (is_mappable, maybe_int, plain_double);
    D_TEST_TRAIT_TRUE (is_mappable, maybe_int, plain_to_string);
    // negative: a non-callable; a non-monad first arg.
    D_TEST_TRAIT_FALSE(is_mappable, maybe_int, int);
    D_TEST_TRAIT_FALSE(is_mappable, not_a_monad, plain_double);

    // ---- is_monad_combinator ---------------------------------------------
    using bind_combinator_t = decltype(::djinterp::bind_with(arrow_inc()));
    using map_combinator_t  = decltype(::djinterp::map_with(plain_double()));
    using then_combinator_t = decltype(
        ::djinterp::then_with(test_maybe<int>(0)));

    D_TEST_TRAIT_TRUE (is_monad_combinator, bind_combinator_t, maybe_int);
    D_TEST_TRAIT_TRUE (is_monad_combinator, map_combinator_t,  maybe_int);
    D_TEST_TRAIT_TRUE (is_monad_combinator, then_combinator_t, maybe_int);
    // negative: a plain type has no apply(monad) member.
    D_TEST_TRAIT_FALSE(is_monad_combinator, not_a_monad,       maybe_int);
    D_TEST_TRAIT_FALSE(is_monad_combinator, int,               maybe_int);

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
    D_TEST_CONCEPT_TRUE (monad, maybe_int);
    D_TEST_CONCEPT_FALSE(monad, not_a_monad);
    D_TEST_CONCEPT_TRUE (monadic_function_for, arrow_inc, maybe_int);
    D_TEST_CONCEPT_FALSE(monadic_function_for, plain_double, maybe_int);
    D_TEST_CONCEPT_TRUE (bindable_with, maybe_int, arrow_inc);
    D_TEST_CONCEPT_FALSE(bindable_with, maybe_int, int);
    D_TEST_CONCEPT_TRUE (mappable_with, maybe_int, plain_double);
    D_TEST_CONCEPT_TRUE (monad_combinator_for, bind_combinator_t, maybe_int);
    D_TEST_CONCEPT_FALSE(monad_combinator_for, not_a_monad, maybe_int);
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
void
monad_tests_traits(
    test_handler& _handler
)
{
    unit_test_tally tally;

    run_unit_test(
        _handler,
        tally,
        "monad traits + concepts (compile-time suite)",
        [&]()
        {
            record_assertion(
                _handler,
                true,
                "Section 0 predicate traits and concepts compiled",
                "all static_assert checks held",
                nullptr);
        });

    return;
}


NS_END  // testing
NS_END  // djinterp
