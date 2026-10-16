/*******************************************************************************
* djinterp [text]                                     test_number_to_string_it.c
*
*
* path:      /tests/djinterp/c/text/test_number_to_string_it.c
* link(s):   TBA
* author(s): TBA                                                    created: TBA
*                                                            revised: 2026.09.21
*******************************************************************************/
// std
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
// djinterp
#include "../../../../inc/djinterp/c/text/number_to_string_it.h"

static int check_cardinal(int64_t value, const char* expected)
{
    char buf[NTS_IT_I64_BUFFER_SIZE];
    size_t n = nts_it_cardinal_i64(buf, sizeof(buf), value, NTS_IT_FLAGS_DEFAULT);

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
    char buf[NTS_IT_I64_BUFFER_SIZE];
    size_t n = nts_it_ordinal_i64(buf, sizeof(buf), value, NTS_IT_FLAGS_DEFAULT);

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
    char buf[NTS_IT_I64_BUFFER_SIZE];
    size_t n = nts_it_ordinal_abbrev_i64(buf, sizeof(buf), value, flags);

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
    rc |= check_cardinal(21, "ventuno");
    rc |= check_cardinal(23, "ventitré");
    rc |= check_cardinal(28, "ventotto");
    rc |= check_cardinal(108, "centootto");
    rc |= check_cardinal(180, "centottanta");
    rc |= check_cardinal(1234, "milleduecentotrentaquattro");
    rc |= check_cardinal(12345, "dodicimilatrecentoquarantacinque");
    rc |= check_cardinal(1000000, "un milione");
    rc |= check_cardinal(2000000, "due milioni");
    rc |= check_cardinal(1000001, "un milione uno");

    rc |= check_ordinal(1, "primo");
    rc |= check_ordinal(10, "decimo");
    rc |= check_ordinal(21, "ventunesimo");
    rc |= check_ordinal(23, "ventitreesimo");
    rc |= check_ordinal(26, "ventiseiesimo");
    rc |= check_ordinal(108, "centoottesimo");
    rc |= check_ordinal(180, "centottantesimo");
    rc |= check_ordinal(200, "duecentesimo");
    rc |= check_ordinal(1000, "millesimo");
    rc |= check_ordinal(2000, "duemillesimo");
    rc |= check_ordinal(1234, "milleduecentotrentaquattresimo");
    rc |= check_ordinal(1000000, "milionesimo");
    rc |= check_ordinal(2000000, "duemilionesimo");
    rc |= check_ordinal(1000001, "un milione primo");
    rc |= check_ordinal(1234567, "un milione duecentotrentaquattromilacinquecentosessantasettesimo");

    rc |= check_ordinal_abbrev(1, NTS_IT_FLAGS_DEFAULT, "1°");
    rc |= check_ordinal_abbrev(1234, NTS_IT_FLAGS_DEFAULT, "1234°");
    rc |= check_ordinal_abbrev(1234, NTS_IT_FLAG_GROUP_DOTS, "1.234°");
    rc |= check_ordinal_abbrev(-1234, NTS_IT_FLAG_GROUP_DOTS, "meno 1.234°");

    if (rc == 0) {
        puts("All Italian number-to-string tests passed.");
    }

    return rc;
}
