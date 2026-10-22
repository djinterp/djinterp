/*******************************************************************************
* djinterp [c]                                                     t_file_wasi.c
*
*   Runtime harness for c/filesys on WASI, beside t_file_noplat.c, which holds
* it to what every build on the ISO C backend must do. WASI's libc has no
* tmpfile, and a clock only where the build asks for its emulation
* (cfg_file_common.h, 1.9.18 and 1.9.19); this checks that the build knows
* both, that d_file_temp_stream then fails with ENOSYS, that a temporary name
* is still made, seeded without the clock, and that a file round-trips. It
* works in the current directory, which the runtime must preopen as ".".
*   ci/check_filesys_platforms.sh runs it under wasmtime, with and without
* the clock emulation; by hand, from the repo root, with zig:
*     zig cc -target wasm32-wasi -std=c99 -Wall -Wextra -Werror              \
*        src/djinterp/c/filesys/file_*.c                                       \
*        tests/djinterp/c/filesys/t_file_wasi.c -o t_file_wasi.wasm
*     wasmtime run --dir=. t_file_wasi.wasm
*
*
* path:      /tests/djinterp/c/filesys/t_file_wasi.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.19
*******************************************************************************/
// std
#include <errno.h>   // errno, ENOSYS
#include <stdio.h>   // FILE, printf, remove, stdout
#include <stdlib.h>  // free
#include <string.h>  // memcmp, strstr
// djinterp
#include "../../../../inc/djinterp/c/filesys/dfile.h"  // unit under test


static int g_checks   = 0;
static int g_failures = 0;

// CHECK
//   macro: count a check, and report it when it fails.
#define CHECK(_cond, _what)                                                    \
    do                                                                         \
    {                                                                          \
        ++g_checks;                                                            \
                                                                               \
        if (!(_cond))                                                          \
        {                                                                      \
            ++g_failures;                                                      \
            printf("FAIL %s (line %d, errno %d)\n",                            \
                   (_what),                                                    \
                   __LINE__,                                                   \
                   errno);                                                     \
        }                                                                      \
    } while (0)

// T_WASI_CLOCK
//   constant: the clock gate this build must resolve to -- on only where the
// build asks for the emulation.
#if defined(_WASI_EMULATED_PROCESS_CLOCKS)
    #define T_WASI_CLOCK 1
#else
    #define T_WASI_CLOCK 0
#endif


/*
t_wasi_gates
  The build is on the ISO C backend, and knows what WASI's libc lacks.
*/
static void
t_wasi_gates(
    void
)
{
    CHECK(D_FILE_BACKEND_IS_STDC,
          "WASI builds on the ISO C backend");
    CHECK(D_CFG_IS_OFF(D_CFG_FILE_HAS_TMPFILE),
          "the build knows there is no tmpfile");
    CHECK(D_CFG_IS_ON(D_CFG_FILE_HAS_CLOCK) == T_WASI_CLOCK,
          "the build reads a processor clock only where it asked for one");

    return;
}

/*
t_wasi_temporaries
  No anonymous temporary, said with ENOSYS.
*/
static void
t_wasi_temporaries(
    void
)
{
    FILE* stream = stdout;  // anything but NULL, to see it cleared

    errno = 0;
    CHECK( (d_file_temp_stream() == NULL) &&
           (errno == ENOSYS),
           "d_file_temp_stream is ENOSYS");
    CHECK( (d_file_temp_stream_s(&stream) == ENOSYS) &&
           (stream == NULL),
           "d_file_temp_stream_s is ENOSYS, its stream NULL");

    return;
}

#if (D_INTERNAL_FILE_TEMP_TMPNAM == 1)

/*
t_wasi_name
  A temporary name, made without the clock.
*/
static void
t_wasi_name(
    void
)
{
    char name[256] = {0};

    CHECK( (d_file_temp_name(name, sizeof(name)) == 0) &&
           (strstr(name, "djtmp_") != NULL),
           "a temporary name is made, seeded without the clock");

    return;
}

#else

/*
t_wasi_name
  This build makes no racy names (D_CFG_FILE_TEMP_TMPNAM).
*/
static void
t_wasi_name(
    void
)
{
    return;
}

#endif  // D_INTERNAL_FILE_TEMP_TMPNAM

/*
t_wasi_streams
  A file written, read back and removed.
*/
static void
t_wasi_streams(
    void
)
{
    size_t length = 0;
    char*  data   = NULL;

    CHECK(d_file_write_all("w.txt", "wasi", 4) == 0,
          "a stream write");

    data = d_file_read_all("w.txt", &length);
    CHECK( (data != NULL) &&
           (length == 4)  &&
           (memcmp(data, "wasi", 4) == 0),
           "  reads back");
    free(data);
    CHECK(remove("w.txt") == 0,
          "  and is removed");

    return;
}


int
main(
    void
)
{
    t_wasi_gates();
    t_wasi_temporaries();
    t_wasi_name();
    t_wasi_streams();

    printf("t_file_wasi: %d checks, %d failed\n",
           g_checks,
           g_failures);

    return (g_failures == 0) ? 0 : 1;
}
