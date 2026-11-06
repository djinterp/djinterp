/*******************************************************************************
* djinterp [tools]                                                  test_dedit.c
*
* Exercises `dedit.h`'s runs of lines: d_edit_replace_lines,
* d_edit_may_shift_lines and d_edit_shift_lines, which drepair.c's repairs
* write through, and d_edit_insert_line, which puts an empty line in.
*   Each check writes a small file, edits it, and compares every byte with
* what the edit should leave. Every check runs with LF and with CRLF line
* ends, because the one thing dedit must never do is change a line end it
* was not asked about. A refused edit must leave the file as it was.
*
*   test_dedit [directory]    the directory holds the files the test writes
*
* path:      /tests/djinterp/tools/dawk/test_dedit.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.05
*******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dedit.h"  // corresponding header
// std
#include <stdio.h>   // printf, fopen, fread, fwrite, snprintf
#include <stdlib.h>  // free, malloc
#include <string.h>  // memcmp, strlen


static int  g_checks = 0;
static int  g_failed = 0;
static char g_path[1024];


/*
d_tests_check
  Counts one check, and names it when it fails.
*/
static
void d_tests_check(
    bool        _condition,
    const char* _what,
    const char* _ends
)
{
    g_checks++;

    // a failure names the check and the line ends it ran with
    if (!_condition)
    {
        g_failed++;
        printf("  FAIL  %s (%s)\n",
               _what,
               _ends);
    }

    return;
}

/*
d_tests_spell
  Writes `_text`, whose lines end in `\n`, into `_out` with each `\n`
spelled as `_eol`.
*/
static
void d_tests_spell(
    const char* _text,
    const char* _eol,
    char*       _out,
    size_t      _out_size
)
{
    size_t used = 0u;

    for (size_t at = 0u; (_text[at] != '\0') && (used + 3u < _out_size); ++at)
    {
        if (_text[at] == '\n')
        {
            for (size_t e = 0u; _eol[e] != '\0'; ++e)
            {
                _out[used++] = _eol[e];
            }

            continue;
        }

        _out[used++] = _text[at];
    }

    _out[used] = '\0';

    return;
}

/*
d_tests_write
  Writes the test file, its line ends spelled as `_eol`.
*/
static
bool d_tests_write(
    const char* _text,
    const char* _eol
)
{
    char        spelt[4096] = "";
    FILE* const out         = fopen(g_path, "wb");

    if (!out)
    {
        return false;
    }

    d_tests_spell(_text, _eol, spelt, sizeof(spelt));

    const size_t length = strlen(spelt);
    const bool   ok     = (fwrite(spelt, 1u, length, out) == length);

    return (fclose(out) == 0) && ok;
}

/*
d_tests_holds
  Whether the test file holds exactly `_text`, its line ends spelled as
`_eol`.
*/
static
bool d_tests_holds(
    const char* _text,
    const char* _eol
)
{
    char        spelt[4096] = "";
    char        read[4096]  = "";
    FILE* const in          = fopen(g_path, "rb");

    if (!in)
    {
        return false;
    }

    const size_t got = fread(read, 1u, sizeof(read), in);

    (void)fclose(in);
    d_tests_spell(_text, _eol, spelt, sizeof(spelt));

    return ( (got == strlen(spelt)) &&
             (memcmp(read, spelt, got) == 0) );
}

/*
d_tests_replace
  d_edit_replace_lines, with the line ends given:
  - one line replaced by two, the new break spelled as the file's
  - three lines replaced by one, the run's last terminator kept
  - the file's last line, which has no terminator, replaced without one
  - a run past the file's end, or backwards, refused, the file unchanged
*/
static
void d_tests_replace(
    const char* _eol,
    const char* _name
)
{
    const char* const start = "one\ntwo\nthree\nfour\n";

    // one line becomes two
    (void)d_tests_write(start, _eol);
    d_tests_check( (d_edit_replace_lines(g_path, 2u, 2u, "2a\n2b")) &&
                   (d_tests_holds("one\n2a\n2b\nthree\nfour\n", _eol)),
                   "replace: one line by two",
                   _name);

    // three lines become one
    (void)d_tests_write(start, _eol);
    d_tests_check( (d_edit_replace_lines(g_path, 1u, 3u, "123")) &&
                   (d_tests_holds("123\nfour\n", _eol)),
                   "replace: three lines by one",
                   _name);

    // the last line has no terminator, and gains none
    (void)d_tests_write("one\ntwo", _eol);
    d_tests_check( (d_edit_replace_lines(g_path, 2u, 2u, "2")) &&
                   (d_tests_holds("one\n2", _eol)),
                   "replace: a last line without a terminator",
                   _name);

    // runs that do not lie within the file
    (void)d_tests_write(start, _eol);
    d_tests_check( (!d_edit_replace_lines(g_path, 4u, 5u, "x"))     &&
                   (!d_edit_replace_lines(g_path, 5u, 5u, "x"))     &&
                   (!d_edit_replace_lines(g_path, 3u, 2u, "x"))     &&
                   (!d_edit_replace_lines(g_path, 0u, 1u, "x"))     &&
                   (d_tests_holds(start, _eol)),
                   "replace: a run outside the file is refused",
                   _name);

    return;
}

/*
d_tests_shift
  d_edit_may_shift_lines and d_edit_shift_lines, with the line ends given:
  - a run moved right by four, its empty line left empty
  - the same run moved back, to what it was
  - left past a line's spaces: refused, and the file unchanged
  - right past 80 characters: refused, and the file unchanged; at exactly
    80 it moves
  - a run past the file's end: refused
*/
static
void d_tests_shift(
    const char* _eol,
    const char* _name
)
{
    const char* const start =
        "a\n"
        "    b\n"
        "\n"
        "    c\n"
        "d\n";
    const char* const moved =
        "a\n"
        "        b\n"
        "\n"
        "        c\n"
        "d\n";

    // right, and back
    (void)d_tests_write(start, _eol);
    d_tests_check( (d_edit_may_shift_lines(g_path, 2u, 4u, 4))  &&
                   (d_edit_shift_lines(g_path, 2u, 4u, 4))      &&
                   (d_tests_holds(moved, _eol)),
                   "shift: right by four, the empty line kept empty",
                   _name);
    d_tests_check( (d_edit_shift_lines(g_path, 2u, 4u, -4)) &&
                   (d_tests_holds(start, _eol)),
                   "shift: left by four, back to the start",
                   _name);

    // further left than a line's spaces go
    d_tests_check( (!d_edit_may_shift_lines(g_path, 2u, 4u, -8)) &&
                   (!d_edit_shift_lines(g_path, 2u, 4u, -8))     &&
                   (d_tests_holds(start, _eol)),
                   "shift: left past the spaces is refused",
                   _name);

    // a line of 76 characters may move four, and not eight
    char line[128] = "";
    char text[256] = "";

    memset(line, 'x', 76u);
    line[76] = '\0';
    (void)snprintf(text, sizeof(text), "%s\nend\n", line);
    (void)d_tests_write(text, _eol);
    d_tests_check( (!d_edit_shift_lines(g_path, 1u, 1u, 8)) &&
                   (d_tests_holds(text, _eol)),
                   "shift: past the eightieth character is refused",
                   _name);
    d_tests_check( d_edit_shift_lines(g_path, 1u, 1u, 4),
                   "shift: to the eightieth character exactly",
                   _name);

    // a run that runs past the last line
    (void)d_tests_write(start, _eol);
    d_tests_check( (!d_edit_may_shift_lines(g_path, 4u, 9u, 4)) &&
                   (!d_edit_may_shift_lines(g_path, 0u, 1u, 4)) &&
                   (d_tests_holds(start, _eol)),
                   "shift: a run past the file is refused",
                   _name);

    return;
}

/*
d_tests_insert
  d_edit_insert_line, with the line ends given: an empty line put in the
middle, a line at the top and one past the last line each take the file's
own terminator; a line past that is refused, and the file left unchanged.
*/
static
void d_tests_insert(
    const char* _eol,
    const char* _name
)
{
    const char* const start = "one\ntwo\n";

    // between two lines, above the first, and past the last
    (void)d_tests_write(start, _eol);
    d_tests_check( (d_edit_insert_line(g_path, 2u, ""))  &&
                   (d_edit_insert_line(g_path, 1u, "0")) &&
                   (d_edit_insert_line(g_path, 5u, "3")) &&
                   (d_tests_holds("0\none\n\ntwo\n3\n", _eol)),
                   "insert: each line ends as the file's do",
                   _name);

    // two past the last line
    (void)d_tests_write(start, _eol);
    d_tests_check( (!d_edit_insert_line(g_path, 4u, "x")) &&
                   (d_tests_holds(start, _eol)),
                   "insert: past the end is refused",
                   _name);

    return;
}

/*
main
  Runs each group with LF and with CRLF, reports the tally, and exits
non-zero on any failure.
*/
int main(
    int    _argc,
    char** _argv
)
{
    const char* const work = (_argc > 1) ? _argv[1] : ".";

    (void)snprintf(g_path, sizeof(g_path), "%s/test_dedit.txt", work);

    d_tests_replace("\n", "LF");
    d_tests_replace("\r\n", "CRLF");
    d_tests_shift("\n", "LF");
    d_tests_shift("\r\n", "CRLF");
    d_tests_insert("\n", "LF");
    d_tests_insert("\r\n", "CRLF");

    (void)remove(g_path);
    printf("%d checks, %d failed\n",
           g_checks,
           g_failed);

    return (g_failed == 0) ? 0 : 1;
}
