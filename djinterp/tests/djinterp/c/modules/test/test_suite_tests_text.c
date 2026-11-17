/*******************************************************************************
* djinterp [c][test]                                     test_suite_tests_text.c
*
* test_suite.h's suite, section 5: comparing texts.
*   Each case runs one text check in a quiet inner suite and reads its report
* back, so what is checked is the note as a reader of a failing suite sees it,
* word for word: the report prints a note's first line after "note " and each
* line after it at the same depth, and a text note's detail lines start with
* two spaces of their own; D_TESTS_TEST_SUITE_MORE spells the two together.
*
*
* path:      /tests/djinterp/c/modules/test/test_suite_tests_text.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.15
*                                                            revised: 2026.11.15
*******************************************************************************/
#include "./test_suite_tests.h"  // corresponding header

// std
#include <stdio.h>   // fopen, fwrite, fclose, remove, snprintf
#include <stdlib.h>  // malloc, free
#include <string.h>  // memset, memcpy, strcmp, strlen


// D_TESTS_TEST_SUITE_MORE
//   constant: what starts each detail line of a text note, as the report
// prints it: the note's depth and the width of "note " (15 columns), then
// the detail line's own two spaces.
#define D_TESTS_TEST_SUITE_MORE "\n                 "

// D_TESTS_TEST_SUITE_DIR
//   constant: where the file cases write their files: the leaf's output
// directory when the build gives one, else the working directory.
#ifdef D_TEST_OUTPUT_DIR
    #define D_TESTS_TEST_SUITE_DIR D_TEST_OUTPUT_DIR
#else
    #define D_TESTS_TEST_SUITE_DIR "."
#endif  // D_TEST_OUTPUT_DIR

//==============================================================================
// 5.  TEXT
//==============================================================================

/**
 * @brief Runs one text check, named "text", in a quiet inner suite.
 *
 * @param[in]  _got   the actual side.
 * @param[in]  _want  the expected side.
 * @param[in]  _show  the report's detail, D_TEST_SUITE_SHOW_*.
 * @param[out] _held  what the check returned.
 * @return the inner report, which the caller frees, or `NULL`.
 */
static char*
d_tests_test_suite_text_run(
    struct dj_test_text _got,
    struct dj_test_text _want,
    int                 _show,
    bool*               _held
)
{
    FILE* stream = NULL;

    struct dj_test_suite* const suite =
        d_tests_test_suite_quiet(_show,
                                 &stream);

    *_held = false;

    // check if memory allocation was successful
    if (!suite)
    {
        return NULL;
    }

    (void)d_test_suite_test(suite,
                            "t");
    *_held = d_test_suite_check_text(suite,
                                     "text",
                                     _got,
                                     _want);

    return d_tests_test_suite_report(suite,
                                     stream,
                                     NULL,
                                     NULL);
}

/**
 * @brief Whether a text check holds, and its report, which lists every check
 *        and note, has no note.
 *
 * @param[in] _got   the actual side.
 * @param[in] _want  the expected side.
 * @return `true` if the check held and the report says nothing more.
 */
static bool
d_tests_test_suite_text_holds(
    struct dj_test_text _got,
    struct dj_test_text _want
)
{
    bool        held   = false;
    char* const report = d_tests_test_suite_text_run(_got,
                                                     _want,
                                                     D_TEST_SUITE_SHOW_CHECKS,
                                                     &held);
    const bool  quiet  = ( (report) &&
                           (!d_tests_test_suite_contains(report,
                                                         "note ")) );

    free(report);

    return ( (held) &&
             (quiet) );
}

/**
 * @brief Whether a text check fails with exactly the note given.
 *
 * @param[in] _got   the actual side.
 * @param[in] _want  the expected side.
 * @param[in] _note  the note as the report prints it, from "note " to the
 *                   newline that ends it.
 * @return `true` if the check failed and the report holds the note.
 *
 * When it does not, a note on the running test shows what was wanted and the
 * report that came instead.
 */
static bool
d_tests_test_suite_text_notes(
    struct dj_test_text _got,
    struct dj_test_text _want,
    const char*         _note
)
{
    bool        held   = true;
    char* const report = d_tests_test_suite_text_run(_got,
                                                     _want,
                                                     D_TEST_SUITE_SHOW_TESTS,
                                                     &held);
    const bool  noted  = ( (!held) &&
                           (d_tests_test_suite_contains(report,
                                                        _note)) );

    // what was wanted, and what came, for the reader of a failure
    if (!noted)
    {
        D_TEST_NOTE("wanted:\n%s\nin:\n%s",
                    _note,
                    (report) ? report : "(no report)");
    }

    free(report);

    return noted;
}

/**
 * @brief Writes bytes to a file in the output directory, in binary.
 *
 * @param[out] _path    receives the file's path.
 * @param[in]  _size    the size of _path.
 * @param[in]  _name    the file's name.
 * @param[in]  _bytes   the bytes.
 * @param[in]  _length  how many.
 * @return `true` if the whole file was written.
 */
static bool
d_tests_test_suite_text_write(
    char*       _path,
    size_t      _size,
    const char* _name,
    const char* _bytes,
    size_t      _length
)
{
    const int written = snprintf(_path,
                                 _size,
                                 "%s/%s",
                                 D_TESTS_TEST_SUITE_DIR,
                                 _name);

    // a path that did not fit
    if ( (written < 0) ||
         ((size_t)written >= _size) )
    {
        return false;
    }

    FILE* const file = fopen(_path,
                             "wb");

    // a file that could not be made
    if (!file)
    {
        return false;
    }

    const bool all = (fwrite(_bytes,
                             1,
                             _length,
                             file) == _length);

    return ( (fclose(file) == 0) &&
             (all) );
}

/*
d_tests_test_suite_text_makers
  The three makers.
  Tests the following:
  - a buffer keeps its bytes and length, a NUL among them
  - a NULL buffer of length 0 is the empty text; of a length, no text
  - a string is its bytes to its NUL; NULL is no text
  - a file is its path alone, read at the check
*/
bool
d_tests_test_suite_text_makers(void)
{
    static const char bytes[4] = { 'a', '\0', 'b', '\n' };

    struct dj_test_text text = d_test_text_buffer(bytes,
                                                  4);

    D_TEST_REQUIRE( (text.text == bytes) &&
                    (text.length == 4)   &&
                    (text.path == NULL) );
    text = d_test_text_buffer(NULL,
                              0);
    D_TEST_REQUIRE( (text.text != NULL) &&
                    (text.length == 0)  &&
                    (text.path == NULL) );
    text = d_test_text_buffer(NULL,
                              3);
    D_TEST_REQUIRE( (text.text == NULL) &&
                    (text.length == 0)  &&
                    (text.path == NULL) );
    text = d_test_text_cstr("abc");
    D_TEST_REQUIRE( (text.text != NULL) &&
                    (text.length == 3)  &&
                    (text.path == NULL) );
    text = d_test_text_cstr(NULL);
    D_TEST_REQUIRE( (text.text == NULL) &&
                    (text.length == 0)  &&
                    (text.path == NULL) );
    text = d_test_text_file("x.txt");
    D_TEST_REQUIRE( (text.text == NULL) &&
                    (text.length == 0)  &&
                    (text.path != NULL) &&
                    (strcmp(text.path,
                            "x.txt") == 0) );

    return true;
}

/*
d_tests_test_suite_text_same
  The same texts pass, quietly.
  Tests the following:
  - a buffer against a string, and the empty text against itself
  - CRLF against LF, either way round, and CRLF against CRLF
  - a buffer read to its length, not its NUL: a NUL among the bytes counts
  - a buffer whose bytes run on past its length is compared to its length
*/
bool
d_tests_test_suite_text_same(void)
{
    static const char sink[] = "frame  2  shell\nNOT WRITTEN";

    D_TEST_REQUIRE(d_tests_test_suite_text_holds(
                       d_test_text_buffer("ab\ncd\n",
                                          6),
                       d_test_text_cstr("ab\ncd\n")));
    D_TEST_REQUIRE(d_tests_test_suite_text_holds(
                       d_test_text_buffer(NULL,
                                          0),
                       d_test_text_cstr("")));
    D_TEST_REQUIRE(d_tests_test_suite_text_holds(
                       d_test_text_cstr("ab\r\ncd\r\n"),
                       d_test_text_cstr("ab\ncd\n")));
    D_TEST_REQUIRE(d_tests_test_suite_text_holds(
                       d_test_text_cstr("ab\ncd\n"),
                       d_test_text_cstr("ab\r\ncd\r\n")));
    D_TEST_REQUIRE(d_tests_test_suite_text_holds(
                       d_test_text_cstr("ab\r\ncd"),
                       d_test_text_cstr("ab\ncd")));
    D_TEST_REQUIRE(d_tests_test_suite_text_holds(
                       d_test_text_buffer("a\0b\n",
                                          4),
                       d_test_text_buffer("a\0b\n",
                                          4)));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_buffer("a\0b\n",
                                          4),
                       d_test_text_buffer("a\0c\n",
                                          4),
                       "note text: line 1 differs, at column 3"
                       D_TESTS_TEST_SUITE_MORE "got:  \"a\\x00b\""
                       D_TESTS_TEST_SUITE_MORE "want: \"a\\x00c\"\n"));
    D_TEST_REQUIRE(d_tests_test_suite_text_holds(
                       d_test_text_buffer(sink,
                                          16),
                       d_test_text_cstr("frame  2  shell\n")));

    return true;
}

/*
d_tests_test_suite_text_line
  A line that differs is named, with its column and both lines.
  Tests the following:
  - lane G's example: line 4, column 8, both lines quoted
  - a difference in the first byte is column 1
  - a line that begins the other: the column is one past its end, either way
  - CRs before LFs on one side do not move the line or the column
*/
bool
d_tests_test_suite_text_line(void)
{
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr("a\nb\nc\nframe   2  shell\n"),
                       d_test_text_cstr("a\nb\nc\nframe  2  shell\n"),
                       "note text: line 4 differs, at column 8"
                       D_TESTS_TEST_SUITE_MORE "got:  \"frame   2  shell\""
                       D_TESTS_TEST_SUITE_MORE "want: \"frame  2  shell\"\n"));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr("x\n"),
                       d_test_text_cstr("y\n"),
                       "note text: line 1 differs, at column 1"
                       D_TESTS_TEST_SUITE_MORE "got:  \"x\""
                       D_TESTS_TEST_SUITE_MORE "want: \"y\"\n"));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr("abc\n"),
                       d_test_text_cstr("abcdef\n"),
                       "note text: line 1 differs, at column 4"
                       D_TESTS_TEST_SUITE_MORE "got:  \"abc\""
                       D_TESTS_TEST_SUITE_MORE "want: \"abcdef\"\n"));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr("abcdef\n"),
                       d_test_text_cstr("abc\n"),
                       "note text: line 1 differs, at column 4"
                       D_TESTS_TEST_SUITE_MORE "got:  \"abcdef\""
                       D_TESTS_TEST_SUITE_MORE "want: \"abc\"\n"));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr("one\r\ntwo\r\nthree\r\n"),
                       d_test_text_cstr("one\ntwo\nthre\n"),
                       "note text: line 3 differs, at column 5"
                       D_TESTS_TEST_SUITE_MORE "got:  \"three\""
                       D_TESTS_TEST_SUITE_MORE "want: \"thre\"\n"));

    return true;
}

/*
d_tests_test_suite_text_escapes
  The lines shown escape what would read as something else.
  Tests the following:
  - a tab, a CR not before an LF, a quote and a backslash, by name
  - any other control byte as \xNN, DEL included; UTF-8 as it is
  - the column counts the line's own bytes, not its escapes
  - of CRs before an LF, only the last is dropped
*/
bool
d_tests_test_suite_text_escapes(void)
{
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr("\tx\n"),
                       d_test_text_cstr("\ty\n"),
                       "note text: line 1 differs, at column 2"
                       D_TESTS_TEST_SUITE_MORE "got:  \"\\tx\""
                       D_TESTS_TEST_SUITE_MORE "want: \"\\ty\"\n"));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr("a\rb\n"),
                       d_test_text_cstr("a b\n"),
                       "note text: line 1 differs, at column 2"
                       D_TESTS_TEST_SUITE_MORE "got:  \"a\\rb\""
                       D_TESTS_TEST_SUITE_MORE "want: \"a b\"\n"));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr("a\r\r\n"),
                       d_test_text_cstr("a\n"),
                       "note text: line 1 differs, at column 2"
                       D_TESTS_TEST_SUITE_MORE "got:  \"a\\r\""
                       D_TESTS_TEST_SUITE_MORE "want: \"a\"\n"));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr("\"q\\\x01\x1b\x7f\n"),
                       d_test_text_cstr("\"q\\\x01\x1b\x7e\n"),
                       "note text: line 1 differs, at column 6"
                       D_TESTS_TEST_SUITE_MORE
                       "got:  \"\\\"q\\\\\\x01\\x1b\\x7f\""
                       D_TESTS_TEST_SUITE_MORE
                       "want: \"\\\"q\\\\\\x01\\x1b~\"\n"));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr("caf\xc3\xa9\n"),
                       d_test_text_cstr("caf\xc3\xa8\n"),
                       "note text: line 1 differs, at column 5"
                       D_TESTS_TEST_SUITE_MORE "got:  \"caf\xc3\xa9\""
                       D_TESTS_TEST_SUITE_MORE "want: \"caf\xc3\xa8\"\n"));

    return true;
}

/**
 * @brief Writes the note line a window shows: a label, then _shown bytes of
 *        'a' from _from, with the byte at _mark as _byte, quoted, with "..."
 *        outside each cut end.
 *
 * @param[out] _out     the line; room for at least 160 bytes.
 * @param[in]  _label   "got:  " or "want: ".
 * @param[in]  _from    the window's first byte.
 * @param[in]  _shown   the bytes shown.
 * @param[in]  _length  the line's length.
 * @param[in]  _mark    the byte that is not 'a', or _length for none.
 * @param[in]  _byte    that byte.
 */
static void
d_tests_test_suite_text_window_line(
    char*       _out,
    const char* _label,
    size_t      _from,
    size_t      _shown,
    size_t      _length,
    size_t      _mark,
    char        _byte
)
{
    size_t at = 0;

    memcpy(_out,
           _label,
           strlen(_label));
    at = strlen(_label);

    // a cut front
    if (_from > 0)
    {
        memcpy(_out + at,
               "...",
               3);
        at += 3;
    }

    _out[at++] = '"';

    // the window's bytes
    for (size_t i = _from; i < (_from + _shown); i++)
    {
        _out[at++] = (i == _mark) ? _byte : 'a';
    }

    _out[at++] = '"';

    // a cut end
    if ((_from + _shown) < _length)
    {
        memcpy(_out + at,
               "...",
               3);
        at += 3;
    }

    _out[at] = '\0';

    return;
}

/**
 * @brief Whether two lines of 'a' that differ at one byte, or where one
 *        ends, are shown through the window given.
 *
 * @param[in] _got     got's length.
 * @param[in] _want    want's length.
 * @param[in] _mark    the byte of want that is 'B', or _want for none.
 * @param[in] _column  the column the note should give.
 * @param[in] _from    the window's first byte.
 * @return `true` if the check failed with exactly that note.
 */
static bool
d_tests_test_suite_text_windowed(
    size_t _got,
    size_t _want,
    size_t _mark,
    size_t _column,
    size_t _from
)
{
    char* const got          = (char*)malloc(_got + 1);
    char* const want         = (char*)malloc(_want + 1);
    char* const note         = (char*)malloc(512);
    char        line[2][160] = { {0} };
    bool        noted        = false;

    // the lines, and the note they should give
    if ( (got) &&
         (want) &&
         (note) )
    {
        const size_t width = 120;

        memset(got,
               'a',
               _got);
        memset(want,
               'a',
               _want);

        // the one byte of want that differs, when there is one
        if (_mark < _want)
        {
            want[_mark] = 'B';
        }

        d_tests_test_suite_text_window_line(line[0],
                                            "got:  ",
                                            _from,
                                            ((_got - _from) < width)
                                                ? (_got - _from) : width,
                                            _got,
                                            _got,
                                            'a');
        d_tests_test_suite_text_window_line(line[1],
                                            "want: ",
                                            _from,
                                            ((_want - _from) < width)
                                                ? (_want - _from) : width,
                                            _want,
                                            _mark,
                                            'B');
        snprintf(note,
                 512,
                 "note text: line 1 differs, at column %lu%s%s%s%s\n",
                 (unsigned long)_column,
                 D_TESTS_TEST_SUITE_MORE,
                 line[0],
                 D_TESTS_TEST_SUITE_MORE,
                 line[1]);
        noted = d_tests_test_suite_text_notes(
                    d_test_text_buffer(got,
                                       _got),
                    d_test_text_buffer(want,
                                       _want),
                    note);
    }

    free(got);
    free(want);
    free(note);

    return noted;
}

/*
d_tests_test_suite_text_window
  A line over 120 bytes shows the 120 around its column.
  Tests the following:
  - lines of 120 bytes are shown whole; of 121, cut, "..." outside the quotes
  - the column 60 bytes into a window that is cut at both ends
  - near the start, no front cut; near the end, a full window, no end cut
  - a short line beside a long one shares its window, so the column is level
*/
bool
d_tests_test_suite_text_window(void)
{
    D_TEST_REQUIRE(d_tests_test_suite_text_windowed(120,
                                                    120,
                                                    119,
                                                    120,
                                                    0));
    D_TEST_REQUIRE(d_tests_test_suite_text_windowed(121,
                                                    121,
                                                    9,
                                                    10,
                                                    0));
    D_TEST_REQUIRE(d_tests_test_suite_text_windowed(300,
                                                    300,
                                                    199,
                                                    200,
                                                    139));
    D_TEST_REQUIRE(d_tests_test_suite_text_windowed(300,
                                                    300,
                                                    294,
                                                    295,
                                                    180));
    D_TEST_REQUIRE(d_tests_test_suite_text_windowed(100,
                                                    200,
                                                    200,
                                                    101,
                                                    40));

    return true;
}

/*
d_tests_test_suite_text_runs_out
  A side that runs out of lines first is named, with the other's next line.
  Tests the following:
  - got ends first, then want; the count of lines the other has left
  - an empty side is said to be empty, and the other's line count given
  - the other's next line is shown, cut at 120 bytes from its start
*/
bool
d_tests_test_suite_text_runs_out(void)
{
    char long_line[202] = {0};

    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr("a\nb\n"),
                       d_test_text_cstr("a\nb\nc\nd\n"),
                       "note text: got ends after line 2, want has 2 more "
                       "lines"
                       D_TESTS_TEST_SUITE_MORE "want: \"c\"\n"));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr("a\nb\nc\n"),
                       d_test_text_cstr("a\n"),
                       "note text: want ends after line 1, got has 2 more "
                       "lines"
                       D_TESTS_TEST_SUITE_MORE "got:  \"b\"\n"));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr("a\nb\n"),
                       d_test_text_cstr("a\nb\nc\n"),
                       "note text: got ends after line 2, want has 1 more "
                       "line"
                       D_TESTS_TEST_SUITE_MORE "want: \"c\"\n"));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr(""),
                       d_test_text_cstr("x\n"),
                       "note text: got is empty, want has 1 line"
                       D_TESTS_TEST_SUITE_MORE "want: \"x\"\n"));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr("x\ny\n"),
                       d_test_text_cstr(""),
                       "note text: want is empty, got has 2 lines"
                       D_TESTS_TEST_SUITE_MORE "got:  \"x\"\n"));

    memset(long_line,
           'z',
           200);
    long_line[200] = '\n';
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr(""),
                       d_test_text_cstr(long_line),
                       "note text: got is empty, want has 1 line"
                       D_TESTS_TEST_SUITE_MORE "want: \"zzzzzzzzzzzzzzzzzzzz"
                       "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
                       "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz"
                       "\"...\n"));

    return true;
}

/*
d_tests_test_suite_text_newline
  A missing final newline is a difference of its own, said last.
  Tests the following:
  - only the newline differs: said so, and which side lacks it
  - after a line that differs, and after a side that runs out
  - a final CRLF counts as a final newline; a final CR does not
  - two sides that both lack one are the same
*/
bool
d_tests_test_suite_text_newline(void)
{
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr("a\nb"),
                       d_test_text_cstr("a\nb\n"),
                       "note text: only the final newline differs"
                       D_TESTS_TEST_SUITE_MORE
                       "got:  ends without a final newline\n"));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr("a\r\n"),
                       d_test_text_cstr("a"),
                       "note text: only the final newline differs"
                       D_TESTS_TEST_SUITE_MORE
                       "want: ends without a final newline\n"));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr("a\nX"),
                       d_test_text_cstr("a\nb\n"),
                       "note text: line 2 differs, at column 1"
                       D_TESTS_TEST_SUITE_MORE "got:  \"X\""
                       D_TESTS_TEST_SUITE_MORE "want: \"b\""
                       D_TESTS_TEST_SUITE_MORE
                       "got:  ends without a final newline\n"));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr("a"),
                       d_test_text_cstr("a\nb\n"),
                       "note text: got ends after line 1, want has 1 more "
                       "line"
                       D_TESTS_TEST_SUITE_MORE "want: \"b\""
                       D_TESTS_TEST_SUITE_MORE
                       "got:  ends without a final newline\n"));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr("a\r"),
                       d_test_text_cstr("a\n"),
                       "note text: line 1 differs, at column 2"
                       D_TESTS_TEST_SUITE_MORE "got:  \"a\\r\""
                       D_TESTS_TEST_SUITE_MORE "want: \"a\""
                       D_TESTS_TEST_SUITE_MORE
                       "got:  ends without a final newline\n"));
    D_TEST_REQUIRE(d_tests_test_suite_text_holds(d_test_text_cstr("a\nb"),
                                                 d_test_text_cstr("a\nb")));

    return true;
}

/*
d_tests_test_suite_text_files
  A file is read whole, in binary, on either side.
  Tests the following:
  - a CRLF file against an LF buffer, and against an LF file
  - a file's bytes as they are: a NUL and a ^Z, which a text read would stop
    at on Windows, are compared
  - a file of 10,000 bytes, past the first read's buffer, read whole
  - an empty file is the empty text; a file that differs gives its note
*/
bool
d_tests_test_suite_text_files(void)
{
    static const char crlf[]       = "one\r\ntwo\r\n";
    static const char lf[]         = "one\ntwo\n";
    static const char odd[6]       = { 'a', '\0', 'b', 0x1a, 'c', '\n' };
    char              path[4][512] = { {0} };
    char*             big          = (char*)malloc(10000);

    D_TEST_REQUIRE(big != NULL);
    memset(big,
           'x',
           10000);
    big[4999] = '\n';
    big[9999] = '\n';

    const bool written =
        ( (d_tests_test_suite_text_write(path[0],
                                         sizeof(path[0]),
                                         "test_suite_text_crlf.txt",
                                         crlf,
                                         sizeof(crlf) - 1)) &&
          (d_tests_test_suite_text_write(path[1],
                                         sizeof(path[1]),
                                         "test_suite_text_lf.txt",
                                         lf,
                                         sizeof(lf) - 1))   &&
          (d_tests_test_suite_text_write(path[2],
                                         sizeof(path[2]),
                                         "test_suite_text_odd.bin",
                                         odd,
                                         sizeof(odd)))      &&
          (d_tests_test_suite_text_write(path[3],
                                         sizeof(path[3]),
                                         "test_suite_text_big.txt",
                                         big,
                                         10000)) );
    const bool same =
        ( (written) &&
          (d_tests_test_suite_text_holds(d_test_text_file(path[0]),
                                         d_test_text_cstr(lf)))       &&
          (d_tests_test_suite_text_holds(d_test_text_cstr(lf),
                                         d_test_text_file(path[0])))  &&
          (d_tests_test_suite_text_holds(d_test_text_file(path[0]),
                                         d_test_text_file(path[1])))  &&
          (d_tests_test_suite_text_holds(d_test_text_file(path[2]),
                                         d_test_text_buffer(odd,
                                                            6)))      &&
          (d_tests_test_suite_text_holds(d_test_text_file(path[3]),
                                         d_test_text_buffer(big,
                                                            10000))) );
    const bool differs =
        ( (written) &&
          (d_tests_test_suite_text_notes(
               d_test_text_file(path[1]),
               d_test_text_cstr("one\nto\n"),
               "note text: line 2 differs, at column 2"
               D_TESTS_TEST_SUITE_MORE "got:  \"two\""
               D_TESTS_TEST_SUITE_MORE "want: \"to\"\n")) );

    free(big);

    // the files go, written or not
    for (size_t i = 0; i < 4; i++)
    {
        (void)remove(path[i]);
    }

    D_TEST_REQUIRE(written);
    D_TEST_REQUIRE(same);
    D_TEST_REQUIRE(differs);

    // an empty file, made and gone
    D_TEST_REQUIRE(d_tests_test_suite_text_write(path[0],
                                                 sizeof(path[0]),
                                                 "test_suite_text_empty.txt",
                                                 "",
                                                 0));

    const bool empty = d_tests_test_suite_text_holds(d_test_text_file(path[0]),
                                                     d_test_text_cstr(""));

    (void)remove(path[0]);
    D_TEST_REQUIRE(empty);

    return true;
}

/*
d_tests_test_suite_text_unread
  No text, or a file that cannot be read, fails the check and says why.
  Tests the following:
  - a file that is not there: its path, quoted, and the system's reason
  - a directory, which cannot be read as a file
  - a NULL string, a NULL buffer with a length, and a NULL path: no text
  - both sides at once, each on its own line
*/
bool
d_tests_test_suite_text_unread(void)
{
    char path[512] = {0};

    snprintf(path,
             sizeof(path),
             "%s/test_suite_text_not_there.txt",
             D_TESTS_TEST_SUITE_DIR);
    (void)remove(path);

    char   note[1100] = {0};
    size_t at         = 0;

    at = (size_t)snprintf(note,
                          sizeof(note),
                          "note text: could not compare"
                          D_TESTS_TEST_SUITE_MORE "want: cannot read \"");

    // the path as the note shows it: a backslash or a quote escaped
    for (size_t i = 0; path[i] != '\0'; i++)
    {
        // a backslash or a quote takes a backslash before it
        if ( (path[i] == '\\') ||
             (path[i] == '"') )
        {
            note[at++] = '\\';
        }

        note[at++] = path[i];
    }

    memcpy(note + at,
           "\": ",
           4);
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(d_test_text_cstr("a\n"),
                                                 d_test_text_file(path),
                                                 note));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_file(D_TESTS_TEST_SUITE_DIR),
                       d_test_text_cstr(""),
                       "note text: could not compare"
                       D_TESTS_TEST_SUITE_MORE "got:  cannot read \""));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr(NULL),
                       d_test_text_cstr("a\n"),
                       "note text: could not compare"
                       D_TESTS_TEST_SUITE_MORE "got:  no text (NULL)\n"));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_cstr("a\n"),
                       d_test_text_buffer(NULL,
                                          3),
                       "note text: could not compare"
                       D_TESTS_TEST_SUITE_MORE "want: no text (NULL)\n"));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_file(NULL),
                       d_test_text_cstr(NULL),
                       "note text: could not compare"
                       D_TESTS_TEST_SUITE_MORE "got:  no text (NULL)"
                       D_TESTS_TEST_SUITE_MORE "want: no text (NULL)\n"));

    return true;
}

/*
d_tests_test_suite_inner_text_require
  An inner test whose first text requirement fails: the second is never made.
*/
static bool
d_tests_test_suite_inner_text_require(void)
{
    const struct dj_test_text got  = d_test_text_cstr("ab\n");
    const struct dj_test_text want = d_test_text_cstr("ac\n");

    D_TEST_REQUIRE_TEXT(got,
                        want);
    D_TEST_REQUIRE_TEXT(got,
                        got);

    return true;
}

/*
d_tests_test_suite_inner_text_expect
  An inner test whose first text expectation fails, and which goes on to a
second that holds; it returns what the two gave, false then true.
*/
static bool
d_tests_test_suite_inner_text_expect(void)
{
    const struct dj_test_text got  = d_test_text_cstr("ab\n");
    const struct dj_test_text want = d_test_text_cstr("ac\n");

    const bool first  = D_TEST_EXPECT_TEXT(got,
                                           want);
    const bool second = D_TEST_EXPECT_TEXT(want,
                                           want);

    return ( (!first) &&
             (second) );
}

/*
d_tests_test_suite_text_spec
  The spec macros: D_TEST_REQUIRE_TEXT stops, D_TEST_EXPECT_TEXT goes on.
  Tests the following:
  - REQUIRE_TEXT records one failed check and returns from the test
  - EXPECT_TEXT records both, the failed one and the one after it, and each
    gives its check's result
  - a check is named by its first argument's source text, and placed by its
    file and line; the note starts with that name
*/
bool
d_tests_test_suite_text_spec(void)
{
    struct dj_test_case_spec row = { "require", "stops",
                                     d_tests_test_suite_inner_text_require };

    struct dj_test_summary summary = {0};
    int                    code    = 0;

    char* report = d_tests_test_suite_run_row(&row,
                                              NULL,
                                              &summary,
                                              &code);

    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(summary.checks == 1);
    D_TEST_REQUIRE(summary.checks_failed == 1);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_FAILED);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "test_suite_tests_text.c:"));
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               ": got\n          note got: "
                                               "line 1 differs, at column 2"));
    free(report);

    row.name = "expect";
    row.fn   = d_tests_test_suite_inner_text_expect;
    report   = d_tests_test_suite_run_row(&row,
                                          NULL,
                                          &summary,
                                          &code);
    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(summary.checks == 2);
    D_TEST_REQUIRE(summary.checks_failed == 1);
    D_TEST_REQUIRE(code == D_TEST_SUITE_EXIT_FAILED);
    D_TEST_REQUIRE(!d_tests_test_suite_contains(report,
                                                "the test returned false"));
    free(report);

    return true;
}

/*
d_tests_test_suite_inner_text_current
  An inner test that checks text with a NULL suite, once the same, once not.
*/
static bool
d_tests_test_suite_inner_text_current(void)
{
    (void)d_test_suite_check_text(NULL,
                                  "same",
                                  d_test_text_cstr("a\n"),
                                  d_test_text_cstr("a\r\n"));
    (void)d_test_suite_check_text(NULL,
                                  "differs",
                                  d_test_text_cstr("a\n"),
                                  d_test_text_cstr("b\n"));

    return true;
}

/*
d_tests_test_suite_text_current
  With a NULL suite, a text check goes to the spec test that is running.
  Tests the following:
  - both checks are the inner test's, one failed; the note is there too
  - d_test_suite_check_text has no place: the check's line has none
*/
bool
d_tests_test_suite_text_current(void)
{
    struct dj_test_case_spec row = { "current", "a NULL suite",
                                     d_tests_test_suite_inner_text_current };

    struct dj_test_summary summary = {0};
    int                    code    = 0;

    char* report = d_tests_test_suite_run_row(&row,
                                              NULL,
                                              &summary,
                                              &code);

    D_TEST_REQUIRE(report != NULL);
    D_TEST_REQUIRE(summary.checks == 2);
    D_TEST_REQUIRE(summary.checks_failed == 1);
    D_TEST_REQUIRE(d_tests_test_suite_contains(report,
                                               "          FAIL differs\n"
                                               "          note differs: line 1 "
                                               "differs, at column 1\n"));
    free(report);

    return true;
}

/*
d_tests_test_suite_text_lane_g
  Lane G's two local compares, as their suites will call the helper.
  Tests the following:
  - the djin list, from djin_tests_text.c: a sink's buffer, read to its
    written length, against the text in the source; then one space too many
  - the kit's compare (cli_tests_support.c): an actual that wrote nothing
    (NULL, 0) against an expected usage, and an actual cut short
*/
bool
d_tests_test_suite_text_lane_g(void)
{
    static const char expected[] =
        "awk      0.9.0  the awk machine\n"
        "bare     1      nothing\n"
        "printer  -      settings only\n";
    static const char wrote[] =
        "awk      0.9.0  the awk machine\n"
        "bare      1      nothing\n"
        "printer  -      settings only\n";
    char sink[128] = {0};

    memset(sink,
           '#',
           sizeof(sink));
    memcpy(sink,
           expected,
           sizeof(expected) - 1);
    D_TEST_REQUIRE(d_tests_test_suite_text_holds(
                       d_test_text_buffer(sink,
                                          sizeof(expected) - 1),
                       d_test_text_cstr(expected)));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_buffer(wrote,
                                          sizeof(wrote) - 1),
                       d_test_text_cstr(expected),
                       "note text: line 2 differs, at column 10"
                       D_TESTS_TEST_SUITE_MORE
                       "got:  \"bare      1      nothing\""
                       D_TESTS_TEST_SUITE_MORE
                       "want: \"bare     1      nothing\"\n"));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_buffer(NULL,
                                          0),
                       d_test_text_cstr("usage: cli [options] FILE\n"),
                       "note text: got is empty, want has 1 line"
                       D_TESTS_TEST_SUITE_MORE
                       "want: \"usage: cli [options] FILE\"\n"));
    D_TEST_REQUIRE(d_tests_test_suite_text_notes(
                       d_test_text_buffer("usage: cli [opt",
                                          15),
                       d_test_text_cstr("usage: cli [options] FILE\n"),
                       "note text: line 1 differs, at column 16"
                       D_TESTS_TEST_SUITE_MORE "got:  \"usage: cli [opt\""
                       D_TESTS_TEST_SUITE_MORE
                       "want: \"usage: cli [options] FILE\""
                       D_TESTS_TEST_SUITE_MORE
                       "got:  ends without a final newline\n"));

    return true;
}
