#include "./test_standalone_tests_sa.h"


/******************************************************************************
 * XIV. FAILURE TRACKING TESTS
 *****************************************************************************/

/*
d_tests_sa_standalone_failure_entry_struct
  Tests the d_test_sa_failure_entry structure.
  Tests the following:
  - Structure has expected members
  - module_name member is accessible
  - test_name member is accessible
  - message member is accessible
*/
bool
d_tests_sa_standalone_failure_entry_struct
(
    struct d_test_counter* _counter
)
{
    bool                           result;
    struct d_test_sa_failure_entry  entry;

    result = true;

    // test 1: module_name member is accessible
    entry.module_name = "test_module";

    result = d_assert_standalone(
        entry.module_name != NULL,
        "failure_entry_module_name",
        "module_name member should be accessible",
        _counter) && result;

    // test 2: test_name member is accessible
    entry.test_name = "test_assertion";

    result = d_assert_standalone(
        entry.test_name != NULL,
        "failure_entry_test_name",
        "test_name member should be accessible",
        _counter) && result;

    // test 3: message member is accessible
    entry.message = "Expected true, got false";

    result = d_assert_standalone(
        entry.message != NULL,
        "failure_entry_message",
        "message member should be accessible",
        _counter) && result;

    // test 4: members can be NULL
    entry.module_name = NULL;
    entry.test_name   = NULL;
    entry.message     = NULL;

    result = d_assert_standalone(
        (entry.module_name == NULL) &&
        (entry.test_name == NULL)   &&
        (entry.message == NULL),
        "failure_entry_nullable",
        "All members should be assignable to NULL",
        _counter) && result;

    // test 5: structure size is reasonable (three pointers)
    result = d_assert_standalone(
        sizeof(struct d_test_sa_failure_entry) >=
            (3 * sizeof(void*)),
        "failure_entry_size",
        "failure_entry should be at least 3 pointers",
        _counter) && result;

    return result;
}

/*
d_tests_sa_standalone_failure_list_struct
  Tests the d_test_sa_failure_list structure.
  Tests the following:
  - Structure has expected members
  - count member is accessible
  - capacity member is accessible
  - entries member is accessible
*/
bool
d_tests_sa_standalone_failure_list_struct
(
    struct d_test_counter* _counter
)
{
    bool                          result;
    struct d_test_sa_failure_list  list;

    result = true;

    // test 1: count member is accessible
    list.count = 0;

    result = d_assert_standalone(
        list.count == 0,
        "failure_list_count_accessible",
        "count member should be accessible",
        _counter) && result;

    // test 2: capacity member is accessible
    list.capacity = 64;

    result = d_assert_standalone(
        list.capacity == 64,
        "failure_list_capacity_accessible",
        "capacity member should be accessible",
        _counter) && result;

    // test 3: entries member is accessible
    list.entries = NULL;

    result = d_assert_standalone(
        list.entries == NULL,
        "failure_list_entries_accessible",
        "entries member should be accessible",
        _counter) && result;

    return result;
}

/*
d_tests_sa_standalone_failure_list_init
  Tests the d_test_sa_failure_list_init function.
  Tests the following:
  - NULL list is handled safely
  - count is initialized to 0
  - capacity is initialized to non-zero
  - entries is allocated (non-NULL)
*/
bool
d_tests_sa_standalone_failure_list_init
(
    struct d_test_counter* _counter
)
{
    bool                          result;
    struct d_test_sa_failure_list  list;

    result = true;

    // test 1: NULL list is handled safely
    d_test_sa_failure_list_init(NULL);

    result = d_assert_standalone(
        true,
        "failure_list_init_null_safe",
        "failure_list_init(NULL) should not crash",
        _counter) && result;

    // test 2: count is initialized to 0
    d_test_sa_failure_list_init(&list);

    result = d_assert_standalone(
        list.count == 0,
        "failure_list_init_count_zero",
        "count should be initialized to 0",
        _counter) && result;

    // test 3: capacity is initialized to non-zero
    result = d_assert_standalone(
        list.capacity > 0,
        "failure_list_init_capacity_nonzero",
        "capacity should be initialized to non-zero",
        _counter) && result;

    // test 4: entries is allocated
    result = d_assert_standalone(
        list.entries != NULL,
        "failure_list_init_entries_allocated",
        "entries should be allocated (non-NULL)",
        _counter) && result;

    // cleanup
    d_test_sa_failure_list_free(&list);

    return result;
}

/*
d_tests_sa_standalone_failure_list_add
  Tests the d_test_sa_failure_list_add function.
  Tests the following:
  - NULL list is handled safely
  - Entry is added correctly
  - count is incremented
  - Entry fields are stored
  - Multiple entries can be added
*/
bool
d_tests_sa_standalone_failure_list_add
(
    struct d_test_counter* _counter
)
{
    bool                          result;
    struct d_test_sa_failure_list  list;

    result = true;

    // test 1: NULL list is handled safely
    d_test_sa_failure_list_add(NULL, "mod", "test", "msg");

    result = d_assert_standalone(
        true,
        "failure_list_add_null_safe",
        "failure_list_add(NULL) should not crash",
        _counter) && result;

    // test 2: entry is added, count is incremented
    d_test_sa_failure_list_init(&list);
    d_test_sa_failure_list_add(&list, "module1",
                                "test_assert", "failed");

    result = d_assert_standalone(
        list.count == 1,
        "failure_list_add_count",
        "count should be 1 after adding entry",
        _counter) && result;

    // test 3: module_name is stored
    result = d_assert_standalone(
        list.entries[0].module_name != NULL,
        "failure_list_add_module_stored",
        "module_name should be stored",
        _counter) && result;

    result = d_assert_standalone(
        strcmp(list.entries[0].module_name, "module1") == 0,
        "failure_list_add_module_value",
        "module_name should match input",
        _counter) && result;

    // test 4: test_name is stored
    result = d_assert_standalone(
        list.entries[0].test_name != NULL,
        "failure_list_add_test_stored",
        "test_name should be stored",
        _counter) && result;

    result = d_assert_standalone(
        strcmp(list.entries[0].test_name, "test_assert") == 0,
        "failure_list_add_test_value",
        "test_name should match input",
        _counter) && result;

    // test 5: message is stored
    result = d_assert_standalone(
        list.entries[0].message != NULL,
        "failure_list_add_message_stored",
        "message should be stored",
        _counter) && result;

    // test 6: multiple entries can be added
    d_test_sa_failure_list_add(&list, "module2",
                                "test2", "msg2");
    d_test_sa_failure_list_add(&list, "module3",
                                "test3", "msg3");

    result = d_assert_standalone(
        list.count == 3,
        "failure_list_add_multiple",
        "count should be 3 after adding 3 entries",
        _counter) && result;

    // test 7: second entry is correct
    result = d_assert_standalone(
        strcmp(list.entries[1].module_name, "module2") == 0,
        "failure_list_add_second_entry",
        "Second entry module_name should match",
        _counter) && result;

    // test 8: NULL fields are stored as-is
    d_test_sa_failure_list_add(&list, NULL, NULL, NULL);

    result = d_assert_standalone(
        list.count == 4,
        "failure_list_add_null_fields",
        "Entry with NULL fields should be added",
        _counter) && result;

    result = d_assert_standalone(
        list.entries[3].module_name == NULL,
        "failure_list_add_null_module",
        "NULL module_name should be stored as NULL",
        _counter) && result;

    // cleanup
    d_test_sa_failure_list_free(&list);

    return result;
}

/*
d_tests_sa_standalone_failure_list_print_fn
  Tests the d_test_sa_failure_list_print function.
  Tests the following:
  - NULL list is handled safely
  - Empty list does not crash
  - Non-empty list does not crash
*/
bool
d_tests_sa_standalone_failure_list_print_fn
(
    struct d_test_counter* _counter
)
{
    bool                          result;
    struct d_test_sa_failure_list  list;

    result = true;

    // test 1: NULL list is handled safely
    d_test_sa_failure_list_print(NULL);

    result = d_assert_standalone(
        true,
        "failure_list_print_null_safe",
        "failure_list_print(NULL) should not crash",
        _counter) && result;

    // test 2: empty list does not crash
    d_test_sa_failure_list_init(&list);
    d_test_sa_failure_list_print(&list);

    result = d_assert_standalone(
        true,
        "failure_list_print_empty_safe",
        "Printing empty list should not crash",
        _counter) && result;

    // test 3: non-empty list does not crash
    d_test_sa_failure_list_add(&list, "mod", "test", "msg");
    d_test_sa_failure_list_print(&list);

    result = d_assert_standalone(
        true,
        "failure_list_print_nonempty_safe",
        "Printing non-empty list should not crash",
        _counter) && result;

    // cleanup
    d_test_sa_failure_list_free(&list);

    return result;
}

/*
d_tests_sa_standalone_failure_list_print_file_fn
  Tests the d_test_sa_failure_list_print_file function.
  Tests the following:
  - NULL file is handled safely
  - NULL list is handled safely
  - Non-empty list writes to file
*/
bool
d_tests_sa_standalone_failure_list_print_file_fn
(
    struct d_test_counter* _counter
)
{
    bool                          result;
    struct d_test_sa_failure_list  list;

    result = true;

    // test 1: NULL file is handled safely
    d_test_sa_failure_list_init(&list);
    d_test_sa_failure_list_add(&list, "mod", "test", "msg");
    d_test_sa_failure_list_print_file(NULL, &list);

    result = d_assert_standalone(
        true,
        "failure_list_print_file_null_file_safe",
        "print_file with NULL file should not crash",
        _counter) && result;

    // test 2: NULL list is handled safely
    d_test_sa_failure_list_print_file(stdout, NULL);

    result = d_assert_standalone(
        true,
        "failure_list_print_file_null_list_safe",
        "print_file with NULL list should not crash",
        _counter) && result;

    // test 3: non-empty list writes to stdout without crash
    d_test_sa_failure_list_print_file(stdout, &list);

    result = d_assert_standalone(
        true,
        "failure_list_print_file_nonempty",
        "print_file with non-empty list should not crash",
        _counter) && result;

    // cleanup
    d_test_sa_failure_list_free(&list);

    return result;
}

/*
d_tests_sa_standalone_failure_list_free
  Tests the d_test_sa_failure_list_free function.
  Tests the following:
  - NULL list is handled safely
  - Empty list is freed safely
  - Non-empty list is freed safely
*/
bool
d_tests_sa_standalone_failure_list_free
(
    struct d_test_counter* _counter
)
{
    bool                          result;
    struct d_test_sa_failure_list  list;

    result = true;

    // test 1: NULL list is handled safely
    d_test_sa_failure_list_free(NULL);

    result = d_assert_standalone(
        true,
        "failure_list_free_null_safe",
        "failure_list_free(NULL) should not crash",
        _counter) && result;

    // test 2: empty list is freed safely
    d_test_sa_failure_list_init(&list);
    d_test_sa_failure_list_free(&list);

    result = d_assert_standalone(
        true,
        "failure_list_free_empty_safe",
        "Freeing empty list should not crash",
        _counter) && result;

    // test 3: non-empty list is freed safely
    d_test_sa_failure_list_init(&list);
    d_test_sa_failure_list_add(&list, "mod1", "test1", "msg1");
    d_test_sa_failure_list_add(&list, "mod2", "test2", "msg2");
    d_test_sa_failure_list_free(&list);

    result = d_assert_standalone(
        true,
        "failure_list_free_nonempty_safe",
        "Freeing non-empty list should not crash",
        _counter) && result;

    // test 4: double free is handled safely
    d_test_sa_failure_list_free(&list);

    result = d_assert_standalone(
        true,
        "failure_list_free_double_safe",
        "Double free should not crash",
        _counter) && result;

    return result;
}

/*
d_tests_sa_standalone_failure_all
  Aggregation function that runs all failure tracking tests.
*/
bool
d_tests_sa_standalone_failure_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Failure Tracking\n");
    printf("  ---------------------------\n");

    result = d_tests_sa_standalone_failure_entry_struct(_counter)
             && result;
    result = d_tests_sa_standalone_failure_list_struct(_counter)
             && result;
    result = d_tests_sa_standalone_failure_list_init(_counter)
             && result;
    result = d_tests_sa_standalone_failure_list_add(_counter)
             && result;
    result = d_tests_sa_standalone_failure_list_print_fn(_counter)
             && result;
    result = d_tests_sa_standalone_failure_list_print_file_fn(_counter)
             && result;
    result = d_tests_sa_standalone_failure_list_free(_counter)
             && result;

    return result;
}
