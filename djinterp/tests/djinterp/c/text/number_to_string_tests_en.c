/*******************************************************************************
* djinterp [c][text]                                 number_to_string_tests_en.c
*
* The number_to_string suite's English block: number_to_string_en.h's cardinals,
* ordinals and abbreviated ordinals, each a table of values, flags and the text
* they give, compared with what the function writes and the length it returns.
*
*
* path:      /tests/djinterp/c/text/number_to_string_tests_en.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.15
*                                                            revised: 2026.11.23
*******************************************************************************/
#include "./number_to_string_tests.h"  // corresponding header


/*
d_tests_number_to_string_en_cardinal
  nts_cardinal_i64.
  Tests the following:
  - zero; the tens joined by a hyphen; hundreds with "and"
  - thousands and millions, with and without commas
  - the flags: without hyphens, and without any
  - a negative value, and INT64_MIN, whose magnitude no int64_t holds
*/
bool
d_tests_number_to_string_en_cardinal(void)
{
    static const struct dj_tests_number_to_string_row rows[] =
    {
        { 0, NTS_FLAGS_DEFAULT, "zero" },
        { 21, NTS_FLAGS_DEFAULT, "twenty-one" },
        { 21, NTS_FLAG_COMMAS | NTS_FLAG_AND, "twenty one" },
        { 101, NTS_FLAGS_DEFAULT, "one hundred and one" },
        { 1001, NTS_FLAGS_DEFAULT, "one thousand and one" },
        { 1234, NTS_FLAGS_DEFAULT,
          "one thousand, two hundred and thirty-four" },
        { 1234, NTS_FLAG_NONE, "one thousand two hundred thirty four" },
        { 1000000, NTS_FLAGS_DEFAULT, "one million" },
        { 1000001, NTS_FLAGS_DEFAULT, "one million and one" },
        { -1, NTS_FLAGS_DEFAULT, "negative one" },
        { INT64_MIN, NTS_FLAGS_DEFAULT,
          "negative nine quintillion, two hundred and twenty-three "
          "quadrillion, three hundred and seventy-two trillion, thirty-six "
          "billion, eight hundred and fifty-four million, seven hundred and "
          "seventy-five thousand, eight hundred and eight" }
    };

    return d_tests_number_to_string_rows(&nts_cardinal_i64,
                                         rows,
                                         sizeof(rows) /
                                             sizeof(rows[0]));
}

/*
d_tests_number_to_string_en_ordinal
  nts_ordinal_i64.
  Tests the following:
  - the last word made ordinal: zeroth, twenty-first, millionth
  - the same values and flags as the cardinals
*/
bool
d_tests_number_to_string_en_ordinal(void)
{
    static const struct dj_tests_number_to_string_row rows[] =
    {
        { 0, NTS_FLAGS_DEFAULT, "zeroth" },
        { 21, NTS_FLAGS_DEFAULT, "twenty-first" },
        { 21, NTS_FLAG_COMMAS | NTS_FLAG_AND, "twenty first" },
        { 101, NTS_FLAGS_DEFAULT, "one hundred and first" },
        { 1001, NTS_FLAGS_DEFAULT, "one thousand and first" },
        { 1234, NTS_FLAGS_DEFAULT,
          "one thousand, two hundred and thirty-fourth" },
        { 1234, NTS_FLAG_NONE, "one thousand two hundred thirty fourth" },
        { 1000000, NTS_FLAGS_DEFAULT, "one millionth" },
        { 1000001, NTS_FLAGS_DEFAULT, "one million and first" },
        { -1, NTS_FLAGS_DEFAULT, "negative first" },
        { INT64_MIN, NTS_FLAGS_DEFAULT,
          "negative nine quintillion, two hundred and twenty-three "
          "quadrillion, three hundred and seventy-two trillion, thirty-six "
          "billion, eight hundred and fifty-four million, seven hundred and "
          "seventy-five thousand, eight hundred and eighth" }
    };

    return d_tests_number_to_string_rows(&nts_ordinal_i64,
                                         rows,
                                         sizeof(rows) /
                                             sizeof(rows[0]));
}

/*
d_tests_number_to_string_en_abbrev
  nts_ordinal_abbrev_i64.
  Tests the following:
  - st, nd, rd and th by the last two digits; the grouping commas
  - the sign kept on a negative value
*/
bool
d_tests_number_to_string_en_abbrev(void)
{
    static const struct dj_tests_number_to_string_row rows[] =
    {
        { 0, NTS_FLAGS_DEFAULT, "0th" },
        { 21, NTS_FLAGS_DEFAULT, "21st" },
        { 21, NTS_FLAG_COMMAS | NTS_FLAG_AND, "21st" },
        { 101, NTS_FLAGS_DEFAULT, "101st" },
        { 1001, NTS_FLAGS_DEFAULT, "1,001st" },
        { 1234, NTS_FLAGS_DEFAULT, "1,234th" },
        { 1234, NTS_FLAG_NONE, "1234th" },
        { 1000000, NTS_FLAGS_DEFAULT, "1,000,000th" },
        { 1000001, NTS_FLAGS_DEFAULT, "1,000,001st" },
        { -1, NTS_FLAGS_DEFAULT, "-1st" },
        { INT64_MIN, NTS_FLAGS_DEFAULT, "-9,223,372,036,854,775,808th" }
    };

    return d_tests_number_to_string_rows(&nts_ordinal_abbrev_i64,
                                         rows,
                                         sizeof(rows) /
                                             sizeof(rows[0]));
}
