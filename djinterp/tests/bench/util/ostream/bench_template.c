/*******************************************************************************
* djinterp [test]                                               bench_template.c
*
*   The template engine's and the environment printer's cost, measured. One
* line of the text style's entry -- two spaces, a label padded to a column,
* two spaces, a value -- written four ways: by hand through the stream, by a
* compiled template with a literal width and with a width from a field, and
* by snprintf; then a template's compilation, a whole report printed as text
* and as JSON, and a printer made and configured by a script. Every stream
* writes into a 64 KiB array, rewound before it fills. Each figure is the
* best of seven runs.
*
*
* path:      /tests/bench/util/ostream/bench_template.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.11.09
*******************************************************************************/
// std
#include <stdio.h>   // printf, snprintf
#include <string.h>  // memcmp, strlen
#include <time.h>    // clock_gettime
// djinterp
#include "djinterp/c/util/ostream/ostream.h"           // the stream
#include "djinterp/c/text/ostream_template.h"  // templates
#include "djinterp/env/env_printer.h"          // the printer


#define BENCH_RUNS    7
#define BENCH_LINES   2000000L
#define BENCH_REPORTS 20000L

static const char* const BENCH_LABELS[4] =
{
    "Compiler", "Full Name", "Version", "Major"
};

static const char* const BENCH_VALUES[4] =
{
    "GCC", "GNU Compiler Collection", "13.3.0", "13"
};

static const char* const BENCH_FIELDS[4] =
{
    "prefix", "label", "width", "display"
};

static char          g_lines[1 << 16];
static char          g_report[1 << 16];
static volatile long g_sink;

/*
bench_now
  Seconds, monotonic.
*/
static double
bench_now(void)
{
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);

    return (double)now.tv_sec + ((double)now.tv_nsec * 1e-9);
}

/*
bench_lookup
  The line's four fields, by name.
*/
static int32_t
bench_lookup(
    void*       _context,
    const char* _name,
    size_t      _size
)
{
    (void)_context;

    for (int32_t i = 0; i < 4; ++i)
    {
        if ( (strlen(BENCH_FIELDS[i]) == _size) &&
             (memcmp(BENCH_FIELDS[i], _name, _size) == 0) )
        {
            return i;
        }
    }

    return -1;
}

/*
bench_resolve
  The line's values: no prefix, the line's label, a width of 9, its value.
*/
static int32_t
bench_resolve(
    void*                   _context,
    uint32_t                _field,
    struct dj_ostream_value* _value
)
{
    const long line = *(const long*)_context;

    switch (_field)
    {
        case 0u:  *_value = d_ostream_value_text_n("", 0u);              break;
        case 1u:  *_value = d_ostream_value_text(BENCH_LABELS[line & 3]); break;
        case 2u:  *_value = d_ostream_value_long(9L);                    break;
        default:  *_value = d_ostream_value_text(BENCH_VALUES[line & 3]); break;
    }

    return 1;
}

/*
bench_hand
  The line by hand: the floor a template is held to.
*/
static double
bench_hand(void)
{
    struct dj_ostream       os;
    struct dj_ostream_fixed fixed;
    const double           start = bench_now();

    for (long i = 0; i < BENCH_LINES; ++i)
    {
        const char* const label = BENCH_LABELS[i & 3];
        const size_t      size  = strlen(label);

        if ((i & 511) == 0)
        {
            (void)d_ostream_init_fixed(&os, &fixed, g_lines,
                                       sizeof(g_lines), 0);
        }

        (void)d_ostream_write(&os, "  ", 2u);
        (void)d_ostream_write(&os, label, size);
        (void)d_ostream_fill(&os, ' ', 9u - size);
        (void)d_ostream_write(&os, "  ", 2u);
        (void)d_ostream_write_cstr(&os, BENCH_VALUES[i & 3]);
        (void)d_ostream_write_char(&os, '\n');
    }

    g_sink += (long)d_ostream_written(&os);

    return (bench_now() - start) / (double)BENCH_LINES * 1e9;
}

/*
bench_template
  The line by a compiled template.
*/
static double
bench_template(
    const char* _source
)
{
    struct dj_ostream_template_op    ops[16];
    struct dj_ostream_template       compiled;
    struct dj_ostream_template_error error;
    struct dj_ostream                os;
    struct dj_ostream_fixed          fixed;
    long                            line = 0;

    (void)d_ostream_template_compile(&compiled, ops, 16u, _source,
                                     bench_lookup, NULL, &error);

    const double start = bench_now();

    for (line = 0; line < BENCH_LINES; ++line)
    {
        if ((line & 511) == 0)
        {
            (void)d_ostream_init_fixed(&os, &fixed, g_lines,
                                       sizeof(g_lines), 0);
        }

        (void)d_ostream_template_render(&os, &compiled, bench_resolve, &line);
    }

    g_sink += (long)d_ostream_written(&os);

    return (bench_now() - start) / (double)BENCH_LINES * 1e9;
}

/*
bench_snprintf
  The line by snprintf.
*/
static double
bench_snprintf(void)
{
    size_t       at    = 0u;
    const double start = bench_now();

    for (long i = 0; i < BENCH_LINES; ++i)
    {
        if ((i & 511) == 0)
        {
            at = 0u;
        }

        at += (size_t)snprintf(g_lines + at, sizeof(g_lines) - at,
                               "%s  %-*s  %s\n", "", 9, BENCH_LABELS[i & 3],
                               BENCH_VALUES[i & 3]);
    }

    g_sink += (long)at;

    return (bench_now() - start) / (double)BENCH_LINES * 1e9;
}

/*
bench_compile
  The line's template, compiled.
*/
static double
bench_compile(void)
{
    struct dj_ostream_template_op    ops[16];
    struct dj_ostream_template       compiled;
    struct dj_ostream_template_error error;
    const double                    start = bench_now();

    for (long i = 0; i < BENCH_LINES / 10; ++i)
    {
        (void)d_ostream_template_compile(
                  &compiled, ops, 16u,
                  "{prefix}  {label:<{width}}  {display}\n",
                  bench_lookup, NULL, &error);
        g_sink += (long)compiled.count;
    }

    return (bench_now() - start) / (double)(BENCH_LINES / 10) * 1e9;
}

/*
bench_report
  A whole report -- this unit's, every section -- printed, in microseconds.
*/
static double
bench_report(
    struct dj_env_printer* _printer
)
{
    struct dj_ostream       os;
    struct dj_ostream_fixed fixed;
    const double           start = bench_now();

    for (long i = 0; i < BENCH_REPORTS; ++i)
    {
        (void)d_ostream_init_fixed(&os, &fixed, g_report, sizeof(g_report), 0);
        (void)d_env_printer_print(_printer, &os, d_env_report_here());
    }

    g_sink += (long)d_ostream_written(&os);

    return (bench_now() - start) / (double)BENCH_REPORTS * 1e6;
}

/*
bench_configure
  A printer made, and configured by a three-statement script, in
microseconds.
*/
static double
bench_configure(void)
{
    const double start = bench_now();

    for (long i = 0; i < BENCH_REPORTS; ++i)
    {
        struct dj_env_printer printer;

        (void)d_env_printer_init(&printer);
        (void)d_env_printer_load(&printer,
                                 "style=json; sections=all; key_width=20",
                                 NULL);
        g_sink += (long)printer.compiled;
    }

    return (bench_now() - start) / (double)BENCH_REPORTS * 1e6;
}

/*
bench_best
  The least of a figure and a new one.
*/
static double
bench_best(
    double _best,
    double _figure
)
{
    return ( (_best == 0.0) ||
             (_figure < _best) ) ? _figure : _best;
}

/*
main
  Seven runs of each, the best kept.
*/
int
main(void)
{
    struct dj_env_printer printer;
    double               best[8] = { 0.0 };

    (void)d_env_printer_init(&printer);

    for (int run = 0; run < BENCH_RUNS; ++run)
    {
        best[0] = bench_best(best[0], bench_hand());
        best[1] = bench_best(best[1], bench_template(
                      "{prefix}  {label:<9}  {display}\n"));
        best[2] = bench_best(best[2], bench_template(
                      "{prefix}  {label:<{width}}  {display}\n"));
        best[3] = bench_best(best[3], bench_snprintf());
        best[4] = bench_best(best[4], bench_compile());
        (void)d_env_printer_put(&printer, "style=text; sections=all");
        best[5] = bench_best(best[5], bench_report(&printer));
        (void)d_env_printer_put(&printer, "style=json");
        best[6] = bench_best(best[6], bench_report(&printer));
        best[7] = bench_best(best[7], bench_configure());
    }

    printf("One line: two spaces, a label padded to 9, two spaces, a value\n");
    printf("by hand, through the stream          %6.1f ns\n", best[0]);
    printf("template, width 9                    %6.1f ns\n", best[1]);
    printf("template, width from a field         %6.1f ns\n", best[2]);
    printf("snprintf(\"%%s  %%-*s  %%s\\n\")          %6.1f ns\n", best[3]);
    printf("compiling that template              %6.1f ns\n", best[4]);
    printf("\nThe environment printer, this C unit's report, every section\n");
    printf("as text                              %6.2f us\n", best[5]);
    printf("as JSON                              %6.2f us\n", best[6]);
    printf("made and configured by a script      %6.2f us\n", best[7]);
    printf("sizeof(struct dj_env_printer)         %6lu bytes\n",
           (unsigned long)sizeof(struct dj_env_printer));

    return (g_sink == 0) ? 1 : 0;
}
