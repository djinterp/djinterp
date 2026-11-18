/*******************************************************************************
* djinterp [test]                                  ostream_template_tests_json.c
*
*   JSON's groups of the template engine's suite: reals written with the
* fewest digits that read back as the same double, checked by hand and on
* random doubles under three locales; and text as UTF-8 under JSON and XML,
* each invalid run written as U+FFFD (the default D_CFG_OSTREAM_TEMPLATE_UTF8,
* REPLACE; the refusing knob has its own suite, ostream_template_refuse).
*
*
* path:      /tests/djinterp/c/text/ostream_template_tests_json.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.13
*                                                            revised: 2026.11.13
*******************************************************************************/
#include "ostream_template_tests.h"  // the shared checks and helpers
// std
#include <float.h>   // DBL_MAX, DBL_MIN
#include <locale.h>  // setlocale, LC_NUMERIC
#include <math.h>    // NAN, INFINITY
#include <stdio.h>   // snprintf
#include <stdlib.h>  // malloc, free, strtod
#include <string.h>  // memcpy, strchr, strlen
// djinterp
#include "../../../../inc/djinterp/c/text/ostream_template.h"  // the engine


// D_TESTS_TEMPLATE_JSON_COUNT
//   constant: random doubles per locale.
#define D_TESTS_TEMPLATE_JSON_COUNT  30000u

// D_TESTS_TEMPLATE_JSON_WIDTH
//   constant: the room each written double is kept in.
#define D_TESTS_TEMPLATE_JSON_WIDTH  40u

// D_TESTS_TEMPLATE_FFFD
//   constant: U+FFFD as UTF-8.
#define D_TESTS_TEMPLATE_FFFD  "\xEF\xBF\xBD"


/*
d_tests_template_json_reals
  Tests the following:
  - a JSON real has the fewest digits that read back: 0.1, 100, 1e+300,
    1e+21, -0, 2^53 in 16 digits, a third in 16, 0.1 + 0.2 in 17
  - integers below 10^15, written without the search, and the first ones
    past it: 1e+15, and 2^52 + 1 in 16 digits
  - of the two notations, the shorter: 30716667960829400 rather than
    3.07166679608294e+16, but 1e+16 rather than 10000000000000000
  - the extremes: DBL_MAX and DBL_MIN in 17 digits, the smallest subnormal
    as 5e-324
  - powers of two, whose interval is wider above: 2^-1017 in 16 digits,
    the decimal above the nearest; 2^-960 in 17, 2^1023 in 15; 2^55's 16
    digits in full, padded with a zero, and 2^54's 17
  - an explicit precision still rules, and nan and the infinities are null
*/
void
d_tests_template_json_reals(void)
{
    d_tests_template_open("JSON reals",
                          "the fewest digits that read back as the same "
                          "double");

    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_real(0.1),
                                               "!json", "0.1"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_real(100.0),
                                               "!json", "100"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_real(1e300),
                                               "!json", "1e+300"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_real(1e21),
                                               "!json", "1e+21"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_real(-0.0),
                                               "!json", "-0"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_real(9007199254740992.0), "!json",
        "9007199254740992"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_real(-123456.0), "!json", "-123456"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_real(999999999999999.0), "!json",
        "999999999999999"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_real(1e15),
                                               "!json", "1e+15"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_real(4503599627370497.0), "!json",
        "4503599627370497"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_real(1.0 / 3.0), "!json", "0.3333333333333333"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_real(0.1 + 0.2), "!json", "0.30000000000000004"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_real(30716667960829400.0), "!json",
        "30716667960829400"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_real(1e16),
                                               "!json", "1e+16"));

    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_real(DBL_MAX), "!json", "1.7976931348623157e+308"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_real(DBL_MIN), "!json", "2.2250738585072014e-308"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_real(4.9406564584124654e-324), "!json", "5e-324"));

    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_real(7.120236347223045e-307), "!json",
        "7.120236347223045e-307"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_real(1.0261342003245941e-289), "!json",
        "1.0261342003245941e-289"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_real(8.98846567431158e+307), "!json",
        "8.98846567431158e+307"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_real(36028797018963968.0), "!json",
        "36028797018963970"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_real(-36028797018963968.0), "!json",
        "-36028797018963970"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_real(18014398509481984.0), "!json",
        "18014398509481984"));

    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_real(0.1),
                                               "!json:.3", "0.1"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_real(2.0 / 3.0),
                                               "!json:.3", "0.667"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_real(NAN),
                                               "!json", "null"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_real(INFINITY),
                                               "!json", "null"));

    return;
}


/*
d_tests_template_json_write_all
  Writes _count random finite doubles as JSON into _texts, in the program's
locale as it is, keeping each double in _values.
*/
static void
d_tests_template_json_write_all(
    double* _values,
    char*   _texts,
    size_t  _count
)
{
    size_t i = 0u;

    while (i < _count)
    {
        const uint64_t bits = d_tests_template_next();
        double         value;

        memcpy(&value, &bits, sizeof(value));

        // the finite ones: not-a-number and the infinities are null
        if ( (value != value) ||
             (value > DBL_MAX) ||
             (value < -DBL_MAX) )
        {
            continue;
        }

        _values[i] = value;
        (void)d_tests_template_format(
                  _texts + (i * D_TESTS_TEMPLATE_JSON_WIDTH),
                  D_TESTS_TEMPLATE_JSON_WIDTH,
                  d_ostream_value_real(value),
                  "!json");
        ++i;
    }

    return;
}


/*
d_tests_template_json_bad
  How many of _count written doubles fail to read back in the C locale, or
are longer than %.17g's form, or hold a comma; the first five are noted.
*/
static size_t
d_tests_template_json_bad(
    const double* _values,
    const char*   _texts,
    size_t        _count,
    const char*   _locale
)
{
    size_t bad = 0u;

    for (size_t i = 0u; i < _count; ++i)
    {
        const char* const text = _texts + (i * D_TESTS_TEMPLATE_JSON_WIDTH);
        char*             end  = NULL;
        char              longest[40];
        const double      back = strtod(text, &end);

        (void)snprintf(longest, sizeof(longest), "%.17g", _values[i]);

        if ( (end == text) ||
             (*end != '\0') ||
             (back != _values[i]) ||
             (strlen(text) > strlen(longest)) ||
             (strchr(text, ',') != NULL) )
        {
            if (++bad <= 5u)
            {
                (void)d_test_suite_note(d_tests_template_suite,
                                        "%s: [%s] for %.17g",
                                        _locale,
                                        text,
                                        _values[i]);
            }
        }
    }

    return bad;
}


/*
d_tests_template_json_random
  Tests the following:
  - random doubles, every bit pattern that is finite, written as JSON under
    the C, German and Pashto locales (a comma, and a two-byte point), read
    back exactly in the C locale, never longer than %.17g's form and never
    with the locale's point
*/
void
d_tests_template_json_random(void)
{
    static const char* const locales[] =
    {
        "C", "de_DE.UTF-8", "ps_AF.UTF-8"
    };
    const size_t count  = D_TESTS_TEMPLATE_JSON_COUNT;
    double*      values = (double*)malloc(count * sizeof(double));
    char*        texts  = (char*)malloc(count * D_TESTS_TEMPLATE_JSON_WIDTH);
    size_t       bad    = 0u;

    d_tests_template_open("JSON reals at random",
                          "random doubles read back exactly, in the C locale "
                          "and two others");

    if ( (values == NULL) ||
         (texts == NULL) )
    {
        D_TESTS_TEMPLATE_CHECK((values != NULL) && (texts != NULL));
        free(values);
        free(texts);

        return;
    }

    for (size_t i = 0u; i < (sizeof(locales) / sizeof(locales[0])); ++i)
    {
        if (setlocale(LC_NUMERIC, locales[i]) == NULL)
        {
            (void)d_test_suite_note(d_tests_template_suite,
                                    "left out: locale %s not installed",
                                    locales[i]);

            continue;
        }

        d_tests_template_json_write_all(values, texts, count);
        (void)setlocale(LC_NUMERIC, "C");
        bad += d_tests_template_json_bad(values, texts, count, locales[i]);
    }

    (void)setlocale(LC_NUMERIC, "C");
    D_TESTS_TEMPLATE_CHECK(bad == 0u);

    free(values);
    free(texts);

    return;
}


/*
d_tests_template_json_utf8
  Tests the following:
  - "x\xFF\xC3y" holds two invalid runs, written as two U+FFFD
  - a sequence cut short at the end is one run; an overlong form, a
    surrogate and a code point past U+10FFFF are a run per byte that can't
    go on
  - valid sequences of two, three and four bytes, U+10FFFF included, stay
  - XML replaces the same way; a width counts each U+FFFD as one character
  - raw text and CSV pass every byte through
*/
void
d_tests_template_json_utf8(void)
{
    d_tests_template_open("JSON and XML text as UTF-8",
                          "valid sequences kept, each invalid run written as "
                          "U+FFFD");

    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_text("x\xFF\xC3y"), "!json",
        "\"x" D_TESTS_TEMPLATE_FFFD D_TESTS_TEMPLATE_FFFD "y\""));

    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_text("ab\xE2\x82"), "!json",
        "\"ab" D_TESTS_TEMPLATE_FFFD "\""));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_text("\xC0\xAF"), "!json",
        "\"" D_TESTS_TEMPLATE_FFFD D_TESTS_TEMPLATE_FFFD "\""));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_text("\xED\xA0\x80"), "!json",
        "\"" D_TESTS_TEMPLATE_FFFD D_TESTS_TEMPLATE_FFFD
        D_TESTS_TEMPLATE_FFFD "\""));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_text("\xF4\x90\x80\x80"), "!json",
        "\"" D_TESTS_TEMPLATE_FFFD D_TESTS_TEMPLATE_FFFD
        D_TESTS_TEMPLATE_FFFD D_TESTS_TEMPLATE_FFFD "\""));

    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_text("caf\xC3\xA9 \xE2\x82\xAC \xF0\x9F\x98\x80 "
                             "\xF4\x8F\xBF\xBF"), "!json",
        "\"caf\xC3\xA9 \xE2\x82\xAC \xF0\x9F\x98\x80 \xF4\x8F\xBF\xBF\""));

    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_text("a\xFF<"), "!xml",
        "a" D_TESTS_TEMPLATE_FFFD "&lt;"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_text("\xFF"), "!json:>4",
        " \"" D_TESTS_TEMPLATE_FFFD "\""));

    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_text("x\xFFy"), "!raw", "x\xFFy"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_text("x\xFFy"), "!csv", "x\xFFy"));

    return;
}
