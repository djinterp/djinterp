/******************************************************************************
* djinterp [parse]                                                   charset.c
*
*   Definitions for the non-inline declarations in charset.h.
*
*
* path:      /src/djinterp/parse/charset.c
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/
#include "../../../inc/djinterp/parse/charset.h"  // corresponding header
// std
#include <stdio.h>   // snprintf
#include <string.h>  // memset, memcmp


/*
d_parse_charset_clear
  Empties a set.

Parameter(s):
  _set: the set to empty; ignored if NULL.
Return:
  none.
*/
void
d_parse_charset_clear(
    struct d_parse_charset* _set
)
{
    if (!_set)
    {
        return;
    }

    memset(_set->bits, 0, sizeof(_set->bits));

    return;
}


/*
d_parse_charset_fill
  Puts every byte value into a set.

Parameter(s):
  _set: the set to fill; ignored if NULL.
Return:
  none.
*/
void
d_parse_charset_fill(
    struct d_parse_charset* _set
)
{
    if (!_set)
    {
        return;
    }

    memset(_set->bits, 0xFF, sizeof(_set->bits));

    return;
}


/*
d_parse_charset_add_range
  Adds an inclusive range of byte values to a set.
NOTE:
  A reversed range is normalised rather than rejected, because `[z-a]` in a
grammar is a typo the frontend should diagnose, not a condition this level
should have an opinion about.

Parameter(s):
  _set:  the set to modify; ignored if NULL.
  _low:  the first value of the range.
  _high: the last value of the range, inclusive.
Return:
  none.
*/
void
d_parse_charset_add_range(
    struct d_parse_charset* _set,
    unsigned char           _low,
    unsigned char           _high
)
{
    if (!_set)
    {
        return;
    }

    const unsigned first = (_low <= _high) ? _low : _high;
    const unsigned last  = (_low <= _high) ? _high : _low;

    // walk the range inclusively; `last` may be 255, so the counter is wider
    for (unsigned value = first; value <= last; value++)
    {
        d_parse_charset_add(_set, (unsigned char)value);
    }

    return;
}


/*
d_parse_charset_negate
  Replaces a set with its complement over all 256 byte values.

Parameter(s):
  _set: the set to complement; ignored if NULL.
Return:
  none.
*/
void
d_parse_charset_negate(
    struct d_parse_charset* _set
)
{
    if (!_set)
    {
        return;
    }

    for (uint32_t index = 0u; index < D_PARSE_CHARSET_BYTES; index++)
    {
        _set->bits[index] = (uint8_t)~_set->bits[index];
    }

    return;
}


/*
d_parse_charset_unite
  Adds every member of one set to another.

Parameter(s):
  _set:   the set to modify; ignored if NULL.
  _other: the set to add; ignored if NULL.
Return:
  none.
*/
void
d_parse_charset_unite(
    struct d_parse_charset*       _set,
    const struct d_parse_charset* _other
)
{
    if ( (!_set) ||
         (!_other) )
    {
        return;
    }

    for (uint32_t index = 0u; index < D_PARSE_CHARSET_BYTES; index++)
    {
        _set->bits[index] |= _other->bits[index];
    }

    return;
}


/*
d_parse_charset_intersect
  Removes from a set every value not also in another.

Parameter(s):
  _set:   the set to modify; ignored if NULL.
  _other: the set to intersect with; ignored if NULL.
Return:
  none.
*/
void
d_parse_charset_intersect(
    struct d_parse_charset*       _set,
    const struct d_parse_charset* _other
)
{
    if ( (!_set) ||
         (!_other) )
    {
        return;
    }

    for (uint32_t index = 0u; index < D_PARSE_CHARSET_BYTES; index++)
    {
        _set->bits[index] &= _other->bits[index];
    }

    return;
}


/*
d_parse_charset_subtract
  Removes from a set every value that is in another.

Parameter(s):
  _set:   the set to modify; ignored if NULL.
  _other: the set whose members to remove; ignored if NULL.
Return:
  none.
*/
void
d_parse_charset_subtract(
    struct d_parse_charset*       _set,
    const struct d_parse_charset* _other
)
{
    if ( (!_set) ||
         (!_other) )
    {
        return;
    }

    for (uint32_t index = 0u; index < D_PARSE_CHARSET_BYTES; index++)
    {
        _set->bits[index] &= (uint8_t)~_other->bits[index];
    }

    return;
}


/*
d_parse_charset_equal
  Whether two sets have the same members.
NOTE:
  Because a set is its bits and nothing else, this is the equality the intern
pool uses -- which is what makes two spellings of one class share an entry.

Parameter(s):
  _set:   the first set; may be NULL.
  _other: the second set; may be NULL.
Return:
  A boolean value corresponding to either:
  - 1, if both are the same set or both are NULL, or
  - 0, otherwise.
*/
int
d_parse_charset_equal(
    const struct d_parse_charset* _set,
    const struct d_parse_charset* _other
)
{
    if (_set == _other)
    {
        return 1;
    }

    if ( (!_set) ||
         (!_other) )
    {
        return 0;
    }

    return (memcmp(_set->bits, _other->bits, D_PARSE_CHARSET_BYTES) == 0)
           ? 1
           : 0;
}


/*
d_parse_charset_disjoint
  Whether two sets share no member.
NOTE:
  This is the first-set test an ordered choice needs before its alternatives
may be compiled as a dispatch rather than as a cascade, so it is the cheap
sufficient condition for that transform.

Parameter(s):
  _set:   the first set; may be NULL, which is disjoint with anything.
  _other: the second set; may be NULL.
Return:
  A boolean value corresponding to either:
  - 1, if no value is in both sets, or
  - 0, otherwise.
*/
int
d_parse_charset_disjoint(
    const struct d_parse_charset* _set,
    const struct d_parse_charset* _other
)
{
    if ( (!_set) ||
         (!_other) )
    {
        return 1;
    }

    for (uint32_t index = 0u; index < D_PARSE_CHARSET_BYTES; index++)
    {
        if ((_set->bits[index] & _other->bits[index]) != 0u)
        {
            return 0;
        }
    }

    return 1;
}


/*
d_parse_charset_count
  How many byte values are in a set.
NOTE:
  Density decides an encoding: a handful of members compiles to compares, a
crowd to a table. This is that measurement.

Parameter(s):
  _set: the set to measure; may be NULL.
Return:
  The number of members, from 0 to 256.
*/
uint32_t
d_parse_charset_count(
    const struct d_parse_charset* _set
)
{
    if (!_set)
    {
        return 0u;
    }

    uint32_t total = 0u;

    // count bits a byte at a time; the table is small enough to stay hot and
    // avoids depending on a popcount builtin this framework does not require
    static const uint8_t nibble[16] =
    {
        0u, 1u, 1u, 2u, 1u, 2u, 2u, 3u, 1u, 2u, 2u, 3u, 2u, 3u, 3u, 4u
    };

    for (uint32_t index = 0u; index < D_PARSE_CHARSET_BYTES; index++)
    {
        const uint8_t byte = _set->bits[index];

        total += nibble[byte & 0x0Fu];
        total += nibble[(byte >> 4) & 0x0Fu];
    }

    return total;
}


/*
d_parse_charset_range
  Whether a set is exactly one contiguous run of values, and which.
NOTE:
  A set that passes this compiles to two compares -- `c - low <= high - low`
after the usual trick -- instead of a table load, which is the single biggest
per-class win available to a code generator. The empty set is not a range.

Parameter(s):
  _set:  the set to inspect; may be NULL.
  _low:  receives the first value of the run; optional.
  _high: receives the last value of the run; optional.
Return:
  A boolean value corresponding to either:
  - 1, if the set is one non-empty contiguous run, or
  - 0, otherwise.
*/
int
d_parse_charset_range(
    const struct d_parse_charset* _set,
    unsigned char*                _low,
    unsigned char*                _high
)
{
    if (!_set)
    {
        return 0;
    }

    int first = -1;
    int last  = -1;

    // find the extent of the members, and reject the moment a gap appears
    // inside it
    for (unsigned value = 0u; value < D_PARSE_CHARSET_VALUES; value++)
    {
        if (d_parse_charset_test(_set, (unsigned char)value))
        {
            // a member after a gap means more than one run
            if ( (last >= 0) &&
                 ((int)value != (last + 1)) )
            {
                return 0;
            }

            if (first < 0)
            {
                first = (int)value;
            }

            last = (int)value;
        }
    }

    // the empty set is not a range, and has no bounds to report
    if (first < 0)
    {
        return 0;
    }

    if (_low)
    {
        *_low = (unsigned char)first;
    }

    if (_high)
    {
        *_high = (unsigned char)last;
    }

    return 1;
}


/*
d_parse_charset_parse
  Builds a set from the body of a class notation -- the text between the
brackets.
NOTE:
  A leading `^` negates. `a-z` is a range; a `-` first, last, or after a range
is a literal. A backslash escapes the next character and understands the usual
\n \r \t \0 \\ spellings. Anything else stands for itself, so this level makes
no judgement about what a grammar may write.

Parameter(s):
  _set:  the set to build into; must not be NULL. Cleared first.
  _spec: the class body, without the surrounding brackets; must not be NULL.
Return:
  0 on success; -1 on a NULL argument or a trailing backslash with nothing to
escape.
*/
int
d_parse_charset_parse(
    struct d_parse_charset* _set,
    const char*             _spec
)
{
    if ( (!_set) ||
         (!_spec) )
    {
        return -1;
    }

    d_parse_charset_clear(_set);

    const char* cursor = _spec;
    int         negate = 0;

    // a leading caret negates the whole set, as in every class notation
    if (*cursor == '^')
    {
        negate = 1;
        cursor++;
    }

    while (*cursor != '\0')
    {
        unsigned char low = (unsigned char)*cursor;

        // an escape stands for the character it names, never for notation
        if (*cursor == '\\')
        {
            cursor++;

            if (*cursor == '\0')
            {
                return -1;
            }

            switch (*cursor)
            {
                case 'n': low = (unsigned char)'\n'; break;
                case 'r': low = (unsigned char)'\r'; break;
                case 't': low = (unsigned char)'\t'; break;
                case '0': low = (unsigned char)'\0'; break;
                default:  low = (unsigned char)*cursor; break;
            }
        }

        cursor++;

        // a hyphen forms a range only when a character follows it
        if ( (*cursor == '-') &&
             (*(cursor + 1) != '\0') )
        {
            cursor++;

            unsigned char high = (unsigned char)*cursor;

            if (*cursor == '\\')
            {
                cursor++;

                if (*cursor == '\0')
                {
                    return -1;
                }

                switch (*cursor)
                {
                    case 'n': high = (unsigned char)'\n'; break;
                    case 'r': high = (unsigned char)'\r'; break;
                    case 't': high = (unsigned char)'\t'; break;
                    case '0': high = (unsigned char)'\0'; break;
                    default:  high = (unsigned char)*cursor; break;
                }
            }

            cursor++;

            d_parse_charset_add_range(_set, low, high);
        }
        else
        {
            d_parse_charset_add(_set, low);
        }
    }

    if (negate)
    {
        d_parse_charset_negate(_set);
    }

    return 0;
}


/*
d_parse_charset_internal_escape
  Writes one byte value in the class notation, escaping what would otherwise
read as syntax.

Parameter(s):
  _value: the value to write.
  _out:   the buffer to write into; may be NULL when _size is 0.
  _size:  the size of _out in bytes, including the terminator.
Return:
  The number of characters the value would occupy, as snprintf reports it.
*/
static size_t
d_parse_charset_internal_escape(
    unsigned char _value,
    char*         _out,
    size_t        _size
)
{
    int written = 0;

    switch (_value)
    {
        case '\n': written = snprintf(_out, _size, "\\n");  break;
        case '\r': written = snprintf(_out, _size, "\\r");  break;
        case '\t': written = snprintf(_out, _size, "\\t");  break;
        case '\\': written = snprintf(_out, _size, "\\\\"); break;
        case '-':  written = snprintf(_out, _size, "\\-");  break;
        case ']':  written = snprintf(_out, _size, "\\]");  break;
        case '^':  written = snprintf(_out, _size, "\\^");  break;
        default:
            // printable ASCII stands for itself; anything else is numeric, so
            // a rendering never depends on the reader's encoding
            if ( (_value >= 0x20u) &&
                 (_value < 0x7Fu) )
            {
                written = snprintf(_out, _size, "%c", (char)_value);
            }
            else
            {
                written = snprintf(_out, _size, "\\x%02X", (unsigned)_value);
            }
            break;
    }

    return (written < 0) ? 0u : (size_t)written;
}


/*
d_parse_charset_render
  Writes a set in the class notation, brackets included.
NOTE:
  The rendering folds runs back into ranges and is therefore canonical: two
spellings that mean one set render identically, which is what makes a
disassembly comparable across builds. A set with more than half the values is
rendered negated, so `[^\n]` does not come out as 255 characters.

Parameter(s):
  _set:  the set to render; may be NULL, which renders as `[]`.
  _out:  the buffer to write into; may be NULL when _size is 0.
  _size: the size of _out in bytes, including the terminator.
Return:
  The number of characters the full rendering would occupy, excluding the
terminator -- so a return of _size or more means the output was truncated.
*/
size_t
d_parse_charset_render(
    const struct d_parse_charset* _set,
    char*                         _out,
    size_t                        _size
)
{
    struct d_parse_charset local;

    d_parse_charset_clear(&local);

    if (_set)
    {
        local = *_set;
    }

    const int negated = (d_parse_charset_count(&local) >
                         (D_PARSE_CHARSET_VALUES / 2u));

    if (negated)
    {
        d_parse_charset_negate(&local);
    }

    size_t needed = 0u;

    // a small local writer: every append reports what it would have needed,
    // so the return stays correct once the buffer is full
    char*  cursor    = _out;
    size_t remaining = _size;

    #define D_INTERNAL_CHARSET_PUT(literal)                                   \
        do                                                                    \
        {                                                                     \
            const int put = snprintf(cursor, remaining, "%s", (literal));     \
            const size_t grew = (put < 0) ? 0u : (size_t)put;                 \
            needed += grew;                                                   \
            if (grew < remaining)                                             \
            {                                                                 \
                cursor    += grew;                                            \
                remaining -= grew;                                            \
            }                                                                 \
            else                                                              \
            {                                                                 \
                cursor    = NULL;                                             \
                remaining = 0u;                                               \
            }                                                                 \
        } while (0)

    D_INTERNAL_CHARSET_PUT("[");

    if (negated)
    {
        D_INTERNAL_CHARSET_PUT("^");
    }

    unsigned value = 0u;

    // fold each run of members into a single range, so the output is canonical
    while (value < D_PARSE_CHARSET_VALUES)
    {
        if (!d_parse_charset_test(&local, (unsigned char)value))
        {
            value++;

            continue;
        }

        const unsigned start = value;

        while ( (value < D_PARSE_CHARSET_VALUES) &&
                (d_parse_charset_test(&local, (unsigned char)value)) )
        {
            value++;
        }

        const unsigned last = value - 1u;

        char piece[16];

        (void)d_parse_charset_internal_escape((unsigned char)start,
                                              piece,
                                              sizeof(piece));

        D_INTERNAL_CHARSET_PUT(piece);

        // a run of three or more is worth a range; two is not
        if (last > (start + 1u))
        {
            D_INTERNAL_CHARSET_PUT("-");

            (void)d_parse_charset_internal_escape((unsigned char)last,
                                                  piece,
                                                  sizeof(piece));

            D_INTERNAL_CHARSET_PUT(piece);
        }
        else if (last == (start + 1u))
        {
            (void)d_parse_charset_internal_escape((unsigned char)last,
                                                  piece,
                                                  sizeof(piece));

            D_INTERNAL_CHARSET_PUT(piece);
        }
    }

    D_INTERNAL_CHARSET_PUT("]");

    #undef D_INTERNAL_CHARSET_PUT

    return needed;
}
