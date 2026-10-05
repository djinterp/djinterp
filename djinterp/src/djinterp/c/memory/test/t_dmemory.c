/*******************************************************************************
* djinterp [test]                                                    t_dmemory.c
*
* Run-time checks of dmemory.h's bounded functions: d_memset_s and d_memcpy_s
* return 0, EINVAL or ERANGE as Annex K's memset_s and memcpy_s do, clear or
* fill as they promise on failure, and the d_memdup pair duplicates or refuses.
*
*   Build (from the repo root), at C99 or later:
*     cc -std=c99 -pedantic-errors -Wall -Wextra                              \
*         src/djinterp/c/memory/test/t_dmemory.c                              \
*         src/djinterp/c/memory/dmemory.c -o t_dmemory
*
*
* path:      /src/djinterp/c/memory/test/t_dmemory.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.29
*                                                            revised: 2026.09.29
*******************************************************************************/
#include "../../../../../inc/djinterp/c/memory/dmemory.h"  // module under test

// std
#include <stdio.h>   // printf
#include <stdlib.h>  // free
#include <string.h>  // memcmp


static int t_failures = 0;

// T_CHECK
//   macro: records and reports a failed condition, then carries on.
#define T_CHECK(_cond)                                                         \
    do                                                                         \
    {                                                                          \
        if (!(_cond))                                                          \
        {                                                                      \
            printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #_cond);             \
            ++t_failures;                                                      \
        }                                                                      \
    } while (0)


int
main(void)
{
    unsigned char buffer[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    unsigned char source[4] = { 9, 9, 9, 9 };
    void*         copy;

    // d_memset_s: a fill that fits, one that does not, and bad arguments
    T_CHECK( (d_memset_s(buffer, sizeof(buffer), 0, 4) == 0) &&
             (buffer[3] == 0) && (buffer[4] == 5) );
    // too long: the whole destination is filled, then ERANGE is reported
    T_CHECK( (d_memset_s(buffer, sizeof(buffer), 7, 9) == ERANGE) &&
             (buffer[7] == 7) );
    T_CHECK(d_memset_s(NULL, 8, 0, 1) == EINVAL);
    // a negative size converted to size_t is refused
    T_CHECK(d_memset_s(buffer, (size_t)-1, 0, 1) == EINVAL);

    // d_memcpy_s: a copy that fits, one that does not, and a null source
    T_CHECK( (d_memcpy_s(buffer, sizeof(buffer), source, 4) == 0) &&
             (buffer[0] == 9) && (buffer[4] == 7) );
    // too long: the destination is cleared
    T_CHECK( (d_memcpy_s(buffer, 2, source, 4) == ERANGE) &&
             (buffer[0] == 0) && (buffer[1] == 0) );
    T_CHECK(d_memcpy_s(buffer, sizeof(buffer), NULL, 1) == EINVAL);

    // d_memdup and d_memdup_s
    copy = d_memdup(source, sizeof(source));
    T_CHECK( (copy != NULL) && (memcmp(copy, source, 4) == 0) );
    free(copy);

    copy = d_memdup_s(source, 0);
    T_CHECK(copy == NULL);

    copy = d_memdup_s(source, sizeof(source));
    T_CHECK( (copy != NULL) && (memcmp(copy, source, 4) == 0) );
    free(copy);

    printf("DMEMORY: %s (%d failures)\n",
           (t_failures ? "FAILED" : "all checks passed"),
           t_failures);

    return (t_failures != 0);
}
