/*******************************************************************************
* djinterp [test]                                                  bench_stdio.c
*
*   The C library's own costs for the same operations as bench.c, to /dev/null
* through a 64 KiB stdio buffer, best of seven runs, nanoseconds per
* operation: the yardstick the stream is measured against.
*
*
* path:      /tests/bench/util/ostream/bench_stdio.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.11.09
*******************************************************************************/
// std
#include <stdio.h>   // FILE, fopen, fputc, putc_unlocked, fwrite, fprintf
#include <time.h>    // clock_gettime

#define BENCH_RUNS  7
#define BENCH_CHARS 100000000L

static char g_buffer[1 << 16];
static char g_text[64];

static double
bench_now(void)
{
    struct timespec t;

    clock_gettime(CLOCK_MONOTONIC, &t);

    return (double)t.tv_sec + ((double)t.tv_nsec * 1e-9);
}

#define BENCH(name, count, body)                                               \
    do                                                                         \
    {                                                                          \
        double best = 1e30;                                                    \
        for (int run = 0; run < BENCH_RUNS; ++run)                             \
        {                                                                      \
            const double start = bench_now();                                  \
            for (long i = 0; i < (count); ++i)                                 \
            {                                                                  \
                body;                                                          \
            }                                                                  \
            (void)fflush(f);                                                   \
            const double took = bench_now() - start;                           \
            best = (took < best) ? took : best;                                \
        }                                                                      \
        printf("%-34s %7.2f ns\n", (name), best * 1e9 / (double)(count));      \
    } while (0)

int
main(void)
{
    FILE* const f = fopen("/dev/null", "wb");

    if (f == NULL)
    {
        return 1;
    }

    (void)setvbuf(f, g_buffer, _IOFBF, sizeof(g_buffer));
    printf("C library (glibc), to /dev/null\n");
    BENCH("fputc", BENCH_CHARS, (void)fputc((int)(i & 0x7F), f));
    BENCH("putc_unlocked", BENCH_CHARS, (void)putc_unlocked((int)(i & 0x7F), f));
    BENCH("fwrite(8 bytes)", BENCH_CHARS / 8, (void)fwrite("abcdefgh", 1, 8, f));
    BENCH("fprintf(\"%ld\")", BENCH_CHARS / 10, (void)fprintf(f, "%ld", i));
    BENCH("snprintf(\"%ld\") to a buffer", BENCH_CHARS / 10,
          (void)snprintf(g_text, sizeof(g_text), "%ld", i));
    BENCH("fprintf(\"%d,%d;\")", BENCH_CHARS / 50,
          (void)fprintf(f, "%d,%d;", (int)i, (int)(i >> 3)));
    (void)fclose(f);

    return 0;
}
