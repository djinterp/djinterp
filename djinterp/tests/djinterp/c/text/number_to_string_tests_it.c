/*******************************************************************************
* djinterp [c][text]                                 number_to_string_tests_it.c
*
* The number_to_string suite's Italian block: number_to_string_it.h's cardinals,
* ordinals and abbreviated ordinals, each a table of values, flags and the text
* they give, compared with what the function writes and the length it returns.
*
*
* path:      /tests/djinterp/c/text/number_to_string_tests_it.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.23
*******************************************************************************/
#include "./number_to_string_tests.h"  // corresponding header


/*
d_tests_number_to_string_it_cardinal
  nts_it_cardinal_i64.
  Tests the following:
  - a vowel elided before uno and otto: ventuno, ventotto
  - one word below a million; un milione, due milioni
*/
bool
d_tests_number_to_string_it_cardinal(void)
{
    static const struct dj_tests_number_to_string_row rows[] =
    {
        { 0, NTS_IT_FLAGS_DEFAULT, "zero" },
        { 21, NTS_IT_FLAGS_DEFAULT, "ventuno" },
        { 23, NTS_IT_FLAGS_DEFAULT, "ventitré" },
        { 28, NTS_IT_FLAGS_DEFAULT, "ventotto" },
        { 108, NTS_IT_FLAGS_DEFAULT, "centootto" },
        { 180, NTS_IT_FLAGS_DEFAULT, "centottanta" },
        { 1234, NTS_IT_FLAGS_DEFAULT, "milleduecentotrentaquattro" },
        { 12345, NTS_IT_FLAGS_DEFAULT, "dodicimilatrecentoquarantacinque" },
        { 1000000, NTS_IT_FLAGS_DEFAULT, "un milione" },
        { 2000000, NTS_IT_FLAGS_DEFAULT, "due milioni" },
        { 1000001, NTS_IT_FLAGS_DEFAULT, "un milione uno" }
    };

    return d_tests_number_to_string_rows(&nts_it_cardinal_i64,
                                         rows,
                                         sizeof(rows) /
                                             sizeof(rows[0]));
}

/*
d_tests_number_to_string_it_ordinal
  nts_it_ordinal_i64.
  Tests the following:
  - primo to decimo, then -esimo on the last word
  - the stress mark dropped and the vowel kept: ventitreesimo
*/
bool
d_tests_number_to_string_it_ordinal(void)
{
    static const struct dj_tests_number_to_string_row rows[] =
    {
        { 1, NTS_IT_FLAGS_DEFAULT, "primo" },
        { 10, NTS_IT_FLAGS_DEFAULT, "decimo" },
        { 21, NTS_IT_FLAGS_DEFAULT, "ventunesimo" },
        { 23, NTS_IT_FLAGS_DEFAULT, "ventitreesimo" },
        { 26, NTS_IT_FLAGS_DEFAULT, "ventiseiesimo" },
        { 108, NTS_IT_FLAGS_DEFAULT, "centoottesimo" },
        { 180, NTS_IT_FLAGS_DEFAULT, "centottantesimo" },
        { 200, NTS_IT_FLAGS_DEFAULT, "duecentesimo" },
        { 1000, NTS_IT_FLAGS_DEFAULT, "millesimo" },
        { 2000, NTS_IT_FLAGS_DEFAULT, "duemillesimo" },
        { 1234, NTS_IT_FLAGS_DEFAULT, "milleduecentotrentaquattresimo" },
        { 1000000, NTS_IT_FLAGS_DEFAULT, "milionesimo" },
        { 2000000, NTS_IT_FLAGS_DEFAULT, "duemilionesimo" },
        { 1000001, NTS_IT_FLAGS_DEFAULT, "un milione primo" },
        { 1234567, NTS_IT_FLAGS_DEFAULT,
          "un milione duecentotrentaquattromilacinquecentosessantasettesimo" }
    };

    return d_tests_number_to_string_rows(&nts_it_ordinal_i64,
                                         rows,
                                         sizeof(rows) /
                                             sizeof(rows[0]));
}

/*
d_tests_number_to_string_it_abbrev
  nts_it_ordinal_abbrev_i64.
  Tests the following:
  - the degree sign; grouping by dots, by flag; meno for a minus
*/
bool
d_tests_number_to_string_it_abbrev(void)
{
    static const struct dj_tests_number_to_string_row rows[] =
    {
        { 1, NTS_IT_FLAGS_DEFAULT, "1°" },
        { 1234, NTS_IT_FLAGS_DEFAULT, "1234°" },
        { 1234, NTS_IT_FLAG_GROUP_DOTS, "1.234°" },
        { -1234, NTS_IT_FLAG_GROUP_DOTS, "meno 1.234°" }
    };

    return d_tests_number_to_string_rows(&nts_it_ordinal_abbrev_i64,
                                         rows,
                                         sizeof(rows) /
                                             sizeof(rows[0]));
}
