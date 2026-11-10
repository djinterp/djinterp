/*******************************************************************************
* djinterp [test]                                       ostream_template_tests.c
*
*   The template engine's suite (c/text/ostream_template.h), in DTest's C face
* (c/modules/test/test_suite.h): values, one value's formatting, the four
* conversions, the compiler's every error, and running templates. Each group
* below is a test of the module "ostream_template", and each of its checks is
* recorded against that test, so a failing check does not stop its group.
*
*   TWO DIFFERENTIAL TESTS CARRY THE NUMBERS. Random integers -- every width
* to 64 bits, both signs -- with random widths, signs, zero padding and bases
* are formatted here and by snprintf, and must agree byte for byte; so must
* random doubles under f, e and g. Binary and centring, which printf does not
* have, are checked by hand.
*
*
* path:      /tests/djinterp/c/text/ostream_template_tests.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.28
*                                                            revised: 2026.11.09
*******************************************************************************/
#include "ostream_template_tests.h"  // the suite's entry point, DTest's C face
// std
#include <limits.h>   // LONG_MIN, LONG_MAX, ULONG_MAX
#include <math.h>     // NAN, INFINITY
#include <stdbool.h>  // bool
#include <stdio.h>    // printf, snprintf
#include <string.h>   // memcmp, strlen, strcmp
// djinterp
#include "../../../../inc/djinterp/c/util/ostream/ostream.h"   // streams
#include "../../../../inc/djinterp/c/text/ostream_template.h"  // the engine
// re_std
#include "../../../../inc/re_std/c/cstdint/dstdint.h"  // int64_t, uint64_t


// D_TESTS_TEMPLATE_CHECK
//   macro: one check: counts it, and prints the expression and line on
// failure.
#define D_TESTS_TEMPLATE_CHECK(expr)                                           \
    d_tests_template_check(((expr) ? 1 : 0), #expr, __LINE__)

// D_TESTS_TEMPLATE_OPS
//   constant: the operations a test template may compile to.
#define D_TESTS_TEMPLATE_OPS  64u

// d_tests_template_suite
//   the suite being built, for the checks; set by d_tests_template_all.
static struct dj_test_suite* d_tests_template_suite;
static uint64_t dj_tests_template_state = 0x9E3779B97F4A7C15u;

/*
d_tests_template_check
  Records one check against the open test.
*/
static bool
d_tests_template_check(
    int         _ok,
    const char* _expression,
    int         _line
)
{
    return d_test_suite_check(d_tests_template_suite,
                              (_ok != 0),
                              _expression,
                              __FILE__,
                              _line);
}

/*
d_tests_template_open
  Opens a group's test, with its descriptor.
*/
static void
d_tests_template_open(
    const char* _name,
    const char* _descriptor
)
{
    (void)d_test_suite_test(d_tests_template_suite, _name);
    (void)d_test_suite_describe(d_tests_template_suite, _descriptor);

    return;
}

/*
d_tests_template_next
  xorshift64*: a repeatable stream of random numbers.
*/
static uint64_t
d_tests_template_next(void)
{
    dj_tests_template_state ^= dj_tests_template_state >> 12;
    dj_tests_template_state ^= dj_tests_template_state << 25;
    dj_tests_template_state ^= dj_tests_template_state >> 27;

    return dj_tests_template_state * 0x2545F4914F6CDD1Du;
}

/*
d_tests_template_format
  Writes one value with a specification's text into a buffer, NUL-
terminated, and returns the buffer.
*/
static const char*
d_tests_template_format(
    char*                          _buffer,
    size_t                         _size,
    struct dj_ostream_value        _value,
    const char*                    _spec
)
{
    struct dj_ostream       os;
    struct dj_ostream_fixed fixed;
    struct dj_ostream_spec  spec;

    if (d_ostream_spec_parse(&spec, _spec, strlen(_spec)) !=
        D_OSTREAM_TEMPLATE_OK)
    {
        return "(bad spec)";
    }

    (void)d_ostream_init_fixed(&os, &fixed, _buffer, _size, 1);
    (void)d_ostream_write_value(&os, &_value, &spec);
    (void)d_ostream_close(&os);

    return _buffer;
}

/*
d_tests_template_is
  Whether a value formats as expected.
*/
static int
d_tests_template_is(
    struct dj_ostream_value _value,
    const char*             _spec,
    const char*             _expected
)
{
    char        buffer[512];
    const char* got = d_tests_template_format(buffer,
                                              sizeof(buffer),
                                              _value,
                                              _spec);

    if (strcmp(got, _expected) != 0)
    {
        (void)d_test_suite_note(d_tests_template_suite,
                                "\"%s\" with \"%s\": got [%s], expected [%s]",
                                (_value.kind == D_OSTREAM_VALUE_TEXT)
                                    ? _value.text
                                    : "-",
                                _spec,
                                got,
                                _expected);

        return 0;
    }

    return 1;
}

/*
d_tests_template_values
  Tests the following:
  - each constructor's kind, halves and sign, LONG_MIN, ULONG_MAX, INT64_MIN
    and UINT64_MAX included
  - to_long's clamping
  - truthiness of every kind
*/
static void
d_tests_template_values(void)
{
    struct dj_ostream_value value = d_ostream_value_long(LONG_MIN);

    d_tests_template_open("values",
                          "each constructor's kind and sign, clamping and "
                          "truthiness");

    D_TESTS_TEMPLATE_CHECK( (value.kind == D_OSTREAM_VALUE_INTEGER) &&
                            (value.negative == 1) );
    D_TESTS_TEMPLATE_CHECK(d_ostream_value_to_long(&value) == LONG_MIN);
    value = d_ostream_value_long(LONG_MAX);
    D_TESTS_TEMPLATE_CHECK(d_ostream_value_to_long(&value) == LONG_MAX);
    value = d_ostream_value_ulong(ULONG_MAX);
    D_TESTS_TEMPLATE_CHECK(d_ostream_value_to_long(&value) == LONG_MAX);
    value = d_ostream_value_int64(INT64_MIN);
    D_TESTS_TEMPLATE_CHECK( (value.high == 0x80000000u) &&
                            (value.low == 0u)           &&
                            (value.negative == 1) );
    D_TESTS_TEMPLATE_CHECK(d_ostream_value_to_long(&value) == LONG_MIN);
    value = d_ostream_value_uint64(UINT64_MAX);
    D_TESTS_TEMPLATE_CHECK( (value.high == 0xFFFFFFFFu) &&
                            (value.low == 0xFFFFFFFFu)  &&
                            (value.negative == 0) );
    value = d_ostream_value_size((size_t)-1);
    D_TESTS_TEMPLATE_CHECK(value.low == 0xFFFFFFFFu);
    value = d_ostream_value_long(-5L);
    D_TESTS_TEMPLATE_CHECK(d_ostream_value_to_long(&value) == -5L);

    // text, and null text, which is NONE
    value = d_ostream_value_text("abc");
    D_TESTS_TEMPLATE_CHECK( (value.kind == D_OSTREAM_VALUE_TEXT) &&
                            (value.size == 3u) );
    value = d_ostream_value_text(NULL);
    D_TESTS_TEMPLATE_CHECK(value.kind == D_OSTREAM_VALUE_NONE);

    // truthiness
    value = d_ostream_value_text("");
    D_TESTS_TEMPLATE_CHECK(d_ostream_value_is_true(&value) == 0);
    value = d_ostream_value_text("0");
    D_TESTS_TEMPLATE_CHECK(d_ostream_value_is_true(&value) == 1);
    value = d_ostream_value_long(0L);
    D_TESTS_TEMPLATE_CHECK(d_ostream_value_is_true(&value) == 0);
    value = d_ostream_value_uint64((uint64_t)1u << 40);
    D_TESTS_TEMPLATE_CHECK(d_ostream_value_is_true(&value) == 1);
    value = d_ostream_value_bool(0);
    D_TESTS_TEMPLATE_CHECK(d_ostream_value_is_true(&value) == 0);
    value = d_ostream_value_bool(7);
    D_TESTS_TEMPLATE_CHECK( (d_ostream_value_is_true(&value) == 1) &&
                            (value.low == 1u) );
    value = d_ostream_value_real(0.0);
    D_TESTS_TEMPLATE_CHECK(d_ostream_value_is_true(&value) == 0);
    value = d_ostream_value_real(NAN);
    D_TESTS_TEMPLATE_CHECK(d_ostream_value_is_true(&value) == 1);
    value = d_ostream_value_none();
    D_TESTS_TEMPLATE_CHECK(d_ostream_value_is_true(&value) == 0);
    D_TESTS_TEMPLATE_CHECK(d_ostream_value_is_true(NULL) == 0);

    return;
}

/*
d_tests_template_specs
  Tests the following:
  - every part of a specification parses, in its place
  - a field, a stray character, a big width, a bad fill, an empty precision
    and an unknown conversion are refused, and leave the default
*/
static void
d_tests_template_specs(void)
{
    struct dj_ostream_spec spec;

    d_tests_template_open("specifications",
                          "a specification's parts parse, and bad ones are "
                          "refused");

    D_TESTS_TEMPLATE_CHECK(d_ostream_spec_parse(&spec, "", 0u)
                           == D_OSTREAM_TEMPLATE_OK);
    D_TESTS_TEMPLATE_CHECK( (spec.width == 0u) &&
                            (spec.precision == D_OSTREAM_SPEC_NO_PRECISION) &&
                            (spec.fill == ' ') );
    D_TESTS_TEMPLATE_CHECK(d_ostream_spec_parse(&spec, "!csv:*^+#012.5x", 15u)
                           == D_OSTREAM_TEMPLATE_OK);
    D_TESTS_TEMPLATE_CHECK( (spec.convert == D_OSTREAM_CONVERT_CSV)         &&
                            (spec.fill == '*')                              &&
                            (spec.align == D_OSTREAM_ALIGN_CENTER)          &&
                            (spec.sign == D_OSTREAM_SIGN_PLUS)              &&
                            (spec.flags == (D_OSTREAM_SPEC_ALTERNATE |
                                            D_OSTREAM_SPEC_ZERO))           &&
                            (spec.width == 12u)                             &&
                            (spec.precision == 5u)                          &&
                            (spec.type == 'x') );
    D_TESTS_TEMPLATE_CHECK(d_ostream_spec_parse(&spec, ":0>4", 4u)
                           == D_OSTREAM_TEMPLATE_OK);
    D_TESTS_TEMPLATE_CHECK( (spec.fill == '0') &&
                            (spec.align == D_OSTREAM_ALIGN_RIGHT) &&
                            (spec.width == 4u) &&
                            (spec.flags == 0u) );

    // refusals, each leaving the default
    D_TESTS_TEMPLATE_CHECK(d_ostream_spec_parse(&spec, ":{w}", 4u)
                           == D_OSTREAM_TEMPLATE_BAD_SPEC);
    D_TESTS_TEMPLATE_CHECK(d_ostream_spec_parse(&spec, ":5q", 3u)
                           == D_OSTREAM_TEMPLATE_BAD_SPEC);
    D_TESTS_TEMPLATE_CHECK(d_ostream_spec_parse(&spec, ":5000", 5u)
                           == D_OSTREAM_TEMPLATE_BAD_SPEC);
    D_TESTS_TEMPLATE_CHECK(d_ostream_spec_parse(&spec, ":\t<5", 4u)
                           == D_OSTREAM_TEMPLATE_BAD_SPEC);
    D_TESTS_TEMPLATE_CHECK(d_ostream_spec_parse(&spec, ":5.", 3u)
                           == D_OSTREAM_TEMPLATE_BAD_SPEC);
    D_TESTS_TEMPLATE_CHECK(d_ostream_spec_parse(&spec, "!yaml", 5u)
                           == D_OSTREAM_TEMPLATE_BAD_CONVERSION);
    D_TESTS_TEMPLATE_CHECK(d_ostream_spec_parse(&spec, "x", 1u)
                           == D_OSTREAM_TEMPLATE_BAD_SPEC);
    D_TESTS_TEMPLATE_CHECK( (spec.width == 0u) &&
                            (spec.convert == D_OSTREAM_CONVERT_RAW) );
    D_TESTS_TEMPLATE_CHECK(d_ostream_spec_parse(NULL, "", 0u)
                           == D_OSTREAM_TEMPLATE_INVALID);

    return;
}

/*
d_tests_template_text
  Tests the following:
  - widths and alignment, by code points, UTF-8 included
  - precision cuts by code points, before conversion
  - every conversion's escapes, and CSV's quoting rule
  - a width counts what a conversion writes
*/
static void
d_tests_template_text(void)
{
    d_tests_template_open("text and conversions",
                          "widths, precision and every conversion's escapes");

    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_text("ab"),
                                               ":5", "ab   "));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_text("ab"),
                                               ":>5", "   ab"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_text("ab"),
                                               ":*^5", "*ab**"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_text("abcdef"),
                                               ":3", "abcdef"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_text("caf\xC3\xA9"), ":>6", "  caf\xC3\xA9"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_text("\xC3\xA9t\xC3\xA9s"), ":.2", "\xC3\xA9t"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_text("abcdef"),
                                               ":-<5.3", "abc--"));

    // JSON: quoted, escaped; a cut string stays a string
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_text("a\"b\\c\n\x01"), "!json",
        "\"a\\\"b\\\\c\\n\\u0001\""));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_text("a\"bcd"), "!json:.2", "\"a\\\"\""));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_text("a\""), "!json:>8", "   \"a\\\"\""));

    // CSV: quoted only when it must be
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_text("plain"),
                                               "!csv", "plain"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_text("a,b"),
                                               "!csv", "\"a,b\""));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_text("say \"hi\""), "!csv", "\"say \"\"hi\"\"\""));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_text(" pad"),
                                               "!csv", "\" pad\""));

    // Markdown and XML
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_text("__cpp_x|*y*`z`\n"), "!md",
        "\\_\\_cpp\\_x\\|\\*y\\*\\`z\\` "));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_text("<a href='x'>&\"\x02"), "!xml",
        "&lt;a href=&#39;x&#39;&gt;&amp;&quot;\xEF\xBF\xBD"));

    // NONE: empty, or JSON's null; raw text is untouched
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_none(),
                                               ":3", "   "));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_none(),
                                               "!json", "null"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_text("a\"<|"),
                                               "!raw", "a\"<|"));

    return;
}

/*
d_tests_template_integers
  Tests the following:
  - the extremes of 64 bits, signed and unsigned, in every base
  - signs, the alternate prefix, zero padding after sign and prefix
  - booleans as words, as numbers and as JSON; JSON ignores the flags
*/
static void
d_tests_template_integers(void)
{
    d_tests_template_open("integers and booleans",
                          "64-bit extremes in every base, signs and booleans");

    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_int64(INT64_MIN), "", "-9223372036854775808"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_uint64(UINT64_MAX), "", "18446744073709551615"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(
        d_ostream_value_uint64(UINT64_MAX), ":#X", "0XFFFFFFFFFFFFFFFF"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_long(5L),
                                               ":#010b", "0b00000101"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_long(8L),
                                               ":#o", "0o10"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_long(-42L),
                                               ":06", "-00042"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_long(42L),
                                               ":+", "+42"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_long(42L),
                                               ": ", " 42"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_long(42L),
                                               ":<6", "42    "));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_long(42L),
                                               ":^7", "  42   "));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_long(42L),
                                               ":*<06", "42****"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_long(0L),
                                               ":x", "0"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_long(42L),
                                               ":f", "42"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_long(-42L),
                                               "!json:+#08x", "     -42"));

    // booleans
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_bool(1),
                                               "", "true"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_bool(0),
                                               ":>7", "  false"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_bool(1),
                                               ":d", "1"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_bool(0),
                                               "!json:d", "false"));

    return;
}

/*
d_tests_template_reals
  Tests the following:
  - styles and precision, signs and zero padding
  - JSON: 17 significant digits; nan and the infinities as null
  - non-finite values are never zero-padded
*/
static void
d_tests_template_reals(void)
{
    d_tests_template_open("reals",
                          "styles, precision, JSON's form and non-finite "
                          "values");

    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_real(1.5),
                                               "", "1.5"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_real(1.5),
                                               ":+08.2f", "+0001.50"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_real(-1.5),
                                               ":08.1f", "-00001.5"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_real(1234.5),
                                               ":.2e", "1.23e+03"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_real(0.1),
                                               "!json", "0.10000000000000001"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_real(NAN),
                                               "!json", "null"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_real(-INFINITY),
                                               "!json", "null"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_real(INFINITY),
                                               ":06", "   inf"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_real(INFINITY),
                                               ":+", "+inf"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_is(d_ostream_value_real(2.0),
                                               ":^7", "   2   "));

    return;
}

/*
d_tests_template_random_integers
  Tests the following:
  - 200000 random integers with random specifications agree with snprintf
*/
static void
d_tests_template_random_integers(void)
{
    static const char TYPES[] = "dxXo";
    size_t            failed  = 0u;

    d_tests_template_open("random integers against snprintf",
                          "200000 random integers agree with snprintf");

    for (size_t i = 0u; i < 200000u; ++i)
    {
        const uint64_t bits  = d_tests_template_next();
        const unsigned shift = (unsigned)(d_tests_template_next() % 64u);
        const char     type  = TYPES[d_tests_template_next() % 4u];
        const unsigned width = (unsigned)(d_tests_template_next() % 24u);
        const unsigned flags = (unsigned)(d_tests_template_next() % 16u);
        const int      sign  = (type == 'd') && ((flags & 1u) != 0u);
        const int      left  = ((flags & 2u) != 0u);
        const int      zero  = ((flags & 4u) != 0u) && (left == 0);
        const int      alt   = ((flags & 8u) != 0u) && (type != 'o')
                               && (type != 'd');
        char           spec[32];
        char           mine[128];
        char           theirs[128];
        char           format[32];

        // the value: signed for d, unsigned for the rest
        snprintf(spec, sizeof(spec), ":%s%s%s%s%u%c",
                 left ? "<" : "",
                 sign ? "+" : "",
                 alt ? "#" : "",
                 zero ? "0" : "",
                 width,
                 type);
        snprintf(format, sizeof(format), "%%%s%s%s%s%ull%c",
                 left ? "-" : "",
                 sign ? "+" : "",
                 alt ? "#" : "",
                 zero ? "0" : "",
                 width,
                 type);

        if (type == 'd')
        {
            const long long value = (long long)(int64_t)(bits >> shift);

            snprintf(theirs, sizeof(theirs), format, value);
            (void)d_tests_template_format(mine, sizeof(mine),
                                          d_ostream_value_int64(value),
                                          spec);
        }
        else
        {
            const unsigned long long value =
                (unsigned long long)(bits >> shift);

            snprintf(theirs, sizeof(theirs), format, value);
            (void)d_tests_template_format(mine, sizeof(mine),
                                          d_ostream_value_uint64(value),
                                          spec);
        }

        // printf writes 0 with '#x' as plain 0; this engine as 0x0
        if ( (alt != 0) &&
             (strcmp(theirs + strspn(theirs, " 0"), "") == 0) )
        {
            continue;
        }

        if (strcmp(mine, theirs) != 0)
        {
            if (++failed <= 5u)
            {
                (void)d_test_suite_note(d_tests_template_suite,
                                        "spec %s: [%s] snprintf %s: [%s]",
                                        spec,
                                        mine,
                                        format,
                                        theirs);
            }
        }
    }

    D_TESTS_TEMPLATE_CHECK(failed == 0u);

    return;
}

/*
d_tests_template_random_reals
  Tests the following:
  - 100000 random doubles with random styles, precisions, widths, signs and
    zero padding agree with snprintf in the C locale
*/
static void
d_tests_template_random_reals(void)
{
    static const char STYLES[] = "feg";
    size_t            failed   = 0u;

    d_tests_template_open("random reals against snprintf",
                          "100000 random doubles agree with snprintf");

    for (size_t i = 0u; i < 100000u; ++i)
    {
        const uint64_t bits      = d_tests_template_next();
        const char     style     = STYLES[d_tests_template_next() % 3u];
        const unsigned precision = (unsigned)(d_tests_template_next() % 12u);
        const unsigned width     = (unsigned)(d_tests_template_next() % 30u);
        const unsigned flags     = (unsigned)(d_tests_template_next() % 8u);
        const int      exponent  = (int)(d_tests_template_next() % 40u) - 20;
        double         value     = (double)(int64_t)(bits >> 11) /
                                   (double)((uint64_t)1u << 40);
        char           spec[32];
        char           format[32];
        char           mine[512];
        char           theirs[512];

        // spread the magnitudes over forty decades
        for (int e = 0; e < ((exponent < 0) ? -exponent : exponent); ++e)
        {
            value = (exponent < 0) ? (value / 10.0) : (value * 10.0);
        }

        snprintf(spec, sizeof(spec), ":%s%s%u.%u%c",
                 ((flags & 1u) != 0u) ? "+" : "",
                 ((flags & 2u) != 0u) ? "0" : "",
                 width,
                 precision,
                 style);
        snprintf(format, sizeof(format), "%%%s%s%u.%u%c",
                 ((flags & 1u) != 0u) ? "+" : "",
                 ((flags & 2u) != 0u) ? "0" : "",
                 width,
                 precision,
                 style);
        snprintf(theirs, sizeof(theirs), format, value);
        (void)d_tests_template_format(mine, sizeof(mine),
                                      d_ostream_value_real(value), spec);

        if (strcmp(mine, theirs) != 0)
        {
            if (++failed <= 5u)
            {
                (void)d_test_suite_note(d_tests_template_suite,
                                        "spec %s: [%s] snprintf %s: [%s]",
                                        spec,
                                        mine,
                                        format,
                                        theirs);
            }
        }
    }

    D_TESTS_TEMPLATE_CHECK(failed == 0u);

    return;
}

// the harness's fields: a, b, flag, none, width, prec, n
static const char* const D_TESTS_TEMPLATE_FIELDS[] =
{
    "a", "b", "flag", "none", "width", "prec", "n"
};

/*
d_tests_template_lookup
  The harness's fields, by name.
*/
static int32_t
d_tests_template_lookup(
    void*       _context,
    const char* _name,
    size_t      _size
)
{
    (void)_context;

    const size_t count = sizeof(D_TESTS_TEMPLATE_FIELDS) /
                         sizeof(D_TESTS_TEMPLATE_FIELDS[0]);

    for (size_t i = 0u; i < count; ++i)
    {
        if ( (strlen(D_TESTS_TEMPLATE_FIELDS[i]) == _size) &&
             (memcmp(D_TESTS_TEMPLATE_FIELDS[i], _name, _size) == 0) )
        {
            return (int32_t)i;
        }
    }

    return -1;
}

// a resolver's state: the flag, and a field that stops it
struct dj_tests_template_state
{
    int    flag;
    int    stop_at;
    size_t calls;
};

/*
d_tests_template_resolve
  The harness's values: a is "alpha", b is 42, flag is the state's, none is
NONE, width is 8, prec is 2, n is -7; stop_at stops at that field.
*/
static int32_t
d_tests_template_resolve(
    void*                    _context,
    uint32_t                 _field,
    struct dj_ostream_value* _value
)
{
    struct dj_tests_template_state* const state =
        (struct dj_tests_template_state*)_context;

    ++state->calls;

    if ((int)_field == state->stop_at)
    {
        return 0;
    }

    switch (_field)
    {
        case 0u: *_value = d_ostream_value_text("alpha");      break;
        case 1u: *_value = d_ostream_value_long(42L);          break;
        case 2u: *_value = d_ostream_value_bool(state->flag);  break;
        case 3u: *_value = d_ostream_value_none();             break;
        case 4u: *_value = d_ostream_value_long(8L);           break;
        case 5u: *_value = d_ostream_value_long(2L);           break;
        case 6u: *_value = d_ostream_value_long(-7L);          break;
        default: return 0;
    }

    return 1;
}

/*
d_tests_template_run
  Compiles and runs a template into a buffer; returns the buffer, or a
marker naming the compile status.
*/
static const char*
d_tests_template_run(
    char*       _buffer,
    size_t      _size,
    const char* _source,
    int         _flag
)
{
    struct dj_ostream_template_op    ops[D_TESTS_TEMPLATE_OPS];
    struct dj_ostream_template       compiled;
    struct dj_ostream_template_error error;
    struct dj_tests_template_state   state;
    struct dj_ostream                os;
    struct dj_ostream_fixed          fixed;

    state.flag    = _flag;
    state.stop_at = -1;
    state.calls   = 0u;

    if (d_ostream_template_compile(&compiled,
                                   ops,
                                   D_TESTS_TEMPLATE_OPS,
                                   _source,
                                   d_tests_template_lookup,
                                   NULL,
                                   &error) != D_OSTREAM_TEMPLATE_OK)
    {
        return d_ostream_template_status_name(error.status);
    }

    (void)d_ostream_init_fixed(&os, &fixed, _buffer, _size, 1);
    (void)d_ostream_template_render(&os,
                                    &compiled,
                                    d_tests_template_resolve,
                                    &state);
    (void)d_ostream_close(&os);

    return _buffer;
}

/*
d_tests_template_renders
  Whether a template runs to the text expected.
*/
static int
d_tests_template_renders(
    const char* _source,
    int         _flag,
    const char* _expected
)
{
    char        buffer[512];
    const char* got = d_tests_template_run(buffer,
                                           sizeof(buffer),
                                           _source,
                                           _flag);

    if (strcmp(got, _expected) != 0)
    {
        (void)d_test_suite_note(d_tests_template_suite,
                                "[%s] flag %d: got [%s], expected [%s]",
                                _source,
                                _flag,
                                got,
                                _expected);

        return 0;
    }

    return 1;
}

/*
d_tests_template_compile_errors
  Whether a template fails to compile with a status at an offset.
*/
static int
d_tests_template_fails(
    const char* _source,
    int32_t     _status,
    size_t      _offset
)
{
    struct dj_ostream_template_op    ops[D_TESTS_TEMPLATE_OPS];
    struct dj_ostream_template       compiled;
    struct dj_ostream_template_error error;
    const int32_t status = d_ostream_template_compile(&compiled,
                                                      ops,
                                                      D_TESTS_TEMPLATE_OPS,
                                                      _source,
                                                      d_tests_template_lookup,
                                                      NULL,
                                                      &error);

    if ( (status != _status)       ||
         (error.status != _status) ||
         (error.offset != _offset) ||
         (compiled.count != 0u) )
    {
        (void)d_test_suite_note(d_tests_template_suite,
                                "[%s]: got %s at %lu, expected %s at %lu",
                                _source,
                                d_ostream_template_status_name(error.status),
                                (unsigned long)error.offset,
                                d_ostream_template_status_name(_status),
                                (unsigned long)_offset);

        return 0;
    }

    return 1;
}

/*
d_tests_template_templates
  Tests the following:
  - literals, escaped braces, fields with conversions and specifications
  - widths and precisions from fields
  - blocks: ?, ^, {:}, nesting, {/name}
*/
static void
d_tests_template_templates(void)
{
    d_tests_template_open("templates",
                          "literals, fields, widths from fields and blocks");

    D_TESTS_TEMPLATE_CHECK(d_tests_template_renders("", 0, ""));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_renders("plain text", 0,
                                                    "plain text"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_renders("{{a}} {a}}}", 0,
                                                    "{a} alpha}"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_renders("[{a:>{width}}|{b:04}]", 0,
                                                    "[   alpha|0042]"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_renders("[{a:.{prec}}]", 0,
                                                    "[al]"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_renders("{a!json}: {none!json}", 0,
                                                    "\"alpha\": null"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_renders("{n:{a}}", 0, "-7"));

    // blocks
    D_TESTS_TEMPLATE_CHECK(d_tests_template_renders("<{?flag}yes{/}>", 1,
                                                    "<yes>"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_renders("<{?flag}yes{/}>", 0,
                                                    "<>"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_renders("<{?flag}y{:}n{/flag}>",
                                                    0, "<n>"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_renders("<{^flag}y{:}n{/}>", 0,
                                                    "<y>"));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_renders(
        "{?a}A{?none}N{:}{^flag}F{/}{/}{:}x{/}.", 0, "AF."));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_renders(
        "{?a}A{?none}N{:}{^flag}F{/}{/}{:}x{/}.", 1, "A."));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_renders("{?b}{b}{/}{?n}{n}{/}", 0,
                                                    "42-7"));

    return;
}

/*
d_tests_template_errors
  Tests the following:
  - every compile error, at its offset: stray and unclosed braces, empty and
    malformed tags, unknown fields (in a value, a width, a block), unknown
    conversions, bad specifications, unbalanced and misnamed blocks, a second
    {:}, nesting past D_CFG_OSTREAM_TEMPLATE_DEPTH
*/
static void
d_tests_template_errors(void)
{
    d_tests_template_open("template errors",
                          "every compile error, at its offset");

    D_TESTS_TEMPLATE_CHECK(d_tests_template_fails("ab}c",
                                                  D_OSTREAM_TEMPLATE_SYNTAX,
                                                  2u));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_fails("ab{a",
                                                  D_OSTREAM_TEMPLATE_SYNTAX,
                                                  2u));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_fails("x{}",
                                                  D_OSTREAM_TEMPLATE_SYNTAX,
                                                  1u));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_fails("x{ a}",
                                                  D_OSTREAM_TEMPLATE_SYNTAX,
                                                  1u));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_fails("x{zz}",
        D_OSTREAM_TEMPLATE_UNKNOWN_FIELD, 2u));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_fails("{a:{zz}}",
        D_OSTREAM_TEMPLATE_UNKNOWN_FIELD, 3u));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_fails("{a!yaml}",
        D_OSTREAM_TEMPLATE_BAD_CONVERSION, 2u));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_fails("{a:5q}",
                                                  D_OSTREAM_TEMPLATE_BAD_SPEC,
                                                  4u));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_fails("{a;}",
                                                  D_OSTREAM_TEMPLATE_SYNTAX,
                                                  2u));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_fails("x{/}",
                                                  D_OSTREAM_TEMPLATE_NESTING,
                                                  1u));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_fails("{:}",
                                                  D_OSTREAM_TEMPLATE_NESTING,
                                                  0u));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_fails("{?a}..{?b}",
                                                  D_OSTREAM_TEMPLATE_NESTING,
                                                  6u));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_fails("{?a}{/b}",
                                                  D_OSTREAM_TEMPLATE_NESTING,
                                                  4u));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_fails("{?a}{:}{:}{/}",
                                                  D_OSTREAM_TEMPLATE_NESTING,
                                                  7u));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_fails("{?}",
                                                  D_OSTREAM_TEMPLATE_SYNTAX,
                                                  0u));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_fails("{?zz}",
        D_OSTREAM_TEMPLATE_UNKNOWN_FIELD, 2u));
    D_TESTS_TEMPLATE_CHECK(d_tests_template_fails(
        "{?a}{?a}{?a}{?a}{?a}{?a}{?a}{?a}{?a}{?a}{?a}{?a}{?a}{?a}{?a}{?a}{?a}",
        D_OSTREAM_TEMPLATE_NESTING, 64u));

    return;
}

/*
d_tests_template_limits
  Tests the following:
  - a template too large reports the operations it needs, and compiles none
  - a null source is an empty template; a null template is INVALID; a null
    lookup knows no field
  - a resolver that stops is INVALID on the stream, the text before it kept;
    a null resolver or stream renders nothing
  - an unknown status has a name
*/
static void
d_tests_template_limits(void)
{
    struct dj_ostream_template_op    ops[4];
    struct dj_ostream_template       compiled;
    struct dj_ostream_template_error error;
    struct dj_tests_template_state   state;
    struct dj_ostream                os;
    struct dj_ostream_fixed          fixed;
    char                             buffer[64];

    d_tests_template_open("template limits",
                          "too large, null source, null template, null "
                          "lookup");

    // too large: the room it needs is reported
    D_TESTS_TEMPLATE_CHECK(d_ostream_template_compile(&compiled, ops, 4u,
                                                      "{a}-{b}-{a}-{b}",
                                                      d_tests_template_lookup,
                                                      NULL, &error)
                           == D_OSTREAM_TEMPLATE_CAPACITY);
    D_TESTS_TEMPLATE_CHECK( (error.needed == 7u) &&
                            (compiled.count == 0u) );
    D_TESTS_TEMPLATE_CHECK(d_ostream_template_compile(&compiled, ops, 4u,
                                                      NULL, NULL, NULL, NULL)
                           == D_OSTREAM_TEMPLATE_OK);
    D_TESTS_TEMPLATE_CHECK(compiled.count == 0u);
    D_TESTS_TEMPLATE_CHECK(d_ostream_template_compile(NULL, ops, 4u, "", NULL,
                                                      NULL, &error)
                           == D_OSTREAM_TEMPLATE_INVALID);
    D_TESTS_TEMPLATE_CHECK(d_ostream_template_compile(&compiled, ops, 4u, "{a}",
                                                      NULL, NULL, &error)
                           == D_OSTREAM_TEMPLATE_UNKNOWN_FIELD);

    // a resolver that stops: INVALID on the stream, the text before kept
    D_TESTS_TEMPLATE_CHECK(d_ostream_template_compile(&compiled, ops, 4u,
                                                      "ok {b} {a}",
                                                      d_tests_template_lookup,
                                                      NULL, &error)
                           == D_OSTREAM_TEMPLATE_OK);
    state.flag    = 0;
    state.stop_at = 0;
    state.calls   = 0u;
    (void)d_ostream_init_fixed(&os, &fixed, buffer, sizeof(buffer), 1);
    D_TESTS_TEMPLATE_CHECK(d_ostream_template_render(&os, &compiled,
                                                     d_tests_template_resolve,
                                                     &state) == 0);
    D_TESTS_TEMPLATE_CHECK(d_ostream_status(&os) == D_OSTREAM_INVALID);
    (void)d_ostream_close(&os);
    D_TESTS_TEMPLATE_CHECK(strcmp(buffer, "ok 42 ") == 0);
    D_TESTS_TEMPLATE_CHECK(state.calls == 2u);
    (void)d_ostream_init_fixed(&os, &fixed, buffer, sizeof(buffer), 1);
    D_TESTS_TEMPLATE_CHECK(d_ostream_template_render(&os, &compiled, NULL,
                                                     NULL) == 0);
    D_TESTS_TEMPLATE_CHECK(d_ostream_template_render(NULL, &compiled, NULL,
                                                     NULL) == 0);
    D_TESTS_TEMPLATE_CHECK(strcmp(d_ostream_template_status_name(99),
                                  "unknown") == 0);

    return;
}

/*
d_tests_template_all
  Opens the module and its block, then runs every group, each as a test.
*/
void
d_tests_template_all(
    struct dj_test_suite* _suite
)
{
    d_tests_template_suite = _suite;

    (void)d_test_suite_module(_suite, "ostream_template");
    (void)d_test_suite_describe(_suite,
                                "the template engine: values, formatting, "
                                "conversions, compiling and running");
    (void)d_test_suite_block(_suite, "ostream_template.h");

    d_tests_template_values();
    d_tests_template_specs();
    d_tests_template_text();
    d_tests_template_integers();
    d_tests_template_reals();
    d_tests_template_random_integers();
    d_tests_template_random_reals();
    d_tests_template_templates();
    d_tests_template_errors();
    d_tests_template_limits();

    d_tests_template_suite = NULL;

    return;
}
