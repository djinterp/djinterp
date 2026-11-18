/*******************************************************************************
* djinterp [test]                                ostream_template_refuse_tests.c
*
*   The template engine (c/text/ostream_template.h) built with the refusing
* UTF-8 knob, D_CFG_OSTREAM_TEMPLATE_UTF8_REFUSE, in DTest's C face
* (c/modules/test/test_suite.h). The leaf compiles the engine with the knob.
*
*   WHAT THIS PINS. Text that isn't valid UTF-8, written for JSON or XML, is
* refused whole: nothing of it reaches the stream, not even its quote or its
* padding, and the stream fails with D_OSTREAM_INVALID and EILSEQ. Valid text
* is written as under the default, and raw text and CSV pass every byte.
*
*
* path:      /tests/djinterp/c/text/ostream_template_refuse_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/

#include "ostream_template_refuse_tests.h"  // the suite's entry point
// std
#include <errno.h>    // EILSEQ
#include <stdbool.h>  // bool
#include <stddef.h>   // NULL, size_t
#include <string.h>   // strcmp, strlen
// djinterp
#include "../../../../inc/djinterp/c/text/ostream_template.h"  // the engine
#include "../../../../inc/djinterp/c/util/ostream/ostream.h"   // streams
#include "../../../../inc/djinterp/config/c/text/cfg_ostream_template.h"
                                                              // the knob
// re_std
#include "../../../../inc/re_std/c/cstdint/dstdint.h"  // int32_t


#if (D_CFG_NORM(D_CFG_OSTREAM_TEMPLATE_UTF8) !=                                \
     D_CFG_OSTREAM_TEMPLATE_UTF8_REFUSE)
    #error "this suite is built with D_CFG_OSTREAM_TEMPLATE_UTF8_REFUSE"
#endif

// D_TESTS_TEMPLATE_REFUSE_CHECK
//   macro: one check: counts it, and prints the expression and line on
// failure.
#define D_TESTS_TEMPLATE_REFUSE_CHECK(expr)                                    \
    d_tests_template_refuse_check(((expr) ? 1 : 0), #expr, __LINE__)

// d_tests_template_refuse_suite
//   the suite being built, for the checks; set by
// d_tests_template_refuse_all.
static struct dj_test_suite* d_tests_template_refuse_suite;


/*
d_tests_template_refuse_check
  Records one check against the open test.
*/
static bool
d_tests_template_refuse_check(
    int         _ok,
    const char* _expression,
    int         _line
)
{
    return d_test_suite_check(d_tests_template_refuse_suite,
                              (_ok != 0),
                              _expression,
                              __FILE__,
                              _line);
}

/*
d_tests_template_refuse_open
  Opens a group's test, with its descriptor.
*/
static void
d_tests_template_refuse_open(
    const char* _name,
    const char* _descriptor
)
{
    (void)d_test_suite_test(d_tests_template_refuse_suite, _name);
    (void)d_test_suite_describe(d_tests_template_refuse_suite, _descriptor);

    return;
}

/*
d_tests_template_refuse_write
  Writes "[" into a fixed stream and flushes it, so the array holds "[" and
its terminator; then writes one text value under a specification, and gives
the stream's status, its errno and the bytes it took. A value that is
refused leaves the array as the flush did, and the count at 1: nothing of
it, not even padding, reached the window.
*/
static void
d_tests_template_refuse_write(
    const char* _text,
    const char* _spec,
    char*       _out,
    size_t      _capacity,
    int32_t*    _status,
    int32_t*    _errno,
    size_t*     _written
)
{
    struct dj_ostream             os;
    struct dj_ostream_fixed       fixed;
    struct dj_ostream_spec        spec;
    const struct dj_ostream_value value = d_ostream_value_text(_text);

    (void)d_ostream_spec_parse(&spec, _spec, strlen(_spec));
    (void)d_ostream_init_fixed(&os, &fixed, _out, _capacity, 1);
    (void)d_ostream_write(&os, "[", 1u);
    (void)d_ostream_flush(&os);
    (void)d_ostream_write_value(&os, &value, &spec);

    *_status = d_ostream_status(&os);
    *_errno  = d_ostream_last_errno(&os);
    (void)d_ostream_close(&os);
    *_written = d_ostream_written(&os);

    return;
}

/*
d_tests_template_refuse_refused
  Tests the following:
  - invalid UTF-8 for JSON is refused: the array holds only what came
    before, the stream took no byte of it, and it fails with
    D_OSTREAM_INVALID and EILSEQ
  - with a width, no padding is written either
  - XML refuses the same way, a truncated sequence included
*/
static void
d_tests_template_refuse_refused(void)
{
    char    out[64];
    int32_t status  = 0;
    int32_t error   = 0;
    size_t  written = 0u;

    d_tests_template_refuse_open("refused",
                                 "invalid UTF-8 for JSON or XML writes "
                                 "nothing and fails the stream");

    d_tests_template_refuse_write("x\xFF\xC3y", "!json", out, sizeof(out),
                                  &status, &error, &written);
    D_TESTS_TEMPLATE_REFUSE_CHECK((strcmp(out, "[") == 0) &&
                                  (written == 1u));
    D_TESTS_TEMPLATE_REFUSE_CHECK((status == D_OSTREAM_INVALID) &&
                                  (error == EILSEQ));

    d_tests_template_refuse_write("\xC0\xAF", "!json:>8", out, sizeof(out),
                                  &status, &error, &written);
    D_TESTS_TEMPLATE_REFUSE_CHECK((strcmp(out, "[") == 0) &&
                                  (written == 1u) &&
                                  (status == D_OSTREAM_INVALID));

    d_tests_template_refuse_write("a<b\xE2\x82", "!xml", out, sizeof(out),
                                  &status, &error, &written);
    D_TESTS_TEMPLATE_REFUSE_CHECK((strcmp(out, "[") == 0) &&
                                  (written == 1u) &&
                                  (status == D_OSTREAM_INVALID) &&
                                  (error == EILSEQ));

    return;
}

/*
d_tests_template_refuse_kept
  Tests the following:
  - valid UTF-8 for JSON and XML is written as under the default
  - raw text and CSV pass invalid bytes through, and the stream stays well
*/
static void
d_tests_template_refuse_kept(void)
{
    char    out[64];
    int32_t status  = 0;
    int32_t error   = 0;
    size_t  written = 0u;

    d_tests_template_refuse_open("kept",
                                 "valid text, and the conversions that carry "
                                 "any byte, are written");

    d_tests_template_refuse_write("caf\xC3\xA9 \xF0\x9F\x98\x80", "!json",
                                  out, sizeof(out), &status, &error,
                                  &written);
    D_TESTS_TEMPLATE_REFUSE_CHECK(
        strcmp(out, "[\"caf\xC3\xA9 \xF0\x9F\x98\x80\"") == 0);
    D_TESTS_TEMPLATE_REFUSE_CHECK((status == D_OSTREAM_OK) &&
                                  (written == strlen(out)));

    d_tests_template_refuse_write("a&\xC3\xA9", "!xml", out, sizeof(out),
                                  &status, &error, &written);
    D_TESTS_TEMPLATE_REFUSE_CHECK((strcmp(out, "[a&amp;\xC3\xA9") == 0) &&
                                  (status == D_OSTREAM_OK));

    d_tests_template_refuse_write("x\xFFy", "!raw", out, sizeof(out),
                                  &status, &error, &written);
    D_TESTS_TEMPLATE_REFUSE_CHECK((strcmp(out, "[x\xFFy") == 0) &&
                                  (status == D_OSTREAM_OK));

    d_tests_template_refuse_write("x\xFFy", "!csv", out, sizeof(out),
                                  &status, &error, &written);
    D_TESTS_TEMPLATE_REFUSE_CHECK((strcmp(out, "[x\xFFy") == 0) &&
                                  (status == D_OSTREAM_OK));

    return;
}

/*
d_tests_template_refuse_all
  Opens the module and its block, then runs both groups, each as a test.
*/
void
d_tests_template_refuse_all(
    struct dj_test_suite* _suite
)
{
    d_tests_template_refuse_suite = _suite;

    (void)d_test_suite_module(_suite, "ostream_template_refuse");
    (void)d_test_suite_describe(_suite,
                                "the template engine with the refusing UTF-8 "
                                "knob");
    (void)d_test_suite_block(_suite, "ostream_template.h");

    d_tests_template_refuse_refused();
    d_tests_template_refuse_kept();

    d_tests_template_refuse_suite = NULL;

    return;
}
