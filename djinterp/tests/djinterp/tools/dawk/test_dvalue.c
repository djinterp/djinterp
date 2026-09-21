/******************************************************************************
* djinterp [dawk]                                                test_dvalue.c
*
*   Unit tests for the value model in dvalue.h.
*     The comparison matrix is written out in full -- every tag against every
* tag, both orders -- because that table is the step where reimplementations
* of awk fail, and they fail silently. A case omitted here is a case nobody
* will notice is wrong.
*
*
* path:      /tests/djinterp/c/dawk/test_dvalue.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/
#include "../../../../inc/djinterp/tools/dawk/dvalue.h"  // corresponding header
// std
#include <stdio.h>   // printf
#include <string.h>  // strlen, strcmp


static int g_checks = 0;
static int g_failed = 0;


/*
d_tests_report
  Records one boolean assertion.

Parameter(s):
  _passed:  the outcome.
  _label:   what was being asserted.
Return:
  none.
*/
static void
d_tests_report(
    bool        _passed,
    const char* _label
)
{
    g_checks++;

    // only a failure is worth printing
    if (!_passed)
    {
        g_failed++;
        printf("  FAIL  %s\n", _label);
    }

    return;
}


/*
d_tests_numeric_strings
  brief description
  Tests the following:
  - the shapes POSIX admits as numeric strings, including signs and blanks
  - hexadecimal, infinity and NaN spellings are rejected, as awk rejects them
  - a partial numeric prefix converts but does not make a numeric string
  - the empty string and blanks alone are not numeric
*/
static void
d_tests_numeric_strings(void)
{
    printf("numeric strings\n");

    static const struct
    {
        const char* text;
        bool        numeric;
        double      value;
    }
    cases[] =
    {
        { "10",      true,  10.0   },
        { "+3",      true,  3.0    },
        { "-3.5",    true,  -3.5   },
        { " 42 ",    true,  42.0   },
        { "1e3",     true,  1000.0 },
        { "1E-2",    true,  0.01   },
        { ".5",      true,  0.5    },
        { "5.",      true,  5.0    },
        { "0x10",    false, 0.0    },
        { "inf",     false, 0.0    },
        { "nan",     false, 0.0    },
        { "10abc",   false, 0.0    },
        { "abc",     false, 0.0    },
        { "",        false, 0.0    },
        { "   ",     false, 0.0    },
        { "1e",      false, 0.0    },
        { "-",       false, 0.0    }
    };

    // each shape is judged on its own
    for (size_t at = 0; at < (sizeof(cases) / sizeof(cases[0])); ++at)
    {
        double     value  = 0.0;
        const bool result = d_awk_looks_numeric(cases[at].text,
                                                strlen(cases[at].text),
                                                &value);

        char label[96];

        snprintf(label,
                 sizeof(label),
                 "looks_numeric(\"%s\") == %s",
                 cases[at].text,
                 cases[at].numeric ? "true" : "false");

        d_tests_report( (result == cases[at].numeric) &&
                        ((!result) || (value == cases[at].value)),
                        label );
    }

    // arithmetic takes the longest prefix, so "3abc" is three
    d_tests_report((d_awk_text_to_number("3abc", 4) == 3.0),
                   "text_to_number(\"3abc\") == 3");
    d_tests_report((d_awk_text_to_number("abc", 3) == 0.0),
                   "text_to_number(\"abc\") == 0");
    d_tests_report((d_awk_text_to_number("0x10", 4) == 0.0),
                   "text_to_number(\"0x10\") == 0, not 16");

    return;
}


/*
d_tests_conversion
  brief description
  Tests the following:
  - an integral value renders as an integer whatever the format says
  - a fractional value honours the supplied format
  - the exceptional values have fixed spellings
*/
static void
d_tests_conversion(void)
{
    printf("number rendering\n");

    char buffer[64];

    (void)d_awk_number_to_text(17.0, "%.2f", buffer, sizeof(buffer));
    d_tests_report((strcmp(buffer, "17") == 0),
                   "integral ignores CONVFMT: 17.0 -> \"17\"");

    (void)d_awk_number_to_text(-3.0, "%.6g", buffer, sizeof(buffer));
    d_tests_report((strcmp(buffer, "-3") == 0), "integral negative -> \"-3\"");

    (void)d_awk_number_to_text(0.5, "%.2f", buffer, sizeof(buffer));
    d_tests_report((strcmp(buffer, "0.50") == 0),
                   "fractional honours format: 0.5 -> \"0.50\"");

    (void)d_awk_number_to_text(1.0 / 3.0, NULL, buffer, sizeof(buffer));
    d_tests_report((strcmp(buffer, "0.333333") == 0),
                   "default CONVFMT is %.6g");

    // an integral value renders in full well past 2^53, as awk does
    (void)d_awk_number_to_text(1e17, NULL, buffer, sizeof(buffer));
    d_tests_report((strcmp(buffer, "100000000000000000") == 0),
                   "1e17 renders in full, not as 1e+17");

    (void)d_awk_number_to_text(9007199254740993.0, NULL, buffer,
                               sizeof(buffer));
    d_tests_report((strcmp(buffer, "9007199254740992") == 0),
                   "past 2^53 the double's own value is rendered");

    // a value beyond long long falls back to the format rather than trapping
    (void)d_awk_number_to_text(1e300, NULL, buffer, sizeof(buffer));
    d_tests_report((strcmp(buffer, "1e+300") == 0),
                   "beyond long long, the format applies");

    return;
}


/*
d_tests_comparison_matrix
  brief description
  Tests the following:
  - every pair of tags, in both orders, against a known expected ordering
  - the strnum rule: a field "10" equals 10, a string constant "10" does not
    compare numerically against 9 and therefore orders before it as text
  - an uninitialised value is both the empty string and zero
  - an extern value behaves as a field, so a node whose text is numeric
    compares numerically
*/
static void
d_tests_comparison_matrix(void)
{
    printf("comparison matrix\n");

    struct d_awk_value* uninit = d_awk_value_new();
    struct d_awk_value* num10  = d_awk_value_new();
    struct d_awk_value* num9   = d_awk_value_new();
    struct d_awk_value* num0   = d_awk_value_new();
    struct d_awk_value* str10  = d_awk_value_new();
    struct d_awk_value* str9   = d_awk_value_new();
    struct d_awk_value* strmt  = d_awk_value_new();
    struct d_awk_value* snum10 = d_awk_value_new();
    struct d_awk_value* snum9  = d_awk_value_new();
    struct d_awk_value* sinp   = d_awk_value_new();

    d_awk_value_set_number(num10, 10.0);
    d_awk_value_set_number(num9, 9.0);
    d_awk_value_set_number(num0, 0.0);
    (void)d_awk_value_set_string(str10, "10", 2);
    (void)d_awk_value_set_string(str9, "9", 1);
    (void)d_awk_value_set_string(strmt, "", 0);
    (void)d_awk_value_set_input(snum10, "10", 2);
    (void)d_awk_value_set_input(snum9, "9", 1);
    (void)d_awk_value_set_input(sinp, "abc", 3);

    d_tests_report((d_awk_value_tag_of(snum10) == D_AWK_VAL_STRNUM),
                   "input \"10\" tags as STRNUM");
    d_tests_report((d_awk_value_tag_of(sinp) == D_AWK_VAL_STRING),
                   "input \"abc\" tags as STRING");
    d_tests_report((d_awk_value_tag_of(str10) == D_AWK_VAL_STRING),
                   "constant \"10\" tags as STRING, not STRNUM");

    static const char* const names[] =
    {
        "uninit", "num10", "num9", "num0",
        "str10", "str9", "strmt", "snum10", "snum9"
    };

    struct d_awk_value* values[] =
    {
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
    };

    values[0] = uninit;
    values[1] = num10;
    values[2] = num9;
    values[3] = num0;
    values[4] = str10;
    values[5] = str9;
    values[6] = strmt;
    values[7] = snum10;
    values[8] = snum9;

    // Expected orderings, row by row: -1, 0 or 1 for left against right.
    // The cells that matter are the numbers against the string constants. A
    // number against a STRING is a TEXT comparison, so 10 against "9" orders
    // BELOW ("10" < "9" byte by byte) while 9 against "10" orders ABOVE; both
    // invert once the right operand is a field, because that pair is numeric.
    // Four cells of this table were transcribed backwards on the first pass
    // and the implementation was right -- which is the argument for writing
    // the matrix out rather than spot-checking it.
    static const int expected[9][9] =
    {
        /*            unin num10 num9 num0 str10 str9 strmt snum10 snum9 */
        /* uninit */ {   0,  -1,  -1,   0,   -1,  -1,    0,    -1,   -1 },
        /* num10  */ {   1,   0,   1,   1,    0,  -1,    1,     0,    1 },
        /* num9   */ {   1,  -1,   0,   1,    1,   0,    1,    -1,    0 },
        /* num0   */ {   0,  -1,  -1,   0,   -1,  -1,    1,    -1,   -1 },
        /* str10  */ {   1,   0,  -1,   1,    0,  -1,    1,     0,   -1 },
        /* str9   */ {   1,   1,   0,   1,    1,   0,    1,     1,    0 },
        /* strmt  */ {   0,  -1,  -1,  -1,   -1,  -1,    0,    -1,   -1 },
        /* snum10 */ {   1,   0,   1,   1,    0,  -1,    1,     0,    1 },
        /* snum9  */ {   1,  -1,   0,   1,    1,   0,    1,    -1,    0 }
    };

    // every pair, in both orders
    for (size_t left = 0; left < 9u; ++left)
    {
        for (size_t right = 0; right < 9u; ++right)
        {
            const int result = d_awk_value_compare(values[left],
                                                   values[right],
                                                   NULL);
            const int sign   = (result < 0) ? -1 : ((result > 0) ? 1 : 0);

            char label[96];

            snprintf(label,
                     sizeof(label),
                     "compare(%s, %s) == %d, got %d",
                     names[left],
                     names[right],
                     expected[left][right],
                     sign);

            d_tests_report((sign == expected[left][right]), label);
        }
    }

    d_awk_value_free(uninit);
    d_awk_value_free(num10);
    d_awk_value_free(num9);
    d_awk_value_free(num0);
    d_awk_value_free(str10);
    d_awk_value_free(str9);
    d_awk_value_free(strmt);
    d_awk_value_free(snum10);
    d_awk_value_free(snum9);
    d_awk_value_free(sinp);

    return;
}


/*
d_tests_truth
  brief description
  Tests the following:
  - the field "0" is false while the string constant "0" is true
  - an uninitialised value is false
  - a non-empty non-numeric string is true
*/
static void
d_tests_truth(void)
{
    printf("truth\n");

    struct d_awk_value* field  = d_awk_value_new();
    struct d_awk_value* text   = d_awk_value_new();
    struct d_awk_value* blank  = d_awk_value_new();
    struct d_awk_value* uninit = d_awk_value_new();

    (void)d_awk_value_set_input(field, "0", 1);
    (void)d_awk_value_set_string(text, "0", 1);
    (void)d_awk_value_set_string(blank, "", 0);

    d_tests_report((!d_awk_value_is_true(field)), "field \"0\" is false");
    d_tests_report((d_awk_value_is_true(text)), "constant \"0\" is true");
    d_tests_report((!d_awk_value_is_true(blank)), "\"\" is false");
    d_tests_report((!d_awk_value_is_true(uninit)), "uninitialised is false");

    d_awk_value_free(field);
    d_awk_value_free(text);
    d_awk_value_free(blank);
    d_awk_value_free(uninit);

    return;
}


static const char*
d_tests_node_text(
    void*   _object,
    void*   _user,
    size_t* _out_length
)
{
    (void)_user;

    const char* const text = (const char*)_object;

    *_out_length = strlen(text);

    return text;
}


/*
d_tests_extern
  brief description
  Tests the following:
  - an extern value renders as the text its callback supplies
  - an extern value whose text is numeric compares numerically, so the same
    expression behaves alike over a flat record and over a tree node
*/
static void
d_tests_extern(void)
{
    printf("extern values\n");

    struct d_awk_value* node   = d_awk_value_new();
    struct d_awk_value* number = d_awk_value_new();
    char                body[] = "10";

    d_awk_value_set_extern(node, body, d_tests_node_text, NULL);
    d_awk_value_set_number(number, 10.0);

    size_t length = 0;

    d_tests_report((strcmp(d_awk_value_text(node, NULL, &length), "10") == 0),
                   "extern renders as its callback's text");
    d_tests_report((length == 2u), "extern reports its length");
    d_tests_report((d_awk_value_compare(node, number, NULL) == 0),
                   "extern \"10\" compares equal to the number 10");
    d_tests_report((d_awk_value_number(node) == 10.0),
                   "extern \"10\" converts to 10");

    d_awk_value_free(node);
    d_awk_value_free(number);

    return;
}


/*
d_tests_arrays
  brief description
  Tests the following:
  - referencing an element creates it while testing membership does not
  - deletion removes an element and leaves the walk usable
  - iteration reaches every live element exactly once
  - a subscript joins its parts with SUBSEP
*/
static void
d_tests_arrays(void)
{
    printf("associative arrays\n");

    struct d_awk_array* array = d_awk_array_new();

    d_tests_report((d_awk_array_find(array, "a", 1) == NULL),
                   "find does not create");
    d_tests_report((d_awk_array_count(array) == 0),
                   "find left the array empty");

    struct d_awk_value* cell = d_awk_array_lookup(array, "a", 1);

    d_tests_report((cell != NULL), "lookup creates");
    d_tests_report((d_awk_array_count(array) == 1), "lookup grew the array");
    d_tests_report((d_awk_value_tag_of(cell) == D_AWK_VAL_UNINIT),
                   "a created element is uninitialised");
    d_tests_report((d_awk_array_find(array, "a", 1) == cell),
                   "find now locates it");

    // enough elements to force at least one rehash
    for (int at = 0; at < 64; ++at)
    {
        char key[16];
        const int written = snprintf(key, sizeof(key), "k%d", at);

        struct d_awk_value* slot = d_awk_array_lookup(array,
                                                      key,
                                                      (size_t)written);

        d_awk_value_set_number(slot, (double)at);
    }

    d_tests_report((d_awk_array_count(array) == 65u),
                   "65 elements after growth");

    struct d_awk_value* probe = d_awk_array_find(array, "k40", 3);

    d_tests_report(((probe) && (d_awk_value_number(probe) == 40.0)),
                   "values survive rehashing");

    d_tests_report((d_awk_array_delete(array, "k40", 3)), "delete reports hit");
    d_tests_report((!d_awk_array_delete(array, "k40", 3)),
                   "a second delete reports miss");
    d_tests_report((d_awk_array_find(array, "k40", 3) == NULL),
                   "the deleted element is gone");
    d_tests_report((d_awk_array_count(array) == 64u),
                   "the count follows the deletion");

    size_t cursor  = 0;
    size_t visited = 0;

    while (d_awk_array_next(array, &cursor, NULL, NULL, NULL))
    {
        visited++;
    }

    d_tests_report((visited == 64u), "the walk reaches every live element");

    d_awk_array_clear(array);
    d_tests_report((d_awk_array_count(array) == 0), "clear empties the array");

    d_awk_array_free(array);

    // a joined subscript is the parts separated by SUBSEP
    char               key[32];
    const char* const  parts[]   = { "a", "b", "c" };
    const size_t       lengths[] = { 1u, 1u, 1u };
    const size_t       needed    = d_awk_subscript_join(key,
                                                        sizeof(key),
                                                        parts,
                                                        lengths,
                                                        3u,
                                                        NULL);

    d_tests_report((needed == 5u), "a three-part subscript needs five bytes");
    d_tests_report((memcmp(key, "a\034b\034c", 5) == 0),
                   "the parts are joined with SUBSEP");

    return;
}


/*
main
  Runs every test group and reports the aggregate result.

Parameter(s):
  none.
Return:
  Zero when every check passed, and one otherwise.
*/
int
main(void)
{
    d_tests_numeric_strings();
    d_tests_conversion();
    d_tests_comparison_matrix();
    d_tests_truth();
    d_tests_extern();
    d_tests_arrays();

    printf("\n%d checks, %d failed\n", g_checks, g_failed);

    return (g_failed == 0) ? 0 : 1;
}
