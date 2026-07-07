/******************************************************************************
* djinterp [container]                                              encode.hpp
*
*   The foundational half of the SERIALIZATION externalisation axis on the WRITE
* side: the ENCODER.  The formal model (containers.tex, Serialization) casts a
* container's value out of memory as a flat run of bits, the medium being the set
* B* of finite bit strings over {0,1}, grouped into bytes as convenient.  This
* header owns the medium, the LEAF element encoder enc_tau : tau -> B*, and the
* extension point through which a user type supplies its own enc_tau; the
* recursion that lifts enc_tau to a whole container lives in container_encode.hpp.
*
*   THE ELEMENT ENCODER enc_tau:
*   The model builds every container encoder "upon a serialiser for what it
* holds" - a leaf is written by enc_tau, a node by the encoder of its own type,
* recursively.  This header is that enc_tau for the built-in leaves (the integral
* family incl. bool and the character types, enumerations via their underlying
* type, and the 4- and 8-byte floating-point types) and the customization surface
* (a member `encode_into(sink)`) by which any other leaf becomes encodable.
*
*   UNIQUELY DECODABLE BY CONSTRUCTION:
*   Encodability asks only for a function to B*; decodability asks that the image
* be unambiguously parseable (containers.tex, Serialization).  Every leaf here is
* written at a FIXED WIDTH (sizeof for the integral/floating leaves), so field
* boundaries are recoverable without delimiters - the fixed-width strategy the
* model names.  Integers are written BIG-ENDIAN (most-significant byte first), a
* fixed byte order so the stream is portable across host endianness; a signed
* integer is written through its two's-complement unsigned pattern, a float
* through the same-width unsigned bit pattern, each reversed exactly on decode.
*
*   THE SINK:
*   enc_tau is expressed as `encode_into(sink, value)` writing into a BYTE SINK -
* any type exposing `push_back(byte)` (the default `byte_string` is one) - so a
* container encoder may concatenate component bits into one buffer without a
* temporary per leaf.  `encode(value)` is the convenience that allocates a fresh
* `byte_string`, encodes into it, and returns it - the total function enc_tau of
* the model, realised.
*
*   STAGE:
*   Encoding occurs at a stage (containers.tex, Lifetime): the model admits a
* compile-time-expressible value being encoded at compile time.  In practice the
* writers here are RUNTIME function templates - they fill a byte sink, and the
* default sink (a `byte_string`/std::vector) is not a constant-expression buffer
* before C++20 - so, like content_equality.hpp and byte_size.hpp, they are not
* marked constexpr; the stage distinction remains a property of the value, not of
* these functions.
*
*   PORTABILITY:
*   C++11 baseline.  Runtime function templates (they append to a byte sink),
* matching content_equality.hpp and byte_size.hpp.  The `_v` companions degrade
* with the language as elsewhere.
*
*
* path:      /inc/djinterp/core/container/serial/encode.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.06
******************************************************************************/

#ifndef DJINTERP_ENCODE_
#define DJINTERP_ENCODE_ 1

// std
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"            // clean_t, NS_*, feature macros
#include "../../meta/trait_detect.hpp"  // D_VOID_T, D_TYPE_TRAIT_VALUE_BOOL


NS_DJINTERP


// ===========================================================================
// I.   The medium (B*)
// ===========================================================================

// byte
//   type: one octet of the binary medium - the unit into which B* is grouped.
using byte = unsigned char;

// byte_string
//   type: a finite run of bytes - the concrete realisation of the model's B*,
// the flat structureless form a value is folded into.
using byte_string = std::vector<byte>;

// encode_length_type
//   type: the width-fixed unsigned integer a container encoder writes as its
// leading size (element count).  Exposed here since the count is itself encoded
// as a leaf; container_encode.hpp writes one before a container's components.
using encode_length_type = std::uint64_t;


// ===========================================================================
// II.  Byte-sink detection
// ===========================================================================

// has_push_back_byte
//   trait: true iff `_Sink` accepts a byte through `push_back` - the sole
// requirement this module places on a sink.  `byte_string` satisfies it, as does
// any growable byte buffer.
template<typename _Sink,
         typename = void>
struct has_push_back_byte : std::false_type
{};

template<typename _Sink>
struct has_push_back_byte<_Sink,
    D_VOID_T<decltype(std::declval<_Sink&>().push_back(std::declval<byte>()))>>
    : std::true_type
{};

D_TYPE_TRAIT_VALUE_BOOL(has_push_back_byte)


// ===========================================================================
// III. Member-encoder detection (the customization surface)
// ===========================================================================

// has_member_encode_into
//   trait: true iff a value of `_Type` can write itself into a `_Sink` through a
// member `encode_into(sink)` - the extension point by which a non-built-in leaf
// supplies its own enc_tau.  A member so found takes precedence over every
// built-in leaf writer below.
template<typename _Type,
         typename _Sink,
         typename = void>
struct has_member_encode_into : std::false_type
{};

template<typename _Type,
         typename _Sink>
struct has_member_encode_into<_Type, _Sink,
    D_VOID_T<decltype(std::declval<const clean_t<_Type>&>().encode_into(
        std::declval<_Sink&>()))>>
    : std::true_type
{};


// ===========================================================================
// IV.  Fixed-width byte writers (internal)
// ===========================================================================

NS_INTERNAL

    // put_uint_be
    //   helper: append the low `_width` bytes of `_value` to `_sink` in
    // BIG-ENDIAN order (most-significant first).  The fixed byte order is what
    // makes the field boundary recoverable and the stream host-independent.
    template<typename _Sink>
    void
    put_uint_be(
        _Sink&              _sink,
        encode_length_type  _value,
        std::size_t         _width
    )
    {
        // write most-significant byte first, so a wider decode reading the same
        // width recovers the value irrespective of the host's own byte order
        for (std::size_t _i = 0; _i < _width; ++_i)
        {
            const std::size_t _shift = ((_width - 1) - _i) * 8;
            _sink.push_back(
                static_cast<byte>((_value >> _shift) & static_cast<byte>(0xFF)));
        }

        return;
    }

    // encode_integral_leaf
    //   helper: write an integral (non-bool) value at its natural width through
    // its unsigned pattern.  static_cast to the same-width unsigned type is the
    // two's-complement bit pattern, reversed exactly on decode; only `sizeof`
    // bytes are emitted, so the widening to encode_length_type loses nothing.
    template<typename _Sink,
             typename _Integral>
    void
    encode_integral_leaf(
        _Sink&    _sink,
        _Integral _value
    )
    {
        using unsigned_type = typename std::make_unsigned<_Integral>::type;

        put_uint_be(_sink,
                    static_cast<encode_length_type>(
                        static_cast<unsigned_type>(_value)),
                    sizeof(_Integral));

        return;
    }

    // encode_floating_leaf
    //   helper: write a 4- or 8-byte floating value through its same-width
    // unsigned BIT PATTERN (obtained by std::memcpy, the well-defined type-pun),
    // then big-endian.  The pattern is reproduced exactly on decode.
    template<typename _Sink,
             typename _Float>
    void
    encode_floating_leaf(
        _Sink& _sink,
        _Float _value
    )
    {
        // 4-byte -> uint32 pattern, 8-byte -> uint64 pattern; the enclosing
        // overload admits only these two widths.
        if (sizeof(_Float) == 4)
        {
            std::uint32_t _bits = 0;
            std::memcpy(&_bits, &_value, 4);
            put_uint_be(_sink, static_cast<encode_length_type>(_bits), 4);
        }
        else
        {
            std::uint64_t _bits = 0;
            std::memcpy(&_bits, &_value, 8);
            put_uint_be(_sink, static_cast<encode_length_type>(_bits), 8);
        }

        return;
    }

NS_END  // internal


// ===========================================================================
// V.   enc_tau - the leaf element encoder
// ===========================================================================
//   Five mutually-exclusive overloads of `encode_into`: the member surface
// first (highest precedence, guarded away from the built-ins below), then the
// four built-in leaf families.  A leaf that is none of these has no enc_tau and
// is (correctly) not encodable - `is_leaf_encodable` reports as much.

// encode_into (member surface)
//   function: a value carrying its own `encode_into(sink)` writes itself.
template<typename _Sink,
         typename _Type,
         typename std::enable_if<
             has_member_encode_into<clean_t<_Type>, _Sink>::value,
             int>::type = 0>
void
encode_into(_Sink& _sink, const _Type& _value)
{
    _value.encode_into(_sink);

    return;
}

// encode_into (bool)
//   function: a boolean is one byte, 0 or 1.
template<typename _Sink,
         typename _Type,
         typename std::enable_if<
             ( std::is_same<clean_t<_Type>, bool>::value &&
               !has_member_encode_into<clean_t<_Type>, _Sink>::value ),
             int>::type = 0>
void
encode_into(_Sink& _sink, const _Type& _value)
{
    _sink.push_back(_value ? static_cast<byte>(1) : static_cast<byte>(0));

    return;
}

// encode_into (integral, non-bool)
//   function: an integral leaf at its natural width, big-endian, through its
// unsigned pattern.  The character types (char, wchar_t, char16_t, ...) are
// integral and travel this path.
template<typename _Sink,
         typename _Type,
         typename std::enable_if<
             ( std::is_integral<clean_t<_Type>>::value    &&
               !std::is_same<clean_t<_Type>, bool>::value &&
               ( sizeof(clean_t<_Type>) <= 8 )            &&
               !has_member_encode_into<clean_t<_Type>, _Sink>::value ),
             int>::type = 0>
void
encode_into(_Sink& _sink, const _Type& _value)
{
    internal::encode_integral_leaf(_sink, static_cast<clean_t<_Type>>(_value));

    return;
}

// encode_into (enum)
//   function: an enumeration through its underlying integral type, so a scoped
// or unscoped enum encodes exactly as the integer it names.
template<typename _Sink,
         typename _Type,
         typename std::enable_if<
             ( std::is_enum<clean_t<_Type>>::value &&
               !has_member_encode_into<clean_t<_Type>, _Sink>::value ),
             int>::type = 0>
void
encode_into(_Sink& _sink, const _Type& _value)
{
    using underlying_type =
        typename std::underlying_type<clean_t<_Type>>::type;

    internal::encode_integral_leaf(_sink,
        static_cast<underlying_type>(_value));

    return;
}

// encode_into (floating, 4- or 8-byte)
//   function: a float or double through its same-width unsigned bit pattern,
// big-endian.  A wider floating type (a 10-/12-/16-byte long double) has no
// fixed same-width unsigned target and so is not a built-in leaf; a user may
// give it a member `encode_into`.
template<typename _Sink,
         typename _Type,
         typename std::enable_if<
             ( std::is_floating_point<clean_t<_Type>>::value          &&
               ( sizeof(clean_t<_Type>) == 4 ||
                 sizeof(clean_t<_Type>) == 8 )                        &&
               !has_member_encode_into<clean_t<_Type>, _Sink>::value ),
             int>::type = 0>
void
encode_into(_Sink& _sink, const _Type& _value)
{
    internal::encode_floating_leaf(_sink, static_cast<clean_t<_Type>>(_value));

    return;
}


// ===========================================================================
// VI.  Convenience: enc_tau as a total function to B*
// ===========================================================================

// encode
//   function: the leaf encoder as the model's total map enc_tau : tau -> B* -
// allocates a fresh `byte_string`, writes `_value` into it, and returns it.
template<typename _Type>
byte_string
encode(const _Type& _value)
{
    byte_string _out;
    encode_into(_out, _value);

    return _out;
}


// ===========================================================================
// VII. Leaf-encodability trait
// ===========================================================================

NS_INTERNAL

    // encode_builtin_leaf_ok
    //   helper: whether `_Type` is a built-in leaf of a fixed encodable width.
    // Gated on arithmetic-or-enum so `sizeof` is only ever applied to an object
    // type - a non-object leaf (in particular void, the "no element" type of a
    // non-container) selects the primary and reports false without a sizeof.
    template<typename _Type,
             bool = ( std::is_integral<_Type>::value
                   || std::is_floating_point<_Type>::value
                   || std::is_enum<_Type>::value )>
    struct encode_builtin_leaf_ok : std::false_type
    {};

    template<typename _Type>
    struct encode_builtin_leaf_ok<_Type, true>
        : std::integral_constant<bool,
              ( std::is_enum<_Type>::value
             || std::is_same<_Type, bool>::value
             || ( std::is_integral<_Type>::value
               && ( sizeof(_Type) <= 8 ) )
             || ( std::is_floating_point<_Type>::value
               && ( sizeof(_Type) == 4 || sizeof(_Type) == 8 ) ) )>
    {};

NS_END  // internal

// is_leaf_encodable
//   trait: true iff `_Type` has a leaf enc_tau in this header - a member
// `encode_into`, or one of the built-in leaf families (bool, an <=8-byte
// integral, an enum, or a 4-/8-byte floating type).  The sink is taken as the
// default `byte_string` for the member probe.
template<typename _Type>
struct is_leaf_encodable
    : std::integral_constant<bool,
          ( has_member_encode_into<clean_t<_Type>, byte_string>::value
         || internal::encode_builtin_leaf_ok<clean_t<_Type>>::value )>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_leaf_encodable)


NS_END  // djinterp


#endif  // DJINTERP_ENCODE_
