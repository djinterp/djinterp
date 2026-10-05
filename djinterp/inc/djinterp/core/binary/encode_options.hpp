/*******************************************************************************
* djinterp [core]                                             encode_options.hpp
*
*   The CONFIGURABLE face of the leaf encoder.  encode.hpp fixes one encoding
* (big-endian scalars, an 8-byte length prefix) - the sensible default.  This
* header parameterises the two degrees of freedom the formal model leaves open
* (containers.tex, Serialization) and drives them from compile-time options:
*
*     1. BYTE ORDER.  The medium B* is a run of bits "grouped into bytes as
*        convenient" - the grouping order is latitude, not law.  serial_endian
*        selects big- or little-endian for every multi-byte scalar and length.
*
*     2. LENGTH ENCODING.  Unique decodability may rest "by fixed widths, by
*        length prefixes ..., or by a self-delimiting (prefix-free) element
*        code"; and "a bounded type admits a fixed-width count ... an unbounded
*        one a variable length."  serial_length selects the width of the leading
*        size - a fixed 2/4/8-byte field, or a variable-length (LEB128) varint.
*
*   A third axis, serial_fidelity, names the LEVEL L at which a round trip is
* faithful (containers.tex, "Fidelity").  It is carried and documented rather
* than driving a distinct byte layout here: a native-faithful encoding is a
* fortiori faithful at every coarser level (=_str => =_seq => =_bag => =_set),
* so declaring a coarser target is always sound; it merely forgoes the compaction
* an order-blind encoding could take.  The knob is exposed so a caller may state
* the intended level, and so a later canonicalising encoder can honour it.
*
*   This header owns the serial PARAMETER enums and the parameterised LEAF
* encoder enc_tau<endian>.  The recursion that lifts it to a container, the
* option vocabulary (option<> keys), and the fluent / procedural front ends live
* in container_options.hpp.  The trait and dispatch machinery (the sink probe,
* the member surface, is_leaf_encodable) is reused verbatim from encode.hpp - only
* the byte-order and length logic is re-parameterised here.
*
*   PORTABILITY:
*   C++11 baseline, as encode.hpp.  Nothing here depends on option<> (that layer,
* and its C++17 requirement, is confined to container_options.hpp).
*
*
* path:      /inc/djinterp/core/binary/encode_options.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    serial parameter enums      (serial_endian / serial_length / serial_fidelity)
      -----------------------------------------------------------------------------

II.   parameterised byte writers  (put_uint<E> / put_length<L,E>)
      -----------------------------------------------------------

III.  enc_tau<E>                  (parameterised leaf encoder)
      --------------------------------------------------------

IV.   convenience                 (encode_leaf<E>)
      --------------------------------------------
*/

#ifndef DJINTERP_BINARY_ENCODE_OPTIONS_HPP
#define DJINTERP_BINARY_ENCODE_OPTIONS_HPP 1

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
#include "../../djinterp.hpp"            // NS_*, feature macros
#include "../meta/type_utility.hpp"      // clean_t
#include "./encode.hpp"                  // medium, sink probe, member surface, traits
// re_std
#include "../../../re_std/cstdint/cstdint.hpp"  // re_std::uint32_t, uint64_t


NS_DJINTERP


// ===========================================================================
// I.   Serial parameter enums
// ===========================================================================

// serial_endian
//   axis: byte order for multi-byte scalars and length fields.  The medium's
// grouping of bits into bytes is "as convenient" (containers.tex,
// Serialization), so either order is a faithful realisation; the choice fixes
// portability, and both ends of a round trip must agree.
enum class serial_endian
{
    big,        // most-significant byte first (network order) - encode.hpp's default
    little      // least-significant byte first (native x86/ARM order)
};

// serial_length
//   axis: how a container's leading size (its element count) is encoded.  A
// bounded container admits a fixed-width count; an unbounded one a variable
// length (containers.tex, "What the encoding must carry").  The fixed widths
// each assert the count fits; varint is self-delimiting and unbounded.
enum class serial_length
{
    u16,        // fixed 2-byte count  (count < 2^16)
    u32,        // fixed 4-byte count  (count < 2^32)
    u64,        // fixed 8-byte count  (count < 2^64) - encode.hpp's default
    varint      // LEB128 variable-length count (unbounded, self-delimiting)
};

// serial_fidelity
//   axis: the level L at which the round trip is faithful (containers.tex,
// "Fidelity").  native keeps everything the container's own level distinguishes;
// the coarser levels name a weaker target (order-, then multiplicity-blind).
// See the header note: carried and documented; native-faithful subsumes all.
enum class serial_fidelity
{
    native,     // the container's own level - lossless
    sequence,   // =_seq : element order, not the finer string level
    bag,        // =_bag : multiplicity, not order
    set         // =_set : membership only
};


// default_endian / default_length / default_fidelity
//   values: the positions encode.hpp / container_encode.hpp realise, so the
// options layer and the unconfigured foundational layer agree by construction.
D_STATIC_CONSTEXPR serial_endian   default_endian   = serial_endian::big;
D_STATIC_CONSTEXPR serial_length   default_length   = serial_length::u64;
D_STATIC_CONSTEXPR serial_fidelity default_fidelity = serial_fidelity::native;


// ===========================================================================
// II.  Parameterised byte writers
// ===========================================================================

NS_INTERNAL

    // put_uint
    //   helper: append the low `_width` bytes of `_value` to `_sink` in the
    // order named by `E`.  Big-endian writes most-significant first (matching
    // encode.hpp's put_uint_be); little-endian writes least-significant first.
    template<serial_endian E,
             typename       Sink>
    void
    put_uint(
        Sink&            _sink,
        re_std::uint64_t _value,
        std::size_t      _width
    )
    {
        for (std::size_t _i = 0; _i < _width; ++_i)
        {
            const std::size_t _shift =
                ( E == serial_endian::big )
                    ? ( ( ( _width - 1 ) - _i ) * 8 )
                    : ( _i * 8 );
            _sink.push_back(
                static_cast<byte>((_value >> _shift) & static_cast<byte>(0xFF)));
        }

        return;
    }

    // put_varint
    //   helper: append `_value` as an unsigned LEB128 varint - seven bits per
    // byte, low group first, the high bit set on every byte but the last.  Self-
    // delimiting, so it needs no companion width and is byte-order-independent.
    template<typename Sink>
    void
    put_varint(
        Sink&            _sink,
        re_std::uint64_t _value
    )
    {
        while (_value >= 0x80u)
        {
            _sink.push_back(
                static_cast<byte>((_value & 0x7Fu) | 0x80u));
            _value >>= 7;
        }
        _sink.push_back(static_cast<byte>(_value & 0x7Fu));

        return;
    }

    // put_length
    //   helper: write a container's element count per `L` (byte order `E` for
    // the fixed widths; varint carries its own order).  A fixed width asserts
    // the count fits - a wider count silently wraps, exactly as choosing a
    // bounded encoding for an over-large container would.
    template<serial_length L,
             serial_endian  E,
             typename       Sink>
    void
    put_length(
        Sink&            _sink,
        re_std::uint64_t _count
    )
    {
        if (L == serial_length::u16)
        {
            put_uint<E>(_sink, _count, 2);
        }
        else if (L == serial_length::u32)
        {
            put_uint<E>(_sink, _count, 4);
        }
        else if (L == serial_length::u64)
        {
            put_uint<E>(_sink, _count, 8);
        }
        else  // serial_length::varint
        {
            put_varint(_sink, _count);
        }

        return;
    }

NS_END  // internal


// ===========================================================================
// III. enc_tau<E> - the parameterised leaf element encoder
// ===========================================================================
//   The endian-aware counterpart to encode.hpp's `encode_into`.  Same five
// mutually-exclusive families and the same precedence (the member surface
// first), differing only in that multi-byte scalars are written through
// internal::put_uint<E>.  A member encoder owns its own byte layout and is
// therefore endian-agnostic - the option governs the built-in scalars, which is
// where byte order is observable.

// encode_leaf_into (member surface)
//   function: a value carrying its own `encode_into(sink)` writes itself; its
// format is its own, so `E` does not reach it.
template<serial_endian E,
         typename       Sink,
         typename       Type,
         typename std::enable_if<
             has_member_encode_into<clean_t<Type>, Sink>::value,
             int>::type = 0>
void
encode_leaf_into(Sink& _sink, const Type& _value)
{
    _value.encode_into(_sink);

    return;
}

// encode_leaf_into (bool)
//   function: one byte, 0 or 1 - byte order is immaterial to a single byte.
template<serial_endian E,
         typename       Sink,
         typename       Type,
         typename std::enable_if<
             ( std::is_same<clean_t<Type>, bool>::value &&
               !has_member_encode_into<clean_t<Type>, Sink>::value ),
             int>::type = 0>
void
encode_leaf_into(Sink& _sink, const Type& _value)
{
    _sink.push_back(_value ? static_cast<byte>(1) : static_cast<byte>(0));

    return;
}

// encode_leaf_into (integral, non-bool)
//   function: an integral leaf at its natural width, in `E` order, through its
// two's-complement unsigned pattern.
template<serial_endian E,
         typename       Sink,
         typename       Type,
         typename std::enable_if<
             ( std::is_integral<clean_t<Type>>::value    &&
               !std::is_same<clean_t<Type>, bool>::value &&
               ( sizeof(clean_t<Type>) <= 8 )            &&
               !has_member_encode_into<clean_t<Type>, Sink>::value ),
             int>::type = 0>
void
encode_leaf_into(Sink& _sink, const Type& _value)
{
    using clean_type    = clean_t<Type>;
    using unsigned_type = typename std::make_unsigned<clean_type>::type;

    internal::put_uint<E>(_sink,
        static_cast<re_std::uint64_t>(
            static_cast<unsigned_type>(static_cast<clean_type>(_value))),
        sizeof(clean_type));

    return;
}

// encode_leaf_into (enum)
//   function: an enumeration through its underlying integral type, in `E` order.
template<serial_endian E,
         typename       Sink,
         typename       Type,
         typename std::enable_if<
             ( std::is_enum<clean_t<Type>>::value &&
               !has_member_encode_into<clean_t<Type>, Sink>::value ),
             int>::type = 0>
void
encode_leaf_into(Sink& _sink, const Type& _value)
{
    using underlying_type =
        typename std::underlying_type<clean_t<Type>>::type;
    using unsigned_type =
        typename std::make_unsigned<underlying_type>::type;

    internal::put_uint<E>(_sink,
        static_cast<re_std::uint64_t>(
            static_cast<unsigned_type>(
                static_cast<underlying_type>(_value))),
        sizeof(underlying_type));

    return;
}

// encode_leaf_into (floating, 4- or 8-byte)
//   function: a float or double through its same-width unsigned bit pattern
// (via std::memcpy), then in `E` order.
template<serial_endian E,
         typename       Sink,
         typename       Type,
         typename std::enable_if<
             ( std::is_floating_point<clean_t<Type>>::value          &&
               ( sizeof(clean_t<Type>) == 4 ||
                 sizeof(clean_t<Type>) == 8 )                        &&
               !has_member_encode_into<clean_t<Type>, Sink>::value ),
             int>::type = 0>
void
encode_leaf_into(Sink& _sink, const Type& _value)
{
    using clean_type = clean_t<Type>;
    const clean_type _v = static_cast<clean_type>(_value);

    if (sizeof(clean_type) == 4)
    {
        re_std::uint32_t _bits = 0;
        std::memcpy(&_bits, &_v, 4);
        internal::put_uint<E>(_sink, static_cast<re_std::uint64_t>(_bits), 4);
    }
    else
    {
        re_std::uint64_t _bits = 0;
        std::memcpy(&_bits, &_v, 8);
        internal::put_uint<E>(_sink, _bits, 8);
    }

    return;
}


// ===========================================================================
// IV.  Convenience
// ===========================================================================

// encode_leaf
//   function: enc_tau<E> as a total map to a fresh byte_string - the endian-
// parameterised counterpart to encode.hpp's `encode(value)`.
template<serial_endian E,
         typename       Type>
byte_string
encode_leaf(const Type& _value)
{
    byte_string _out;
    encode_leaf_into<E>(_out, _value);

    return _out;
}


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_BINARY_ENCODE_OPTIONS_HPP
