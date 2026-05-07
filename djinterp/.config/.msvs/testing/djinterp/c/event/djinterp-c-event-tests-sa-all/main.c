/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Combined test runner for the event subsystem standalone tests.
*   Runs all 4 modules:
*     1. event           - core types, constructors, compare, find, free
*     2. event_table_common - hash functions, primes, nodes, stats
*     3. event_table     - hash table CRUD, management, iterator, stats
*     4. event_handler   - handler lifecycle, listeners, fire/queue/process
*
*
* path:      /.config/.msvs/testing/event/djinterp-c-event-tests-sa-all/main.c
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/
#include "../../../../../../inc/c/test/test_standalone.h"
#include "../../../../../../tests/c/event/event_tests_sa.h"
#include "../../../../../../tests/c/event/event_table_common_tests_sa.h"
#include "../../../../../../tests/c/event/event_table_tests_sa.h"
#include "../../../../../../tests/c/event/event_handler_tests_sa.h"


/******************************************************************************
 * MODULE 1: event - IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_ev_status_items[] =
{
    { "[INFO]", "d_event struct uses single void* context - "
                "caller-owned, opaque to event system" },
    { "[INFO]", "d_event_free does NOT free context pointer - "
                "caller manages context lifetime" },
    { "[INFO]", "d_event_new_ctx stores pointer directly with "
                "no copy or allocation" },
    { "[INFO]", "fn_callback signature is void(*)(void*) - "
                "context passed directly to callback" },
    { "[INFO]", "d_event_listener_new rejects NULL callback" },
    { "[INFO]", "d_event_compare uses shallow pointer comparison "
                "on context when IDs match" },
    { "[INFO]", "d_event_compare_deep delegates to fn_comparator "
                "for context, falls back to shallow" },
    { "[INFO]", "d_event_listener_compare is ID-based only - "
                "ignores enabled flag and fn pointer" }
};

static const struct d_test_sa_note_item g_ev_issues_items[] =
{
    { "[NOTE]", "d_event_listener_find_index_of returns LAST "
                "match index - original code overwrites on every "
                "match; updated implementation returns FIRST" },
    { "[NOTE]", "d_event_compare_deep with NULL comparator and "
                "non-NULL context falls back to shallow pointer "
                "comparison" }
};

static const struct d_test_sa_note_section g_ev_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_ev_status_items) / sizeof(g_ev_status_items[0]),
      g_ev_status_items },
    { "KNOWN ISSUES",
      sizeof(g_ev_issues_items) / sizeof(g_ev_issues_items[0]),
      g_ev_issues_items }
};


/******************************************************************************
 * MODULE 2: event_table_common - IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_etc_status_items[] =
{
    { "[INFO]", "d_event_hash_function (FNV-1a) produces bounded, "
                "deterministic, well-distributed hashes" },
    { "[INFO]", "d_event_hash_next_prime correctly finds next "
                "prime >= input for all tested values" },
    { "[INFO]", "d_event_hash_node_free frees the node without "
                "freeing the listener" },
    { "[INFO]", "d_event_hash_table_print_stats handles NULL "
                "stats and NULL label safely" }
};

static const struct d_test_sa_note_item g_etc_issues_items[] =
{
    { "[WARN]", "d_event_hash_simple performs (_key >> 32) which "
                "is undefined behavior when d_event_id is 32-bit" },
    { "[NOTE]", "d_event_hash_function and d_event_hash_simple "
                "are D_INLINE - ensure visibility in test "
                "compilation units" }
};

static const struct d_test_sa_note_section g_etc_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_etc_status_items) / sizeof(g_etc_status_items[0]),
      g_etc_status_items },
    { "KNOWN ISSUES",
      sizeof(g_etc_issues_items) / sizeof(g_etc_issues_items[0]),
      g_etc_issues_items }
};


/******************************************************************************
 * MODULE 3: event_table - IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_et_status_items[] =
{
    { "[INFO]", "d_event_hash_table_new rounds size up to "
                "nearest prime via d_event_hash_next_prime" },
    { "[INFO]", "insert detects duplicate keys and updates "
                "in-place with correct enabled_count tracking" },
    { "[INFO]", "clear frees nodes but NOT listeners - "
                "callers must free listeners separately" },
    { "[INFO]", "iterator traverses all buckets and chains "
                "via bucket_index + current_node advancement" }
};

static const struct d_test_sa_note_item g_et_issues_items[] =
{
    { "[WARN]", "d_event_hash_table_remove dereferences "
                "current->value->enabled - will crash if value "
                "is NULL (no NULL value guard)" },
    { "[WARN]", "d_event_hash_table_clear frees nodes but not "
                "listeners - original tests had memory leak "
                "(now fixed)" }
};

static const struct d_test_sa_note_section g_et_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_et_status_items) / sizeof(g_et_status_items[0]),
      g_et_status_items },
    { "KNOWN ISSUES",
      sizeof(g_et_issues_items) / sizeof(g_et_issues_items[0]),
      g_et_issues_items }
};


/******************************************************************************
 * MODULE 4: event_handler - IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_eh_status_items[] =
{
    { "[INFO]", "d_event_handler_new creates d_circular_array "
                "event queue and d_event_hash_table listener "
                "table" },
    { "[INFO]", "fire_event passes _event->context directly to "
                "listener->fn (clean fn_callback match)" },
    { "[INFO]", "process_events pops from circular array and "
                "fires each, respecting max_events limit" },
    { "[INFO]", "handler does not own listeners - callers must "
                "free listeners separately" }
};

static const struct d_test_sa_note_item g_eh_issues_items[] =
{
    { "[WARN]", "enable_listener and disable_listener directly "
                "modify _handler->listeners->enabled_count - "
                "bypasses hash table encapsulation" },
    { "[NOTE]", "d_circular_array_pop returns pointer into "
                "internal buffer - data consumed before next "
                "push" }
};

static const struct d_test_sa_note_section g_eh_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_eh_status_items) / sizeof(g_eh_status_items[0]),
      g_eh_status_items },
    { "KNOWN ISSUES",
      sizeof(g_eh_issues_items) / sizeof(g_eh_issues_items[0]),
      g_eh_issues_items }
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

    (void)_argc;
    (void)_argv;

    d_test_sa_runner_init(&runner,
                          "djinterp Event Subsystem",
                          "Combined Testing of event, "
                          "event_table_common, event_table, "
                          "and event_handler Modules");

    // module 1: event
    d_test_sa_runner_add_module_counter(&runner,
                                        "event",
                                        "d_event_new, d_event_new_ctx, "
                                        "d_event_free, "
                                        "d_event_listener_new, "
                                        "d_event_listener_new_default, "
                                        "d_event_listener_free, "
                                        "d_event_compare, "
                                        "d_event_compare_deep, "
                                        "d_event_listener_compare, "
                                        "d_event_listener_find_index_of",
                                        d_tests_sa_event_run_all,
                                        sizeof(g_ev_notes) /
                                            sizeof(g_ev_notes[0]),
                                        g_ev_notes);

    // module 2: event_table_common
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

    // module 3: event_table
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

    // module 4: event_handler
    d_test_sa_runner_add_module_counter(&runner,
                                        "event_handler",
                                        "d_event_handler_new, free, "
                                        "bind, unbind, get_listener, "
                                        "enable/disable_listener, "
                                        "fire_event, queue_event, "
                                        "process_events, "
                                        "listener_count, enabled_count, "
                                        "pending_events",
                                        d_tests_sa_event_handler_all,
                                        sizeof(g_eh_notes) /
                                            sizeof(g_eh_notes[0]),
                                        g_eh_notes);

    return d_test_sa_runner_execute(&runner);
}