/*******************************************************************************
* djinterp [parsegen]                                foundation_tests_runner.cpp
*
* Entry point for the parse/parsegen foundation's C++ suites.
*   A suite is a table of sections its own source defines; this walks each
* table in turn and reports. It carries no condition of its own: which sections
* a language level or a configuration has is decided where the sections are,
* and what arrives here is whatever this build has. The count it prints
* therefore differs by level and by configuration, by design -- see
* run_foundation_tests.sh for the count each level expects.
*   The suites run substrate first, so the first failure reported is the most
* fundamental one.
*
*
* path:      /build/cmake/config/testing/djinterp/parsegen/foundation/foundation_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.19
*                                                            revised: 2026.10.04
*******************************************************************************/
// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf
// djinterp
#include "../../../../../../../tests/djinterp/parse/parse_substrate_tests.hpp"       // diagnostics, machine, interop
#include "../../../../../../../tests/djinterp/parse/parse_program_tests.hpp"         // charset, pool, program
#include "../../../../../../../tests/djinterp/parsegen/parsegen_registry_tests.hpp"  // features, registry, storage
#include "../../../../../../../tests/djinterp/parsegen/parsegen_grammar_tests.hpp"   // the neutral grammar
#include "../../../../../../../tests/djinterp/parsegen/parsegen_analysis_tests.hpp"  // analysis and routing
#if D_ENV_LANG_IS_CPP11_OR_HIGHER  // the floor the suites have


namespace
{

// suite
//   struct: one suite -- its table, and where its length is kept. The length
// is another translation unit's constant, so it is reached by address.
struct suite
{
    const d_tests_section* sections;
    const std::size_t*     count;
};

// SUITES
//   constant: every suite, the substrate before the generator built on it.
const suite SUITES[] =
{
    { d_tests_parse_substrate,   &d_tests_parse_substrate_count   },
    { d_tests_parse_program,     &d_tests_parse_program_count     },
    { d_tests_parsegen_registry, &d_tests_parsegen_registry_count },
    { d_tests_parsegen_grammar,  &d_tests_parsegen_grammar_count  },
    { d_tests_parsegen_analysis, &d_tests_parsegen_analysis_count }
};

}  // namespace

/*
main
  Runs every section of every suite, reporting each as it finishes so that a
crash still shows which one it was in.
*/
int
main(void)
{
    int passed = 0;
    int failed = 0;

    std::printf("djinterp parse/parsegen foundation: C++ tests\n");

    for (const suite& each : SUITES)
    {
        for (std::size_t index = 0u; index < *each.count; index++)
        {
            const d_tests_section& section = each.sections[index];
            const bool             ok      = section.run();

            std::printf("  [%s] %s\n", ok ? "PASS" : "FAIL", section.name);

            // tally for the summary line
            if (ok)
            {
                passed++;
            }
            else
            {
                failed++;
            }
        }
    }

    std::printf("passed: %d   failed: %d\n", passed, failed);

    return (failed == 0) ? 0 : 1;
}

#endif  // floor, for now
