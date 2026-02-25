/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Test runner for event_table module standalone tests.
*   Tests hash table creation/destruction, CRUD operations,
*   table management, iterator traversal, statistics, and
*   integration/stress scenarios.
*
*
* path:      /.config/.msvs/testing/event/djinterp-c-event-table-tests-sa/main.c
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/
#include "../../../../../../inc/c/test/test_standalone.h"
#include "../../../../../../tests/c/event/event_table_tests_sa.h"


/******************************************************************************
 * IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_et_status_items[] =
{
    { "[INFO]", "d_event_hash_table_new rounds size up to "
                "nearest prime via d_event_hash_next_prime" },
    { "[INFO]", "d_event_hash_table_new(0) falls back to "
                "D_EVENT_HASH_TABLE_DEFAULT_SIZE (101)" },
    { "[INFO]", "insert detects duplicate keys and updates "
                "in-place with correct enabled_count tracking" },
    { "[INFO]", "insert triggers automatic resize when "
                "load factor exceeds D_EVENT_HASH_TABLE_LOAD_FACTOR" },
    { "[INFO]", "remove unlinks from chain, updates count "
                "and enabled_count" },
    { "[INFO]", "resize rehashes all nodes into new bucket "
                "array, preserving all elements" },
    { "[INFO]", "clear frees nodes but NOT listeners — "
                "callers must free listeners separately" },
    { "[INFO]", "iterator traverses all buckets and chains "
                "via bucket_index + current_node advancement" },
    { "[INFO]", "get_stats computes used_buckets, max_chain_length, "
                "and average_chain_length via single pass" }
};

static const struct d_test_sa_note_item g_et_issues_items[] =
{
    { "[WARN]", "d_event_hash_table_clear frees nodes but not "
                "listeners — original tests had a memory leak in "
                "the clear test (now fixed)" },
    { "[WARN]", "d_event_hash_table_remove dereferences "
                "current->value->enabled — will crash if value is "
                "NULL (no NULL value guard)" },
    { "[NOTE]", "d_event_hash_table_insert does not take "
                "ownership of listeners — table stores pointer "
                "only" },
    { "[NOTE]", "d_event_hash_table_free calls clear then "
                "frees buckets and table — listeners survive" }
};

static const struct d_test_sa_note_item g_et_steps_items[] =
{
    { "[TODO]", "Add NULL value guard in remove to prevent "
                "crash on node with NULL listener" },
    { "[TODO]", "Add tests for iterator stability during "
                "concurrent modification" },
    { "[TODO]", "Add tests for resize shrinking (new_size < "
                "current size)" },
    { "[TODO]", "Add tests verifying load_factor calculation "
                "matches count/size ratio exactly" },
    { "[TODO]", "Add tests for d_event_hash_table_insert "
                "when malloc fails (node creation failure)" }
};

static const struct d_test_sa_note_item g_et_guidelines_items[] =
{
    { "[BEST]", "Always free listeners separately after "
                "d_event_hash_table_free or clear" },
    { "[BEST]", "Use d_event_hash_table_new_default for "
                "typical use; use new(size) only when table "
                "size is known in advance" },
    { "[BEST]", "Check return value of insert — false means "
                "NULL table, NULL listener, or allocation failure" },
    { "[BEST]", "Use contains for existence checks rather "
                "than comparing lookup result to NULL" },
    { "[BEST]", "Always free iterator via "
                "d_event_hash_table_iterator_free after use" }
};

static const struct d_test_sa_note_section g_et_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_et_status_items) / sizeof(g_et_status_items[0]),
      g_et_status_items },
    { "KNOWN ISSUES",
      sizeof(g_et_issues_items) / sizeof(g_et_issues_items[0]),
      g_et_issues_items },
    { "NEXT STEPS",
      sizeof(g_et_steps_items) / sizeof(g_et_steps_items[0]),
      g_et_steps_items },
    { "BEST PRACTICES",
      sizeof(g_et_guidelines_items) / sizeof(g_et_guidelines_items[0]),
      g_et_guidelines_items }
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
                          "djinterp event_table Module",
                          "Comprehensive Testing of Hash Table "
                          "Creation, CRUD Operations, Management, "
                          "Iterator, Statistics, and Edge Cases");

    // register the event_table module
    d_test_sa_runner_add_module_counter(&runner,
                                        "event_table",
                                        "d_event_hash_table_new, "
                                        "new_default, free, insert, "
                                        "lookup, remove, contains, "
                                        "size, count, enabled_count, "
                                        "load_factor, resize, clear, "
                                        "iterator, get_stats",
                                        d_tests_sa_event_hash_table_all,
                                        sizeof(g_et_notes) /
                                            sizeof(g_et_notes[0]),
                                        g_et_notes);

    // execute all tests and return result
    return d_test_sa_runner_execute(&runner);
}