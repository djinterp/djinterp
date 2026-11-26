/*******************************************************************************
* djinterp [c][text]                                 number_to_string_tests_fr.c
*
* The number_to_string suite's French block: number_to_string_fr.h's cardinals,
* ordinals and abbreviated ordinals, each a table of values, flags and the text
* they give, compared with what the function writes and the length it returns.
*
*
* path:      /tests/djinterp/c/text/number_to_string_tests_fr.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.23
*******************************************************************************/
#include "./number_to_string_tests.h"  // corresponding header


/*
d_tests_number_to_string_fr_cardinal
  nts_fr_cardinal_i64.
  Tests the following:
  - et un in the tens to soixante-dix; quatre-vingts and its s
  - cents and its s; mille, never pluralised
  - commas by flag; the long scale to INT64_MIN
*/
bool
d_tests_number_to_string_fr_cardinal(void)
{
    static const struct dj_tests_number_to_string_row rows[] =
    {
        { 0, NTS_FR_FLAGS_DEFAULT, "zéro" },
        { 1, NTS_FR_FLAGS_DEFAULT, "un" },
        { 2, NTS_FR_FLAGS_DEFAULT, "deux" },
        { 21, NTS_FR_FLAGS_DEFAULT, "vingt et un" },
        { 31, NTS_FR_FLAGS_DEFAULT, "trente et un" },
        { 71, NTS_FR_FLAGS_DEFAULT, "soixante et onze" },
        { 80, NTS_FR_FLAGS_DEFAULT, "quatre-vingts" },
        { 81, NTS_FR_FLAGS_DEFAULT, "quatre-vingt-un" },
        { 91, NTS_FR_FLAGS_DEFAULT, "quatre-vingt-onze" },
        { 100, NTS_FR_FLAGS_DEFAULT, "cent" },
        { 200, NTS_FR_FLAGS_DEFAULT, "deux cents" },
        { 201, NTS_FR_FLAGS_DEFAULT, "deux cent un" },
        { 1000, NTS_FR_FLAGS_DEFAULT, "mille" },
        { 1001, NTS_FR_FLAGS_DEFAULT, "mille un" },
        { 1234, NTS_FR_FLAG_COMMAS, "mille, deux cent trente quatre" },
        { 2000, NTS_FR_FLAGS_DEFAULT, "deux mille" },
        { 1000000, NTS_FR_FLAGS_DEFAULT, "un million" },
        { 1000001, NTS_FR_FLAGS_DEFAULT, "un million un" },
        { -1, NTS_FR_FLAGS_DEFAULT, "moins un" },
        { INT64_MIN, NTS_FR_FLAGS_DEFAULT,
          "moins neuf trillions deux cent vingt-trois billiards trois cent "
          "soixante-douze billions trente-six milliards huit cent "
          "cinquante-quatre millions sept cent soixante-quinze mille huit cent "
          "huit" }
    };

    return d_tests_number_to_string_rows(&nts_fr_cardinal_i64,
                                         rows,
                                         sizeof(rows) /
                                             sizeof(rows[0]));
}

/*
d_tests_number_to_string_fr_ordinal
  nts_fr_ordinal_i64.
  Tests the following:
  - premier, then unième in a compound
  - the ordinal suffix on the last word, its e dropped
*/
bool
d_tests_number_to_string_fr_ordinal(void)
{
    static const struct dj_tests_number_to_string_row rows[] =
    {
        { 0, NTS_FR_FLAGS_DEFAULT, "zéroième" },
        { 1, NTS_FR_FLAGS_DEFAULT, "premier" },
        { 2, NTS_FR_FLAGS_DEFAULT, "deuxième" },
        { 21, NTS_FR_FLAGS_DEFAULT, "vingt et unième" },
        { 31, NTS_FR_FLAGS_DEFAULT, "trente et unième" },
        { 71, NTS_FR_FLAGS_DEFAULT, "soixante et onzième" },
        { 80, NTS_FR_FLAGS_DEFAULT, "quatre-vingtième" },
        { 81, NTS_FR_FLAGS_DEFAULT, "quatre-vingt-unième" },
        { 91, NTS_FR_FLAGS_DEFAULT, "quatre-vingt-onzième" },
        { 100, NTS_FR_FLAGS_DEFAULT, "centième" },
        { 200, NTS_FR_FLAGS_DEFAULT, "deux centième" },
        { 201, NTS_FR_FLAGS_DEFAULT, "deux cent unième" },
        { 1000, NTS_FR_FLAGS_DEFAULT, "millième" },
        { 1001, NTS_FR_FLAGS_DEFAULT, "mille unième" },
        { 1234, NTS_FR_FLAG_COMMAS, "mille, deux cent trente quatrième" },
        { 2000, NTS_FR_FLAGS_DEFAULT, "deux millième" },
        { 1000000, NTS_FR_FLAGS_DEFAULT, "millionième" },
        { 1000001, NTS_FR_FLAGS_DEFAULT, "un million unième" },
        { -1, NTS_FR_FLAGS_DEFAULT, "moins premier" },
        { INT64_MIN, NTS_FR_FLAGS_DEFAULT,
          "moins neuf trillions deux cent vingt-trois billiards trois cent "
          "soixante-douze billions trente-six milliards huit cent "
          "cinquante-quatre millions sept cent soixante-quinze mille huit cent "
          "huitième" }
    };

    return d_tests_number_to_string_rows(&nts_fr_ordinal_i64,
                                         rows,
                                         sizeof(rows) /
                                             sizeof(rows[0]));
}

/*
d_tests_number_to_string_fr_abbrev
  nts_fr_ordinal_abbrev_i64.
  Tests the following:
  - 1er and every other value with e; the sign kept
  - commas by flag
*/
bool
d_tests_number_to_string_fr_abbrev(void)
{
    static const struct dj_tests_number_to_string_row rows[] =
    {
        { 0, NTS_FR_FLAGS_DEFAULT, "0e" },
        { 1, NTS_FR_FLAGS_DEFAULT, "1er" },
        { 2, NTS_FR_FLAGS_DEFAULT, "2e" },
        { 21, NTS_FR_FLAGS_DEFAULT, "21e" },
        { 31, NTS_FR_FLAGS_DEFAULT, "31e" },
        { 71, NTS_FR_FLAGS_DEFAULT, "71e" },
        { 80, NTS_FR_FLAGS_DEFAULT, "80e" },
        { 81, NTS_FR_FLAGS_DEFAULT, "81e" },
        { 91, NTS_FR_FLAGS_DEFAULT, "91e" },
        { 100, NTS_FR_FLAGS_DEFAULT, "100e" },
        { 200, NTS_FR_FLAGS_DEFAULT, "200e" },
        { 201, NTS_FR_FLAGS_DEFAULT, "201e" },
        { 1000, NTS_FR_FLAGS_DEFAULT, "1000e" },
        { 1001, NTS_FR_FLAGS_DEFAULT, "1001e" },
        { 1234, NTS_FR_FLAG_COMMAS, "1,234e" },
        { 2000, NTS_FR_FLAGS_DEFAULT, "2000e" },
        { 1000000, NTS_FR_FLAGS_DEFAULT, "1000000e" },
        { 1000001, NTS_FR_FLAGS_DEFAULT, "1000001e" },
        { -1, NTS_FR_FLAGS_DEFAULT, "-1er" },
        { INT64_MIN, NTS_FR_FLAGS_DEFAULT, "-9223372036854775808e" }
    };

    return d_tests_number_to_string_rows(&nts_fr_ordinal_abbrev_i64,
                                         rows,
                                         sizeof(rows) /
                                             sizeof(rows[0]));
}
