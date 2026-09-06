/******************************************************************************
* djinterp [meta]                                                       kv.hpp
*
* The C++ face of kv.h: a field described once, expressible at compile time as
* a pointer-to-member and at run time as a `d_kv_field`, with one lowering
* between them.
*
*
* WHY THERE ARE TWO DESCRIPTORS AND NOT ONE
* =========================================
*   kv.h describes a field as {offset, size, type, flags} because C has no
* other way to say it. C++ does: a pointer-to-member names the same field
* exactly, and `record.*pointer` reads it with the member's own type, its own
* alignment and no bounds check -- because there is nothing left to check.
* Collapsing that into the runtime triple would throw the type system away for
* the sake of one spelling.
*
*   SO BOTH EXIST AND ONE LOWERS INTO THE OTHER. `member_field` is the
* compile-time form and costs nothing; `kv_field` is the runtime form and is
* the C struct. `lower()` turns the first into the second, AND IS AVAILABLE
* ONLY WHEN THE OFFSET IS KNOWN -- which is the interesting part.
*
*
* THE OFFSET IS THE HARD PART, AND SFINAE IS WHY
* ==============================================
*   A pointer-to-member cannot yield its offset in a constant expression.
* The familiar trick -- taking the address of a member of a null object and
* subtracting -- is undefined however universally it works, and no standard
* facility replaces it. `offsetof` gets the answer, but it needs the member's
* NAME and so can only be written by a macro at the declaration site.
*
*   Rather than pretend, `member_field` carries the offset as an optional
* non-type parameter defaulted to `kv_offset_unknown`, and `lower()` is
* enable_if'd on it being known. A field built by hand from a
* pointer-to-member is a perfectly good compile-time field and simply cannot
* cross to C; a field built by D_KV_MEMBER_FIELD supplies offsetof and can.
* THE COMPILER REPORTS THE DIFFERENCE INSTEAD OF THE PROGRAMMER REMEMBERING IT.
*
*
* WHAT THE TRAITS ARE FOR
* =======================
*   `is_kv_addressable` is the precondition every lowering rests on: a type
* whose layout the C side may compute offsets into must be standard-layout,
* and one whose bytes may be copied must be trivially copyable. Both are
* asserted where lowering happens rather than assumed, so a record that
* acquired a virtual function fails at the boundary with its own name in the
* message.
*
*   `is_kv_pair` and its accessors are the structural contract kv_pair.hpp
* already satisfies -- a `.key` and a `.value` -- detected rather than
* inherited, so a caller's own row type participates without deriving from
* anything.
*
*
* path:      /inc/djinterp/core/meta/kv.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.05
******************************************************************************/

#ifndef DJINTERP_META_KV_
#define DJINTERP_META_KV_ 1

// std
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../../c/meta/kv.h"


NS_DJINTERP


// I.     detection helpers

NS_INTERNAL

    // kv_void_helper
    //   trait: maps any pack of types to void, for the detection idiom. Named
    // locally so this header stands alone at the C++11 floor, where
    // std::void_t does not exist.
    template<typename... _Types>
    struct kv_void_helper
    {
        using type = void;
    };

    // kv_key_member_t
    //   type: the type of a `.key` member, if there is one.
    //   AN ALIAS TEMPLATE, NOT A CLASS TEMPLATE, and the distinction is the
    // whole detection idiom. A member typedef inside a class template is
    // formed in the class's BODY, so `int` having no `.key` is a hard error at
    // instantiation and never reaches the partial specialization that was
    // supposed to reject it. An alias substitutes in the IMMEDIATE CONTEXT of
    // argument deduction, where a failure is a substitution failure and the
    // primary template wins as intended.
    template<typename _Type>
    using kv_key_member_t = decltype(std::declval<const _Type&>().key);

    // kv_value_member_t
    //   type: the type of a `.value` member, if there is one.
    template<typename _Type>
    using kv_value_member_t = decltype(std::declval<const _Type&>().value);

NS_END  // internal

// kv_void_t
//   type: void for any well-formed pack; the substitution vehicle every trait
// below is written in terms of.
template<typename... _Types>
using kv_void_t = typename internal::kv_void_helper<_Types...>::type;


// II.    structural traits

// has_kv_key
//   trait: whether `_Type` has a readable `.key` member (failure case).
template<typename _Type,
         typename _Enable = void>
struct has_kv_key : std::false_type
{};

// has_kv_key
//   trait: partial specialization for a type whose `.key` is well-formed.
template<typename _Type>
struct has_kv_key<_Type,
                  kv_void_t<internal::kv_key_member_t<_Type>>>
    : std::true_type
{};

// has_kv_value
//   trait: whether `_Type` has a readable `.value` member (failure case).
template<typename _Type,
         typename _Enable = void>
struct has_kv_value : std::false_type
{};

// has_kv_value
//   trait: partial specialization for a type whose `.value` is well-formed.
template<typename _Type>
struct has_kv_value<_Type,
                    kv_void_t<internal::kv_value_member_t<_Type>>>
    : std::true_type
{};

// is_kv_pair
//   trait: whether `_Type` satisfies the key/value structural contract. This
// is the shape kv_pair.hpp has, DETECTED RATHER THAN INHERITED, so a caller's
// own row type participates without deriving from anything.
template<typename _Type>
struct is_kv_pair
{
    static D_CONSTEXPR bool value =
        ( has_kv_key<_Type>::value &&
          has_kv_value<_Type>::value );
};

// kv_key_t
//   type: the decayed type of `_Type`'s key member.
template<typename _Type>
using kv_key_t =
    typename std::decay<internal::kv_key_member_t<_Type>>::type;

// kv_value_t
//   type: the decayed type of `_Type`'s value member.
template<typename _Type>
using kv_value_t =
    typename std::decay<internal::kv_value_member_t<_Type>>::type;

// is_kv_addressable
//   trait: whether the C side may compute offsets into `_Type` and copy its
// bytes. THE PRECONDITION EVERY LOWERING RESTS ON: offsetof is defined only
// for standard-layout types, and a byte-wise copy is defined only for
// trivially copyable ones. Asserted where lowering happens rather than
// assumed, so a record that acquired a virtual function fails at the boundary
// with its own name in the message.
template<typename _Type>
struct is_kv_addressable
{
    static D_CONSTEXPR bool value =
        ( std::is_standard_layout<_Type>::value &&
          std::is_trivially_copyable<_Type>::value );
};

// is_kv_scalar
//   trait: whether a member may be carried through the C widened load path at
// all -- an integral or enumeration type of at most eight bytes. A float, a
// pointer and a struct are each addressable and none of them is this.
template<typename _Type>
struct is_kv_scalar
{
    static D_CONSTEXPR bool value =
        ( ( std::is_integral<_Type>::value ||
            std::is_enum<_Type>::value )   &&
          (sizeof(_Type) <= 8u) );
};


// III.   the compile-time field

// kv_offset_unknown
//   constant: the offset of a field built from a pointer-to-member alone. A
// pointer-to-member cannot yield its offset in a constant expression, and the
// null-object subtraction that appears to is undefined -- so the absence is
// named rather than guessed, and `lower()` is gated on it.
D_CONSTEXPR_INLINE_VAR std::size_t kv_offset_unknown =
    static_cast<std::size_t>(-1);

// member_field
//   trait: a field named by a pointer-to-member. Costs nothing: `get()` is
// `record.*pointer`, which the compiler resolves to the member's own type at
// its own alignment with no bounds check, because the type system already did
// the checking a bounds check would repeat.
template<typename           _Record,
         typename           _Member,
         _Member _Record::* _Pointer,
         std::size_t        _Offset = kv_offset_unknown>
struct member_field
{
    using record_type = _Record;
    using member_type = _Member;

    static D_CONSTEXPR _Member _Record::* pointer    = _Pointer;
    static D_CONSTEXPR std::size_t        offset     = _Offset;
    static D_CONSTEXPR std::size_t        size       = sizeof(_Member);
    static D_CONSTEXPR bool               has_offset =
        (_Offset != kv_offset_unknown);
    static D_CONSTEXPR bool               is_signed  =
        std::is_signed<_Member>::value;
    static D_CONSTEXPR bool               is_scalar  =
        is_kv_scalar<_Member>::value;

    // get
    //   function: the field of a record, by reference and by its own type.
    static D_CONSTEXPR const _Member&
    get(
        const _Record& _record
    ) D_NOEXCEPT
    {
        return _record.*_Pointer;
    }

    // set
    //   function: assign the field of a record.
    static void
    set(
        _Record&       _record,
        const _Member& _value
    ) D_NOEXCEPT
    {
        _record.*_Pointer = _value;

        return;
    }
};

// D_KV_MEMBER_TYPE
//   macro: the declared type of a member. The sizeof/decltype operand is
// unevaluated, so the null pointer is never dereferenced.
#define D_KV_MEMBER_TYPE(record, member)                                       \
    decltype(((record*)0)->member)

// D_KV_MEMBER_FIELD
//   macro: a member_field WITH ITS OFFSET, which is the only way to get one.
// offsetof needs the member's name and so cannot be written by a template;
// this is the declaration-site spelling that supplies it, and a field built
// this way is the one that can lower to C.
#define D_KV_MEMBER_FIELD(record, member)                                      \
    ::djinterp::member_field<record,                                           \
                             D_KV_MEMBER_TYPE(record, member),                 \
                             &record::member,                                  \
                             offsetof(record, member)>

// D_KV_MEMBER_FIELD_NO_OFFSET
//   macro: the same without offsetof, for a record that is not standard-layout
// and therefore has no offsets to speak of. Usable everywhere in C++ and
// nowhere across the boundary, which `lower()` enforces rather than documents.
#define D_KV_MEMBER_FIELD_NO_OFFSET(record, member)                            \
    ::djinterp::member_field<record,                                           \
                             D_KV_MEMBER_TYPE(record, member),                 \
                             &record::member>


// IV.    the runtime field

// kv_field
//   class: the runtime descriptor. PRIVATELY INHERITS THE C STRUCT AND ADDS NO
// DATA MEMBER, so its layout is the C struct's, `sizeof` is unchanged and the
// address of one is the address of the other -- the same arrangement
// AGENT_README records for `option`.
class kv_field : private ::d_kv_field
{
public:
    // kv_field
    //   function: the empty descriptor -- width zero, which every C operation
    // reads as "there is nothing there".
    D_CONSTEXPR kv_field() D_NOEXCEPT
        : ::d_kv_field()
    {}

    // kv_field
    //   function: a descriptor from loose parts.
    kv_field(
        std::uint32_t  _offset,
        std::uint32_t  _size,
        ::d_type_info16 _type  = static_cast< ::d_type_info16>(0),
        std::uint16_t  _flags = D_KV_FLAG_NONE
    ) D_NOEXCEPT
    {
        static_cast< ::d_kv_field&>(*this) =
            ::d_kv_field_make(_offset, _size, _type, _flags);
    }

    std::uint32_t offset() const D_NOEXCEPT
    {
        return ::d_kv_field::offset;
    }

    std::uint32_t size() const D_NOEXCEPT
    {
        return ::d_kv_field::size;
    }

    ::d_type_info16 type() const D_NOEXCEPT
    {
        return ::d_kv_field::type;
    }

    std::uint16_t flags() const D_NOEXCEPT
    {
        return ::d_kv_field::flags;
    }

    bool empty() const D_NOEXCEPT
    {
        return ::d_kv_field_is_empty(this);
    }

    bool is_signed() const D_NOEXCEPT
    {
        return ::d_kv_field_is_signed(this);
    }

    bool fits(std::uint32_t _capacity) const D_NOEXCEPT
    {
        return ::d_kv_field_fits(this, _capacity);
    }

    bool disjoint(const kv_field& _other) const D_NOEXCEPT
    {
        return ::d_kv_disjoint(this, &_other);
    }

    // c_ref
    //   function: the C struct, for handing to the C entry points unchanged.
    // No conversion happens: this IS the base subobject.
    const ::d_kv_field& c_ref() const D_NOEXCEPT
    {
        return *this;
    }

    ::d_kv_field& c_ref() D_NOEXCEPT
    {
        return *this;
    }
};


// V.     lowering

// lower_field
//   function: the compile-time field as a runtime descriptor. ENABLED ONLY
// WHEN THE OFFSET IS KNOWN -- a field built from a bare pointer-to-member has
// no offset to give, and this is the substitution failure that says so at the
// call site instead of producing a descriptor pointing at byte zero.
template<typename _Field>
D_INLINE
typename std::enable_if<_Field::has_offset, kv_field>::type
lower_field() D_NOEXCEPT
{
    static_assert(is_kv_addressable<typename _Field::record_type>::value,
                  "kv: a record whose fields cross to C must be "
                  "standard-layout and trivially copyable.");

    return kv_field(static_cast<std::uint32_t>(_Field::offset),
                    static_cast<std::uint32_t>(_Field::size),
                    static_cast< ::d_type_info16>(0),
                    _Field::is_signed ? D_KV_FLAG_SIGNED : D_KV_FLAG_NONE);
}


// VI.    runtime typed access
//   These exist for the case the compile-time form cannot serve: bytes whose
// record type is not visible where the read happens -- a mapped file, a row
// reached through a `void*`, a descriptor read from a table. They are the C
// primitives with the widening and narrowing spelled by the type system.

// kv_load
//   function: read a scalar through a runtime descriptor, routed to the signed
// or unsigned C load by the type rather than by the caller.
template<typename _Type>
D_INLINE
typename std::enable_if<is_kv_scalar<_Type>::value, _Type>::type
kv_load(
    const void*     _base,
    const kv_field& _field,
    std::uint32_t   _capacity
) D_NOEXCEPT
{
    // a signed destination needs the sign-extending load, whatever the
    // descriptor's own flag says: the destination is the stronger statement
    if (std::is_signed<_Type>::value)
    {
        return static_cast<_Type>(::d_kv_load_signed(_base,
                                                     _field.offset(),
                                                     _field.size(),
                                                     _capacity));
    }

    return static_cast<_Type>(::d_kv_load_unsigned(_base,
                                                   _field.offset(),
                                                   _field.size(),
                                                   _capacity));
}

// kv_store
//   function: write a scalar through a runtime descriptor. False when the
// value does not fit the slot, which is a refusal and not a truncation.
template<typename _Type>
D_INLINE
typename std::enable_if<is_kv_scalar<_Type>::value, bool>::type
kv_store(
    void*           _base,
    const kv_field& _field,
    std::uint32_t   _capacity,
    _Type           _value
) D_NOEXCEPT
{
    // a signed source needs the range test against the slot's signed bounds
    if (std::is_signed<_Type>::value)
    {
        return ::d_kv_store_signed(_base,
                                   _field.offset(),
                                   _field.size(),
                                   _capacity,
                                   static_cast<std::int64_t>(_value));
    }

    return ::d_kv_store_unsigned(_base,
                                 _field.offset(),
                                 _field.size(),
                                 _capacity,
                                 static_cast<std::uint64_t>(_value));
}

// kv_read
//   function: copy an opaque field out into a typed destination. The width
// contract is the C one -- the slot and the destination must agree exactly --
// and the trivially-copyable requirement is asserted rather than hoped for.
template<typename _Type>
D_INLINE bool
kv_read(
    const void*     _base,
    const kv_field& _field,
    std::uint32_t   _capacity,
    _Type&          _out
) D_NOEXCEPT
{
    static_assert(std::is_trivially_copyable<_Type>::value,
                  "kv: only a trivially copyable type may be read "
                  "byte-wise.");

    return ::d_kv_field_read(&_field.c_ref(),
                             _base,
                             _capacity,
                             &_out,
                             sizeof(_Type));
}


// VII.   layout assertions
//   The Layout law: one declaration, asserted in both dialects. kv.h asserts
// the C struct; what is asserted here is that the WRAPPER did not disturb it,
// which is the claim the private-inheritance arrangement makes.

static_assert((sizeof(kv_field) == sizeof(::d_kv_field)),
              "kv_field must add no storage to d_kv_field.");
static_assert(std::is_standard_layout<kv_field>::value,
              "kv_field must be standard-layout for the base to sit at "
              "offset zero.");
static_assert(std::is_trivially_copyable<kv_field>::value,
              "kv_field must stay trivially copyable, as d_kv_field is.");
static_assert(is_kv_addressable< ::d_kv_field>::value,
              "d_kv_field must satisfy the precondition it describes.");


NS_END  // djinterp


#endif  // DJINTERP_META_KV_
