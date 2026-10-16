/*******************************************************************************
* djinterp [testing]                         allocator_memory_strategy_tests.hpp
*
* Unit tests for allocator_memory_strategy.hpp.  This is a spec-mode suite (see djinterp_testing.md):
* a two-faced header whose normal face (the section TUs) pulls in the code under
* test and the shared fixtures, and whose spec face (the runner) exposes the
* allocator_memory_strategy_spec() provider.  The bool tests_*() declarations are visible in both.
*
*
* path:      /tests/djinterp/core/memory/allocator_memory_strategy_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTING_ALLOCATOR_MEMORY_STRATEGY_TESTS_
#define DJINTERP_TESTING_ALLOCATOR_MEMORY_STRATEGY_TESTS_ 1

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


// D_AL_CHECK: evaluate once; on failure print and early-return false.
#define D_AL_CHECK(...)                                                       \
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
// allocator
bool tests_allocator_strategy_contract();
bool tests_allocator_strategy_runtime();
bool tests_allocator_strategy_recognition();
bool tests_allocator_strategy_factory();


// -- spec provider (spec mode) ----------------------------------------------
#ifdef DTEST_SPEC_MODE
inline dt::module_spec
allocator_memory_strategy_spec()
{
    return dt::module_spec{
        "allocator_memory_strategy",
        "The std-allocator adapter: the dynamic descriptive surface it adds "
        "over a std::allocator, a real heap round-trip, recognition, and "
        "the deducing factory.",
        {
            dt::block_spec{
                "allocator",
                "The std-allocator adapter.",
                {
                    { "tests_allocator_strategy_contract",
                      "allocator_memory_strategy adds the dynamic surface over a "
                      "std allocator",
                      &tests_allocator_strategy_contract },
                    { "tests_allocator_strategy_runtime",
                      "a real heap allocate/deallocate round-trips",
                      &tests_allocator_strategy_runtime },
                    { "tests_allocator_strategy_recognition",
                      "is_allocator_memory_strategy / _c recognize the wrapper",
                      &tests_allocator_strategy_recognition },
                    { "tests_allocator_strategy_factory",
                      "make_allocator_strategy deduces the allocator and wraps it",
                      &tests_allocator_strategy_factory },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_ALLOCATOR_MEMORY_STRATEGY_TESTS_
