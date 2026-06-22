// std
#include <cstdio>
// djinterp
#include "test_common_tests.hpp"


namespace
{
    int g_passed = 0;
    int g_failed = 0;

    // run
    //   helper: invokes one test, prints a per-test verdict, and tallies it.
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
    }
}  // namespace


// D_TC_RUN
//   macro: runs ::djinterp::testing::<fn> under its own stringized name.
#define D_TC_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)


int
main()
{
    std::printf("test_common.hpp unit tests\n");
    std::printf("--------------------------\n");

    // I.   TEST TYPE IDENTIFICATION
    D_TC_RUN(tests_test_type_id);
    D_TC_RUN(tests_test_callable_id);
    D_TC_RUN(tests_k_no_callable);

    // II.  STATUS CLASSIFICATION
    D_TC_RUN(tests_test_status);

    // III. EVENT SYSTEM
    D_TC_RUN(tests_test_event_id);
    D_TC_RUN(tests_test_event);

    // IV.  CONSTEXPR SUPPORT MACROS
    D_TC_RUN(tests_d_test_constexpr);
    D_TC_RUN(tests_d_test_static_constexpr);

    std::printf("--------------------------\n");
    std::printf("passed: %d   failed: %d\n", g_passed, g_failed);

    return (g_failed == 0) ? 0 : 1;
}

#undef D_TC_RUN
