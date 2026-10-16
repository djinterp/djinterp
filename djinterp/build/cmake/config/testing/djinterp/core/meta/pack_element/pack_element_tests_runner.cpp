// std
#include <cstdio>
// djinterp
#include "pack_element_tests.hpp"

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

// D_PE_RUN
//   macro: runs one suite function by name.  The PE letters are unique to
// this suite so co-compiled suites never collide on the macro name.
#define D_PE_RUN(_fn)   run(#_fn, &::djinterp::testing::_fn)

int main()
{
    std::printf("pack_element.hpp unit tests\n");

    section("I.   indexing");
    D_PE_RUN(tests_pack_element_base_case);
    D_PE_RUN(tests_pack_element_recursion);
    D_PE_RUN(tests_pack_element_trait_shape);

    section("II.  the pack_element_t alias");
    D_PE_RUN(tests_pack_element_alias_equivalence);
    D_PE_RUN(tests_pack_element_alias_arity);

    section("III. element fidelity");
    D_PE_RUN(tests_pack_element_type_preservation);
    D_PE_RUN(tests_pack_element_hostile_pack);
    D_PE_RUN(tests_pack_element_duplicates);

    section("IV.  bounds and the guard");
    D_PE_RUN(tests_pack_element_arity_contract);
    D_PE_RUN(tests_pack_element_guarded_access);
    D_PE_RUN(tests_pack_element_depth);

    std::printf("\npassed: %d   failed: %d\n", g_passed, g_failed);
    return (g_failed == 0) ? 0 : 1;
}

#undef D_PE_RUN
