/*******************************************************************************
* djinterp [c][text]                                 number_to_string_tests_la.c
*
* The number_to_string suite's Latin block: number_to_string_la.h's cardinals,
* ordinals and abbreviated ordinals, each a table of values, flags and the text
* they give, compared with what the function writes and the length it returns.
*
*
* path:      /tests/djinterp/c/text/number_to_string_tests_la.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.23
*******************************************************************************/
#include "./number_to_string_tests.h"  // corresponding header


/*
d_tests_number_to_string_la_cardinal
  nts_la_cardinal_i64.
  Tests the following:
  - nihil for zero; et between the parts, by flag
  - thousands as milia, with commas by flag; minus for a minus
*/
bool
d_tests_number_to_string_la_cardinal(void)
{
    static const struct dj_tests_number_to_string_row rows[] =
    {
        { 0, NTS_LA_FLAGS_DEFAULT, "nihil" },
        { 21, NTS_LA_FLAGS_DEFAULT, "viginti unus" },
        { 21, NTS_LA_FLAG_ET, "viginti et unus" },
        { 101, NTS_LA_FLAGS_DEFAULT, "centum unus" },
        { 101, NTS_LA_FLAG_ET, "centum et unus" },
        { 1234, NTS_LA_FLAGS_DEFAULT, "mille ducenti triginta quattuor" },
        { 1234567, NTS_LA_FLAGS_DEFAULT,
          "mille ducenta triginta quattuor milia quingenti sexaginta septem" },
        { 1234567, NTS_LA_FLAG_COMMAS,
          "mille, ducenta triginta quattuor milia, quingenti sexaginta "
          "septem" },
        { -44, NTS_LA_FLAGS_DEFAULT, "minus quadraginta quattuor" }
    };

    return d_tests_number_to_string_rows(&nts_la_cardinal_i64,
                                         rows,
                                         sizeof(rows) /
                                             sizeof(rows[0]));
}

/*
d_tests_number_to_string_la_ordinal
  nts_la_ordinal_i64.
  Tests the following:
  - masculine ordinals: primus, vicesimus primus
  - bis millesimus and millies millesimus
*/
bool
d_tests_number_to_string_la_ordinal(void)
{
    static const struct dj_tests_number_to_string_row rows[] =
    {
        { 1, NTS_LA_FLAGS_DEFAULT, "primus" },
        { 21, NTS_LA_FLAGS_DEFAULT, "vicesimus primus" },
        { 115, NTS_LA_FLAGS_DEFAULT, "centesimus quintus decimus" },
        { 1000, NTS_LA_FLAGS_DEFAULT, "millesimus" },
        { 2000, NTS_LA_FLAGS_DEFAULT, "bis millesimus" },
        { 1234, NTS_LA_FLAGS_DEFAULT,
          "millesimus ducentesimus tricesimus quartus" },
        { 1000000, NTS_LA_FLAGS_DEFAULT, "millies millesimus" },
        { -9, NTS_LA_FLAGS_DEFAULT, "minus nonus" }
    };

    return d_tests_number_to_string_rows(&nts_la_ordinal_i64,
                                         rows,
                                         sizeof(rows) /
                                             sizeof(rows[0]));
}

/*
d_tests_number_to_string_la_abbrev
  nts_la_ordinal_abbrev_i64.
  Tests the following:
  - Roman numerals and a period, to MMMCMXCIX.; the sign kept
  - digits and a period for 0, and past 3999, to INT64_MIN
*/
bool
d_tests_number_to_string_la_abbrev(void)
{
    static const struct dj_tests_number_to_string_row rows[] =
    {
        { 0, NTS_LA_FLAGS_DEFAULT, "0." },
        { 1, NTS_LA_FLAGS_DEFAULT, "I." },
        { 21, NTS_LA_FLAGS_DEFAULT, "XXI." },
        { 2026, NTS_LA_FLAGS_DEFAULT, "MMXXVI." },
        { -44, NTS_LA_FLAGS_DEFAULT, "-XLIV." },
        { 3999, NTS_LA_FLAGS_DEFAULT, "MMMCMXCIX." },
        { 4000, NTS_LA_FLAGS_DEFAULT, "4000." },
        { INT64_MIN, NTS_LA_FLAGS_DEFAULT, "-9223372036854775808." }
    };

    return d_tests_number_to_string_rows(&nts_la_ordinal_abbrev_i64,
                                         rows,
                                         sizeof(rows) /
                                             sizeof(rows[0]));
}
