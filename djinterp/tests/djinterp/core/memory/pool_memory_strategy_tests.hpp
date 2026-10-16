/*******************************************************************************
* djinterp [testing]                              pool_memory_strategy_tests.hpp
*
* Unit tests for pool_memory_strategy.hpp.  This is a spec-mode suite (see djinterp_testing.md):
* a two-faced header whose normal face (the section TUs) pulls in the code under
* test and the shared fixtures, and whose spec face (the runner) exposes the
* pool_memory_strategy_spec() provider.  The bool tests_*() declarations are visible in both.
*
*
* path:      /tests/djinterp/core/memory/pool_memory_strategy_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTING_POOL_MEMORY_STRATEGY_TESTS_
#define DJINTERP_TESTING_POOL_MEMORY_STRATEGY_TESTS_ 1

// std
#include <cstdio>
// djinterp
// -- mode-gated includes ----------------------------------------------------
#include "djinterp/djinterp.hpp"
#ifndef DTEST_SPEC_MODE
#include "memory_strategy_test_fixtures.hpp"
#include "../../../../inc/djinterp/core/memory/pool_memory_strategy.hpp"
#include "../../../../inc/djinterp/core/memory/pool/pool.hpp"
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


// D_STRATEGY_CHECK: evaluate once; on failure print and early-return false.
#define D_STRATEGY_CHECK(...)                                                       \
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
// pool-backed strategy - OPT-IN, gated on D_MS_TEST_POOL_LAYER; the section
// TU compiles empty and the provider block is gated on the same macro.
#if defined(D_MS_TEST_POOL_LAYER)
// pool
bool tests_pool_strategy_contract();
bool tests_pool_strategy_runtime();
bool tests_pool_strategy_recognition();
bool tests_pool_strategy_factory();
#endif  // D_MS_TEST_POOL_LAYER


// -- spec provider (spec mode) ----------------------------------------------
#ifdef DTEST_SPEC_MODE
inline dt::module_spec
pool_memory_strategy_spec()
{
    return dt::module_spec{
        "pool_memory_strategy",
        "The pool-backed element strategy: the always-dynamic surface it "
        "presents over a pool_resource, the acquire/release mapping with "
        "an operator-new fallback, recognition, and the deducing factory.",
        {
#if defined(D_MS_TEST_POOL_LAYER)
            dt::block_spec{
                "pool",
                "The pool-backed element strategy (opt-in).",
                {
                    { "tests_pool_strategy_contract",
                      "pool_memory_strategy presents an always-dynamic element "
                      "surface and forwards the pool's policy constants",
                      &tests_pool_strategy_contract },
                    { "tests_pool_strategy_runtime",
                      "allocate(1)/deallocate(p,1) map to the pool's "
                      "acquire/release, with an operator-new fallback for n>1",
                      &tests_pool_strategy_runtime },
                    { "tests_pool_strategy_recognition",
                      "is_pool_memory_strategy / _c recognize the pool strategy",
                      &tests_pool_strategy_recognition },
                    { "tests_pool_strategy_factory",
                      "make_pool_strategy deduces the pool type and binds a "
                      "strategy to it",
                      &tests_pool_strategy_factory },
                }
            },
#endif  // D_MS_TEST_POOL_LAYER
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_POOL_MEMORY_STRATEGY_TESTS_
