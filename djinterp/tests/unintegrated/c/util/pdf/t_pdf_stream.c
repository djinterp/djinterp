/*******************************************************************************
* djinterp [test]                                                 t_pdf_stream.c
*
*   The PDF writer on the output stream, from C: the decimal-point ruling and
* save_to (stream 1). d_pdf_num and d_pdf_set_color write numbers through
* d_ostream_write_double, so they give "%g"'s bytes in the C locale whatever
* the program's locale: checked against snprintf under the C locale, at
* random, and then under de_DE.UTF-8 and ps_AF.UTF-8, whose decimal points
* are ',' and a two-byte one, where "%g" would have written them. Both keep
* snprintf's contract: a NULL buffer measures, a short one truncates and
* terminates. The facade's d_pdf_document_save_to dispatches the appended
* slot, and refuses a table from before it.
*   The locale checks skip where those locales are not installed
* (localedef -i de_DE -f UTF-8 de_DE.UTF-8). A standalone program, as the
* other tests not yet in DTest are (tests/unintegrated/README.md): a line per
* group and per failed check, then "N checks, M failed", and exit status 0
* only when nothing failed.
*
*
* path:      /tests/unintegrated/c/util/pdf/t_pdf_stream.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.05
*                                                            revised: 2026.11.05
*******************************************************************************/
// std
#include <locale.h>  // setlocale, LC_ALL
#include <stddef.h>  // size_t, offsetof
#include <stdio.h>   // printf, snprintf
#include <stdlib.h>  // rand, srand
#include <string.h>  // strcmp, strlen, memset
// djinterp
#include "../../../../../inc/djinterp/c/util/pdf/pdf_builtin_backend.h"
                                            // d_pdf_num, d_pdf_set_color
#include "../../../../../inc/djinterp/c/util/pdf/pdf_document.h"
                                            // dj_pdf_backend, the facade
#include "../../../../../inc/djinterp/c/util/ostream/ostream.h"
                                            // dj_ostream, the fixed array
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // int32_t


// D_TESTS_PDF_CHECK
//   macro: one check: counts it, and prints the expression and line on
// failure.
#define D_TESTS_PDF_CHECK(expr)                                                \
    d_tests_pdf_check(((expr) ? 1 : 0), #expr, __LINE__)

// the counts every check adds to
static unsigned long d_tests_pdf_checks = 0;
static unsigned long d_tests_pdf_failed = 0;

// what the toy backend's save_to writes, and how it answers
struct dj_tests_pdf_toy
{
    int32_t calls;
    int32_t fail;
};


/*
d_tests_pdf_check
  Counts one check, and prints it where it failed.
*/
static void
d_tests_pdf_check(
    int         _passed,
    const char* _expression,
    int         _line
)
{
    ++d_tests_pdf_checks;

    if (!_passed)
    {
        ++d_tests_pdf_failed;
        printf("    FAILED line %d: %s\n", _line, _expression);
    }

    return;
}

/*
d_tests_pdf_num_is
  Whether d_pdf_num writes _expected, and measures it with a NULL buffer.
*/
static int
d_tests_pdf_num_is(
    double      _value,
    const char* _expected
)
{
    char buffer[D_PDF_NUM_MAX + 1];

    memset(buffer, 'x', sizeof(buffer));

    const size_t written  = d_pdf_num(_value, buffer, sizeof(buffer));
    const size_t measured = d_pdf_num(_value, NULL, 0u);

    return ( (strcmp(buffer, _expected) == 0) &&
             (written == strlen(_expected))   &&
             (measured == written) );
}

/*
d_tests_pdf_color_is
  Whether d_pdf_set_color writes _expected for a colour, filled or not.
*/
static int
d_tests_pdf_color_is(
    struct dj_pdf_color _color,
    int32_t             _fill,
    const char*         _expected
)
{
    char buffer[128];

    const size_t written  = d_pdf_set_color(&_color,
                                            _fill,
                                            buffer,
                                            sizeof(buffer));
    const size_t measured = d_pdf_set_color(&_color, _fill, NULL, 0u);

    return ( (strcmp(buffer, _expected) == 0) &&
             (written == strlen(_expected))   &&
             (measured == written) );
}

/*
d_tests_pdf_numbers
  Tests the following:
  - integers, fractions, signs, exponents and the specials as "%g" writes them
  - 20,000 random values, byte for byte with snprintf("%g") in the C locale
  - a NULL buffer measures; a short buffer truncates, terminates and still
    returns the whole length
*/
static void
d_tests_pdf_numbers(void)
{
    char ours[D_PDF_NUM_MAX + 1];
    char theirs[64];
    int  same = 1;

    printf("  numbers\n");

    D_TESTS_PDF_CHECK(d_tests_pdf_num_is(0.0, "0"));
    D_TESTS_PDF_CHECK(d_tests_pdf_num_is(1.5, "1.5"));
    D_TESTS_PDF_CHECK(d_tests_pdf_num_is(-72.25, "-72.25"));
    D_TESTS_PDF_CHECK(d_tests_pdf_num_is(612.0, "612"));
    D_TESTS_PDF_CHECK(d_tests_pdf_num_is(0.000125, "0.000125"));
    D_TESTS_PDF_CHECK(d_tests_pdf_num_is(1234567.0, "1.23457e+06"));
    D_TESTS_PDF_CHECK(d_tests_pdf_num_is(1.0e-7, "1e-07"));

    srand(20261105u);

    for (int i = 0; i < 20000; ++i)
    {
        const double value = ((double)rand() - (RAND_MAX / 2)) /
                             (double)((rand() % 1000) + 1);

        (void)snprintf(theirs, sizeof(theirs), "%g", value);
        (void)d_pdf_num(value, ours, sizeof(ours));

        if (strcmp(ours, theirs) != 0)
        {
            printf("    %s against %s\n", ours, theirs);
            same = 0;

            break;
        }
    }

    D_TESTS_PDF_CHECK(same);

    // snprintf's contract
    D_TESTS_PDF_CHECK(d_pdf_num(-72.25, NULL, 0u) == 6u);
    memset(ours, 'x', sizeof(ours));
    D_TESTS_PDF_CHECK(d_pdf_num(-72.25, ours, 4u) == 6u);
    D_TESTS_PDF_CHECK(strcmp(ours, "-72") == 0);
    D_TESTS_PDF_CHECK(d_pdf_num(-72.25, ours, 1u) == 6u);
    D_TESTS_PDF_CHECK(ours[0] == '\0');

    return;
}

/*
d_tests_pdf_colors
  Tests the following:
  - gray, cmyk and rgb, filled and stroked, as the C++ operators write them
  - no colour writes nothing, and terminates
*/
static void
d_tests_pdf_colors(void)
{
    char buffer[8];

    printf("  colours\n");

    D_TESTS_PDF_CHECK(d_tests_pdf_color_is(d_pdf_color_gray(0.5),
                                           1,
                                           "0.5 g\n"));
    D_TESTS_PDF_CHECK(d_tests_pdf_color_is(d_pdf_color_gray(0.25),
                                           0,
                                           "0.25 G\n"));
    D_TESTS_PDF_CHECK(d_tests_pdf_color_is(d_pdf_color_cmyk(0.1, 0.2, 0.3, 0.4),
                                           1,
                                           "0.1 0.2 0.3 0.4 k\n"));
    D_TESTS_PDF_CHECK(d_tests_pdf_color_is(d_pdf_color_rgb(1.0, 0.5, 0.0),
                                           0,
                                           "1 0.5 0 RG\n"));

    memset(buffer, 'x', sizeof(buffer));
    D_TESTS_PDF_CHECK(d_pdf_set_color(NULL, 1, buffer, sizeof(buffer)) == 0u);
    D_TESTS_PDF_CHECK(buffer[0] == '\0');

    return;
}

/*
d_tests_pdf_locales
  Tests the following:
  - under de_DE.UTF-8 and ps_AF.UTF-8, where "%g" writes the locale's decimal
    point, numbers and colours still write '.'
*/
static void
d_tests_pdf_locales(void)
{
    const char* const locales[] = { "de_DE.UTF-8", "ps_AF.UTF-8" };

    printf("  locales\n");

    for (size_t i = 0u; i < (sizeof(locales) / sizeof(locales[0])); ++i)
    {
        if (setlocale(LC_ALL, locales[i]) == NULL)
        {
            printf("    (skipped: locale %s not installed)\n", locales[i]);

            continue;
        }

        D_TESTS_PDF_CHECK(d_tests_pdf_num_is(1.5, "1.5"));
        D_TESTS_PDF_CHECK(d_tests_pdf_num_is(-0.125, "-0.125"));
        D_TESTS_PDF_CHECK(d_tests_pdf_color_is(d_pdf_color_rgb(0.5, 0.25, 1.0),
                                               1,
                                               "0.5 0.25 1 rg\n"));
    }

    (void)setlocale(LC_ALL, "C");

    return;
}

/*
d_tests_pdf_toy_slot
  A slot of the toy backend that does nothing and succeeds.
*/
static int32_t
d_tests_pdf_toy_slot(
    void* _context
)
{
    (void)_context;

    return 1;
}

/*
d_tests_pdf_toy_save
  The toy backend's save: a path is never written.
*/
static int32_t
d_tests_pdf_toy_save(
    void*       _context,
    const char* _path
)
{
    (void)_context;
    (void)_path;

    return 0;
}

/*
d_tests_pdf_toy_save_to
  The toy backend's save_to: writes a fixed document, or fails on request.
*/
static int32_t
d_tests_pdf_toy_save_to(
    void*              _context,
    struct dj_ostream* _os
)
{
    struct dj_tests_pdf_toy* const toy = (struct dj_tests_pdf_toy*)_context;

    ++toy->calls;

    if (toy->fail)
    {
        return 0;
    }

    (void)d_ostream_write_cstr(_os, "%PDF-1.4\n%%EOF\n");

    return d_ostream_is_ok(_os);
}

/*
d_tests_pdf_save_to
  Tests the following:
  - save_to closes the document and writes the backend's bytes to a stream
  - a slot that fails is D_PDF_DOC_BACKEND_FAILED
  - a null slot, and a table from before the slot (size short of it), are
    D_PDF_DOC_UNSUPPORTED, and the slot is not called
*/
static void
d_tests_pdf_save_to(void)
{
    struct dj_tests_pdf_toy toy     = { 0, 0 };
    struct dj_pdf_backend   backend;
    struct dj_pdf_document  doc;
    struct dj_ostream       os;
    struct dj_ostream_fixed fixed;
    char                    buffer[64];

    printf("  save_to\n");

    memset(&backend, 0, sizeof(backend));
    backend.size           = sizeof(backend);
    backend.context        = &toy;
    backend.begin_document = d_tests_pdf_toy_slot;
    backend.end_document   = d_tests_pdf_toy_slot;
    backend.end_page       = d_tests_pdf_toy_slot;
    backend.save           = d_tests_pdf_toy_save;
    backend.save_to        = d_tests_pdf_toy_save_to;

    // the bytes reach the stream
    D_TESTS_PDF_CHECK(d_pdf_document_init(&doc, &backend));
    D_TESTS_PDF_CHECK(d_pdf_document_open(&doc));
    (void)d_ostream_init_fixed(&os, &fixed, buffer, sizeof(buffer), 1);
    D_TESTS_PDF_CHECK(d_pdf_document_save_to(&doc, &os));
    (void)d_ostream_close(&os);
    D_TESTS_PDF_CHECK(strcmp(buffer, "%PDF-1.4\n%%EOF\n") == 0);
    D_TESTS_PDF_CHECK(toy.calls == 1);

    // the backend fails
    toy.fail = 1;
    (void)d_ostream_init_fixed(&os, &fixed, buffer, sizeof(buffer), 1);
    D_TESTS_PDF_CHECK(!d_pdf_document_save_to(&doc, &os));
    D_TESTS_PDF_CHECK(d_pdf_document_last_status(&doc) ==
                      D_PDF_DOC_BACKEND_FAILED);

    // a null slot
    backend.save_to = NULL;
    D_TESTS_PDF_CHECK(!d_pdf_document_save_to(&doc, &os));
    D_TESTS_PDF_CHECK(d_pdf_document_last_status(&doc) ==
                      D_PDF_DOC_UNSUPPORTED);

    // a table from before the slot: it is never read
    backend.save_to = d_tests_pdf_toy_save_to;
    backend.size    = offsetof(struct dj_pdf_backend, save_to);
    toy.calls       = 0;
    D_TESTS_PDF_CHECK(d_pdf_backend_is_valid(&backend));
    D_TESTS_PDF_CHECK(!d_pdf_document_save_to(&doc, &os));
    D_TESTS_PDF_CHECK(d_pdf_document_last_status(&doc) ==
                      D_PDF_DOC_UNSUPPORTED);
    D_TESTS_PDF_CHECK(toy.calls == 0);
    (void)d_ostream_close(&os);

    D_TESTS_PDF_CHECK(!d_pdf_document_save_to(NULL, &os));

    return;
}


/*
main
  Every group, then the count.
*/
int
main(void)
{
    printf("t_pdf_stream\n");

    d_tests_pdf_numbers();
    d_tests_pdf_colors();
    d_tests_pdf_locales();
    d_tests_pdf_save_to();

    printf("%lu checks, %lu failed\n", d_tests_pdf_checks, d_tests_pdf_failed);

    return (d_tests_pdf_failed == 0u) ? 0 : 1;
}
