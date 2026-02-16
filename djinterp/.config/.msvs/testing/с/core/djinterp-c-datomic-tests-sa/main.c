/******************************************************************************
* djinterp [test]                                                       main.c
*
*   Test runner for datomic module standalone tests.
*   Tests the d_atomic types and associated atomic operations.
*
*
* path:      \.config\.msvs\testing\atomic\djinterp-c-datomic-tests-sa\main.c
* author(s): Samuel 'teer' Neal-Blim
******************************************************************************/
#include "..\..\..\..\..\inc\c\test\test_standalone.h"
#include "..\..\..\..\..\tests\c\datomic_tests_sa.h"


/******************************************************************************
 * IMPLEMENTATION NOTES
 *****************************************************************************/

static const struct d_test_sa_note_item g_datomic_status_items[] =
{
    { "[INFO]", "Atomic flag operations (test_and_set, clear) validated" },
    { "[INFO]", "Atomic initialization for all integer types and pointer" },
    { "[INFO]", "Load/store operations tested for all types" },
    { "[INFO]", "Exchange operations tested for all types" },
    { "[INFO]", "Compare-and-exchange (strong and weak) validated" },
    { "[INFO]", "Fetch-and-modify (add, sub, or, xor, and) tested" },
    { "[INFO]", "Explicit memory order variants validated" },
    { "[INFO]", "Memory fences (thread and signal) working correctly" },
    { "[INFO]", "Null-safety guards validated for all operations" }
};

static const struct d_test_sa_note_item g_datomic_issues_items[] =
{
    { "[NOTE]", "Three backends: C11 stdatomic, Windows Interlocked, GCC __sync" },
    { "[NOTE]", "Integer types stamped via D_ATOMIC_INTEGER_TYPES X-macro" },
    { "[NOTE]", "Pointer atomics lack arithmetic/bitwise operations by design" },
    { "[NOTE]", "Weak CAS may spuriously fail; tests retry up to 100 times" },
    { "[WARN]", "__sync backend ignores explicit memory order parameters" }
};

static const struct d_test_sa_note_item g_datomic_steps_items[] =
{
    { "[TODO]", "Add stress tests with higher iteration counts" },
    { "[TODO]", "Add memory order visibility tests across threads" },
    { "[TODO]", "Test Windows Interlocked backend on MSVC" },
    { "[TODO]", "Add performance benchmarks for contended atomics" },
    { "[TODO]", "Test 32-bit vs 64-bit width dispatch on Windows" }
};

static const struct d_test_sa_note_item g_datomic_guidelines_items[] =
{
    { "[BEST]", "Always initialize atomics before concurrent access" },
    { "[BEST]", "Use _explicit variants when relaxed ordering suffices" },
    { "[BEST]", "Prefer fetch_add over CAS loops for simple counters" },
    { "[BEST]", "Check d_atomic_is_lock_free_N for platform guarantees" },
    { "[BEST]", "Use atomic_flag for lightweight spinlocks only" }
};

static const struct d_test_sa_note_section g_datomic_notes[] =
{
    { "CURRENT STATUS",
      sizeof(g_datomic_status_items) / sizeof(g_datomic_status_items[0]),
      g_datomic_status_items },
    { "KNOWN ISSUES",
      sizeof(g_datomic_issues_items) / sizeof(g_datomic_issues_items[0]),
      g_datomic_issues_items },
    { "NEXT STEPS",
      sizeof(g_datomic_steps_items) / sizeof(g_datomic_steps_items[0]),
      g_datomic_steps_items },
    { "BEST PRACTICES",
      sizeof(g_datomic_guidelines_items) / sizeof(g_datomic_guidelines_items[0]),
      g_datomic_guidelines_items }
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
                          "djinterp datomic Module",
                          "Comprehensive Testing of d_atomic Types "
                          "and Cross-Platform Atomic Operations");

    // register the datomic module
    d_test_sa_runner_add_module_counter(&runner,
                                        "datomic",
                                        "d_atomic operations for "
                                        "initialization, load, store, "
                                        "exchange, compare-and-exchange, "
                                        "fetch-and-modify, flags, fences, "
                                        "and null-safety guards",
                                        d_tests_sa_atomic_run_all,
                                        ( sizeof(g_datomic_notes) /
                                            sizeof(g_datomic_notes[0]) ),
                                        g_datomic_notes);

    // execute all tests and return result
    return d_test_sa_runner_execute(&runner);
}