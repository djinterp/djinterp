/******************************************************************************
* djinterp [meta]                                                        kv.h
*
* Extraction of a key and a value from a byte-oriented record, at offsets and
* widths the record itself declares. Tier 0 -- compiled by BOTH faces.
*
*
* THE PACKED DEFAULT WAS THE BUG, AND IT IS WHY THIS REVISION EXISTS
* ==================================================================
*   The previous form computed a value's offset as `sizeof(key_type)`:
*
*       #define D_KV_VALUE(_KeyType, _ValueType, _Ptr, _Out)                 \
*           D_EXTRACT_TYPE_AT(_ValueType, (_Ptr), sizeof(_KeyType), (_Out))
*
* which is the offset only when the compiler inserted no padding between the
* two members. For the records this framework actually holds, it does.
* `struct d_test_kv` is { uint32_t key; void* value; } -- key at 0, value at
* 8 -- and sizeof(uint32_t) is 4. The macro read four bytes of padding and the
* top half of the pointer, returned something plausible, and compiled clean:
*
*       sizeof(uint32_t)     = 4    <- what the macro used
*       offsetof(row, value) = 8    <- what was correct
*       D_KV_VALUE  -> 0x5566778800000000
*       offsetof    -> 0x1122334455667788
*
*   offsetof IS THE ONLY CORRECT SOURCE FOR A MEMBER'S OFFSET, and it is the
* right primitive for a second reason: it is the same token in C and in C++,
* defined for standard-layout types in both. One spelling therefore serves a C
* static initialiser and the C++ face's `lower()` alike, and neither language
* has to know the other's notation. That is what makes section I the bridge
* between `d_option`'s value column and `registry.hpp`'s _KeyProj / _ValueProj.
*
*   THE PACKED FORMS SURVIVE, UNDER A NAME THAT SAYS SO. A wire format, a
* serialised row, a hand-built byte buffer -- these genuinely have no padding,
* and computing offsets from widths is correct there. They are spelled
* D_KV_*_PACKED, so a call site declares the record is packed rather than
* assuming it silently, and D_KV_ASSERT_PACKED turns that declaration into a
* compile error on the day a member is widened or reordered.
*
*
* WHAT ELSE CHANGED, SO IT IS NOT FOUND BY SURPRISE
* =================================================
*   Three defects sat in the same lines as the offset one and are fixed here
* rather than left for a second pass.
*
*   A NARROW READ HAD NO ENDIAN BRANCH. `memcpy` of `size` bytes into a wider
* carrier lands in the LOW ADDRESSES, which is the low-order bytes only on a
* little-endian target. Same call, different value, no diagnostic.
* `byte_array.h` already carries D_ENV_ARCH_IS_BIG_ENDIAN for exactly this.
*
*   A NARROW SIGNED READ HAD NO SIGN EXTENSION. An int16_t of -1 read through
* a two-byte request returned 65535. There was no signed path at all, so a
* generated int32_t family was silently wrong at every width below its own.
*
*   NOTHING COULD BOUNDS-CHECK. No function took a capacity, so no signature
* could refuse a read past the end of the record. Every runtime entry point
* takes one now, and the comparison is done in 64 bits because the obvious
* 32-bit form answers "it fits" for an offset near UINT32_MAX.
*
*   TWO THINGS ARE WITHDRAWN. Width-parameterised POINTER reads are gone:
* asking for four bytes of a pointer on a 64-bit target produced a half-
* initialised result, and a pointer is not width-configurable data.
* `d_kv_load_pointer` reads sizeof(void*) or nothing. The example
* instantiations moved out of the header, where they compiled into every
* includer used or not.
*
*
* WHAT THIS HEADER IS NOT
* =======================
*   This is the FIELD and ACCESS layer only. The interned-key vocabulary --
* the reserved zero shared by d_test_key_id, d_test_callable_id and
* DTestMetadataFlag -- and the carrier that d_option_carrier becomes are
* separate sections and land separately. A consumer takes the layers it needs.
*
*   IT DOES NOT ABSORB struct d_type_struct_field. That triple is the same
* shape and was the precedent for this one, but it sits inside
* `d_type_struct_ext` behind a flexible array member and its widths are 16-bit
* because that descriptor gets written down. Widening it would change a
* serialised format to buy a spelling, so it converts instead -- see
* d_kv_field_from_struct_field in section I.
*
*
* path:      /inc/djinterp/c/meta/kv.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.05
******************************************************************************/

#ifndef DJINTERP_C_KV_
#define DJINTERP_C_KV_ 1

// std
// c
#include <stddef.h>
#include <stdint.h>
#include <string.h>
// djinterp
#include "../djinterp.h"
#include "type_info.h"


//   djinterp.h IS MANDATORY AND FIRST. It carries D_EXTERN_C_BEGIN, and a
// header that lets that macro become conditional trades a compile-time
// diagnostic naming this file for a link error naming nothing. It also carries
// D_ENV_ARCH_IS_BIG_ENDIAN by way of env_arch.h, without which section III
// cannot be written.

D_EXTERN_C_BEGIN


// I.     the field descriptor

// d_kv_field
//   struct: where a datum sits inside a record, how wide it is, and what its
// bytes mean. The offset is relative to a base the CALLER supplies; this
// struct never holds one, which is what lets one descriptor address a cell, a
// value block, or a mapped file.
struct d_kv_field
{
    uint32_t      offset;   // bytes from the caller's base
    uint32_t      size;     // width of the datum, in bytes
    d_type_info16 type;     // what the bytes mean; 0 when untyped
    uint16_t      flags;    // D_KV_FLAG_*; reserved bits written zero
};

// D_KV_FLAG_NONE
//   constant: no flags. The ordinary descriptor.
#define D_KV_FLAG_NONE      ((uint16_t)0u)

// D_KV_FLAG_SIGNED
//   constant: the datum is a signed integer. The one flag that changes
// behaviour: a descriptor-driven read cannot infer signedness from a width, so
// it is recorded here rather than guessed at the call site.
#define D_KV_FLAG_SIGNED    ((uint16_t)(1u << 0))

// D_KV_FLAG_MASK
//   constant: every flag this version defines. Bits outside the mask are
// reserved and must be written zero, so a later reader can tell an old
// descriptor from a corrupt one.
#define D_KV_FLAG_MASK      ((uint16_t)0x0001u)

// D_KV_OFFSET
//   macro: a member's offset, taken from the record rather than computed from
// the widths that precede it.
#define D_KV_OFFSET(record, member)     ((uint32_t)offsetof(record, member))

// D_KV_WIDTH
//   macro: a member's width. The sizeof operand is UNEVALUATED, so the null
// pointer is never dereferenced and the form is defined in both languages. It
// is the standard idiom, spelled once so no call site invents a second.
#define D_KV_WIDTH(record, member)                                             \
    ((uint32_t)sizeof(((record*)0)->member))

// D_KV_FIELD_OF
//   macro: the descriptor for a member of a standard-layout record. THE
// LOWERING, and a macro rather than a constexpr function on purpose: offsetof
// is defined in both dialects, so this one spelling serves a C static
// initialiser table and the C++ face's lower(). A constexpr function taking a
// pointer-to-member cannot be written without the null-dereference trick,
// which is undefined however universally it happens to work.
#define D_KV_FIELD_OF(record, member, info)                                    \
{                                                                              \
    D_KV_OFFSET(record, member), D_KV_WIDTH(record, member),                   \
    (d_type_info16)(info), D_KV_FLAG_NONE                                      \
}

// D_KV_FIELD_OF_SIGNED
//   macro: the same, for a member whose bytes are a signed integer.
#define D_KV_FIELD_OF_SIGNED(record, member, info)                             \
{                                                                              \
    D_KV_OFFSET(record, member), D_KV_WIDTH(record, member),                   \
    (d_type_info16)(info), D_KV_FLAG_SIGNED                                    \
}

// D_KV_FIELD_INIT
//   macro: the empty descriptor -- width zero, which every operation below
// reads as "there is nothing there". Spelled once so no call site invents a
// second spelling of an absent column.
#define D_KV_FIELD_INIT     { 0u, 0u, (d_type_info16)0, D_KV_FLAG_NONE }

// D_KV_ASSERT_PACKED
//   macro: a compile-time check that two members really are adjacent with no
// padding between them. PUT THIS BESIDE EVERY _PACKED USE in section IV: it is
// what turns "the record happens to be packed today" into a diagnostic on the
// day it stops being, which is the failure the old default form had no way to
// report.
#define D_KV_ASSERT_PACKED(record, key_member, value_member)                   \
    D_STATIC_ASSERT(offsetof(record, value_member) ==                          \
                        (size_t)D_KV_WIDTH(record, key_member),                \
                    #record ": " #value_member " is not packed against "       \
                    #key_member " -- the _PACKED forms would read padding")


// II.    descriptor construction and inspection

// d_kv_field_make
//   function: a descriptor from loose parts, for a layout discovered at run
// time rather than declared in a struct.
D_NODISCARD D_INLINE struct d_kv_field
d_kv_field_make(
    uint32_t      _offset,
    uint32_t      _size,
    d_type_info16 _type,
    uint16_t      _flags
)
{
    struct d_kv_field field;

    field.offset = _offset;
    field.size   = _size;
    field.type   = _type;
    field.flags  = (uint16_t)(_flags & D_KV_FLAG_MASK);

    return field;
}

// d_kv_field_from_struct_field
//   function: convert type_info_common.h's descriptor to this one. The two are
// NOT merged, for the reason the file header gives: that triple is part of a
// serialised descriptor format and widening it would change the format.
D_NODISCARD D_INLINE struct d_kv_field
d_kv_field_from_struct_field(
    const struct d_type_struct_field* _field
)
{
    struct d_kv_field result = D_KV_FIELD_INIT;

    // a null descriptor describes nothing, which is the empty field
    if (!_field)
    {
        return result;
    }

    result.offset = (uint32_t)_field->offset;
    result.size   = (uint32_t)_field->size;
    result.type   = _field->type;
    result.flags  = D_TYPE_IS_SIGNED(_field->type) ? D_KV_FLAG_SIGNED
                                                   : D_KV_FLAG_NONE;

    return result;
}

// d_kv_field_is_empty
//   function: a zero-width descriptor, which is the same answer as "this
// record has no such datum" -- what a unary option's value column is.
D_NODISCARD D_INLINE bool
d_kv_field_is_empty(
    const struct d_kv_field* _field
)
{
    return ( (!_field) ||
             (_field->size == 0u) );
}

// d_kv_field_is_signed
//   function: whether the descriptor says its bytes are a signed integer.
D_NODISCARD D_INLINE bool
d_kv_field_is_signed(
    const struct d_kv_field* _field
)
{
    return ( (_field != NULL) &&
             ((_field->flags & D_KV_FLAG_SIGNED) != 0u) );
}

// d_kv_disjoint
//   function: whether two fields share no byte. This is registry.hpp's
// _KeyDisjoint arrived at by arithmetic instead of by declaration: its cases
// (A) and (B) are two disjoint fields, and case (C) -- the whole-record value
// -- is a value field that contains the key field.
D_NODISCARD D_INLINE bool
d_kv_disjoint(
    const struct d_kv_field* _lhs,
    const struct d_kv_field* _rhs
)
{
    uint64_t lhs_end;
    uint64_t rhs_end;

    // a field that does not exist overlaps nothing
    if ( (!_lhs) ||
         (!_rhs) )
    {
        return true;
    }

    lhs_end = (uint64_t)_lhs->offset + (uint64_t)_lhs->size;
    rhs_end = (uint64_t)_rhs->offset + (uint64_t)_rhs->size;

    return ( (lhs_end <= (uint64_t)_rhs->offset) ||
             (rhs_end <= (uint64_t)_lhs->offset) );
}


// III.   bounds, packing and addressing

// d_kv_fits
//   function: whether [_offset, _offset + _size) lies inside _capacity.
//   THE ADDITION IS DONE IN 64 BITS, and that is the whole point of the
// function. `(_offset + _size) <= _capacity` in 32-bit arithmetic answers
// "yes, it fits" for an offset near UINT32_MAX, which is the bounds check
// failing in exactly the case a bounds check exists for.
D_NODISCARD D_INLINE bool
d_kv_fits(
    uint32_t _offset,
    uint32_t _size,
    uint32_t _capacity
)
{
    return ( ((uint64_t)_offset + (uint64_t)_size) <= (uint64_t)_capacity );
}

// d_kv_field_fits
//   function: the same question asked of a descriptor.
D_NODISCARD D_INLINE bool
d_kv_field_fits(
    const struct d_kv_field* _field,
    uint32_t                 _capacity
)
{
    // an absent field cannot fit, because there is nothing to place
    if (d_kv_field_is_empty(_field))
    {
        return false;
    }

    return d_kv_fits(_field->offset, _field->size, _capacity);
}

// d_kv_align_up
//   function: round _offset up to a multiple of _align. This is the packing
// step `d_option_set_add` performs when it assigns a slot from the running
// high-water mark.
//   A NON-POWER-OF-TWO ALIGNMENT IS RETURNED UNCHANGED rather than producing a
// wrong answer: there is nothing sensible to round to, and a silent adjustment
// would be worse than none.
D_NODISCARD D_INLINE uint32_t
d_kv_align_up(
    uint32_t _offset,
    uint32_t _align
)
{
    uint32_t mask;

    // reject a zero or non-power-of-two alignment before computing a mask
    if ( (_align == 0u) ||
         ((_align & (_align - 1u)) != 0u) )
    {
        return _offset;
    }

    mask = (uint32_t)(_align - 1u);

    return (uint32_t)((_offset + mask) & ~mask);
}

// d_kv_at
//   function: the address of a field within a base. NULL when the base is
// null, so the result can be tested rather than the arguments.
D_NODISCARD D_INLINE void*
d_kv_at(
    void*    _base,
    uint32_t _offset
)
{
    // there is no address inside a null record
    if (!_base)
    {
        return NULL;
    }

    return (void*)(((unsigned char*)_base) + _offset);
}

// d_kv_at_const
//   function: the const form of d_kv_at.
D_NODISCARD D_INLINE const void*
d_kv_at_const(
    const void* _base,
    uint32_t    _offset
)
{
    // there is no address inside a null record
    if (!_base)
    {
        return NULL;
    }

    return (const void*)(((const unsigned char*)_base) + _offset);
}


// IV.    runtime access
//   Every entry point here takes a capacity and refuses a read that would
// leave the record. memcpy rather than a cast throughout: _offset is
// arbitrary, so the cast form is an alignment fault on every target that traps
// and a strict-aliasing violation on all of them. Every compiler this
// framework targets folds a memcpy of a constant width to a single load.

// d_kv_load_unsigned
//   function: read _size bytes as an unsigned value, widened to the carrier.
// Zero for a null base, a width outside 1..8, or a read that would leave the
// record -- the same answer as "there is nothing there".
D_NODISCARD D_INLINE uint64_t
d_kv_load_unsigned(
    const void* _base,
    uint32_t    _offset,
    uint32_t    _size,
    uint32_t    _capacity
)
{
    uint64_t             value;
    const unsigned char* at;

    // refuse a null record, an unrepresentable width, or a read past the end
    if ( (!_base)                                  ||
         (_size == 0u)                             ||
         (_size > 8u)                              ||
         (!d_kv_fits(_offset, _size, _capacity)) )
    {
        return 0u;
    }

    value = 0u;
    at    = ((const unsigned char*)_base) + _offset;

    //   A NARROW READ LANDS IN THE LOW ADDRESSES, which is the low-order bytes
    // only on a little-endian target. Without this branch the same call
    // returns two different values on two targets and reports nothing.
#if D_ENV_ARCH_IS_BIG_ENDIAN
    memcpy(((unsigned char*)&value) + (8u - _size), at, (size_t)_size);
#else
    memcpy(&value, at, (size_t)_size);
#endif

    return value;
}

// d_kv_load_signed
//   function: the same read, sign-extended from _size bytes.
//   THE EXTENSION IS (v ^ m) - m, NOT A SHIFT PAIR. Shifting left into the
// sign bit and arithmetic-shifting back is the familiar spelling and its right
// shift is implementation-defined; this form is defined by the standard at
// every width.
D_NODISCARD D_INLINE int64_t
d_kv_load_signed(
    const void* _base,
    uint32_t    _offset,
    uint32_t    _size,
    uint32_t    _capacity
)
{
    uint64_t value;
    uint64_t mask;

    value = d_kv_load_unsigned(_base, _offset, _size, _capacity);

    // a full-width or absent read has no sign bit to propagate
    if ( (_size == 0u) ||
         (_size >= 8u) )
    {
        return (int64_t)value;
    }

    mask = ((uint64_t)1) << ((_size * 8u) - 1u);

    return (int64_t)((value ^ mask) - mask);
}

// d_kv_load_pointer
//   function: read a pointer. NOT WIDTH-PARAMETERISED, deliberately: the
// previous header let a caller ask for four bytes of a pointer on a 64-bit
// target and returned the half-initialised result without complaint. A pointer
// is not width-configurable data, so this reads sizeof(void*) or nothing.
D_NODISCARD D_INLINE const void*
d_kv_load_pointer(
    const void* _base,
    uint32_t    _offset,
    uint32_t    _capacity
)
{
    const void* value;

    // refuse a null record or a read that would leave it
    if ( (!_base) ||
         (!d_kv_fits(_offset, (uint32_t)sizeof(const void*), _capacity)) )
    {
        return NULL;
    }

    value = NULL;

    memcpy(&value,
           ((const unsigned char*)_base) + _offset,
           sizeof(const void*));

    return value;
}

// d_kv_store_unsigned
//   function: write _value into _size bytes. A value too wide for the slot is
// REFUSED rather than truncated -- the same choice d_option_write makes about
// a width mismatch, and for the same reason: a truncating write reads as
// success and is discovered as corrupt data much later.
D_NODISCARD D_INLINE bool
d_kv_store_unsigned(
    void*    _base,
    uint32_t _offset,
    uint32_t _size,
    uint32_t _capacity,
    uint64_t _value
)
{
    unsigned char* at;
    uint64_t       limit;

    // refuse a null record, an unrepresentable width, or a write past the end
    if ( (!_base)                                  ||
         (_size == 0u)                             ||
         (_size > 8u)                              ||
         (!d_kv_fits(_offset, _size, _capacity)) )
    {
        return false;
    }

    // a narrow slot cannot carry every value the carrier can
    if (_size < 8u)
    {
        limit = (((uint64_t)1) << (_size * 8u)) - 1u;

        if (_value > limit)
        {
            return false;
        }
    }

    at = ((unsigned char*)_base) + _offset;

#if D_ENV_ARCH_IS_BIG_ENDIAN
    memcpy(at, ((const unsigned char*)&_value) + (8u - _size), (size_t)_size);
#else
    memcpy(at, &_value, (size_t)_size);
#endif

    return true;
}

// d_kv_store_signed
//   function: the signed counterpart. The range test is against the slot's
// two's-complement bounds, so a value that would change sign on narrowing is
// refused rather than stored.
D_NODISCARD D_INLINE bool
d_kv_store_signed(
    void*    _base,
    uint32_t _offset,
    uint32_t _size,
    uint32_t _capacity,
    int64_t  _value
)
{
    int64_t  high;
    int64_t  low;
    uint64_t mask;

    // a width the carrier cannot express is refused before any arithmetic
    if ( (_size == 0u) ||
         (_size > 8u) )
    {
        return false;
    }

    mask = ~(uint64_t)0;

    // a narrow slot has bounds the carrier does not
    if (_size < 8u)
    {
        high = (int64_t)((((uint64_t)1) << ((_size * 8u) - 1u)) - 1u);
        low  = -high - 1;

        if ( (_value > high) ||
             (_value < low) )
        {
            return false;
        }

        mask = (((uint64_t)1) << (_size * 8u)) - 1u;
    }

    return d_kv_store_unsigned(_base,
                               _offset,
                               _size,
                               _capacity,
                               ((uint64_t)_value & mask));
}

// d_kv_read
//   function: copy opaque bytes out. _out_size must EQUAL the slot width; a
// mismatch is a refusal rather than a partial copy, which is what
// d_option_read already promises about its own out-parameter.
D_NODISCARD D_INLINE bool
d_kv_read(
    const void* _base,
    uint32_t    _offset,
    uint32_t    _size,
    uint32_t    _capacity,
    void*       _out,
    size_t      _out_size
)
{
    // refuse a null argument, an absent slot, a width mismatch, or an overrun
    if ( (!_base)                                  ||
         (!_out)                                   ||
         (_size == 0u)                             ||
         ((size_t)_size != _out_size)              ||
         (!d_kv_fits(_offset, _size, _capacity)) )
    {
        return false;
    }

    memcpy(_out,
           ((const unsigned char*)_base) + _offset,
           (size_t)_size);

    return true;
}

// d_kv_write
//   function: copy opaque bytes in, under the same width contract as
// d_kv_read.
D_NODISCARD D_INLINE bool
d_kv_write(
    void*       _base,
    uint32_t    _offset,
    uint32_t    _size,
    uint32_t    _capacity,
    const void* _in,
    size_t      _in_size
)
{
    // refuse a null argument, an absent slot, a width mismatch, or an overrun
    if ( (!_base)                                  ||
         (!_in)                                    ||
         (_size == 0u)                             ||
         ((size_t)_size != _in_size)               ||
         (!d_kv_fits(_offset, _size, _capacity)) )
    {
        return false;
    }

    memcpy(((unsigned char*)_base) + _offset,
           _in,
           (size_t)_size);

    return true;
}


// V.     descriptor-driven access
//   The same operations addressed by a stored descriptor rather than by loose
// arguments. This is the form a container holds: `d_registry` keeps one key
// field and one value field for every row it owns, which is what lets it offer
// a generic get and set at all.
//   A CELL PROJECTS, A CONTAINER STORES. A descriptor that is the same for
// every element belongs on the container; one that varies per element and sits
// in a hot, scanned array is built on demand instead, so `d_option` returns
// one by value and its cell stays 24 bytes with its exact layout assertions
// intact.

// d_kv_field_load
//   function: read through a descriptor, signed or unsigned as the
// descriptor's D_KV_FLAG_SIGNED says. The result is int64_t because it must
// carry either; an unsigned 64-bit slot at its top end is the one value this
// cannot represent, and such a slot is read with d_kv_load_unsigned directly.
D_NODISCARD D_INLINE int64_t
d_kv_field_load(
    const struct d_kv_field* _field,
    const void*              _base,
    uint32_t                 _capacity
)
{
    // an absent column reads as zero, not as an error
    if (d_kv_field_is_empty(_field))
    {
        return 0;
    }

    // the descriptor, not the call site, decides how the bytes are signed
    if (d_kv_field_is_signed(_field))
    {
        return d_kv_load_signed(_base,
                                _field->offset,
                                _field->size,
                                _capacity);
    }

    return (int64_t)d_kv_load_unsigned(_base,
                                       _field->offset,
                                       _field->size,
                                       _capacity);
}

// d_kv_field_read
//   function: opaque copy out through a descriptor.
D_NODISCARD D_INLINE bool
d_kv_field_read(
    const struct d_kv_field* _field,
    const void*              _base,
    uint32_t                 _capacity,
    void*                    _out,
    size_t                   _out_size
)
{
    // an absent column has nothing to copy
    if (d_kv_field_is_empty(_field))
    {
        return false;
    }

    return d_kv_read(_base,
                     _field->offset,
                     _field->size,
                     _capacity,
                     _out,
                     _out_size);
}

// d_kv_field_write
//   function: opaque copy in through a descriptor.
D_NODISCARD D_INLINE bool
d_kv_field_write(
    const struct d_kv_field* _field,
    void*                    _base,
    uint32_t                 _capacity,
    const void*              _in,
    size_t                   _in_size
)
{
    // an absent column has nowhere to copy to
    if (d_kv_field_is_empty(_field))
    {
        return false;
    }

    return d_kv_write(_base,
                      _field->offset,
                      _field->size,
                      _capacity,
                      _in,
                      _in_size);
}


// VI.    record-derived extraction
//   The call-site forms, for a record whose type is known where the read
// happens. These take the RECORD and the MEMBER NAMES, so offsets come from
// offsetof and widths from sizeof, and padding is accounted for by
// construction rather than by the caller remembering it exists.
//   THE POINTER ARGUMENT IS EVALUATED ONCE in every form below. The previous
// D_KV_BOTH expanded it twice, so D_KV_BOTH(K, V, next_row(), out) advanced
// the cursor twice and read two different rows into one pair.

// D_KV_GET
//   macro: read one member of a record into `out`. The width comes from the
// member, so `out` must be at least that wide.
#define D_KV_GET(record, member, ptr, out)                                     \
    do                                                                         \
    {                                                                          \
        const unsigned char* d_internal_kv_base = (const unsigned char*)(ptr); \
                                                                               \
        memcpy(&(out),                                                         \
               d_internal_kv_base + D_KV_OFFSET(record, member),               \
               (size_t)D_KV_WIDTH(record, member));                            \
    } while (0)

// D_KV_KEY_OF
//   macro: read the key column. Identical to D_KV_GET and spelled separately
// so a reader of a call site can see which column is touched without going and
// looking at the record.
#define D_KV_KEY_OF(record, key_member, ptr, out)                              \
    D_KV_GET(record, key_member, (ptr), (out))

// D_KV_VALUE_OF
//   macro: read the value column.
#define D_KV_VALUE_OF(record, value_member, ptr, out)                          \
    D_KV_GET(record, value_member, (ptr), (out))

// D_KV_BOTH_OF
//   macro: both halves from one record, each at its own true offset. THE FORM
// THAT REPLACES THE OLD PACKED DEFAULT, and the one to reach for unless the
// record is genuinely a wire format.
#define D_KV_BOTH_OF(record, key_member, value_member, ptr, out)               \
    do                                                                         \
    {                                                                          \
        const unsigned char* d_internal_kv_base = (const unsigned char*)(ptr); \
                                                                               \
        memcpy(&((out).key),                                                   \
               d_internal_kv_base + D_KV_OFFSET(record, key_member),           \
               (size_t)D_KV_WIDTH(record, key_member));                        \
        memcpy(&((out).value),                                                 \
               d_internal_kv_base + D_KV_OFFSET(record, value_member),         \
               (size_t)D_KV_WIDTH(record, value_member));                      \
    } while (0)


// VII.   explicit and packed extraction
//   For a buffer that is not a declared struct. The explicit forms were
// already correct -- they take the offset outright -- and are kept but for the
// single-evaluation fix.
//   THE PACKED FORMS ARE THE OLD DEFAULTS UNDER AN HONEST NAME. Computing a
// value's offset from the key's WIDTH is right for a serialised row, a wire
// record, or a hand-built byte buffer, and wrong for every padded struct.
// Spelling it _PACKED makes a call site declare which it has, and
// D_KV_ASSERT_PACKED turns that declaration into a compile error on the day it
// stops being true.

// D_KV_AT
//   macro: read from an explicit offset at the width of a type.
#define D_KV_AT(type, ptr, offset, out)                                        \
    do                                                                         \
    {                                                                          \
        const unsigned char* d_internal_kv_base = (const unsigned char*)(ptr); \
                                                                               \
        memcpy(&(out),                                                         \
               d_internal_kv_base + (size_t)(offset),                          \
               sizeof(type));                                                  \
    } while (0)

// D_KV_AT_SIZE
//   macro: read from an explicit offset at an explicit width.
#define D_KV_AT_SIZE(ptr, offset, size, out)                                   \
    do                                                                         \
    {                                                                          \
        const unsigned char* d_internal_kv_base = (const unsigned char*)(ptr); \
                                                                               \
        memcpy(&(out),                                                         \
               d_internal_kv_base + (size_t)(offset),                          \
               (size_t)(size));                                                \
    } while (0)

// D_KV_VALUE_PACKED
//   macro: the value of a PACKED key/value record -- offset sizeof(key_type),
// width sizeof(value_type). Pair every use with D_KV_ASSERT_PACKED.
#define D_KV_VALUE_PACKED(key_type, value_type, ptr, out)                      \
    D_KV_AT(value_type, (ptr), sizeof(key_type), (out))

// D_KV_BOTH_PACKED
//   macro: both halves of a PACKED record. Pair every use with
// D_KV_ASSERT_PACKED.
#define D_KV_BOTH_PACKED(key_type, value_type, ptr, out)                       \
    do                                                                         \
    {                                                                          \
        const unsigned char* d_internal_kv_base = (const unsigned char*)(ptr); \
                                                                               \
        memcpy(&((out).key),                                                   \
               d_internal_kv_base,                                             \
               sizeof(key_type));                                              \
        memcpy(&((out).value),                                                 \
               d_internal_kv_base + sizeof(key_type),                          \
               sizeof(value_type));                                            \
    } while (0)


// VIII.  generated typed families
//   C has no function templates, so a caller that wants a typed return rather
// than an out-parameter needs a generated family. These generate THIN WRAPPERS
// over section IV, not a copy of its logic per type, so N types cost N
// narrowing casts and the endian branch, the sign extension and the bounds
// check exist exactly once.
//   NO POINTER FAMILY. d_kv_load_pointer is the pointer path and it is not
// width-parameterised. Instantiating a family over void* is what produced a
// half-initialised pointer from a four-byte request.

// D_KV_DEFINE_UNSIGNED
//   macro: generate d_kv_<name>_at and d_kv_<name>_field for an unsigned
// integral type. _at reads at the TYPE's width; _field reads at the width the
// descriptor declares.
#define D_KV_DEFINE_UNSIGNED(type, name)                                       \
    D_NODISCARD D_INLINE type                                                  \
    d_kv_##name##_at(                                                          \
        const void* _base,                                                     \
        uint32_t    _offset,                                                   \
        uint32_t    _capacity                                                  \
    )                                                                          \
    {                                                                          \
        return (type)d_kv_load_unsigned(_base,                                 \
                                        _offset,                               \
                                        (uint32_t)sizeof(type),                \
                                        _capacity);                            \
    }                                                                          \
                                                                               \
    D_NODISCARD D_INLINE type                                                  \
    d_kv_##name##_field(                                                       \
        const struct d_kv_field* _field,                                       \
        const void*              _base,                                        \
        uint32_t                 _capacity                                     \
    )                                                                          \
    {                                                                          \
        if (d_kv_field_is_empty(_field))                                       \
        {                                                                      \
            return (type)0;                                                    \
        }                                                                      \
                                                                               \
        return (type)d_kv_load_unsigned(_base,                                 \
                                        _field->offset,                        \
                                        _field->size,                          \
                                        _capacity);                            \
    }

// D_KV_DEFINE_SIGNED
//   macro: the same for a signed integral type, routed through the
// sign-extending load.
#define D_KV_DEFINE_SIGNED(type, name)                                         \
    D_NODISCARD D_INLINE type                                                  \
    d_kv_##name##_at(                                                          \
        const void* _base,                                                     \
        uint32_t    _offset,                                                   \
        uint32_t    _capacity                                                  \
    )                                                                          \
    {                                                                          \
        return (type)d_kv_load_signed(_base,                                   \
                                      _offset,                                 \
                                      (uint32_t)sizeof(type),                  \
                                      _capacity);                              \
    }                                                                          \
                                                                               \
    D_NODISCARD D_INLINE type                                                  \
    d_kv_##name##_field(                                                       \
        const struct d_kv_field* _field,                                       \
        const void*              _base,                                        \
        uint32_t                 _capacity                                     \
    )                                                                          \
    {                                                                          \
        if (d_kv_field_is_empty(_field))                                       \
        {                                                                      \
            return (type)0;                                                    \
        }                                                                      \
                                                                               \
        return (type)d_kv_load_signed(_base,                                   \
                                      _field->offset,                          \
                                      _field->size,                            \
                                      _capacity);                              \
    }

//   THE FRAMEWORK'S OWN WIDTHS, AND NOTHING ELSE. A consumer that wants a
// family over its own type invokes the generator in ITS header; instantiating
// the whole matrix here is what compiled eight unused function families into
// every translation unit that included the previous version.
D_KV_DEFINE_UNSIGNED(uint8_t,  u8)
D_KV_DEFINE_UNSIGNED(uint16_t, u16)
D_KV_DEFINE_UNSIGNED(uint32_t, u32)
D_KV_DEFINE_UNSIGNED(uint64_t, u64)
D_KV_DEFINE_SIGNED(int8_t,  i8)
D_KV_DEFINE_SIGNED(int16_t, i16)
D_KV_DEFINE_SIGNED(int32_t, i32)
D_KV_DEFINE_SIGNED(int64_t, i64)


// IX.    layout assertions
//   The Layout law: one declaration, sizeof and offsetof asserted in BOTH
// dialects. This header is compiled by both, so these fire in both and drift
// becomes a compile error rather than a wire-format bug.

D_STATIC_ASSERT(sizeof(struct d_kv_field) == 12,
                "d_kv_field layout drift: expected 12 bytes");
D_STATIC_ASSERT(offsetof(struct d_kv_field, offset) == 0,
                "d_kv_field layout drift: offset must lead");
D_STATIC_ASSERT(offsetof(struct d_kv_field, size) == 4,
                "d_kv_field layout drift: size");
D_STATIC_ASSERT(offsetof(struct d_kv_field, type) == 8,
                "d_kv_field layout drift: type");
D_STATIC_ASSERT(offsetof(struct d_kv_field, flags) == 10,
                "d_kv_field layout drift: flags");

//   NO INTERIOR PADDING. The exact offsets above already imply it, but the sum
// is stated separately so that a build which changes d_type_info16's width
// fails with the reason rather than with four offsets at once.
D_STATIC_ASSERT(sizeof(struct d_kv_field) ==
                    ( sizeof(uint32_t) + sizeof(uint32_t) +
                      sizeof(d_type_info16) + sizeof(uint16_t) ),
                "d_kv_field: a member width has introduced padding");

//   THE DESCRIPTOR MUST DESCRIBE WHAT type_info CAN. A field narrower than
// d_type_struct_field's would silently truncate a converted descriptor, so the
// relation is asserted rather than assumed.
D_STATIC_ASSERT(sizeof(((struct d_kv_field*)0)->size) >=
                    sizeof(((struct d_type_struct_field*)0)->size),
                "d_kv_field: size is narrower than d_type_struct_field's");

//   THE FLAG SET IS CLOSED. The mask is only meaningful while every defined
// flag sits inside it.
D_STATIC_ASSERT((D_KV_FLAG_SIGNED & D_KV_FLAG_MASK) == D_KV_FLAG_SIGNED,
                "d_kv_field: a flag has escaped D_KV_FLAG_MASK");


D_EXTERN_C_END


#endif  // DJINTERP_C_KV_
