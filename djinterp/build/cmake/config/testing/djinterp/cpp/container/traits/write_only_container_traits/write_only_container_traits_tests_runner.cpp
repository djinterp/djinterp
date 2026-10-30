/******************************************************************************
* djinterp [testing]              write_only_container_traits_tests_runner.cpp
*
*   Entry point for the write_only_container_traits.hpp unit suite.
* Calls every test function in the order the test header declares its
* sections, prints a per-test verdict, and reports a non-zero exit status
* if any test failed - so CTest reads the result without any further
* reporting machinery.
*
*   The tests themselves live in the section TUs under
* tests/djinterp/cpp/container/traits; this file holds only the driver, per
* the config-tree convention that a runner sits beside the CMakeLists that
* builds it.
*
*   The header under test carries no numbered sections - it is a single
* predicate - so the banners below name the two groups of claims the suite is
* organised around rather than any division in the source.
*
* path:      /build/cmake/config/testing/djinterp/cpp/container/traits/write_only_container_traits/write_only_container_traits_tests_runner.cpp
*                                write_only_container_traits_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.20
******************************************************************************/

// std
#include <cstdio>
// djinterp
#include "write_only_container_traits_tests.hpp"


namespace
{
    int g_passed = 0;
    int g_failed = 0;

    // run
    //   function: invokes one test function and records its verdict.
    void run(
        const char* _name,
        bool (*_test)()
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
    void section(
        const char* _title
    )
    {
        std::printf("\n%s\n", _title);

        return;
    }

}  // namespace

// D_WO_RUN
//   macro: names a test once, as both the label and the callee, so the two
// cannot drift apart.
#define D_WO_RUN(_fn)  run(#_fn, &::djinterp::testing::_fn)


int main()
{
    std::printf("write_only_container_traits.hpp - unit suite\n");

    section("A. The predicate");
    D_WO_RUN(tests_is_write_only_container);
    D_WO_RUN(tests_is_write_only_structural);
    D_WO_RUN(tests_is_write_only_tagged);
    D_WO_RUN(tests_is_write_only_std_types);
    D_WO_RUN(tests_is_write_only_cv_ref);
    D_WO_RUN(tests_is_write_only_companions);

    section("B. Delegation, metadata, lattice and orthogonality");
    D_WO_RUN(tests_write_only_delegates);
    D_WO_RUN(tests_write_only_metadata_exclusion);
    D_WO_RUN(tests_write_only_lattice_exclusivity);
    D_WO_RUN(tests_write_only_is_not_complement);
    D_WO_RUN(tests_write_only_orthogonal_to_mutability);
    D_WO_RUN(tests_write_only_tag_agreement);

    std::printf("\npassed: %d   failed: %d\n", g_passed, g_failed);

    return (g_failed == 0) ? 0 : 1;
}

#undef D_WO_RUN
