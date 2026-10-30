/*******************************************************************************
* djinterp [parse]                                   arg_parser_c_tests_runner.c
*
* Entry point for the argument parser's C suites.
*   Built as C and linked against the parser's library, built as C at the C
* floor. A suite is a table of sections its own source defines, and this
* walks each table in turn: which sections a configuration has is decided
* where the sections are, so the count printed here differs by
* configuration, by design. Each result is flushed as it is printed, so
* that a section that crashes -- under a sanitizer, say -- is the one after
* the last result shown.
*
*
* path:      /build/cmake/config/testing/djinterp/c/modules/parse/cli/arg_parser/arg_parser_c_tests_runner.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.13
*                                                            revised: 2026.10.17
*******************************************************************************/
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <stdio.h>    // printf, fflush
// djinterp
#include "../../../../../../../../../../tests/djinterp/c/modules/parse/cli/arg_parser/arg_argv_tests.h"  // arguments from text
#include "../../../../../../../../../../tests/djinterp/c/modules/parse/cli/arg_parser/arg_command_tests.h"  // commands
#include "../../../../../../../../../../tests/djinterp/c/modules/parse/cli/arg_parser/arg_common_tests.h"  // the vocabulary
#include "../../../../../../../../../../tests/djinterp/c/modules/parse/cli/arg_parser/arg_completion_tests.h"  // shell completion
#include "../../../../../../../../../../tests/djinterp/c/modules/parse/cli/arg_parser/arg_config_tests.h"  // configuration files
#include "../../../../../../../../../../tests/djinterp/c/modules/parse/cli/arg_parser/arg_formatter_tests.h"  // help
#include "../../../../../../../../../../tests/djinterp/c/modules/parse/cli/arg_parser/arg_option_tests.h"  // options
#include "../../../../../../../../../../tests/djinterp/c/modules/parse/cli/arg_parser/arg_parse_tests.h"  // the parse
#include "../../../../../../../../../../tests/djinterp/c/modules/parse/cli/arg_parser/arg_parser_tests.h"  // the parser itself
#include "../../../../../../../../../../tests/djinterp/c/modules/parse/cli/arg_parser/arg_process_tests.h"  // its processing
#include "../../../../../../../../../../tests/djinterp/c/modules/parse/cli/arg_parser/arg_smoke_tests.h"  // the C API end to end
#include "../../../../../../../../../../tests/djinterp/c/modules/parse/cli/arg_parser/arg_suggest_tests.h"  // suggestions
#include "../../../../../../../../../../tests/djinterp/c/modules/parse/cli/arg_parser/arg_type_tests.h"  // value types
#include "../../../../../../../../../../tests/djinterp/c/modules/parse/cli/arg_parser/arg_validator_extra_tests.h"  // extra checks
#include "../../../../../../../../../../tests/djinterp/c/modules/parse/cli/arg_parser/arg_validator_member_tests.h"  // sets and mappings
#include "../../../../../../../../../../tests/djinterp/c/modules/parse/cli/arg_parser/arg_validator_path_tests.h"  // the file system
#include "../../../../../../../../../../tests/djinterp/c/modules/parse/cli/arg_parser/arg_validator_tests.h"  // validators
#include "../../../../../../../../../../tests/djinterp/c/modules/parse/cli/arg_parser/arg_validator_unit_tests.h"  // numbers with units


// dj_tests_arg_c_suite
//   struct: one suite -- its table, and where its length is kept. The length
// is another translation unit's constant, so it is reached by address.
struct dj_tests_arg_c_suite
{
    const struct dj_tests_section* sections;
    const size_t*                 count;
};

// SUITES
//   constant: every C suite this build has, in the order they run: each
// module's after the modules it builds on, then the C API end to end. A
// suite of a part a knob leaves out is not compiled, and is not here.
static const struct dj_tests_arg_c_suite SUITES[] =
{
    { d_tests_arg_common,     &d_tests_arg_common_count     },
    { d_tests_arg_type,       &d_tests_arg_type_count       },
    { d_tests_arg_validator,  &d_tests_arg_validator_count  },
#if (D_INTERNAL_ARG_PARSER_EXTRA_VALIDATORS == 1)
    { d_tests_arg_extra,      &d_tests_arg_extra_count      },
    { d_tests_arg_member,     &d_tests_arg_member_count     },
    { d_tests_arg_unit,       &d_tests_arg_unit_count       },
#endif
#if (D_INTERNAL_ARG_PARSER_PATH_VALIDATORS == 1)
    { d_tests_arg_path,       &d_tests_arg_path_count       },
#endif
    { d_tests_arg_option,     &d_tests_arg_option_count     },
    { d_tests_arg_command,    &d_tests_arg_command_count    },
    { d_tests_arg_formatter,  &d_tests_arg_formatter_count  },
    { d_tests_arg_parsing,    &d_tests_arg_parsing_count    },
    { d_tests_arg_process,    &d_tests_arg_process_count    },
    { d_tests_arg_config,     &d_tests_arg_config_count     },
    { d_tests_arg_text,       &d_tests_arg_text_count       },
    { d_tests_arg_parser,     &d_tests_arg_parser_count     },
    { d_tests_arg_suggest,    &d_tests_arg_suggest_count    },
#if (D_INTERNAL_ARG_PARSER_COMPLETION == 1)
    { d_tests_arg_completion, &d_tests_arg_completion_count },
#endif
    { d_tests_arg_smoke,      &d_tests_arg_smoke_count      }
};

/*
main
  Runs every section of every C suite, reporting each as it finishes.
*/
int main(void)
{
    int passed = 0;
    int failed = 0;

    printf("djinterp argument parser: C tests\n");

    // each suite
    for (size_t s = 0u; s < (sizeof(SUITES) / sizeof(SUITES[0])); s++)
    {
        // each section of the suite
        for (size_t i = 0u; i < *SUITES[s].count; i++)
        {
            const struct dj_tests_section* const section =
                &SUITES[s].sections[i];
            const bool                          ok      = section->run();

            printf("  [%s] %s\n",
                   ok ? "PASS" : "FAIL",
                   section->name);
            (void)fflush(stdout);

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

    printf("passed: %d   failed: %d\n",
           passed,
           failed);

    return (failed == 0) ? 0 : 1;
}
