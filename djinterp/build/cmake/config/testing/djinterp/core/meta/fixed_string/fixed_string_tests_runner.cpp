// std
#include <cstdio>
// djinterp
#include "fixed_string_tests.hpp"

namespace
{
    int g_passed = 0;
    int g_failed = 0;

    void run(const char* _name, bool (*_test)())
    {
        const bool ok = _test();
        std::printf("  [%s] %s\n", ok ? "PASS" : "FAIL", _name);
        if (ok) { ++g_passed; } else { ++g_failed; }
    }

    void section(const char* _title)
    {
        std::printf("\n--- %s ---\n", _title);
    }
}

// D_FS_RUN
//   macro: runs one suite function by name.  The FS letters are unique to
// this suite so co-compiled suites never collide on the macro name.
#define D_FS_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)

int main()
{
    std::printf("fixed_string.hpp unit tests\n");

    section("I.   type shape");
    D_FS_RUN(tests_fixed_string_layout);
    D_FS_RUN(tests_fixed_string_structural_requirements);
    D_FS_RUN(tests_fixed_string_distinctness);

    section("II.  construction and deduction");
    D_FS_RUN(tests_fixed_string_construction);
    D_FS_RUN(tests_fixed_string_deduction_guide);
    D_FS_RUN(tests_fixed_string_embedded_nulls);

    section("III. accessors");
    D_FS_RUN(tests_fixed_string_c_str);
    D_FS_RUN(tests_fixed_string_size);
    D_FS_RUN(tests_fixed_string_view);

    section("IV.  use as a class-type NTTP");
    D_FS_RUN(tests_fixed_string_as_nttp);
    D_FS_RUN(tests_fixed_string_argument_equivalence);
    D_FS_RUN(tests_fixed_string_nttp_round_trip);

    std::printf("\npassed: %d   failed: %d\n", g_passed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}

#undef D_FS_RUN
