/*******************************************************************************
* djinterp [tests]                              test_threadsafe_tests_traits.cpp
*
*   Section TU: the folded threadsafe-testable trait and concept surface
* arrives through the umbrella.
*
*   This is the fold regression guard.  test_thread_traits.hpp and
* test_thread_concepts.hpp were folded into test_thread.hpp and the umbrella's
* two #include lines for them were removed; if the fold is ever undone, or the
* traits move namespace, or test_thread.hpp is dropped from the umbrella, this
* TU stops compiling.
*
*   Behavioural coverage of the traits lives in the test_thread suite - here
* only reachability and a single positive/negative pair are asserted, so the
* two suites do not duplicate each other.
*
*
* path:      /tests/djinterp/test/sync/test_threadsafe_tests_traits.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                                created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/

// djinterp
#include "test_threadsafe_tests.hpp"


NS_DJINTERP
NS_TESTING

namespace tr = ::djinterp::test::traits;


bool tests_utraits_namespace_reachable()
{
    // naming anything in the traits namespace proves it resolved through the
    // umbrella without the retired standalone headers
    D_HU_CHECK(tr::has_concurrency_strategy_tag<hu_probe>::value);

    return true;
}

bool tests_utraits_positive_and_negative()
{
    D_HU_CHECK(tr::has_concurrency_strategy_tag<hu_probe>::value);
    D_HU_CHECK(!tr::has_concurrency_strategy_tag<hu_plain>::value);

    D_HU_CHECK(tr::is_threadsafe_testable<hu_probe>::value);
    D_HU_CHECK(!tr::is_threadsafe_testable<hu_plain>::value);

    return true;
}

bool tests_utraits_composites_reachable()
{
    // the composites and the summary class template came through too
    D_HU_CHECK(!tr::has_atomic_interface<hu_plain>::value);
    D_HU_CHECK(!tr::is_lock_free_testable<hu_plain>::value);
    D_HU_CHECK(!tr::is_locked_testable<hu_plain>::value);

    typedef tr::thread_test_class<hu_probe> probe_class;
    typedef tr::thread_test_class<hu_plain> plain_class;

    D_HU_CHECK(probe_class::has_strategy_tag);
    D_HU_CHECK(probe_class::is_threadsafe);
    D_HU_CHECK(!plain_class::is_threadsafe);

    return true;
}

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

bool tests_utraits_concepts_reachable()
{
    D_HU_CHECK(tr::strategy_tagged_type<hu_probe>);
    D_HU_CHECK(!tr::strategy_tagged_type<hu_plain>);

    D_HU_CHECK(tr::threadsafe_testable_type<hu_probe>);
    D_HU_CHECK(!tr::threadsafe_testable_type<hu_plain>);

    // the concept layer and the trait layer must agree
    D_HU_CHECK(tr::strategy_tagged_type<hu_probe> ==
               tr::has_concurrency_strategy_tag<hu_probe>::value);
    D_HU_CHECK(tr::threadsafe_testable_type<hu_plain> ==
               tr::is_threadsafe_testable<hu_plain>::value);

    return true;
}

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // testing
NS_END  // djinterp
