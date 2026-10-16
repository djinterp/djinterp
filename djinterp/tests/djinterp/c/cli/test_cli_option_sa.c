/*******************************************************************************
* djinterp [cli]                                            test_cli_option_sa.c
*
*
* path:      /tests/djinterp/c/cli/test_cli_option_sa.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "test_cli_option_sa.h"
#include "../../../../inc/djinterp/test/c/test_counter.h"


/******************************************************************************
 * NOTES ON MACRO FIXES NEEDED IN cli_option.h
 *
 * The following macros need _key_length support to match the current
 * function signatures. Suggested corrections:
 *
 * D_CLI_ENTRY:
 *   current:  { (_key), (_value) }                     // 2 fields, struct has 3
 *   fix:      { (_key), (sizeof(_key) - 1), (_value) } // auto-compute from literal
 *
 * D_CLI_FIND_ENTRY:
 *   current:  d_cli_find((_array), D_CLI_COUNT(_array), (_key))
 *   fix:      d_cli_find((_array), D_CLI_COUNT(_array), (_key), (_key_length))
 *
 * D_CLI_FIND / D_CLI_FIND_IN:
 *   current:  d_cli_find_by_field(..., (_key))
 *   fix:      d_cli_find_by_field(..., (_key), (_key_length))
 *
 * d_cli_find (in .c):
 *   typo:     d_string_cli_entry_s -> d_string_cli_entry
 *
 * d_cli_find_with (in .c):
 *   guard:    (_key_length) -> (!_key_length)
 *
 * d_cli_parse (in .c):
 *   incomplete: key_length = <missing RHS>;
 *   fix:        key_length = strlen(_argv[i]);
 *   wrong var:  _key_length -> key_length
 *   missing arg: d_cli_find(_entries, _count, _argv[i])
 *   fix:         d_cli_find(_entries, _count, _argv[i], key_length)
 *
 * Tests below call functions directly to avoid broken macros.
 *****************************************************************************/


/******************************************************************************
 * TEST DATA
 *****************************************************************************/

static bool g_opt_verbose = false;
static bool g_opt_quiet   = true;
static bool g_opt_help    = false;
static int  g_opt_level   = 0;

static void
test_reset
(
    void
)
{
    g_opt_verbose = false;
    g_opt_quiet   = true;
    g_opt_help    = false;
    g_opt_level   = 0;

    return;
}


/******************************************************************************
 * TEST TABLE (plain string keys, no convention)
 *
 * Using direct initialization since D_CLI_ENTRY is currently missing
 * the key_length field.
 *****************************************************************************/

static struct d_string_cli_entry g_test_entries[] =
{
    { "verbose", 7, &g_opt_verbose },
    { "v",       1, &g_opt_verbose },
    { "quiet",   5, &g_opt_quiet   },
    { "q",       1, &g_opt_quiet   },
    { "help",    4, &g_opt_help    },
    { "h",       1, &g_opt_help    },
    { "level",   5, &g_opt_level   }
};

#define TEST_ENTRY_COUNT D_CLI_COUNT(g_test_entries)


/******************************************************************************
 * CUSTOM STRUCT FOR GENERIC FIND TESTS
 *****************************************************************************/

struct test_row
{
    int         id;
    const char* name;
    const char* tag;
};

static struct test_row g_custom_table[] =
{
    { 1, "alpha", "a" },
    { 2, "beta",  "b" },
    { 3, "gamma", "g" },
    { 4, NULL,    "d" }
};

#define CUSTOM_COUNT D_CLI_COUNT(g_custom_table)


/******************************************************************************
 * MATCH HANDLERS (fn_cli_match signature)
 *****************************************************************************/

static int g_match_count = 0;

/*
test_match_toggle_bool
  Toggles the bool at value. Returns true (continue).
*/
static bool
test_match_toggle_bool
(
    const char* _key,
    size_t      _key_length,
    void*       _value,
    const char* _next_arg,
    void*       _context
)
{
    (void)_key;
    (void)_key_length;
    (void)_next_arg;
    (void)_context;

    if (_value)
    {
        *(bool*)_value = !*(bool*)_value;
    }

    g_match_count++;

    return true;
}

/*
test_match_stop
  Always stops parsing.
*/
static bool
test_match_stop
(
    const char* _key,
    size_t      _key_length,
    void*       _value,
    const char* _next_arg,
    void*       _context
)
{
    (void)_key;
    (void)_key_length;
    (void)_value;
    (void)_next_arg;
    (void)_context;

    g_match_count++;

    return false;
}

/*
test_match_with_context
  Increments an int context.
*/
static bool
test_match_with_context
(
    const char* _key,
    size_t      _key_length,
    void*       _value,
    const char* _next_arg,
    void*       _context
)
{
    (void)_key;
    (void)_key_length;
    (void)_value;
    (void)_next_arg;

    if (_context)
    {
        (*(int*)_context)++;
    }

    return true;
}

/*
test_match_consume_next
  Reads next_arg as int into value. Returns true (continue).
*/
static bool
test_match_consume_next
(
    const char* _key,
    size_t      _key_length,
    void*       _value,
    const char* _next_arg,
    void*       _context
)
{
    (void)_key;
    (void)_key_length;
    (void)_context;

    if ( (_value) && (_next_arg) )
    {
        *(int*)_value = atoi(_next_arg);
    }

    g_match_count++;

    return true;
}

/*
test_match_check_key_length
  Verifies _key_length matches strlen(_key). Stores result in context.
*/
static bool
test_match_check_key_length
(
    const char* _key,
    size_t      _key_length,
    void*       _value,
    const char* _next_arg,
    void*       _context
)
{
    (void)_value;
    (void)_next_arg;

    if ( (_context) && (_key) )
    {
        *(bool*)_context =
            (*(bool*)_context) &&
            (_key_length == strlen(_key));
    }

    g_match_count++;

    return true;
}


/******************************************************************************
 * CUSTOM LOOKUP FOR find_with TESTS (fn_cli_lookup signature)
 *****************************************************************************/

static const void*
test_custom_lookup
(
    const void* _array,
    size_t      _array_count,
    const char* _key,
    size_t      _key_length,
    void*       _context
)
{
    const struct d_string_cli_entry* entries;
    size_t                           i;

    entries = (const struct d_string_cli_entry*)_array;

    if (_context)
    {
        (*(int*)_context)++;
    }

    if ( (!entries) || (!_key) || (!_key_length) )
    {
        return NULL;
    }

    for (i = 0; i < _array_count; i++)
    {
        if ( (entries[i].key) &&
             (entries[i].key_length == _key_length) &&
             (d_strcmp_n(entries[i].key,
                        entries[i].key_length,
                        _key,
                        _key_length) == 0) )
        {
            return &entries[i];
        }
    }

    return NULL;
}


/******************************************************************************
 * I. STRING ENTRY LOOKUP TESTS
 *****************************************************************************/

/*
d_tests_sa_test_cli_string_entry
  Tests d_string_cli_entry struct and d_cli_find.
  Tests the following:
  - struct has key, key_length, and value fields
  - d_cli_find returns value for existing key with correct length
  - multiple keys can map to the same value
  - returns NULL for missing key
  - returns NULL for NULL / zero-length inputs
  - key_length is respected (partial match does not succeed)
*/
bool
d_tests_sa_test_cli_string_entry
(
    struct d_test_counter* _counter
)
{
    bool                      result;
    int                       val;
    void*                     found;
    struct d_string_cli_entry entry =
        (struct d_string_cli_entry)
        {
            .key = "test",
            .key_length = 4,
            .value = &val
        };
    result = true;

    // test 1: struct fields accessible
    val   = 42;

    result = d_assert_standalone(
        (entry.key != NULL) &&
        (strcmp(entry.key, "test") == 0) &&
        (entry.key_length == 4) &&
        (entry.value == &val) &&
        (*(int*)entry.value == 42),
        "entry_fields",
        "d_string_cli_entry should store key, key_length, and value",
        _counter) && result;

    // test 2: struct size (two pointers + size_t)
    result = d_assert_standalone(
        sizeof(struct d_string_cli_entry) >=
            (sizeof(const char*) + sizeof(size_t) + sizeof(void*)),
        "entry_size",
        "d_string_cli_entry should be at least char*+size_t+void*",
        _counter) && result;

    // test 3: d_cli_find returns value for "v" (length 1)
    found = d_cli_find(g_test_entries,
                       TEST_ENTRY_COUNT,
                       "v",
                       1);
    result = d_assert_standalone(
        found == &g_opt_verbose,
        "find_v",
        "d_cli_find should return &g_opt_verbose for 'v'",
        _counter) && result;

    // test 4: d_cli_find returns value for "verbose" (length 7)
    found = d_cli_find(g_test_entries, TEST_ENTRY_COUNT,
                       "verbose", 7);
    result = d_assert_standalone(
        found == &g_opt_verbose,
        "find_verbose",
        "d_cli_find should return &g_opt_verbose for 'verbose'",
        _counter) && result;

    // test 5: both forms map to same pointer
    {
        void* short_val;
        void* long_val;

        short_val = d_cli_find(g_test_entries, TEST_ENTRY_COUNT,
                               "q", 1);
        long_val  = d_cli_find(g_test_entries, TEST_ENTRY_COUNT,
                               "quiet", 5);

        result = d_assert_standalone(
            (short_val == long_val) &&
            (short_val == &g_opt_quiet),
            "find_both_forms",
            "short and long keys should map to same pointer",
            _counter) && result;
    }

    // test 6: missing key returns NULL
    found = d_cli_find(g_test_entries, TEST_ENTRY_COUNT,
                       "nonexistent", 11);
    result = d_assert_standalone(
        found == NULL,
        "find_missing",
        "d_cli_find should return NULL for missing key",
        _counter) && result;

    // test 7: NULL key returns NULL
    found = d_cli_find(g_test_entries, TEST_ENTRY_COUNT,
                       NULL, 0);
    result = d_assert_standalone(
        found == NULL,
        "find_null_key",
        "d_cli_find should return NULL for NULL key",
        _counter) && result;

    // test 8: zero key_length returns NULL
    found = d_cli_find(g_test_entries, TEST_ENTRY_COUNT,
                       "v", 0);
    result = d_assert_standalone(
        found == NULL,
        "find_zero_length",
        "d_cli_find should return NULL for zero key_length",
        _counter) && result;

    // test 9: NULL entries returns NULL
    found = d_cli_find(NULL, 5, "v", 1);
    result = d_assert_standalone(
        found == NULL,
        "find_null_entries",
        "d_cli_find should return NULL for NULL entries",
        _counter) && result;

    // test 10: zero count returns NULL
    found = d_cli_find(g_test_entries, 0, "v", 1);
    result = d_assert_standalone(
        found == NULL,
        "find_zero_count",
        "d_cli_find should return NULL for zero count",
        _counter) && result;

    // test 11: wrong key_length does not match
    found = d_cli_find(g_test_entries, TEST_ENTRY_COUNT,
                       "verbose", 4);
    result = d_assert_standalone(
        found == NULL,
        "find_wrong_length",
        "d_cli_find should not match when key_length is wrong",
        _counter) && result;

    // test 12: case sensitive
    found = d_cli_find(g_test_entries, TEST_ENTRY_COUNT,
                       "Verbose", 7);
    result = d_assert_standalone(
        found == NULL,
        "find_case_sensitive",
        "d_cli_find should be case sensitive",
        _counter) && result;

    return result;
}


/******************************************************************************
 * II. GENERIC FIELD-BASED LOOKUP TESTS
 *****************************************************************************/

/*
d_tests_sa_test_cli_find_by_field
  Tests the generic d_cli_find_by_field on a custom struct.
  Tests the following:
  - finds element by name field
  - finds element by tag field (different offset)
  - skips NULL field values
  - returns NULL for missing key and invalid inputs
  - key_length is passed through correctly
*/
bool
d_tests_sa_test_cli_find_by_field
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct test_row* found;

    result = true;

    // test 1: find "beta" by name (length 4)
    found = (struct test_row*)
        d_cli_find_by_field(g_custom_table,
                            CUSTOM_COUNT,
                            sizeof(struct test_row),
                            offsetof(struct test_row, name),
                            "beta", 4);
    result = d_assert_standalone(
        (found != NULL) && (found->id == 2),
        "field_name_beta",
        "should find 'beta' by name (id==2)",
        _counter) && result;

    // test 2: find "g" by tag (length 1)
    found = (struct test_row*)
        d_cli_find_by_field(g_custom_table,
                            CUSTOM_COUNT,
                            sizeof(struct test_row),
                            offsetof(struct test_row, tag),
                            "g", 1);
    result = d_assert_standalone(
        (found != NULL) && (found->id == 3),
        "field_tag_g",
        "should find 'gamma' by tag (id==3)",
        _counter) && result;

    // test 3: NULL field in row[3].name is skipped
    found = (struct test_row*)
        d_cli_find_by_field(g_custom_table,
                            CUSTOM_COUNT,
                            sizeof(struct test_row),
                            offsetof(struct test_row, name),
                            "alpha", 5);
    result = d_assert_standalone(
        (found != NULL) && (found->id == 1),
        "field_skip_null",
        "should skip NULL name fields and find 'alpha'",
        _counter) && result;

    // test 4: missing key
    found = (struct test_row*)
        d_cli_find_by_field(g_custom_table,
                            CUSTOM_COUNT,
                            sizeof(struct test_row),
                            offsetof(struct test_row, name),
                            "omega", 5);
    result = d_assert_standalone(
        found == NULL,
        "field_missing",
        "should return NULL for missing key",
        _counter) && result;

    // test 5: NULL array
    found = (struct test_row*)
        d_cli_find_by_field(NULL,
                            CUSTOM_COUNT,
                            sizeof(struct test_row),
                            offsetof(struct test_row, name),
                            "alpha", 5);
    result = d_assert_standalone(
        found == NULL,
        "field_null_array",
        "should return NULL for NULL array",
        _counter) && result;

    // test 6: zero element_size
    found = (struct test_row*)
        d_cli_find_by_field(g_custom_table,
                            CUSTOM_COUNT,
                            0,
                            offsetof(struct test_row, name),
                            "alpha", 5);
    result = d_assert_standalone(
        found == NULL,
        "field_zero_elem",
        "should return NULL for zero element_size",
        _counter) && result;

    // test 7: zero key_length
    found = (struct test_row*)
        d_cli_find_by_field(g_custom_table,
                            CUSTOM_COUNT,
                            sizeof(struct test_row),
                            offsetof(struct test_row, name),
                            "alpha", 0);
    result = d_assert_standalone(
        found == NULL,
        "field_zero_key_len",
        "should return NULL for zero key_length",
        _counter) && result;

    // test 8: NULL key
    found = (struct test_row*)
        d_cli_find_by_field(g_custom_table,
                            CUSTOM_COUNT,
                            sizeof(struct test_row),
                            offsetof(struct test_row, name),
                            NULL, 5);
    result = d_assert_standalone(
        found == NULL,
        "field_null_key",
        "should return NULL for NULL key",
        _counter) && result;

    return result;
}


/******************************************************************************
 * III. FIND MACRO TESTS
 *****************************************************************************/

/*
d_tests_sa_test_cli_find_macro
  Tests D_CLI_FIND and D_CLI_FIND_IN macros on custom structs.
  NOTE: these macros currently lack the _key_length parameter.
  Tests are written for the corrected signatures:
    D_CLI_FIND(_type, _array, _count, _field, _key, _key_length)
    D_CLI_FIND_IN(_type, _array, _field, _key, _key_length)
  Until fixed, call d_cli_find_by_field directly.
*/
bool
d_tests_sa_test_cli_find_macro
(
    struct d_test_counter* _counter
)
{
    bool             result;
    struct test_row* found;

    result = true;

    // test 1: find by name via d_cli_find_by_field (macro stand-in)
    found = (struct test_row*)
        d_cli_find_by_field(g_custom_table,
                            CUSTOM_COUNT,
                            sizeof(struct test_row),
                            offsetof(struct test_row, name),
                            "gamma", 5);
    result = d_assert_standalone(
        (found != NULL) && (found->id == 3),
        "macro_find_name",
        "should find 'gamma' (id==3)",
        _counter) && result;

    // test 2: find by tag (different field)
    found = (struct test_row*)
        d_cli_find_by_field(g_custom_table,
                            CUSTOM_COUNT,
                            sizeof(struct test_row),
                            offsetof(struct test_row, tag),
                            "b", 1);
    result = d_assert_standalone(
        (found != NULL) && (found->id == 2),
        "macro_find_tag",
        "should find tag 'b' (id==2)",
        _counter) && result;

    // test 3: auto-count equivalent (all entries)
    found = (struct test_row*)
        d_cli_find_by_field(g_custom_table,
                            D_CLI_COUNT(g_custom_table),
                            sizeof(struct test_row),
                            offsetof(struct test_row, name),
                            "alpha", 5);
    result = d_assert_standalone(
        (found != NULL) && (found->id == 1),
        "macro_find_in",
        "D_CLI_COUNT should derive count and find 'alpha'",
        _counter) && result;

    // test 4: missing
    found = (struct test_row*)
        d_cli_find_by_field(g_custom_table,
                            CUSTOM_COUNT,
                            sizeof(struct test_row),
                            offsetof(struct test_row, name),
                            "omega", 5);
    result = d_assert_standalone(
        found == NULL,
        "macro_find_missing",
        "should return NULL for missing key",
        _counter) && result;

    // test 5: case sensitive
    found = (struct test_row*)
        d_cli_find_by_field(g_custom_table,
                            CUSTOM_COUNT,
                            sizeof(struct test_row),
                            offsetof(struct test_row, name),
                            "Alpha", 5);
    result = d_assert_standalone(
        found == NULL,
        "macro_find_case",
        "should be case sensitive",
        _counter) && result;

    return result;
}


/******************************************************************************
 * IV. CUSTOM LOOKUP TESTS
 *****************************************************************************/

/*
d_tests_sa_test_cli_find_with
  Tests d_cli_find_with custom lookup + context.
  Tests the following:
  - custom function invoked and finds entry
  - key_length passed through to custom lookup
  - context passed through and used
  - returns NULL for missing key and NULL inputs
*/
bool
d_tests_sa_test_cli_find_with
(
    struct d_test_counter* _counter
)
{
    bool                              result;
    const void*                       found;
    const struct d_string_cli_entry*  entry;
    int                               call_count;

    result = true;

    // test 1: finds "q" (length 1)
    call_count = 0;
    found      = d_cli_find_with(g_test_entries,
                                 TEST_ENTRY_COUNT,
                                 "q", 1,
                                 test_custom_lookup,
                                 &call_count);
    entry      = (const struct d_string_cli_entry*)found;
    result     = d_assert_standalone(
        (entry != NULL) && (entry->value == &g_opt_quiet),
        "with_found",
        "custom lookup should find 'q' -> &g_opt_quiet",
        _counter) && result;

    // test 2: context incremented
    result = d_assert_standalone(
        call_count == 1,
        "with_context",
        "context should be incremented to 1",
        _counter) && result;

    // test 3: finds "verbose" (length 7)
    call_count = 0;
    found      = d_cli_find_with(g_test_entries,
                                 TEST_ENTRY_COUNT,
                                 "verbose", 7,
                                 test_custom_lookup,
                                 &call_count);
    entry      = (const struct d_string_cli_entry*)found;
    result     = d_assert_standalone(
        (entry != NULL) && (entry->value == &g_opt_verbose),
        "with_found_long",
        "custom lookup should find 'verbose' -> &g_opt_verbose",
        _counter) && result;

    // test 4: missing returns NULL
    found = d_cli_find_with(g_test_entries,
                            TEST_ENTRY_COUNT,
                            "z", 1,
                            test_custom_lookup,
                            &call_count);
    result = d_assert_standalone(
        found == NULL,
        "with_missing",
        "custom lookup should return NULL for missing",
        _counter) && result;

    // test 5: NULL lookup fn returns NULL
    found = d_cli_find_with(g_test_entries,
                            TEST_ENTRY_COUNT,
                            "v", 1,
                            NULL, NULL);
    result = d_assert_standalone(
        found == NULL,
        "with_null_fn",
        "should return NULL for NULL lookup function",
        _counter) && result;

    // test 6: zero key_length returns NULL
    found = d_cli_find_with(g_test_entries,
                            TEST_ENTRY_COUNT,
                            "v", 0,
                            test_custom_lookup,
                            NULL);
    result = d_assert_standalone(
        found == NULL,
        "with_zero_key_len",
        "should return NULL for zero key_length",
        _counter) && result;

    return result;
}


/******************************************************************************
 * V. ARGV PARSE TESTS
 *****************************************************************************/

/*
d_tests_sa_test_cli_parse
  Tests d_cli_parse — exact token match.
  Tests the following:
  - matches exact key in argv
  - handler receives key_length
  - handler can toggle bool via value pointer
  - handler returning false stops parsing
  - handler receives next_arg
  - handler receives context
  - multiple matches in one command
  - double toggle restores original
  - unrecognized args skipped
  - NULL handler silently consumes
  - NULL argv returns true
  - D_CLI_PARSE macro works
*/
bool
d_tests_sa_test_cli_parse
(
    struct d_test_counter* _counter
)
{
    bool result;
    bool ok;

    result = true;

    // test 1: exact match "verbose" toggles bool
    test_reset();
    g_match_count = 0;

    {
        char* argv[] = {"prog", "verbose", NULL};

        ok = d_cli_parse(g_test_entries, TEST_ENTRY_COUNT,
                         2, argv,
                         test_match_toggle_bool, NULL);
    }

    result = d_assert_standalone(
        ok && (g_opt_verbose == true) && (g_match_count == 1),
        "parse_exact_match",
        "'verbose' should match and toggle verbose true",
        _counter) && result;

    // test 2: "v" matches the short key
    test_reset();
    g_match_count = 0;

    {
        char* argv[] = {"prog", "v", NULL};

        ok = d_cli_parse(g_test_entries, TEST_ENTRY_COUNT,
                         2, argv,
                         test_match_toggle_bool, NULL);
    }

    result = d_assert_standalone(
        ok && (g_opt_verbose == true) && (g_match_count == 1),
        "parse_short_key",
        "'v' should match and toggle verbose true",
        _counter) && result;

    // test 3: "q" toggles quiet true->false
    test_reset();
    g_match_count = 0;

    {
        char* argv[] = {"prog", "q", NULL};

        ok = d_cli_parse(g_test_entries, TEST_ENTRY_COUNT,
                         2, argv,
                         test_match_toggle_bool, NULL);
    }

    result = d_assert_standalone(
        ok && (g_opt_quiet == false),
        "parse_toggle_true_false",
        "'q' should toggle quiet true->false",
        _counter) && result;

    // test 4: multiple matches
    test_reset();
    g_match_count = 0;

    {
        char* argv[] = {"prog", "v", "q", NULL};

        ok = d_cli_parse(g_test_entries, TEST_ENTRY_COUNT,
                         3, argv,
                         test_match_toggle_bool, NULL);
    }

    result = d_assert_standalone(
        ok &&
        (g_opt_verbose == true) &&
        (g_opt_quiet == false) &&
        (g_match_count == 2),
        "parse_multiple",
        "multiple keys should each trigger handler",
        _counter) && result;

    // test 5: handler stops parsing
    test_reset();
    g_match_count = 0;

    {
        char* argv[] = {"prog", "h", "v", NULL};

        ok = d_cli_parse(g_test_entries, TEST_ENTRY_COUNT,
                         3, argv,
                         test_match_stop, NULL);
    }

    result = d_assert_standalone(
        (!ok) && (g_match_count == 1),
        "parse_stop",
        "handler returning false should stop parsing",
        _counter) && result;

    // test 6: handler receives context
    test_reset();
    {
        int   ctx    = 0;
        char* argv[] = {"prog", "v", "q", NULL};

        ok = d_cli_parse(g_test_entries, TEST_ENTRY_COUNT,
                         3, argv,
                         test_match_with_context, &ctx);

        result = d_assert_standalone(
            ok && (ctx == 2),
            "parse_context",
            "handler should receive and use context (ctx==2)",
            _counter) && result;
    }

    // test 7: handler receives key_length matching strlen(key)
    test_reset();

    g_match_count = 0;
    {
        bool  lengths_ok = true;
        char* argv[]     = {"prog", "verbose", "q", NULL};

        ok = d_cli_parse(g_test_entries, TEST_ENTRY_COUNT,
                         3, argv,
                         test_match_check_key_length,
                         &lengths_ok);

        result = d_assert_standalone(
            ok && lengths_ok && (g_match_count == 2),
            "parse_key_length",
            "handler should receive key_length == strlen(key)",
            _counter) && result;
    }

    // test 8: unrecognized args skipped
    test_reset();
    g_match_count = 0;

    {
        char* argv[] = {"prog", "bogus", "v", NULL};

        ok = d_cli_parse(g_test_entries, TEST_ENTRY_COUNT,
                         3, argv,
                         test_match_toggle_bool, NULL);
    }

    result = d_assert_standalone(
        ok && (g_opt_verbose == true) && (g_match_count == 1),
        "parse_skip_unknown",
        "unrecognized args should be silently skipped",
        _counter) && result;

    // test 9: NULL handler silently consumes
    test_reset();

    {
        char* argv[] = {"prog", "v", NULL};

        ok = d_cli_parse(g_test_entries, TEST_ENTRY_COUNT,
                         2, argv,
                         NULL, NULL);
    }

    result = d_assert_standalone(
        ok && (g_opt_verbose == false),
        "parse_null_handler",
        "NULL handler should silently consume (no toggle)",
        _counter) && result;

    // test 10: NULL argv returns true
    ok = d_cli_parse(g_test_entries, TEST_ENTRY_COUNT,
                     0, NULL,
                     test_match_toggle_bool, NULL);
    result = d_assert_standalone(
        ok,
        "parse_null_argv",
        "NULL argv should return true",
        _counter) && result;

    // test 11: double toggle restores original
    test_reset();
    g_match_count = 0;

    {
        char* argv[] = {"prog", "v", "v", NULL};

        ok = d_cli_parse(g_test_entries, TEST_ENTRY_COUNT,
                         3, argv,
                         test_match_toggle_bool, NULL);
    }

    result = d_assert_standalone(
        ok && (g_opt_verbose == false) && (g_match_count == 2),
        "parse_double_toggle",
        "toggling twice should restore original value",
        _counter) && result;

    // test 12: D_CLI_PARSE macro
    test_reset();
    g_match_count = 0;

    {
        char* argv[] = {"prog", "verbose", "quiet", NULL};

        ok = D_CLI_PARSE(g_test_entries, 3, argv,
                         test_match_toggle_bool, NULL);
    }

    result = d_assert_standalone(
        ok && (g_opt_verbose == true) && (g_opt_quiet == false),
        "macro_parse",
        "D_CLI_PARSE should work on static array",
        _counter) && result;

    return result;
}


/******************************************************************************
 * VI. TABLE MACRO TESTS
 *****************************************************************************/

/*
d_tests_sa_test_cli_table_macros
  Tests D_CLI_COUNT, D_CLI_TOTAL_SIZE.
  Tests the following:
  - D_CLI_COUNT returns correct element count
  - D_CLI_TOTAL_SIZE returns correct byte count
  - macros work on both global and stack arrays
  - key_length stored correctly in table entries
*/
bool
d_tests_sa_test_cli_table_macros
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    // test 1: D_CLI_COUNT on global entry array
    result = d_assert_standalone(
        D_CLI_COUNT(g_test_entries) == 7,
        "macro_count_global",
        "D_CLI_COUNT should return 7 for g_test_entries",
        _counter) && result;

    // test 2: D_CLI_COUNT on stack array
    {
        struct d_string_cli_entry local[] =
        {
            { "a", 1, NULL },
            { "b", 1, NULL },
            { "c", 1, NULL }
        };

        result = d_assert_standalone(
            D_CLI_COUNT(local) == 3,
            "macro_count_stack",
            "D_CLI_COUNT should return 3 for stack array",
            _counter) && result;
    }

    // test 3: D_CLI_TOTAL_SIZE
    result = d_assert_standalone(
        D_CLI_TOTAL_SIZE(g_test_entries) ==
            (7 * sizeof(struct d_string_cli_entry)),
        "macro_total_size",
        "D_CLI_TOTAL_SIZE should equal count * element size",
        _counter) && result;

    // test 4: D_CLI_COUNT on non-entry array
    result = d_assert_standalone(
        D_CLI_COUNT(g_custom_table) == 4,
        "macro_count_custom",
        "D_CLI_COUNT should work on any static array",
        _counter) && result;

    // test 5: key_length values match actual key lengths
    {
        size_t i;
        bool   all_match = true;

        for (i = 0; i < TEST_ENTRY_COUNT; i++)
        {
            if (g_test_entries[i].key)
            {
                if (g_test_entries[i].key_length !=
                    strlen(g_test_entries[i].key))
                {
                    all_match = false;
                }
            }
        }

        result = d_assert_standalone(
            all_match,
            "entry_key_lengths",
            "all key_length values should match strlen(key)",
            _counter) && result;
    }

    return result;
}


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/

bool
d_tests_sa_test_cli_option_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] CLI Options\n");
    printf("  ----------------------\n");

    result = d_tests_sa_test_cli_string_entry(_counter) && result;
    result = d_tests_sa_test_cli_find_by_field(_counter) && result;
    result = d_tests_sa_test_cli_find_macro(_counter)    && result;
    result = d_tests_sa_test_cli_find_with(_counter)     && result;
    result = d_tests_sa_test_cli_parse(_counter)         && result;
    result = d_tests_sa_test_cli_table_macros(_counter)  && result;

    return result;
}
