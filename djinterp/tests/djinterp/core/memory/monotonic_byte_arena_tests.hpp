/*******************************************************************************
* djinterp [testing]                              monotonic_byte_arena_tests.hpp
*
* Unit tests for monotonic_byte_arena.hpp.  This is a spec-mode suite (see djinterp_testing.md):
* a two-faced header whose normal face (the section TUs) pulls in the code under
* test and the shared fixtures, and whose spec face (the runner) exposes the
* monotonic_byte_arena_spec() provider.  The bool tests_*() declarations are visible in both.
*
*
* path:      /tests/djinterp/core/memory/monotonic_byte_arena_tests.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.22
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_TESTING_MONOTONIC_BYTE_ARENA_TESTS_
#define DJINTERP_TESTING_MONOTONIC_BYTE_ARENA_TESTS_ 1

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


// D_BA_CHECK: evaluate once; on failure print and early-return false.
#define D_BA_CHECK(...)                                                       \
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
// arena
bool tests_align_up_helper();
bool tests_monotonic_arena_contract();
bool tests_monotonic_arena_runtime();
bool tests_dynamic_arena_contract();
bool tests_dynamic_arena_runtime();
bool tests_byte_arena_recognition();


// -- spec provider (spec mode) ----------------------------------------------
#ifdef DTEST_SPEC_MODE
inline dt::module_spec
monotonic_byte_arena_spec()
{
    return dt::module_spec{
        "monotonic_byte_arena",
        "The byte arenas: align_up, the static monotonic_byte_arena and the "
        "heap-backed dynamic_byte_arena - descriptive surface, aligned "
        "overflow-safe bump allocation, and recognition.",
        {
            dt::block_spec{
                "arena",
                "The byte arenas (static and heap-backed) and align_up.",
                {
                    { "tests_align_up_helper",
                      "internal::align_up rounds up to the next power-of-two "
                      "multiple",
                      &tests_align_up_helper },
                    { "tests_monotonic_arena_contract",
                      "monotonic_byte_arena's descriptive surface and properties",
                      &tests_monotonic_arena_contract },
                    { "tests_monotonic_arena_runtime",
                      "aligned, overflow-safe bounded bump allocation",
                      &tests_monotonic_arena_runtime },
                    { "tests_dynamic_arena_contract",
                      "dynamic_byte_arena's descriptive surface and properties",
                      &tests_dynamic_arena_contract },
                    { "tests_dynamic_arena_runtime",
                      "aligned, overflow-safe bump allocation over a heap block",
                      &tests_dynamic_arena_runtime },
                    { "tests_byte_arena_recognition",
                      "byte_arena_strategy_c recognizes a monotonic byte strategy",
                      &tests_byte_arena_recognition },
                }
            },
        }
    };
}
#endif  // DTEST_SPEC_MODE


NS_END  // testing
NS_END  // djinterp


#endif  // DJINTERP_TESTING_MONOTONIC_BYTE_ARENA_TESTS_
