/*******************************************************************************
* djinterp [c]                                                           ascii.c
*
* ASCII classification and case conversion.
*   Defines the functions ascii.h declares. Each answers for 7-bit ASCII only,
* whatever the locale: a byte outside 0x00-0x7F -- including every negative
* `char` where char is signed -- belongs to no class and converts to itself.
* The classes are those of <ctype.h> in the "C" locale.
*
*
* path:      /src/djinterp/c/text/symbol/ascii.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.30
*                                                            revised: 2026.09.30
*******************************************************************************/
#include "../../../../../inc/djinterp/c/text/symbol/ascii.h"  // corresponding header
// std
#include <stdbool.h>  // bool


/*
d_internal_ascii_code
  The character as an unsigned code, so that a negative char -- a high byte
on a signed-char platform -- lands at 0x80 or above, outside every ASCII
range, rather than below them.
*/
static unsigned int
d_internal_ascii_code(
    char _c
)
{
    return (unsigned int)(unsigned char)_c;
}

bool
d_ascii_char_is_numeric(
    char _c
)
{
    const unsigned int code = d_internal_ascii_code(_c);

    return ( (code >= 0x30u) &&
             (code <= 0x39u) );
}

bool
d_ascii_char_is_alphabetical(
    char _c
)
{
    const unsigned int code = d_internal_ascii_code(_c);

    return ( ( (code >= 0x41u) && (code <= 0x5Au) ) ||
             ( (code >= 0x61u) && (code <= 0x7Au) ) );
}

bool
d_ascii_char_is_alphanumeric(
    char _c
)
{
    return ( (d_ascii_char_is_alphabetical(_c)) ||
             (d_ascii_char_is_numeric(_c)) );
}

/*
d_ascii_char_is_whitespace
  The six characters isspace accepts in the "C" locale: space, and \t \n \v
\f \r, which are 0x09 to 0x0D.
*/
bool
d_ascii_char_is_whitespace(
    char _c
)
{
    const unsigned int code = d_internal_ascii_code(_c);

    return ( (code == 0x20u) ||
             ( (code >= 0x09u) && (code <= 0x0Du) ) );
}

bool
d_ascii_char_is_null(
    char _c
)
{
    return (d_internal_ascii_code(_c) == 0x00u);
}

bool
d_ascii_char_is_control(
    char _c
)
{
    const unsigned int code = d_internal_ascii_code(_c);

    return ( (code <= 0x1Fu) ||
             (code == 0x7Fu) );
}

/*
d_ascii_char_is_printable
  Space counts, as it does for isprint; DEL (0x7F) does not.
*/
bool
d_ascii_char_is_printable(
    char _c
)
{
    const unsigned int code = d_internal_ascii_code(_c);

    return ( (code >= 0x20u) &&
             (code <= 0x7Eu) );
}

/*
d_ascii_char_is_punctuation
  Every printable character that is neither a space nor alphanumeric: the
four runs 0x21-0x2F, 0x3A-0x40, 0x5B-0x60 and 0x7B-0x7E.
*/
bool
d_ascii_char_is_punctuation(
    char _c
)
{
    const unsigned int code = d_internal_ascii_code(_c);

    return ( (code > 0x20u)                       &&
             (code <= 0x7Eu)                      &&
             (!d_ascii_char_is_alphanumeric(_c)) );
}

bool
d_ascii_char_is_hex_digit(
    char _c
)
{
    const unsigned int code = d_internal_ascii_code(_c);

    return ( (d_ascii_char_is_numeric(_c))              ||
             ( (code >= 0x41u) && (code <= 0x46u) )     ||
             ( (code >= 0x61u) && (code <= 0x66u) ) );
}

/*
d_ascii_char_to_upper
  Only a-z move, by the 0x20 that separates the two cases in ASCII.
*/
char
d_ascii_char_to_upper(
    char _c
)
{
    const unsigned int code = d_internal_ascii_code(_c);

    // a lower-case letter moves up by 0x20; anything else is itself
    if ( (code >= 0x61u) &&
         (code <= 0x7Au) )
    {
        return (char)(code - 0x20u);
    }

    return _c;
}

/*
d_ascii_char_to_lower
  Only A-Z move, by the 0x20 that separates the two cases in ASCII.
*/
char
d_ascii_char_to_lower(
    char _c
)
{
    const unsigned int code = d_internal_ascii_code(_c);

    // an upper-case letter moves down by 0x20; anything else is itself
    if ( (code >= 0x41u) &&
         (code <= 0x5Au) )
    {
        return (char)(code + 0x20u);
    }

    return _c;
}
