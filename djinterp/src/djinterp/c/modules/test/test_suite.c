/*******************************************************************************
* djinterp [test]                                                   test_suite.c
*
* DTest's C face: the suite, its builders, the spec runner and the report.
*   A suite keeps its nodes (modules, blocks and tests) in one array, in the
* order they were opened, and the checks worth reporting in a second, in the
* order they were made. Every check goes to the test opened last, and opening a
* module or a block closes the open test, so the checks of any one test sit
* together, after its node and before the next test's: the report walks both
* arrays in step and needs no index.
*
*   Only failing checks are kept, unless the options ask for every check: a
* kit such as dckdint's makes six hundred thousand checks a configuration, and
* keeping each would cost tens of megabytes for lines nobody asked to see.
*
*
* path:      /src/djinterp/c/modules/test/test_suite.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.05
*******************************************************************************/
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // corresponding header

// std
#include <stdlib.h>  // malloc, realloc, free
#include <string.h>  // memcpy, strlen, strncmp, strstr
// djinterp
#include "../../../../../inc/djinterp/c/dckdint.h"  // d_ckd_mul, d_ckd_add


// D_INTERNAL_TEST_SUITE_NONE
//   constant: no node of this kind is open.
#define D_INTERNAL_TEST_SUITE_NONE ((size_t)-1)

// D_INTERNAL_TEST_SUITE_GROWTH
//   constant: the capacity an empty array first grows to.
#define D_INTERNAL_TEST_SUITE_GROWTH 16

// D_INTERNAL_TEST_SUITE_KIND_*
//   constant: what a node is.
#define D_INTERNAL_TEST_SUITE_KIND_MODULE 0
#define D_INTERNAL_TEST_SUITE_KIND_BLOCK  1
#define D_INTERNAL_TEST_SUITE_KIND_TEST   2

// dj_test_suite_node
//   struct: a module, block or test, with a test's tallies.
struct dj_test_suite_node
{
    int    kind;           // D_INTERNAL_TEST_SUITE_KIND_*
    char*  name;           // owned
    char*  descriptor;     // owned; may be NULL
    char*  skip_reason;    // owned; may be NULL
    bool   skipped;        // a test marked skipped
    size_t checks;         // a test's checks
    size_t checks_failed;  // a test's failed checks
};

// dj_test_suite_record
//   struct: a check kept for the report.
struct dj_test_suite_record
{
    size_t node;     // the test it was made against
    bool   result;   // whether it held
    char*  text;     // the expression, or the check's name; owned
    char*  message;  // owned; may be NULL
    char*  file;     // owned; may be NULL
    int    line;
};

// dj_test_suite
//   struct: the suite, defined here so that callers depend on its functions
// only.
struct dj_test_suite
{
    struct dj_test_suite_options options;
    struct dj_test_suite_node*   nodes;
    size_t                       node_count;
    size_t                       node_capacity;
    struct dj_test_suite_record* records;
    size_t                       record_count;
    size_t                       record_capacity;
    size_t                       open_module;
    size_t                       open_block;
    size_t                       open_test;
    size_t                       open_last;   // the node describe applies to
    bool                         error;       // a node or check was lost
};

// g_d_test_suite_current
//   variable: the suite a spec test is running in, for D_TEST_REQUIRE; NULL
// outside d_test_suite_add_module's call of a test function. Calls nest: each
// restores the suite it found.
static struct dj_test_suite* g_d_test_suite_current = NULL;


///////////////////////////////////////////////////////////////////////////////
///             I.    STORAGE                                               ///
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Copies a string for the suite to own.
 *
 * @param[in,out] _suite  the suite, whose error flag a failure sets.
 * @param[in]     _text   the string; `NULL` copies to `NULL`.
 * @return the copy, or `NULL` for `NULL` or when allocation failed.
 */
static char*
d_internal_test_suite_strdup(
    struct dj_test_suite* _suite,
    const char*           _text
)
{
    size_t length;
    char*  copy;

    if (!_text)
    {
        return NULL;
    }

    length = strlen(_text);
    copy   = (char*)malloc(length + 1);

    if (!copy)
    {
        _suite->error = true;

        return NULL;
    }

    memcpy(copy, _text, length + 1);

    return copy;
}

/**
 * @brief Makes room for one more element in a suite array.
 *
 * @param[in,out] _suite     the suite, whose error flag a failure sets.
 * @param[in,out] _array     the array's pointer.
 * @param[in]     _count     the elements it holds.
 * @param[in,out] _capacity  the elements it has room for.
 * @param[in]     _size      the size of one element.
 * @return `true` if one more fits, `false` if the array could not grow.
 *
 * Doubles the capacity, with both the doubling and the byte count taken
 * through d_ckd_*, so a count that cannot be represented fails as an
 * allocation would rather than wrapping.
 */
static bool
d_internal_test_suite_reserve(
    struct dj_test_suite* _suite,
    void**                _array,
    size_t                _count,
    size_t*               _capacity,
    size_t                _size
)
{
    size_t capacity;
    size_t bytes;
    void*  grown;

    if (_count < *_capacity)
    {
        return true;
    }

    capacity = D_INTERNAL_TEST_SUITE_GROWTH;

    if ( (*_capacity != 0) &&
         (d_ckd_mul(&capacity, *_capacity, (size_t)2)) )
    {
        _suite->error = true;

        return false;
    }

    if (d_ckd_mul(&bytes, capacity, _size))
    {
        _suite->error = true;

        return false;
    }

    grown = realloc(*_array, bytes);

    if (!grown)
    {
        _suite->error = true;

        return false;
    }

    *_array    = grown;
    *_capacity = capacity;

    return true;
}

/**
 * @brief Appends a node and makes it the open one of its kind.
 *
 * @param[in,out] _suite  the suite.
 * @param[in]     _kind   D_INTERNAL_TEST_SUITE_KIND_*.
 * @param[in]     _name   the node's name; copied; `NULL` reads as "".
 * @return `true` if the node was appended, `false` otherwise.
 *
 * Opening a module closes the open block and test; opening a block closes
 * the open test. That is what keeps each test's checks contiguous.
 */
static bool
d_internal_test_suite_open(
    struct dj_test_suite* _suite,
    int                   _kind,
    const char*           _name
)
{
    struct dj_test_suite_node* node;
    void*                      nodes;

    if (!_suite)
    {
        return false;
    }

    nodes = _suite->nodes;

    if (!d_internal_test_suite_reserve(_suite,
                                       &nodes,
                                       _suite->node_count,
                                       &_suite->node_capacity,
                                       sizeof(struct dj_test_suite_node)))
    {
        return false;
    }

    _suite->nodes = (struct dj_test_suite_node*)nodes;
    node          = &_suite->nodes[_suite->node_count];

    memset(node, 0, sizeof(*node));
    node->kind = _kind;
    node->name = d_internal_test_suite_strdup(_suite, _name ? _name : "");

    if (!node->name)
    {
        return false;
    }

    if (_kind == D_INTERNAL_TEST_SUITE_KIND_MODULE)
    {
        _suite->open_module = _suite->node_count;
        _suite->open_block  = D_INTERNAL_TEST_SUITE_NONE;
        _suite->open_test   = D_INTERNAL_TEST_SUITE_NONE;
    }
    else if (_kind == D_INTERNAL_TEST_SUITE_KIND_BLOCK)
    {
        _suite->open_block = _suite->node_count;
        _suite->open_test  = D_INTERNAL_TEST_SUITE_NONE;
    }
    else
    {
        _suite->open_test = _suite->node_count;
    }

    _suite->open_last = _suite->node_count;
    _suite->node_count++;

    return true;
}

/**
 * @brief Keeps a check for the report.
 *
 * @param[in,out] _suite    the suite.
 * @param[in]     _result   whether it held.
 * @param[in]     _text     the expression or name; copied.
 * @param[in]     _message  copied; may be `NULL`.
 * @param[in]     _file     copied; may be `NULL`.
 * @param[in]     _line     the source line.
 */
static void
d_internal_test_suite_keep(
    struct dj_test_suite* _suite,
    bool                  _result,
    const char*           _text,
    const char*           _message,
    const char*           _file,
    int                   _line
)
{
    struct dj_test_suite_record* record;
    void*                        records;

    records = _suite->records;

    if (!d_internal_test_suite_reserve(_suite,
                                       &records,
                                       _suite->record_count,
                                       &_suite->record_capacity,
                                       sizeof(struct dj_test_suite_record)))
    {
        return;
    }

    _suite->records = (struct dj_test_suite_record*)records;
    record          = &_suite->records[_suite->record_count];

    record->node    = _suite->open_test;
    record->result  = _result;
    record->text    = d_internal_test_suite_strdup(_suite, _text ? _text : "");
    record->message = d_internal_test_suite_strdup(_suite, _message);
    record->file    = d_internal_test_suite_strdup(_suite, _file);
    record->line    = _line;

    _suite->record_count++;

    return;
}


///////////////////////////////////////////////////////////////////////////////
///             II.   LIFETIME AND OPTIONS                                  ///
///////////////////////////////////////////////////////////////////////////////

/*
d_test_suite_new
  Every open index starts at NONE, so a check made before any test is open is
caught by d_test_suite_check rather than written to a node that is not there.
*/
struct dj_test_suite*
d_test_suite_new(void)
{
    struct dj_test_suite* suite;

    suite = (struct dj_test_suite*)malloc(sizeof(struct dj_test_suite));

    if (!suite)
    {
        return NULL;
    }

    memset(suite, 0, sizeof(*suite));
    d_test_suite_options_init(&suite->options);

    suite->open_module = D_INTERNAL_TEST_SUITE_NONE;
    suite->open_block  = D_INTERNAL_TEST_SUITE_NONE;
    suite->open_test   = D_INTERNAL_TEST_SUITE_NONE;
    suite->open_last   = D_INTERNAL_TEST_SUITE_NONE;

    return suite;
}

/*
d_test_suite_free
  Strings first, then the two arrays, then the suite: every string is owned by
exactly one node or record.
*/
void
d_test_suite_free(
    struct dj_test_suite* _suite
)
{
    size_t i;

    if (!_suite)
    {
        return;
    }

    for (i = 0; i < _suite->node_count; i++)
    {
        free(_suite->nodes[i].name);
        free(_suite->nodes[i].descriptor);
        free(_suite->nodes[i].skip_reason);
    }

    for (i = 0; i < _suite->record_count; i++)
    {
        free(_suite->records[i].text);
        free(_suite->records[i].message);
        free(_suite->records[i].file);
    }

    free(_suite->nodes);
    free(_suite->records);

    if (g_d_test_suite_current == _suite)
    {
        g_d_test_suite_current = NULL;
    }

    free(_suite);

    return;
}

/*
d_test_suite_options_init
  The table is on by default because the C++ entry points always print it:
the two faces' consoles then read alike.
*/
void
d_test_suite_options_init(
    struct dj_test_suite_options* _options
)
{
    if (!_options)
    {
        return;
    }

    _options->show   = D_TEST_SUITE_SHOW_TESTS;
    _options->table  = true;
    _options->color  = false;
    _options->stream = NULL;
    _options->filter = NULL;

    return;
}

/**
 * @brief Prints a runner's usage.
 *
 * @param[in] _out      where to print it.
 * @param[in] _program  the program's name; may be `NULL`.
 */
static void
d_internal_test_suite_usage(
    FILE*       _out,
    const char* _program
)
{
    fprintf(_out,
            "usage: %s [--failures | --tests | --checks] [--no-table]\n"
            "          [--filter=TEXT] [--help]\n"
            "  --failures     list only failing and skipped tests\n"
            "  --tests        list every test (the default)\n"
            "  --checks       list every check as well\n"
            "  --no-table     leave out each module's results table\n"
            "  --filter=TEXT  run only tests whose module/block/test path\n"
            "                 contains TEXT\n",
            (_program) ? _program : "tests");

    return;
}

/*
d_test_suite_options_parse
  Strict on purpose: a misspelt option that ran every test anyway would look
like a filter that matched everything. The usage goes to the options' stream
when one is set, so a suite that tests its own runner keeps it off the
console.
*/
int
d_test_suite_options_parse(
    struct dj_test_suite_options* _options,
    int                           _argc,
    char**                        _argv
)
{
    const char* arg;
    int         i;

    if ( (!_options) ||
         (_argc < 1) ||
         (!_argv) )
    {
        return 0;
    }

    for (i = 1; i < _argc; i++)
    {
        arg = _argv[i];

        if (!arg)
        {
            continue;
        }

        if (strcmp(arg, "--failures") == 0)
        {
            _options->show = D_TEST_SUITE_SHOW_FAILURES;
        }
        else if (strcmp(arg, "--tests") == 0)
        {
            _options->show = D_TEST_SUITE_SHOW_TESTS;
        }
        else if (strcmp(arg, "--checks") == 0)
        {
            _options->show = D_TEST_SUITE_SHOW_CHECKS;
        }
        else if (strcmp(arg, "--no-table") == 0)
        {
            _options->table = false;
        }
        else if (strncmp(arg, "--filter=", 9) == 0)
        {
            _options->filter = (arg[9] != '\0') ? (arg + 9) : NULL;
        }
        else if ( (strcmp(arg, "--help") == 0) ||
                  (strcmp(arg, "-h") == 0) )
        {
            d_internal_test_suite_usage(
                (_options->stream) ? _options->stream : stdout, _argv[0]);

            return 1;
        }
        else
        {
            fprintf((_options->stream) ? _options->stream : stderr,
                    "%s: unknown argument '%s'\n",
                    (_argv[0]) ? _argv[0] : "tests", arg);
            d_internal_test_suite_usage(
                (_options->stream) ? _options->stream : stderr, _argv[0]);

            return -1;
        }
    }

    return 0;
}

/*
d_test_suite_set_options
  A copy, so the caller's struct may go out of scope; the filter is a pointer
into the caller's text, which is why the contract asks it to outlive the run.
*/
void
d_test_suite_set_options(
    struct dj_test_suite*               _suite,
    const struct dj_test_suite_options* _options
)
{
    if ( (!_suite) ||
         (!_options) )
    {
        return;
    }

    _suite->options = *_options;

    return;
}


///////////////////////////////////////////////////////////////////////////////
///             III.  BUILDING                                              ///
///////////////////////////////////////////////////////////////////////////////

/*
d_test_suite_module
  Opening closes whatever block and test were open.
*/
bool
d_test_suite_module(
    struct dj_test_suite* _suite,
    const char*           _name
)
{
    return d_internal_test_suite_open(_suite,
                                      D_INTERNAL_TEST_SUITE_KIND_MODULE,
                                      _name);
}

/*
d_test_suite_block
  A block opened before any module still counts: the report shows it at the
top level.
*/
bool
d_test_suite_block(
    struct dj_test_suite* _suite,
    const char*           _name
)
{
    return d_internal_test_suite_open(_suite,
                                      D_INTERNAL_TEST_SUITE_KIND_BLOCK,
                                      _name);
}

/*
d_test_suite_test
  The test stays open until the next open of any kind.
*/
bool
d_test_suite_test(
    struct dj_test_suite* _suite,
    const char*           _name
)
{
    return d_internal_test_suite_open(_suite,
                                      D_INTERNAL_TEST_SUITE_KIND_TEST,
                                      _name);
}

/*
d_test_suite_describe
  Applies to the node opened last, whatever its kind, which is the order a
section writes them in: open, then describe.
*/
bool
d_test_suite_describe(
    struct dj_test_suite* _suite,
    const char*           _descriptor
)
{
    struct dj_test_suite_node* node;

    if ( (!_suite) ||
         (_suite->open_last == D_INTERNAL_TEST_SUITE_NONE) )
    {
        return false;
    }

    node = &_suite->nodes[_suite->open_last];
    free(node->descriptor);
    node->descriptor = d_internal_test_suite_strdup(_suite, _descriptor);

    return ( (node->descriptor != NULL) ||
             (_descriptor == NULL) );
}

/**
 * @brief Counts a check against the open test, opening one first if none is.
 *
 * @param[in,out] _suite    the suite.
 * @param[in]     _result   whether the check held.
 * @param[in]     _text     the expression or name.
 * @param[in]     _message  may be `NULL`.
 * @param[in]     _file     may be `NULL`.
 * @param[in]     _line     the source line.
 * @return `_result`.
 */
static bool
d_internal_test_suite_count(
    struct dj_test_suite* _suite,
    bool                  _result,
    const char*           _text,
    const char*           _message,
    const char*           _file,
    int                   _line
)
{
    struct dj_test_suite_node* test;

    if (!_suite)
    {
        if (!_result)
        {
            fprintf(stderr, "%s:%d: check failed (no suite): %s\n",
                    (_file) ? _file : "?", _line, (_text) ? _text : "");
        }

        return _result;
    }

    if ( (_suite->open_test == D_INTERNAL_TEST_SUITE_NONE) &&
         (!d_test_suite_test(_suite, _text)) )
    {
        return _result;
    }

    test = &_suite->nodes[_suite->open_test];
    test->checks++;

    if (!_result)
    {
        test->checks_failed++;
    }

    if ( (!_result) ||
         (_suite->options.show == D_TEST_SUITE_SHOW_CHECKS) )
    {
        d_internal_test_suite_keep(_suite, _result, _text, _message,
                                   _file, _line);
    }

    return _result;
}

/*
d_test_suite_check
  A thin wrapper: the expression is the check's text.
*/
bool
d_test_suite_check(
    struct dj_test_suite* _suite,
    bool                  _result,
    const char*           _expr,
    const char*           _file,
    int                   _line
)
{
    return d_internal_test_suite_count(_suite, _result, _expr, NULL,
                                       _file, _line);
}

/*
d_test_suite_check_msg
  As d_test_suite_check, with the leaf's name as the text and its message kept
beside it.
*/
bool
d_test_suite_check_msg(
    struct dj_test_suite* _suite,
    bool                  _result,
    const char*           _name,
    const char*           _message,
    const char*           _file,
    int                   _line
)
{
    return d_internal_test_suite_count(_suite, _result, _name, _message,
                                       _file, _line);
}

/*
d_test_suite_skip
  A skipped test keeps any checks it made before it was skipped; the report
counts it as skipped all the same, since the skip is the test's own verdict.
*/
bool
d_test_suite_skip(
    struct dj_test_suite* _suite,
    const char*           _reason
)
{
    struct dj_test_suite_node* test;

    if ( (!_suite) ||
         (_suite->open_test == D_INTERNAL_TEST_SUITE_NONE) )
    {
        return false;
    }

    test          = &_suite->nodes[_suite->open_test];
    test->skipped = true;
    free(test->skip_reason);
    test->skip_reason = d_internal_test_suite_strdup(_suite, _reason);

    return ( (test->skip_reason != NULL) ||
             (_reason == NULL) );
}

/*
d_test_suite_require
  The spec macros' way in: the suite is the one d_test_suite_add_module set,
or none, in which case d_internal_test_suite_count writes a failure to
stderr.
*/
bool
d_test_suite_require(
    bool        _result,
    const char* _expr,
    const char* _file,
    int         _line
)
{
    return d_internal_test_suite_count(g_d_test_suite_current, _result,
                                       _expr, NULL, _file, _line);
}

/*
d_test_suite_str_equal
  The standalone framework's d_test_str_equal, under the face's prefix; the
converter maps D_ASSERT_STR_EQUAL onto it.
*/
bool
d_test_suite_str_equal(
    const char* _a,
    const char* _b
)
{
    if ( (!_a) ||
         (!_b) )
    {
        return (_a == _b);
    }

    return (strcmp(_a, _b) == 0);
}

/*
d_test_suite_skip_current
  Outside a spec run there is no test to mark, and nothing to say.
*/
void
d_test_suite_skip_current(
    const char* _reason
)
{
    if (g_d_test_suite_current)
    {
        d_test_suite_skip(g_d_test_suite_current, _reason);
    }

    return;
}


///////////////////////////////////////////////////////////////////////////////
///             IV.   SPECS                                                 ///
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief Whether a spec test's path passes the options' filter.
 *
 * @param[in] _filter  the filter text; `NULL` passes everything.
 * @param[in] _module  the module's name; may be `NULL`.
 * @param[in] _block   the block's name; may be `NULL`.
 * @param[in] _test    the test's name; may be `NULL`.
 * @return `true` if the test runs.
 *
 * The path is module/block/test, so a filter can name a whole module, a block
 * within it, or one test. A path too long for the buffer is matched name by
 * name instead, which can only widen the match.
 */
static bool
d_internal_test_suite_selected(
    const char* _filter,
    const char* _module,
    const char* _block,
    const char* _test
)
{
    char path[512];
    int  written;

    if (!_filter)
    {
        return true;
    }

    written = snprintf(path, sizeof(path), "%s/%s/%s",
                       (_module) ? _module : "",
                       (_block) ? _block : "",
                       (_test) ? _test : "");

    if ( (written > 0) &&
         ((size_t)written < sizeof(path)) )
    {
        return (strstr(path, _filter) != NULL);
    }

    return ( ((_module) && (strstr(_module, _filter))) ||
             ((_block) && (strstr(_block, _filter))) ||
             ((_test) && (strstr(_test, _filter))) );
}

/**
 * @brief Runs one spec test as the open test.
 *
 * @param[in,out] _suite  the suite, with the test just opened.
 * @param[in]     _case   the row.
 *
 * The function's return is the test's own verdict: false fails it even when
 * every require held, and a test that made no require is counted as one check
 * of that return, which is what a C++ spec test's bool is.
 */
static void
d_internal_test_suite_run_case(
    struct dj_test_suite*           _suite,
    const struct dj_test_case_spec* _case
)
{
    struct dj_test_suite_node* test;
    struct dj_test_suite*      outer;
    size_t                     checks;
    size_t                     failed;
    bool                       returned;

    test   = &_suite->nodes[_suite->open_test];
    checks = test->checks;
    failed = test->checks_failed;

    if (!_case->fn)
    {
        d_internal_test_suite_count(_suite, false, "no test function", NULL,
                                    NULL, 0);

        return;
    }

    // restored, not cleared: a test may run a suite of its own (DTest's own
    // suite does), and the outer test's requires must still reach the outer
    outer                  = g_d_test_suite_current;
    g_d_test_suite_current = _suite;
    returned               = _case->fn();
    g_d_test_suite_current = outer;

    // the function may have grown the array: take the node again
    test = &_suite->nodes[_suite->open_test];

    if ( (!returned) &&
         (test->checks_failed == failed) )
    {
        d_internal_test_suite_count(_suite, false, "the test returned false",
                                    NULL, NULL, 0);
    }
    else if ( (returned) &&
              (test->checks == checks) &&
              (!test->skipped) )
    {
        d_internal_test_suite_count(_suite, true, "the test returned true",
                                    NULL, NULL, 0);
    }

    return;
}

/*
d_test_suite_add_module
  Modules and blocks open lazily, at their first selected test, so a filter
leaves no empty headings behind it.
*/
bool
d_test_suite_add_module(
    struct dj_test_suite*             _suite,
    const struct dj_test_module_spec* _module
)
{
    const struct dj_test_block_spec* block;
    const struct dj_test_case_spec*  test;
    bool                             module_open;
    bool                             block_open;
    size_t                           b;
    size_t                           t;

    if ( (!_suite) ||
         (!_module) )
    {
        return false;
    }

    module_open = false;

    for (b = 0; b < _module->count; b++)
    {
        block      = &_module->blocks[b];
        block_open = false;

        for (t = 0; t < block->count; t++)
        {
            test = &block->tests[t];

            if (!d_internal_test_suite_selected(_suite->options.filter,
                                                _module->name,
                                                block->name,
                                                test->name))
            {
                continue;
            }

            if (!module_open)
            {
                module_open = d_test_suite_module(_suite, _module->name) &&
                              d_test_suite_describe(_suite,
                                                    _module->descriptor);
            }

            if (!block_open)
            {
                block_open = d_test_suite_block(_suite, block->name) &&
                             d_test_suite_describe(_suite, block->descriptor);
            }

            if ( (!module_open) ||
                 (!block_open) ||
                 (!d_test_suite_test(_suite, test->name)) ||
                 (!d_test_suite_describe(_suite, test->descriptor)) )
            {
                return false;
            }

            d_internal_test_suite_run_case(_suite, test);
        }
    }

    return (!_suite->error);
}


///////////////////////////////////////////////////////////////////////////////
///             V.    REPORT                                                ///
///////////////////////////////////////////////////////////////////////////////

/**
 * @brief A test's verdict word, as the report and the table print it.
 *
 * @param[in] _test  a test node.
 * @return "SKIP", "FAIL" (a failed check, or no check at all) or "PASS".
 */
static const char*
d_internal_test_suite_verdict(
    const struct dj_test_suite_node* _test
)
{
    if (_test->skipped)
    {
        return "SKIP";
    }

    if ( (_test->checks_failed != 0) ||
         (_test->checks == 0) )
    {
        return "FAIL";
    }

    return "PASS";
}

/**
 * @brief Counts what a suite recorded.
 *
 * @param[in]  _suite    the suite.
 * @param[out] _summary  the counts.
 */
static void
d_internal_test_suite_tally(
    const struct dj_test_suite* _suite,
    struct dj_test_summary*     _summary
)
{
    const struct dj_test_suite_node* node;
    size_t                           i;

    memset(_summary, 0, sizeof(*_summary));

    for (i = 0; i < _suite->node_count; i++)
    {
        node = &_suite->nodes[i];

        if (node->kind == D_INTERNAL_TEST_SUITE_KIND_MODULE)
        {
            _summary->modules++;
        }
        else if (node->kind == D_INTERNAL_TEST_SUITE_KIND_TEST)
        {
            _summary->tests++;
            _summary->checks        += node->checks;
            _summary->checks_failed += node->checks_failed;

            if (node->skipped)
            {
                _summary->tests_skipped++;
            }
            else if (d_internal_test_suite_verdict(node)[0] == 'F')
            {
                _summary->tests_failed++;
            }
        }
    }

    return;
}

/**
 * @brief Prints one kept check beneath its test.
 *
 * @param[in] _out     the stream.
 * @param[in] _record  the check.
 */
static void
d_internal_test_suite_print_record(
    FILE*                              _out,
    const struct dj_test_suite_record* _record
)
{
    fprintf(_out, "          %s ", (_record->result) ? "ok  " : "FAIL");

    if (_record->file)
    {
        fprintf(_out, "%s:%d: ", _record->file, _record->line);
    }

    fprintf(_out, "%s", (_record->text) ? _record->text : "");

    if (_record->message)
    {
        fprintf(_out, " -- %s", _record->message);
    }

    fputc('\n', _out);

    return;
}

/**
 * @brief Whether a heading holds a test the report lists.
 *
 * @param[in] _suite    the suite.
 * @param[in] _heading  a module or block node.
 * @return `true` when the options list every test, or when a test beneath the
 *         heading, before the next heading of its rank or above, did not pass.
 */
static bool
d_internal_test_suite_heading_listed(
    const struct dj_test_suite* _suite,
    size_t                      _heading
)
{
    const struct dj_test_suite_node* node;
    int                              rank;
    size_t                           i;

    if (_suite->options.show != D_TEST_SUITE_SHOW_FAILURES)
    {
        return true;
    }

    rank = _suite->nodes[_heading].kind;

    for (i = _heading + 1; i < _suite->node_count; i++)
    {
        node = &_suite->nodes[i];

        if (node->kind <= rank)
        {
            break;
        }

        if ( (node->kind == D_INTERNAL_TEST_SUITE_KIND_TEST) &&
             (d_internal_test_suite_verdict(node)[0] != 'P') )
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Prints the tree: headings, tests and their kept checks.
 *
 * @param[in] _suite  the suite.
 * @param[in] _out    the stream.
 *
 * Walks the nodes and the records in step; a record belongs to the test it
 * names, and every record of a test follows that test's node.
 */
static void
d_internal_test_suite_print_tree(
    const struct dj_test_suite* _suite,
    FILE*                       _out
)
{
    const struct dj_test_suite_node* node;
    const char*                      word;
    size_t                           i;
    size_t                           r;
    bool                             listed;

    r = 0;

    for (i = 0; i < _suite->node_count; i++)
    {
        node = &_suite->nodes[i];

        if (node->kind == D_INTERNAL_TEST_SUITE_KIND_MODULE)
        {
            if (d_internal_test_suite_heading_listed(_suite, i))
            {
                fprintf(_out, "\nmodule %s%s%s\n", node->name,
                        (node->descriptor) ? " -- " : "",
                        (node->descriptor) ? node->descriptor : "");
            }

            continue;
        }

        if (node->kind == D_INTERNAL_TEST_SUITE_KIND_BLOCK)
        {
            if (d_internal_test_suite_heading_listed(_suite, i))
            {
                fprintf(_out, "  block %s\n", node->name);
            }

            continue;
        }

        word   = d_internal_test_suite_verdict(node);
        listed = ( (_suite->options.show != D_TEST_SUITE_SHOW_FAILURES) ||
                   (word[0] != 'P') );

        if (listed)
        {
            fprintf(_out, "    [%s] %s", word, node->name);

            if (node->skipped)
            {
                fprintf(_out, ": %s",
                        (node->skip_reason) ? node->skip_reason : "skipped");
            }
            else if (node->checks == 0)
            {
                fprintf(_out, ": no checks");
            }

            fputc('\n', _out);
        }

        while ( (r < _suite->record_count) &&
                (_suite->records[r].node == i) )
        {
            if (listed)
            {
                d_internal_test_suite_print_record(_out, &_suite->records[r]);
            }

            r++;
        }
    }

    return;
}

/**
 * @brief The length of a table cell once escaped.
 *
 * @param[in] _text  the cell's text; may be `NULL`.
 * @return its length without the carriage returns the escape drops.
 */
static size_t
d_internal_test_suite_cell_length(
    const char* _text
)
{
    size_t length;

    length = 0;

    while ( (_text) &&
            (*_text) )
    {
        if (*_text != '\r')
        {
            length++;
        }

        _text++;
    }

    return length;
}

/**
 * @brief Prints a table cell, escaped as test_defaults.hpp's pipe_escape
 *        does, and padded to a width: "| " text padding " ".
 *
 * @param[in] _out    the stream.
 * @param[in] _text   the cell's text; may be `NULL`.
 * @param[in] _width  the column's width.
 */
static void
d_internal_test_suite_cell(
    FILE*       _out,
    const char* _text,
    size_t      _width
)
{
    size_t length;

    fputs("| ", _out);
    length = 0;

    while ( (_text) &&
            (*_text) )
    {
        if (*_text == '|')
        {
            fputc('/', _out);
            length++;
        }
        else if (*_text == '\n')
        {
            fputc(' ', _out);
            length++;
        }
        else if (*_text != '\r')
        {
            fputc(*_text, _out);
            length++;
        }

        _text++;
    }

    for (; length < _width; length++)
    {
        fputc(' ', _out);
    }

    fputc(' ', _out);

    return;
}

/**
 * @brief Prints one module's results table, in test_defaults.hpp's columns:
 *        RESULT | # | TEST | DESCRIPTION.
 *
 * @param[in] _suite  the suite.
 * @param[in] _out    the stream.
 * @param[in] _first  the module's node.
 * @param[in] _end    the node after the module's last.
 *
 * Two passes over the module's tests: the widths, then the rows. The caption
 * and the rule are C++'s, so a reader that slices one face's table out of a
 * console stream slices the other's too.
 */
static void
d_internal_test_suite_print_table(
    const struct dj_test_suite* _suite,
    FILE*                       _out,
    size_t                      _first,
    size_t                      _end
)
{
    const struct dj_test_suite_node* node;
    char                             number[24];
    size_t                           width[4];
    size_t                           rule;
    size_t                           n;
    size_t                           i;

    width[0] = 6;   // "RESULT"
    width[1] = 1;   // "#"
    width[2] = 4;   // "TEST"
    width[3] = 11;  // "DESCRIPTION"
    n        = 0;

    for (i = _first; i < _end; i++)
    {
        node = &_suite->nodes[i];

        if (node->kind != D_INTERNAL_TEST_SUITE_KIND_TEST)
        {
            continue;
        }

        n++;
        sprintf(number, "%lu", (unsigned long)n);

        if (strlen(number) > width[1])
        {
            width[1] = strlen(number);
        }

        if (d_internal_test_suite_cell_length(node->name) > width[2])
        {
            width[2] = d_internal_test_suite_cell_length(node->name);
        }

        if (d_internal_test_suite_cell_length(node->descriptor) > width[3])
        {
            width[3] = d_internal_test_suite_cell_length(node->descriptor);
        }
    }

    fprintf(_out, "\n|-- results table (%s) --|\n", _suite->nodes[_first].name);
    d_internal_test_suite_cell(_out, "RESULT", width[0]);
    d_internal_test_suite_cell(_out, "#", width[1]);
    d_internal_test_suite_cell(_out, "TEST", width[2]);
    d_internal_test_suite_cell(_out, "DESCRIPTION", width[3]);
    fputs("|\n|", _out);

    // the header row's length less "|", its final "|" and the newline
    rule = (width[0] + width[1] + width[2] + width[3]) + (4 * 3) + 1 - 2;

    for (i = 0; i < rule; i++)
    {
        fputc('-', _out);
    }

    fputs("|\n", _out);
    n = 0;

    for (i = _first; i < _end; i++)
    {
        node = &_suite->nodes[i];

        if (node->kind != D_INTERNAL_TEST_SUITE_KIND_TEST)
        {
            continue;
        }

        n++;
        sprintf(number, "%lu", (unsigned long)n);
        d_internal_test_suite_cell(_out, d_internal_test_suite_verdict(node),
                                   width[0]);
        d_internal_test_suite_cell(_out, number, width[1]);
        d_internal_test_suite_cell(_out, node->name, width[2]);
        d_internal_test_suite_cell(_out, node->descriptor, width[3]);
        fputs("|\n", _out);
    }

    return;
}

/**
 * @brief Prints every module's results table.
 *
 * @param[in] _suite  the suite.
 * @param[in] _out    the stream.
 */
static void
d_internal_test_suite_print_tables(
    const struct dj_test_suite* _suite,
    FILE*                       _out
)
{
    size_t first;
    size_t i;

    first = D_INTERNAL_TEST_SUITE_NONE;

    for (i = 0; i <= _suite->node_count; i++)
    {
        if ( (i < _suite->node_count) &&
             (_suite->nodes[i].kind != D_INTERNAL_TEST_SUITE_KIND_MODULE) )
        {
            continue;
        }

        if (first != D_INTERNAL_TEST_SUITE_NONE)
        {
            d_internal_test_suite_print_table(_suite, _out, first, i);
        }

        first = i;
    }

    return;
}

/*
d_test_suite_run
  A run with no test at all fails: a runner that ran nothing has proved
nothing, and saying PASSED would hide a broken spec or an overeager filter.
*/
int
d_test_suite_run(
    const struct dj_test_suite* _suite,
    struct dj_test_summary*     _summary
)
{
    struct dj_test_summary summary;
    FILE*                  out;
    int                    code;

    if (!_suite)
    {
        return D_TEST_SUITE_EXIT_ERROR;
    }

    out = (_suite->options.stream) ? _suite->options.stream : stdout;

    d_internal_test_suite_tally(_suite, &summary);
    d_internal_test_suite_print_tree(_suite, out);

    if (_suite->options.table)
    {
        d_internal_test_suite_print_tables(_suite, out);
    }

    if (_suite->error)
    {
        code = D_TEST_SUITE_EXIT_ERROR;
    }
    else if ( (summary.tests_failed != 0) ||
              (summary.tests == 0) )
    {
        code = D_TEST_SUITE_EXIT_FAILED;
    }
    else
    {
        code = D_TEST_SUITE_EXIT_PASSED;
    }

    fprintf(out,
            "\n-- summary --\n"
            "  modules: %lu\n"
            "  tests:   %lu (%lu passed, %lu failed, %lu skipped)\n"
            "  checks:  %lu (%lu failed)\n"
            "  result:  %s\n",
            (unsigned long)summary.modules,
            (unsigned long)summary.tests,
            (unsigned long)(summary.tests - summary.tests_failed
                                          - summary.tests_skipped),
            (unsigned long)summary.tests_failed,
            (unsigned long)summary.tests_skipped,
            (unsigned long)summary.checks,
            (unsigned long)summary.checks_failed,
            (code == D_TEST_SUITE_EXIT_PASSED) ? "PASSED"
            : (code == D_TEST_SUITE_EXIT_FAILED) ? "FAILED"
            : "ERROR (a node or check could not be recorded)");
    fflush(out);

    if (_summary)
    {
        *_summary = summary;
    }

    return code;
}

/*
d_test_run_modules
  Every module is added even after one fails to record, so the report shows
as much as the suite managed to hold.
*/
int
d_test_run_modules(
    int                                      _argc,
    char**                                   _argv,
    const char*                              _title,
    const struct dj_test_module_spec* const* _modules,
    size_t                                   _count
)
{
    struct dj_test_suite_options options;
    struct dj_test_suite*        suite;
    size_t                       i;
    int                          parsed;
    int                          code;

    d_test_suite_options_init(&options);
    parsed = d_test_suite_options_parse(&options, _argc, _argv);

    if (parsed != 0)
    {
        return (parsed > 0) ? D_TEST_SUITE_EXIT_PASSED
                            : D_TEST_SUITE_EXIT_ERROR;
    }

    suite = d_test_suite_new();

    if (!suite)
    {
        fprintf(stderr, "fatal: could not create the test suite\n");

        return D_TEST_SUITE_EXIT_ERROR;
    }

    d_test_suite_set_options(suite, &options);
    printf("== %s ==\n", (_title) ? _title : "tests");

    for (i = 0; i < _count; i++)
    {
        if (_modules)
        {
            d_test_suite_add_module(suite, _modules[i]);
        }
    }

    code = d_test_suite_run(suite, NULL);
    d_test_suite_free(suite);

    return code;
}
