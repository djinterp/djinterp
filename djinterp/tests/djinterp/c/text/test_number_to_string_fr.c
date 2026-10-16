/*******************************************************************************
* djinterp [text]                                     test_number_to_string_fr.c
*
*
* path:      /tests/djinterp/c/text/test_number_to_string_fr.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <stdio.h>
// djinterp
#include "../../../../inc/djinterp/c/text/number_to_string_fr.h"

static void show(int64_t value, unsigned flags)
{
    char buf[NTS_FR_I64_BUFFER_SIZE];

    nts_fr_cardinal_i64(buf, sizeof(buf), value, flags);
    printf("cardinal(%lld) = %s\n", (long long)value, buf);

    nts_fr_ordinal_i64(buf, sizeof(buf), value, flags);
    printf("ordinal (%lld) = %s\n", (long long)value, buf);

    nts_fr_ordinal_abbrev_i64(buf, sizeof(buf), value, flags);
    printf("abbr    (%lld) = %s\n", (long long)value, buf);
}

int main(void)
{
    show(0, NTS_FR_FLAGS_DEFAULT);
    show(1, NTS_FR_FLAGS_DEFAULT);
    show(2, NTS_FR_FLAGS_DEFAULT);
    show(21, NTS_FR_FLAGS_DEFAULT);
    show(31, NTS_FR_FLAGS_DEFAULT);
    show(71, NTS_FR_FLAGS_DEFAULT);
    show(80, NTS_FR_FLAGS_DEFAULT);
    show(81, NTS_FR_FLAGS_DEFAULT);
    show(91, NTS_FR_FLAGS_DEFAULT);
    show(100, NTS_FR_FLAGS_DEFAULT);
    show(200, NTS_FR_FLAGS_DEFAULT);
    show(201, NTS_FR_FLAGS_DEFAULT);
    show(1000, NTS_FR_FLAGS_DEFAULT);
    show(1001, NTS_FR_FLAGS_DEFAULT);
    show(2000, NTS_FR_FLAGS_DEFAULT);
    show(1000000, NTS_FR_FLAGS_DEFAULT);
    show(1000001, NTS_FR_FLAGS_DEFAULT);
    show(-1, NTS_FR_FLAGS_DEFAULT);
    show(-9223372036854775807LL - 1LL, NTS_FR_FLAGS_DEFAULT);
    return 0;
}
