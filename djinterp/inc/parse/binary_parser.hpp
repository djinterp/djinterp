/******************************************************************************
* djinterp [core]                                           binary_parser.hpp
*
* Binary parser specializations:
*   This header extends the generic parser framework with types and
* utilities specific to parsing byte-oriented (binary) input.  It
* provides:
*   - byte_order          — endianness indicator
*   - binary_parse_state  — a parse_state<unsigned char> with byte-order
*                           awareness and primitive extraction helpers
*   - binary_parser_base  — CRTP base threading binary_parse_state through
*                           the derived parser's do_parse
*   - built-in extractors for fixed-width integer types
*
*
* path:      /inc/cpp/parse/binary_parser.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2025.03.15
******************************************************************************/

#ifndef DJINTERP_BINARY_PARSER_
#define DJINTERP_BINARY_PARSER_ 1

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include "./parser.hpp"


NS_DJINTERP
NS_PARSE


// ================================================================
//  byte_order
// ================================================================

// DByteOrder
//   enum: specifies the byte order used when extracting multi-
// byte values from a binary stream.
enum DByteOrder
{
    DByteOrderLittle    = 0,
    DByteOrderBig       = 1,
    DByteOrderNative    = 2
};


// ================================================================
//  byte-swap helpers
// ================================================================

NS_INTERNAL

    // bswap16
    //   function: byte-swaps a 16-bit unsigned integer.
    D_STATIC_CONSTEXPR_INLINE
    std::uint16_t bswap16(std::uint16_t _v)
    {
        return static_cast<std::uint16_t>(
            ((_v & 0x00FFu) << 8u) |
            ((_v & 0xFF00u) >> 8u)
        );
    }

    // bswap32
    //   function: byte-swaps a 32-bit unsigned integer.
    D_STATIC_CONSTEXPR_INLINE
    std::uint32_t bswap32(std::uint32_t _v)
    {
        return ( ((_v & 0x000000FFu) << 24u) |
                 ((_v & 0x0000FF00u) <<  8u) |
                 ((_v & 0x00FF0000u) >>  8u) |
                 ((_v & 0xFF000000u) >> 24u) );
    }

    // bswap64
    //   function: byte-swaps a 64-bit unsigned integer.
    D_STATIC_CONSTEXPR_INLINE
    std::uint64_t bswap64(std::uint64_t _v)
    {
        return ( ((_v & 0x00000000000000FFull) << 56u) |
                 ((_v & 0x000000000000FF00ull) << 40u) |
                 ((_v & 0x0000000000FF0000ull) << 24u) |
                 ((_v & 0x00000000FF000000ull) <<  8u) |
                 ((_v & 0x000000FF00000000ull) >>  8u) |
                 ((_v & 0x0000FF0000000000ull) >> 24u) |
                 ((_v & 0x00FF000000000000ull) >> 40u) |
                 ((_v & 0xFF00000000000000ull) >> 56u) );
    }

    // is_native_little_endian
    //   function: returns true if the host is little-endian.
    // Performs a runtime check; on most compilers this will be
    // constant-folded.
    D_STATIC_INLINE bool is_native_little_endian()
    {
        const std::uint32_t probe = 0x01u;

        unsigned char first_byte;
        std::memcpy(&first_byte, &probe, 1);

        return (first_byte == 0x01u);
    }

    // needs_swap
    //   function: returns true when bytes must be swapped for
    // the requested order on this host.
    D_STATIC_INLINE bool needs_swap(DByteOrder _order)
    {
        if (_order == DByteOrderNative)
        {
            return false;
        }

        bool host_le = is_native_little_endian();

        return ( (_order == DByteOrderLittle && !host_le) ||
                 (_order == DByteOrderBig    &&  host_le) );
    }

NS_END  // internal


// ================================================================
//  binary_parse_state
// ================================================================

// binary_parse_state
//   struct: extends parse_state<unsigned char> with byte-order
// awareness and primitive value extraction helpers.
struct binary_parse_state : public parse_state<unsigned char>
{
private:
    using base_type = parse_state<unsigned char>;

public:
    using element_type = unsigned char;

    DByteOrder  byte_order;

    binary_parse_state()
        : base_type  ()
        , byte_order (DByteOrderNative)
    {
    }

    binary_parse_state(const unsigned char*    _data,
                       std::size_t             _length,
                       DByteOrder              _byte_order = DByteOrderNative,
                       std::size_t             _offset     = 0)
        : base_type  (_data, _length, _offset)
        , byte_order (_byte_order)
    {
    }

    // read_u8
    //   extracts a single unsigned byte and advances.
    parse_result<std::uint8_t> read_u8()
    {
        if (remaining() < 1)
        {
            return parse_result<std::uint8_t>::make_error(
                DParseStatusEndOfInput,
                offset,
                "insufficient bytes for u8"
            );
        }

        std::uint8_t value = data[offset];
        advance(1);

        return parse_result<std::uint8_t>(value);
    }

    // read_u16
    //   extracts a 16-bit unsigned integer, applying byte-order
    // correction, and advances.
    parse_result<std::uint16_t> read_u16()
    {
        if (remaining() < 2)
        {
            return parse_result<std::uint16_t>::make_error(
                DParseStatusEndOfInput,
                offset,
                "insufficient bytes for u16"
            );
        }

        std::uint16_t raw;
        std::memcpy(&raw, data + offset, 2);
        advance(2);

        if (internal::needs_swap(byte_order))
        {
            raw = internal::bswap16(raw);
        }

        return parse_result<std::uint16_t>(raw);
    }

    // read_u32
    //   extracts a 32-bit unsigned integer, applying byte-order
    // correction, and advances.
    parse_result<std::uint32_t> read_u32()
    {
        if (remaining() < 4)
        {
            return parse_result<std::uint32_t>::make_error(
                DParseStatusEndOfInput,
                offset,
                "insufficient bytes for u32"
            );
        }

        std::uint32_t raw;
        std::memcpy(&raw, data + offset, 4);
        advance(4);

        if (internal::needs_swap(byte_order))
        {
            raw = internal::bswap32(raw);
        }

        return parse_result<std::uint32_t>(raw);
    }

    // read_u64
    //   extracts a 64-bit unsigned integer, applying byte-order
    // correction, and advances.
    parse_result<std::uint64_t> read_u64()
    {
        if (remaining() < 8)
        {
            return parse_result<std::uint64_t>::make_error(
                DParseStatusEndOfInput,
                offset,
                "insufficient bytes for u64"
            );
        }

        std::uint64_t raw;
        std::memcpy(&raw, data + offset, 8);
        advance(8);

        if (internal::needs_swap(byte_order))
        {
            raw = internal::bswap64(raw);
        }

        return parse_result<std::uint64_t>(raw);
    }

    // read_i8
    //   extracts a signed 8-bit integer and advances.
    parse_result<std::int8_t> read_i8()
    {
        auto result = read_u8();

        if (!result.ok())
        {
            return parse_result<std::int8_t>(result.error());
        }

        return parse_result<std::int8_t>(
            static_cast<std::int8_t>(result.value())
        );
    }

    // read_i16
    //   extracts a signed 16-bit integer and advances.
    parse_result<std::int16_t> read_i16()
    {
        auto result = read_u16();

        if (!result.ok())
        {
            return parse_result<std::int16_t>(result.error());
        }

        return parse_result<std::int16_t>(
            static_cast<std::int16_t>(result.value())
        );
    }

    // read_i32
    //   extracts a signed 32-bit integer and advances.
    parse_result<std::int32_t> read_i32()
    {
        auto result = read_u32();

        if (!result.ok())
        {
            return parse_result<std::int32_t>(result.error());
        }

        return parse_result<std::int32_t>(
            static_cast<std::int32_t>(result.value())
        );
    }

    // read_i64
    //   extracts a signed 64-bit integer and advances.
    parse_result<std::int64_t> read_i64()
    {
        auto result = read_u64();

        if (!result.ok())
        {
            return parse_result<std::int64_t>(result.error());
        }

        return parse_result<std::int64_t>(
            static_cast<std::int64_t>(result.value())
        );
    }

    // read_bytes
    //   extracts a span of _count bytes without interpretation.
    // Returns the offset at which the span began; the caller
    // can use (data + returned_offset) to access the raw bytes.
    parse_result<std::size_t> read_bytes(std::size_t _count)
    {
        if (remaining() < _count)
        {
            return parse_result<std::size_t>::make_error(
                DParseStatusEndOfInput,
                offset,
                "insufficient bytes for read_bytes"
            );
        }

        std::size_t start = offset;
        advance(_count);

        return parse_result<std::size_t>(start);
    }
};


// ================================================================
//  binary_parser_base
// ================================================================

// binary_parser_base
//   class: CRTP base for binary parsers.  Threads a
// binary_parse_state through the derived parser's do_parse.
//
//   _Derived must expose:
//     - `using result_type = ...;`
//     - `parse_result<result_type>
//        do_parse(binary_parse_state&);`
template<typename _Derived>
class binary_parser_base
{
private:
    using derived_type = _Derived;

    derived_type& self()
    {
        return static_cast<derived_type&>(*this);
    }

    const derived_type& self() const
    {
        return static_cast<const derived_type&>(*this);
    }

protected:
    binary_parser_base()
    {
    }

    ~binary_parser_base()
    {
    }

public:
    using input_type = unsigned char;

    // parse
    //   delegates to the derived do_parse with a
    // binary_parse_state.
    auto parse(binary_parse_state& _state)
        -> parse_result<typename derived_type::result_type>
    {
        return self().do_parse(_state);
    }

    // parse (convenience — raw pointer + length)
    //   constructs a binary_parse_state and delegates.
    auto parse(const unsigned char* _data,
               std::size_t          _length,
               DByteOrder           _order = DByteOrderNative)
        -> parse_result<typename derived_type::result_type>
    {
        binary_parse_state state(_data, _length, _order);

        return parse(state);
    }
};


// ================================================================
//  built-in binary parsers
// ================================================================

// extract_u8
//   parser: extracts a single unsigned byte.
class extract_u8 : public binary_parser_base<extract_u8>
{
public:
    using result_type = std::uint8_t;

    parse_result<std::uint8_t> do_parse(binary_parse_state& _state)
    {
        return _state.read_u8();
    }
};

// extract_u16
//   parser: extracts a 16-bit unsigned integer with byte-order
// correction.
class extract_u16 : public binary_parser_base<extract_u16>
{
public:
    using result_type = std::uint16_t;

    parse_result<std::uint16_t> do_parse(binary_parse_state& _state)
    {
        return _state.read_u16();
    }
};

// extract_u32
//   parser: extracts a 32-bit unsigned integer with byte-order
// correction.
class extract_u32 : public binary_parser_base<extract_u32>
{
public:
    using result_type = std::uint32_t;

    parse_result<std::uint32_t> do_parse(binary_parse_state& _state)
    {
        return _state.read_u32();
    }
};

// extract_u64
//   parser: extracts a 64-bit unsigned integer with byte-order
// correction.
class extract_u64 : public binary_parser_base<extract_u64>
{
public:
    using result_type = std::uint64_t;

    parse_result<std::uint64_t> do_parse(binary_parse_state& _state)
    {
        return _state.read_u64();
    }
};

// extract_bytes
//   parser: extracts a fixed-length span of raw bytes, returning
// the starting offset of the span.
template<std::size_t _N>
class extract_bytes : public binary_parser_base<extract_bytes<_N>>
{
public:
    using result_type = std::size_t;

    parse_result<std::size_t> do_parse(binary_parse_state& _state)
    {
        return _state.read_bytes(_N);
    }
};


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_BINARY_PARSER_
