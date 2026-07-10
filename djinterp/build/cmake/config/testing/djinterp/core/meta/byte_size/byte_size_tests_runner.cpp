/******************************************************************************
* djinterp [test]                                    byte_size_tests_runner.cpp
*
*   Standalone runner (main) for the byte_size.hpp suite.  It includes the test
* header and drives every tests_byte_size_* predicate the section translation
* units define (flat in djinterp::testing).  Linking this runner with those
* sections yields the `byte_size_tests` executable.
*
*   static_byte_size and the storage/shape signals are compile-time and are
* additionally enforced by the static_assert wall in each section .cpp; the
* footprint FUNCTIONS are run-time, so each predicate re-derives the expected
* bytes from the same public surface and returns the fold.  One PASS/FAIL line
* per group; process exit code 0 iff all groups pass.
*
*   Placement / build follow AGENTS-cmake.md: this runner lives in the config
* tree next to its CMakeLists.txt, the sections live under tests/, and the
* leaf's INCLUDES carry both inc/djinterp/core/container and
* tests/djinterp/core/container.
*
* path:      /build/cmake/config/testing/djinterp/core/container/byte_size_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.03
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "byte_size_tests.hpp"


namespace
{
    int g_passed = 0;
    int g_failed = 0;

    void
    run(
        const char* _name,
        bool      (*_test)()
    )
    {
        const bool ok = _test();
        std::printf("  [%s] %s\n", ok ? "PASS" : "FAIL", _name);
        if (ok) { ++g_passed; } else { ++g_failed; }
    }
}


// unique letters per suite (D_BZ_ = byte-size) so co-compiled suites never
// collide on the macro name.
#define D_BZ_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    std::printf("byte_size.hpp unit tests\n");

    // ---- I.  static_byte_size ----
    D_BZ_RUN(tests_byte_size_static_exact);
    D_BZ_RUN(tests_byte_size_static_cleans);
    D_BZ_RUN(tests_byte_size_static_v);

    // ---- II. storage-shape signals ----
    D_BZ_RUN(tests_byte_size_signal_capacity);
    D_BZ_RUN(tests_byte_size_signal_size);
    D_BZ_RUN(tests_byte_size_signal_allocator);
    D_BZ_RUN(tests_byte_size_signal_reserve);
    D_BZ_RUN(tests_byte_size_signal_c_str);
    D_BZ_RUN(tests_byte_size_signal_dynamically_sited);
    D_BZ_RUN(tests_byte_size_signal_recurse_into_element);
    D_BZ_RUN(tests_byte_size_signal_cleans);

    // ---- III. shape + own dynamic footprint ----
    D_BZ_RUN(tests_byte_size_shape_of);
    D_BZ_RUN(tests_byte_size_own_contiguous);
    D_BZ_RUN(tests_byte_size_own_node_size);
    D_BZ_RUN(tests_byte_size_own_node_distance);
    D_BZ_RUN(tests_byte_size_own_static);

    // ---- IV. recursive dynamic descent ----
    D_BZ_RUN(tests_byte_size_descent_leaf_element);
    D_BZ_RUN(tests_byte_size_descent_nested);
    D_BZ_RUN(tests_byte_size_descent_c_str_frontier);
    D_BZ_RUN(tests_byte_size_descent_static_elements);
    D_BZ_RUN(tests_byte_size_descent_mixed);
    D_BZ_RUN(tests_byte_size_descent_empty);

    // ---- V. public footprint functions ----
    D_BZ_RUN(tests_byte_size_total_identity);
    D_BZ_RUN(tests_byte_size_total_static_sited);
    D_BZ_RUN(tests_byte_size_total_const_input);
    D_BZ_RUN(tests_byte_size_integration);

    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}

#undef D_BZ_RUN
