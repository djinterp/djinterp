/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Test runner for event_table_common module standalone tests.
*   Tests hash functions (FNV-1a and simple), prime utilities,
*   hash node management, statistics printing, and integration scenarios.
*
*
* path:      /.config/.msvs/testing/event/djinterp-c-event-table-common-tests-sa/main.c
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/
#include "../../../../../../inc/c/test/test_standalone.h"
#include "../../../../../../tests/c/event/event_table_common_tests_sa.h"


/******************************************************************************
 * IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_etc_status_items[] =
{
    { "[INFO]", "d_event_hash_function (FNV-1a) produces bounded, "
                "deterministic, well-distributed hashes" },
    { "[INFO]", "d_event_hash_simple provides alternative modulo-based "
                "hashing" },
    { "[INFO]", "d_event_hash_next_prime correctly finds next prime >= "
                "input for all tested values" },
    { "[INFO]", "d_event_hash_node_new creates nodes with correct key, "
                "value, and NULL next" },
    { "[INFO]", "d_event_hash_node_free frees the node without freeing "
                "the listener" },
    { "[INFO]", "Node chaining via next pointers works correctly" },
    { "[INFO]", "d_event_hash_table_print_stats handles NULL stats and "
                "NULL label safely" },
    { "[INFO]", "Constant definitions verified (DEFAULT_SIZE, "
                "LOAD_FACTOR, RESIZE_FACTOR)" },
    { "[INFO]", "Statistics structure layout and member accessibility "
                "confirmed" }
};

static const struct d_test_sa_note_item g_etc_issues_items[] =
{
    { "[WARN]", "d_event_hash_simple performs (_key >> 32) which is "
                "undefined behavior when d_event_id is 32-bit int" },
    { "[WARN]", "Original node tests used "
                "d_event_listener_new_default(id, NULL) which always "
                "returns NULL — fixed in updated tests" },
    { "[NOTE]", "d_event_hash_function and d_event_hash_simple are "
                "marked D_INLINE; ensure visibility in test compilation "
                "units" }
};

static const struct d_test_sa_note_item g_etc_steps_items[] =
{
    { "[TODO]", "Fix d_event_hash_simple to not shift by >= type width" },
    { "[TODO]", "Add tests for d_event_hash_function with sizeof("
                "d_event_id) != 4 configurations" },
    { "[TODO]", "Add resize-trigger tests once full hash table is "
                "available" },
    { "[TODO]", "Add thread-safety tests for concurrent node insertion" },
    { "[TODO]", "Profile hash distribution with real-world event ID "
                "patterns" }
};

static const struct d_test_sa_note_item g_etc_guidelines_items[] =
{
    { "[BEST]", "Always use d_event_hash_node_free (not raw free) for "
                "nodes" },
    { "[BEST]", "Always free listeners separately from nodes — nodes "
                "do not own their values" },
    { "[BEST]", "Use d_event_hash_next_prime for table sizing to "
                "minimize clustering" },
    { "[BEST]", "Use d_event_hash_function (FNV-1a) for production; "
                "d_event_hash_simple for debug/comparison only" },
    { "[BEST]", "Always pass a real fn_callback to "
                "d_event_listener_new_default — NULL returns NULL" }
};

static const struct d_test_sa_note_section g_etc_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_etc_status_items) / sizeof(g_etc_status_items[0]),
      g_etc_status_items },
    { "KNOWN ISSUES",
      sizeof(g_etc_issues_items) / sizeof(g_etc_issues_items[0]),
      g_etc_issues_items },
    { "NEXT STEPS",
      sizeof(g_etc_steps_items) / sizeof(g_etc_steps_items[0]),
      g_etc_steps_items },
    { "BEST PRACTICES",
      sizeof(g_etc_guidelines_items) / sizeof(g_etc_guidelines_items[0]),
      g_etc_guidelines_items }
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
                          "djinterp event_table_common Module",
                          "Comprehensive Testing of Hash Functions, "
                          "Prime Utilities, Node Management, "
                          "Statistics, and Integration Scenarios");

    // register the event_table_common module
    d_test_sa_runner_add_module_counter(&runner,
                                        "event_table_common",
                                        "d_event_hash_function, "
                                        "d_event_hash_simple, "
                                        "d_event_hash_next_prime, "
                                        "d_event_hash_node_new/free, "
                                        "d_event_hash_table_print_stats",
                                        d_tests_sa_event_table_common_all,
                                        sizeof(g_etc_notes) /
                                            sizeof(g_etc_notes[0]),
                                        g_etc_notes);

    // execute all tests and return result
    return d_test_sa_runner_execute(&runner);
}