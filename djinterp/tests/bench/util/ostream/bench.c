/*******************************************************************************
* djinterp [test]                                                        bench.c
*
*   The output stream's cost, measured: one source, built against this tree's
* kernel and against the 2026.10.07 one (-DBENCH_OLD with that branch's
* headers first on the include path), so the two run the same loops. Every
* stream writes to a sink that discards, through a 64 KiB buffer, so what is
* measured is the stream and nothing behind it; a char array written by hand
* is the floor. Each figure is the best of seven runs, in nanoseconds per
* operation.
*
*
* path:      /tests/bench/util/ostream/bench.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.11.09
*******************************************************************************/
// std
#include <stdio.h>   // printf, snprintf
#include <string.h>  // memcpy
#include <time.h>    // clock_gettime
// djinterp
#include "djinterp/c/util/ostream/ostream.h"         // the stream
#include "djinterp/c/text/ostream_format.h"  // d_ostream_printf
#include "djinterp/c/dstring.h"              // d_string


#define BENCH_RUNS  7
#define BENCH_CHARS 100000000L

static unsigned char  g_buffer[1 << 16];
static volatile int   g_sink_bytes;

/*
bench_discard
  The sink: takes everything, keeps nothing.
*/
static size_t
bench_discard(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    (void)_context;
    g_sink_bytes += (int)((const unsigned char*)_data)[0];

    return _size;
}

/*
bench_now
  Seconds, monotonic.
*/
static double
bench_now(void)
{
    struct timespec t;

    clock_gettime(CLOCK_MONOTONIC, &t);

    return (double)t.tv_sec + ((double)t.tv_nsec * 1e-9);
}

/*
bench_open
  A stream over the discarding sink, through the 64 KiB buffer.
*/
static void
bench_open(
    struct dj_ostream* _os
)
{
    struct dj_pack_sink sink;

    sink.write   = &bench_discard;
    sink.context = NULL;
    (void)d_ostream_init(_os, sink, NULL, 0);
    (void)d_ostream_set_buffer(_os, g_buffer, sizeof(g_buffer));

    return;
}

// BENCH
//   macro: times a loop body over _count iterations, best of BENCH_RUNS, and
// prints nanoseconds per iteration.
#define BENCH(name, count, setup, body, teardown)                              \
    do                                                                         \
    {                                                                          \
        double best = 1e30;                                                    \
        for (int run = 0; run < BENCH_RUNS; ++run)                             \
        {                                                                      \
            setup;                                                             \
            const double start = bench_now();                                  \
            for (long i = 0; i < (count); ++i)                                 \
            {                                                                  \
                body;                                                          \
            }                                                                  \
            const double took = bench_now() - start;                           \
            teardown;                                                          \
            best = (took < best) ? took : best;                                \
        }                                                                      \
        printf("%-34s %7.2f ns\n", (name), best * 1e9 / (double)(count));      \
    } while (0)

int
main(void)
{
    struct dj_ostream os;
    unsigned char*   p = NULL;
    unsigned char*   e = NULL;

#if defined(BENCH_OLD)
    printf("kernel of 2026.10.07\n");
#else
    printf("kernel of 2026.10.28\n");
#endif

    BENCH("floor: char array, by hand", BENCH_CHARS,
          (p = g_buffer, e = g_buffer + sizeof(g_buffer)),
          { if (p == e) { g_sink_bytes += p[-1]; p = g_buffer; }
            *p++ = (unsigned char)i; },
          (void)0);

    BENCH("write_char", BENCH_CHARS,
          bench_open(&os),
          (void)d_ostream_write_char(&os, (char)i),
          (void)d_ostream_close(&os));

#if !defined(BENCH_OLD)
    {
        struct dj_ostream_cursor c;

        BENCH("cursor_put", BENCH_CHARS,
              (bench_open(&os), c = d_ostream_cursor_take(&os)),
              c = d_ostream_cursor_put(&os, c, (char)i),
              ((void)d_ostream_cursor_give(&os, c), (void)d_ostream_close(&os)));
    }
#endif

    BENCH("write_cstr(\"abcdefgh\")", BENCH_CHARS / 8,
          bench_open(&os),
          (void)d_ostream_write_cstr(&os, "abcdefgh"),
          (void)d_ostream_close(&os));

#if !defined(BENCH_OLD)
    {
        struct dj_ostream_cursor c;

        BENCH("cursor_write(8 bytes)", BENCH_CHARS / 8,
              (bench_open(&os), c = d_ostream_cursor_take(&os)),
              c = d_ostream_cursor_write(&os, c, "abcdefgh", 8u),
              ((void)d_ostream_cursor_give(&os, c), (void)d_ostream_close(&os)));
    }
#endif

    BENCH("write_long(i)", BENCH_CHARS / 10,
          bench_open(&os),
          (void)d_ostream_write_long(&os, i),
          (void)d_ostream_close(&os));

    BENCH("printf(\"%d,%d;\")", BENCH_CHARS / 50,
          bench_open(&os),
          (void)d_ostream_printf(&os, "%d,%d;", (int)i, (int)(i >> 3)),
          (void)d_ostream_close(&os));

    {
        struct dj_string* s = NULL;

        BENCH("d_string: write_cstr(8 bytes)", BENCH_CHARS / 8,
              (s = d_string_new(), (void)d_ostream_init_string(&os, s)),
              (void)d_ostream_write_cstr(&os, "abcdefgh"),
              ((void)d_ostream_close(&os), d_string_free(s)));

        BENCH("d_string: write_char", BENCH_CHARS / 4,
              (s = d_string_new(), (void)d_ostream_init_string(&os, s)),
              (void)d_ostream_write_char(&os, (char)i),
              ((void)d_ostream_close(&os), d_string_free(s)));
    }

    return (g_sink_bytes == 12345) ? 1 : 0;
}
