/******************************************************************************
* djinterp [testing]                 runtime_container_traits_tests_runner.cpp
*
*   Entry point for the runtime_container_traits.hpp unit suite.
* Calls every test function in the order the test header declares its
* sections, prints a per-test verdict, and reports a non-zero exit status
* if any test failed - so CTest reads the result without any further
* reporting machinery.
*
*   The tests themselves live in the section TUs under
* tests/djinterp/core/container/traits; this file holds only the driver, per
* the config-tree convention that a runner sits beside the CMakeLists that
* builds it.
*
*   The header divides into three numbered sections and the sections below
* follow them: the structural probes, the two classification umbrellas built
* on those probes, and the aggregate that restates both.
*
* path:      /build/cmake/config/testing/djinterp/core/container/traits/
*                                   runtime_container_traits_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.20
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "runtime_container_traits_tests.hpp"


namespace
{
    int g_passed = 0;
    int g_failed = 0;

    // run
    //   function: invokes one test function and records its verdict.
    void
    run(
        const char* _name,
        bool      (*_test)()
    )
    {
        const bool ok = _test();

        std::printf("  [%s] %s\n", ok ? "PASS" : "FAIL", _name);

        if (ok)
        {
            ++g_passed;
        }
        else
        {
            ++g_failed;
        }

        return;
    }

    // section
    //   function: prints a section banner.
    void
    section(
        const char* _title
    )
    {
        std::printf("\n%s\n", _title);

        return;
    }

}  // namespace

// D_RT_RUN
//   macro: names a test once, as both the label and the callee, so the two
// cannot drift apart.
#define D_RT_RUN(_fn)  run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    std::printf("runtime_container_traits.hpp - unit suite\n");

    section("I.   Structural signals");
    D_RT_RUN(tests_has_size_accessor_signal);
    D_RT_RUN(tests_has_allocator_alias);
    D_RT_RUN(tests_has_reserve_method_signal);
    D_RT_RUN(tests_signals_are_independent);
    D_RT_RUN(tests_signals_std_types);
    D_RT_RUN(tests_signals_cv_ref);
    D_RT_RUN(tests_signals_companions);

    section("II.  Classification umbrellas");
    D_RT_RUN(tests_is_runtime_container);
    D_RT_RUN(tests_is_runtime_container_needs_shape);
    D_RT_RUN(tests_is_runtime_container_needs_runtime_only);
    D_RT_RUN(tests_runtime_only_is_exact_equality);
    D_RT_RUN(tests_requires_runtime_storage);
    D_RT_RUN(tests_requires_runtime_storage_is_independent);
    D_RT_RUN(tests_classification_std_types);
    D_RT_RUN(tests_classification_cv_ref);
    D_RT_RUN(tests_classification_companions);

    section("III. Aggregate snapshot");
    D_RT_RUN(tests_runtime_container_class_signals);
    D_RT_RUN(tests_runtime_container_class_verdicts);
    D_RT_RUN(tests_runtime_container_class_life);
    D_RT_RUN(tests_runtime_container_class_agreement);
    D_RT_RUN(tests_runtime_container_class_cv_ref);

    std::printf("\npassed: %d   failed: %d\n", g_passed, g_failed);

    return (g_failed == 0) ? 0 : 1;
}

#undef D_RT_RUN
