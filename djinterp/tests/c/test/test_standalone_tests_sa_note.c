#include ".\test_standalone_tests_sa.h"


/******************************************************************************
 * VI. NOTE STRUCTURE TESTS
 *****************************************************************************/

/*
d_tests_sa_standalone_note_item_struct
  Tests the d_test_sa_note_item structure.
  Tests the following:
  - Structure has expected members
  - prefix member is accessible
  - message member is accessible
*/
bool
d_tests_sa_standalone_note_item_struct
(
    struct d_test_counter* _counter
)
{
    bool                       result;
    struct d_test_sa_note_item note_item;

    result = true;

    // test 1: prefix member is accessible
    note_item.prefix = "[INFO]";

    result = d_assert_standalone(
        note_item.prefix != NULL,
        "note_item_prefix_accessible",
        "prefix member should be accessible",
        _counter) && result;

    // test 2: message member is accessible
    note_item.message = "This is a test note";

    result = d_assert_standalone(
        note_item.message != NULL,
        "note_item_message_accessible",
        "message member should be accessible",
        _counter) && result;

    // test 3: structure size is reasonable (two pointers)
    result = d_assert_standalone(
        sizeof(struct d_test_sa_note_item) >= (2 * sizeof(void*)),
        "note_item_size",
        "d_test_sa_note_item should be at least 2 pointers",
        _counter) && result;

    // test 4: can use various prefix types
    note_item.prefix = "[WARN]";

    result = d_assert_standalone(
        d_strcasecmp(note_item.prefix, "[WARN]") == 0,
        "note_item_prefix_warn",
        "prefix should accept [WARN]",
        _counter) && result;

    note_item.prefix = "[TODO]";

    result = d_assert_standalone(
        d_strcasecmp(note_item.prefix, "[TODO]") == 0,
        "note_item_prefix_todo",
        "prefix should accept [TODO]",
        _counter) && result;

    note_item.prefix = "[BEST]";

    result = d_assert_standalone(
        d_strcasecmp(note_item.prefix, "[BEST]") == 0,
        "note_item_prefix_best",
        "prefix should accept [BEST]",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_note_section_struct
  Tests the d_test_sa_note_section structure.
  Tests the following:
  - Structure has expected members
  - title member is accessible
  - count member is accessible
  - items member is accessible
*/
bool
d_tests_sa_standalone_note_section_struct
(
    struct d_test_counter* _counter
)
{
    bool                          result;
    struct d_test_sa_note_section note_section;
    struct d_test_sa_note_item    items[3];

    result = true;

    // test 1: title member is accessible
    note_section.title = "CURRENT STATUS";

    result = d_assert_standalone(
        note_section.title != NULL,
        "note_section_title_accessible",
        "title member should be accessible",
        _counter) && result;

    // test 2: count member is accessible
    note_section.count = 3;

    result = d_assert_standalone(
        note_section.count == 3,
        "note_section_count_accessible",
        "count member should be accessible",
        _counter) && result;

    // test 3: items member is accessible
    items[0].prefix  = "[INFO]";
    items[0].message = "First note";
    items[1].prefix  = "[INFO]";
    items[1].message = "Second note";
    items[2].prefix  = "[WARN]";
    items[2].message = "Third note";

    note_section.items = items;

    result = d_assert_standalone(
        note_section.items != NULL,
        "note_section_items_accessible",
        "items member should be accessible",
        _counter) && result;

    // test 4: can access items through section
    result = d_assert_standalone(
        d_strcasecmp(note_section.items[0].prefix, "[INFO]") == 0,
        "note_section_items_indexable",
        "items should be indexable through section",
        _counter) && result;

    // test 5: structure size is reasonable
    result = d_assert_standalone(
        sizeof(struct d_test_sa_note_section) >= (sizeof(void*) + sizeof(size_t) + sizeof(void*)),
        "note_section_size",
        "d_test_sa_note_section should have expected size",
        _counter) && result;

    // test 6: count can be zero
    note_section.count = 0;
    note_section.items = NULL;

    result = d_assert_standalone(
        note_section.count == 0 && note_section.items == NULL,
        "note_section_empty",
        "Note section can be empty (count=0, items=NULL)",
        _counter) && result;

    return result;
}


/*
d_tests_sa_standalone_note_all
  Aggregation function that runs all note structure tests.
*/
bool
d_tests_sa_standalone_note_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] Note Structures\n");
    printf("  --------------------------\n");

    result = d_tests_sa_standalone_note_item_struct(_counter) && result;
    result = d_tests_sa_standalone_note_section_struct(_counter) && result;

    return result;
}
