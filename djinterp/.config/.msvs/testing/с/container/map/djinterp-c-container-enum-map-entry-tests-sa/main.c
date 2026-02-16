/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Test runner for enum_map_entry module standalone tests.
*   Tests the d_enum_map_entry type and associated macros.
*
*
* path:      \.config\.msvs\testing\container\djinterp-c-enum-map-entry-tests-sa\main.c
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/
#include "..\..\..\..\..\..\inc\c\test\test_standalone.h"
#include "..\..\..\..\..\..\tests\c\container\map\enum_map_entry_tests_sa.h"


/******************************************************************************
 * IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_enum_map_entry_status_items[] =
{
    { "[INFO]", "All entry creation macros validated"                },
    { "[INFO]", "Entry comparison macros working correctly"          },
    { "[INFO]", "Sentinel marker macros functioning as designed"     },
    { "[INFO]", "Typed entry macros (STR, INT, NULL, SELF) tested"   },
    { "[INFO]", "Legacy D_ENUM_KEY_ENTRY alias confirmed compatible" }
};

static const struct d_test_sa_note_item g_enum_map_entry_notes_items[] =
{
    { "[NOTE]", "This module only tests entry macros, not map operations" },
    { "[NOTE]", "Map operations tested separately in min_enum_map tests"  },
    { "[NOTE]", "INT_MIN is reserved as the sentinel key value"           },
    { "[NOTE]", "D_ENUM_ENTRY_INT only safe for intptr_t-sized integers"  }
};

static const struct d_test_sa_note_item g_enum_map_entry_guidelines_items[] =
{
    { "[BEST]", "Use D_ENUM_ENTRY_STR for string literal values"         },
    { "[BEST]", "Use D_ENUM_ENTRY_INT for small integer values"          },
    { "[BEST]", "Use D_ENUM_ENTRY_NULL for explicit NULL values"         },
    { "[BEST]", "Use D_ENUM_ENTRY_SELF for identity mappings"            },
    { "[BEST]", "Avoid using INT_MIN as a key (conflicts with sentinel)" },
    { "[BEST]", "Use D_ENUM_ENTRY_SENTINEL to terminate static arrays"   }
};

static const struct d_test_sa_note_section g_enum_map_entry_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_enum_map_entry_status_items) / sizeof(g_enum_map_entry_status_items[0]),
      g_enum_map_entry_status_items },
    { "IMPORTANT NOTES",
      sizeof(g_enum_map_entry_notes_items) / sizeof(g_enum_map_entry_notes_items[0]),
      g_enum_map_entry_notes_items },
    { "BEST PRACTICES",
      sizeof(g_enum_map_entry_guidelines_items) / sizeof(g_enum_map_entry_guidelines_items[0]),
      g_enum_map_entry_guidelines_items }
};


/******************************************************************************
 * MAIN ENTRY POINT
 *****************************************************************************/

int
main
(
    int    _argc,
    char** _argv
)
{
    struct d_test_sa_runner runner;

    // suppress unused parameter warnings
    (void)_argc;
    (void)_argv;

    // initialize the test runner
    d_test_sa_runner_init(&runner,
                          "djinterp enum_map_entry Module",
                          "Comprehensive Testing of d_enum_map_entry Structure "
                          "and Associated Macros");

    // register the enum_map_entry module
    d_test_sa_runner_add_module(&runner,
                                "enum_map_entry",
                                "d_enum_map_entry structure, creation macros, "
                                "comparison macros, and sentinel markers",
                                d_tests_enum_map_entry_run_all,
                                sizeof(g_enum_map_entry_notes) /
                                    sizeof(g_enum_map_entry_notes[0]),
                                g_enum_map_entry_notes);

    // execute all tests and return result
    return d_test_sa_runner_execute(&runner);
}