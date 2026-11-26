/*******************************************************************************
* djinterp [c][text]                                 number_to_string_tests_es.c
*
* The number_to_string suite's Spanish block: number_to_string_es.h's cardinals,
* ordinals and abbreviated ordinals, each a table of values, flags and the text
* they give, compared with what the function writes and the length it returns.
*
*
* path:      /tests/djinterp/c/text/number_to_string_tests_es.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.23
*******************************************************************************/
#include "./number_to_string_tests.h"  // corresponding header


/*
d_tests_number_to_string_es_cardinal
  nts_es_cardinal_i64.
  Tests the following:
  - veintiuno, and veintiún before mil
  - mil uno, cien mil, dos mil millones: the long scale
  - INT64_MIN, in trillones and billones
*/
bool
d_tests_number_to_string_es_cardinal(void)
{
    static const struct dj_tests_number_to_string_row rows[] =
    {
        { 0, NTS_ES_FLAGS_DEFAULT, "cero" },
        { 21, NTS_ES_FLAGS_DEFAULT, "veintiuno" },
        { 31, NTS_ES_FLAGS_DEFAULT, "treinta y uno" },
        { 121, NTS_ES_FLAGS_DEFAULT, "ciento veintiuno" },
        { 1000, NTS_ES_FLAGS_DEFAULT, "mil" },
        { 1001, NTS_ES_FLAGS_DEFAULT, "mil uno" },
        { 21000, NTS_ES_FLAGS_DEFAULT, "veintiún mil" },
        { 100000, NTS_ES_FLAGS_DEFAULT, "cien mil" },
        { 1000000, NTS_ES_FLAGS_DEFAULT, "un millón" },
        { 2000000000, NTS_ES_FLAGS_DEFAULT, "dos mil millones" },
        { INT64_MIN, NTS_ES_FLAGS_DEFAULT,
          "menos nueve trillones doscientos veintitrés mil billones "
          "trescientos setenta y dos billones treinta y seis mil millones "
          "ochocientos cincuenta y cuatro millones setecientos setenta y cinco "
          "mil ochocientos ocho" }
    };

    return d_tests_number_to_string_rows(&nts_es_cardinal_i64,
                                         rows,
                                         sizeof(rows) /
                                             sizeof(rows[0]));
}

/*
d_tests_number_to_string_es_ordinal
  nts_es_ordinal_i64.
  Tests the following:
  - primero and undécimo; tens and units as two words
  - one word for a thousand's multiple: dosmilésimo, cienmilésimo
*/
bool
d_tests_number_to_string_es_ordinal(void)
{
    static const struct dj_tests_number_to_string_row rows[] =
    {
        { 1, NTS_ES_FLAGS_DEFAULT, "primero" },
        { 11, NTS_ES_FLAGS_DEFAULT, "undécimo" },
        { 21, NTS_ES_FLAGS_DEFAULT, "vigésimo primero" },
        { 31, NTS_ES_FLAGS_DEFAULT, "trigésimo primero" },
        { 101, NTS_ES_FLAGS_DEFAULT, "centésimo primero" },
        { 1248, NTS_ES_FLAGS_DEFAULT,
          "milésimo ducentésimo cuadragésimo octavo" },
        { 2000, NTS_ES_FLAGS_DEFAULT, "dosmilésimo" },
        { 100000, NTS_ES_FLAGS_DEFAULT, "cienmilésimo" },
        { 100001, NTS_ES_FLAGS_DEFAULT, "cienmilésimo primero" },
        { 1000000, NTS_ES_FLAGS_DEFAULT, "millonésimo" },
        { 1000001, NTS_ES_FLAGS_DEFAULT, "millonésimo primero" }
    };

    return d_tests_number_to_string_rows(&nts_es_ordinal_i64,
                                         rows,
                                         sizeof(rows) /
                                             sizeof(rows[0]));
}

/*
d_tests_number_to_string_es_abbrev
  nts_es_ordinal_abbrev_i64.
  Tests the following:
  - the masculine ordinal indicator after a period
  - grouping by dots, by flag
*/
bool
d_tests_number_to_string_es_abbrev(void)
{
    static const struct dj_tests_number_to_string_row rows[] =
    {
        { 1, NTS_ES_FLAGS_DEFAULT, "1.º" },
        { 21, NTS_ES_FLAGS_DEFAULT, "21.º" },
        { 1234, NTS_ES_FLAGS_DEFAULT, "1234.º" },
        { 1234, NTS_ES_FLAG_GROUP_DOTS, "1.234.º" }
    };

    return d_tests_number_to_string_rows(&nts_es_ordinal_abbrev_i64,
                                         rows,
                                         sizeof(rows) /
                                             sizeof(rows[0]));
}
