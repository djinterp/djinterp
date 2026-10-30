/*******************************************************************************
* djinterp [parse]                               arg_parser_cpp_tests_runner.cpp
*
* Entry point for the argument parser's C++ suites.
*   Built as C++ and linked against the parser's library, built as C at the
* C floor, across a real language boundary: the suites call the C++ face,
* and the face calls the C library. A suite is a table of sections its own
* source defines, and this walks each table in turn: which sections a
* language level, exceptions or a configuration knob leave a suite is
* decided where the sections are, so the count printed here differs by
* level and by configuration, by design. Each result is flushed as it is
* printed, so that a section that crashes -- under a sanitizer, say -- is
* the one after the last result shown.
*
*
* path:      /build/cmake/config/testing/djinterp/c/modules/parse/cli/arg_parser/arg_parser_cpp_tests_runner.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.20
*                                                            revised: 2026.10.20
*******************************************************************************/
// std
#include <cstddef>  // std::size_t
#include <cstdio>   // std::printf, std::fflush
// djinterp
#include "../../../../../../../../../../tests/djinterp/cpp/modules/parse/cli/arg_parser/arg_argv_tests.hpp"  // arguments from text
#include "../../../../../../../../../../tests/djinterp/cpp/modules/parse/cli/arg_parser/arg_boundary_tests.hpp"  // the C boundary
#include "../../../../../../../../../../tests/djinterp/cpp/modules/parse/cli/arg_parser/arg_command_tests.hpp"  // commands
#include "../../../../../../../../../../tests/djinterp/cpp/modules/parse/cli/arg_parser/arg_common_tests.hpp"  // the vocabulary
#include "../../../../../../../../../../tests/djinterp/cpp/modules/parse/cli/arg_parser/arg_completion_tests.hpp"  // shell completion
#include "../../../../../../../../../../tests/djinterp/cpp/modules/parse/cli/arg_parser/arg_config_tests.hpp"  // configuration files
#include "../../../../../../../../../../tests/djinterp/cpp/modules/parse/cli/arg_parser/arg_examples_tests.hpp"  // CLI11's examples
#include "../../../../../../../../../../tests/djinterp/cpp/modules/parse/cli/arg_parser/arg_formatter_tests.hpp"  // help
#include "../../../../../../../../../../tests/djinterp/cpp/modules/parse/cli/arg_parser/arg_option_tests.hpp"  // options
#include "../../../../../../../../../../tests/djinterp/cpp/modules/parse/cli/arg_parser/arg_parser_tests.hpp"  // the parser itself
#include "../../../../../../../../../../tests/djinterp/cpp/modules/parse/cli/arg_parser/arg_type_tests.hpp"  // value types
#include "../../../../../../../../../../tests/djinterp/cpp/modules/parse/cli/arg_parser/arg_validator_tests.hpp"  // validators


namespace
{

// suite
//   struct: one suite -- its table, and where its length is kept. The length
// is another translation unit's constant, so it is reached by address.
struct suite
{
    const dj_tests_section* sections;
    const std::size_t*     count;
};

// SUITES
//   constant: every C++ suite this build has, in the order they run: each
// part of the face after the parts it builds on, then the functions the C
// library calls back, then CLI11's examples end to end. A suite of a part a
// knob leaves out, or below its language level -- the examples are C++11 --
// is not compiled, and is not here.
const suite SUITES[] =
{
    { d_tests_arg_cpp_common,     &d_tests_arg_cpp_common_count     },
    { d_tests_arg_cpp_type,       &d_tests_arg_cpp_type_count       },
    { d_tests_arg_cpp_validator,  &d_tests_arg_cpp_validator_count  },
    { d_tests_arg_cpp_option,     &d_tests_arg_cpp_option_count     },
    { d_tests_arg_cpp_command,    &d_tests_arg_cpp_command_count    },
    { d_tests_arg_cpp_formatter,  &d_tests_arg_cpp_formatter_count  },
    { d_tests_arg_cpp_config,     &d_tests_arg_cpp_config_count     },
    { d_tests_arg_cpp_argv,       &d_tests_arg_cpp_argv_count       },
    { d_tests_arg_cpp_parser,     &d_tests_arg_cpp_parser_count     },
#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)
    { d_tests_arg_cpp_completion, &d_tests_arg_cpp_completion_count },
#endif
    { d_tests_arg_cpp_boundary,   &d_tests_arg_cpp_boundary_count   },
#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    { d_tests_arg_cpp_examples,   &d_tests_arg_cpp_examples_count   }
#endif
};

}  // namespace

/*
main
  Runs every section of every C++ suite, reporting each as it finishes.
*/
int main()
{
    int passed = 0;
    int failed = 0;

    std::printf("djinterp argument parser: C++ tests\n");

    // each suite
    for (std::size_t s = 0u; s < (sizeof(SUITES) / sizeof(SUITES[0])); ++s)
    {
        // each section of the suite
        for (std::size_t i = 0u; i < *SUITES[s].count; ++i)
        {
            const dj_tests_section& section = SUITES[s].sections[i];
            const bool             ok      = section.run();

            std::printf("  [%s] %s\n",
                        ok ? "PASS" : "FAIL",
                        section.name);
            (void)std::fflush(stdout);

            // tally for the summary line
            if (ok)
            {
                ++passed;
            }
            else
            {
                ++failed;
            }
        }
    }

    std::printf("passed: %d   failed: %d\n",
                passed,
                failed);

    return (failed == 0) ? 0 : 1;
}
