/*******************************************************************************
* djinterp [test]                                         maybe_tests_traits.cpp
*
* Predicate & structural trait tests for maybe.hpp section II:
*   is_maybe<T>, is_maybe_predicate<P, T>, the variable-template shorthands
*   (is_maybe_v / is_maybe_predicate_v), and the C++20 concept parallels
*   (maybe_type / maybe_predicate_for).
*
*   Compile-time guarantees are asserted at file scope via static_assert (the
* trait results are usable in constant expressions); the runtime section
* mirrors them so the counts roll into the report and so the gated
* variable-template / concept paths are exercised when available.
*
*
* path:      /tests/djinterp/cpp/functional/maybe_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.06.01
*                                                            revised: 2026.11.09
*******************************************************************************/

// std
#include <string>
#include <vector>
// djinterp
#include "./maybe_tests.hpp"


NS_DJINTERP
NS_TESTING


// ---- compile-time guarantees (struct form, always available) ----

// is_maybe: positive forms, including cv-/ref-qualified
static_assert(is_maybe<maybe<int>>::value,
    "is_maybe: maybe<int> is a maybe");
static_assert(is_maybe<maybe<std::string>>::value,
    "is_maybe: maybe<string> is a maybe");
static_assert(is_maybe<const maybe<int>&>::value,
    "is_maybe: cv/ref-qualified maybe is still a maybe");

// is_maybe: negative forms
static_assert(!is_maybe<int>::value,
    "is_maybe: int is not a maybe");
static_assert(!is_maybe<std::vector<int>>::value,
    "is_maybe: vector is not a maybe");
static_assert(!is_maybe<nothing_t>::value,
    "is_maybe: nothing_t is not a maybe");

// is_maybe_predicate: positive forms
static_assert(is_maybe_predicate<pred_is_even, int>::value,
    "is_maybe_predicate: even-test is a predicate over int");
static_assert(is_maybe_predicate<pred_is_positive, int>::value,
    "is_maybe_predicate: positive-test is a predicate over int");

// is_maybe_predicate: negative forms
static_assert(!is_maybe_predicate<DFnReturnsVoid, int>::value,
    "is_maybe_predicate: void result is not a predicate");
static_assert(!is_maybe_predicate<DFnStringOnly, int>::value,
    "is_maybe_predicate: not callable with int is not a predicate over int");
static_assert(!is_maybe_predicate<int, int>::value,
    "is_maybe_predicate: a non-callable is not a predicate");


#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
static_assert(is_maybe_v<maybe<int>>,
    "is_maybe_v: positive");
static_assert(!is_maybe_v<int>,
    "is_maybe_v: negative");
static_assert(is_maybe_predicate_v<pred_is_even, int>,
    "is_maybe_predicate_v: positive");
static_assert(!is_maybe_predicate_v<DFnReturnsVoid, int>,
    "is_maybe_predicate_v: negative");
#endif


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
static_assert(maybe_type<maybe<int>>,
    "maybe_type: positive");
static_assert(!maybe_type<int>,
    "maybe_type: negative");
static_assert(maybe_predicate_for<pred_is_even, int>,
    "maybe_predicate_for: positive");
static_assert(!maybe_predicate_for<DFnReturnsVoid, int>,
    "maybe_predicate_for: negative");
#endif


bool test_traits()
{
    bool verdict = true;

    // ---- is_maybe (structural) ----
    record_assertion(verdict, is_maybe<maybe<int>>::value,
        "trait: is_maybe true for maybe<int>");
    record_assertion(verdict, is_maybe<const maybe<double>&>::value,
        "trait: is_maybe strips cv/ref before matching");
    record_assertion(verdict, !is_maybe<int>::value,
        "trait: is_maybe false for a plain value type");
    record_assertion(verdict, !is_maybe<nothing_t>::value,
        "trait: is_maybe false for nothing_t");

    // ---- is_maybe_predicate (predicate SFINAE) ----
    record_assertion(verdict, is_maybe_predicate<pred_is_even, int>::value,
        "trait: is_maybe_predicate true for a bool(const int&) callable");
    record_assertion(verdict, !is_maybe_predicate<DFnReturnsVoid, int>::value,
        "trait: is_maybe_predicate false for a void-returning callable");
    record_assertion(verdict, !is_maybe_predicate<DFnStringOnly, int>::value,
        "trait: is_maybe_predicate false when not callable with int");
    record_assertion(verdict, !is_maybe_predicate<int, int>::value,
        "trait: is_maybe_predicate false for a non-callable");

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    record_assertion(verdict,
        is_maybe_v<maybe<int>> && !is_maybe_v<int>,
        "trait: is_maybe_v matches the struct form");
    record_assertion(verdict,
        is_maybe_predicate_v<pred_is_positive, int> &&
        !is_maybe_predicate_v<DFnReturnsVoid, int>,
        "trait: is_maybe_predicate_v matches the struct form");
#endif

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS
    record_assertion(verdict,
        maybe_type<maybe<int>> && !maybe_type<int>,
        "trait: maybe_type concept matches is_maybe");
    record_assertion(verdict,
        maybe_predicate_for<pred_is_even, int> &&
        !maybe_predicate_for<DFnStringOnly, int>,
        "trait: maybe_predicate_for concept matches is_maybe_predicate");
#endif

    return verdict;
}


NS_END  // testing
NS_END  // djinterp
