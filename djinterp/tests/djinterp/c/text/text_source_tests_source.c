/*******************************************************************************
* djinterp [c][text]                                  text_source_tests_source.c
*
* The text_source suite: every block. A helper asks a source for a line's
* span and compares it with what is expected; another for an offset's line
* and column.
*
*
* path:      /tests/djinterp/c/text/text_source_tests_source.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.17
*                                                            revised: 2026.11.17
*******************************************************************************/
#include "./text_source_tests.h"  // corresponding header

// std
#include <stdio.h>   // fopen, fclose, remove
#include <stdlib.h>  // free
#include <string.h>  // memcmp, strcmp, strlen, strstr


// D_TESTS_TEXT_SOURCE_FILE
//   constant: the file the file tests write, in the working directory.
#define D_TESTS_TEXT_SOURCE_FILE "d_tests_text_source.tmp"


/*
d_tests_text_source_span
  Whether line _line of _source spans [_start, _end).
*/
static bool
d_tests_text_source_span(
    const struct dj_text_source* _source,
    size_t                       _line,
    size_t                       _start,
    size_t                       _end
)
{
    size_t start = 99u;
    size_t end   = 99u;

    return ( (d_text_source_line(_source, _line, &start, &end)) &&
             (start == _start)                                    &&
             (end == _end) );
}

/*
d_tests_text_source_at
  Whether offset _offset of _source is line _line, column _column.
*/
static bool
d_tests_text_source_at(
    const struct dj_text_source* _source,
    size_t                       _offset,
    size_t                       _line,
    size_t                       _column
)
{
    size_t line   = 99u;
    size_t column = 99u;

    return ( (d_text_source_locate(_source, _offset, &line, &column)) &&
             (line == _line)                                          &&
             (column == _column) );
}

/*
d_tests_text_source_exists
  Whether a file can be opened for reading.
*/
static bool
d_tests_text_source_exists(
    const char* _path
)
{
    FILE* const handle = fopen(_path, "rb");

    if (handle)
    {
        (void)fclose(handle);
    }

    return (handle != NULL);
}


/*
d_tests_text_source_new
  A copied source.
  Tests the following:
  - the text is the source's copy, terminated, of the length given
  - a counted text need not be terminated: three bytes of "abcdef"
  - the caller's buffer may change after
*/
bool
d_tests_text_source_new(void)
{
    char                   buffer[8] = "abcdef";
    size_t                 length    = 99u;
    struct dj_text_source* source    = d_text_source_new(buffer, 3u);
    const char*            text;

    D_TEST_REQUIRE(source != NULL);
    buffer[0] = 'X';
    text      = d_text_source_text(source, &length);
    D_TEST_EXPECT(text != buffer);
    D_TEST_EXPECT(length == 3u);
    D_TEST_EXPECT(memcmp(text, "abc", 4u) == 0);
    D_TEST_EXPECT(d_text_source_line_count(source) == 1u);
    d_text_source_free(source);

    source = d_text_source_new("", 0u);
    D_TEST_REQUIRE(source != NULL);
    D_TEST_EXPECT(strcmp(d_text_source_text(source, NULL), "") == 0);
    D_TEST_EXPECT(d_text_source_line_count(source) == 0u);
    d_text_source_free(source);

    return true;
}

/*
d_tests_text_source_borrow
  A borrowed source.
  Tests the following:
  - the text is the caller's own pointer, and its lines are indexed
*/
bool
d_tests_text_source_borrow(void)
{
    static const char      text[] = "one\ntwo\n";
    size_t                 length = 99u;
    struct dj_text_source* source = d_text_source_borrow(text, 8u);

    D_TEST_REQUIRE(source != NULL);
    D_TEST_EXPECT(d_text_source_text(source, &length) == text);
    D_TEST_EXPECT(length == 8u);
    D_TEST_EXPECT(d_text_source_line_count(source) == 2u);
    d_text_source_free(source);

    return true;
}

/*
d_tests_text_source_null
  NULL anywhere.
  Tests the following:
  - NULL text with a length is refused by new and borrow; with none, it is
    the empty text
  - free, text, line_count, line and locate take a NULL source
*/
bool
d_tests_text_source_null(void)
{
    struct dj_text_source* source;
    size_t                 length = 99u;

    D_TEST_EXPECT(d_text_source_new(NULL, 1u) == NULL);
    D_TEST_EXPECT(d_text_source_borrow(NULL, 1u) == NULL);
    D_TEST_EXPECT(d_text_source_read(NULL) == NULL);

    source = d_text_source_new(NULL, 0u);
    D_TEST_REQUIRE(source != NULL);
    D_TEST_EXPECT(d_text_source_line_count(source) == 0u);
    d_text_source_free(source);

    source = d_text_source_borrow(NULL, 0u);
    D_TEST_REQUIRE(source != NULL);
    D_TEST_EXPECT(d_text_source_line_count(source) == 0u);
    d_text_source_free(source);

    d_text_source_free(NULL);
    D_TEST_EXPECT(d_text_source_text(NULL, &length) == NULL);
    D_TEST_EXPECT(length == 0u);
    D_TEST_EXPECT(d_text_source_line_count(NULL) == 0u);
    D_TEST_EXPECT(!d_text_source_line(NULL, 1u, NULL, NULL));
    D_TEST_EXPECT(!d_text_source_locate(NULL, 0u, NULL, NULL));

    return true;
}

/*
d_tests_text_source_line_count
  Counting lines.
  Tests the following:
  - "" has none; "a", "a\n" and "\n" have one
  - "a\nb" and "a\n\n" have two: an empty line counts when something, even
    a newline, follows it
*/
bool
d_tests_text_source_line_count(void)
{
    static const char* const texts[]  = { "", "a", "a\n", "\n", "a\nb",
                                          "a\n\n", "\n\n\n" };
    static const size_t      counts[] = { 0u, 1u, 1u, 1u, 2u, 2u, 3u };
    size_t                   at;
    bool                     ok = true;

    for (at = 0u; at < (sizeof(counts) / sizeof(counts[0])); ++at)
    {
        struct dj_text_source* const source =
            d_text_source_new(texts[at], strlen(texts[at]));

        if ( (!source) ||
             (d_text_source_line_count(source) != counts[at]) )
        {
            ok = false;
        }

        d_text_source_free(source);
    }

    D_TEST_EXPECT(ok);

    return true;
}

/*
d_tests_text_source_line_spans
  Each line's span.
  Tests the following:
  - in "ab\n\ncde", lines are [0,2), [3,3) and [4,7)
  - a last line ending in a newline excludes it
  - line 0 and a line past the last give false and write nothing
*/
bool
d_tests_text_source_line_spans(void)
{
    struct dj_text_source* source = d_text_source_new("ab\n\ncde", 7u);
    size_t                 start  = 99u;
    size_t                 end    = 99u;

    D_TEST_REQUIRE(source != NULL);
    D_TEST_EXPECT(d_tests_text_source_span(source, 1u, 0u, 2u));
    D_TEST_EXPECT(d_tests_text_source_span(source, 2u, 3u, 3u));
    D_TEST_EXPECT(d_tests_text_source_span(source, 3u, 4u, 7u));
    D_TEST_EXPECT(!d_text_source_line(source, 0u, &start, &end));
    D_TEST_EXPECT(!d_text_source_line(source, 4u, &start, &end));
    D_TEST_EXPECT( (start == 99u) && (end == 99u) );
    D_TEST_EXPECT(d_text_source_line(source, 2u, NULL, NULL));
    d_text_source_free(source);

    source = d_text_source_new("x\ny\n", 4u);
    D_TEST_REQUIRE(source != NULL);
    D_TEST_EXPECT(d_tests_text_source_span(source, 2u, 2u, 3u));
    d_text_source_free(source);

    return true;
}

/*
d_tests_text_source_line_crlf
  Windows line endings.
  Tests the following:
  - "a\r\nb\r\n" has two lines, and each span keeps its CR
  - a lone CR does not end a line
*/
bool
d_tests_text_source_line_crlf(void)
{
    struct dj_text_source* source = d_text_source_new("a\r\nb\r\n", 6u);

    D_TEST_REQUIRE(source != NULL);
    D_TEST_EXPECT(d_text_source_line_count(source) == 2u);
    D_TEST_EXPECT(d_tests_text_source_span(source, 1u, 0u, 2u));
    D_TEST_EXPECT(d_tests_text_source_span(source, 2u, 3u, 5u));
    d_text_source_free(source);

    source = d_text_source_new("a\rb", 3u);
    D_TEST_REQUIRE(source != NULL);
    D_TEST_EXPECT(d_text_source_line_count(source) == 1u);
    d_text_source_free(source);

    return true;
}

/*
d_tests_text_source_locate
  Telling offsets as lines and columns.
  Tests the following:
  - in "ab\ncd", offsets 0 to 4 are (1,1), (1,2), (1,3) - the newline, the
    last byte of line 1 - (2,1) and (2,2)
  - on many lines, the binary search finds each line's first and last byte
  - an offset past the end gives false and writes nothing
*/
bool
d_tests_text_source_locate(void)
{
    struct dj_text_source* source = d_text_source_new("ab\ncd", 5u);
    char                   text[300];
    size_t                 line   = 99u;
    size_t                 at;
    bool                   ok = true;

    D_TEST_REQUIRE(source != NULL);
    D_TEST_EXPECT(d_tests_text_source_at(source, 0u, 1u, 1u));
    D_TEST_EXPECT(d_tests_text_source_at(source, 1u, 1u, 2u));
    D_TEST_EXPECT(d_tests_text_source_at(source, 2u, 1u, 3u));
    D_TEST_EXPECT(d_tests_text_source_at(source, 3u, 2u, 1u));
    D_TEST_EXPECT(d_tests_text_source_at(source, 4u, 2u, 2u));
    D_TEST_EXPECT(!d_text_source_locate(source, 6u, &line, NULL));
    D_TEST_EXPECT(line == 99u);
    d_text_source_free(source);

    // one hundred lines of "xy\n": line n starts at 3(n - 1)
    for (at = 0u; at < 100u; ++at)
    {
        text[(at * 3u)]      = 'x';
        text[(at * 3u) + 1u] = 'y';
        text[(at * 3u) + 2u] = '\n';
    }

    source = d_text_source_borrow(text, 300u);
    D_TEST_REQUIRE(source != NULL);
    D_TEST_EXPECT(d_text_source_line_count(source) == 100u);

    for (at = 0u; at < 100u; ++at)
    {
        if ( (!d_tests_text_source_at(source, at * 3u, at + 1u, 1u)) ||
             (!d_tests_text_source_at(source, (at * 3u) + 2u,
                                      at + 1u, 3u)) )
        {
            ok = false;
        }
    }

    D_TEST_EXPECT(ok);
    d_text_source_free(source);

    return true;
}

/*
d_tests_text_source_locate_end
  The text's end.
  Tests the following:
  - "ab" ends at (1,3), after its last byte
  - "ab\n" ends at (2,1), the line a following text would start
  - "" ends at (1,1)
*/
bool
d_tests_text_source_locate_end(void)
{
    struct dj_text_source* source = d_text_source_new("ab", 2u);

    D_TEST_REQUIRE(source != NULL);
    D_TEST_EXPECT(d_tests_text_source_at(source, 2u, 1u, 3u));
    d_text_source_free(source);

    source = d_text_source_new("ab\n", 3u);
    D_TEST_REQUIRE(source != NULL);
    D_TEST_EXPECT(d_tests_text_source_at(source, 3u, 2u, 1u));
    d_text_source_free(source);

    source = d_text_source_new("", 0u);
    D_TEST_REQUIRE(source != NULL);
    D_TEST_EXPECT(d_tests_text_source_at(source, 0u, 1u, 1u));
    D_TEST_EXPECT(!d_text_source_locate(source, 1u, NULL, NULL));
    d_text_source_free(source);

    return true;
}

/*
d_tests_text_source_write_read
  A round trip through a file.
  Tests the following:
  - d_text_write writes the bytes, a CR before a newline and a NUL among
    them, and d_text_read gives them back, terminated
  - d_text_source_read makes a source over the same bytes
  - an empty text writes an empty file
*/
bool
d_tests_text_source_write_read(void)
{
    static const char      bytes[] = "one\r\ntwo\0three\n";
    const size_t           length  = sizeof(bytes) - 1u;
    char                   problem[128] = "";
    size_t                 read_length  = 99u;
    char*                  text;
    struct dj_text_source* source;

    D_TEST_REQUIRE(d_text_write(D_TESTS_TEXT_SOURCE_FILE, bytes, length,
                                problem, sizeof(problem)));
    text = d_text_read(D_TESTS_TEXT_SOURCE_FILE, &read_length);
    D_TEST_EXPECT(text != NULL);
    D_TEST_EXPECT(read_length == length);
    D_TEST_EXPECT( (text) && (memcmp(text, bytes, length) == 0) &&
                   (text[length] == '\0') );
    free(text);

    source = d_text_source_read(D_TESTS_TEXT_SOURCE_FILE);
    D_TEST_EXPECT(source != NULL);
    D_TEST_EXPECT(d_text_source_line_count(source) == 2u);
    d_text_source_free(source);

    D_TEST_EXPECT(d_text_write(D_TESTS_TEXT_SOURCE_FILE, NULL, 0u,
                               problem, sizeof(problem)));
    text = d_text_read(D_TESTS_TEXT_SOURCE_FILE, &read_length);
    D_TEST_EXPECT( (text) && (read_length == 0u) && (text[0] == '\0') );
    free(text);
    (void)remove(D_TESTS_TEXT_SOURCE_FILE);

    return true;
}

/*
d_tests_text_source_write_replaces
  Writing over a file.
  Tests the following:
  - a longer file written over by a shorter text holds the shorter text
    only
  - no <path>.part file is left
  - a text larger than the first read chunk (4096 bytes) reads back whole
*/
bool
d_tests_text_source_write_replaces(void)
{
    static char problem[128];
    static char big[10000];
    size_t      length = 0u;
    size_t      at;
    char*       text;

    for (at = 0u; at < sizeof(big); ++at)
    {
        big[at] = (char)('a' + (at % 26u));
    }

    D_TEST_REQUIRE(d_text_write(D_TESTS_TEXT_SOURCE_FILE, big, sizeof(big),
                                problem, sizeof(problem)));
    text = d_text_read(D_TESTS_TEXT_SOURCE_FILE, &length);
    D_TEST_EXPECT( (text) && (length == sizeof(big)) &&
                   (memcmp(text, big, sizeof(big)) == 0) );
    free(text);

    D_TEST_REQUIRE(d_text_write(D_TESTS_TEXT_SOURCE_FILE, "short", 5u,
                                problem, sizeof(problem)));
    text = d_text_read(D_TESTS_TEXT_SOURCE_FILE, &length);
    D_TEST_EXPECT( (text) && (length == 5u) &&
                   (memcmp(text, "short", 5u) == 0) );
    free(text);
    D_TEST_EXPECT(!d_tests_text_source_exists(D_TESTS_TEXT_SOURCE_FILE
                                              ".part"));
    (void)remove(D_TESTS_TEXT_SOURCE_FILE);

    return true;
}

/*
d_tests_text_source_read_missing
  A file that is not there.
  Tests the following:
  - d_text_read gives NULL and length 0; d_text_source_read gives NULL
  - a NULL length is allowed
*/
bool
d_tests_text_source_read_missing(void)
{
    size_t length = 99u;

    (void)remove("d_tests_text_source_missing.tmp");
    D_TEST_EXPECT(d_text_read("d_tests_text_source_missing.tmp",
                              &length) == NULL);
    D_TEST_EXPECT(length == 0u);
    D_TEST_EXPECT(d_text_read("d_tests_text_source_missing.tmp",
                              NULL) == NULL);
    D_TEST_EXPECT(d_text_source_read("d_tests_text_source_missing.tmp") ==
                  NULL);

    return true;
}

/*
d_tests_text_source_write_fails
  A write that cannot be made.
  Tests the following:
  - into a directory that does not exist: false, a problem that names the
    file, and no part file
  - a NULL path, or NULL text with a length: false, a problem
  - a NULL problem buffer is allowed
*/
bool
d_tests_text_source_write_fails(void)
{
    static const char path[] = "d_tests_text_source_no_dir/sub/file.txt";
    char              problem[160] = "";

    D_TEST_EXPECT(!d_text_write(path, "x", 1u, problem, sizeof(problem)));
    D_TEST_EXPECT(strstr(problem, "d_tests_text_source_no_dir") != NULL);
    D_TEST_EXPECT(!d_tests_text_source_exists(
                      "d_tests_text_source_no_dir/sub/file.txt.part"));

    problem[0] = '\0';
    D_TEST_EXPECT(!d_text_write(NULL, "x", 1u, problem, sizeof(problem)));
    D_TEST_EXPECT(problem[0] != '\0');

    problem[0] = '\0';
    D_TEST_EXPECT(!d_text_write(D_TESTS_TEXT_SOURCE_FILE, NULL, 1u,
                                problem, sizeof(problem)));
    D_TEST_EXPECT(problem[0] != '\0');
    D_TEST_EXPECT(!d_text_write(path, "x", 1u, NULL, 0u));

    return true;
}
