/******************************************************************************
* djinterp [meta]                                                        kv.h
*
* Reading and writing a datum at an offset and a width the caller supplies,
* without the alignment, aliasing, endian, sign-extension and overflow faults
* that each hand-written version of this gets wrong differently. Tier 0 is
* compiled by BOTH faces.
*
*
* THERE IS NO DESCRIPTOR TYPE HERE, AND THAT IS THE REVISION
* ==========================================================
*   An earlier form of this header carried `struct d_kv_field {offset, size,
* type, flags}` and a layer of operations addressed through it. It is gone, and
* the reasoning is worth keeping because it inverts what this module was first
* argued for.
*
*   THE DUPLICATION THAT CAUSED BUGS WAS IN THE OPERATIONS, NOT IN THE TYPE.
* Four modules had four load implementations with four different endian,
* sign-extension and bounds errors between them. Sharing the loads fixes that
* completely. Sharing a struct on top was tidiness, and it was weighed as
* though it carried the same weight.
*
*   THREE OF THE SIX CONSUMERS ONLY EVER PROJECTED ONE. d_option, test_cvar and
* test_metadata each had a function whose entire body copied three of their own
* members into a descriptor so a caller could read those three members back
* out. Those functions are pure ceremony and are deleted with the type.
*
*   THE CONSUMERS THAT STORE ONE ALL GOT SMALLER WITHOUT IT, because a 12-byte
* descriptor carrying a type and a flags word is heavier than the eight bytes
* most of them needed. Measured: d_nest_link 16 to 12, d_nest_children 32 to
* 24, d_nest_desc 132 to 104, the registry's pair 24 to 16. d_lookup_view is
* unchanged at 40, its padding absorbing the difference.
*
*   THE offsetof MACROS SURVIVE UNTOUCHED, and they were always the valuable
* part. D_KV_OFFSET and D_KV_WIDTH need no struct, and what made the C++ bridge
* work was offsetof being the same token in both dialects (not a descriptor
* wrapped around it).
*
*   WHAT IS GIVEN UP is the atomicity of the pair: an offset and a width can
* now drift apart with nothing to catch it. That cost is real and it is smaller
* than it looks, because every consumer that STORED a descriptor already held
* those as separate members and assigned them separately. The struct only ever
* enforced atomicity in the consumers that did not need a descriptor at all.
*
*   SIGNEDNESS IMPROVED BY LOSING ITS FLAG. It used to live in the descriptor
* and a dispatching load branched on it; it is now expressed by WHICH FUNCTION
* THE CALLER CALLS, at the site where the destination type is known. That moves
* the decision toward the information rather than away from it.
*
*
* THE PACKED DEFAULT WAS A BUG AND IS STILL WORTH NAMING
* =====================================================
*   An older form computed a value's offset as `sizeof(key_type)`, which is the
* offset only when the compiler inserted no padding. For the records this
* framework holds, it does. struct d_test_kv is { uint32_t key; void* value; }: 
* key at 0, value at 8, and sizeof(uint32_t) is 4, so the macro read four
* bytes of padding and the top half of the pointer and compiled clean.
*
*   offsetof IS THE ONLY CORRECT SOURCE FOR A MEMBER'S OFFSET. The packed forms
* survive in section V under a name that admits what they assume, because a
* wire record or a hand-built byte buffer genuinely has no padding, and
* D_KV_ASSERT_PACKED turns the assumption into a compile error on the day a
* member is widened or reordered.
*
*
* THE FOUR HAZARDS THIS MODULE EXISTS FOR
* =======================================
*   ALIGNMENT AND ALIASING. `_offset` is arbitrary, so a cast to a wider
* pointer type faults on every target that traps and violates strict aliasing
* on all of them. Every read below is a memcpy, which every compiler this
* framework targets folds to a single load at a constant width.
*
*   ENDIANNESS. A narrow read into a wide carrier lands in the LOW ADDRESSES,
* which is the low-order bytes only on a little-endian target. Same call,
* different value, no diagnostic.
*
*   SIGN EXTENSION. A narrow signed read has to propagate the sign bit, and the
* familiar shift-left-then-arithmetic-shift-right spelling has an
* implementation-defined right shift. (v ^ m) - m is defined at every width.
*
*   THE BOUNDS CHECK WRAPS. `(offset + size) <= capacity` in 32-bit arithmetic
* answers "yes, it fits" for an offset near UINT32_MAX, which is the check
* failing in exactly the case a check exists for.
*
*
* THIS HEADER IS A LEAF
* =====================
*   With the descriptor gone, nothing here holds a d_type_info16, so the
* type_info dependency goes with it. kv.h now needs djinterp.h and three C
* standard headers and nothing else, which is what lets every tier include it
* without an ordering question.
*
*
* path:      /inc/djinterp/c/meta/kv.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.06
******************************************************************************/

#ifndef DJINTERP_C_KV_
#define DJINTERP_C_KV_ 1

// std
#include <stddef.h>
#include <stdint.h>
#include <string.h>
// djinterp
#include "../djinterp.h"


D_EXTERN_C_BEGIN

// I.     layout macros

// D_KV_OFFSET
//   macro: a member's offset, taken from the record rather than computed from
// the widths that precede it, AS A TYPE THE CALLER NAMES.
//
//   THE WHOLE MODULE RESTS ON offsetof. It is the correct answer where
// `sizeof(previous_member)` is a guess, and it is the same token in C and C++,
// defined for standard-layout types in both, so one spelling serves a C
// static initialiser and the C++ face alike.
//
//   THE TYPE IS A PARAMETER BECAUSE THE MACRO CANNOT KNOW IT. An earlier form
// cast to uint32_t unconditionally, which presumed the caller's storage width:
// a consumer holding offsets in size_t got a narrowing followed by a widening,
// and D_KV_ASSERT_PACKED below had to cast the result straight back. Naming the
// type at the call site is the same move as everything else in this revision;
// the caller keeps track of width, so the caller says which width.
//
//   THE CAST IS UNCHECKED, and naming a narrow type deliberately is now easy,
// so pair a narrow one with D_KV_ASSERT_ADDRESSABLE below. A uint16_t offset
// into a record with a member past 65535 truncates silently and nothing else
// will say so.
#define D_KV_OFFSET(record, member, type)                                      \
    ((type)offsetof(record, member))

// D_KV_WIDTH
//   macro: a member's width, as a type the caller names. The sizeof operand is
// UNEVALUATED, so the null pointer is never dereferenced and the form is
// defined in both languages. It is the standard idiom, spelled once so no call
// site invents a second.
#define D_KV_WIDTH(record, member, type)                                       \
    ((type)sizeof(((record*)0)->member))

// D_KV_ASSERT_ADDRESSABLE
//   macro: a compile-time check that `type` can hold every offset into
// `record`, and that it is unsigned.
//   THE UNSIGNED TEST IS NOT DECORATION. Without it the size test passes
// vacuously for a signed type: (int16_t)(-1) widens to SIZE_MAX and every
// record compares smaller than it: so the assertion that was meant to catch
// a too-narrow offset would report nothing at all.
#define D_KV_ASSERT_ADDRESSABLE(record, type)                                  \
    D_STATIC_ASSERT(( (((type)-1) > 0) &&                                      \
                      (sizeof(record) <= (size_t)((type)-1)) ),                \
                    #record " does not fit an offset of type " #type           \
                    "; the type must be unsigned and wide enough")

// D_KV_ASSERT_PACKED
//   macro: a compile-time check that two members really are adjacent with no
// padding between them. PUT THIS BESIDE EVERY _PACKED USE in section V: it is
// what turns "the record happens to be packed today" into a diagnostic on the
// day it stops being, which is the failure the old default form had no way to
// report.
#define D_KV_ASSERT_PACKED(record, key_member, value_member)                   \
    D_STATIC_ASSERT(offsetof(record, value_member) ==                          \
                        D_KV_WIDTH(record, key_member, size_t),                \
                    #record ": " #value_member " is not packed against "       \
                    #key_member "; the _PACKED forms would read padding")


// II.    bounds, packing and addressing

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

// d_kv_disjoint
//   function: whether two spans share no byte.
//   FOUR SCALARS RATHER THAN TWO DESCRIPTORS, which is the one place dropping
// the descriptor type costs ergonomics rather than saving them. It is a
// rarely-called predicate and the trade is accepted rather than hidden.
//   This is registry.hpp's _KeyDisjoint arrived at by arithmetic instead of by
// declaration: two disjoint fields are the ordinary case, and a whole-record
// value that contains the key field is the one that is not.
D_NODISCARD D_INLINE bool
d_kv_disjoint(
    uint32_t _lhs_offset,
    uint32_t _lhs_size,
    uint32_t _rhs_offset,
    uint32_t _rhs_size
)
{
    uint64_t lhs_end;
    uint64_t rhs_end;

    // a span with no width overlaps nothing
    if ( (_lhs_size == 0u) ||
         (_rhs_size == 0u) )
    {
        return true;
    }

    lhs_end = (uint64_t)_lhs_offset + (uint64_t)_lhs_size;
    rhs_end = (uint64_t)_rhs_offset + (uint64_t)_rhs_size;

    return ( (lhs_end <= (uint64_t)_rhs_offset) ||
             (rhs_end <= (uint64_t)_lhs_offset) );
}

// d_kv_align_up
//   function: round _offset up to a multiple of _align. This is the packing
// step d_option_set_add performs when it assigns a slot from the running
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


// III.   runtime access
//   Every entry point here takes a capacity and refuses a read that would
// leave the record. memcpy rather than a cast throughout, for the reason the
// file header gives.

// d_kv_load_unsigned
//   function: read _size bytes as an unsigned value, widened to the carrier.
// Zero for a null base, a width outside 1..8, or a read that would leave the
// record; the same answer as "there is nothing there".
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
    if ( (!_base)       ||
         (_size == 0u)  ||
         (_size > 8u)   ||
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
    d_memcpy(((unsigned char*)&value) + (8u - _size), at, (size_t)_size);
#else
    d_memcpy(&value, at, (size_t)_size);
#endif

    return value;
}

// d_kv_load_signed
//   function: the same read, sign-extended from _size bytes.
//   THE CALLER PICKS THIS OR THE UNSIGNED FORM, and that is the improvement
// the descriptor's SIGNED flag used to obscure: the decision belongs at the
// site where the destination type is known, not in a bit a dispatching load
// branched on.
//   THE EXTENSION IS (v ^ m) - m, NOT A SHIFT PAIR. Shifting left into the
// sign bit and arithmetic-shifting back is the familiar spelling and its right
// shift is implementation-defined; this form is defined at every width.
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
//   function: read a pointer. NOT WIDTH-PARAMETERISED, deliberately: an
// earlier form let a caller ask for four bytes of a pointer on a 64-bit target
// and returned the half-initialised result without complaint. A pointer is not
// width-configurable data, so this reads sizeof(void*) or nothing.
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

    d_memcpy(&value,
            ((const unsigned char*)_base) + _offset,
            sizeof(const void*));

    return value;
}

// d_kv_store_unsigned
//   function: write _value into _size bytes. A value too wide for the slot is
// refused rather than truncated: the same choice d_option_write makes about
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
    if ( (!_base)       ||
         (_size == 0u)  ||
         (_size > 8u)   ||
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
    d_memcpy(at, ((const unsigned char*)&_value) + (8u - _size), (size_t)_size);
#else
    d_memcpy(at, &_value, (size_t)_size);
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
    if ( (!_base)                      ||
         (!_out)                       ||
         (_size == 0u)                 ||
         ((size_t)_size != _out_size)  ||
         (!d_kv_fits(_offset, _size, _capacity)) )
    {
        return false;
    }

    d_memcpy(_out,
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
    if ( (!_base)                     ||
         (!_in)                       ||
         (_size == 0u)                ||
         ((size_t)_size != _in_size)  ||
         (!d_kv_fits(_offset, _size, _capacity)) )
    {
        return false;
    }

    d_memcpy(((unsigned char*)_base) + _offset,
            _in,
            (size_t)_size);

    return true;
}


// IV.    record-derived extraction
//   The call-site forms, for a record whose type is known where the read
// happens. These take the RECORD and the MEMBER NAMES, so offsets come from
// offsetof and widths from sizeof, and padding is accounted for by
// construction rather than by the caller remembering it exists.
//   THE POINTER ARGUMENT IS EVALUATED ONCE in every form below. An earlier
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
               d_internal_kv_base + D_KV_OFFSET(record, member, size_t),       \
               D_KV_WIDTH(record, member, size_t));                            \
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
               d_internal_kv_base + D_KV_OFFSET(record, key_member, size_t),   \
               D_KV_WIDTH(record, key_member, size_t));                        \
        memcpy(&((out).value),                                                 \
               d_internal_kv_base + D_KV_OFFSET(record, value_member, size_t), \
               D_KV_WIDTH(record, value_member, size_t));                      \
    } while (0)


// V.     explicit and packed extraction
//   For a buffer that is not a declared struct. The explicit forms take the
// offset outright and were always correct.
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
//   macro: the value of a PACKED key/value record: offset sizeof(key_type), 
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


// VI.    generated typed families
//   C has no function templates, so a caller that wants a typed return rather
// than an out-parameter needs a generated family. These generate THIN WRAPPERS
// over section III, not a copy of its logic per type. So N types cost N
// narrowing casts and the endian branch, the sign extension and the bounds
// check exist exactly once.
//   NO POINTER FAMILY. d_kv_load_pointer is the pointer path and it is not
// width-parameterised. Instantiating a family over void* is what produced a
// half-initialised pointer from a four-byte request.

// D_KV_DEFINE_UNSIGNED
//   macro: generate d_kv_<name>_at and d_kv_<name>_narrow for an unsigned
// integral type. _at reads at the TYPE's width; _narrow reads at a width the
// caller names, which is what a stored slot description supplies now that
// there is no descriptor to pass.
#define D_KV_DEFINE_UNSIGNED(type, fn_name_at, fn_narrow_name)                 \
    D_NODISCARD D_INLINE type                                                  \
    fn_name_at(                                                                \
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
    fn_narrow_name(                                                            \
        const void* _base,                                                     \
        uint32_t    _offset,                                                   \
        uint32_t    _size,                                                     \
        uint32_t    _capacity                                                  \
    )                                                                          \
    {                                                                          \
        return (type)d_kv_load_unsigned(_base, _offset, _size, _capacity);     \
    }

// D_KV_DEFINE_SIGNED
//   macro: the same for a signed integral type, routed through the
// sign-extending load.
#define D_KV_DEFINE_SIGNED(type, fn_name_at, fn_narrow_name)                   \
    D_NODISCARD D_INLINE type                                                  \
    fn_name_at(                                                                \
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
    fn_narrow_name(                                                            \
        const void* _base,                                                     \
        uint32_t    _offset,                                                   \
        uint32_t    _size,                                                     \
        uint32_t    _capacity                                                  \
    )                                                                          \
    {                                                                          \
        return (type)d_kv_load_signed(_base, _offset, _size, _capacity);       \
    }

//   THE FRAMEWORK'S OWN WIDTHS, AND NOTHING ELSE. A consumer that wants a
// family over its own type invokes the generator in ITS header; instantiating
// the whole matrix here is what compiled unused function families into every
// translation unit that included an earlier version.
D_KV_DEFINE_SIGNED(int8_t,     d_kv_int8_at,   d_kv_int8_narrow)
D_KV_DEFINE_SIGNED(int16_t,    d_kv_int16_at,  d_kv_int16_narrow)
D_KV_DEFINE_SIGNED(int32_t,    d_kv_int32_at,  d_kv_int32_narrow)
D_KV_DEFINE_SIGNED(int64_t,    d_kv_int64_at,  d_kv_int64_narrow)
D_KV_DEFINE_UNSIGNED(uint8_t,  d_kv_uint8_at,  d_kv_uint8_narrow)
D_KV_DEFINE_UNSIGNED(uint16_t, d_kv_uint16_at, d_kv_uint16_narrow)
D_KV_DEFINE_UNSIGNED(uint32_t, d_kv_uint32_at, d_kv_uint32_narrow)
D_KV_DEFINE_UNSIGNED(uint64_t, d_kv_uint64_at, d_kv_uint64_narrow)

//   THERE IS NO LAYOUT ASSERTION SECTION, and its absence is the point. Every
// other module in this tier declares a struct and asserts its size and
// offsets, because a struct is a thing that can drift. This header declares
// none, so there is nothing to drift and nothing to pin. The assertions that
// used to sit here were about d_kv_field's own layout; they protected the
// descriptor, not the reads.


D_EXTERN_C_END


#endif  // DJINTERP_C_KV_