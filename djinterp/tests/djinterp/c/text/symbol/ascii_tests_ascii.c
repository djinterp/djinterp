/*******************************************************************************
* djinterp [c][text]                                         ascii_tests_ascii.c
*
* The ascii suite: its one block. The references are <ctype.h>'s, in the "C"
* locale, which a program is in until it calls setlocale.
*
*
* path:      /tests/djinterp/c/text/symbol/ascii_tests_ascii.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.11.23
*******************************************************************************/
#include "./ascii_tests.h"  // corresponding header

// std
#include <ctype.h>  // the "C" locale's classes


// dj_tests_ascii_class
//   struct: one predicate and the <ctype.h> class it must agree with.
struct dj_tests_ascii_class
{
    const char* name;              // the class, for its check
    bool        (*mine)(char);     // ascii.h's predicate
    int         (*reference)(int); // <ctype.h>'s, or the suite's own
};

/*
d_tests_ascii_is_null
  The reference for the null class, which <ctype.h> lacks.
*/
static int
d_tests_ascii_is_null(
    int _c
)
{
    return (_c == 0);
}


/*
d_tests_ascii_classes
  The classes.
  Tests the following:
  - numeric, alphabetical, alphanumeric, whitespace, null, control,
    printable, punctuation and hex_digit each agree with their reference on
    all 256 byte values, one check per class
*/
bool
d_tests_ascii_classes(void)
{
    static const struct dj_tests_ascii_class classes[] =
    {
        { "numeric",      &d_ascii_char_is_numeric,      &isdigit  },
        { "alphabetical", &d_ascii_char_is_alphabetical, &isalpha  },
        { "alphanumeric", &d_ascii_char_is_alphanumeric, &isalnum  },
        { "whitespace",   &d_ascii_char_is_whitespace,   &isspace  },
        { "null",         &d_ascii_char_is_null,
                          &d_tests_ascii_is_null },
        { "control",      &d_ascii_char_is_control,      &iscntrl  },
        { "printable",    &d_ascii_char_is_printable,    &isprint  },
        { "punctuation",  &d_ascii_char_is_punctuation,  &ispunct  },
        { "hex_digit",    &d_ascii_char_is_hex_digit,    &isxdigit }
    };
    bool   ok = true;
    size_t k;
    int    byte;

    for (k = 0u; k < (sizeof(classes) / sizeof(classes[0])); ++k)
    {
        bool agrees = true;

        for (byte = 0; byte < 256; ++byte)
        {
            const bool mine      = classes[k].mine((char)byte);
            const bool reference = (classes[k].reference(byte) != 0);

            agrees = ( (agrees) && (mine == reference) );
        }

        ok = d_test_suite_require(agrees,
                                  classes[k].name,
                                  __FILE__,
                                  __LINE__) && ok;
    }

    return ok;
}

/*
d_tests_ascii_case
  Case.
  Tests the following:
  - to_upper agrees with toupper, and to_lower with tolower, on all 256
    byte values
*/
bool
d_tests_ascii_case(void)
{
    bool upper = true;
    bool lower = true;
    bool ok    = true;
    int  byte;

    for (byte = 0; byte < 256; ++byte)
    {
        upper = ( (upper) &&
                  ((unsigned char)d_ascii_char_to_upper((char)byte) ==
                   (unsigned char)toupper(byte)) );
        lower = ( (lower) &&
                  ((unsigned char)d_ascii_char_to_lower((char)byte) ==
                   (unsigned char)tolower(byte)) );
    }

    ok = D_TEST_EXPECT(upper) && ok;
    ok = D_TEST_EXPECT(lower) && ok;

    return ok;
}

/*
d_tests_ascii_fixed
  Fixed points.
  Tests the following:
  - 'q' rises to 'Q' and 'Q' falls to 'q'; '7' and 0xE9 stay as they are
  - '7' is numeric and a hex digit, 'g' neither; '\t' is whitespace
*/
bool
d_tests_ascii_fixed(void)
{
    bool ok = true;

    ok = D_TEST_EXPECT(d_ascii_char_to_upper('q') == 'Q') && ok;
    ok = D_TEST_EXPECT(d_ascii_char_to_lower('Q') == 'q') && ok;
    ok = D_TEST_EXPECT(d_ascii_char_to_upper('7') == '7') && ok;
    ok = D_TEST_EXPECT(d_ascii_char_to_upper((char)0xE9) == (char)0xE9) &&
         ok;
    ok = D_TEST_EXPECT(d_ascii_char_is_numeric('7')) && ok;
    ok = D_TEST_EXPECT(d_ascii_char_is_hex_digit('7')) && ok;
    ok = D_TEST_EXPECT(!d_ascii_char_is_hex_digit('g')) && ok;
    ok = D_TEST_EXPECT(d_ascii_char_is_whitespace('\t')) && ok;

    return ok;
}
