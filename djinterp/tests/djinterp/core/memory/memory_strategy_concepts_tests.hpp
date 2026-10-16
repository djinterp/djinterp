/*******************************************************************************
* djinterp [testing]                          memory_strategy_concepts_tests.hpp
*
* Unit tests for memory_strategy_concepts.hpp.  This is a spec-mode suite (see djinterp_testing.md):
* a two-faced header whose normal face (the section TUs) pulls in the code under
* test and the shared fixtures, and whose spec face (the runner) exposes the
* memory_strategy_concepts_spec() provider.  The bool tests_*() declarations are visible in both.
*
*
* path:      /tests/djinterp/core/memory/memory_strategy_concepts_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTING_MEMORY_STRATEGY_CONCEPTS_TESTS_
#define DJINTERP_TESTING_MEMORY_STRATEGY_CONCEPTS_TESTS_ 1

// std
#include <cstdio>
// djinterp
// -- mode-gated includes ----------------------------------------------------
#include "djinterp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "memory_strategy_test_fixtures.hpp"
#endif  // !DTEST_SPEC_MODE
#ifdef DTEST_SPEC_MODE
#include "djinterp/test/test_defaults.hpp"
#endif  // DTEST_SPEC_MODE

// forward-declare ::djinterp::test so the dt alias is valid in the normal face.
NS_DJINTERP
NS_TEST
NS_END  // test
NS_END  // djinterp


NS_DJINTERP
NS_TESTING

namespace dt = ::djinterp::test;


// D_MC_CHECK: evaluate once; on failure print and early-return false.
#define D_MC_CHECK(...)                                                       \
    do                                                                        \
    {                                                                         \
        if (!(__VA_ARGS__))                                                   \
        {                                                                     \
            std::printf("    check failed: %s\n    at: %s:%d\n",             \
                        #__VA_ARGS__, __FILE__, __LINE__);                    \
            return false;                                                     \
        }                                                                     \
    }                                                                         \
    while (false)


// -- test declarations (both faces) -----------------------------------------
// concepts
bool tests_concepts_membership();
bool tests_concepts_typing();
bool tests_concepts_storage_discipline();
bool tests_concepts_stability_release();
bool tests_concepts_constraint_dispatch();


// -- spec provider (spec mode) ----------------------------------------------
#ifdef DTEST_SPEC_MODE
inline dt::module_spec
memory_strategy_concepts_spec()
{
    return dt::module_spec{
        "memory_strategy_concepts",
        "The C++20 concept layer over the memory-strategy traits: the "
        "membership, typing, discipline, and refinement concepts, their "
        "agreement with the traits, and their behaviour as constraints.",
        {
            dt::block_spec{
                "concepts",
                "The C++20 concept layer over the traits.",
                {
                    { "tests_concepts_membership",
                      "the membership / allocating concepts agree with the "
                      "traits",
                      &tests_concepts_membership },
                    { "tests_concepts_typing",
                      "the element / byte typing concepts agree with the traits",
                      &tests_concepts_typing },
                    { "tests_concepts_storage_discipline",
                      "the static / fixed / dynamic concepts agree with the kind",
                      &tests_concepts_storage_discipline },
                    { "tests_concepts_stability_release",
                      "the stability / release concepts gate on membership and "
                      "allocation, diverging from the raw traits",
                      &tests_concepts_stability_release },
                    { "tests_concepts_constraint_dispatch",
                      "the concepts drive overload resolution and subsumption",
                      &tests_concepts_constraint_dispatch },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_MEMORY_STRATEGY_CONCEPTS_TESTS_
