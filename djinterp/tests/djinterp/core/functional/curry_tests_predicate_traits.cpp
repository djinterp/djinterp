// djinterp [test] -- curry.hpp Section II (predicate traits & concepts)
#include "./curry_tests.hpp"

#include <type_traits>


NS_DJINTERP
NS_TESTING


// ---- compile-time guarantees ----
//   The traits are compile-time facts, so anchor them with static_asserts at
// file scope first; the section function then funnels the same predicates
// through the handler so they also appear as runtime leaves in the report.
// This double-checks the *_v shorthands and (under C++20) the concept
// parallels resolve identically to the ::value form.

static_assert(is_predicate<is_positive, int>::value,
              "unary bool callable is a predicate");
static_assert(is_predicate<less_than, int, int>::value,
              "binary bool callable is a predicate");
static_assert(is_predicate<always_true>::value,
              "nullary bool callable is a predicate");
static_assert(is_predicate<echo_int, int>::value,
              "int result is convertible to bool -> predicate");
static_assert(is_predicate<returns_pointer, int>::value,
              "pointer result is convertible to bool -> predicate");
static_assert(!is_predicate<returns_void, int>::value,
              "void result is not convertible to bool -> not a predicate");
static_assert(!is_predicate<int, int>::value,
              "non-invocable type is not a predicate");

static_assert(is_nullary_predicate<always_true>::value, "");
static_assert(!is_nullary_predicate<is_positive>::value, "");

static_assert(is_unary_predicate<is_positive, int>::value, "");
static_assert(!is_unary_predicate<less_than, int>::value, "");

static_assert(is_binary_predicate<less_than, int, int>::value, "");
static_assert(!is_binary_predicate<is_positive, int, int>::value, "");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
static_assert(is_predicate_v<is_positive, int>, "");
static_assert(is_nullary_predicate_v<always_true>, "");
static_assert(is_unary_predicate_v<is_positive, int>, "");
static_assert(is_binary_predicate_v<less_than, int, int>, "");
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
static_assert(predicate_for<is_positive, int>, "");
static_assert(predicate_for<less_than, int, int>, "");
static_assert(nullary_predicate<always_true>, "");
static_assert(unary_predicate<is_positive, int>, "");
static_assert(binary_predicate<less_than, int, int>, "");
static_assert(!unary_predicate<less_than, int>, "");
static_assert(!predicate_for<returns_void, int>, "");
#endif


/*
test_predicate_traits
  Exercises the predicate classification traits and their C++20 concept
  parallels.
  Tests the following:
  - is_predicate is true for callables whose result is convertible to bool
    (bool, int, and pointer results) and false for void results
  - is_predicate is false for non-invocable types
  - is_nullary / is_unary / is_binary_predicate enforce exact arity
  - a predicate of the wrong arity is rejected by the arity-specific traits
  - the framework primitive `never` is a predicate of every arity
  - the *_v variable-template shorthands agree with the ::value form
  - the concept parallels (C++20) agree with the structural traits
*/
void
test_predicate_traits(
    test::test_handler& _h
)
{
    // ---- is_predicate: positive cases ----
    test::record_assertion(_h, (is_predicate<is_positive, int>::value),
                           "is_predicate: unary bool callable");
    test::record_assertion(_h, (is_predicate<less_than, int, int>::value),
                           "is_predicate: binary bool callable");
    test::record_assertion(_h, (is_predicate<always_true>::value),
                           "is_predicate: nullary bool callable");
    test::record_assertion(_h, (is_predicate<echo_int, int>::value),
                           "is_predicate: int result convertible to bool");
    test::record_assertion(_h, (is_predicate<returns_pointer, int>::value),
                           "is_predicate: pointer result convertible to bool");

    // ---- is_predicate: negative cases ----
    test::record_assertion(_h, (!is_predicate<returns_void, int>::value),
                           "is_predicate: void result rejected");
    test::record_assertion(_h, (!is_predicate<int, int>::value),
                           "is_predicate: non-invocable rejected");

    // ---- arity-specific traits ----
    test::record_assertion(_h, (is_nullary_predicate<always_true>::value),
                           "is_nullary_predicate: arity-0 accepted");
    test::record_assertion(_h, (!is_nullary_predicate<is_positive>::value),
                           "is_nullary_predicate: arity-1 rejected");

    test::record_assertion(_h, (is_unary_predicate<is_positive, int>::value),
                           "is_unary_predicate: arity-1 accepted");
    test::record_assertion(_h, (!is_unary_predicate<less_than, int>::value),
                           "is_unary_predicate: binary rejected at arity-1");

    test::record_assertion(_h,
                           (is_binary_predicate<less_than, int, int>::value),
                           "is_binary_predicate: arity-2 accepted");
    test::record_assertion(_h,
                           (!is_binary_predicate<is_positive, int, int>::value),
                           "is_binary_predicate: unary rejected at arity-2");

    // ---- the `never` primitive is a predicate of any arity ----
    test::record_assertion(_h, (is_predicate<decltype(never)>::value),
                           "never is a nullary predicate");
    test::record_assertion(_h,
                           (is_predicate<decltype(never), int, double>::value),
                           "never is a predicate of arbitrary arity");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    test::record_assertion(_h, (is_predicate_v<is_positive, int>),
                           "is_predicate_v agrees with ::value");
    test::record_assertion(_h, (is_unary_predicate_v<is_positive, int>),
                           "is_unary_predicate_v agrees with ::value");
    test::record_assertion(_h, (is_binary_predicate_v<less_than, int, int>),
                           "is_binary_predicate_v agrees with ::value");
    test::record_assertion(_h, (is_nullary_predicate_v<always_true>),
                           "is_nullary_predicate_v agrees with ::value");
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    test::record_assertion(_h, (predicate_for<is_positive, int>),
                           "predicate_for concept: unary");
    test::record_assertion(_h, (binary_predicate<less_than, int, int>),
                           "binary_predicate concept: arity-2");
    test::record_assertion(_h, (nullary_predicate<always_true>),
                           "nullary_predicate concept: arity-0");
    test::record_assertion(_h, (!unary_predicate<less_than, int>),
                           "unary_predicate concept: binary rejected");
#endif

    return;
}


NS_END  // testing
NS_END  // djinterp
