/*******************************************************************************
* djinterp [c]                                                         t_ascii.c
*
*   Conformance harness for ascii.h: every function against <ctype.h> in the
* "C" locale, for all 256 byte values -- which also pins down that no byte
* above 0x7F falls in any class or changes case.
*
*   Build (from the repo root), at any language level from C99:
*     cc -std=c99 -Wall -Wextra -Werror -Iinc                                  \
*        src/djinterp/c/text/symbol/ascii.c                                    \
*        tests/djinterp/c/text/symbol/t_ascii.c -o t_ascii
*
*
* path:      /tests/djinterp/c/text/symbol/t_ascii.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.09.30
*******************************************************************************/
// std
#include <ctype.h>    // the "C" locale reference classes
#include <stdbool.h>  // bool
#include <stdio.h>    // printf
// djinterp
#include "../../../../../inc/djinterp/c/text/symbol/ascii.h"  // unit under test


static int g_checks   = 0;
static int g_failures = 0;

/*
d_tests_ascii_expect
  Counts one check, and prints it when it fails.
*/
static void
d_tests_ascii_expect(
    bool        _ok,
    const char* _what
)
{
    ++g_checks;

    // report a failure with what was being checked
    if (!_ok)
    {
        ++g_failures;
        printf("  FAIL: %s\n", _what);
    }

    return;
}

// d_tests_ascii_class
//   struct: one predicate and the <ctype.h> class it must agree with.
struct d_tests_ascii_class
{
    const char* name;
    bool        (*mine)(char);
    int         (*reference)(int);
};

static int
d_tests_ascii_is_null(
    int _c
)
{
    return (_c == 0);
}

int
main(void)
{
    const struct d_tests_ascii_class classes[] =
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

    // each class agrees with its reference on every byte
    for (size_t k = 0u; k < (sizeof(classes) / sizeof(classes[0])); ++k)
    {
        bool agrees = true;

        for (int byte = 0; byte < 256; ++byte)
        {
            const bool mine = classes[k].mine((char)byte);
            const bool ref  = (classes[k].reference(byte) != 0);

            agrees = agrees && (mine == ref);
        }

        d_tests_ascii_expect(agrees, classes[k].name);
    }

    // case conversion agrees with toupper and tolower on every byte
    {
        bool upper = true;
        bool lower = true;

        for (int byte = 0; byte < 256; ++byte)
        {
            upper = upper &&
                    ((unsigned char)d_ascii_char_to_upper((char)byte) ==
                     (unsigned char)toupper(byte));
            lower = lower &&
                    ((unsigned char)d_ascii_char_to_lower((char)byte) ==
                     (unsigned char)tolower(byte));
        }

        d_tests_ascii_expect(upper, "to_upper");
        d_tests_ascii_expect(lower, "to_lower");
    }

    // a few fixed points, independent of the reference
    d_tests_ascii_expect( (d_ascii_char_to_upper('q') == 'Q') &&
                          (d_ascii_char_to_lower('Q') == 'q') &&
                          (d_ascii_char_to_upper('7') == '7'),
                          "fixed points" );

    printf("t_ascii: %d checks, %d failed\n", g_checks, g_failures);

    return (g_failures == 0) ? 0 : 1;
}
