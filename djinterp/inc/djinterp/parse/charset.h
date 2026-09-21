/******************************************************************************
* djinterp [parse]                                                   charset.h
*
* A set of byte values, as 256 bits.
*   What a grammar writes as `[0-9A-F]` and what a recognizer needs at run time
* are different things. The text is a spelling; the set is the meaning, and
* several spellings mean the same set. Resolving to bits at build time buys
* three things at once: membership becomes one shifted load instead of a scan,
* a class becomes a fixed-size POD that can be interned and hashed, and two
* spellings of one set become one entry -- canonical by construction, which no
* amount of care with the text would give.
*
*   It also carries what an optimiser needs to CHOOSE an encoding. A set that
* is one contiguous range compiles to two compares; a sparse one wants the
* bitmap; a dense one may want a 256-byte table. d_parse_charset_range and
* d_parse_charset_count answer those questions in constant and near-constant
* time, so the decision is a lookup rather than an analysis.
*
*   Requires: c/djinterp.h (qualifier kit).
*
* path:      /inc/djinterp/parse/charset.h
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                          created: 2026.09.19
*                                                          revised: 2026.09.19
******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  THE SET
    -------
    1.  Constants
         1.  D_PARSE_CHARSET_BYTES
         2.  D_PARSE_CHARSET_VALUES
    2.  The type
         1.  d_parse_charset
2.  OPERATIONS
    ----------
    1.  Membership
    2.  Construction
    3.  Set algebra
    4.  Shape
    5.  Notation
*/

#ifndef DJINTERP_PARSE_CHARSET_
#define DJINTERP_PARSE_CHARSET_ 1

// std
#include <stddef.h>         // size_t
#include <stdint.h>         // uint8_t, uint32_t
// djinterp
#include "../c/djinterp.h"  // framework root


//==============================================================================
// 1.  THE SET
//==============================================================================


// 1.1    Constants
//------------------------------------------------------------------------------
// 1.1.1
// D_PARSE_CHARSET_BYTES
//   constant: the size of the bitmap -- one bit per byte value.
#define D_PARSE_CHARSET_BYTES       32u

// 1.1.2
// D_PARSE_CHARSET_VALUES
//   constant: how many distinct values the set can hold.
#define D_PARSE_CHARSET_VALUES      256u


// 1.2    The type
//------------------------------------------------------------------------------
// 1.2.1
// d_parse_charset
//   struct: the bitmap. A plain array so the type is trivially copyable, has
// no padding, and hashes and serialises as its own bytes with nothing to
// canonicalise first.
struct d_parse_charset
{
    uint8_t bits[D_PARSE_CHARSET_BYTES];
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


D_EXTERN_C_BEGIN

// 2.1    Membership
//------------------------------------------------------------------------------
/*
d_parse_charset_test
  Whether a byte value is in the set.
NOTE:
  This is the inner loop of every class-matching operator, so it is a shift
and a mask over one loaded byte and nothing else. No bounds check is needed:
an unsigned char cannot index outside the bitmap.

Parameter(s):
  _set:   the set to query; must not be NULL.
  _value: the byte value to look for.
Return:
  A boolean value corresponding to either:
  - 1, if the value is in the set, or
  - 0, otherwise.
*/
D_INLINE int
d_parse_charset_test(
    const struct d_parse_charset* _set,
    unsigned char                 _value
)
{
    return (_set->bits[_value >> 3] >> (_value & 7u)) & 1u;
}

/*
d_parse_charset_add
  Adds a byte value to the set.

Parameter(s):
  _set:   the set to modify; must not be NULL.
  _value: the byte value to add.
Return:
  none.
*/
D_INLINE void
d_parse_charset_add(
    struct d_parse_charset* _set,
    unsigned char           _value
)
{
    _set->bits[_value >> 3] |= (uint8_t)(1u << (_value & 7u));

    return;
}

/*
d_parse_charset_remove
  Removes a byte value from the set.

Parameter(s):
  _set:   the set to modify; must not be NULL.
  _value: the byte value to remove.
Return:
  none.
*/
D_INLINE void
d_parse_charset_remove(
    struct d_parse_charset* _set,
    unsigned char           _value
)
{
    _set->bits[_value >> 3] &= (uint8_t)~(1u << (_value & 7u));

    return;
}

// 2.2    Construction
//------------------------------------------------------------------------------
void            d_parse_charset_clear(struct d_parse_charset* _set);
void            d_parse_charset_fill(struct d_parse_charset* _set);
void            d_parse_charset_add_range(struct d_parse_charset* _set,
                                          unsigned char           _low,
                                          unsigned char           _high);

// 2.3    Set algebra
//------------------------------------------------------------------------------
void            d_parse_charset_negate(struct d_parse_charset* _set);
void            d_parse_charset_unite(struct d_parse_charset*       _set,
                                      const struct d_parse_charset* _other);
void            d_parse_charset_intersect(struct d_parse_charset*       _set,
                                          const struct d_parse_charset* _other);
void            d_parse_charset_subtract(struct d_parse_charset*       _set,
                                         const struct d_parse_charset* _other);
int             d_parse_charset_equal(const struct d_parse_charset* _set,
                                      const struct d_parse_charset* _other);
int             d_parse_charset_disjoint(const struct d_parse_charset* _set,
                                         const struct d_parse_charset* _other);

// 2.4    Shape
//------------------------------------------------------------------------------
//   What an optimiser asks before choosing how to compile a class.
uint32_t        d_parse_charset_count(const struct d_parse_charset* _set);
int             d_parse_charset_range(const struct d_parse_charset* _set,
                                      unsigned char*                _low,
                                      unsigned char*                _high);

// 2.5    Notation
//------------------------------------------------------------------------------
//   Text in and text out, for reading a grammar and for disassembling one.
// The rendering is canonical: it folds runs back into ranges, so two spellings
// of one set render identically.
D_NODISCARD int d_parse_charset_parse(struct d_parse_charset* _set,
                                      const char*             _spec);
size_t          d_parse_charset_render(const struct d_parse_charset* _set,
                                       char*                         _out,
                                       size_t                        _size);

D_EXTERN_C_END


#endif  // DJINTERP_PARSE_CHARSET_
