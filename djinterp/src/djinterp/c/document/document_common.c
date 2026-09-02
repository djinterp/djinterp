/******************************************************************************
* djinterp [c/util/document]                                 document_common.c
*
*   Implementation of the hint bag.  Compiled by both languages from this one
* source.
*
*   THE ONE INTERESTING FUNCTION IS CANONICALISE, and what makes it
* interesting is that it is not a sort.  See the header: canonical form is
* `container_metadata::set` applied left to right, which keeps a repeated
* key's FIRST position and its LAST value.  A stable sort by key would pass
* every test that used distinct keys and fail the moment a document repeated
* one -- which is the failure mode this file exists to not have.
*
* path:      /src/djinterp/c/util/document/document_common.c
* author(s): TBA                                            created: 2026.08.23
******************************************************************************/

#include "djinterp/c/util/document/document_common.h"

D_EXTERN_C_BEGIN


/*   BORROWED STRINGS COMPARE BY CONTENT, NOT BY POINTER.  Two arena entries
   holding the same key are two different pointers -- the parser copies each
   token in as it reads it -- so pointer equality would report every duplicate
   as distinct and canonicalise would become a no-op that still passed its own
   tests.
     NULL IS A KEY.  A malformed attribute can leave one, and treating NULL as
   equal to NULL rather than as never-equal means two of them collapse instead
   of both surviving into a bag that then has a duplicate in it. */
static int
d_internal_doc_key_equal_
(
    const char* _a,
    const char* _b
)
{
    size_t _i = 0;

    if (_a == _b)
    {
        return 1;
    }

    if (!_a || !_b)
    {
        return 0;
    }

    while (_a[_i] != '\0' && _a[_i] == _b[_i])
    {
        ++_i;
    }

    return _a[_i] == _b[_i];
}


struct d_doc_attributes
d_doc_attributes_canonicalise
(
    struct d_doc_attr* _run,
    uint32_t           _count
)
{
    struct d_doc_attributes _bag = D_DOC_ATTRIBUTES_EMPTY;

    uint32_t _read = 0;
    uint32_t _kept = 0;

    if (!_run || _count == 0u)
    {
        return _bag;
    }

    for (_read = 0u; _read < _count; ++_read)
    {
        uint32_t _at    = 0u;
        int      _found = 0;

        /*   FIRST POSITION, LAST VALUE.  A repeat overwrites the value where
           the key already sits; it does NOT move the entry to the end and it
           does NOT keep the earlier value.  Both of those are what a reader
           expects from "canonical" and both are wrong here -- the C++
           container appends only when the key is absent. */
        for (_at = 0u; _at < _kept; ++_at)
        {
            if (d_internal_doc_key_equal_(_run[_at].key, _run[_read].key))
            {
                _run[_at].value = _run[_read].value;
                _found          = 1;

                break;
            }
        }

        if (_found)
        {
            continue;
        }

        /*   COMPACTING IN PLACE.  `_kept` never runs ahead of `_read`, so
           this only ever moves an entry down or leaves it where it is.  The
           self-assignment when nothing has been dropped yet is deliberate --
           branching around it would be a second path to get wrong. */
        _run[_kept] = _run[_read];
        ++_kept;
    }

    _bag.items    = _run;
    _bag.count    = _kept;
    _bag.reserved = 0u;

    return _bag;
}


int32_t
d_doc_attributes_is_canonical
(
    const struct d_doc_attributes* _bag
)
{
    uint32_t _i = 0;
    uint32_t _j = 0;

    /*   AN ABSENT BAG IS CANONICAL, and so is an empty one.  This is an
       invariant check: answering 0 for "there is nothing here" would make
       every caller special-case the empty document. */
    if (!_bag || _bag->count == 0u)
    {
        return 1;
    }

    if (!_bag->items)
    {
        return 0;   /* a non-zero count over a NULL run is not a bag at all */
    }

    for (_i = 0u; _i + 1u < _bag->count; ++_i)
    {
        for (_j = _i + 1u; _j < _bag->count; ++_j)
        {
            if (d_internal_doc_key_equal_(_bag->items[_i].key,
                                          _bag->items[_j].key))
            {
                return 0;
            }
        }
    }

    return 1;
}


const char*
d_doc_attributes_find
(
    const struct d_doc_attributes* _bag,
    const char*                    _key
)
{
    uint32_t _i = 0;

    if (!_bag || !_bag->items)
    {
        return (const char*)0;
    }

    for (_i = 0u; _i < _bag->count; ++_i)
    {
        if (d_internal_doc_key_equal_(_bag->items[_i].key, _key))
        {
            return _bag->items[_i].value;
        }
    }

    return (const char*)0;
}


D_EXTERN_C_END


// ===========================================================================
// RELAY 79 -- vocabulary accessors from the document node tier.
// ===========================================================================
//   Kind/sort classification, the typed hint readers and the alignment
// interchange, taken from pending/document_tier/document_common.c.  All of
// them are order-independent: d_doc_attr_find is a LINEAR scan, so it reads
// an insertion-ordered bag exactly as it reads any other.
//
//   TWO FUNCTIONS FROM THAT FILE WERE NOT TAKEN -- its canonicalise (an
// insertion sort) and its is_canonical (a sortedness test).  Those are above,
// in this tree's own form, and the header's banner says why at length.
// d_doc_attr_key_compare IS taken: d_doc_attr_find needs it for equality.

// ===========================================================================
// I.     kinds and sorts
// ===========================================================================

/*
d_doc_kind_sort
  The sort a node of this kind belongs to.

Parameter(s):
  _kind: the node kind.
Return:
  The kind's sort; D_DOC_SORT_BLOCK for an out-of-range kind, since an
unrecognised kind is treated as an opaque block rather than as an error.
*/
enum d_doc_sort
d_doc_kind_sort(
    enum d_doc_node_kind _kind
)
{
    switch (_kind)
    {
        case D_DOC_KIND_DOCUMENT: { return D_DOC_SORT_DOCUMENT; }
        case D_DOC_KIND_ITEM:     { return D_DOC_SORT_ITEM;     }
        case D_DOC_KIND_COLUMN:   { return D_DOC_SORT_COLUMN;   }
        case D_DOC_KIND_COLUMN_GROUP: { return D_DOC_SORT_COLUMN; }
        case D_DOC_KIND_ROW:      { return D_DOC_SORT_ROW;      }
        case D_DOC_KIND_CELL:     { return D_DOC_SORT_CELL;     }
        default:                  { return D_DOC_SORT_BLOCK;    }
    }
}

/*
d_doc_child_sort
  The sort this kind's children must have.

Parameter(s):
  _kind: the node kind.
Return:
  The required child sort.  A childless kind reports D_DOC_SORT_BLOCK, which
is never consulted because d_doc_kind_takes_children is false for it.
Note: a table's children are of two sorts (column, then row); it reports
D_DOC_SORT_COLUMN and is special-cased by the well-sortedness check.
*/
enum d_doc_sort
d_doc_child_sort(
    enum d_doc_node_kind _kind
)
{
    switch (_kind)
    {
        case D_DOC_KIND_LIST:   { return D_DOC_SORT_ITEM;   }
        case D_DOC_KIND_TABLE:        { return D_DOC_SORT_COLUMN; }
        case D_DOC_KIND_COLUMN_GROUP: { return D_DOC_SORT_COLUMN; }
        case D_DOC_KIND_ROW:    { return D_DOC_SORT_CELL;   }
        default:                { return D_DOC_SORT_BLOCK;  }
    }
}

/*
d_doc_kind_name
  A stable spelling of the kind, for diagnostics and for the parity oracle's
self-identifying rows.

Parameter(s):
  _kind: the node kind.
Return:
  A static string; "unknown" for an out-of-range kind.  Never NULL.
*/
const char*
d_doc_kind_name(
    enum d_doc_node_kind _kind
)
{
    switch (_kind)
    {
        case D_DOC_KIND_ELEMENT:    { return "element";    }
        case D_DOC_KIND_HEADING:    { return "heading";    }
        case D_DOC_KIND_PARAGRAPH:  { return "paragraph";  }
        case D_DOC_KIND_KEY_VALUE:  { return "key_value";  }
        case D_DOC_KIND_RULE:       { return "rule";       }
        case D_DOC_KIND_SPACE:      { return "space";      }
        case D_DOC_KIND_PAGE_BREAK: { return "page_break"; }
        case D_DOC_KIND_LIST:       { return "list";       }
        case D_DOC_KIND_TABLE:      { return "table";      }
        case D_DOC_KIND_REPEAT:     { return "repeat";     }
        case D_DOC_KIND_SLOT:       { return "slot";       }
        case D_DOC_KIND_DOCUMENT:   { return "document";   }
        case D_DOC_KIND_ITEM:       { return "item";       }
        case D_DOC_KIND_COLUMN:     { return "column";     }
        case D_DOC_KIND_ROW:        { return "row";        }
        case D_DOC_KIND_CELL:         { return "cell";         }
        case D_DOC_KIND_COLUMN_GROUP: { return "column_group"; }
        default:                    { return "unknown";    }
    }
}

/*
d_doc_kind_takes_children
  Whether this kind may have children at all.

Parameter(s):
  _kind: the node kind.
Return:
  true for the container constructors, false for the leaves.
*/
bool
d_doc_kind_takes_children(
    enum d_doc_node_kind _kind
)
{
    return ( (_kind == D_DOC_KIND_DOCUMENT) ||
             (_kind == D_DOC_KIND_ELEMENT)  ||
             (_kind == D_DOC_KIND_LIST)     ||
             (_kind == D_DOC_KIND_ITEM)     ||
             (_kind == D_DOC_KIND_TABLE)    ||
             (_kind == D_DOC_KIND_ROW)      ||
             (_kind == D_DOC_KIND_REPEAT)   ||
             (_kind == D_DOC_KIND_COLUMN_GROUP) );
}

/*
d_doc_kind_is_open
  Whether this kind is a hole or a binder -- that is, whether its presence
makes a tree a TEMPLATE rather than a DOCUMENT.

Parameter(s):
  _kind: the node kind.
Return:
  true for slot and repeat, false otherwise.
*/
bool
d_doc_kind_is_open(
    enum d_doc_node_kind _kind
)
{
    return ( (_kind == D_DOC_KIND_SLOT) ||
             (_kind == D_DOC_KIND_REPEAT) );
}


// ===========================================================================
// II.    hint lookup
// ===========================================================================

/*
d_doc_attr_key_compare
  Compares two hint keys by unsigned byte value.  Locale-free and
deterministic, which strcoll is not and strcmp is not required to be for
values above 127.  This is the order of ruling R2.

Parameter(s):
  _left:  the first key; NULL sorts before everything.
  _right: the second key.
Return:
  Negative, zero, or positive as _left sorts before, equal to, or after
_right.
*/
int
d_doc_attr_key_compare(
    const char* _left,
    const char* _right
)
{
    const unsigned char* left;
    const unsigned char* right;

    // a NULL key is the least element, so a malformed bag still has a
    // well-defined order rather than an undefined comparison
    if (!_left)
    {
        return _right ? -1 : 0;
    }

    if (!_right)
    {
        return 1;
    }

    left  = (const unsigned char*)_left;
    right = (const unsigned char*)_right;

    while ((*left) && ((*left) == (*right)))
    {
        ++left;
        ++right;
    }

    return (int)(*left) - (int)(*right);
}

/*
d_doc_attr_find
  The value bound to _key, or NULL when _key is absent.

Parameter(s):
  _attrs: the hint bag; may be NULL.
  _key:   the key to look up; may be NULL.
Return:
  A borrowed pointer to the value, or NULL.  A bound key whose value pointer
is NULL reports the empty string, so a caller never has to distinguish "bound
to nothing" from "bound to empty".
*/
const char*
d_doc_attr_find(
    const struct d_doc_attributes* _attrs,
    const char*                    _key
)
{
    uint32_t index;

    if ( (!_attrs)        ||
         (!_attrs->items) ||
         (!_key) )
    {
        return NULL;
    }

    index = 0u;

    // linear over a canonically ordered bag: hint bags are small (single
    // digits), so a binary search would cost more in code than it saves
    for (index = 0u; index < _attrs->count; ++index)
    {
        if (d_doc_attr_key_compare(_attrs->items[index].key, _key) == 0)
        {
            return _attrs->items[index].value ? _attrs->items[index].value
                                              : "";
        }
    }

    return NULL;
}

/*
d_doc_attr_has
  Whether _attrs binds _key at all.

Parameter(s):
  _attrs: the hint bag; may be NULL.
  _key:   the key to test.
Return:
  true when the key is present, false otherwise.
*/
bool
d_doc_attr_has(
    const struct d_doc_attributes* _attrs,
    const char*                    _key
)
{
    return (d_doc_attr_find(_attrs, _key) != NULL);
}

/*
d_doc_attr_or
  The value bound to _key, or _fallback when absent.

Parameter(s):
  _attrs:    the hint bag; may be NULL.
  _key:      the key to look up.
  _fallback: what to return when the key is absent; may be NULL.
Return:
  A borrowed pointer; _fallback when the key is absent.
*/
const char*
d_doc_attr_or(
    const struct d_doc_attributes* _attrs,
    const char*                    _key,
    const char*                    _fallback
)
{
    const char* value;

    value = d_doc_attr_find(_attrs, _key);

    return value ? value : _fallback;
}

/*
d_doc_attr_flag
  _key read as a boolean.

Parameter(s):
  _attrs:    the hint bag; may be NULL.
  _key:      the key to look up.
  _fallback: what to return when the key is absent or empty.
Return:
  true when the value begins with t, T, 1, y, Y, o, or O; _fallback when the
key is absent or its value is empty; false otherwise.
*/
bool
d_doc_attr_flag(
    const struct d_doc_attributes* _attrs,
    const char*                    _key,
    bool                           _fallback
)
{
    const char* value;
    char        first;

    value = d_doc_attr_find(_attrs, _key);

    // an absent or empty flag keeps the caller's default rather than reading
    // as false, so that "set to nothing" and "not set" agree
    if ( (!value) ||
         (value[0] == '\0') )
    {
        return _fallback;
    }

    first = value[0];

    return ( (first == 't') || (first == 'T') ||
             (first == '1')                   ||
             (first == 'y') || (first == 'Y') ||
             (first == 'o') || (first == 'O') );
}

/*
d_doc_attr_uint
  _key read as an unsigned decimal.

Parameter(s):
  _attrs:    the hint bag; may be NULL.
  _key:      the key to look up.
  _fallback: what to return when the key is absent or carries no leading
             digit.
Return:
  The leading unsigned decimal run of the value, saturating at UINT32_MAX; the
fallback when there is no leading digit.  A value with trailing junk ("12px")
reads as its leading number, which is what a hint carrying a unit suffix
should do.
*/
uint32_t
d_doc_attr_uint(
    const struct d_doc_attributes* _attrs,
    const char*                    _key,
    uint32_t                       _fallback
)
{
    const char* value;
    uint32_t    result;
    uint32_t    digit;

    value = d_doc_attr_find(_attrs, _key);

    // no leading digit means the hint carries no number: treat it as absent
    // rather than as zero, which would silently mean something different
    if ( (!value) ||
         (value[0] < '0') ||
         (value[0] > '9') )
    {
        return _fallback;
    }

    result = 0u;

    while ( (*value >= '0') &&
            (*value <= '9') )
    {
        digit = (uint32_t)(*value - '0');

        // saturate rather than wrap: a wrapped width is a silently wrong
        // layout, a saturated one is an obviously wrong one
        if (result > ((0xFFFFFFFFu - digit) / 10u))
        {
            return 0xFFFFFFFFu;
        }

        result = (result * 10u) + digit;
        ++value;
    }

    return result;
}

/*
d_doc_attr_fixed
  _key read as a FIXED-POINT number: the decimal value times _scale, truncated
towards zero.

Parameter(s):
  _attrs:    the hint bag; may be NULL.
  _key:      the key to look up.
  _scale:    the multiplier -- 1000 reads "10.5" as 10500.
  _fallback: what to return when the key is absent or carries no leading
             digit.
Return:
  The scaled value, saturating at INT32_MAX/INT32_MIN.

Note:
  WHY NOT A DOUBLE.  This is the reader the emission side asked for, and it is
deliberately not `d_doc_attr_real`.  Returning a double would hand back a value
whose exact bits depend on the parse, then let two layers round it differently
-- reintroducing precisely the determinacy hazard that moving `space` to
millipoints removed.  The inverse of a locale-free, FP-free decimal EMITTER is
a fixed-point READER, not a float one.  A caller who genuinely wants a real
converts at the point of use and owns the consequence.
*/
int32_t
d_doc_attr_fixed(
    const struct d_doc_attributes* _attrs,
    const char*                    _key,
    uint32_t                       _scale,
    int32_t                        _fallback
)
{
    const char* value;
    int64_t     whole;
    int64_t     frac;
    int64_t     unit;
    int64_t     result;
    int32_t     sign;

    value = d_doc_attr_find(_attrs, _key);

    if (!value)
    {
        return _fallback;
    }

    sign = 1;

    if ( (*value == '-') ||
         (*value == '+') )
    {
        sign = (*value == '-') ? -1 : 1;
        ++value;
    }

    // no leading digit means the hint carries no number: absent, not zero
    if ( (*value < '0') ||
         (*value > '9') )
    {
        return _fallback;
    }

    whole = 0;

    while ( (*value >= '0') &&
            (*value <= '9') )
    {
        whole = (whole * 10) + (int64_t)(*value - '0');

        if (whole > 0x7FFFFFFF)
        {
            whole = 0x7FFFFFFF;

            break;
        }

        ++value;
    }

    while ( (*value >= '0') &&
            (*value <= '9') )
    {
        ++value;
    }

    frac = 0;
    unit = (int64_t)_scale;

    // the fraction is accumulated by long division against the scale, so no
    // floating point appears anywhere in the path and "10.5" at scale 1000 is
    // exactly 10500 rather than 10499.999...
    if (*value == '.')
    {
        ++value;

        while ( (unit > 1) &&
                (*value >= '0') &&
                (*value <= '9') )
        {
            unit /= 10;
            frac += unit * (int64_t)(*value - '0');
            ++value;
        }
    }

    result = (whole * (int64_t)_scale) + frac;

    if (result > 0x7FFFFFFF)
    {
        result = 0x7FFFFFFF;
    }

    return (int32_t)(sign * (int32_t)result);
}

/*
d_doc_attr_milli
  _key read in thousandths -- the common case, and the unit `space` already
uses.  "10.5" reads as 10500.

Parameter(s):
  _attrs:    the hint bag; may be NULL.
  _key:      the key to look up.
  _fallback: what to return when the key is absent or unparseable.
Return:
  The value in thousandths.
*/
int32_t
d_doc_attr_milli(
    const struct d_doc_attributes* _attrs,
    const char*                    _key,
    int32_t                        _fallback
)
{
    return d_doc_attr_fixed(_attrs, _key, 1000u, _fallback);
}

/*
d_doc_attr_align
  The `align` hint as an alignment.

Parameter(s):
  _attrs:    the hint bag; may be NULL.
  _fallback: what to return when the hint is absent or unrecognised.
Return:
  The decoded alignment, or _fallback.
*/
enum d_doc_align
d_doc_attr_align(
    const struct d_doc_attributes* _attrs,
    enum d_doc_align               _fallback
)
{
    return d_doc_align_from_string(
               d_doc_attr_find(_attrs, D_DOC_ATTR_ALIGN),
               _fallback);
}

/*
d_doc_align_to_string
  The canonical token for an alignment.  Round-trips through
d_doc_align_from_string.

Parameter(s):
  _align: the alignment.
Return:
  A static string; "left" for an out-of-range value.  Never NULL.
*/
const char*
d_doc_align_to_string(
    enum d_doc_align _align
)
{
    switch (_align)
    {
        case D_DOC_ALIGN_LEFT:    { return "left";    }
        case D_DOC_ALIGN_CENTER:  { return "center";  }
        case D_DOC_ALIGN_RIGHT:   { return "right";   }
        case D_DOC_ALIGN_JUSTIFY: { return "justify"; }
        default:                  { return "left";    }
    }
}

/*
d_doc_align_from_string
  The alignment named by a token.  Only the first character is significant,
so "centre" and "center" both read as center.

Parameter(s):
  _token:    the token; may be NULL or empty.
  _fallback: what to return when the token is absent or unrecognised.
Return:
  The decoded alignment, or _fallback.
*/
enum d_doc_align
d_doc_align_from_string(
    const char*      _token,
    enum d_doc_align _fallback
)
{
    // an absent or empty token carries no opinion, and an unrecognised one is
    // treated the same way: a hint the reader cannot parse is a hint it does
    // not read
    if ( (!_token) ||
         (_token[0] == '\0') )
    {
        return _fallback;
    }

    switch (_token[0])
    {
        case 'l': case 'L': { return D_DOC_ALIGN_LEFT;    }
        case 'c': case 'C': { return D_DOC_ALIGN_CENTER;  }
        case 'r': case 'R': { return D_DOC_ALIGN_RIGHT;   }
        case 'j': case 'J': { return D_DOC_ALIGN_JUSTIFY; }
        default:            { return _fallback;           }
    }
}
