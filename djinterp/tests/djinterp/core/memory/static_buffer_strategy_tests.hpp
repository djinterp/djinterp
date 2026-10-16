/*******************************************************************************
* djinterp [testing]                            static_buffer_strategy_tests.hpp
*
* Unit tests for static_buffer_strategy.hpp.  This is a spec-mode suite (see djinterp_testing.md):
* a two-faced header whose normal face (the section TUs) pulls in the code under
* test and the shared fixtures, and whose spec face (the runner) exposes the
* static_buffer_strategy_spec() provider.  The bool tests_*() declarations are visible in both.
*
*
* path:      /tests/djinterp/core/memory/static_buffer_strategy_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTING_STATIC_BUFFER_STRATEGY_TESTS_
#define DJINTERP_TESTING_STATIC_BUFFER_STRATEGY_TESTS_ 1

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


// D_SB_CHECK: evaluate once; on failure print and early-return false.
#define D_SB_CHECK(...)                                                       \
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
// static_buffer
bool tests_static_buffer_contract();
bool tests_static_buffer_runtime();
bool tests_static_extent_contract();
bool tests_static_buffer_recognition();


// -- spec provider (spec mode) ----------------------------------------------
#ifdef DTEST_SPEC_MODE
inline dt::module_spec
static_buffer_strategy_spec()
{
    return dt::module_spec{
        "static_buffer_strategy",
        "The inline element buffer and the descriptive extent marker: the "
        "surface and properties, bump allocation with overflow-safe "
        "bounds and reset, the static_extent marker, and recognition.",
        {
            dt::block_spec{
                "static_buffer",
                "The inline element buffer and the descriptive extent marker.",
                {
                    { "tests_static_buffer_contract",
                      "static_buffer_strategy's surface and properties "
                      "(non-copyable, static, pointer-stable)",
                      &tests_static_buffer_contract },
                    { "tests_static_buffer_runtime",
                      "bump allocation, overflow-safe bounds, and reset",
                      &tests_static_buffer_runtime },
                    { "tests_static_extent_contract",
                      "static_extent_strategy is a descriptive-only static marker",
                      &tests_static_extent_contract },
                    { "tests_static_buffer_recognition",
                      "is_static_buffer_strategy / _c recognize the buffer",
                      &tests_static_buffer_recognition },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_STATIC_BUFFER_STRATEGY_TESTS_
