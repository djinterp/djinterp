/*******************************************************************************
* djinterp [core]                                             decode_options.hpp
*
*   The CONFIGURABLE face of the leaf decoder - the exact inverse of
* encode_options.hpp.  decode.hpp fixes one decoding (big-endian scalars, an
* 8-byte length prefix); this reads the two open degrees of freedom back under
* the same compile-time parameters (containers.tex, Serialization):
*
*     1. BYTE ORDER (serial_endian).  A multi-byte scalar or length is
*        reassembled most- or least-significant byte first, matching the order
*        it was written in.
*
*     2. LENGTH ENCODING (serial_length).  The leading size is read as a fixed
*        2/4/8-byte field or as a variable-length (LEB128) varint - whichever
*        the encoder chose.  The two ends MUST agree; a length read under the
*        wrong parameter is a different number, and the decode fails or
*        misparses (which is exactly the partiality the model admits).
*
*   Both ends of a round trip must share the same (endian, length) parameters:
* the stream carries no self-description of them (that would be a container
* format, not the bare bit string B*).  serial_fidelity is a decode-side level
* declaration only; see the note in encode_options.hpp.
*
*   This header owns the parameterised LEAF decoder dec_tau<endian>.  It reuses
* decode.hpp's reader, partial-result type, member surface, and leaf-decodability
* trait verbatim, and encode_options.hpp's serial PARAMETER enums - re-
* parameterising only the byte-order and length logic.
*
*   PORTABILITY:
*   C++11 baseline, as decode.hpp.  Includes encode_options.hpp solely for the
* shared serial parameter enums (serial_endian / serial_length / serial_fidelity);
* it takes nothing else from the encode side.
*
*
* path:      /inc/djinterp/core/binary/decode_options.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.06
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    parameterised byte readers  (get_uint<E> / get_length<L,E>)
      -----------------------------------------------------------

II.   dec_tau<E>                  (parameterised leaf decoder)
      --------------------------------------------------------

III.  convenience                 (decode_leaf<E>)
      --------------------------------------------
*/

#ifndef DJINTERP_BINARY_DECODE_OPTIONS_HPP
#define DJINTERP_BINARY_DECODE_OPTIONS_HPP 1

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
#include "../../djinterp.hpp"       // NS_*, feature macros
#include "../meta/type_utility.hpp"  // clean_t
#include "./decode.hpp"          // reader, decode_result, member surface, traits
#include "./encode_options.hpp"  // serial_endian / serial_length / serial_fidelity
// re_std
#include "../../../re_std/cstdint/cstdint.hpp"  // re_std::uint32_t, uint64_t


NS_DJINTERP

// ===========================================================================
// I.   Parameterised byte readers
// ===========================================================================

NS_INTERNAL

    // get_uint
    //   helper: read `_width` (<= 8) bytes from `_reader` into `_out`, in the
    // order named by `E`.  The inverse of encode_options.hpp's put_uint<E>;
    // returns false (and leaves the reader bad) on underflow or an over-wide
    // request.
    template<serial_endian E>
    bool
    get_uint(
        byte_reader&      _reader,
        std::size_t       _width,
        re_std::uint64_t& _out
    )
    {
        byte _tmp[8] = {0};

        if (_width > 8)
        {
            return false;
        }
        if (!_reader.take(_tmp, _width))
        {
            return false;
        }

        re_std::uint64_t _value = 0;
        for (std::size_t _i = 0; _i < _width; ++_i)
        {
            if (E == serial_endian::big)
            {
                // most-significant byte first
                _value = ( _value << 8 )
                       | static_cast<re_std::uint64_t>(_tmp[_i]);
            }
            else
            {
                // least-significant byte first
                _value |= ( static_cast<re_std::uint64_t>(_tmp[_i])
                            << (_i * 8) );
            }
        }

        _out = _value;

        return true;
    }

    // get_varint
    //   helper: read an unsigned LEB128 varint - seven bits per byte, low group
    // first, continuing while the high bit is set.  Fails on underflow or an
    // overlong encoding (more groups than a 64-bit value can hold).  The inverse
    // of put_varint.
    inline bool
    get_varint(
        byte_reader&      _reader,
        re_std::uint64_t& _out
    )
    {
        re_std::uint64_t _result = 0;
        std::size_t      _shift  = 0;

        for (;;)
        {
            byte _b = 0;

            if (!_reader.take(&_b, 1))
            {
                return false;
            }

            _result |= ( static_cast<re_std::uint64_t>(_b & 0x7Fu) << _shift );

            if ((_b & 0x80u) == 0)
            {
                break;
            }

            _shift += 7;

            // a well-formed 64-bit varint is at most ten groups; past 63 bits
            // the encoding is malformed
            if (_shift >= 64)
            {
                return false;
            }
        }

        _out = _result;

        return true;
    }

    // get_length
    //   helper: read a container's element count per `L` (byte order `E` for
    // the fixed widths; varint carries its own order).  The inverse of
    // put_length.
    template<serial_length L,
             serial_endian  E>
    bool
    get_length(
        byte_reader&      _reader,
        re_std::uint64_t& _out
    )
    {
        if (L == serial_length::u16)
        {
            return get_uint<E>(_reader, 2, _out);
        }
        else if (L == serial_length::u32)
        {
            return get_uint<E>(_reader, 4, _out);
        }
        else if (L == serial_length::u64)
        {
            return get_uint<E>(_reader, 8, _out);
        }
        else  // serial_length::varint
        {
            return get_varint(_reader, _out);
        }
    }

    // decode_integral_leaf_e
    //   helper: read an integral (non-bool) value written at its natural width
    // in `E` order, reversing encode_leaf_into's integral path exactly.
    template<serial_endian E,
             typename       Integral>
    decode_result<Integral>
    decode_integral_leaf_e(
        byte_reader& _reader
    )
    {
        using unsigned_type = typename std::make_unsigned<Integral>::type;

        re_std::uint64_t _raw = 0;

        if (!get_uint<E>(_reader, sizeof(Integral), _raw))
        {
            return decode_failure<Integral>();
        }

        return decode_success(
            static_cast<Integral>(static_cast<unsigned_type>(_raw)));
    }

    // decode_floating_leaf_e
    //   helper: read a 4- or 8-byte floating value's same-width unsigned pattern
    // in `E` order, then memcpy it back, reversing the floating path exactly.
    template<serial_endian E,
             typename       Float>
    decode_result<Float>
    decode_floating_leaf_e(
        byte_reader& _reader
    )
    {
        re_std::uint64_t _raw = 0;

        if (!get_uint<E>(_reader, sizeof(Float), _raw))
        {
            return decode_failure<Float>();
        }

        Float _value = Float();

        if (sizeof(Float) == 4)
        {
            re_std::uint32_t _bits = static_cast<re_std::uint32_t>(_raw);
            std::memcpy(&_value, &_bits, 4);
        }
        else
        {
            re_std::uint64_t _bits = _raw;
            std::memcpy(&_value, &_bits, 8);
        }

        return decode_success(_value);
    }

NS_END  // internal


// ===========================================================================
// II.  dec_tau<E> - the parameterised leaf decoder
// ===========================================================================
//   The endian-aware counterpart to decode.hpp's `decode`.  Same five families
// and the same precedence (the static member surface first).  `Type` and `E`
// are both explicit: the return type depends on `Type`, and `E` cannot be
// deduced from a value argument (there is none).  A member decoder owns its own
// byte layout, so `E` does not reach it.

// decode_leaf (member surface)
template<serial_endian E,
         typename       Type,
         typename std::enable_if<
             has_member_decode<clean_t<Type>>::value,
             int>::type = 0>
decode_result<clean_t<Type>>
decode_leaf(byte_reader& _reader)
{
    return clean_t<Type>::decode(_reader);
}

// decode_leaf (bool)
template<serial_endian E,
         typename       Type,
         typename std::enable_if<
             ( std::is_same<clean_t<Type>, bool>::value &&
               !has_member_decode<clean_t<Type>>::value ),
             int>::type = 0>
decode_result<clean_t<Type>>
decode_leaf(byte_reader& _reader)
{
    byte _b = 0;

    if (!_reader.take(&_b, 1))
    {
        return decode_failure<bool>();
    }

    return decode_success(static_cast<bool>(_b != 0));
}

// decode_leaf (integral, non-bool)
template<serial_endian E,
         typename       Type,
         typename std::enable_if<
             ( std::is_integral<clean_t<Type>>::value    &&
               !std::is_same<clean_t<Type>, bool>::value &&
               ( sizeof(clean_t<Type>) <= 8 )            &&
               !has_member_decode<clean_t<Type>>::value ),
             int>::type = 0>
decode_result<clean_t<Type>>
decode_leaf(byte_reader& _reader)
{
    return internal::decode_integral_leaf_e<E, clean_t<Type>>(_reader);
}

// decode_leaf (enum)
template<serial_endian E,
         typename       Type,
         typename std::enable_if<
             ( std::is_enum<clean_t<Type>>::value &&
               !has_member_decode<clean_t<Type>>::value ),
             int>::type = 0>
decode_result<clean_t<Type>>
decode_leaf(byte_reader& _reader)
{
    using clean_type      = clean_t<Type>;
    using underlying_type = typename std::underlying_type<clean_type>::type;

    decode_result<underlying_type> _u =
        internal::decode_integral_leaf_e<E, underlying_type>(_reader);

    if (!_u.ok)
    {
        return decode_failure<clean_type>();
    }

    return decode_success(static_cast<clean_type>(_u.value));
}

// decode_leaf (floating, 4- or 8-byte)
template<serial_endian E,
         typename       Type,
         typename std::enable_if<
             ( std::is_floating_point<clean_t<Type>>::value          &&
               ( sizeof(clean_t<Type>) == 4 ||
                 sizeof(clean_t<Type>) == 8 )                        &&
               !has_member_decode<clean_t<Type>>::value ),
             int>::type = 0>
decode_result<clean_t<Type>>
decode_leaf(byte_reader& _reader)
{
    return internal::decode_floating_leaf_e<E, clean_t<Type>>(_reader);
}


// ===========================================================================
// III. Convenience
// ===========================================================================

// decode_leaf
//   function: dec_tau<E> over a whole byte_string - wraps it in a reader and
// reads one leaf `Type` from the front.  The endian-parameterised counterpart
// to decode.hpp's `decode(byte_string)`.
template<serial_endian E,
         typename       Type>
decode_result<clean_t<Type>>
decode_leaf(const byte_string& _bytes)
{
    byte_reader _reader(_bytes);

    return decode_leaf<E, Type>(_reader);
}


NS_END  // djinterp

#endif  // floor, for now


#endif  // DJINTERP_BINARY_DECODE_OPTIONS_HPP
