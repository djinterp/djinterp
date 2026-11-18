/*******************************************************************************
* djinterp [test]                                    ostream_format_null_tests.c
*
*   printf's heap fallback (c/text/ostream_format.h) on a build whose default
* memory source is the null source, in DTest's C face
* (c/modules/test/test_suite.h). The leaf compiles the memory source with
* D_CFG_MEM_SOURCE_DEFAULT_NULL, and the runtime's ostream_format.c resolves
* its source through it, so every request for the heap is refused.
*
*   WHAT THIS PINS. Output that needs the heap fails as any failed allocation
* does -- D_OSTREAM_FAILED with ENOMEM -- and none of it reaches the
* destination; output that fits the stack buffer, or the stream's window, is
* written as before, because it never asks.
*
*
* path:      /tests/djinterp/c/text/ostream_format_null_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/

#include "ostream_format_null_tests.h"  // the suite's entry point
// std
#include <errno.h>    // ENOMEM
#include <stdbool.h>  // bool
#include <stddef.h>   // NULL, size_t
#include <string.h>   // memcmp, memset
// djinterp
#include "../../../../inc/djinterp/c/memory/mem_source.h"     // the source
#include "../../../../inc/djinterp/c/text/ostream_format.h"   // printf
#include "../../../../inc/djinterp/c/util/ostream/ostream.h"  // streams
#include "../../../../inc/djinterp/c/util/sink_common.h"      // buffer sink


#if (D_INTERNAL_MEM_SOURCE_DEFAULT != D_CFG_MEM_SOURCE_DEFAULT_NULL)
    #error "this suite is built with D_CFG_MEM_SOURCE_DEFAULT_NULL"
#endif

// D_TESTS_FORMAT_NULL_CHECK
//   macro: one check: counts it, and prints the expression and line on
// failure.
#define D_TESTS_FORMAT_NULL_CHECK(expr)                                        \
    d_tests_format_null_check(((expr) ? 1 : 0), #expr, __LINE__)

// D_TESTS_FORMAT_NULL_LONG
//   constant: an output longer than printf's 256-byte stack buffer.
#define D_TESTS_FORMAT_NULL_LONG      1000u

// D_TESTS_FORMAT_NULL_CAPACITY
//   constant: the destination's capacity, and the window's.
#define D_TESTS_FORMAT_NULL_CAPACITY  2048u

// d_tests_format_null_suite
//   the suite being built, for the checks; set by d_tests_format_null_all.
static struct dj_test_suite* d_tests_format_null_suite;


/*
d_tests_format_null_check
  Records one check against the open test.
*/
static bool
d_tests_format_null_check(
    int         _ok,
    const char* _expression,
    int         _line
)
{
    return d_test_suite_check(d_tests_format_null_suite,
                              (_ok != 0),
                              _expression,
                              __FILE__,
                              _line);
}

/*
d_tests_format_null_open
  Opens a group's test, with its descriptor.
*/
static void
d_tests_format_null_open(
    const char* _name,
    const char* _descriptor
)
{
    (void)d_test_suite_test(d_tests_format_null_suite, _name);
    (void)d_test_suite_describe(d_tests_format_null_suite, _descriptor);

    return;
}

/*
d_tests_format_null_fill
  Fills a text of D_TESTS_FORMAT_NULL_LONG bytes and its terminator.
*/
static void
d_tests_format_null_fill(
    char* _text
)
{
    memset(_text, 'z', D_TESTS_FORMAT_NULL_LONG);
    _text[D_TESTS_FORMAT_NULL_LONG] = '\0';

    return;
}

/*
d_tests_format_null_source
  The build's default memory source.
  Tests the following:
  - an unset source resolves to a valid source
  - which refuses a request
*/
static void
d_tests_format_null_source(void)
{
    const struct dj_mem_source unset = { NULL, NULL };
    struct dj_mem_source       source;

    d_tests_format_null_open("the default source refuses",
                             "an unset source resolves to the null source");

    source = d_mem_source_resolve(&unset);

    D_TESTS_FORMAT_NULL_CHECK(d_mem_source_is_valid(&source));
    D_TESTS_FORMAT_NULL_CHECK(d_mem_source_allocate(&source, 16u, 0u) == NULL);

    return;
}

/*
d_tests_format_null_long
  printf's output past the window and the stack buffer, with no heap.
  Tests the following:
  - the call fails as a failed allocation does: D_OSTREAM_FAILED, ENOMEM
  - none of the output reaches the destination
  - the failure stays, so a later write is refused
*/
static void
d_tests_format_null_long(void)
{
    unsigned char              bytes[D_TESTS_FORMAT_NULL_CAPACITY];
    char                       text[D_TESTS_FORMAT_NULL_LONG + 1u];
    struct dj_pack_buffer_sink buffer;
    struct dj_ostream          os;

    d_tests_format_null_open("long output fails without a heap",
                             "an unbuffered transport, an output past the "
                             "stack buffer, and a source that refuses");

    d_tests_format_null_fill(text);
    d_pack_buffer_sink_init(&buffer, bytes, sizeof(bytes));
    (void)d_ostream_init(&os, d_pack_sink_from_buffer(&buffer), NULL, 0);

    D_TESTS_FORMAT_NULL_CHECK(d_ostream_printf(&os, "%s", text) == 0);
    D_TESTS_FORMAT_NULL_CHECK(d_ostream_status(&os) == D_OSTREAM_FAILED);
    D_TESTS_FORMAT_NULL_CHECK(d_ostream_last_errno(&os) == ENOMEM);
    D_TESTS_FORMAT_NULL_CHECK(buffer.written == 0u);
    D_TESTS_FORMAT_NULL_CHECK(d_ostream_printf(&os, "%d", 1) == 0);
    D_TESTS_FORMAT_NULL_CHECK(buffer.written == 0u);

    (void)d_ostream_close(&os);

    return;
}

/*
d_tests_format_null_no_heap
  printf's output that never asks for the heap.
  Tests the following:
  - output that fits the stack buffer is written through an unbuffered
    transport
  - output past the stack buffer is formatted in place where the window has
    room for it
*/
static void
d_tests_format_null_no_heap(void)
{
    unsigned char              bytes[D_TESTS_FORMAT_NULL_CAPACITY];
    unsigned char              window[D_TESTS_FORMAT_NULL_CAPACITY];
    char                       text[D_TESTS_FORMAT_NULL_LONG + 1u];
    struct dj_pack_buffer_sink buffer;
    struct dj_ostream          os;

    d_tests_format_null_open("output that needs no heap is written",
                             "the stack buffer and the window, which never "
                             "ask the source");

    d_pack_buffer_sink_init(&buffer, bytes, sizeof(bytes));
    (void)d_ostream_init(&os, d_pack_sink_from_buffer(&buffer), NULL, 0);

    D_TESTS_FORMAT_NULL_CHECK(d_ostream_printf(&os, "%s=%d;", "x", 42) == 1);
    D_TESTS_FORMAT_NULL_CHECK(d_ostream_close(&os) == 1);
    D_TESTS_FORMAT_NULL_CHECK( (buffer.written == 5u) &&
                               (memcmp(bytes, "x=42;", 5u) == 0) );

    d_tests_format_null_fill(text);
    d_pack_buffer_sink_init(&buffer, bytes, sizeof(bytes));
    (void)d_ostream_init(&os, d_pack_sink_from_buffer(&buffer), NULL, 0);
    (void)d_ostream_set_buffer(&os, window, sizeof(window));

    D_TESTS_FORMAT_NULL_CHECK(d_ostream_printf(&os, "%s", text) == 1);
    D_TESTS_FORMAT_NULL_CHECK(d_ostream_close(&os) == 1);
    D_TESTS_FORMAT_NULL_CHECK( (buffer.written == D_TESTS_FORMAT_NULL_LONG) &&
                               (bytes[0] == 'z') &&
                               (bytes[D_TESTS_FORMAT_NULL_LONG - 1u] == 'z') );

    return;
}

/*
d_tests_format_null_all
  Opens the module and its block, then runs every group, each as a test.
*/
void
d_tests_format_null_all(
    struct dj_test_suite* _suite
)
{
    d_tests_format_null_suite = _suite;

    (void)d_test_suite_module(_suite, "ostream_format_null");
    (void)d_test_suite_describe(_suite,
                                "printf's heap fallback when the default "
                                "memory source is the null source");
    (void)d_test_suite_block(_suite, "ostream_format.h");

    d_tests_format_null_source();
    d_tests_format_null_long();
    d_tests_format_null_no_heap();

    d_tests_format_null_suite = NULL;

    return;
}
