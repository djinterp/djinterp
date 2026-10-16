/*******************************************************************************
* djinterp [text]                                     test_number_to_string_la.c
*
*
* path:      /tests/djinterp/c/text/test_number_to_string_la.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <stdio.h>
// djinterp
#include "../../../../inc/djinterp/c/text/number_to_string_la.h"

static void print_cardinal(int64_t value, unsigned flags)
{
    char buf[NTS_LA_I64_BUFFER_SIZE];
    nts_la_cardinal_i64(buf, sizeof(buf), value, flags);
    printf("cardinal(%lld) = %s\n", (long long)value, buf);
}

static void print_ordinal(int64_t value)
{
    char buf[NTS_LA_I64_BUFFER_SIZE];
    nts_la_ordinal_i64(buf, sizeof(buf), value, NTS_LA_FLAGS_DEFAULT);
    printf("ordinal(%lld)  = %s\n", (long long)value, buf);
}

static void print_abbrev(int64_t value)
{
    char buf[NTS_LA_I64_BUFFER_SIZE];
    nts_la_ordinal_abbrev_i64(buf, sizeof(buf), value, NTS_LA_FLAGS_DEFAULT);
    printf("abbrev(%lld)   = %s\n", (long long)value, buf);
}

int main(void)
{
    puts("-- cardinals --");
    print_cardinal(0, NTS_LA_FLAGS_DEFAULT);
    print_cardinal(21, NTS_LA_FLAGS_DEFAULT);
    print_cardinal(21, NTS_LA_FLAG_ET);
    print_cardinal(101, NTS_LA_FLAGS_DEFAULT);
    print_cardinal(101, NTS_LA_FLAG_ET);
    print_cardinal(1234, NTS_LA_FLAGS_DEFAULT);
    print_cardinal(1234567, NTS_LA_FLAGS_DEFAULT);
    print_cardinal(1234567, NTS_LA_FLAG_COMMAS);
    print_cardinal(-44, NTS_LA_FLAGS_DEFAULT);

    puts("\n-- ordinals --");
    print_ordinal(1);
    print_ordinal(21);
    print_ordinal(115);
    print_ordinal(1000);
    print_ordinal(2000);
    print_ordinal(1234);
    print_ordinal(1000000);
    print_ordinal(-9);

    puts("\n-- abbreviated ordinals --");
    print_abbrev(0);
    print_abbrev(1);
    print_abbrev(21);
    print_abbrev(2026);
    print_abbrev(-44);

    return 0;
}
