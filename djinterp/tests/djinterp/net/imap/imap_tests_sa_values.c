/*******************************************************************************
* djinterp [net]                                          imap_tests_sa_values.c
*
* Tests of the IMAP kernel's sequence sets and dates.
*   Sequence sets are written and parsed back, measured, refused where RFC
* 9051's grammar refuses them, and asked about membership with "*" resolved
* to the largest number in use. Dates and date-times are converted to and
* from Unix time across the whole of 0000 to 9999, leap days and a leap
* second included, and written and parsed in both of IMAP's forms.
*
*
* path:      /tests/djinterp/net/imap/imap_tests_sa_values.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.18
*                                                            revised: 2026.10.18
*******************************************************************************/
#include "./imap_tests_sa.h"  // the suite


// d_tests_imap_seqset_case
//   struct: a sequence set as text and as ranges, and the error parsing the
// text gives.
struct d_tests_imap_seqset_case
{
    const char*             text;
    enum d_imap_error       error;
    size_t                  count;
    struct d_imap_seq_range ranges[3];
    const char*             what;
};

// d_tests_imap_unix_case
//   struct: a Unix time and zone, and the error and local time converting
// them gives.
struct d_tests_imap_unix_case
{
    int64_t                seconds;
    int32_t                zone;
    enum d_imap_error      error;
    struct d_imap_datetime datetime;
    const char*            what;
};

// d_tests_imap_render_case
//   struct: a local time, its Unix time or the error converting it gives,
// and its SEARCH date and date-time, NULL where writing them is refused.
struct d_tests_imap_render_case
{
    struct d_imap_datetime datetime;
    enum d_imap_error      error;
    int64_t                seconds;
    const char*            date;
    const char*            date_time;
    const char*            what;
};

// d_tests_imap_parse_case
//   struct: a date, or with `timed` a date-time, and the error and local
// time parsing it gives.
struct d_tests_imap_parse_case
{
    const char*            text;
    bool                   timed;
    enum d_imap_error      error;
    struct d_imap_datetime datetime;
    const char*            what;
};

/*
ROUND_TRIPS, SET_PARSES
  Sets that parse to their ranges and are written back the same, then sets
that only parse, or are refused.
*/
static const struct d_tests_imap_seqset_case ROUND_TRIPS[] =
{
    { "1:4,7,9:*", D_IMAP_OK, 3u,
      { { 1u, 4u }, { 7u, 7u }, { 9u, D_IMAP_SEQ_STAR } },
      "RFC 9051's example" },
    { "4:2", D_IMAP_OK, 1u, { { 4u, 2u } },
      "a range kept as given" },
    { "*", D_IMAP_OK, 1u, { { D_IMAP_SEQ_STAR, D_IMAP_SEQ_STAR } },
      "the largest number in use" },
    { "*:5", D_IMAP_OK, 1u, { { D_IMAP_SEQ_STAR, 5u } },
      "a range from it" },
    { "4294967295", D_IMAP_OK, 1u, { { 4294967295u, 4294967295u } },
      "the largest number" }
};
static const struct d_tests_imap_seqset_case SET_PARSES[] =
{
    { "007", D_IMAP_OK, 1u, { { 7u, 7u } }, "leading zeros are taken" },
    { "0", D_IMAP_ERROR_RANGE, 0u, { { 0u, 0u } }, "0 is no number" },
    { "00", D_IMAP_ERROR_RANGE, 0u, { { 0u, 0u } }, "nor written longer" },
    { "1:0", D_IMAP_ERROR_RANGE, 0u, { { 0u, 0u } }, "nor ending a range" },
    { "4294967296", D_IMAP_ERROR_RANGE, 0u, { { 0u, 0u } },
      "nor is a number past 32 bits" },
    { "", D_IMAP_ERROR_MALFORMED, 0u, { { 0u, 0u } }, "an empty set" },
    { "1,,2", D_IMAP_ERROR_MALFORMED, 0u, { { 0u, 0u } }, "an empty element" },
    { "1:", D_IMAP_ERROR_MALFORMED, 0u, { { 0u, 0u } }, "a range unended" },
    { ":1", D_IMAP_ERROR_MALFORMED, 0u, { { 0u, 0u } }, "or unbegun" },
    { "1:2:3", D_IMAP_ERROR_MALFORMED, 0u, { { 0u, 0u } }, "three bounds" },
    { "1 2", D_IMAP_ERROR_MALFORMED, 0u, { { 0u, 0u } }, "a space" },
    { ",1", D_IMAP_ERROR_MALFORMED, 0u, { { 0u, 0u } }, "a leading comma" },
    { "1,", D_IMAP_ERROR_MALFORMED, 0u, { { 0u, 0u } }, "a trailing one" },
    { "**", D_IMAP_ERROR_MALFORMED, 0u, { { 0u, 0u } }, "two stars" },
    { "$", D_IMAP_ERROR_MALFORMED, 0u, { { 0u, 0u } },
      "the saved result, which is not represented" }
};

/*
FROM_UNIX
  The epoch and around it, a zone either side, the first and last seconds
of 0000 to 9999, and what falls outside them.
*/
static const struct d_tests_imap_unix_case FROM_UNIX[] =
{
    { 0, 0, D_IMAP_OK, { 1970, 1, 1, 0, 0, 0, 0 },
      "the epoch" },
    { 1000000000, -300, D_IMAP_OK, { 2001, 9, 8, 20, 46, 40, -300 },
      "a billion seconds, five hours west" },
    { -1, 0, D_IMAP_OK, { 1969, 12, 31, 23, 59, 59, 0 },
      "a second before the epoch" },
    { 951782400, 1439, D_IMAP_OK, { 2000, 2, 29, 23, 59, 0, 1439 },
      "a leap day, 23:59 east" },
    { INT64_C(253402300799), 0, D_IMAP_OK, { 9999, 12, 31, 23, 59, 59, 0 },
      "the last second of 9999" },
    { INT64_C(-62167219200), 0, D_IMAP_OK, { 0, 1, 1, 0, 0, 0, 0 },
      "the first of 0000" },
    { INT64_C(253402300800), 0, D_IMAP_ERROR_RANGE, { 0, 0, 0, 0, 0, 0, 0 },
      "10000 is out of range" },
    { INT64_C(-62167219201), 0, D_IMAP_ERROR_RANGE, { 0, 0, 0, 0, 0, 0, 0 },
      "and so is before 0000" },
    { 0, 1440, D_IMAP_ERROR_RANGE, { 0, 0, 0, 0, 0, 0, 0 },
      "a zone of a day east" },
    { 0, -1440, D_IMAP_ERROR_RANGE, { 0, 0, 0, 0, 0, 0, 0 },
      "or west" }
};

/*
RENDERINGS
  Local times converted to Unix time and written: the day is unpadded in a
SEARCH date and space-padded in a date-time, a leap second is the next
minute's first, and impossible fields are refused.
*/
static const struct d_tests_imap_render_case RENDERINGS[] =
{
    { { 2026, 2, 1, 9, 5, 7, -300 }, D_IMAP_OK, INT64_C(1769954707),
      "1-Feb-2026", "\" 1-Feb-2026 09:05:07 -0500\"",
      "a morning five hours west" },
    { { 2001, 9, 8, 20, 46, 40, -300 }, D_IMAP_OK, 1000000000,
      "8-Sep-2001", "\" 8-Sep-2001 20:46:40 -0500\"",
      "a billion seconds" },
    { { 2016, 12, 31, 23, 59, 60, 0 }, D_IMAP_OK, INT64_C(1483228800),
      "31-Dec-2016", "\"31-Dec-2016 23:59:60 +0000\"",
      "a leap second is the next minute's first" },
    { { 2026, 1, 31, 0, 0, 0, 330 }, D_IMAP_OK, INT64_C(1769797800),
      "31-Jan-2026", "\"31-Jan-2026 00:00:00 +0530\"",
      "a zone east of UTC" },
    { { 2024, 2, 29, 12, 0, 0, 0 }, D_IMAP_OK, INT64_C(1709208000),
      "29-Feb-2024", "\"29-Feb-2024 12:00:00 +0000\"",
      "a leap day" },
    { { 2000, 2, 29, 0, 0, 0, 0 }, D_IMAP_OK, 951782400,
      "29-Feb-2000", "\"29-Feb-2000 00:00:00 +0000\"",
      "2000 was a leap year" },
    { { 0, 1, 1, 0, 0, 0, 0 }, D_IMAP_OK, INT64_C(-62167219200),
      "1-Jan-0000", "\" 1-Jan-0000 00:00:00 +0000\"",
      "the first of 0000" },
    { { 9999, 12, 31, 23, 59, 59, 0 }, D_IMAP_OK, INT64_C(253402300799),
      "31-Dec-9999", "\"31-Dec-9999 23:59:59 +0000\"",
      "the last of 9999" },
    { { 2026, 2, 29, 0, 0, 0, 0 }, D_IMAP_ERROR_RANGE, 0, NULL, NULL,
      "2026 is no leap year" },
    { { 1900, 2, 29, 0, 0, 0, 0 }, D_IMAP_ERROR_RANGE, 0, NULL, NULL,
      "nor was 1900" },
    { { 2026, 4, 31, 0, 0, 0, 0 }, D_IMAP_ERROR_RANGE, 0, NULL, NULL,
      "April has 30 days" },
    { { 2026, 13, 1, 0, 0, 0, 0 }, D_IMAP_ERROR_RANGE, 0, NULL, NULL,
      "a year has 12 months" },
    { { 10000, 1, 1, 0, 0, 0, 0 }, D_IMAP_ERROR_RANGE, 0, NULL, NULL,
      "a year has four digits" },
    { { -1, 1, 1, 0, 0, 0, 0 }, D_IMAP_ERROR_RANGE, 0, NULL, NULL,
      "and no sign" },
    { { 2026, 1, 1, 24, 0, 0, 0 }, D_IMAP_ERROR_RANGE, 0, "1-Jan-2026", NULL,
      "no hour 24, which a SEARCH date ignores" },
    { { 2026, 1, 1, 0, 60, 0, 0 }, D_IMAP_ERROR_RANGE, 0, "1-Jan-2026", NULL,
      "no minute 60" },
    { { 2026, 1, 1, 0, 0, 61, 0 }, D_IMAP_ERROR_RANGE, 0, "1-Jan-2026", NULL,
      "no second 61" },
    { { 2026, 1, 1, 0, 0, 0, 1440 }, D_IMAP_ERROR_RANGE, 0, "1-Jan-2026", NULL,
      "no zone of a whole day" }
};

/*
PARSES
  SEARCH dates, quoted or not, with one or two digits of day, the month in
any case; date-times, quoted or not, the day space-padded; RFC 3501's
example; and what neither form allows.
*/
static const struct d_tests_imap_parse_case PARSES[] =
{
    { "1-Feb-2026", false, D_IMAP_OK, { 2026, 2, 1, 0, 0, 0, 0 },
      "a SEARCH date" },
    { "\"01-Feb-2026\"", false, D_IMAP_OK, { 2026, 2, 1, 0, 0, 0, 0 },
      "quoted, with two digits of day" },
    { "1-feb-2026", false, D_IMAP_OK, { 2026, 2, 1, 0, 0, 0, 0 },
      "the month in lower case" },
    { "1-FEB-2026", false, D_IMAP_OK, { 2026, 2, 1, 0, 0, 0, 0 },
      "or upper" },
    { "29-Feb-2024", false, D_IMAP_OK, { 2024, 2, 29, 0, 0, 0, 0 },
      "a leap day" },
    { "1-Jan-0000", false, D_IMAP_OK, { 0, 1, 1, 0, 0, 0, 0 },
      "the first year" },
    { "31-Dec-9999", false, D_IMAP_OK, { 9999, 12, 31, 0, 0, 0, 0 },
      "the last" },
    { "31-Feb-2026", false, D_IMAP_ERROR_RANGE, { 0, 0, 0, 0, 0, 0, 0 },
      "February has no 31st" },
    { "29-Feb-2025", false, D_IMAP_ERROR_RANGE, { 0, 0, 0, 0, 0, 0, 0 },
      "nor a 29th in 2025" },
    { "0-Feb-2026", false, D_IMAP_ERROR_RANGE, { 0, 0, 0, 0, 0, 0, 0 },
      "nor a 0th" },
    { "1-Foo-2026", false, D_IMAP_ERROR_MALFORMED, { 0, 0, 0, 0, 0, 0, 0 },
      "no such month" },
    { "1-Feb-26", false, D_IMAP_ERROR_MALFORMED, { 0, 0, 0, 0, 0, 0, 0 },
      "a year of two digits" },
    { "001-Feb-2026", false, D_IMAP_ERROR_MALFORMED, { 0, 0, 0, 0, 0, 0, 0 },
      "a day of three" },
    { " 1-Feb-2026", false, D_IMAP_ERROR_MALFORMED, { 0, 0, 0, 0, 0, 0, 0 },
      "a SEARCH date takes no padding" },
    { "1-Feb-2026x", false, D_IMAP_ERROR_MALFORMED, { 0, 0, 0, 0, 0, 0, 0 },
      "nor anything after it" },
    { "\"1-Feb-2026", false, D_IMAP_ERROR_MALFORMED, { 0, 0, 0, 0, 0, 0, 0 },
      "an unclosed quote" },
    { "\" 1-Feb-2026 09:05:07 -0500\"", true, D_IMAP_OK,
      { 2026, 2, 1, 9, 5, 7, -300 }, "a quoted date-time" },
    { "01-Feb-2026 09:05:07 +0000", true, D_IMAP_OK,
      { 2026, 2, 1, 9, 5, 7, 0 }, "unquoted, two digits of day" },
    { "\"17-Jul-1996 02:44:25 -0700\"", true, D_IMAP_OK,
      { 1996, 7, 17, 2, 44, 25, -420 }, "RFC 3501's example" },
    { " 1-Feb-2026 09:05:07 +2359", true, D_IMAP_OK,
      { 2026, 2, 1, 9, 5, 7, 1439 }, "the widest zone" },
    { "31-Dec-2016 23:59:60 +0000", true, D_IMAP_OK,
      { 2016, 12, 31, 23, 59, 60, 0 }, "a leap second" },
    { " 1-Feb-2026 24:00:00 +0000", true, D_IMAP_ERROR_RANGE,
      { 0, 0, 0, 0, 0, 0, 0 }, "no hour 24" },
    { " 1-Feb-2026 09:05:61 +0000", true, D_IMAP_ERROR_RANGE,
      { 0, 0, 0, 0, 0, 0, 0 }, "no second 61" },
    { " 1-Feb-2026 09:05:07 +0060", true, D_IMAP_ERROR_RANGE,
      { 0, 0, 0, 0, 0, 0, 0 }, "no zone of 60 minutes" },
    { "30-Feb-2026 09:05:07 +0000", true, D_IMAP_ERROR_RANGE,
      { 0, 0, 0, 0, 0, 0, 0 }, "no 30th of February" },
    { " 1-Feb-2026 9:05:07 -0500", true, D_IMAP_ERROR_MALFORMED,
      { 0, 0, 0, 0, 0, 0, 0 }, "an hour of one digit" },
    { " 1-Feb-2026 09:05:07 0500", true, D_IMAP_ERROR_MALFORMED,
      { 0, 0, 0, 0, 0, 0, 0 }, "a zone without its sign" },
    { " 1-Feb-2026 09:05:07", true, D_IMAP_ERROR_MALFORMED,
      { 0, 0, 0, 0, 0, 0, 0 }, "or without a zone" },
    { "\" 1-Feb-2026 09:05:07 -0500", true, D_IMAP_ERROR_MALFORMED,
      { 0, 0, 0, 0, 0, 0, 0 }, "an unclosed quote" }
};

/*
d_tests_imap_datetime_same
  Whether two local times are the same, field by field.
*/
static bool
d_tests_imap_datetime_same(
    const struct d_imap_datetime* _a,
    const struct d_imap_datetime* _b
)
{
    return ( (_a->year == _b->year)     &&
             (_a->month == _b->month)   &&
             (_a->day == _b->day)       &&
             (_a->hour == _b->hour)     &&
             (_a->minute == _b->minute) &&
             (_a->second == _b->second) &&
             (_a->zone == _b->zone) );
}

/*
d_tests_imap_seqset_parsed
  A row's text parsed, compared with its ranges on success.
*/
static bool
d_tests_imap_seqset_parsed(
    const struct d_tests_imap_seqset_case* _row
)
{
    struct d_imap_seq_range ranges[3] = { { 0u, 0u }, { 0u, 0u },
                                          { 0u, 0u } };
    size_t                  count     = 0u;

    const enum d_imap_error error =
        d_imap_seqset_parse(d_pack_text_from_cstr(_row->text),
                            ranges,
                            3u,
                            &count);
    bool same = ( (error == _row->error) &&
                  ( (error != D_IMAP_OK) ||
                    (count == _row->count) ) );

    // each range, on success
    for (size_t i = 0u; (same) && (error == D_IMAP_OK) && (i < count); ++i)
    {
        same = ( (ranges[i].first == _row->ranges[i].first) &&
                 (ranges[i].last == _row->ranges[i].last) );
    }

    return same;
}

/*
d_tests_imap_seqset_written
  A row's ranges written, compared with its text.
*/
static bool
d_tests_imap_seqset_written(
    const struct d_tests_imap_seqset_case* _row
)
{
    struct d_tests_imap_capture capture = { .length = 0u };

    d_tests_imap_capture_init(&capture,
                              sizeof(capture.data));

    return ( (d_imap_seqset_write(d_tests_imap_capture_sink(&capture),
                                  _row->ranges,
                                  _row->count) == D_IMAP_OK) &&
             (d_tests_imap_capture_is(&capture,
                                      _row->text)) );
}

/*
d_tests_imap_seqset_room
  Measuring, too little room, NULL arguments, and writing nothing.
*/
static bool
d_tests_imap_seqset_room(
    struct d_test_counter* _counter
)
{
    struct d_imap_seq_range     two[2]  = { { 0u, 0u }, { 0u, 0u } };
    struct d_tests_imap_capture capture = { .length = 0u };
    const struct d_pack_text    text    = d_pack_text_from_cstr("1:4,7,9:*");
    size_t                      count   = 0u;
    bool                        result  = true;

    d_tests_imap_capture_init(&capture,
                              sizeof(capture.data));

    const bool counted =
        ( (d_imap_seqset_parse(text,
                               NULL,
                               0u,
                               &count) == D_IMAP_ERROR_BUFFER_TOO_SMALL) &&
          (count == 3u)                                                  &&
          (d_imap_seqset_parse(text,
                               two,
                               2u,
                               &count) == D_IMAP_ERROR_BUFFER_TOO_SMALL) &&
          (count == 3u)                                                  &&
          (two[0].first == 1u)                                           &&
          (two[1].first == 7u) );
    const bool refused =
        ( (d_imap_seqset_parse(text,
                               NULL,
                               2u,
                               &count) == D_IMAP_ERROR_INVALID_ARGUMENT) &&
          (d_imap_seqset_parse(text,
                               two,
                               2u,
                               NULL) == D_IMAP_ERROR_INVALID_ARGUMENT)   &&
          (d_imap_seqset_write(d_tests_imap_capture_sink(&capture),
                               two,
                               0u) == D_IMAP_ERROR_INVALID_ARGUMENT)     &&
          (d_imap_seqset_write(d_tests_imap_capture_sink(&capture),
                               NULL,
                               1u) == D_IMAP_ERROR_INVALID_ARGUMENT)     &&
          (capture.length == 0u) );

    result = d_tests_net_assert(counted,
                                "d_imap_seqset_parse",
                                "counts every range whatever the room",
                                _counter) && result;
    result = d_tests_net_assert(refused,
                                "d_imap_seqset_write",
                                "refuses no ranges, and NULL arguments",
                                _counter) && result;

    return result;
}

// d_tests_imap_member_case
//   struct: a value asked about, the largest number in use, and whether
// RFC 9051's example set, "1:4,7,9:*", holds it.
struct d_tests_imap_member_case
{
    uint32_t value;
    uint32_t largest;
    bool     member;
};

/*
MEMBERS
  The example set's edges and gaps with 20 the largest number in use, and
0, which no set holds.
*/
static const struct d_tests_imap_member_case MEMBERS[] =
{
    { 1u, 20u, true },   { 4u, 20u, true },   { 5u, 20u, false },
    { 7u, 20u, true },   { 8u, 20u, false },  { 20u, 20u, true },
    { 21u, 20u, false }, { 0u, 20u, false }
};

/*
d_tests_imap_seqset_members
  Membership: "*" resolves to the largest number in use, a reversed range
holds what lies between its bounds, and a NULL set holds nothing.
*/
static bool
d_tests_imap_seqset_members(
    struct d_test_counter* _counter
)
{
    const struct d_imap_seq_range reversed = { 4u, 2u };
    const struct d_imap_seq_range starred  = { D_IMAP_SEQ_STAR, 5u };
    bool                          member   = true;

    // each value against the example set
    for (size_t i = 0u; i < (sizeof(MEMBERS) / sizeof(MEMBERS[0])); ++i)
    {
        member = ( (d_imap_seqset_contains(ROUND_TRIPS[0].ranges,
                                           ROUND_TRIPS[0].count,
                                           MEMBERS[i].value,
                                           MEMBERS[i].largest) ==
                    MEMBERS[i].member) &&
                   (member) );
    }

    member = ( (d_imap_seqset_contains(&reversed,
                                       1u,
                                       3u,
                                       9u))  &&
               (d_imap_seqset_contains(&starred,
                                       1u,
                                       4u,
                                       3u))  &&
               (!d_imap_seqset_contains(&starred,
                                        1u,
                                        2u,
                                        3u)) &&
               (!d_imap_seqset_contains(NULL,
                                        1u,
                                        1u,
                                        1u)) &&
               (member) );

    return d_tests_net_assert(member,
                              "d_imap_seqset_contains",
                              "* is the largest number; 4:2 is 2:4",
                              _counter);
}

/*
d_tests_sa_imap_seqset
  Tests the following:
  - sets parse to their ranges and are written back the same
  - leading zeros are taken; 0, numbers past 32 bits and malformed sets
    are refused
  - measuring and too little room count every range
  - membership resolves "*" to the largest number, either way round
*/
bool
d_tests_sa_imap_seqset(
    struct d_test_counter* _counter
)
{
    const size_t trips  = sizeof(ROUND_TRIPS) / sizeof(ROUND_TRIPS[0]);
    const size_t parses = sizeof(SET_PARSES) / sizeof(SET_PARSES[0]);
    bool         result = true;

    // each set both ways
    for (size_t i = 0u; i < trips; ++i)
    {
        result = d_tests_net_assert(
                     (d_tests_imap_seqset_parsed(&ROUND_TRIPS[i])) &&
                     (d_tests_imap_seqset_written(&ROUND_TRIPS[i])),
                     "d_imap_seqset_parse",
                     ROUND_TRIPS[i].what,
                     _counter) && result;
    }

    // each set that only parses, or is refused
    for (size_t i = 0u; i < parses; ++i)
    {
        result = d_tests_net_assert(d_tests_imap_seqset_parsed(&SET_PARSES[i]),
                                    "d_imap_seqset_parse",
                                    SET_PARSES[i].what,
                                    _counter) && result;
    }

    result = d_tests_imap_seqset_members(_counter) && result;
    result = d_tests_imap_seqset_room(_counter) && result;

    return result;
}

/*
d_tests_imap_rendered
  A row converted to Unix time and written as a date and a date-time; each
result is compared, the writers' with their text or with RANGE.
*/
static bool
d_tests_imap_rendered(
    const struct d_tests_imap_render_case* _row
)
{
    struct d_tests_imap_capture date    = { .length = 0u };
    struct d_tests_imap_capture timed   = { .length = 0u };
    int64_t                     seconds = 7;

    d_tests_imap_capture_init(&date,
                              sizeof(date.data));
    d_tests_imap_capture_init(&timed,
                              sizeof(timed.data));

    const enum d_imap_error converted =
        d_imap_datetime_to_unix(_row->datetime,
                                &seconds);
    const enum d_imap_error dated =
        d_imap_date_write(d_tests_imap_capture_sink(&date),
                          _row->datetime);
    const enum d_imap_error written =
        d_imap_datetime_write(d_tests_imap_capture_sink(&timed),
                              _row->datetime);

    return ( (converted == _row->error)                           &&
             ( (converted != D_IMAP_OK) ||
               (seconds == _row->seconds) )                       &&
             ( (_row->date == NULL)
                   ? (dated == D_IMAP_ERROR_RANGE)
                   : ( (dated == D_IMAP_OK) &&
                       (d_tests_imap_capture_is(&date,
                                                _row->date)) ) )  &&
             ( (_row->date_time == NULL)
                   ? (written == D_IMAP_ERROR_RANGE)
                   : ( (written == D_IMAP_OK) &&
                       (d_tests_imap_capture_is(&timed,
                                                _row->date_time)) ) ) );
}

/*
d_tests_sa_imap_dates
  Tests the following:
  - Unix times convert to local times, in any zone, across 0000 to 9999
  - local times convert back, a leap second being the next minute's first
  - dates and date-times are written in IMAP's forms; impossible fields
    are refused, and a SEARCH date ignores the time
  - NULL out-pointers are refused
*/
bool
d_tests_sa_imap_dates(
    struct d_test_counter* _counter
)
{
    const struct d_imap_datetime epoch  = { 1970, 1, 1, 0, 0, 0, 0 };
    bool                         result = true;

    // each Unix time
    for (size_t i = 0u; i < (sizeof(FROM_UNIX) / sizeof(FROM_UNIX[0])); ++i)
    {
        struct d_imap_datetime local = { 7, 7, 7, 7, 7, 7, 7 };

        const enum d_imap_error error =
            d_imap_datetime_from_unix(FROM_UNIX[i].seconds,
                                      FROM_UNIX[i].zone,
                                      &local);

        result = d_tests_net_assert(
                     (error == FROM_UNIX[i].error) &&
                     ( (error != D_IMAP_OK) ||
                       (d_tests_imap_datetime_same(&local,
                                                   &FROM_UNIX[i].datetime)) ),
                     "d_imap_datetime_from_unix",
                     FROM_UNIX[i].what,
                     _counter) && result;
    }

    // each local time
    for (size_t i = 0u; i < (sizeof(RENDERINGS) / sizeof(RENDERINGS[0])); ++i)
    {
        result = d_tests_net_assert(d_tests_imap_rendered(&RENDERINGS[i]),
                                    "d_imap_datetime_to_unix",
                                    RENDERINGS[i].what,
                                    _counter) && result;
    }

    const bool nulls =
        ( (d_imap_datetime_from_unix(0,
                                     0,
                                     NULL) == D_IMAP_ERROR_INVALID_ARGUMENT) &&
          (d_imap_datetime_to_unix(epoch,
                                   NULL) == D_IMAP_ERROR_INVALID_ARGUMENT) );

    result = d_tests_net_assert(nulls,
                                "d_imap_datetime_to_unix",
                                "refuse a NULL out-pointer",
                                _counter) && result;

    return result;
}

/*
d_tests_sa_imap_date_parsing
  Tests the following:
  - SEARCH dates and date-times parse in each form IMAP allows
  - impossible dates and times are RANGE, and malformed ones MALFORMED
  - a failure leaves the output untouched, and NULL is refused
*/
bool
d_tests_sa_imap_date_parsing(
    struct d_test_counter* _counter
)
{
    const struct d_imap_datetime sentinel = { 7, 7, 7, 7, 7, 7, 7 };
    bool                         result   = true;

    // each date or date-time, parsed over the sentinel
    for (size_t i = 0u; i < (sizeof(PARSES) / sizeof(PARSES[0])); ++i)
    {
        struct d_imap_datetime   parsed = sentinel;
        const struct d_pack_text text   = d_pack_text_from_cstr(PARSES[i].text);

        const enum d_imap_error error =
            (PARSES[i].timed)
                ? d_imap_datetime_parse(text,
                                        &parsed)
                : d_imap_date_parse(text,
                                    &parsed);

        result = d_tests_net_assert(
                     (error == PARSES[i].error) &&
                     (d_tests_imap_datetime_same(&parsed,
                                                 (error == D_IMAP_OK)
                                                     ? &PARSES[i].datetime
                                                     : &sentinel)),
                     (PARSES[i].timed) ? "d_imap_datetime_parse"
                                       : "d_imap_date_parse",
                     PARSES[i].what,
                     _counter) && result;
    }

    const bool nulls =
        ( (d_imap_date_parse(d_pack_text_from_cstr("1-Feb-2026"),
                             NULL) == D_IMAP_ERROR_INVALID_ARGUMENT) &&
          (d_imap_datetime_parse(d_pack_text_from_cstr(PARSES[16].text),
                                 NULL) == D_IMAP_ERROR_INVALID_ARGUMENT) );

    result = d_tests_net_assert(nulls,
                                "d_imap_date_parse",
                                "refuse a NULL out-pointer",
                                _counter) && result;

    return result;
}
