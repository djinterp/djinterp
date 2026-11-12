/*******************************************************************************
* djinterp [c][test]                                                test_suite.c
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
*   Notes go in the second array too, each against the node opened last,
* which is the open test when one is open; so the array stays in node order,
* a heading's notes sit after its node and before the next node's records, and
* the report's walk takes them in the same step. Every note is kept: whether
* it is printed depends on a verdict not known until the run ends.
*
*
* path:      /src/djinterp/c/modules/test/test_suite.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.08
*******************************************************************************/
#include "../../../../../inc/djinterp/c/modules/test/test_suite.h"  // corresponding header

// std
#include <stdlib.h>  // malloc, realloc, free
#include <string.h>  // memcpy, strchr, strlen, strncmp, strstr
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
//   struct: a check or a note kept for the report.
struct dj_test_suite_record
{
    size_t node;     // the test it was made against; a note's node, any kind
    bool   note;     // a note, whose text is all it holds
    bool   result;   // whether it held
    char*  text;     // the expression, the check's name, or the note; owned
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

//==============================================================================
// 1.  STORAGE
//==============================================================================

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
    // a NULL string copies to NULL
    if (!_text)
    {
        return NULL;
    }

    const size_t length = strlen(_text);
    char*        copy   = (char*)malloc(length + 1);

    // allocation failed: the suite records the error
    if (!copy)
    {
        _suite->error = true;

        return NULL;
    }

    memcpy(copy,
           _text,
           length + 1);

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
    // room for one more already
    if (_count < *_capacity)
    {
        return true;
    }

    size_t capacity = D_INTERNAL_TEST_SUITE_GROWTH;

    // start at the growth step, or double; a doubling that overflows fails
    if ( (*_capacity != 0) &&
         (d_ckd_mul(&capacity,
                    *_capacity,
                    (size_t)2)) )
    {
        _suite->error = true;

        return false;
    }

    size_t bytes = 0;

    // the byte count must be representable too
    if (d_ckd_mul(&bytes,
                  capacity,
                  _size))
    {
        _suite->error = true;

        return false;
    }

    void* grown = realloc(*_array,
                          bytes);

    // realloc failed: the array is as it was
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
    // parameter validation
    if (!_suite)
    {
        return false;
    }

    void* nodes = _suite->nodes;

    // make room for the node
    if (!d_internal_test_suite_reserve(_suite,
                                       &nodes,
                                       _suite->node_count,
                                       &_suite->node_capacity,
                                       sizeof(struct dj_test_suite_node)))
    {
        return false;
    }

    _suite->nodes = (struct dj_test_suite_node*)nodes;

    struct dj_test_suite_node* node = &_suite->nodes[_suite->node_count];

    memset(node,
           0,
           sizeof(*node));
    node->kind = _kind;
    node->name = d_internal_test_suite_strdup(_suite,
                                              _name ? _name : "");

    // the name could not be copied
    if (!node->name)
    {
        return false;
    }

    // a module closes the open block and test; a block, the open test
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
 * @brief Appends an empty record.
 *
 * @param[in,out] _suite  the suite, whose error flag a failure sets.
 * @return the record, zeroed, or `NULL` if the array could not grow.
 */
static struct dj_test_suite_record*
d_internal_test_suite_append(
    struct dj_test_suite* _suite
)
{
    void* records = _suite->records;

    // make room for the record
    if (!d_internal_test_suite_reserve(_suite,
                                       &records,
                                       _suite->record_count,
                                       &_suite->record_capacity,
                                       sizeof(struct dj_test_suite_record)))
    {
        return NULL;
    }

    _suite->records = (struct dj_test_suite_record*)records;

    struct dj_test_suite_record* record =
        &_suite->records[_suite->record_count];

    memset(record,
           0,
           sizeof(*record));
    _suite->record_count++;

    return record;
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
    struct dj_test_suite_record* record = d_internal_test_suite_append(_suite);

    // no room for the check: the suite records the error
    if (!record)
    {
        return;
    }

    record->node    = _suite->open_test;
    record->result  = _result;
    record->text    = d_internal_test_suite_strdup(_suite,
                                                   _text ? _text : "");
    record->message = d_internal_test_suite_strdup(_suite,
                                                   _message);
    record->file    = d_internal_test_suite_strdup(_suite,
                                                   _file);
    record->line    = _line;

    return;
}

/**
 * @brief Keeps a note against the node opened last.
 *
 * @param[in,out] _suite  the suite; a node is open.
 * @param[in]     _text   the note, malloc'd; the suite takes it, kept or not.
 * @return `true` if kept, `false` if the array could not grow.
 */
static bool
d_internal_test_suite_keep_note(
    struct dj_test_suite* _suite,
    char*                 _text
)
{
    struct dj_test_suite_record* record = d_internal_test_suite_append(_suite);

    // no room for the note: its text is freed
    if (!record)
    {
        free(_text);

        return false;
    }

    record->node   = _suite->open_last;
    record->note   = true;
    record->result = true;
    record->text   = _text;

    return true;
}

//==============================================================================
// 2.  LIFETIME AND OPTIONS
//==============================================================================

/*
d_test_suite_new
  Every open index starts at NONE, so a check made before any test is open is
caught by d_test_suite_check rather than written to a node that is not there.
*/
struct dj_test_suite*
d_test_suite_new(void)
{
    struct dj_test_suite* suite = malloc(sizeof(*suite));

    // allocation failed
    if (!suite)
    {
        return NULL;
    }

    memset(suite,
           0,
           sizeof(*suite));
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
    // parameter validation
    if (!_suite)
    {
        return;
    }

    // every node's strings
    for (size_t i = 0; i < _suite->node_count; i++)
    {
        free(_suite->nodes[i].name);
        free(_suite->nodes[i].descriptor);
        free(_suite->nodes[i].skip_reason);
    }

    // every record's strings
    for (size_t i = 0; i < _suite->record_count; i++)
    {
        free(_suite->records[i].text);
        free(_suite->records[i].message);
        free(_suite->records[i].file);
    }

    free(_suite->nodes);
    free(_suite->records);

    // a spec run in progress must not reach a freed suite
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
    // parameter validation
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
    // parameter validation
    if ( (!_options) ||
         (_argc < 1) ||
         (!_argv) )
    {
        return 0;
    }

    const char* arg = NULL;

    // each argument after the program's name
    for (int i = 1; i < _argc; i++)
    {
        arg = _argv[i];

        // a NULL argument is skipped, not refused
        if (!arg)
        {
            continue;
        }

        // the report's detail, the table, the filter, the usage
        if (strcmp(arg,
                   "--failures") == 0)
        {
            _options->show = D_TEST_SUITE_SHOW_FAILURES;
        }
        else if (strcmp(arg,
                        "--tests") == 0)
        {
            _options->show = D_TEST_SUITE_SHOW_TESTS;
        }
        else if (strcmp(arg,
                        "--checks") == 0)
        {
            _options->show = D_TEST_SUITE_SHOW_CHECKS;
        }
        else if (strcmp(arg,
                        "--no-table") == 0)
        {
            _options->table = false;
        }
        else if (strncmp(arg,
                         "--filter=",
                         9) == 0)
        {
            _options->filter = (arg[9] != '\0') ? (arg + 9) : NULL;
        }
        else if ( (strcmp(arg,
                          "--help") == 0) ||
                  (strcmp(arg,
                          "-h") == 0) )
        {
            FILE* const help = (_options->stream) ? _options->stream : stdout;

            d_internal_test_suite_usage(help,
                                        _argv[0]);

            return 1;
        }
        else
        {
            fprintf((_options->stream) ? _options->stream : stderr,
                    "%s: unknown argument '%s'\n",
                    (_argv[0]) ? _argv[0] : "tests",
                    arg);

            FILE* const usage = (_options->stream) ? _options->stream : stderr;

            d_internal_test_suite_usage(usage,
                                        _argv[0]);

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
    // parameter validation
    if ( (!_suite) ||
         (!_options) )
    {
        return;
    }

    _suite->options = *_options;

    return;
}

//==============================================================================
// 3.  BUILDING
//==============================================================================

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
    // parameter validation; a descriptor needs a node
    if ( (!_suite) ||
         (_suite->open_last == D_INTERNAL_TEST_SUITE_NONE) )
    {
        return false;
    }

    struct dj_test_suite_node* node = &_suite->nodes[_suite->open_last];

    free(node->descriptor);
    node->descriptor = d_internal_test_suite_strdup(_suite,
                                                    _descriptor);

    return ( (node->descriptor != NULL) ||
             (_descriptor == NULL) );
}

/*
d_test_suite_vnote
  Measures, then formats into a buffer of that size: a note has no length
limit, and nothing is truncated. A format vsnprintf refuses records nothing
and is not a suite error, since the checks are unaffected.
*/
bool
d_test_suite_vnote(
    struct dj_test_suite* _suite,
    const char*           _format,
    va_list               _args
)
{
    va_list measure;

    // parameter validation; a note needs a node
    if ( (!_suite)  ||
         (!_format) ||
         (_suite->open_last == D_INTERNAL_TEST_SUITE_NONE) )
    {
        return false;
    }

    va_copy(measure,
            _args);

    const int length = vsnprintf(NULL,
                                 0,
                                 _format,
                                 measure);

    va_end(measure);

    // a format vsnprintf refuses records nothing
    if (length < 0)
    {
        return false;
    }

    char* text = (char*)malloc((size_t)length + 1);

    // allocation failed: the suite records the error
    if (!text)
    {
        _suite->error = true;

        return false;
    }

    // a second pass that disagrees with the first records nothing
    if (vsnprintf(text,
                  (size_t)length + 1,
                  _format,
                  _args) != length)
    {
        free(text);

        return false;
    }

    return d_internal_test_suite_keep_note(_suite,
                                           text);
}

/*
d_test_suite_note
  The variadic form of d_test_suite_vnote.
*/
bool
d_test_suite_note(
    struct dj_test_suite* _suite,
    const char*           _format,
    ...
)
{
    va_list args;

    va_start(args,
             _format);

    const bool recorded = d_test_suite_vnote(_suite,
                                             _format,
                                             args);

    va_end(args);

    return recorded;
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
    // no suite: a failure has nowhere to go but stderr
    if (!_suite)
    {
        // a passing check without a suite says nothing
        if (!_result)
        {
            fprintf(stderr,
                    "%s:%d: check failed (no suite): %s\n",
                    (_file) ? _file : "?",
                    _line,
                    (_text) ? _text : "");
        }

        return _result;
    }

    // a check with no test open opens one named after it
    if ( (_suite->open_test == D_INTERNAL_TEST_SUITE_NONE) &&
         (!d_test_suite_test(_suite,
                             _text)) )
    {
        return _result;
    }

    struct dj_test_suite_node* test = &_suite->nodes[_suite->open_test];

    test->checks++;

    // a failure counts against the test
    if (!_result)
    {
        test->checks_failed++;
    }

    // a failing check is always kept; a passing one at --checks
    if ( (!_result) ||
         (_suite->options.show == D_TEST_SUITE_SHOW_CHECKS) )
    {
        d_internal_test_suite_keep(_suite,
                                   _result,
                                   _text,
                                   _message,
                                   _file,
                                   _line);
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
    return d_internal_test_suite_count(_suite,
                                       _result,
                                       _expr,
                                       NULL,
                                       _file,
                                       _line);
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
    return d_internal_test_suite_count(_suite,
                                       _result,
                                       _name,
                                       _message,
                                       _file,
                                       _line);
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
    // parameter validation; skipping needs an open test
    if ( (!_suite) ||
         (_suite->open_test == D_INTERNAL_TEST_SUITE_NONE) )
    {
        return false;
    }

    struct dj_test_suite_node* test = &_suite->nodes[_suite->open_test];

    test->skipped = true;
    free(test->skip_reason);
    test->skip_reason = d_internal_test_suite_strdup(_suite,
                                                     _reason);

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
    return d_internal_test_suite_count(g_d_test_suite_current,
                                       _result,
                                       _expr,
                                       NULL,
                                       _file,
                                       _line);
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
    // two NULLs are equal; one NULL is not
    if ( (!_a) ||
         (!_b) )
    {
        return (_a == _b);
    }

    return (strcmp(_a,
                   _b) == 0);
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
    // outside a spec run there is no test to mark
    if (g_d_test_suite_current)
    {
        d_test_suite_skip(g_d_test_suite_current,
                          _reason);
    }

    return;
}

/*
d_test_suite_note_current
  Outside a spec run there is no suite, and d_test_suite_vnote refuses a NULL
one, so the note is dropped.
*/
bool
d_test_suite_note_current(
    const char* _format,
    ...
)
{
    va_list args;

    va_start(args,
             _format);

    const bool recorded = d_test_suite_vnote(g_d_test_suite_current,
                                             _format,
                                             args);

    va_end(args);

    return recorded;
}

//==============================================================================
// 4.  SPECS
//==============================================================================

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
    // no filter selects everything
    if (!_filter)
    {
        return true;
    }

    char path[512] = {0};

    const int written = snprintf(path,
                                 sizeof(path),
                                 "%s/%s/%s",
                                 (_module) ? _module : "",
                                 (_block) ? _block : "",
                                 (_test) ? _test : "");

    // a path too long for the buffer is not selected
    if ( (written > 0) &&
         ((size_t)written < sizeof(path)) )
    {
        return (strstr(path,
                       _filter) != NULL);
    }

    return ( ( (_module) &&
               (strstr(_module,
                       _filter)) ) ||
             ( (_block) &&
               (strstr(_block,
                       _filter)) ) ||
             ( (_test) &&
               (strstr(_test,
                       _filter)) ) );
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
    struct dj_test_suite_node* test   = &_suite->nodes[_suite->open_test];
    const size_t               checks = test->checks;
    const size_t               failed = test->checks_failed;

    // a row with no function fails its test
    if (!_case->fn)
    {
        d_internal_test_suite_count(_suite,
                                    false,
                                    "no test function",
                                    NULL,
                                    NULL,
                                    0);

        return;
    }

    // restored, not cleared: a test may run a suite of its own (DTest's own
    // suite does), and the outer test's requires must still reach the outer
    struct dj_test_suite* outer = g_d_test_suite_current;

    g_d_test_suite_current = _suite;

    const bool returned = _case->fn();

    g_d_test_suite_current = outer;

    // the function may have grown the array: take the node again
    test = &_suite->nodes[_suite->open_test];

    // returning false fails the test, unless a require already did
    if ( (!returned) &&
         (test->checks_failed == failed) )
    {
        d_internal_test_suite_count(_suite,
                                    false,
                                    "the test returned false",
                                    NULL,
                                    NULL,
                                    0);
    }
    else if ( (returned)               &&
              (test->checks == checks) &&
              (!test->skipped) )
    {
        d_internal_test_suite_count(_suite,
                                    true,
                                    "the test returned true",
                                    NULL,
                                    NULL,
                                    0);
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
    // parameter validation
    if ( (!_suite) ||
         (!_module) )
    {
        return false;
    }

    bool module_open = false;

    // each block of the spec
    for (size_t b = 0; b < _module->count; b++)
    {
        const struct dj_test_block_spec* const block      = &_module->blocks[b];
        bool                                   block_open = false;

        // each test of the block
        for (size_t t = 0; t < block->count; t++)
        {
            const struct dj_test_case_spec* const test = &block->tests[t];

            // a test the filter leaves out opens nothing
            if (!d_internal_test_suite_selected(_suite->options.filter,
                                                _module->name,
                                                block->name,
                                                test->name))
            {
                continue;
            }

            // the module opens with its first selected test
            if (!module_open)
            {
                module_open = ( (d_test_suite_module(_suite,
                                                     _module->name)) &&
                                (d_test_suite_describe(_suite,
                                                       _module->descriptor)) );
            }

            // the block opens with its first selected test
            if (!block_open)
            {
                block_open = ( (d_test_suite_block(_suite,
                                                   block->name)) &&
                               (d_test_suite_describe(_suite,
                                                      block->descriptor)) );
            }

            // a node that could not be opened ends the module
            if ( (!module_open)                   ||
                 (!block_open)                    ||
                 (!d_test_suite_test(_suite,
                                     test->name)) ||
                 (!d_test_suite_describe(_suite,
                                         test->descriptor)) )
            {
                return false;
            }

            d_internal_test_suite_run_case(_suite,
                                           test);
        }
    }

    return (!_suite->error);
}

//==============================================================================
// 5.  REPORT
//==============================================================================

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
    // a skipped test is neither passed nor failed
    if (_test->skipped)
    {
        return "SKIP";
    }

    // a failed check, or no check at all, fails the test
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
    memset(_summary,
           0,
           sizeof(*_summary));

    const struct dj_test_suite_node* node = NULL;

    // each node, once
    for (size_t i = 0; i < _suite->node_count; i++)
    {
        node = &_suite->nodes[i];

        // a module counts once; a test by its verdict
        if (node->kind == D_INTERNAL_TEST_SUITE_KIND_MODULE)
        {
            _summary->modules++;
        }
        else if (node->kind == D_INTERNAL_TEST_SUITE_KIND_TEST)
        {
            _summary->tests++;
            _summary->checks        += node->checks;
            _summary->checks_failed += node->checks_failed;

            // a skip, a failure or a pass
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
    fprintf(_out,
            "          %s ",
            (_record->result) ? "ok  " : "FAIL");

    // where the check was made, when the macro knew
    if (_record->file)
    {
        fprintf(_out,
                "%s:%d: ",
                _record->file,
                _record->line);
    }

    fprintf(_out,
            "%s",
            (_record->text) ? _record->text : "");

    // the check's message, when it has one
    if (_record->message)
    {
        fprintf(_out,
                " -- %s",
                _record->message);
    }

    fputc('\n',
          _out);

    return;
}

/**
 * @brief Whether a test beneath a heading failed, or, with `_skips`, failed
 *        or was skipped.
 *
 * @param[in] _suite    the suite.
 * @param[in] _heading  a module or block node.
 * @param[in] _skips    whether a skipped test counts.
 * @return `true` for such a test before the next heading of the heading's
 *         rank or above.
 */
static bool
d_internal_test_suite_beneath(
    const struct dj_test_suite* _suite,
    size_t                      _heading,
    bool                        _skips
)
{
    const int rank = _suite->nodes[_heading].kind;

    // each node until the next heading of this rank or above
    for (size_t i = _heading + 1; i < _suite->node_count; i++)
    {
        const struct dj_test_suite_node* const node = &_suite->nodes[i];

        // a heading of this rank or above ends the heading's span
        if (node->kind <= rank)
        {
            break;
        }

        // only tests have verdicts
        if (node->kind != D_INTERNAL_TEST_SUITE_KIND_TEST)
        {
            continue;
        }

        const char* const word = d_internal_test_suite_verdict(node);

        // a failure, or a skip when skips count
        if ( (word[0] == 'F') ||
             ( (_skips) &&
               (word[0] == 'S') ) )
        {
            return true;
        }
    }

    return false;
}

/**
 * @brief Whether a heading holds a test the report lists.
 *
 * @param[in] _suite    the suite.
 * @param[in] _heading  a module or block node.
 * @return `true` when the options list every test, or when a test beneath the
 *         heading did not pass.
 */
static bool
d_internal_test_suite_heading_listed(
    const struct dj_test_suite* _suite,
    size_t                      _heading
)
{
    // every heading is listed unless the report lists failures only
    if (_suite->options.show != D_TEST_SUITE_SHOW_FAILURES)
    {
        return true;
    }

    return d_internal_test_suite_beneath(_suite,
                                         _heading,
                                         true);
}

/**
 * @brief Prints a note, a line of output per line of its text.
 *
 * @param[in] _out     the stream.
 * @param[in] _text    the note; may be `NULL`.
 * @param[in] _indent  the columns before "note", which the lines after the
 *                     first keep, plus the word's own width.
 *
 * A final newline ends the note rather than opening an empty line.
 */
static void
d_internal_test_suite_print_note(
    FILE*       _out,
    const char* _text,
    int         _indent
)
{
    const char* line  = (_text) ? _text : "";
    bool        first = true;

    // one output line per line of the note
    for (;;)
    {
        const char* const end    = strchr(line,
                                          '\n');
        const size_t      length = (end) ? (size_t)(end - line) : strlen(line);

        fprintf(_out,
                "%*s%s%.*s\n",
                _indent,
                "",
                (first) ? "note " : "     ",
                (int)length,
                line);

        first = false;

        // the text ends here, or at a final newline, which opens no line
        if ( (!end) ||
             (end[1] == '\0') )
        {
            break;
        }

        line = end + 1;
    }

    return;
}

/**
 * @brief Prints a node's line: a module's or block's heading, or a test's
 *        verdict, as the options list it.
 *
 * @param[in]  _suite  the suite.
 * @param[in]  _out    the stream.
 * @param[in]  _index  the node.
 * @param[in]  _kept   whether the node has kept records.
 * @param[out] _notes  whether the node's notes are printed.
 * @return whether the node is listed, and so its kept checks with it.
 *
 * A heading's notes cost a scan of its tests, which only a heading that has
 * notes pays.
 */
static bool
d_internal_test_suite_print_node(
    const struct dj_test_suite* _suite,
    FILE*                       _out,
    size_t                      _index,
    bool                        _kept,
    bool*                       _notes
)
{
    const struct dj_test_suite_node* const node  = &_suite->nodes[_index];
    const bool                             every =
        (_suite->options.show == D_TEST_SUITE_SHOW_CHECKS);

    // a heading: listed, and its notes, by the tests beneath it
    if (node->kind != D_INTERNAL_TEST_SUITE_KIND_TEST)
    {
        const bool listed = d_internal_test_suite_heading_listed(_suite,
                                                                 _index);

        *_notes = false;

        // a heading with nothing listed beneath it is left out
        if (!listed)
        {
            return false;
        }

        // a module's heading carries its descriptor
        if (node->kind == D_INTERNAL_TEST_SUITE_KIND_MODULE)
        {
            fprintf(_out,
                    "\nmodule %s%s%s\n",
                    node->name,
                    (node->descriptor) ? " -- " : "",
                    (node->descriptor) ? node->descriptor : "");
        }
        else
        {
            fprintf(_out,
                    "  block %s\n",
                    node->name);
        }

        // a heading pays for a scan of its tests only when it has notes
        if (_kept)
        {
            *_notes = ( (every) ||
                        (d_internal_test_suite_beneath(_suite,
                                                       _index,
                                                       false)) );
        }

        return true;
    }

    const char* const word   = d_internal_test_suite_verdict(node);
    const bool        listed =
        ( (_suite->options.show != D_TEST_SUITE_SHOW_FAILURES) ||
          (word[0] != 'P') );

    *_notes = ( (listed) &&
                ( (every) ||
                  (word[0] == 'F') ) );

    // an unlisted test prints nothing, nor its checks
    if (!listed)
    {
        return false;
    }

    fprintf(_out,
            "    [%s] %s",
            word,
            node->name);

    // a skip gives its reason; a test with no checks says so
    if (node->skipped)
    {
        fprintf(_out,
                ": %s",
                (node->skip_reason) ? node->skip_reason : "skipped");
    }
    else if (node->checks == 0)
    {
        fprintf(_out,
                ": no checks");
    }

    fputc('\n',
          _out);

    return true;
}

/**
 * @brief Prints the tree: headings, tests and their kept checks.
 *
 * @param[in] _suite  the suite.
 * @param[in] _out    the stream.
 *
 * Walks the nodes and the records in step; a record belongs to the node it
 * names, and every record of a node follows that node and precedes the next
 * node's. A heading's records are notes; a test's are checks and notes, in
 * the order they were made.
 */
static void
d_internal_test_suite_print_tree(
    const struct dj_test_suite* _suite,
    FILE*                       _out
)
{
    size_t r = 0;

    // the nodes and the records in step
    for (size_t i = 0; i < _suite->node_count; i++)
    {
        // whether this node has records, which follow it
        const bool kept = ( (r < _suite->record_count) &&
                            (_suite->records[r].node == i) );

        bool       notes  = false;
        const bool listed = d_internal_test_suite_print_node(_suite,
                                                             _out,
                                                             i,
                                                             kept,
                                                             &notes);

        // a module's notes sit at its blocks' depth, a block's at its tests'
        const int indent =
            (_suite->nodes[i].kind == D_INTERNAL_TEST_SUITE_KIND_MODULE)
                ? 2
            : (_suite->nodes[i].kind == D_INTERNAL_TEST_SUITE_KIND_BLOCK)
                ? 4
                : 10;

        // the records of this node, which follow it
        while ( (r < _suite->record_count) &&
                (_suite->records[r].node == i) )
        {
            const struct dj_test_suite_record* const record =
                &_suite->records[r];

            // a note prints by its own rule; a check with its node
            if (record->note)
            {
                // the node's notes were asked for
                if (notes)
                {
                    d_internal_test_suite_print_note(_out,
                                                     record->text,
                                                     indent);
                }
            }
            else if (listed)
            {
                d_internal_test_suite_print_record(_out,
                                                   record);
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
    size_t length = 0;

    // every character but a carriage return takes a column
    while ( (_text) &&
            (*_text) )
    {
        // a carriage return takes no column
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
    fputs("| ",
          _out);

    size_t length = 0;

    // every character, with the table's separator replaced
    while ( (_text) &&
            (*_text) )
    {
        // a bar would end the cell: it prints as a slash
        if (*_text == '|')
        {
            fputc('/',
                  _out);
            length++;
        }
        else if (*_text == '\n')
        {
            fputc(' ',
                  _out);
            length++;
        }
        else if (*_text != '\r')
        {
            fputc(*_text,
                  _out);
            length++;
        }

        _text++;
    }

    // pad the cell to its column's width
    for (; length < _width; length++)
    {
        fputc(' ',
              _out);
    }

    fputc(' ',
          _out);

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
    // each column as wide as its header: "RESULT", "#", "TEST", "DESCRIPTION"
    size_t width[4] = { 6, 1, 4, 11 };
    size_t n        = 0;

    // the widths: each column as wide as its widest cell
    for (size_t i = _first; i < _end; i++)
    {
        const struct dj_test_suite_node* const node = &_suite->nodes[i];

        // only tests have rows
        if (node->kind != D_INTERNAL_TEST_SUITE_KIND_TEST)
        {
            continue;
        }

        char number[24] = {0};

        n++;
        sprintf(number,
                "%lu",
                (unsigned long)n);

        // the number column
        if (strlen(number) > width[1])
        {
            width[1] = strlen(number);
        }

        // the name column
        if (d_internal_test_suite_cell_length(node->name) > width[2])
        {
            width[2] = d_internal_test_suite_cell_length(node->name);
        }

        // the descriptor column
        if (d_internal_test_suite_cell_length(node->descriptor) > width[3])
        {
            width[3] = d_internal_test_suite_cell_length(node->descriptor);
        }
    }

    fprintf(_out,
            "\n|-- results table (%s) --|\n",
            _suite->nodes[_first].name);
    d_internal_test_suite_cell(_out,
                               "RESULT",
                               width[0]);
    d_internal_test_suite_cell(_out,
                               "#",
                               width[1]);
    d_internal_test_suite_cell(_out,
                               "TEST",
                               width[2]);
    d_internal_test_suite_cell(_out,
                               "DESCRIPTION",
                               width[3]);
    fputs("|\n|",
          _out);

    // the header row's length less "|", its final "|" and the newline
    const size_t rule = (width[0] + width[1] + width[2] + width[3]) +
                        (4 * 3) + 1 - 2;

    // the rule beneath the header
    for (size_t i = 0; i < rule; i++)
    {
        fputc('-',
              _out);
    }

    fputs("|\n",
          _out);

    n = 0;

    // a row per test
    for (size_t i = _first; i < _end; i++)
    {
        const struct dj_test_suite_node* const node = &_suite->nodes[i];

        // only tests have rows
        if (node->kind != D_INTERNAL_TEST_SUITE_KIND_TEST)
        {
            continue;
        }

        char number[24] = {0};

        n++;
        sprintf(number,
                "%lu",
                (unsigned long)n);
        d_internal_test_suite_cell(_out,
                                   d_internal_test_suite_verdict(node),
                                   width[0]);
        d_internal_test_suite_cell(_out,
                                   number,
                                   width[1]);
        d_internal_test_suite_cell(_out,
                                   node->name,
                                   width[2]);
        d_internal_test_suite_cell(_out,
                                   node->descriptor,
                                   width[3]);
        fputs("|\n",
              _out);
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
    size_t first = D_INTERNAL_TEST_SUITE_NONE;

    // each module's span, ended by the next module or the end
    for (size_t i = 0; i <= _suite->node_count; i++)
    {
        // only a module, or the end, closes a span
        if ( (i < _suite->node_count) &&
             (_suite->nodes[i].kind != D_INTERNAL_TEST_SUITE_KIND_MODULE) )
        {
            continue;
        }

        // a span with a module before it gets its table
        if (first != D_INTERNAL_TEST_SUITE_NONE)
        {
            d_internal_test_suite_print_table(_suite,
                                              _out,
                                              first,
                                              i);
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
    // parameter validation
    if (!_suite)
    {
        return D_TEST_SUITE_EXIT_ERROR;
    }

    FILE* const            out     = (_suite->options.stream)
                                         ? _suite->options.stream
                                         : stdout;
    struct dj_test_summary summary = {0};

    d_internal_test_suite_tally(_suite,
                                &summary);
    d_internal_test_suite_print_tree(_suite,
                                     out);

    // the results tables, when the options ask for them
    if (_suite->options.table)
    {
        d_internal_test_suite_print_tables(_suite,
                                           out);
    }

    int code = 0;

    // an error proves nothing; a failure or no test fails the run
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

    // the caller's copy of the counts
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
    struct dj_test_suite_options options = {0};

    d_test_suite_options_init(&options);

    const int parsed = d_test_suite_options_parse(&options,
                                                  _argc,
                                                  _argv);

    // --help exits passed; a refused argument, with an error
    if (parsed != 0)
    {
        return (parsed > 0) ? D_TEST_SUITE_EXIT_PASSED
                            : D_TEST_SUITE_EXIT_ERROR;
    }

    struct dj_test_suite* suite = d_test_suite_new();

    // allocation failed
    if (!suite)
    {
        fprintf(stderr,
                "fatal: could not create the test suite\n");

        return D_TEST_SUITE_EXIT_ERROR;
    }

    d_test_suite_set_options(suite,
                             &options);

    // the heading goes where the report goes
    fprintf((options.stream) ? options.stream : stdout,
            "== %s ==\n",
            (_title) ? _title : "tests");

    // every module spec, in order
    for (size_t i = 0; i < _count; i++)
    {
        // a NULL array adds nothing
        if (_modules)
        {
            d_test_suite_add_module(suite,
                                    _modules[i]);
        }
    }

    const int code = d_test_suite_run(suite,
                                      NULL);

    d_test_suite_free(suite);

    return code;
}
