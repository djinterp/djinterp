/******************************************************************************
* djinterp [meta]                                                    lookup.h
*
* Finding a record in a contiguous array of records, by a key whose position
* and width the record itself declares. Tier 0 -- compiled by BOTH faces.
*
*
* WHY THIS IS A MODULE AND NOT FOUR SCANS
* =======================================
*   kv.h answers "where does the key sit and how do I read it". It does not
* answer "which record". Four places in this tree answer that separately:
*
*     d_option_set_find    linear scan, uint64_t key compared inline
*     d_registry           sorted index + binary search, const char* key,
*                          reached through a macro that hard-codes offset 0
*     d_test_registry_row  the same, through the registry
*     d_test_metadata      linear scan, interned key compared inline
*     basic_metadata<>     linear scan, keys compared with operator==
*
* They disagree on where the key sits, on how it is compared, on whether the
* array is ordered, and on what a miss returns. This module is the one answer,
* parameterised on a `d_kv_field` so the disagreements become arguments.
*
*
* THREE THINGS THAT ARE EASY TO GET WRONG AND ARE FIXED ONCE HERE
* ==============================================================
*   A COMPARISON IS NOT A SUBTRACTION. `return (int)(lhs - rhs)` is the
* familiar spelling and it wraps: two uint64_t keys a hair apart across the
* signed boundary compare backwards, and the sort built on it silently stops
* being a sort. Every comparison below is the three-branch form.
*
*   CASE FOLDING IS NOT tolower(). tolower() is locale-dependent, so a
* registry keyed case-insensitively answers differently under a Turkish locale
* than under C -- and goal 4 wants one answer. It also takes an int that must
* be representable as unsigned char, so handing it a plain `char` that went
* negative is undefined. d_lookup_fold_ascii is locale-free and takes the
* character as unsigned, and it is deliberately ASCII-only: a fold that
* claimed to handle more would be claiming a Unicode table this framework does
* not have. strcasecmp is not an option regardless -- it is POSIX, not C.
*
*   A BINARY SEARCH MIDPOINT OVERFLOWS. `(low + high) / 2` is wrong for large
* counts; `low + (high - low) / 2` is not. The form is used below even though
* no array here is near the boundary, because the day one is, nothing will
* report it.
*
*
* WHAT A MISS RETURNS
* ===================
*   `found` false, `record` NULL, and `index` set to THE INSERTION POINT --
* where the key would go to keep the array ordered. That is what a caller
* adding a row wants and what a bare "not found" throws away. For an unsorted
* view the insertion point is `count`, because appending is the only placement
* that preserves the (absent) order.
*
*
* WHAT THIS MODULE IS NOT
* =======================
*   IT DOES NOT OWN A SORTED INDEX. d_registry keeps a permutation array --
* `struct d_registry_lookup_entry { const char* key; size_t row_index; }` --
* so that the rows themselves may stay in insertion order while lookup stays
* logarithmic. That is a second container with its own invariants, and it
* lands with the registry rather than being guessed at here. A view over the
* permutation is expressible with what is below; maintaining it is not.
*
*   IT DOES NOT MUTATE. Every view is const. A consumer that found a record
* and wants to write it uses kv.h's store and write entry points against the
* address it got back.
*
*
* path:      /inc/djinterp/c/meta/lookup.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.05
******************************************************************************/

#ifndef DJINTERP_C_LOOKUP_
#define DJINTERP_C_LOOKUP_ 1

// std
// c
#include <stddef.h>
#include <stdint.h>
#include <string.h>
// djinterp
#include "../djinterp.h"
#include "kv.h"


D_EXTERN_C_BEGIN


// I.     the lookup view

// d_lookup_view
//   struct: a contiguous array of records and where the key sits in each. The
// records are BORROWED; this struct owns nothing and outlives nothing.
//   `stride` is the distance between consecutive records, which is the record
// type's sizeof and not the key's width. It doubles as the per-record capacity
// handed to kv.h, so a key field that would read past the end of one record is
// refused by the same bounds check that guards every other read.
struct d_lookup_view
{
    const void*       records;  // borrowed; the first record, or NULL
    size_t            stride;   // bytes between consecutive records
    size_t            count;    // number of records
    struct d_kv_field key;      // where the key sits within one record
    uint16_t          flags;    // D_LOOKUP_FLAG_*
    uint16_t          pad;      // reserved; written zero
};

// d_lookup_result
//   struct: the outcome of a search. On a miss `index` is the INSERTION POINT
// rather than a sentinel, so a caller that failed to find a key already knows
// where to put it.
struct d_lookup_result
{
    const void* record;     // borrowed; NULL when not found
    size_t      index;      // position when found; insertion point when not
    bool        found;      // the verdict
};

// D_LOOKUP_FLAG_NONE
//   constant: no flags. An unsorted view of inline keys.
#define D_LOOKUP_FLAG_NONE                  ((uint16_t)0u)

// D_LOOKUP_FLAG_SORTED
//   constant: the records are already in ascending key order, so a search may
// bisect. SETTING THIS ON AN UNSORTED VIEW IS A LIE THE MODULE CANNOT CATCH:
// the search will miss keys that are present and report an insertion point
// that does not preserve anything.
#define D_LOOKUP_FLAG_SORTED                ((uint16_t)(1u << 0))

// D_LOOKUP_FLAG_KEY_IS_POINTER
//   constant: the key field holds a POINTER TO the key rather than the key
// itself -- a `const char*` member, not a `char[32]` member. This is what
// d_registry's key macro encoded by casting a row to `const char* const*`, and
// it is a property of the record rather than of the search, so it lives on the
// view.
#define D_LOOKUP_FLAG_KEY_IS_POINTER        ((uint16_t)(1u << 1))

// D_LOOKUP_FLAG_CASE_INSENSITIVE
//   constant: string keys are compared with an ASCII case fold. This is
// d_registry's D_REGISTRY_FLAG_CASE_INSENSITIVE, become a comparator choice
// now that the key's type is described rather than assumed.
#define D_LOOKUP_FLAG_CASE_INSENSITIVE      ((uint16_t)(1u << 2))

// D_LOOKUP_FLAG_MASK
//   constant: every flag this version defines. Bits outside the mask are
// reserved and written zero.
#define D_LOOKUP_FLAG_MASK                  ((uint16_t)0x0007u)

// D_LOOKUP_VIEW_INIT
//   macro: the empty view -- no records, no count, no key. Every search
// against it misses at insertion point zero.
#define D_LOOKUP_VIEW_INIT                                                     \
{                                                                              \
    NULL, 0u, 0u, D_KV_FIELD_INIT, D_LOOKUP_FLAG_NONE, 0u                      \
}

// D_LOOKUP_RESULT_MISS
//   macro: the miss at insertion point zero, which is what an invalid view
// returns.
#define D_LOOKUP_RESULT_MISS        { NULL, 0u, false }

// D_LOOKUP_VIEW_OF
//   macro: a view over an array of a known record type. The stride comes from
// the record's sizeof and the key field from offsetof, so a padded record is
// walked correctly without the caller computing anything.
#define D_LOOKUP_VIEW_OF(record, key_member, records, count, info, flags)      \
{                                                                              \
    (const void*)(records), sizeof(record), (size_t)(count),                   \
    D_KV_FIELD_OF(record, key_member, info),                                   \
    (uint16_t)((flags) & D_LOOKUP_FLAG_MASK), 0u                               \
}

// D_LOOKUP_VIEW_OF_SIGNED
//   macro: the same, for a record whose key is a signed integer.
#define D_LOOKUP_VIEW_OF_SIGNED(record, key_member, records, count, info,      \
                                flags)                                         \
{                                                                              \
    (const void*)(records), sizeof(record), (size_t)(count),                   \
    D_KV_FIELD_OF_SIGNED(record, key_member, info),                            \
    (uint16_t)((flags) & D_LOOKUP_FLAG_MASK), 0u                               \
}


// II.    view construction and inspection

// d_lookup_view_make
//   function: a view from loose parts, for an array whose record type is not
// visible where the search happens.
//   A STRIDE WIDER THAN UINT32_MAX YIELDS AN EMPTY VIEW rather than a
// truncated one. The stride is handed to kv.h as a per-record capacity, which
// is 32-bit, and a silent narrowing there would turn the bounds check into
// decoration.
D_NODISCARD D_INLINE struct d_lookup_view
d_lookup_view_make(
    const void*              _records,
    size_t                   _stride,
    size_t                   _count,
    const struct d_kv_field* _key,
    uint16_t                 _flags
)
{
    struct d_lookup_view view = D_LOOKUP_VIEW_INIT;

    // reject a view that cannot describe a record, or a key it cannot reach
    if ( (!_records)                          ||
         (!_key)                              ||
         (_stride == 0u)                      ||
         (_stride > (size_t)UINT32_MAX)       ||
         (!d_kv_field_fits(_key, (uint32_t)_stride)) )
    {
        return view;
    }

    view.records = _records;
    view.stride  = _stride;
    view.count   = _count;
    view.key     = *_key;
    view.flags   = (uint16_t)(_flags & D_LOOKUP_FLAG_MASK);

    return view;
}

// d_lookup_view_is_empty
//   function: whether a view can be searched at all. An invalid view and an
// array of zero records answer the same, because both find nothing.
D_NODISCARD D_INLINE bool
d_lookup_view_is_empty(
    const struct d_lookup_view* _view
)
{
    return ( (!_view)             ||
             (!_view->records)    ||
             (_view->stride == 0u) ||
             (_view->count == 0u) );
}

// d_lookup_view_is_sorted
//   function: whether the view claims ascending key order. A CLAIM, not a
// check: verifying it costs a full scan, which is the thing bisecting was
// meant to avoid.
D_NODISCARD D_INLINE bool
d_lookup_view_is_sorted(
    const struct d_lookup_view* _view
)
{
    return ( (_view != NULL) &&
             ((_view->flags & D_LOOKUP_FLAG_SORTED) != 0u) );
}

// d_lookup_at
//   function: the record at `_index`, or NULL when the index is past the end.
D_NODISCARD D_INLINE const void*
d_lookup_at(
    const struct d_lookup_view* _view,
    size_t                      _index
)
{
    // an empty view has no record at any index
    if ( (d_lookup_view_is_empty(_view)) ||
         (_index >= _view->count) )
    {
        return NULL;
    }

    return (const void*)(((const unsigned char*)_view->records) +
                         (_index * _view->stride));
}


// III.   key comparison
//   Three-way, returning negative, zero or positive. The equivalence these
// induce is ORDERING equivalence, which is not identity: two records whose
// keys compare equal may still differ everywhere else, and a caller wanting
// identity compares the records, not the keys.

// d_lookup_compare_unsigned
//   function: order two unsigned keys.
//   THE THREE-BRANCH FORM IS NOT AN INDULGENCE. `(int)(_lhs - _rhs)` wraps for
// keys far apart and truncates for keys more than INT_MAX apart, and both
// failures produce a comparator that is not a strict weak ordering -- which a
// binary search reads as a permanently missing key.
D_NODISCARD D_INLINE int
d_lookup_compare_unsigned(
    uint64_t _lhs,
    uint64_t _rhs
)
{
    // strictly less
    if (_lhs < _rhs)
    {
        return -1;
    }

    // strictly greater
    if (_lhs > _rhs)
    {
        return 1;
    }

    return 0;
}

// d_lookup_compare_signed
//   function: order two signed keys.
D_NODISCARD D_INLINE int
d_lookup_compare_signed(
    int64_t _lhs,
    int64_t _rhs
)
{
    // strictly less
    if (_lhs < _rhs)
    {
        return -1;
    }

    // strictly greater
    if (_lhs > _rhs)
    {
        return 1;
    }

    return 0;
}

// d_lookup_fold_ascii
//   function: lower-case an ASCII letter, and pass every other byte through.
//   LOCALE-FREE AND DELIBERATELY ASCII-ONLY. tolower() answers differently
// under different locales, so a case-insensitive registry built on it is not
// deterministic across builds -- and it takes an int that must be
// representable as unsigned char, so a plain `char` that went negative is
// undefined behaviour rather than a wrong answer. Folding beyond ASCII would
// require a Unicode table this framework does not have and must not pretend
// to.
D_NODISCARD D_INLINE unsigned char
d_lookup_fold_ascii(
    unsigned char _character
)
{
    // only the twenty-six upper-case ASCII letters fold
    if ( (_character >= (unsigned char)'A') &&
         (_character <= (unsigned char)'Z') )
    {
        return (unsigned char)(_character +
                               ((unsigned char)'a' - (unsigned char)'A'));
    }

    return _character;
}

// d_lookup_compare_string
//   function: order two NUL-terminated strings, optionally case-folded. A
// NULL string orders before every non-NULL one, and two NULLs are equal, so
// that a row with no key sorts to one end instead of faulting.
D_NODISCARD D_INLINE int
d_lookup_compare_string(
    const char* _lhs,
    const char* _rhs,
    bool        _fold_case
)
{
    unsigned char lhs_char;
    unsigned char rhs_char;

    // a missing string orders before a present one
    if ( (!_lhs) ||
         (!_rhs) )
    {
        if (_lhs == _rhs)
        {
            return 0;
        }

        return (!_lhs) ? -1 : 1;
    }

    // walk until the strings differ or one of them ends
    for (;;)
    {
        lhs_char = (unsigned char)*_lhs;
        rhs_char = (unsigned char)*_rhs;

        if (_fold_case)
        {
            lhs_char = d_lookup_fold_ascii(lhs_char);
            rhs_char = d_lookup_fold_ascii(rhs_char);
        }

        if (lhs_char != rhs_char)
        {
            return (lhs_char < rhs_char) ? -1 : 1;
        }

        // equal characters, and both strings ended together
        if (lhs_char == (unsigned char)'\0')
        {
            return 0;
        }

        ++_lhs;
        ++_rhs;
    }
}

// d_lookup_compare_bytes
//   function: order two opaque keys of the same width. memcmp's sign is
// unspecified beyond its direction, so the result is normalised to -1, 0 or 1
// -- a caller storing a comparison result should get the same value from every
// implementation.
D_NODISCARD D_INLINE int
d_lookup_compare_bytes(
    const void* _lhs,
    const void* _rhs,
    size_t      _size
)
{
    int result;

    // a missing key orders before a present one
    if ( (!_lhs) ||
         (!_rhs) )
    {
        if (_lhs == _rhs)
        {
            return 0;
        }

        return (!_lhs) ? -1 : 1;
    }

    // a zero-width key carries no information, so all such keys are equal
    if (_size == 0u)
    {
        return 0;
    }

    result = memcmp(_lhs, _rhs, _size);

    // normalise: memcmp's magnitude is unspecified
    if (result < 0)
    {
        return -1;
    }

    return (result > 0) ? 1 : 0;
}


// IV.    key access
//   Reading the key out of a record, honouring D_LOOKUP_FLAG_KEY_IS_POINTER.
// An inline key is the bytes at the field; an indirect key is the bytes the
// pointer at the field leads to.

// d_lookup_key_unsigned
//   function: the unsigned key of the record at `_index`, widened. Zero for
// an absent record, which is why a caller distinguishing "key is zero" from
// "no such record" tests the index first.
D_NODISCARD D_INLINE uint64_t
d_lookup_key_unsigned(
    const struct d_lookup_view* _view,
    size_t                      _index
)
{
    const void* record;

    record = d_lookup_at(_view, _index);

    // no record, no key
    if (!record)
    {
        return 0u;
    }

    return d_kv_load_unsigned(record,
                              _view->key.offset,
                              _view->key.size,
                              (uint32_t)_view->stride);
}

// d_lookup_key_signed
//   function: the signed key of the record at `_index`, sign-extended.
D_NODISCARD D_INLINE int64_t
d_lookup_key_signed(
    const struct d_lookup_view* _view,
    size_t                      _index
)
{
    const void* record;

    record = d_lookup_at(_view, _index);

    // no record, no key
    if (!record)
    {
        return 0;
    }

    return d_kv_load_signed(record,
                            _view->key.offset,
                            _view->key.size,
                            (uint32_t)_view->stride);
}

// d_lookup_key_string
//   function: the string key of the record at `_index`. With
// D_LOOKUP_FLAG_KEY_IS_POINTER the field holds a `const char*` and is
// dereferenced; without it the field IS the characters, as a `char[]` member.
D_NODISCARD D_INLINE const char*
d_lookup_key_string(
    const struct d_lookup_view* _view,
    size_t                      _index
)
{
    const void* record;

    record = d_lookup_at(_view, _index);

    // no record, no key
    if (!record)
    {
        return NULL;
    }

    // an indirect key is a pointer stored in the row; load it, do not cast it
    if ((_view->flags & D_LOOKUP_FLAG_KEY_IS_POINTER) != 0u)
    {
        return (const char*)d_kv_load_pointer(record,
                                              _view->key.offset,
                                              (uint32_t)_view->stride);
    }

    return (const char*)d_kv_at_const(record, _view->key.offset);
}

// d_lookup_key_bytes
//   function: the address of the opaque key of the record at `_index`, under
// the same indirection rule as d_lookup_key_string.
D_NODISCARD D_INLINE const void*
d_lookup_key_bytes(
    const struct d_lookup_view* _view,
    size_t                      _index
)
{
    const void* record;

    record = d_lookup_at(_view, _index);

    // no record, no key
    if (!record)
    {
        return NULL;
    }

    // an indirect key is a pointer stored in the row; load it, do not cast it
    if ((_view->flags & D_LOOKUP_FLAG_KEY_IS_POINTER) != 0u)
    {
        return d_kv_load_pointer(record,
                                 _view->key.offset,
                                 (uint32_t)_view->stride);
    }

    return d_kv_at_const(record, _view->key.offset);
}


// V.     finding
//   Each entry point picks linear or bisecting from D_LOOKUP_FLAG_SORTED. A
// caller that knows better than the flag has the comparison and key-access
// primitives above and can write the scan it wants; these are the two scans
// worth not writing five times.

// D_INTERNAL_LOOKUP_DEFINE_FIND
//   macro: generate the find for one key form. The scan is identical in every
// form and only the comparison differs, so it is written once here rather than
// four times below.
//   THE MIDPOINT IS low + (high - low) / 2. `(low + high) / 2` overflows for
// counts near SIZE_MAX; no array in this framework is close, and the day one
// is, nothing would report it.
#define D_INTERNAL_LOOKUP_DEFINE_FIND(name, key_param, compare_expr)           \
    D_NODISCARD D_INLINE struct d_lookup_result                                \
    d_lookup_find_##name(                                                      \
        const struct d_lookup_view* _view,                                     \
        key_param                                                              \
    )                                                                          \
    {                                                                          \
        struct d_lookup_result result = D_LOOKUP_RESULT_MISS;                  \
        size_t                 low;                                            \
        size_t                 high;                                           \
        size_t                 mid;                                            \
        int                    order;                                          \
                                                                               \
        /* an empty view misses at insertion point zero */                     \
        if (d_lookup_view_is_empty(_view))                                     \
        {                                                                      \
            return result;                                                     \
        }                                                                      \
                                                                               \
        /* an unordered view has to be walked, and appends on a miss */        \
        if (!d_lookup_view_is_sorted(_view))                                   \
        {                                                                      \
            for (low = 0u; low < _view->count; ++low)                          \
            {                                                                  \
                mid   = low;                                                   \
                order = (compare_expr);                                        \
                                                                               \
                if (order == 0)                                                \
                {                                                              \
                    result.record = d_lookup_at(_view, low);                   \
                    result.index  = low;                                       \
                    result.found  = true;                                      \
                                                                               \
                    return result;                                             \
                }                                                              \
            }                                                                  \
                                                                               \
            result.index = _view->count;                                       \
                                                                               \
            return result;                                                     \
        }                                                                      \
                                                                               \
        low  = 0u;                                                             \
        high = _view->count;                                                   \
                                                                               \
        /* bisect, narrowing to the insertion point when the key is absent */  \
        while (low < high)                                                     \
        {                                                                      \
            mid   = low + ((high - low) / 2u);                                 \
            order = (compare_expr);                                            \
                                                                               \
            if (order == 0)                                                    \
            {                                                                  \
                result.record = d_lookup_at(_view, mid);                       \
                result.index  = mid;                                           \
                result.found  = true;                                          \
                                                                               \
                return result;                                                 \
            }                                                                  \
                                                                               \
            /* the record sorts before the key, so the key is to the right */ \
            if (order < 0)                                                     \
            {                                                                  \
                low = mid + 1u;                                                \
            }                                                                  \
            else                                                               \
            {                                                                  \
                high = mid;                                                    \
            }                                                                  \
        }                                                                      \
                                                                               \
        result.index = low;                                                    \
                                                                               \
        return result;                                                         \
    }

//   THE COMPARISON IS STORED-KEY AGAINST SOUGHT-KEY, in that order, so a
// negative result means the record sorts before the key and the bisection
// moves right. Reversing the operands reverses the search.
D_INTERNAL_LOOKUP_DEFINE_FIND(
    unsigned,
    uint64_t _key,
    d_lookup_compare_unsigned(d_lookup_key_unsigned(_view, mid), _key))

D_INTERNAL_LOOKUP_DEFINE_FIND(
    signed,
    int64_t _key,
    d_lookup_compare_signed(d_lookup_key_signed(_view, mid), _key))

D_INTERNAL_LOOKUP_DEFINE_FIND(
    string,
    const char* _key,
    d_lookup_compare_string(d_lookup_key_string(_view, mid),
                            _key,
                            ((_view->flags & D_LOOKUP_FLAG_CASE_INSENSITIVE)
                                 != 0u)))

D_INTERNAL_LOOKUP_DEFINE_FIND(
    bytes,
    const void* _key,
    d_lookup_compare_bytes(d_lookup_key_bytes(_view, mid),
                           _key,
                           (size_t)_view->key.size))

// d_lookup_find_field
//   function: find by whichever integral form the key field declares, so a
// caller holding only a descriptor need not branch on signedness itself.
//   INTEGRAL KEYS ONLY. A string or opaque key has no widened value to pass,
// and silently treating a pointer-valued key as an integer is the class of
// mistake this module exists to remove -- so those go through the named entry
// points above.
D_NODISCARD D_INLINE struct d_lookup_result
d_lookup_find_field(
    const struct d_lookup_view* _view,
    int64_t                     _key
)
{
    struct d_lookup_result result = D_LOOKUP_RESULT_MISS;

    // an empty view misses at insertion point zero
    if (d_lookup_view_is_empty(_view))
    {
        return result;
    }

    // the descriptor, not the call site, decides how the key is signed
    if (d_kv_field_is_signed(&_view->key))
    {
        return d_lookup_find_signed(_view, _key);
    }

    return d_lookup_find_unsigned(_view, (uint64_t)_key);
}

// d_lookup_contains
//   function: whether the key is present, for a caller that wants the verdict
// and not the record.
D_NODISCARD D_INLINE bool
d_lookup_contains(
    const struct d_lookup_view* _view,
    int64_t                     _key
)
{
    return d_lookup_find_field(_view, _key).found;
}


// VI.    iteration

// D_LOOKUP_FOR_EACH
//   macro: walk every record of a view, binding `var` to each in turn. `view`
// is evaluated once; `var` must already be declared as a `const void*`.
#define D_LOOKUP_FOR_EACH(view, index_var, var)                                \
    for ((index_var) = 0u,                                                     \
             (var) = d_lookup_at((view), 0u);                                  \
         (var) != NULL;                                                        \
         ++(index_var),                                                        \
             (var) = d_lookup_at((view), (index_var)))


// VII.   layout assertions
//   THESE ARE DERIVED, NOT LITERAL, and that is not laziness. Both structs
// below hold a pointer and a size_t, so their exact offsets differ between
// LP64, LLP64 and ILP32 targets. A literal `sizeof == 40` would assert the
// host rather than the layout. What is asserted instead is what actually
// matters: the ordering of the members and the absence of interior padding.

D_STATIC_ASSERT(offsetof(struct d_lookup_view, records) == 0,
                "d_lookup_view layout drift: records must lead");
D_STATIC_ASSERT(offsetof(struct d_lookup_view, stride) <
                    offsetof(struct d_lookup_view, count),
                "d_lookup_view layout drift: stride must precede count");
D_STATIC_ASSERT(offsetof(struct d_lookup_view, count) <
                    offsetof(struct d_lookup_view, key),
                "d_lookup_view layout drift: count must precede key");
D_STATIC_ASSERT(sizeof(struct d_lookup_view) ==
                    ( sizeof(const void*) + sizeof(size_t) + sizeof(size_t) +
                      sizeof(struct d_kv_field) + sizeof(uint16_t) +
                      sizeof(uint16_t) ),
                "d_lookup_view: a member width has introduced padding");

D_STATIC_ASSERT(offsetof(struct d_lookup_result, record) == 0,
                "d_lookup_result layout drift: record must lead");
D_STATIC_ASSERT(offsetof(struct d_lookup_result, record) <
                    offsetof(struct d_lookup_result, index),
                "d_lookup_result layout drift: record must precede index");
D_STATIC_ASSERT(offsetof(struct d_lookup_result, index) <
                    offsetof(struct d_lookup_result, found),
                "d_lookup_result layout drift: index must precede found");

//   THE STRIDE MUST BE ABLE TO SERVE AS A kv CAPACITY. d_lookup_view_make
// refuses a stride wider than UINT32_MAX for this reason; the assertion states
// the relation the refusal is protecting.
D_STATIC_ASSERT(sizeof(((struct d_lookup_view*)0)->stride) >=
                    sizeof(uint32_t),
                "d_lookup_view: stride cannot address a record");

//   THE FLAG SET IS CLOSED. The mask is only meaningful while every defined
// flag sits inside it.
D_STATIC_ASSERT(( D_LOOKUP_FLAG_SORTED           |
                  D_LOOKUP_FLAG_KEY_IS_POINTER   |
                  D_LOOKUP_FLAG_CASE_INSENSITIVE ) == D_LOOKUP_FLAG_MASK,
                "d_lookup_view: a flag has escaped D_LOOKUP_FLAG_MASK");


D_EXTERN_C_END


#endif  // DJINTERP_C_LOOKUP_
