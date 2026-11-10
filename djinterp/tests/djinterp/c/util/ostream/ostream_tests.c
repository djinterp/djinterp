/*******************************************************************************
* djinterp [test]                                                ostream_tests.c
*
*   The output stream's suite, in DTest's C face (c/modules/test/test_suite.h):
* the kernel (c/util/ostream/ostream.h), its destinations -- a sink, a fixed
* array, a dj_string, a fan-out, a FILE*, the console -- the cursors, and the
* formatting unit (c/text/ostream_format.h). Each group below is a test of
* the module "ostream", in a block per header, and each of its checks is
* recorded against that test, so a failing check does not stop its group.
*
*   A DIFFERENTIAL TEST CARRIES MOST OF THE WEIGHT. d_tests_ostream_random
* replays thousands of random operation sequences -- writes of every length,
* single characters, cursor batches, reserve and commit, integers, fills,
* flushes -- onto every kind of destination at once, and holds each to a
* plain byte array that received the same bytes. Window boundaries, drains,
* growth and spills fall wherever the dice put them.
*
*   The double checks under de_DE.UTF-8 and ps_AF.UTF-8 are left out, with a
* note, where those locales are not installed (localedef -i de_DE -f UTF-8
* de_DE.UTF-8). The console check redirects stdout, so it runs on POSIX only
* and is skipped elsewhere.
*
*
* path:      /tests/djinterp/c/util/ostream/ostream_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.11.09
*******************************************************************************/
#include "ostream_tests.h"  // the suite's entry point, DTest's C face
// std
#include <errno.h>    // errno, ENOENT, EIO
#include <limits.h>   // INT_MIN, LONG_MIN, LONG_MAX, ULONG_MAX
#include <locale.h>   // setlocale
#include <math.h>     // NAN, INFINITY
#include <stdbool.h>  // bool
#include <stdio.h>    // printf, snprintf, fopen, FILE
#include <stdlib.h>   // malloc, free
#include <string.h>   // memcmp, memset, strlen, strcmp
// djinterp
#include "../../../../../inc/djinterp/c/util/ostream/ostream.h"  // the kernel
#include "../../../../../inc/djinterp/c/util/ostream/ostream_console.h"
                                                                      // console
#include "../../../../../inc/djinterp/c/util/ostream/ostream_file.h"  // FILE*
#include "../../../../../inc/djinterp/c/text/ostream_format.h"        // doubles
#include "../../../../../inc/djinterp/c/util/sink_common.h"  // d_sink_emit
#include "../../../../../inc/djinterp/c/dstring.h"  // dj_string
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"
                                                // uint64_t, UINT64_MAX

#if (D_INTERNAL_OSTREAM_CONSOLE == D_CFG_OSTREAM_CONSOLE_POSIX)
    #include <fcntl.h>    // open, O_WRONLY, O_CREAT, O_TRUNC
    #include <pthread.h>  // pthread_create, pthread_join
    #include <unistd.h>   // dup, dup2, close

    // D_TESTS_OSTREAM_THREADS
    //   constant: 1 where the locale race can be run (POSIX threads).
    #define D_TESTS_OSTREAM_THREADS  1
#else
    #define D_TESTS_OSTREAM_THREADS  0
#endif


// D_TESTS_OSTREAM_CHECK
//   macro: one check, recorded against the test open in the suite.
#define D_TESTS_OSTREAM_CHECK(expr)                                            \
    d_tests_ostream_check(((expr) ? 1 : 0), #expr, __LINE__)

// D_TESTS_OSTREAM_CAPACITY
//   constant: the bytes a test sink holds.
#define D_TESTS_OSTREAM_CAPACITY  (1u << 20)

// d_tests_ostream_suite
//   the suite being built, for the checks; set by d_tests_ostream_all.
static struct dj_test_suite* d_tests_ostream_suite;

// d_tests_ostream_failures
//   the checks that failed, so the differential test can stop at its first
// failing run.
static size_t d_tests_ostream_failures;

/*
d_tests_ostream_check
  Records one check against the open test, and counts it when it failed.
*/
static bool
d_tests_ostream_check(
    int         _ok,
    const char* _expression,
    int         _line
)
{
    if (_ok == 0)
    {
        ++d_tests_ostream_failures;
    }

    return d_test_suite_check(d_tests_ostream_suite,
                              (_ok != 0),
                              _expression,
                              __FILE__,
                              _line);
}


//==============================================================================
// A TEST SINK
//==============================================================================

// dj_tests_ostream_sink
//   struct: a sink's context that keeps what it is given, counts its calls,
// and refuses everything past fail_after bytes when that is not 0.
struct dj_tests_ostream_sink
{
    unsigned char* data;
    size_t         size;
    size_t         calls;
    size_t         fail_after;
    int            fail_errno;
    int            flushes;
    int            closes;
    int            released;
};

/*
d_tests_ostream_sink_write
  Keeps the bytes, unless they would pass fail_after; then sets errno and
takes none.
*/
static size_t
d_tests_ostream_sink_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    struct dj_tests_ostream_sink* const sink =
        (struct dj_tests_ostream_sink*)_context;

    ++sink->calls;

    if ( ( (sink->fail_after != 0u) &&
           ((sink->size + _size) > sink->fail_after) ) ||
         ((sink->size + _size) > D_TESTS_OSTREAM_CAPACITY) )
    {
        errno = sink->fail_errno;

        return 0u;
    }

    memcpy(sink->data + sink->size, _data, _size);
    sink->size += _size;

    return _size;
}

/*
d_tests_ostream_sink_flush
  Counts a flush.
*/
static int32_t
d_tests_ostream_sink_flush(
    void* _context
)
{
    ++((struct dj_tests_ostream_sink*)_context)->flushes;

    return 1;
}

/*
d_tests_ostream_sink_close
  Counts a close, and whether it released.
*/
static int32_t
d_tests_ostream_sink_close(
    void*   _context,
    int32_t _release
)
{
    struct dj_tests_ostream_sink* const sink =
        (struct dj_tests_ostream_sink*)_context;

    ++sink->closes;
    sink->released += (_release != 0) ? 1 : 0;

    return 1;
}

// d_tests_ostream_sink_ops
//   table: the test sink's operations.
static const struct dj_ostream_ops d_tests_ostream_sink_ops =
{
    sizeof(struct dj_ostream_ops),
    NULL,
    &d_tests_ostream_sink_flush,
    &d_tests_ostream_sink_close,
    NULL
};

/*
d_tests_ostream_sink_new
  A test sink with its storage.
*/
static struct dj_tests_ostream_sink
d_tests_ostream_sink_new(void)
{
    struct dj_tests_ostream_sink sink;

    memset(&sink, 0, sizeof(sink));
    sink.data       = (unsigned char*)malloc(D_TESTS_OSTREAM_CAPACITY);
    sink.fail_errno = EIO;

    return sink;
}

/*
d_tests_ostream_as_pack
  The test sink as a struct dj_pack_sink.
*/
static struct dj_pack_sink
d_tests_ostream_as_pack(
    struct dj_tests_ostream_sink* _sink
)
{
    struct dj_pack_sink sink;

    sink.write   = &d_tests_ostream_sink_write;
    sink.context = _sink;

    return sink;
}

/*
d_tests_ostream_sink_is
  Whether a test sink holds exactly a C string's bytes.
*/
static bool
d_tests_ostream_sink_is(
    const struct dj_tests_ostream_sink* _sink,
    const char*                         _expected
)
{
    const size_t length = strlen(_expected);

    return ( (_sink->size == length) &&
             (memcmp(_sink->data, _expected, length) == 0) );
}


//==============================================================================
// GROUPS
//==============================================================================

/*
d_tests_ostream_init
  The contract of d_ostream_init and its refusals.
  Tests the following:
  - a transport is accepted, unbuffered, and ready
  - neither a write nor a grow, and both, are refused as INVALID
  - OWNS_CONTEXT without a close slot is refused, and the stream owns nothing
  - a table smaller than its first version is refused
  - a refused stream's writes, flush and close are safe and fail
*/
static bool
d_tests_ostream_init(void)
{
    struct dj_tests_ostream_sink sink = d_tests_ostream_sink_new();
    struct dj_ostream            os;
    struct dj_pack_sink          none;
    struct dj_ostream_ops        small = d_tests_ostream_sink_ops;

    none.write   = NULL;
    none.context = NULL;
    small.size   = sizeof(size_t);

    D_TESTS_OSTREAM_CHECK(d_ostream_init(&os,
                                         d_tests_ostream_as_pack(&sink),
                                         NULL,
                                         0) == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_is_ok(&os) == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_init(&os, none, NULL, 0) == 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_status(&os) == D_OSTREAM_INVALID);
    D_TESTS_OSTREAM_CHECK(d_ostream_last_errno(&os) == EINVAL);
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&os, "x") == 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_flush(&os) == 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_close(&os) == 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_init(&os,
                                         d_tests_ostream_as_pack(&sink),
                                         NULL,
                                         D_OSTREAM_OWNS_CONTEXT) == 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_close(&os) == 0);
    D_TESTS_OSTREAM_CHECK(sink.closes == 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_init(&os,
                                         d_tests_ostream_as_pack(&sink),
                                         &small,
                                         0) == 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_init(NULL,
                                         d_tests_ostream_as_pack(&sink),
                                         NULL,
                                         0) == 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_status(NULL) == D_OSTREAM_INVALID);
    D_TESTS_OSTREAM_CHECK(d_ostream_written(NULL) == 0u);
    free(sink.data);

    return true;
}

/*
d_tests_ostream_transport
  A transport's buffering and delivery.
  Tests the following:
  - unbuffered: every write is one sink call
  - buffered: writes gather until the buffer fills or the stream flushes
  - a write at least the buffer's size goes straight on, after the buffer
  - set_buffer delivers the old buffer first
  - written counts what is buffered and what is delivered
  - flush calls the flush slot; close calls the close slot, releasing only
    an owned context
  - a write after close is refused and recorded as CLOSED
*/
static bool
d_tests_ostream_transport(void)
{
    struct dj_tests_ostream_sink sink = d_tests_ostream_sink_new();
    struct dj_ostream            os;
    unsigned char                buffer[8];

    (void)d_ostream_init(&os,
                         d_tests_ostream_as_pack(&sink),
                         &d_tests_ostream_sink_ops,
                         D_OSTREAM_OWNS_CONTEXT);
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&os, "ab") == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_write_char(&os, 'c') == 1);
    D_TESTS_OSTREAM_CHECK(sink.calls == 2u);
    D_TESTS_OSTREAM_CHECK(d_ostream_set_buffer(&os, buffer, sizeof(buffer)));
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&os, "defg") == 1);
    D_TESTS_OSTREAM_CHECK(sink.calls == 2u);
    D_TESTS_OSTREAM_CHECK(d_ostream_written(&os) == 7u);
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&os, "hijklmnop") == 1);
    D_TESTS_OSTREAM_CHECK(d_tests_ostream_sink_is(&sink, "abcdefghijklmnop"));
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&os, "q") == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_set_buffer(&os, NULL, 0u) == 1);
    D_TESTS_OSTREAM_CHECK(d_tests_ostream_sink_is(&sink, "abcdefghijklmnopq"));
    D_TESTS_OSTREAM_CHECK(d_ostream_set_buffer(&os, buffer, sizeof(buffer)));
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&os, "rs") == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_flush(&os) == 1);
    D_TESTS_OSTREAM_CHECK(sink.flushes == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&os, "t") == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_close(&os) == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_close(&os) == 1);
    D_TESTS_OSTREAM_CHECK( (sink.closes == 1) &&
                           (sink.released == 1) );
    D_TESTS_OSTREAM_CHECK(d_tests_ostream_sink_is(&sink,
                                                  "abcdefghijklmnopqrst"));
    D_TESTS_OSTREAM_CHECK(d_ostream_written(&os) == 20u);
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&os, "u") == 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_status(&os) == D_OSTREAM_CLOSED);
    D_TESTS_OSTREAM_CHECK(d_ostream_write(&os, "", 0u) == 0);
    free(sink.data);

    return true;
}

/*
d_tests_ostream_failure
  The sticky status.
  Tests the following:
  - a refused delivery fails the stream with the sink's errno, at the drain
    that meets it
  - later writes are refused without reaching the sink
  - the caller's errno is restored on success
  - close reports the failure, and an owned context is still released
  - d_ostream_fail keeps the first failure only
  - a null write records INVALID
*/
static bool
d_tests_ostream_failure(void)
{
    struct dj_tests_ostream_sink sink = d_tests_ostream_sink_new();
    struct dj_ostream            os;
    unsigned char                buffer[4];

    sink.fail_after = 6u;
    sink.fail_errno = ENOSPC;
    (void)d_ostream_init(&os,
                         d_tests_ostream_as_pack(&sink),
                         &d_tests_ostream_sink_ops,
                         D_OSTREAM_OWNS_CONTEXT);
    (void)d_ostream_set_buffer(&os, buffer, sizeof(buffer));
    errno = 1234;
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&os, "abcd") == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&os, "ef") == 1);
    D_TESTS_OSTREAM_CHECK(errno == 1234);
    // "ghi" is buffered; the drain that refuses it comes with the next write
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&os, "ghi") == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&os, "jk") == 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_status(&os) == D_OSTREAM_FAILED);
    D_TESTS_OSTREAM_CHECK(d_ostream_last_errno(&os) == ENOSPC);

    const size_t calls = sink.calls;

    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&os, "x") == 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_write_char(&os, 'x') == 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_reserve(&os, 1u) == NULL);
    D_TESTS_OSTREAM_CHECK(sink.calls == calls);
    D_TESTS_OSTREAM_CHECK(d_ostream_fail(&os, D_OSTREAM_INVALID, 1) == 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_status(&os) == D_OSTREAM_FAILED);
    D_TESTS_OSTREAM_CHECK(d_ostream_close(&os) == 0);
    D_TESTS_OSTREAM_CHECK(sink.released == 1);

    sink.fail_after = 0u;
    (void)d_ostream_init(&os, d_tests_ostream_as_pack(&sink), NULL, 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_write(&os, NULL, 3u) == 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_status(&os) == D_OSTREAM_INVALID);
    (void)d_ostream_init(&os, d_tests_ostream_as_pack(&sink), NULL, 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&os, NULL) == 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_status(&os) == D_OSTREAM_INVALID);
    free(sink.data);

    return true;
}

/*
d_tests_ostream_line_buffering
  A line-buffered transport.
  Tests the following:
  - bytes without a newline stay buffered
  - a write holding a newline drains the whole buffer
  - reserve and commit drain on a committed newline
  - a cursor batch is checked once, at give
*/
static bool
d_tests_ostream_line_buffering(void)
{
    struct dj_tests_ostream_sink sink = d_tests_ostream_sink_new();
    struct dj_ostream            os;
    unsigned char                buffer[32];

    (void)d_ostream_init(&os,
                         d_tests_ostream_as_pack(&sink),
                         NULL,
                         D_OSTREAM_LINE_BUFFERED);
    (void)d_ostream_set_buffer(&os, buffer, sizeof(buffer));
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&os, "ab") == 1);
    D_TESTS_OSTREAM_CHECK(sink.size == 0u);
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&os, "c\nd") == 1);
    D_TESTS_OSTREAM_CHECK(d_tests_ostream_sink_is(&sink, "abc\nd"));

    char* room = d_ostream_reserve(&os, 3u);

    D_TESTS_OSTREAM_CHECK(room != NULL);

    if (room != NULL)
    {
        memcpy(room, "e\nf", 3u);
        D_TESTS_OSTREAM_CHECK(d_ostream_commit(&os, 3u) == 1);
        D_TESTS_OSTREAM_CHECK(d_tests_ostream_sink_is(&sink, "abc\nde\nf"));
    }

    struct dj_ostream_cursor cursor = d_ostream_cursor_take(&os);

    cursor = d_ostream_cursor_write(&os, cursor, "gh", 2u);
    cursor = d_ostream_cursor_put(&os, cursor, '\n');
    D_TESTS_OSTREAM_CHECK(sink.size == 8u);
    D_TESTS_OSTREAM_CHECK(d_ostream_cursor_give(&os, cursor) == 1);
    D_TESTS_OSTREAM_CHECK(d_tests_ostream_sink_is(&sink, "abc\nde\nfgh\n"));
    D_TESTS_OSTREAM_CHECK(d_ostream_close(&os) == 1);
    free(sink.data);

    return true;
}

/*
d_tests_ostream_fixed
  The fixed destination's snprintf contract.
  Tests the following:
  - what fits is stored and terminated; the rest is counted
  - an exact fit is not truncation; one byte more is
  - reserve near the end gives no room rather than a short one
  - capacity 0 counts only, and set_buffer is refused for storage
*/
static bool
d_tests_ostream_fixed(void)
{
    struct dj_ostream       os;
    struct dj_ostream_fixed fixed;
    char                    array[8];

    D_TESTS_OSTREAM_CHECK(d_ostream_init_fixed(&os, &fixed, array, 8u, 1));
    D_TESTS_OSTREAM_CHECK(array[0] == '\0');
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&os, "abc") == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_flush(&os) == 1);
    D_TESTS_OSTREAM_CHECK(strcmp(array, "abc") == 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_reserve(&os, 5u) == NULL);
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&os, "defg") == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_flush(&os) == 1);
    D_TESTS_OSTREAM_CHECK( (strcmp(array, "abcdefg") == 0) &&
                           (d_ostream_fixed_truncated(&fixed) == 0) );
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&os, "hij") == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_close(&os) == 1);
    D_TESTS_OSTREAM_CHECK(strcmp(array, "abcdefg") == 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_fixed_length(&fixed) == 7u);
    D_TESTS_OSTREAM_CHECK(d_ostream_fixed_truncated(&fixed) == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_written(&os) == 10u);

    D_TESTS_OSTREAM_CHECK(d_ostream_init_fixed(&os, &fixed, NULL, 0u, 0));
    D_TESTS_OSTREAM_CHECK(d_ostream_set_buffer(&os, array, 8u) == 0);

    for (int i = 0; i < 1000; ++i)
    {
        (void)d_ostream_write_cstr(&os, "0123456789");
    }

    D_TESTS_OSTREAM_CHECK(d_ostream_close(&os) == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_written(&os) == 10000u);

    D_TESTS_OSTREAM_CHECK(d_ostream_init_fixed(&os, &fixed, array, 4u, 0));
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&os, "wxyz") == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_close(&os) == 1);
    D_TESTS_OSTREAM_CHECK( (memcmp(array, "wxyz", 4u) == 0) &&
                           (d_ostream_fixed_truncated(&fixed) == 0) );

    return true;
}

/*
d_tests_ostream_string
  The dj_string destination, written in place.
  Tests the following:
  - appends after what the string held, and is current after flush
  - grows from no capacity, through many doublings, and stays terminated
  - reserve and commit write into the string's own capacity
  - a cursor writes into the string
*/
static bool
d_tests_ostream_string(void)
{
    struct dj_string* const string = d_string_new_from_cstr("pre:");
    struct dj_ostream       os;

    D_TESTS_OSTREAM_CHECK(d_ostream_init_string(&os, string) == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&os, "abc") == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_flush(&os) == 1);
    D_TESTS_OSTREAM_CHECK(strcmp(d_string_cstr(string), "pre:abc") == 0);
    D_TESTS_OSTREAM_CHECK(d_string_length(string) == 7u);

    for (int i = 0; i < 10000; ++i)
    {
        (void)d_ostream_write_int(&os, i % 10);
    }

    char* const room = d_ostream_reserve(&os, 4u);

    D_TESTS_OSTREAM_CHECK(room != NULL);

    if (room != NULL)
    {
        memcpy(room, "WXYZ", 4u);
        (void)d_ostream_commit(&os, 4u);
    }

    struct dj_ostream_cursor cursor = d_ostream_cursor_take(&os);

    cursor = d_ostream_cursor_put(&os, cursor, '!');
    D_TESTS_OSTREAM_CHECK(d_ostream_cursor_give(&os, cursor) == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_close(&os) == 1);
    D_TESTS_OSTREAM_CHECK(d_string_length(string) == 7u + 10000u + 5u);
    D_TESTS_OSTREAM_CHECK(strlen(d_string_cstr(string)) ==
                          d_string_length(string));
    D_TESTS_OSTREAM_CHECK(memcmp(d_string_cstr(string) + 7u, "0123", 4u) == 0);
    D_TESTS_OSTREAM_CHECK(strcmp(d_string_cstr(string) + 10007u,
                                 "WXYZ!") == 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_init_string(&os, NULL) == 0);
    d_string_free(string);

    return true;
}

/*
d_tests_ostream_numbers
  The integer writers, in place and through a local array.
  Tests the following:
  - the extremes of every width, and zero
  - the same digits whether the window has room or the stream is unbuffered
  - every digit count: each power of ten, one either side, and 2^32
  - hexadecimal, padded and not
*/
static bool
d_tests_ostream_numbers(void)
{
    struct dj_tests_ostream_sink sink = d_tests_ostream_sink_new();
    char                         expected[256];
    char                         array[256];
    struct dj_ostream_fixed      fixed;

    for (int pass = 0; pass < 2; ++pass)
    {
        struct dj_ostream os;

        sink.size = 0u;

        if (pass == 0)
        {
            (void)d_ostream_init_fixed(&os, &fixed, array, sizeof(array), 1);
        }
        else
        {
            (void)d_ostream_init(&os, d_tests_ostream_as_pack(&sink), NULL, 0);
        }

        (void)d_ostream_write_int(&os, INT_MIN);
        (void)d_ostream_write_char(&os, ' ');
        (void)d_ostream_write_long(&os, LONG_MIN);
        (void)d_ostream_write_char(&os, ' ');
        (void)d_ostream_write_ulong(&os, ULONG_MAX);
        (void)d_ostream_write_char(&os, ' ');
        (void)d_ostream_write_int64(&os, INT64_MIN);
        (void)d_ostream_write_char(&os, ' ');
        (void)d_ostream_write_uint64(&os, UINT64_MAX);
        (void)d_ostream_write_char(&os, ' ');
        (void)d_ostream_write_uint(&os, 0u);
        (void)d_ostream_write_char(&os, ' ');
        (void)d_ostream_write_size(&os, (size_t)4294967296ull);
        (void)d_ostream_write_char(&os, ' ');
        (void)d_ostream_write_hex(&os, 0xBEEFu, 8);
        (void)d_ostream_write_char(&os, ' ');
        (void)d_ostream_write_hex(&os, 0u, 0);
        D_TESTS_OSTREAM_CHECK(d_ostream_close(&os) == 1);

        (void)snprintf(expected,
                       sizeof(expected),
                       "%d %ld %lu %lld %llu 0 %llu 0000beef 0",
                       INT_MIN,
                       LONG_MIN,
                       ULONG_MAX,
                       (long long)INT64_MIN,
                       (unsigned long long)UINT64_MAX,
                       (unsigned long long)(size_t)4294967296ull);

        D_TESTS_OSTREAM_CHECK( (pass == 0)
                                   ? (strcmp(array, expected) == 0)
                                   : d_tests_ostream_sink_is(&sink,
                                                             expected) );
    }

    // every power of ten, either side of it, and the 32-bit edge
    uint64_t power = 1u;

    for (int k = 0; k < 20; ++k)
    {
        const uint64_t probes[5] = { power - 1u, power, power + 1u,
                                     (uint64_t)UINT32_MAX,
                                     (uint64_t)UINT32_MAX + 1u };

        for (size_t i = 0u; i < 5u; ++i)
        {
            struct dj_ostream os;

            (void)d_ostream_init_fixed(&os, &fixed, array, sizeof(array), 1);
            (void)d_ostream_write_uint64(&os, probes[i]);
            (void)d_ostream_close(&os);
            (void)snprintf(expected,
                           sizeof(expected),
                           "%llu",
                           (unsigned long long)probes[i]);
            D_TESTS_OSTREAM_CHECK(strcmp(array, expected) == 0);
        }

        power = (k < 19) ? (power * 10u) : power;
    }

    free(sink.data);

    return true;
}

/*
d_tests_ostream_doubles
  Doubles in the C locale's form, whatever the locale.
  Tests the following:
  - the styles and precisions, nan and the infinities
  - an unknown style is INVALID
  - under de_DE and ps_AF, whose decimal points are ',' and a two-byte
    character, the point is still '.'
*/
static bool
d_tests_ostream_doubles(void)
{
    const char* const locales[] = { "C", "de_DE.UTF-8", "ps_AF.UTF-8" };

    for (size_t i = 0u; i < (sizeof(locales) / sizeof(locales[0])); ++i)
    {
        struct dj_ostream       os;
        struct dj_ostream_fixed fixed;
        char                    array[128];

        if (setlocale(LC_NUMERIC, locales[i]) == NULL)
        {
            (void)d_test_suite_note(d_tests_ostream_suite,
                                    "left out: locale %s not installed",
                                    locales[i]);

            continue;
        }

        (void)d_ostream_init_fixed(&os, &fixed, array, sizeof(array), 1);
        (void)d_ostream_write_double(&os, 1.5, 'g', -1);
        (void)d_ostream_write_char(&os, ' ');
        (void)d_ostream_write_double(&os, -0.125, 'f', 3);
        (void)d_ostream_write_char(&os, ' ');
        (void)d_ostream_write_double(&os, 12345.678, 'e', 2);
        (void)d_ostream_write_char(&os, ' ');
        (void)d_ostream_write_double(&os, (double)NAN, 'g', -1);
        (void)d_ostream_write_char(&os, ' ');
        (void)d_ostream_write_double(&os, -(double)INFINITY, 'g', -1);
        (void)d_ostream_write_char(&os, ' ');
        (void)d_ostream_write_double(&os, 0.1, 'f', 200);
        D_TESTS_OSTREAM_CHECK(d_ostream_close(&os) == 1);
        D_TESTS_OSTREAM_CHECK(strncmp(array,
                                      "1.5 -0.125 1.23e+04 nan -inf 0.1000",
                                      35u) == 0);
        // "0.1" to 64 places: "0." and 64 digits
        D_TESTS_OSTREAM_CHECK(strlen(array) == (29u + 66u));
    }

    (void)setlocale(LC_NUMERIC, "C");

    struct dj_ostream       os;
    struct dj_ostream_fixed fixed;
    char                    array[8];

    (void)d_ostream_init_fixed(&os, &fixed, array, sizeof(array), 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_write_double(&os, 1.0, 'x', 2) == 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_status(&os) == D_OSTREAM_INVALID);

    return true;
}

/*
d_tests_ostream_printf
  printf onto every kind of window.
  Tests the following:
  - formatted in place into a storage window, and into a dj_string
  - through the stack for an unbuffered transport
  - longer than the stack buffer: in place where it can be, on the heap
    where it cannot
  - truncated into a fixed array, the whole length still counted
*/
static bool
d_tests_ostream_printf(void)
{
    struct dj_tests_ostream_sink sink   = d_tests_ostream_sink_new();
    struct dj_string* const      string = d_string_new();
    struct dj_ostream            os;
    char                         long_text[1001];

    memset(long_text, 'z', 1000u);
    long_text[1000] = '\0';

    (void)d_ostream_init_string(&os, string);
    (void)d_ostream_printf(&os, "%s=%d;", "x", 42);
    (void)d_ostream_printf(&os, "%s", long_text);
    D_TESTS_OSTREAM_CHECK(d_ostream_close(&os) == 1);
    D_TESTS_OSTREAM_CHECK(d_string_length(string) == 5u + 1000u);
    D_TESTS_OSTREAM_CHECK(strncmp(d_string_cstr(string), "x=42;zzz", 8u) == 0);

    (void)d_ostream_init(&os, d_tests_ostream_as_pack(&sink), NULL, 0);
    (void)d_ostream_printf(&os, "[%5.2f]", 3.14159);
    (void)d_ostream_printf(&os, "%s", long_text);
    D_TESTS_OSTREAM_CHECK(d_ostream_close(&os) == 1);
    D_TESTS_OSTREAM_CHECK( (sink.size == 7u + 1000u) &&
                           (memcmp(sink.data, "[ 3.14]zz", 9u) == 0) );

    char                    array[16];
    struct dj_ostream_fixed fixed;

    (void)d_ostream_init_fixed(&os, &fixed, array, sizeof(array), 1);
    (void)d_ostream_printf(&os, "%s", long_text);
    D_TESTS_OSTREAM_CHECK(d_ostream_close(&os) == 1);
    D_TESTS_OSTREAM_CHECK( (strlen(array) == 15u) &&
                           (d_ostream_written(&os) == 1000u) &&
                           (d_ostream_fixed_truncated(&fixed) == 1) );
    D_TESTS_OSTREAM_CHECK(d_ostream_printf(&os, "x") == 0);
    d_string_free(string);
    free(sink.data);

    return true;
}

/*
d_tests_ostream_fanout
  Several streams at once.
  Tests the following:
  - every target gets every byte, buffered on the fan-out or not
  - ANY keeps going while one target lives; ALL fails with the first death
  - a dead target never starves the living
  - the stream itself as a target, and an empty fan-out, are refused
*/
static bool
d_tests_ostream_fanout(void)
{
    struct dj_tests_ostream_sink one = d_tests_ostream_sink_new();
    struct dj_tests_ostream_sink two = d_tests_ostream_sink_new();
    struct dj_ostream            first;
    struct dj_ostream            second;
    struct dj_ostream            fan;
    struct dj_ostream*           targets[2] = { &first, &second };
    struct dj_ostream_fanout     fanout = { targets, 2u, D_OSTREAM_FANOUT_ANY,
                                            0 };
    unsigned char               buffer[16];

    two.fail_after = 3u;
    (void)d_ostream_init(&first, d_tests_ostream_as_pack(&one), NULL, 0);
    (void)d_ostream_init(&second, d_tests_ostream_as_pack(&two), NULL, 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_init_fanout(&fan, &fanout) == 1);
    (void)d_ostream_set_buffer(&fan, buffer, sizeof(buffer));
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&fan, "ab") == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_flush(&fan) == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&fan, "cd") == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_close(&fan) == 1);
    D_TESTS_OSTREAM_CHECK(d_tests_ostream_sink_is(&one, "abcd"));
    D_TESTS_OSTREAM_CHECK(d_ostream_status(&second) == D_OSTREAM_FAILED);

    one.size = 0u;
    two.size = 0u;
    (void)d_ostream_init(&first, d_tests_ostream_as_pack(&one), NULL, 0);
    (void)d_ostream_init(&second, d_tests_ostream_as_pack(&two), NULL, 0);
    fanout.policy = D_OSTREAM_FANOUT_ALL;
    (void)d_ostream_init_fanout(&fan, &fanout);
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&fan, "ab") == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_write_cstr(&fan, "cd") == 0);
    D_TESTS_OSTREAM_CHECK(d_tests_ostream_sink_is(&one, "abcd"));

    targets[1] = &fan;
    D_TESTS_OSTREAM_CHECK(d_ostream_init_fanout(&fan, &fanout) == 0);
    fanout.count = 0u;
    D_TESTS_OSTREAM_CHECK(d_ostream_init_fanout(&fan, &fanout) == 0);
    free(one.data);
    free(two.data);

    return true;
}

/*
d_tests_ostream_sink_bridge
  A stream lowered to a sink, for a producer that takes one.
  Tests the following:
  - d_sink_emit through d_ostream_as_sink reaches the stream's destination
  - a refused stream makes the sink's write short
*/
static bool
d_tests_ostream_sink_bridge(void)
{
    struct dj_ostream       os;
    struct dj_ostream_fixed fixed;
    char                    array[16];

    (void)d_ostream_init_fixed(&os, &fixed, array, sizeof(array), 1);

    const struct dj_pack_sink sink = d_ostream_as_sink(&os);

    D_TESTS_OSTREAM_CHECK(d_sink_emit(sink, "hello", 5u) != 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_close(&os) == 1);
    D_TESTS_OSTREAM_CHECK(strcmp(array, "hello") == 0);
    D_TESTS_OSTREAM_CHECK(sink.write(sink.context, "x", 1u) == 0u);

    return true;
}

/*
d_tests_ostream_file
  The FILE* destinations.
  Tests the following:
  - a file opened here is written, closed, and holds the bytes
  - appending keeps what was there
  - a borrowed FILE* is flushed, not closed
  - an open that fails keeps the open's errno
*/
static bool
d_tests_ostream_file(void)
{
    const char* const  path = "t_ostream_file.tmp";
    struct dj_ostream  os;
    unsigned char      buffer[8];
    char               text[64];

    D_TESTS_OSTREAM_CHECK(d_ostream_open_file(&os, path, 0) == 1);
    (void)d_ostream_set_buffer(&os, buffer, sizeof(buffer));
    (void)d_ostream_write_cstr(&os, "first line\n");
    D_TESTS_OSTREAM_CHECK(d_ostream_close(&os) == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_open_file(&os, path, 1) == 1);
    (void)d_ostream_write_cstr(&os, "second\n");
    D_TESTS_OSTREAM_CHECK(d_ostream_close(&os) == 1);

    FILE* const file = fopen(path, "rb");

    D_TESTS_OSTREAM_CHECK(file != NULL);

    if (file != NULL)
    {
        const size_t got = fread(text, 1u, sizeof(text) - 1u, file);

        text[got] = '\0';
        D_TESTS_OSTREAM_CHECK(strcmp(text, "first line\nsecond\n") == 0);
        D_TESTS_OSTREAM_CHECK(d_ostream_init_file(&os, file, 0) == 1);
        D_TESTS_OSTREAM_CHECK(d_ostream_close(&os) == 1);
        D_TESTS_OSTREAM_CHECK(fclose(file) == 0);
    }

    (void)remove(path);
    D_TESTS_OSTREAM_CHECK(d_ostream_open_file(&os,
                                              "no_such_dir/t_ostream.tmp",
                                              0) == 0);
    D_TESTS_OSTREAM_CHECK( (d_ostream_status(&os) == D_OSTREAM_FAILED) &&
                           (d_ostream_last_errno(&os) == ENOENT) );
    D_TESTS_OSTREAM_CHECK(d_ostream_open_file(&os, NULL, 0) == 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_status(&os) == D_OSTREAM_INVALID);

    return true;
}

/*
d_tests_ostream_console
  The console, on POSIX: stdout redirected to a file for the check.
  Tests the following:
  - an unbuffered console stream interleaves with printf in program order
  - a buffered one keeps order once flushed
  - the query of a redirected stdout: not a terminal, binary-safe
*/
static bool
d_tests_ostream_console(void)
{
#if (D_INTERNAL_OSTREAM_CONSOLE ==  D_CFG_OSTREAM_CONSOLE_POSIX)
    const char* const         path  = "t_ostream_console.tmp";
    struct dj_ostream         os;
    struct dj_ostream_console console;
    struct dj_ostream_info    info;
    unsigned char             buffer[64];
    char                      text[64] = "";

    (void)fflush(stdout);

    // stdout's descriptor, pointed at a file; the FILE* stays the same
    const int saved = dup(1);
    const int file  = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);

    D_TESTS_OSTREAM_CHECK( (saved >= 0) &&
                           (file >= 0) &&
                           (dup2(file, 1) == 1) );
    (void)close(file);
    D_TESTS_OSTREAM_CHECK(d_ostream_init_console(&os,
                                                 &console,
                                                 D_OSTREAM_CONSOLE_OUT,
                                                 0) == 1);
    (void)d_ostream_write_cstr(&os, "a");
    printf("b");
    (void)d_ostream_write_cstr(&os, "c");
    (void)d_ostream_set_buffer(&os, buffer, sizeof(buffer));
    (void)d_ostream_write_cstr(&os, "d");
    (void)d_ostream_flush(&os);
    printf("e");
    (void)d_ostream_query(&os, &info);
    (void)d_ostream_close(&os);
    (void)fflush(stdout);
    (void)dup2(saved, 1);
    (void)close(saved);

    FILE* const in = fopen(path, "rb");

    if (in != NULL)
    {
        const size_t got = fread(text, 1u, sizeof(text) - 1u, in);

        text[got] = '\0';
        (void)fclose(in);
    }

    D_TESTS_OSTREAM_CHECK(strcmp(text, "abcde") == 0);
    D_TESTS_OSTREAM_CHECK( (info.is_terminal == 0) &&
                           (info.is_binary_safe == 1) );
    (void)remove(path);
#else
    (void)d_test_suite_skip(d_tests_ostream_suite,
                            "not the POSIX console backend");
#endif

    return true;
}

/*
d_tests_ostream_written64
  The 64-bit count agrees with the size_t one, and the wraps are counted.
  Tests the following:
  - written64 equals written while nothing has wrapped
  - a wrap in delivered is counted once
*/
static bool
d_tests_ostream_written64(void)
{
    struct dj_ostream       os;
    struct dj_ostream_fixed fixed;

    (void)d_ostream_init_fixed(&os, &fixed, NULL, 0u, 0);
    (void)d_ostream_fill(&os, 'x', 1000u);
    D_TESTS_OSTREAM_CHECK(d_ostream_written64(&os) == 1000u);
    (void)d_ostream_flush(&os);

    // a stream that has delivered all but 10 of a size_t's range
    os.delivered = ((size_t)0 - 10u);
    (void)d_ostream_fill(&os, 'x', 300u);
    (void)d_ostream_flush(&os);
    D_TESTS_OSTREAM_CHECK(os.wraps == 1u);
    D_TESTS_OSTREAM_CHECK(d_ostream_written(&os) == 290u);
    D_TESTS_OSTREAM_CHECK( (sizeof(size_t) >= sizeof(uint64_t)) ||
                           (d_ostream_written64(&os) ==
                            (((uint64_t)1 << (sizeof(size_t) * CHAR_BIT)) +
                             290u)) );

    return true;
}


//==============================================================================
// THE DIFFERENTIAL TEST
//==============================================================================

// dj_tests_ostream_random_state
//   struct: one destination under the differential test, and its model.
struct dj_tests_ostream_random_state
{
    struct dj_ostream            os;
    struct dj_tests_ostream_sink sink;
    struct dj_ostream_fixed      fixed;
    struct dj_string*            string;
    unsigned char                buffer[80];
    unsigned char*               array;
    size_t                       capacity;
    int                          kind;
};

static uint64_t d_tests_ostream_rng = 0x9E3779B97F4A7C15ull;

/*
d_tests_ostream_next
  xorshift64*: deterministic, so a failure reproduces.
*/
static uint64_t
d_tests_ostream_next(void)
{
    d_tests_ostream_rng ^= d_tests_ostream_rng >> 12;
    d_tests_ostream_rng ^= d_tests_ostream_rng << 25;
    d_tests_ostream_rng ^= d_tests_ostream_rng >> 27;

    return d_tests_ostream_rng * 0x2545F4914F6CDD1Dull;
}

/*
d_tests_ostream_below
  A random number below _bound.
*/
static size_t
d_tests_ostream_below(
    size_t _bound
)
{
    return (size_t)(d_tests_ostream_next() % (uint64_t)_bound);
}

/*
d_tests_ostream_random_open
  Makes one destination of a kind: 0 unbuffered transport, 1 buffered
transport of a random size, 2 the same line-buffered, 3 a dj_string, 4 a
fixed array of a random size.
*/
static void
d_tests_ostream_random_open(
    struct dj_tests_ostream_random_state* _state,
    int                                   _kind
)
{
    _state->kind     = _kind;
    _state->string   = NULL;
    _state->array    = NULL;
    _state->sink     = d_tests_ostream_sink_new();
    _state->capacity = 1u + d_tests_ostream_below(2000u);

    if (_kind <= 2)
    {
        (void)d_ostream_init(&_state->os,
                             d_tests_ostream_as_pack(&_state->sink),
                             NULL,
                             (_kind == 2) ? D_OSTREAM_LINE_BUFFERED : 0);

        if (_kind != 0)
        {
            (void)d_ostream_set_buffer(&_state->os,
                                       _state->buffer,
                                       1u + d_tests_ostream_below(
                                                sizeof(_state->buffer)));
        }
    }
    else if (_kind == 3)
    {
        _state->string = d_string_new_with_capacity(d_tests_ostream_below(40u));
        (void)d_ostream_init_string(&_state->os, _state->string);
    }
    else
    {
        _state->array = (unsigned char*)malloc(_state->capacity);
        (void)d_ostream_init_fixed(&_state->os,
                                   &_state->fixed,
                                   _state->array,
                                   _state->capacity,
                                   0);
    }

    return;
}

/*
d_tests_ostream_random_step
  One random operation on one stream, appended to the model as well.
*/
static void
d_tests_ostream_random_step(
    struct dj_ostream* _os,
    unsigned char*     _model,
    size_t*            _length,
    int                _operation,
    const char*        _bytes,
    size_t             _size
)
{
    switch (_operation)
    {
        case 0:
        {
            (void)d_ostream_write(_os, _bytes, _size);

            break;
        }
        case 1:
        {
            (void)d_ostream_write_char(_os, _bytes[0]);
            _size = 1u;

            break;
        }
        case 2:
        {
            struct dj_ostream_cursor cursor = d_ostream_cursor_take(_os);

            for (size_t i = 0u; i < _size; ++i)
            {
                cursor = ((i % 3u) == 0u)
                             ? d_ostream_cursor_write(_os,
                                                      cursor,
                                                      _bytes + i,
                                                      1u)
                             : d_ostream_cursor_put(_os, cursor, _bytes[i]);
            }

            (void)d_ostream_cursor_give(_os, cursor);

            break;
        }
        case 3:
        {
            char* const room = d_ostream_reserve(_os, _size + 1u);

            if (room != NULL)
            {
                memcpy(room, _bytes, _size);
                (void)d_ostream_commit(_os, _size);
            }
            else
            {
                (void)d_ostream_write(_os, _bytes, _size);
            }

            break;
        }
        default:
        {
            (void)d_ostream_flush(_os);
            _size = 0u;

            break;
        }
    }

    memcpy(_model + *_length, _bytes, _size);
    *_length += _size;

    return;
}

/*
d_tests_ostream_random_verify
  Closes one destination and holds what it received to the model.
*/
static void
d_tests_ostream_random_verify(
    struct dj_tests_ostream_random_state* _state,
    const unsigned char*                  _model,
    size_t                                _length
)
{
    D_TESTS_OSTREAM_CHECK(d_ostream_close(&_state->os) == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_written(&_state->os) == _length);

    if (_state->kind <= 2)
    {
        D_TESTS_OSTREAM_CHECK( (_state->sink.size == _length) &&
                               (memcmp(_state->sink.data, _model, _length)
                                  == 0) );
    }
    else if (_state->kind == 3)
    {
        D_TESTS_OSTREAM_CHECK( (d_string_length(_state->string) == _length) &&
                               (memcmp(d_string_cstr(_state->string),
                                       _model,
                                       _length) == 0) &&
                               (d_string_cstr(_state->string)[_length]
                                  == '\0') );
        d_string_free(_state->string);
    }
    else
    {
        const size_t kept = (_length < _state->capacity) ? _length
                                                         : _state->capacity;

        D_TESTS_OSTREAM_CHECK( (d_ostream_fixed_length(&_state->fixed)
                                  == kept) &&
                               (memcmp(_state->array, _model, kept) == 0) &&
                               ( (d_ostream_fixed_truncated(&_state->fixed)
                                    != 0) == (_length > _state->capacity) ) );
        free(_state->array);
    }

    free(_state->sink.data);

    return;
}

/*
d_tests_ostream_random
  The differential test: many runs of random operations on all five kinds
of destination at once, each held to a byte array that saw the same bytes.
  Tests the following:
  - every byte arrives, once, in order, whatever the window, the buffer
    size, the growth or the spill
  - written agrees with the model; a fixed array keeps the longest prefix
*/
static bool
d_tests_ostream_random(void)
{
    const size_t   runs    = 400u;
    const size_t   most    = 200000u;
    unsigned char* model   = (unsigned char*)malloc(most);
    char           bytes[300];
    size_t         before  = d_tests_ostream_failures;

    for (size_t run = 0u; run < runs; ++run)
    {
        struct dj_tests_ostream_random_state states[5];
        size_t                               length = 0u;
        const size_t                         steps  =
            1u + d_tests_ostream_below(400u);

        for (int kind = 0; kind < 5; ++kind)
        {
            d_tests_ostream_random_open(&states[kind], kind);
        }

        for (size_t step = 0u; (step < steps) && (length + 300u < most); ++step)
        {
            const int    operation = (int)d_tests_ostream_below(5u);
            const size_t size      = (d_tests_ostream_below(4u) == 0u)
                                         ? d_tests_ostream_below(300u)
                                         : d_tests_ostream_below(12u);

            for (size_t i = 0u; i < size; ++i)
            {
                const int letter = 'a' + (int)d_tests_ostream_below(26u);

                bytes[i] = (char)((d_tests_ostream_below(9u) == 0u) ? '\n'
                                                                     : letter);
            }

            bytes[0] = (size == 0u) ? 'q' : bytes[0];

            for (int kind = 0; kind < 5; ++kind)
            {
                size_t copy = length;

                d_tests_ostream_random_step(&states[kind].os,
                                            model,
                                            &copy,
                                            operation,
                                            bytes,
                                            size);

                if (kind == 4)
                {
                    length = copy;
                }
            }
        }

        for (int kind = 0; kind < 5; ++kind)
        {
            d_tests_ostream_random_verify(&states[kind], model, length);
        }

        // stop at the first failing run, so its output stays readable
        if (d_tests_ostream_failures != before)
        {
            (void)d_test_suite_note(d_tests_ostream_suite,
                                    "first failure in run %lu",
                                    (unsigned long)run);

            break;
        }
    }

    free(model);

    return true;
}


//==============================================================================
// LOCALES PER CALL (stream 2)
//==============================================================================

/*
d_tests_ostream_locale_case
  Writes 1.5, -0.125 and 12345.678 in a locale, through a fixed array, and
whether the text is _expected; notes what came where it is not.
*/
static bool
d_tests_ostream_locale_case(
    const struct dj_ostream_locale* _locale,
    const char*                     _expected
)
{
    struct dj_ostream       os;
    struct dj_ostream_fixed fixed;
    char                    array[128];

    (void)d_ostream_init_fixed(&os, &fixed, array, sizeof(array), 1);
    (void)d_ostream_write_double_l(&os, 1.5, 'g', -1, _locale);
    (void)d_ostream_write_char(&os, ' ');
    (void)d_ostream_write_double_l(&os, -0.125, 'f', 3, _locale);
    (void)d_ostream_write_char(&os, ' ');
    (void)d_ostream_write_double_l(&os, 12345.678, 'e', 2, _locale);
    (void)d_ostream_close(&os);

    if (strcmp(array, _expected) != 0)
    {
        (void)d_test_suite_note(d_tests_ostream_suite,
                                "got [%s], expected [%s]",
                                array,
                                _expected);

        return false;
    }

    return true;
}

/*
d_tests_ostream_locales_named
  The named locales, where the build can make them: de_DE's ',' and ps_AF's
two-byte point, each written while the program's locale stays C; an unknown
name; and the C form under a program locale of de_DE.
*/
static void
d_tests_ostream_locales_named(
    const struct dj_ostream_locale* _c_form
)
{
    static const char* const names[]    = { "de_DE.UTF-8", "ps_AF.UTF-8" };
    static const char* const expected[] =
    {
        "1,5 -0,125 1,23e+04",
        "1\xD9\xAB" "5 -0\xD9\xAB" "125 1\xD9\xAB" "23e+04"
    };
    struct dj_ostream_locale locale;
    char                     text[32];

    for (size_t i = 0u; i < (sizeof(names) / sizeof(names[0])); ++i)
    {
        errno = 0;

        if (d_ostream_locale_init(&locale, names[i]) == 0)
        {
            D_TESTS_OSTREAM_CHECK(errno == ENOENT);
            (void)d_test_suite_note(d_tests_ostream_suite,
                                    "left out: locale %s not installed",
                                    names[i]);

            continue;
        }

        D_TESTS_OSTREAM_CHECK( (locale.handle != NULL) &&
                               (locale.is_c == 0) );
        D_TESTS_OSTREAM_CHECK(d_tests_ostream_locale_case(&locale,
                                                          expected[i]));
        D_TESTS_OSTREAM_CHECK(strcmp(setlocale(LC_NUMERIC, NULL), "C") == 0);
        d_ostream_locale_close(&locale);
        d_ostream_locale_close(&locale);
        D_TESTS_OSTREAM_CHECK( (locale.handle == NULL) &&
                               (locale.is_c == 0) );
    }

    errno = 0;
    D_TESTS_OSTREAM_CHECK(d_ostream_locale_init(&locale, "xx_YY.nope") == 0);
    D_TESTS_OSTREAM_CHECK(errno == ENOENT);

    // the program in de_DE: the C form is still '.', and the call leaves the
    // program's locale, and this thread's, as they were
    if ( (setlocale(LC_NUMERIC, "de_DE.UTF-8") == NULL) ||
         (d_ostream_locale_init(&locale, "de_DE.UTF-8") == 0) )
    {
        (void)setlocale(LC_NUMERIC, "C");

        return;
    }

    D_TESTS_OSTREAM_CHECK(d_tests_ostream_locale_case(NULL,
                                                      "1.5 -0.125 1.23e+04"));
    D_TESTS_OSTREAM_CHECK(d_tests_ostream_locale_case(_c_form,
                                                      "1.5 -0.125 1.23e+04"));
    D_TESTS_OSTREAM_CHECK(d_tests_ostream_locale_case(&locale,
                                                      "1,5 -0,125 1,23e+04"));
    (void)snprintf(text, sizeof(text), "%g", 1.5);
    D_TESTS_OSTREAM_CHECK(strcmp(text, "1,5") == 0);
    D_TESTS_OSTREAM_CHECK(strcmp(setlocale(LC_NUMERIC, NULL),
                                 "de_DE.UTF-8") == 0);

    d_ostream_locale_close(&locale);
    (void)setlocale(LC_NUMERIC, "C");

    return;
}

/*
d_tests_ostream_locales
  Doubles in a locale the caller names, for one call.
  Tests the following:
  - "C" and "POSIX" are the C form under every mechanism, as is no locale
  - a NULL locale or name is EINVAL, and leaves no locale; a write with no
    locale (a zeroed one) is refused as INVALID
  - with locale objects: de_DE's ',' and ps_AF's two-byte point while the
    program stays in C; an unknown name is ENOENT; with the program in de_DE,
    the C form is still '.' and both locales are left as they were
  - without them: a named locale is ENOTSUP
*/
static bool
d_tests_ostream_locales(void)
{
    struct dj_ostream        os;
    struct dj_ostream_fixed  fixed;
    struct dj_ostream_locale c_form;
    struct dj_ostream_locale other;
    char                     array[32];

    D_TESTS_OSTREAM_CHECK(d_ostream_locale_init(&c_form, "C") == 1);
    D_TESTS_OSTREAM_CHECK(c_form.is_c == 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_locale_init(&other, "POSIX") == 1);
    D_TESTS_OSTREAM_CHECK(other.is_c == 1);
    D_TESTS_OSTREAM_CHECK(d_tests_ostream_locale_case(NULL,
                                                      "1.5 -0.125 1.23e+04"));
    D_TESTS_OSTREAM_CHECK(d_tests_ostream_locale_case(&other,
                                                      "1.5 -0.125 1.23e+04"));
    d_ostream_locale_close(&other);

    errno = 0;
    D_TESTS_OSTREAM_CHECK( (d_ostream_locale_init(NULL, "C") == 0) &&
                           (errno == EINVAL) );
    errno = 0;
    D_TESTS_OSTREAM_CHECK( (d_ostream_locale_init(&other, NULL) == 0) &&
                           (errno == EINVAL) );
    D_TESTS_OSTREAM_CHECK( (other.handle == NULL) &&
                           (other.is_c == 0) );

    // no locale is refused, and the stream records it
    (void)d_ostream_init_fixed(&os, &fixed, array, sizeof(array), 1);
    D_TESTS_OSTREAM_CHECK(d_ostream_write_double_l(&os, 1.5, 'g', -1,
                                                   &other) == 0);
    D_TESTS_OSTREAM_CHECK(d_ostream_status(&os) == D_OSTREAM_INVALID);
    (void)d_ostream_close(&os);
    d_ostream_locale_close(NULL);

    if (d_ostream_locale_named() != 0)
    {
        d_tests_ostream_locales_named(&c_form);
    }
    else
    {
        errno = 0;
        D_TESTS_OSTREAM_CHECK(d_ostream_locale_init(&other,
                                                    "de_DE.UTF-8") == 0);
        D_TESTS_OSTREAM_CHECK(errno != 0);
        (void)d_test_suite_note(d_tests_ostream_suite,
                                "no locale objects in this build: the C "
                                "form only");
    }

    d_ostream_locale_close(&c_form);

    return true;
}

#if (D_TESTS_OSTREAM_THREADS == 1)

// D_TESTS_OSTREAM_RACE
//   constant: the doubles written, and the locale switches made, at once.
#define D_TESTS_OSTREAM_RACE  20000

/*
d_tests_ostream_race_switch
  The other thread: switches LC_NUMERIC between C and de_DE.UTF-8 again and
again.
*/
static void*
d_tests_ostream_race_switch(
    void* _unused
)
{
    (void)_unused;

    for (int i = 0; i < D_TESTS_OSTREAM_RACE; ++i)
    {
        (void)setlocale(LC_NUMERIC, ((i & 1) != 0) ? "de_DE.UTF-8" : "C");
    }

    return NULL;
}

#endif  // D_TESTS_OSTREAM_THREADS

/*
d_tests_ostream_locale_race
  The C form while another thread changes the program's locale.
  Tests the following:
  - with locale objects, every one of 20000 doubles written while another
    thread switches LC_NUMERIC between C and de_DE.UTF-8 has '.'; nothing
    is checked until both threads are done, so the suite's own formatting
    never meets the switching
*/
static bool
d_tests_ostream_locale_race(void)
{
#if (D_TESTS_OSTREAM_THREADS == 1)
    struct dj_ostream_locale probe;
    pthread_t                other;
    size_t                   wrong = 0u;
    int                      started;

    if ( (d_ostream_locale_named() == 0) ||
         (d_ostream_locale_init(&probe, "de_DE.UTF-8") == 0) )
    {
        (void)d_test_suite_skip(d_tests_ostream_suite,
                                "needs locale objects and de_DE.UTF-8");

        return true;
    }

    d_ostream_locale_close(&probe);

    started = pthread_create(&other, NULL, d_tests_ostream_race_switch, NULL);
    D_TESTS_OSTREAM_CHECK(started == 0);

    if (started != 0)
    {
        return false;
    }

    for (int i = 0; i < D_TESTS_OSTREAM_RACE; ++i)
    {
        struct dj_ostream       os;
        struct dj_ostream_fixed fixed;
        char                    array[16];

        (void)d_ostream_init_fixed(&os, &fixed, array, sizeof(array), 1);
        (void)d_ostream_write_double(&os, 1.5, 'g', -1);
        (void)d_ostream_close(&os);

        if (memcmp(array, "1.5", 4u) != 0)
        {
            ++wrong;
        }
    }

    (void)pthread_join(other, NULL);
    (void)setlocale(LC_NUMERIC, "C");

    D_TESTS_OSTREAM_CHECK(wrong == 0u);

    if (wrong != 0u)
    {
        (void)d_test_suite_note(d_tests_ostream_suite,
                                "%lu of %d had another point",
                                (unsigned long)wrong,
                                D_TESTS_OSTREAM_RACE);
    }
#else
    (void)d_test_suite_skip(d_tests_ostream_suite,
                            "needs POSIX threads");
#endif

    return true;
}


//==============================================================================
// THE SUITE
//==============================================================================

// dj_tests_ostream_group
//   struct: one group of checks: the header it tests (the block), its name
// and descriptor (the test), and the function that makes its checks.
struct dj_tests_ostream_group
{
    const char* block;
    const char* name;
    const char* descriptor;
    bool        (*run)(void);
};

/*
d_tests_ostream_all
  Opens the module, a block per header and a test per group, and runs each
group against its test.
*/
void
d_tests_ostream_all(
    struct dj_test_suite* _suite
)
{
    static const struct dj_tests_ostream_group groups[] =
    {
        { "ostream.h", "init",
          "the contract of d_ostream_init and its refusals",
          &d_tests_ostream_init },
        { "ostream.h", "transport",
          "a transport's buffering and delivery",
          &d_tests_ostream_transport },
        { "ostream.h", "failure",
          "the sticky status",
          &d_tests_ostream_failure },
        { "ostream.h", "line buffering",
          "a line-buffered transport",
          &d_tests_ostream_line_buffering },
        { "ostream.h", "fixed",
          "the fixed destination's snprintf contract",
          &d_tests_ostream_fixed },
        { "ostream.h", "dj_string",
          "the dj_string destination, written in place",
          &d_tests_ostream_string },
        { "ostream.h", "numbers",
          "the integer writers, in place and through a local array",
          &d_tests_ostream_numbers },
        { "ostream.h", "fan-out",
          "several streams at once",
          &d_tests_ostream_fanout },
        { "ostream.h", "sink bridge",
          "a stream lowered to a sink, for a producer that takes one",
          &d_tests_ostream_sink_bridge },
        { "ostream.h", "written64",
          "the 64-bit count agrees with the size_t one, and wraps count",
          &d_tests_ostream_written64 },
        { "ostream_file.h", "file",
          "the FILE* destinations",
          &d_tests_ostream_file },
        { "ostream_console.h", "console",
          "the console, on POSIX, with stdout redirected to a file",
          &d_tests_ostream_console },
        { "ostream_format.h", "doubles",
          "doubles in the C locale's form, whatever the locale",
          &d_tests_ostream_doubles },
        { "ostream_format.h", "printf",
          "printf onto every kind of window",
          &d_tests_ostream_printf },
        { "ostream_format.h", "locales",
          "doubles in a locale the caller names, for one call",
          &d_tests_ostream_locales },
        { "ostream_format.h", "locale race",
          "the C form while another thread changes the program's locale",
          &d_tests_ostream_locale_race },
        { "differential", "random operations",
          "every destination at once, held to a byte array",
          &d_tests_ostream_random }
    };

    const char* block = NULL;

    d_tests_ostream_suite    = _suite;
    d_tests_ostream_failures = 0u;

    (void)d_test_suite_module(_suite, "ostream");
    (void)d_test_suite_describe(_suite,
                                "the output stream: its kernel, its "
                                "destinations and its formatting");

    for (size_t i = 0u; i < (sizeof(groups) / sizeof(groups[0])); ++i)
    {
        if ( (block == NULL) ||
             (strcmp(block, groups[i].block) != 0) )
        {
            block = groups[i].block;
            (void)d_test_suite_block(_suite, block);
        }

        (void)d_test_suite_test(_suite, groups[i].name);
        (void)d_test_suite_describe(_suite, groups[i].descriptor);
        (void)groups[i].run();
    }

    d_tests_ostream_suite = NULL;

    return;
}
