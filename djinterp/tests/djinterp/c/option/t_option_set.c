/*******************************************************************************
* djinterp [c]                                                    t_option_set.c
*
*   Conformance harness for option_set.h's pristine mark and by-value status:
* the mark lowers every cell's ASSIGNED flag and nothing else, the test sees
* any single assigned cell, and the status of a result matches its pointer
* form in both arms.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -Wall -Wextra -Werror -Iinc                                  \
*        src/djinterp/c/option/option_set.c                                    \
*        src/djinterp/c/option/option_common.c                                 \
*        tests/djinterp/c/option/t_option_set.c -o t_option_set
*
*
* path:      /tests/djinterp/c/option/t_option_set.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.09.30
*******************************************************************************/
// std
#include <stdbool.h>  // bool
#include <stdio.h>    // printf
#include <string.h>   // memset
// djinterp
#include "../../../../inc/djinterp/c/option/option_set.h"  // unit under test


static int g_checks   = 0;
static int g_failures = 0;

static void
d_tests_option_expect(
    bool        _ok,
    const char* _what
)
{
    ++g_checks;

    // report a failure with what was being checked
    if (!_ok)
    {
        ++g_failures;
        printf("  FAIL: %s\n", _what);
    }

    return;
}

int
main(void)
{
    struct d_option     cells[4];
    struct d_option_set set;

    memset(cells, 0, sizeof(cells));
    memset(&set, 0, sizeof(set));
    set.options  = cells;
    set.count    = 3u;
    set.capacity = 4u;

    // a missing set, and a fresh one, are pristine
    d_tests_option_expect(d_option_set_is_pristine(NULL), "NULL is pristine");
    d_tests_option_expect(d_option_set_is_pristine(&set), "fresh is pristine");

    // one assigned cell is enough to end it
    cells[2].flags = D_OPTION_FLAG_ASSIGNED;
    d_tests_option_expect(!d_option_set_is_pristine(&set), "one assigned");

    // the mark lowers ASSIGNED everywhere and leaves every other flag
    cells[0].flags = D_OPTION_FLAG_ASSIGNED | 0x10u;
    cells[3].flags = D_OPTION_FLAG_ASSIGNED;
    d_option_set_mark_pristine(&set);
    d_tests_option_expect(d_option_set_is_pristine(&set), "marked pristine");
    d_tests_option_expect( (cells[0].flags == 0x10u) &&
                           (cells[2].flags == 0u),
                           "only ASSIGNED lowered" );
    d_tests_option_expect(cells[3].flags == D_OPTION_FLAG_ASSIGNED,
                          "cells past count untouched");
    d_option_set_mark_pristine(NULL);

    // the by-value status agrees with the pointer form in both arms
    {
        const struct d_option_result ok   = d_option_ok(7u);
        const struct d_option_result fail =
            d_option_fail((int32_t)D_OPTION_STATUS_BUFFER_TOO_SMALL);

        d_tests_option_expect(
            (d_option_result_status_of(ok) == (int32_t)D_OPTION_STATUS_OK) &&
            (d_option_result_status_of(fail) ==
             (int32_t)D_OPTION_STATUS_BUFFER_TOO_SMALL) &&
            (d_option_result_status_of(fail) ==
             d_option_result_status(&fail)),
            "status of");
    }

    printf("t_option_set: %d checks, %d failed\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
