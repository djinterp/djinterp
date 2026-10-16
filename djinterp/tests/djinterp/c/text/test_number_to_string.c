/*******************************************************************************
* djinterp [text]                                        test_number_to_string.c
*
*
* path:      /tests/djinterp/c/text/test_number_to_string.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <stdio.h>
// djinterp

static void show(int64_t value, unsigned flags)
{
    char buf[NTS_I64_BUFFER_SIZE];

    nts_cardinal_i64(buf, sizeof(buf), value, flags);
    printf("cardinal(%lld) = %s\n", (long long)value, buf);

    nts_ordinal_i64(buf, sizeof(buf), value, flags);
    printf("ordinal (%lld) = %s\n", (long long)value, buf);

    nts_ordinal_abbrev_i64(buf, sizeof(buf), value, flags);
    printf("abbr    (%lld) = %s\n", (long long)value, buf);
}

int main(void)
{
    show(0, NTS_FLAGS_DEFAULT);
    show(21, NTS_FLAGS_DEFAULT);
    show(21, NTS_FLAG_COMMAS | NTS_FLAG_AND);
    show(101, NTS_FLAGS_DEFAULT);
    show(1001, NTS_FLAGS_DEFAULT);
    show(1234, NTS_FLAGS_DEFAULT);
    show(1000000, NTS_FLAGS_DEFAULT);
    show(1000001, NTS_FLAGS_DEFAULT);
    show(-1, NTS_FLAGS_DEFAULT);
    show(-9223372036854775807LL - 1LL, NTS_FLAGS_DEFAULT);
    return 0;
}
