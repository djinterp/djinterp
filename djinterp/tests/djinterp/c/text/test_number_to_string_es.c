/*******************************************************************************
* djinterp [text]                                     test_number_to_string_es.c
*
*
* path:      /tests/djinterp/c/text/test_number_to_string_es.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <stdio.h>
// djinterp
#include "../../../../inc/djinterp/c/text/number_to_string_es.h"

static void test_cardinal(long long value)
{
    char buf[NTS_ES_I64_BUFFER_SIZE];
    nts_es_cardinal_i64(buf, sizeof(buf), (int64_t)value, NTS_ES_FLAGS_DEFAULT);
    printf("cardinal(%lld) = %s\n", value, buf);
}

static void test_ordinal(long long value)
{
    char buf[NTS_ES_I64_BUFFER_SIZE];
    nts_es_ordinal_i64(buf, sizeof(buf), (int64_t)value, NTS_ES_FLAGS_DEFAULT);
    printf("ordinal(%lld) = %s\n", value, buf);
}

static void test_ordinal_abbrev(long long value, unsigned flags)
{
    char buf[NTS_ES_I64_BUFFER_SIZE];
    nts_es_ordinal_abbrev_i64(buf, sizeof(buf), (int64_t)value, flags);
    printf("ordinal_abbrev(%lld) = %s\n", value, buf);
}

int main(void)
{
    test_cardinal(0);
    test_cardinal(21);
    test_cardinal(31);
    test_cardinal(121);
    test_cardinal(1000);
    test_cardinal(1001);
    test_cardinal(21000);
    test_cardinal(100000);
    test_cardinal(1000000);
    test_cardinal(2000000000LL);
    test_cardinal(-9223372036854775807LL - 1LL);

    putchar('\n');

    test_ordinal(1);
    test_ordinal(11);
    test_ordinal(21);
    test_ordinal(31);
    test_ordinal(101);
    test_ordinal(1248);
    test_ordinal(2000);
    test_ordinal(100000);
    test_ordinal(100001);
    test_ordinal(1000000);
    test_ordinal(1000001);

    putchar('\n');

    test_ordinal_abbrev(1, NTS_ES_FLAGS_DEFAULT);
    test_ordinal_abbrev(21, NTS_ES_FLAGS_DEFAULT);
    test_ordinal_abbrev(1234, NTS_ES_FLAGS_DEFAULT);
    test_ordinal_abbrev(1234, NTS_ES_FLAG_GROUP_DOTS);

    return 0;
}
