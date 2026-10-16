/*******************************************************************************
* djinterp [text]                                     test_number_to_string_pt.c
*
*
* path:      /tests/djinterp/c/text/test_number_to_string_pt.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
// djinterp
#include "../../../../inc/djinterp/c/text/number_to_string_pt.h"

static int check_cardinal(int64_t value, const char* expected)
{
    char buf[NTS_PT_I64_BUFFER_SIZE];
    size_t n = nts_pt_cardinal_i64(buf, sizeof(buf), value, NTS_PT_FLAGS_DEFAULT);

    if (strcmp(buf, expected) != 0) {
        fprintf(stderr, "cardinal %" PRId64 "\n  got:      %s\n  expected: %s\n", value, buf, expected);
        return 1;
    }

    if (n != strlen(expected)) {
        fprintf(stderr, "cardinal length mismatch for %" PRId64 "\n", value);
        return 1;
    }

    return 0;
}

static int check_ordinal(int64_t value, const char* expected)
{
    char buf[NTS_PT_I64_BUFFER_SIZE];
    size_t n = nts_pt_ordinal_i64(buf, sizeof(buf), value, NTS_PT_FLAGS_DEFAULT);

    if (strcmp(buf, expected) != 0) {
        fprintf(stderr, "ordinal %" PRId64 "\n  got:      %s\n  expected: %s\n", value, buf, expected);
        return 1;
    }

    if (n != strlen(expected)) {
        fprintf(stderr, "ordinal length mismatch for %" PRId64 "\n", value);
        return 1;
    }

    return 0;
}

static int check_ordinal_abbrev(int64_t value, unsigned flags, const char* expected)
{
    char buf[NTS_PT_I64_BUFFER_SIZE];
    size_t n = nts_pt_ordinal_abbrev_i64(buf, sizeof(buf), value, flags);

    if (strcmp(buf, expected) != 0) {
        fprintf(stderr, "ordinal abbrev %" PRId64 "\n  got:      %s\n  expected: %s\n", value, buf, expected);
        return 1;
    }

    if (n != strlen(expected)) {
        fprintf(stderr, "ordinal abbrev length mismatch for %" PRId64 "\n", value);
        return 1;
    }

    return 0;
}

int main(void)
{
    int rc = 0;

    rc |= check_cardinal(0, "zero");
    rc |= check_cardinal(21, "vinte e um");
    rc |= check_cardinal(100, "cem");
    rc |= check_cardinal(101, "cento e um");
    rc |= check_cardinal(1100, "mil e cem");
    rc |= check_cardinal(1101, "mil cento e um");
    rc |= check_cardinal(1234, "mil duzentos e trinta e quatro");
    rc |= check_cardinal(1000000, "um milhão");
    rc |= check_cardinal(2000000, "dois milhões");
    rc |= check_cardinal(1000001, "um milhão e um");
    rc |= check_cardinal(1001000, "um milhão e mil");
    rc |= check_cardinal(1000000000LL, "um bilhão");

    rc |= check_ordinal(1, "primeiro");
    rc |= check_ordinal(11, "décimo primeiro");
    rc |= check_ordinal(21, "vigésimo primeiro");
    rc |= check_ordinal(100, "centésimo");
    rc |= check_ordinal(101, "centésimo primeiro");
    rc |= check_ordinal(1000, "milésimo");
    rc |= check_ordinal(1001, "milésimo primeiro");
    rc |= check_ordinal(1100, "milésimo centésimo");
    rc |= check_ordinal(2000, "dois milésimo");
    rc |= check_ordinal(2300, "segundo milésimo trecentésimo");
    rc |= check_ordinal(4444, "quarto milésimo quadringentésimo quadragésimo quarto");
    rc |= check_ordinal(1000000, "milionésimo");
    rc |= check_ordinal(2000000, "dois milionésimo");
    rc |= check_ordinal(1000001, "milionésimo primeiro");
    rc |= check_ordinal(1000000000LL, "bilionésimo");

    rc |= check_ordinal_abbrev(1, NTS_PT_FLAGS_DEFAULT, "1.º");
    rc |= check_ordinal_abbrev(1234, NTS_PT_FLAGS_DEFAULT, "1234.º");
    rc |= check_ordinal_abbrev(1234, NTS_PT_FLAG_GROUP_DOTS, "1.234.º");
    rc |= check_ordinal_abbrev(-1234, NTS_PT_FLAG_GROUP_DOTS, "menos 1.234.º");

    if (rc == 0) {
        puts("All Portuguese number-to-string tests passed.");
    }

    return rc;
}
