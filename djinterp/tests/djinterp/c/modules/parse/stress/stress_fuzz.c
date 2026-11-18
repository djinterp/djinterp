/*******************************************************************************
* djinterp [c/parse]                                               stress_fuzz.c
*
* Expression core v2's libFuzzer harness: each input is scanned and parsed
* by dawk's signature, through the same path as the stress suite's random
* bytes (d_tests_stress_fuzz_one), and an input that leaves the parse out of
* its bounds aborts, so the fuzzer keeps it.
*   Built with -fsanitize=fuzzer where the compiler offers it. Built with
* D_TESTS_PARSE_STRESS_REPLAY instead, the same file is a program that runs
* the harness over the files it is given: the corpus, replayed as a test on
* every compiler, each find kept there as a case.
*
*
* path:      /tests/djinterp/c/modules/parse/stress/stress_fuzz.c
* link(s):   /docs/djinterp/parse/expression_core_v2.md
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.18
*                                                            revised: 2026.11.18
*******************************************************************************/
#include "./stress_tests.h"  // d_tests_stress_fuzz_one
// std
#include <stdlib.h>  // abort, malloc, free


#ifdef D_TESTS_PARSE_STRESS_REPLAY

/*
d_tests_stress_replay_file
  Runs the harness over one file's bytes: 0 where it held, 1 where it did
not or the file could not be read.
*/
static int
d_tests_stress_replay_file(
    const char* _path
)
{
    FILE*          file = fopen(_path, "rb");
    unsigned char* data = NULL;
    long           size = -1;
    int            failed;

    if (file)
    {
        if (fseek(file, 0L, SEEK_END) == 0)
        {
            size = ftell(file);
        }

        if ( (size >= 0) &&
             (fseek(file, 0L, SEEK_SET) == 0) )
        {
            data = (unsigned char*)malloc((size > 0) ? (size_t)size : 1u);
        }
    }

    failed = ( (!data) ||
               (fread(data, 1u, (size_t)size, file) != (size_t)size) ||
               (!d_tests_stress_fuzz_one(data, (size_t)size)) );

    if (failed)
    {
        fprintf(stderr, "replay: %s did not hold\n", _path);
    }

    free(data);

    if (file)
    {
        (void)fclose(file);
    }

    return failed;
}

/*
main
  The replay: each file named, through the harness. Returns 0 only when
every one held, and says how many it ran.
*/
int
main(
    int   _argc,
    char* _argv[]
)
{
    int failed = 0;

    for (int i = 1; i < _argc; ++i)
    {
        failed += d_tests_stress_replay_file(_argv[i]);
    }

    printf("replay: %d inputs, %d did not hold\n", _argc - 1, failed);

    return (failed == 0) ? 0 : 1;
}

#else

/*
LLVMFuzzerTestOneInput
  libFuzzer's entry: one input through the harness, which aborts where the
parse left its bounds.
*/
int LLVMFuzzerTestOneInput(const uint8_t* _data,
                           size_t         _size);

int
LLVMFuzzerTestOneInput(
    const uint8_t* _data,
    size_t         _size
)
{
    if (!d_tests_stress_fuzz_one(_data, _size))
    {
        abort();
    }

    return 0;
}

#endif  // D_TESTS_PARSE_STRESS_REPLAY
