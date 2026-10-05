/*******************************************************************************
* djinterp [core]                                                         kv.hpp
*
* The C++ face of kv.h: a field named by a pointer-to-member, read with its own
* type at no cost, and passed across to C as the two constants it already is.
*
*
* THE LOWERING IS GONE BECAUSE THERE IS NOTHING LEFT TO LOWER
* ===========================================================
*   An earlier form of this header carried a `kv_field` class -- private
* inheritance from the C struct, named accessors, four layout assertions
* proving it added no storage -- and a `lower_field<Field>()` that turned a
* compile-time descriptor into a runtime one.
*
*   ALL OF THAT EXISTED ONLY BECAUSE d_kv_field EXISTED. With the C side taking
* an offset and a width directly, `Field::offset` and `Field::size` ARE the
* arguments; there is no conversion, no wrapper, and no layout to assert. The
* bridge did not get simpler -- it stopped being a bridge and became a pair of
* integral constants the compiler already had.
*
*
* THE SFINAE GATE SURVIVES, ATTACHED TO THE OPERATION
* ===================================================
*   It was worth keeping and it moved. A pointer-to-member CANNOT YIELD ITS
* OFFSET in a constant expression: the familiar null-object subtraction is
* undefined however universally it works, and offsetof needs the member's NAME,
* so only a macro at the declaration site can supply it.
*
*   `member_field` therefore carries the offset as an optional non-type
* parameter defaulting to `kv_offset_unknown`, and the functions that hand a
* field to C -- section V -- are enable_if'd on it being known. A field built
* by hand from a bare pointer-to-member is a perfectly good compile-time field
* and simply cannot address bytes through C; a field built by
* D_KV_MEMBER_FIELD supplies offsetof and can. THE COMPILER REPORTS THE
* DIFFERENCE INSTEAD OF THE PROGRAMMER REMEMBERING IT.
*
*   Previously that gate sat on `lower_field()`. It now sits on
* `kv_load_field()` and its siblings, which is a better place: the constraint
* is not "can this become a descriptor" but "can this address bytes at all",
* and the second is the question a call site is actually asking.
*
*
* WHY BOTH FORMS STILL EXIST
* ==========================
*   `member_field::get()` is `record.*pointer` -- the member's own type, its
* own alignment, no bounds check, because the type system already did the
* checking a bounds check would repeat. Nothing in section V should be reached
* for when the record type is visible.
*
*   Section V is for what the compile-time form cannot serve: bytes whose
* record type is not visible where the read happens -- a mapped file, a row
* reached through a void*, a width read from a table at run time.
*
*
* path:      /inc/djinterp/core/meta/kv.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.06
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_META_KV_HPP
#define DJINTERP_META_KV_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (README
// rule 5). The owner's ruling: compile at every level first; port to C++98
// only where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../../c/meta/kv.h"
// re_std
#include "../../../re_std/cstdint/cstdint.hpp"  // re_std::int64_t, uint32_t,
                                                // uint64_t


NS_DJINTERP


// I.     detection helpers

NS_INTERNAL

    // kv_void_helper
    //   trait: maps any pack of types to void, for the detection idiom. Named
    // locally so this header stands alone at the C++11 floor, where
    // std::void_t does not exist.
    template<typename... Types>
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
    template<typename Type>
    using kv_key_member_t = decltype(std::declval<const Type&>().key);

    // kv_value_member_t
    //   type: the type of a `.value` member, if there is one.
    template<typename Type>
    using kv_value_member_t = decltype(std::declval<const Type&>().value);

NS_END  // internal

// kv_void_t
//   type: void for any well-formed pack; the substitution vehicle every trait
// below is written in terms of.
template<typename... Types>
using kv_void_t = typename internal::kv_void_helper<Types...>::type;


// II.    structural traits

// has_kv_key
//   trait: whether `Type` has a readable `.key` member (failure case).
template<typename Type,
         typename Enable = void>
struct has_kv_key : std::false_type
{};

// has_kv_key
//   trait: partial specialization for a type whose `.key` is well-formed.
template<typename Type>
struct has_kv_key<Type,
                  kv_void_t<internal::kv_key_member_t<Type>>>
    : std::true_type
{};

// has_kv_value
//   trait: whether `Type` has a readable `.value` member (failure case).
template<typename Type,
         typename Enable = void>
struct has_kv_value : std::false_type
{};

// has_kv_value
//   trait: partial specialization for a type whose `.value` is well-formed.
template<typename Type>
struct has_kv_value<Type,
                    kv_void_t<internal::kv_value_member_t<Type>>>
    : std::true_type
{};

// is_kv_pair
//   trait: whether `Type` satisfies the key/value structural contract. This
// is the shape kv_pair.hpp has, DETECTED RATHER THAN INHERITED, so a caller's
// own row type participates without deriving from anything.
template<typename Type>
struct is_kv_pair
{
    static D_CONSTEXPR bool value =
        ( has_kv_key<Type>::value &&
          has_kv_value<Type>::value );
};

// kv_key_t
//   type: the decayed type of `Type`'s key member.
template<typename Type>
using kv_key_t =
    typename std::decay<internal::kv_key_member_t<Type>>::type;

// kv_value_t
//   type: the decayed type of `Type`'s value member.
template<typename Type>
using kv_value_t =
    typename std::decay<internal::kv_value_member_t<Type>>::type;

// is_kv_addressable
//   trait: whether the C side may compute offsets into `Type` and copy its
// bytes. THE PRECONDITION EVERY CROSSING RESTS ON: offsetof is defined only
// for standard-layout types, and a byte-wise copy is defined only for
// trivially copyable ones. Asserted where the crossing happens rather than
// assumed, so a record that acquired a virtual function fails at the boundary
// with its own name in the message.
template<typename Type>
struct is_kv_addressable
{
    static D_CONSTEXPR bool value =
        ( std::is_standard_layout<Type>::value &&
          std::is_trivially_copyable<Type>::value );
};

// is_kv_scalar
//   trait: whether a member may be carried through the C widened load path at
// all -- an integral or enumeration type of at most eight bytes. A float, a
// pointer and a struct are each addressable and none of them is this.
template<typename Type>
struct is_kv_scalar
{
    static D_CONSTEXPR bool value =
        ( ( std::is_integral<Type>::value ||
            std::is_enum<Type>::value )   &&
          (sizeof(Type) <= 8u) );
};


// III.   the compile-time field

// kv_offset_unknown
//   constant: the offset of a field built from a pointer-to-member alone. A
// pointer-to-member cannot yield its offset in a constant expression, and the
// null-object subtraction that appears to is undefined -- so the absence is
// named rather than guessed, and section V is gated on it.
D_CONSTEXPR_INLINE_VAR std::size_t kv_offset_unknown =
    static_cast<std::size_t>(-1);

// member_field
//   trait: a field named by a pointer-to-member. Costs nothing: `get()` is
// `record.*pointer`, which the compiler resolves to the member's own type at
// its own alignment with no bounds check, because the type system already did
// the checking a bounds check would repeat.
//   `offset` AND `size` ARE THE C ARGUMENTS, not the ingredients of a
// descriptor. Nothing converts them; a call into kv.h passes them straight.
template<typename           Record,
         typename           Member,
         Member Record::* Pointer,
         std::size_t        Offset = kv_offset_unknown>
struct member_field
{
    using record_type = Record;
    using member_type = Member;

    static D_CONSTEXPR Member Record::* pointer    = Pointer;
    static D_CONSTEXPR std::size_t        offset     = Offset;
    static D_CONSTEXPR std::size_t        size       = sizeof(Member);
    static D_CONSTEXPR bool               has_offset =
        (Offset != kv_offset_unknown);
    static D_CONSTEXPR bool               is_signed  =
        std::is_signed<Member>::value;
    static D_CONSTEXPR bool               is_scalar  =
        is_kv_scalar<Member>::value;

    // get
    //   function: the field of a record, by reference and by its own type.
    static D_CONSTEXPR const Member&
    get(
        const Record& _record
    ) D_NOEXCEPT
    {
        return _record.*Pointer;
    }

    // set
    //   function: assign the field of a record.
    static void
    set(
        Record&       _record,
        const Member& _value
    ) D_NOEXCEPT
    {
        _record.*Pointer = _value;

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
// this way is the one that can address bytes through C.
#define D_KV_MEMBER_FIELD(record, member)                                      \
    ::djinterp::member_field<record,                                           \
                             D_KV_MEMBER_TYPE(record, member),                 \
                             &record::member,                                  \
                             offsetof(record, member)>

// D_KV_MEMBER_FIELD_NO_OFFSET
//   macro: the same without offsetof, for a record that is not standard-layout
// and therefore has no offsets to speak of. Usable everywhere in C++ and
// nowhere across the boundary, which section V enforces rather than documents.
#define D_KV_MEMBER_FIELD_NO_OFFSET(record, member)                            \
    ::djinterp::member_field<record,                                           \
                             D_KV_MEMBER_TYPE(record, member),                 \
                             &record::member>


// IV.    runtime typed access
//   The C primitives with the widening and narrowing spelled by the type
// system. For bytes whose record type is not visible where the read happens.

// kv_load
//   function: read a scalar at an offset and width, routed to the signed or
// unsigned C load by the DESTINATION type.
//   THE DESTINATION DECIDES, and that is the improvement dropping the
// descriptor's SIGNED flag bought: the branch used to live in a bit on a
// struct, read by a dispatching load that could not see what the value was
// being read into.
template<typename Type>
D_INLINE
typename std::enable_if<is_kv_scalar<Type>::value, Type>::type
kv_load(
    const void*      _base,
    re_std::uint32_t _offset,
    re_std::uint32_t _size,
    re_std::uint32_t _capacity
) D_NOEXCEPT
{
    // a signed destination needs the sign-extending load
    if (std::is_signed<Type>::value)
    {
        return static_cast<Type>(::d_kv_load_signed(_base,
                                                     _offset,
                                                     _size,
                                                     _capacity));
    }

    return static_cast<Type>(::d_kv_load_unsigned(_base,
                                                   _offset,
                                                   _size,
                                                   _capacity));
}

// kv_store
//   function: write a scalar at an offset and width. False when the value does
// not fit the slot, which is a refusal and not a truncation.
template<typename Type>
D_INLINE
typename std::enable_if<is_kv_scalar<Type>::value, bool>::type
kv_store(
    void*            _base,
    re_std::uint32_t _offset,
    re_std::uint32_t _size,
    re_std::uint32_t _capacity,
    Type             _value
) D_NOEXCEPT
{
    // a signed source needs the range test against the slot's signed bounds
    if (std::is_signed<Type>::value)
    {
        return ::d_kv_store_signed(_base,
                                   _offset,
                                   _size,
                                   _capacity,
                                   static_cast<re_std::int64_t>(_value));
    }

    return ::d_kv_store_unsigned(_base,
                                 _offset,
                                 _size,
                                 _capacity,
                                 static_cast<re_std::uint64_t>(_value));
}

// kv_read
//   function: copy an opaque span out into a typed destination. The width
// contract is the C one -- the slot and the destination must agree exactly --
// and the trivially-copyable requirement is asserted rather than hoped for.
template<typename Type>
D_INLINE bool
kv_read(
    const void*      _base,
    re_std::uint32_t _offset,
    re_std::uint32_t _capacity,
    Type&            _out
) D_NOEXCEPT
{
    static_assert(std::is_trivially_copyable<Type>::value,
                  "kv: only a trivially copyable type may be read "
                  "byte-wise.");

    return ::d_kv_read(_base,
                       _offset,
                       static_cast<re_std::uint32_t>(sizeof(Type)),
                       _capacity,
                       &_out,
                       sizeof(Type));
}


// V.     crossing to C
//   The compile-time field's constants handed to the C primitives. THESE ARE
// THE GATED ENTRY POINTS: each is enabled only when the field carries an
// offset, because a field built from a bare pointer-to-member has none to
// give and passing zero would silently address the wrong bytes.

// kv_load_field
//   function: read a field through C, at the member's own offset and width.
template<typename Field>
D_INLINE
typename std::enable_if<
    ( Field::has_offset && Field::is_scalar ),
    typename Field::member_type>::type
kv_load_field(
    const void*      _base,
    re_std::uint32_t _capacity
) D_NOEXCEPT
{
    static_assert(is_kv_addressable<typename Field::record_type>::value,
                  "kv: a record whose fields cross to C must be "
                  "standard-layout and trivially copyable.");

    return kv_load<typename Field::member_type>(
        _base,
        static_cast<re_std::uint32_t>(Field::offset),
        static_cast<re_std::uint32_t>(Field::size),
        _capacity);
}

// kv_store_field
//   function: write a field through C, under the same gate.
template<typename Field>
D_INLINE
typename std::enable_if<
    ( Field::has_offset && Field::is_scalar ),
    bool>::type
kv_store_field(
    void*                             _base,
    re_std::uint32_t                  _capacity,
    typename Field::member_type       _value
) D_NOEXCEPT
{
    static_assert(is_kv_addressable<typename Field::record_type>::value,
                  "kv: a record whose fields cross to C must be "
                  "standard-layout and trivially copyable.");

    return kv_store<typename Field::member_type>(
        _base,
        static_cast<re_std::uint32_t>(Field::offset),
        static_cast<re_std::uint32_t>(Field::size),
        _capacity,
        _value);
}

// kv_field_fits
//   function: whether a field lies inside a capacity, through C's widened
// bounds check rather than a repeat of it here.
template<typename Field>
D_INLINE
typename std::enable_if<Field::has_offset, bool>::type
kv_field_fits(
    re_std::uint32_t _capacity
) D_NOEXCEPT
{
    return ::d_kv_fits(static_cast<re_std::uint32_t>(Field::offset),
                       static_cast<re_std::uint32_t>(Field::size),
                       _capacity);
}

// kv_fields_disjoint
//   function: whether two fields of a record share no byte. This is
// registry.hpp's _KeyDisjoint computed rather than declared.
template<typename Lhs,
         typename Rhs>
D_CONSTEXPR
typename std::enable_if<( Lhs::has_offset && Rhs::has_offset ), bool>::type
kv_fields_disjoint() D_NOEXCEPT
{
    return ::d_kv_disjoint(static_cast<re_std::uint32_t>(Lhs::offset),
                           static_cast<re_std::uint32_t>(Lhs::size),
                           static_cast<re_std::uint32_t>(Rhs::offset),
                           static_cast<re_std::uint32_t>(Rhs::size));
}


//   THERE ARE NO LAYOUT ASSERTIONS HERE EITHER, and for the same reason as in
// kv.h. The four that used to sit at the bottom of this file all asserted that
// the `kv_field` wrapper had not disturbed the C struct it inherited from --
// that its sizeof matched, that it stayed standard-layout, that it stayed
// trivially copyable. With no wrapper and no struct, there is no such claim to
// make and nothing that could quietly stop being true.


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_META_KV_HPP
