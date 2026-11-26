/*******************************************************************************
* djinterp [c][text]                                 number_to_string_tests_pt.c
*
* The number_to_string suite's Portuguese block: number_to_string_pt.h's
* cardinals, ordinals and abbreviated ordinals, each a table of values, flags
* and the text they give, compared with what the function writes and the length
* it returns.
*
*
* path:      /tests/djinterp/c/text/number_to_string_tests_pt.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.23
*******************************************************************************/
#include "./number_to_string_tests.h"  // corresponding header


/*
d_tests_number_to_string_pt_cardinal
  nts_pt_cardinal_i64.
  Tests the following:
  - cem alone and cento e before units; mil e cem
  - um milhão e mil; the short scale's bilhão
*/
bool
d_tests_number_to_string_pt_cardinal(void)
{
    static const struct dj_tests_number_to_string_row rows[] =
    {
        { 0, NTS_PT_FLAGS_DEFAULT, "zero" },
        { 21, NTS_PT_FLAGS_DEFAULT, "vinte e um" },
        { 100, NTS_PT_FLAGS_DEFAULT, "cem" },
        { 101, NTS_PT_FLAGS_DEFAULT, "cento e um" },
        { 1100, NTS_PT_FLAGS_DEFAULT, "mil e cem" },
        { 1101, NTS_PT_FLAGS_DEFAULT, "mil cento e um" },
        { 1234, NTS_PT_FLAGS_DEFAULT, "mil duzentos e trinta e quatro" },
        { 1000000, NTS_PT_FLAGS_DEFAULT, "um milhão" },
        { 2000000, NTS_PT_FLAGS_DEFAULT, "dois milhões" },
        { 1000001, NTS_PT_FLAGS_DEFAULT, "um milhão e um" },
        { 1001000, NTS_PT_FLAGS_DEFAULT, "um milhão e mil" },
        { 1000000000, NTS_PT_FLAGS_DEFAULT, "um bilhão" }
    };

    return d_tests_number_to_string_rows(&nts_pt_cardinal_i64,
                                         rows,
                                         sizeof(rows) /
                                             sizeof(rows[0]));
}

/*
d_tests_number_to_string_pt_ordinal
  nts_pt_ordinal_i64.
  Tests the following:
  - décimo primeiro; thousands as multiples: segundo milésimo
  - milionésimo and bilionésimo
*/
bool
d_tests_number_to_string_pt_ordinal(void)
{
    static const struct dj_tests_number_to_string_row rows[] =
    {
        { 1, NTS_PT_FLAGS_DEFAULT, "primeiro" },
        { 11, NTS_PT_FLAGS_DEFAULT, "décimo primeiro" },
        { 21, NTS_PT_FLAGS_DEFAULT, "vigésimo primeiro" },
        { 100, NTS_PT_FLAGS_DEFAULT, "centésimo" },
        { 101, NTS_PT_FLAGS_DEFAULT, "centésimo primeiro" },
        { 1000, NTS_PT_FLAGS_DEFAULT, "milésimo" },
        { 1001, NTS_PT_FLAGS_DEFAULT, "milésimo primeiro" },
        { 1100, NTS_PT_FLAGS_DEFAULT, "milésimo centésimo" },
        { 2000, NTS_PT_FLAGS_DEFAULT, "dois milésimo" },
        { 2300, NTS_PT_FLAGS_DEFAULT, "segundo milésimo trecentésimo" },
        { 4444, NTS_PT_FLAGS_DEFAULT,
          "quarto milésimo quadringentésimo quadragésimo quarto" },
        { 1000000, NTS_PT_FLAGS_DEFAULT, "milionésimo" },
        { 2000000, NTS_PT_FLAGS_DEFAULT, "dois milionésimo" },
        { 1000001, NTS_PT_FLAGS_DEFAULT, "milionésimo primeiro" },
        { 1000000000, NTS_PT_FLAGS_DEFAULT, "bilionésimo" }
    };

    return d_tests_number_to_string_rows(&nts_pt_ordinal_i64,
                                         rows,
                                         sizeof(rows) /
                                             sizeof(rows[0]));
}

/*
d_tests_number_to_string_pt_abbrev
  nts_pt_ordinal_abbrev_i64.
  Tests the following:
  - the masculine ordinal indicator after a period
  - grouping by dots, by flag; menos for a minus
*/
bool
d_tests_number_to_string_pt_abbrev(void)
{
    static const struct dj_tests_number_to_string_row rows[] =
    {
        { 1, NTS_PT_FLAGS_DEFAULT, "1.º" },
        { 1234, NTS_PT_FLAGS_DEFAULT, "1234.º" },
        { 1234, NTS_PT_FLAG_GROUP_DOTS, "1.234.º" },
        { -1234, NTS_PT_FLAG_GROUP_DOTS, "menos 1.234.º" }
    };

    return d_tests_number_to_string_rows(&nts_pt_ordinal_abbrev_i64,
                                         rows,
                                         sizeof(rows) /
                                             sizeof(rows[0]));
}
