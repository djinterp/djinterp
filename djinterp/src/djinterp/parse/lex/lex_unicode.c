/*******************************************************************************
* djinterp [parse]                                                 lex_unicode.c
*
* Definitions for lex_unicode.h.
*   Every table is a sorted array of disjoint closed ranges, searched by
* bisection, so a lookup costs about ten comparisons for the largest table and
* no table needs a second representation.  The tables themselves are in the
* generated lex_unicode_data.h beside this file, which is private: nothing
* outside this translation unit sees a range.
*
*
* path:      /src/djinterp/parse/lex/lex_unicode.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.22
*                                                            revised: 2026.10.02
*******************************************************************************/
#include "../../../../inc/djinterp/parse/lex/lex_unicode.h"  // corresponding header
// std
#include <stdbool.h>  // bool
#include <stddef.h>   // size_t
#include <string.h>   // memcmp
// djinterp
#include "./lex_unicode_data.h"  // generated range tables, d_lex_unicode_range
// re_std
#include "../../../../inc/re_std/cstdint/dstdint.h"  // uint32_t

// D_INTERNAL_COUNT
//   macro: the element count of a static array.
#define D_INTERNAL_COUNT(_array)  (sizeof(_array) / sizeof((_array)[0]))


/**
 * @brief Reports whether a code point lies in a range table.
 *
 * @param[in] _table       the table, sorted and disjoint.
 * @param[in] _count       its element count.
 * @param[in] _code_point  the code point to find.
 * @return `true` if some range holds the code point, `false` otherwise.
 */
static bool
d_internal_in_table(
    const struct d_lex_unicode_range* _table,
    size_t                            _count,
    uint32_t                          _code_point
)
{
    size_t low  = 0u;
    size_t high = _count;

    // bisect on the range's last element, so the survivor is the only range
    // that can contain the code point
    while (low < high)
    {
        const size_t middle = low + ((high - low) / 2u);

        if (_table[middle].last < _code_point)
        {
            low = middle + 1u;
        }
        else
        {
            high = middle;
        }
    }

    return ( (low < _count) &&
             (_table[low].first <= _code_point) );
}


/*
d_lex_utf8_length
  The encoded width, by the boundaries of Table 3-6.
*/
size_t
d_lex_utf8_length(
    uint32_t _code_point
)
{
    // a surrogate or an out-of-range value has no UTF-8 form
    if (!d_lex_unicode_is_scalar(_code_point))
    {
        return 0u;
    }

    if (_code_point < 0x80u)
    {
        return 1u;
    }

    if (_code_point < 0x800u)
    {
        return 2u;
    }

    return (_code_point < 0x10000u) ? 3u : 4u;
}


/*
d_lex_unicode_is_scalar
  A scalar value is any code point but a surrogate.
*/
bool
d_lex_unicode_is_scalar(
    uint32_t _code_point
)
{
    return ( (_code_point <= 0x10FFFFu) &&
             ( (_code_point < 0xD800u) ||
               (_code_point > 0xDFFFu) ) );
}


/*
d_lex_unicode_is_id_start
  The annex sets list what may begin a name directly; XID_Start is the UAX #31
equivalent.  ASCII is answered false for every set because the scanner tests
the basic characters itself and must not accept `$` or a digit by accident of
a table.
*/
bool
d_lex_unicode_is_id_start(
    unsigned _set,
    uint32_t _code_point
)
{
    // the basic characters are the scanner's to test
    if (_code_point < 0x80u)
    {
        return false;
    }

    switch (_set)
    {
        case D_LEX_IDSET_C99:
            return d_internal_in_table(d_internal_c99_start,
                                       D_INTERNAL_COUNT(d_internal_c99_start),
                                       _code_point);
        case D_LEX_IDSET_C11:
            return d_internal_in_table(d_internal_c11_start,
                                       D_INTERNAL_COUNT(d_internal_c11_start),
                                       _code_point);
        case D_LEX_IDSET_XID:
            return d_internal_in_table(d_internal_xid_start,
                                       D_INTERNAL_COUNT(d_internal_xid_start),
                                       _code_point);
        default:
            return false;
    }
}


/*
d_lex_unicode_is_id_continue
  Each annex keeps its not-initially characters in a second table, so a
continuing character is in either; XID_Continue already includes XID_Start.
*/
bool
d_lex_unicode_is_id_continue(
    unsigned _set,
    uint32_t _code_point
)
{
    // the basic characters are the scanner's to test
    if (_code_point < 0x80u)
    {
        return false;
    }

    switch (_set)
    {
        case D_LEX_IDSET_C99:
            return ( d_lex_unicode_is_id_start(_set,
                                               _code_point) ||
                     d_internal_in_table(
                         d_internal_c99_digit,
                         D_INTERNAL_COUNT(d_internal_c99_digit),
                         _code_point) );
        case D_LEX_IDSET_C11:
            return ( d_lex_unicode_is_id_start(_set,
                                               _code_point) ||
                     d_internal_in_table(
                         d_internal_c11_nostart,
                         D_INTERNAL_COUNT(d_internal_c11_nostart),
                         _code_point) );
        case D_LEX_IDSET_XID:
            return d_internal_in_table(
                       d_internal_xid_continue,
                       D_INTERNAL_COUNT(d_internal_xid_continue),
                       _code_point);
        default:
            return false;
    }
}


/*
d_lex_unicode_nfc_check
  The two property tables are disjoint by definition, so the order of the
lookups does not matter; NO is tried first only because it is the answer that
ends the question.
*/
int
d_lex_unicode_nfc_check(
    uint32_t _code_point
)
{
    if (d_internal_in_table(d_internal_nfc_no,
                            D_INTERNAL_COUNT(d_internal_nfc_no),
                            _code_point))
    {
        return D_LEX_NFC_NO;
    }

    if (d_internal_in_table(d_internal_nfc_maybe,
                            D_INTERNAL_COUNT(d_internal_nfc_maybe),
                            _code_point))
    {
        return D_LEX_NFC_MAYBE;
    }

    return D_LEX_NFC_YES;
}


// D_INTERNAL_S_BASE and the rest
//   constant: Hangul syllables are composed and decomposed arithmetically,
// UAX #15 section 3.12, and appear in no table.
#define D_INTERNAL_S_BASE   0xAC00u
#define D_INTERNAL_L_BASE   0x1100u
#define D_INTERNAL_V_BASE   0x1161u
#define D_INTERNAL_T_BASE   0x11A7u
#define D_INTERNAL_L_COUNT  19u
#define D_INTERNAL_V_COUNT  21u
#define D_INTERNAL_T_COUNT  28u
#define D_INTERNAL_N_COUNT  (D_INTERNAL_V_COUNT * D_INTERNAL_T_COUNT)
#define D_INTERNAL_S_COUNT  (D_INTERNAL_L_COUNT * D_INTERNAL_N_COUNT)

// D_INTERNAL_NFC_BUFFER
//   constant: code points the full check can hold once decomposed.  No
// canonical decomposition is longer than four, so a name of up to 128 code
// points -- the scanner's bound -- always fits.
#define D_INTERNAL_NFC_BUFFER 512


/**
 * @brief Reports a code point's canonical combining class.
 *
 * @param[in] _code  the code point.
 * @return the class, zero for a starter.
 */
static uint32_t
d_internal_class_of(
    uint32_t _code
)
{
    size_t low  = 0u;
    size_t high = D_INTERNAL_COUNT(d_internal_ccc);

    while (low < high)
    {
        const size_t middle = low + ((high - low) / 2u);

        if (d_internal_ccc[middle].last < _code)
        {
            low = middle + 1u;
        }
        else
        {
            high = middle;
        }
    }

    return ( (low < D_INTERNAL_COUNT(d_internal_ccc)) &&
             (d_internal_ccc[low].first <= _code) )
           ? d_internal_ccc[low].value
           : 0u;
}


/**
 * @brief Finds a row of a pair table by its key and, optionally, its first.
 *
 * @param[in] _table     the table, sorted by key then first.
 * @param[in] _count     its row count.
 * @param[in] _key       the key to find.
 * @param[in] _first     the first to match, when `_by_first`.
 * @param[in] _by_first  whether `first` is part of the key.
 * @return the row, or `NULL` when there is none.
 */
static const struct d_lex_unicode_pair*
d_internal_find_pair(
    const struct d_lex_unicode_pair* _table,
    size_t                           _count,
    uint32_t                         _key,
    uint32_t                         _first,
    bool                             _by_first
)
{
    size_t low  = 0u;
    size_t high = _count;

    while (low < high)
    {
        const size_t middle = low + ((high - low) / 2u);
        const bool   before =
            (_table[middle].key < _key) ||
            ( _by_first &&
              (_table[middle].key == _key) &&
              (_table[middle].first < _first) );

        if (before)
        {
            low = middle + 1u;
        }
        else
        {
            high = middle;
        }
    }

    if ( (low >= _count)             ||
         (_table[low].key != _key)   ||
         ( _by_first &&
           (_table[low].first != _first) ) )
    {
        return NULL;
    }

    return &_table[low];
}


/**
 * @brief Appends a code point's full canonical decomposition to a buffer.
 *
 * @param[in]     _code    the code point.
 * @param[out]    _buffer  the buffer.
 * @param[in,out] _used    its fill, advanced.
 * @return `false` if the buffer filled, `true` otherwise.
 */
static bool
d_internal_decompose(
    uint32_t  _code,
    uint32_t* _buffer,
    size_t*   _used
)
{
    // a precomposed Hangul syllable splits into two or three jamo
    if ( (_code >= D_INTERNAL_S_BASE) &&
         (_code < (D_INTERNAL_S_BASE + D_INTERNAL_S_COUNT)) )
    {
        const uint32_t index = _code - D_INTERNAL_S_BASE;
        const uint32_t tail  = index % D_INTERNAL_T_COUNT;

        return ( d_internal_decompose(D_INTERNAL_L_BASE +
                                      (index / D_INTERNAL_N_COUNT),
                                      _buffer,
                                      _used) &&
                 d_internal_decompose(D_INTERNAL_V_BASE +
                                      ((index % D_INTERNAL_N_COUNT) /
                                       D_INTERNAL_T_COUNT),
                                      _buffer,
                                      _used) &&
                 ( (tail == 0u) ||
                   d_internal_decompose(D_INTERNAL_T_BASE + tail,
                                        _buffer,
                                        _used) ) );
    }

    const struct d_lex_unicode_pair* const row =
        d_internal_find_pair(d_internal_decomposition,
                             D_INTERNAL_COUNT(d_internal_decomposition),
                             _code,
                             0u,
                             false);

    // a code point with no mapping is its own decomposition
    if (!row)
    {
        if (*_used == (size_t)D_INTERNAL_NFC_BUFFER)
        {
            return false;
        }

        _buffer[*_used] = _code;
        ++*_used;

        return true;
    }

    return ( d_internal_decompose(row->first,
                                  _buffer,
                                  _used) &&
             ( (row->second == 0u) ||
               d_internal_decompose(row->second,
                                    _buffer,
                                    _used) ) );
}


/**
 * @brief Reports the primary composite of two code points, if there is one.
 *
 * @param[in] _first   the starter.
 * @param[in] _second  the code point following it.
 * @return the composite, or zero.
 */
static uint32_t
d_internal_compose(
    uint32_t _first,
    uint32_t _second
)
{
    // leading and vowel jamo make an LV syllable
    if ( (_first >= D_INTERNAL_L_BASE) &&
         (_first < (D_INTERNAL_L_BASE + D_INTERNAL_L_COUNT)) &&
         (_second >= D_INTERNAL_V_BASE) &&
         (_second < (D_INTERNAL_V_BASE + D_INTERNAL_V_COUNT)) )
    {
        return D_INTERNAL_S_BASE +
               ( ( (_first - D_INTERNAL_L_BASE) * D_INTERNAL_N_COUNT ) +
                 ( (_second - D_INTERNAL_V_BASE) * D_INTERNAL_T_COUNT ) );
    }

    // an LV syllable and a trailing jamo make an LVT syllable
    if ( (_first >= D_INTERNAL_S_BASE) &&
         (_first < (D_INTERNAL_S_BASE + D_INTERNAL_S_COUNT)) &&
         (((_first - D_INTERNAL_S_BASE) % D_INTERNAL_T_COUNT) == 0u) &&
         (_second > D_INTERNAL_T_BASE) &&
         (_second < (D_INTERNAL_T_BASE + D_INTERNAL_T_COUNT)) )
    {
        return _first + (_second - D_INTERNAL_T_BASE);
    }

    const struct d_lex_unicode_pair* const row =
        d_internal_find_pair(d_internal_composition,
                             D_INTERNAL_COUNT(d_internal_composition),
                             _first,
                             _second,
                             true);

    return row ? row->second : 0u;
}


/**
 * @brief Puts each run of combining marks into canonical order.
 *
 * @note A stable insertion sort on the class: runs are a handful of marks
 *       long, and stability is what the canonical ordering algorithm needs.
 *
 * @param[in,out] _buffer  the decomposed sequence.
 * @param[in]     _used    its length.
 */
static void
d_internal_reorder(
    uint32_t* _buffer,
    size_t    _used
)
{
    for (size_t at = 1u; at < _used; ++at)
    {
        const uint32_t code  = _buffer[at];
        const uint32_t value = d_internal_class_of(code);
        size_t         to    = at;

        // a mark moves before every mark of a higher class, never a starter
        while ( (value != 0u) &&
                (to > 0u) &&
                (d_internal_class_of(_buffer[to - 1u]) > value) )
        {
            _buffer[to] = _buffer[to - 1u];
            --to;
        }

        _buffer[to] = code;
    }

    return;
}


/**
 * @brief Recomposes a canonically ordered sequence in place.
 *
 * @note UAX #15's canonical composition: a mark composes with the last
 *       starter unless a mark between them has a class at least its own, or
 *       a starter intervenes.  `last` is 256 while no starter has been seen,
 *       which blocks everything.
 *
 * @param[in,out] _buffer  the sequence.
 * @param[in]     _used    its length.
 * @return the recomposed length.
 */
static size_t
d_internal_recompose(
    uint32_t* _buffer,
    size_t    _used
)
{
    size_t   starter = 0u;
    size_t   out     = 1u;
    uint32_t last    = d_internal_class_of(_buffer[0]);

    last = (last == 0u) ? 0u : 256u;

    for (size_t at = 1u; at < _used; ++at)
    {
        const uint32_t code      = _buffer[at];
        const uint32_t value     = d_internal_class_of(code);
        const uint32_t composite = d_internal_compose(_buffer[starter],
                                                      code);

        // an unblocked pair folds into the starter
        if ( (composite != 0u) &&
             ( (last < value) ||
               (last == 0u) ) )
        {
            _buffer[starter] = composite;
            continue;
        }

        if (value == 0u)
        {
            starter = out;
        }

        last         = value;
        _buffer[out] = code;
        ++out;
    }

    return out;
}


/**
 * @brief Applies the quick check to a sequence.
 *
 * @param[in]  _code_points  the sequence.
 * @param[in]  _count        its length.
 * @param[out] _out_maybe    receives whether any character answered MAYBE.
 * @return `false` on a definite NO, `true` otherwise.
 */
static bool
d_internal_quick_check(
    const uint32_t* _code_points,
    size_t          _count,
    bool*           _out_maybe
)
{
    uint32_t last = 0u;

    *_out_maybe = false;

    for (size_t at = 0u; at < _count; ++at)
    {
        const uint32_t value = d_internal_class_of(_code_points[at]);
        const int      check = d_lex_unicode_nfc_check(_code_points[at]);

        // marks out of canonical order, or a character NFC never contains
        if ( ( (value != 0u) &&
               (value < last) ) ||
             (check == D_LEX_NFC_NO) )
        {
            return false;
        }

        if (check == D_LEX_NFC_MAYBE)
        {
            *_out_maybe = true;
        }

        last = value;
    }

    return true;
}


/*
d_lex_unicode_is_nfc
  The quick check settles almost every name.  Only a MAYBE -- a mark or jamo
that could compose with what precedes it -- needs the full round trip, and
then the name is normalized in a stack buffer and compared with itself.
*/
bool
d_lex_unicode_is_nfc(
    const uint32_t* _code_points,
    size_t          _count,
    bool            _complete
)
{
    bool maybe = false;

    // an empty or absent sequence is trivially normalized
    if ( (!_code_points) ||
         (_count == 0u) )
    {
        return true;
    }

    if (!d_internal_quick_check(_code_points,
                                _count,
                                &maybe))
    {
        return false;
    }

    // only a complete name can be normalized and compared
    if ( (!maybe) ||
         (!_complete) )
    {
        return true;
    }

    uint32_t buffer[D_INTERNAL_NFC_BUFFER];
    size_t   used = 0u;

    for (size_t at = 0u; at < _count; ++at)
    {
        // a name too long to decompose here is given the benefit
        if (!d_internal_decompose(_code_points[at],
                                  buffer,
                                  &used))
        {
            return true;
        }
    }

    d_internal_reorder(buffer,
                       used);
    used = d_internal_recompose(buffer,
                                used);

    return ( (used == _count) &&
             (memcmp(buffer,
                     _code_points,
                     used * sizeof(uint32_t)) == 0) );
}


/*
d_lex_unicode_version
  Names the Unicode release the generated tables came from.
*/
const char*
d_lex_unicode_version(
    void
)
{
    return D_INTERNAL_UNICODE_VERSION;
}
