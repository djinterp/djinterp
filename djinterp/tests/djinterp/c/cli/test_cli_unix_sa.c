/*******************************************************************************
* djinterp [cli]                                              test_cli_unix_sa.c
*
*
* path:      /tests/djinterp/c/cli/test_cli_unix_sa.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// djinterp
#include "test_cli_unix_sa.h"
#include "../../../../inc/djinterp/test/c/test_counter.h"


/******************************************************************************
 * TEST DATA
 *****************************************************************************/

static bool g_opt_verbose = false;
static bool g_opt_quiet   = true;
static bool g_opt_extract = false;
static bool g_opt_force   = false;
static bool g_opt_help    = false;
static int  g_opt_level   = 0;
static const char* g_opt_output = NULL;

static void
test_unix_reset
(
    void
)
{
    g_opt_verbose = false;
    g_opt_quiet   = true;
    g_opt_extract = false;
    g_opt_force   = false;
    g_opt_help    = false;
    g_opt_level   = 0;
    g_opt_output  = NULL;

    return;
}


/******************************************************************************
 * TEST TABLE (bare keys, no operators)
 *****************************************************************************/

static struct d_string_cli_entry g_unix_entries[] =
{
    { "v",       1, &g_opt_verbose },
    { "verbose", 7, &g_opt_verbose },
    { "q",       1, &g_opt_quiet   },
    { "quiet",   5, &g_opt_quiet   },
    { "x",       1, &g_opt_extract },
    { "f",       1, &g_opt_force   },
    { "h",       1, &g_opt_help    },
    { "help",    4, &g_opt_help    },
    { "o",       1, &g_opt_output  },
    { "output",  6, &g_opt_output  },
    { "level",   5, &g_opt_level   }
};


/******************************************************************************
 * MATCH HANDLERS (fn_cli_unix_match signature)
 *****************************************************************************/

static int g_unix_match_count = 0;
static const char* g_unix_last_key = NULL;
static size_t g_unix_last_key_len  = 0;

/*
test_unix_toggle
  Toggles bool at value. Returns 0 (did not consume next_arg).
*/
static int
test_unix_toggle
(
    const char* _key,
    size_t      _key_length,
    void*       _value,
    const char* _next_arg,
    void*       _context
)
{
    (void)_next_arg;
    (void)_context;

    g_unix_last_key     = _key;
    g_unix_last_key_len = _key_length;

    if (_value)
    {
        *(bool*)_value = !*(bool*)_value;
    }

    g_unix_match_count++;

    return 0;
}

/*
test_unix_stop
  Stops parsing immediately.
*/
static int
test_unix_stop
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

    g_unix_match_count++;

    return -1;
}

/*
test_unix_dispatch
  Routes to toggle or consume based on key.
  "o"/"output" -> consume string (return 1).
  "level" -> consume int (return 1).
  else -> toggle bool (return 0).
*/
static int
test_unix_dispatch
(
    const char* _key,
    size_t      _key_length,
    void*       _value,
    const char* _next_arg,
    void*       _context
)
{
    (void)_context;

    g_unix_last_key     = _key;
    g_unix_last_key_len = _key_length;
    g_unix_match_count++;

    if ( (d_strcmp_n(_key, _key_length, "o", 1) == 0) ||
         (d_strcmp_n(_key, _key_length, "output", 6) == 0) )
    {
        if (_value)
        {
            *(const char**)_value = _next_arg;
        }

        return 1;
    }

    if (d_strcmp_n(_key, _key_length, "level", 5) == 0)
    {
        if ( (_value) && (_next_arg) )
        {
            *(int*)_value = atoi(_next_arg);
        }

        return 1;
    }

    // default: toggle bool
    if (_value)
    {
        *(bool*)_value = !*(bool*)_value;
    }

    return 0;
}

/*
test_unix_with_context
  Increments int context. Returns 0.
*/
static int
test_unix_with_context
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

    return 0;
}

/*
test_unix_check_key_length
  Verifies _key_length == strlen(_key). Stores result in context bool.
*/
static int
test_unix_check_key_length
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

    g_unix_match_count++;

    return 0;
}


/******************************************************************************
 * I. LONG OPTION TESTS
 *****************************************************************************/

/*
d_tests_sa_test_cli_unix_long_option
  Tests long option parsing (--key).
  Tests the following:
  - "--verbose" matches "verbose" and toggles
  - "--quiet" toggles true->false
  - unrecognized "--bogus" is skipped
  - handler receives correct key_length
*/
bool
d_tests_sa_test_cli_unix_long_option
(
    struct d_test_counter* _counter
)
{
    bool result;
    bool ok;

    result = true;

    // test 1: "--verbose" matches
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "--verbose", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              2, argv,
                              test_unix_toggle, NULL);
    }

    result = d_assert_standalone(
        ok && (g_opt_verbose == true) && (g_unix_match_count == 1),
        "long_verbose",
        "'--verbose' should match and toggle verbose true",
        _counter) && result;

    // test 2: key_length correct for long option
    result = d_assert_standalone(
        g_unix_last_key_len == 7,
        "long_key_length",
        "handler should receive key_length==7 for 'verbose'",
        _counter) && result;

    // test 3: "--quiet" toggles true->false
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "--quiet", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              2, argv,
                              test_unix_toggle, NULL);
    }

    result = d_assert_standalone(
        ok && (g_opt_quiet == false),
        "long_quiet",
        "'--quiet' should toggle quiet true->false",
        _counter) && result;

    // test 4: unrecognized long option skipped
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "--bogus", "--verbose", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              3, argv,
                              test_unix_toggle, NULL);
    }

    result = d_assert_standalone(
        ok && (g_opt_verbose == true) && (g_unix_match_count == 1),
        "long_skip_unknown",
        "unrecognized '--bogus' should be skipped",
        _counter) && result;

    return result;
}


/*
d_tests_sa_test_cli_unix_long_equals
  Tests long option with embedded value (--key=val).
  Tests the following:
  - "--output=file.txt" matches with next_arg "file.txt"
  - "--output=" matches with empty string next_arg
  - embedded value does not consume next argv
  - handler receives correct key_length (excludes =val)
*/
bool
d_tests_sa_test_cli_unix_long_equals
(
    struct d_test_counter* _counter
)
{
    bool result;
    bool ok;

    result = true;

    // test 1: "--output=file.txt"
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "--output=file.txt", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              2, argv,
                              test_unix_dispatch, NULL);
    }

    result = d_assert_standalone(
        ok &&
        (g_opt_output != NULL) &&
        (strcmp(g_opt_output, "file.txt") == 0),
        "long_equals_value",
        "'--output=file.txt' should pass 'file.txt' as next_arg",
        _counter) && result;

    // test 2: key_length == 6 ("output" without "=file.txt")
    result = d_assert_standalone(
        g_unix_last_key_len == 6,
        "long_equals_key_len",
        "key_length should be 6 for 'output' (excludes =val)",
        _counter) && result;

    // test 3: "--output=" gives empty string
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "--output=", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              2, argv,
                              test_unix_dispatch, NULL);
    }

    result = d_assert_standalone(
        ok &&
        (g_opt_output != NULL) &&
        (strcmp(g_opt_output, "") == 0),
        "long_equals_empty",
        "'--output=' should pass empty string as next_arg",
        _counter) && result;

    // test 4: embedded value does not consume next argv
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "--output=one", "--verbose", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              3, argv,
                              test_unix_dispatch, NULL);
    }

    result = d_assert_standalone(
        ok &&
        (strcmp(g_opt_output, "one") == 0) &&
        (g_opt_verbose == true) &&
        (g_unix_match_count == 2),
        "long_equals_no_advance",
        "embedded '=' should not consume next argv element",
        _counter) && result;

    return result;
}


/******************************************************************************
 * II. SHORT FLAG TESTS
 *****************************************************************************/

/*
d_tests_sa_test_cli_unix_short_single
  Tests single short flag parsing (-v).
  Tests the following:
  - "-v" matches "v" and toggles
  - "-q" toggles true->false
  - unrecognized "-z" is skipped
  - handler receives key_length==1
*/
bool
d_tests_sa_test_cli_unix_short_single
(
    struct d_test_counter* _counter
)
{
    bool result;
    bool ok;

    result = true;

    // test 1: "-v"
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "-v", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              2, argv,
                              test_unix_toggle, NULL);
    }

    result = d_assert_standalone(
        ok && (g_opt_verbose == true) && (g_unix_match_count == 1),
        "short_v",
        "'-v' should match 'v' and toggle",
        _counter) && result;

    // test 2: key_length==1 for short flag
    result = d_assert_standalone(
        g_unix_last_key_len == 1,
        "short_key_length",
        "handler should receive key_length==1 for short flag",
        _counter) && result;

    // test 3: "-q"
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "-q", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              2, argv,
                              test_unix_toggle, NULL);
    }

    result = d_assert_standalone(
        ok && (g_opt_quiet == false),
        "short_q",
        "'-q' should toggle quiet true->false",
        _counter) && result;

    // test 4: unrecognized "-z" skipped
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "-z", "-v", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              3, argv,
                              test_unix_toggle, NULL);
    }

    result = d_assert_standalone(
        ok && (g_opt_verbose == true) && (g_unix_match_count == 1),
        "short_skip_unknown",
        "unrecognized '-z' should be skipped",
        _counter) && result;

    return result;
}


/*
d_tests_sa_test_cli_unix_short_combined
  Tests combined short flags (-xvf).
  Tests the following:
  - "-xvf" expands to three matches: "x", "v", "f"
  - each flag toggled independently
  - unknown chars in combination are skipped
  - all handlers receive key_length==1
*/
bool
d_tests_sa_test_cli_unix_short_combined
(
    struct d_test_counter* _counter
)
{
    bool result;
    bool ok;

    result = true;

    // test 1: "-xvf" expands all three
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "-xvf", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              2, argv,
                              test_unix_toggle, NULL);
    }

    result = d_assert_standalone(
        ok &&
        (g_opt_extract == true) &&
        (g_opt_verbose == true) &&
        (g_opt_force == true) &&
        (g_unix_match_count == 3),
        "combined_xvf",
        "'-xvf' should expand to 'x', 'v', 'f' (3 matches)",
        _counter) && result;

    // test 2: "-vq" mixed toggles
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "-vq", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              2, argv,
                              test_unix_toggle, NULL);
    }

    result = d_assert_standalone(
        ok &&
        (g_opt_verbose == true) &&
        (g_opt_quiet == false) &&
        (g_unix_match_count == 2),
        "combined_vq",
        "'-vq' should toggle both verbose and quiet",
        _counter) && result;

    // test 3: unknown char in combination skipped
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "-vzf", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              2, argv,
                              test_unix_toggle, NULL);
    }

    result = d_assert_standalone(
        ok &&
        (g_opt_verbose == true) &&
        (g_opt_force == true) &&
        (g_unix_match_count == 2),
        "combined_skip_unknown",
        "'-vzf' should match 'v' and 'f', skip 'z'",
        _counter) && result;

    // test 4: all expanded flags have key_length==1
    test_unix_reset();
    g_unix_match_count = 0;
    {
        bool  lengths_ok = true;
        char* argv[]     = {"prog", "-xvf", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              2, argv,
                              test_unix_check_key_length,
                              &lengths_ok);

        result = d_assert_standalone(
            ok && lengths_ok && (g_unix_match_count == 3),
            "combined_key_lengths",
            "all expanded short flags should have key_length==1",
            _counter) && result;
    }

    return result;
}


/******************************************************************************
 * III. NEXT-ARG CONSUMPTION TESTS
 *****************************************************************************/

/*
d_tests_sa_test_cli_unix_consume_next
  Tests short and long options consuming next argv.
  Tests the following:
  - "-o file.txt" consumes next arg (handler returns 1)
  - "--output file.txt" consumes next arg
  - consumed arg is not re-processed as a flag
  - last char in combined flags can consume: "-vo file.txt"
*/
bool
d_tests_sa_test_cli_unix_consume_next
(
    struct d_test_counter* _counter
)
{
    bool result;
    bool ok;

    result = true;

    // test 1: "-o file.txt" consumes next
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "-o", "file.txt", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              3, argv,
                              test_unix_dispatch, NULL);
    }

    result = d_assert_standalone(
        ok &&
        (g_opt_output != NULL) &&
        (strcmp(g_opt_output, "file.txt") == 0),
        "consume_short",
        "'-o file.txt' should consume next arg as output path",
        _counter) && result;

    // test 2: "--output file.txt" consumes next
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "--output", "file.txt", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              3, argv,
                              test_unix_dispatch, NULL);
    }

    result = d_assert_standalone(
        ok &&
        (g_opt_output != NULL) &&
        (strcmp(g_opt_output, "file.txt") == 0),
        "consume_long",
        "'--output file.txt' should consume next arg",
        _counter) && result;

    // test 3: consumed arg not re-processed
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "-o", "-v", "--verbose", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              4, argv,
                              test_unix_dispatch, NULL);
    }

    result = d_assert_standalone(
        ok &&
        (g_opt_output != NULL) &&
        (strcmp(g_opt_output, "-v") == 0) &&
        (g_opt_verbose == true) &&
        (g_unix_match_count == 2),
        "consume_skip",
        "consumed arg '-v' should not be processed as flag",
        _counter) && result;

    // test 4: "-vo file.txt" last char consumes
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "-vo", "file.txt", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              3, argv,
                              test_unix_dispatch, NULL);
    }

    result = d_assert_standalone(
        ok &&
        (g_opt_verbose == true) &&
        (g_opt_output != NULL) &&
        (strcmp(g_opt_output, "file.txt") == 0) &&
        (g_unix_match_count == 2),
        "combined_last_consumes",
        "'-vo file.txt' should toggle v, then o consumes file.txt",
        _counter) && result;

    return result;
}


/******************************************************************************
 * IV. STOP TOKEN TESTS
 *****************************************************************************/

/*
d_tests_sa_test_cli_unix_stop_token
  Tests the "--" stop token.
  Tests the following:
  - "--" stops option processing
  - args after "--" are not processed
  - flags before "--" are processed
*/
bool
d_tests_sa_test_cli_unix_stop_token
(
    struct d_test_counter* _counter
)
{
    bool result;
    bool ok;

    result = true;

    // test 1: "--" stops processing
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "--", "-v", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              3, argv,
                              test_unix_toggle, NULL);
    }

    result = d_assert_standalone(
        ok && (g_opt_verbose == false) && (g_unix_match_count == 0),
        "stop_basic",
        "'--' should stop processing; '-v' after it ignored",
        _counter) && result;

    // test 2: flags before "--" still processed
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "-v", "--", "--quiet", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              4, argv,
                              test_unix_toggle, NULL);
    }

    result = d_assert_standalone(
        ok &&
        (g_opt_verbose == true) &&
        (g_opt_quiet == true) &&
        (g_unix_match_count == 1),
        "stop_before_after",
        "flags before '--' processed, after '--' ignored",
        _counter) && result;

    return result;
}


/******************************************************************************
 * V. EDGE CASE TESTS
 *****************************************************************************/

/*
d_tests_sa_test_cli_unix_edge_cases
  Tests edge cases and miscellaneous behavior.
  Tests the following:
  - "-" alone is skipped as positional
  - positional args skipped
  - handler returns < 0 stops parsing
  - NULL handler silently consumes
  - NULL argv returns true
  - handler receives context
  - D_CLI_UNIX_PARSE macro works
  - mixed long, short, combined, equals in one command
*/
bool
d_tests_sa_test_cli_unix_edge_cases
(
    struct d_test_counter* _counter
)
{
    bool result;
    bool ok;

    result = true;

    // test 1: "-" alone is positional, skipped
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "-", "-v", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              3, argv,
                              test_unix_toggle, NULL);
    }

    result = d_assert_standalone(
        ok && (g_opt_verbose == true) && (g_unix_match_count == 1),
        "bare_dash_skipped",
        "'-' alone should be skipped (positional)",
        _counter) && result;

    // test 2: positional args skipped
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "positional", "-v", "another", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              4, argv,
                              test_unix_toggle, NULL);
    }

    result = d_assert_standalone(
        ok && (g_opt_verbose == true) && (g_unix_match_count == 1),
        "positional_skipped",
        "bare words should be skipped as positional args",
        _counter) && result;

    // test 3: handler stops parsing
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "-h", "-v", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              3, argv,
                              test_unix_stop, NULL);
    }

    result = d_assert_standalone(
        (!ok) && (g_unix_match_count == 1),
        "handler_stop",
        "handler returning < 0 should stop parsing",
        _counter) && result;

    // test 4: NULL handler silently consumes
    test_unix_reset();

    {
        char* argv[] = {"prog", "-v", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              2, argv,
                              NULL, NULL);
    }

    result = d_assert_standalone(
        ok && (g_opt_verbose == false),
        "null_handler",
        "NULL handler should silently consume (no toggle)",
        _counter) && result;

    // test 5: NULL argv returns true
    ok = d_cli_unix_parse(g_unix_entries,
                          D_CLI_COUNT(g_unix_entries),
                          0, NULL,
                          test_unix_toggle, NULL);
    result = d_assert_standalone(
        ok,
        "null_argv",
        "NULL argv should return true",
        _counter) && result;

    // test 6: handler receives context
    test_unix_reset();
    {
        int   ctx    = 0;
        char* argv[] = {"prog", "-v", "--quiet", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              3, argv,
                              test_unix_with_context, &ctx);

        result = d_assert_standalone(
            ok && (ctx == 2),
            "handler_context",
            "handler should receive context (ctx==2)",
            _counter) && result;
    }

    // test 7: D_CLI_UNIX_PARSE macro
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "--verbose", "-q", NULL};

        ok = D_CLI_UNIX_PARSE(g_unix_entries, 3, argv,
                              test_unix_toggle, NULL);
    }

    result = d_assert_standalone(
        ok &&
        (g_opt_verbose == true) &&
        (g_opt_quiet == false) &&
        (g_unix_match_count == 2),
        "macro_parse",
        "D_CLI_UNIX_PARSE should work on static array",
        _counter) && result;

    // test 8: mixed long, short, combined, equals in one command
    test_unix_reset();
    g_unix_match_count = 0;

    {
        char* argv[] = {"prog", "-xv", "--output=out.bin",
                         "--quiet", NULL};

        ok = d_cli_unix_parse(g_unix_entries,
                              D_CLI_COUNT(g_unix_entries),
                              4, argv,
                              test_unix_dispatch, NULL);
    }

    result = d_assert_standalone(
        ok &&
        (g_opt_extract == true) &&
        (g_opt_verbose == true) &&
        (strcmp(g_opt_output, "out.bin") == 0) &&
        (g_opt_quiet == false) &&
        (g_unix_match_count == 4),
        "mixed_all",
        "mixed styles in one command should all work",
        _counter) && result;

    return result;
}


/******************************************************************************
 * MODULE-LEVEL AGGREGATION
 *****************************************************************************/

bool
d_tests_sa_test_cli_unix_all
(
    struct d_test_counter* _counter
)
{
    bool result;

    result = true;

    printf("\n  [SECTION] CLI Unix\n");
    printf("  --------------------\n");

    result = d_tests_sa_test_cli_unix_long_option(_counter)    && result;
    result = d_tests_sa_test_cli_unix_long_equals(_counter)    && result;
    result = d_tests_sa_test_cli_unix_short_single(_counter)   && result;
    result = d_tests_sa_test_cli_unix_short_combined(_counter) && result;
    result = d_tests_sa_test_cli_unix_consume_next(_counter)   && result;
    result = d_tests_sa_test_cli_unix_stop_token(_counter)     && result;
    result = d_tests_sa_test_cli_unix_edge_cases(_counter)     && result;

    return result;
}
