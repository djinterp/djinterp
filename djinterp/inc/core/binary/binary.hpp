/******************************************************************************
* djinterp [container]                                            binary.hpp
*
* Binary encoding/decoding module for the djinterp container framework.
*   Dispatches through container_binary_traits.hpp to encode containers
* into portable binary blobs and decode them back.
*
*   Canonical contract:
*     WRITE:  std::vector<char> encode() const
*     READ:   static C decode(const char*, std::size_t)
*
*   Three tiers per direction (native → bulk → element), selected at
* compile time.  An optional binary header prepends type metadata.
*
* BINARY HEADER (40 bytes):
*   bytes  0- 3:  magic         (0x4E426A64)
*   bytes  4- 5:  version       (1)
*   bytes  6- 7:  flags         (endianness, options)
*   bytes  8-15:  type_info64   (d_type_info64 descriptor)
*   bytes 16-19:  element_size  (0 if variable)
*   bytes 20-23:  reserved
*   bytes 24-31:  count         (uint64_t element count)
*   bytes 32-39:  payload_size  (uint64_t byte count)
*
* TABLE OF CONTENTS
* =================
* I.      Constants and Header
* II.     Header Operations
* III.    Encode Paths
* IV.     Unified Encode Interface
* V.      Decode Paths
* VI.     Unified Decode Interface
*
*
* path:      \inc\container\binary.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.23
******************************************************************************/

#ifndef DJINTERP_CONTAINER_BINARY_
#define DJINTERP_CONTAINER_BINARY_ 1

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>
#include "..\djinterp.hpp"
#include "..\..\c\type_info.h"
#include "meta\container_binary_traits.hpp"


NS_DJINTERP
NS_CONTAINER

// =============================================================================
// I.   Constants and Header
// =============================================================================

static constexpr std::uint32_t D_BINARY_MAGIC   =
    0x4E426A64u;
static constexpr std::uint16_t D_BINARY_VERSION =
    1u;
static constexpr std::size_t   D_BINARY_HEADER_SIZE =
    40u;

enum class DBinaryFlags : std::uint16_t
{
    none            = 0x0000,
    little_endian   = 0x0001,
    big_endian      = 0x0002,
    has_type_info   = 0x0010,
    fixed_elements  = 0x0020
};

inline constexpr DBinaryFlags
operator|(DBinaryFlags _a, DBinaryFlags _b) noexcept
{
    return static_cast<DBinaryFlags>(
        static_cast<std::uint16_t>(_a) |
        static_cast<std::uint16_t>(_b));
}

inline constexpr bool
has_flag(DBinaryFlags _s,
         DBinaryFlags _f) noexcept
{
    return (static_cast<std::uint16_t>(_s) &
            static_cast<std::uint16_t>(_f)) != 0;
}

struct binary_header
{
    std::uint32_t magic;
    std::uint16_t version;
    DBinaryFlags  flags;
    d_type_info64 type_descriptor;
    std::uint32_t element_size;
    std::uint32_t reserved;
    std::uint64_t count;
    std::uint64_t payload_size;
};

static_assert(sizeof(binary_header) ==
              D_BINARY_HEADER_SIZE,
    "binary_header size mismatch");


// =============================================================================
// II.  Header Operations
// =============================================================================

inline DBinaryFlags
detect_endianness() noexcept
{
    const std::uint16_t t = 0x0001;

    return (*reinterpret_cast<const unsigned char*>(
                &t) == 0x01)
        ? DBinaryFlags::little_endian
        : DBinaryFlags::big_endian;
}

template<typename _Container>
binary_header
make_header(std::uint64_t _count,
            std::uint64_t _payload) noexcept
{
    using C = clean_t<_Container>;

    binary_header h{};

    h.magic        = D_BINARY_MAGIC;
    h.version      = D_BINARY_VERSION;
    h.flags        = detect_endianness();
    h.count        = _count;
    h.payload_size = _payload;

    if constexpr (
        traits::has_type_info_integration_v<C>)
    {
        h.flags = h.flags |
                  DBinaryFlags::has_type_info;
    }

    if constexpr (
        traits::has_trivially_copyable_elements_v<C>)
    {
        h.flags = h.flags |
                  DBinaryFlags::fixed_elements;
        h.element_size =
            static_cast<std::uint32_t>(
                sizeof(typename C::value_type));
    }

    return h;
}

inline bool
read_header(const char*    _data,
            std::size_t    _size,
            binary_header& _out)
{
    if ( (!_data) ||
         (_size < D_BINARY_HEADER_SIZE) )
    {
        return false;
    }

    std::memcpy(&_out, _data, D_BINARY_HEADER_SIZE);

    return ( (_out.magic == D_BINARY_MAGIC) &&
             (_out.version <= D_BINARY_VERSION) );
}


// =============================================================================
// III. Encode Paths
// =============================================================================

NS_INTERNAL

    // --- bulk: contiguous memcpy ---

    template<typename _Container>
    std::vector<char>
    encode_bulk(const _Container& _src,
                bool              _with_hdr)
    {
        using elem = typename _Container::value_type;

        std::size_t n       = _src.size();
        std::size_t payload = n * sizeof(elem);
        std::size_t hdr_sz  = _with_hdr
            ? D_BINARY_HEADER_SIZE : 0;

        std::vector<char> out(hdr_sz + payload);

        if (_with_hdr)
        {
            auto h = make_header<_Container>(
                n, payload);

            std::memcpy(out.data(),
                        &h, D_BINARY_HEADER_SIZE);
        }

        std::memcpy(out.data() + hdr_sz,
                     _src.data(), payload);

        return out;
    }

    // --- element: per-element trivially copyable ---

    template<typename _Container>
    std::vector<char>
    encode_element_trivial(const _Container& _src,
                           bool _with_hdr)
    {
        using elem = typename _Container::value_type;

        std::size_t n       = _src.size();
        std::size_t payload = n * sizeof(elem);
        std::size_t hdr_sz  = _with_hdr
            ? D_BINARY_HEADER_SIZE : 0;

        std::vector<char> out(hdr_sz + payload);

        if (_with_hdr)
        {
            auto h = make_header<_Container>(
                n, payload);

            std::memcpy(out.data(),
                        &h, D_BINARY_HEADER_SIZE);
        }

        char* cursor = out.data() + hdr_sz;

        for (const auto& e : _src)
        {
            std::memcpy(cursor, &e, sizeof(elem));
            cursor += sizeof(elem);
        }

        return out;
    }

    // --- element: per-element encode() ---

    template<typename _Container>
    std::vector<char>
    encode_element_method(const _Container& _src,
                          bool _with_hdr)
    {
        // first pass: collect element blobs
        std::vector<std::vector<char>> blobs;
        blobs.reserve(_src.size());

        std::size_t payload = 0;

        for (const auto& e : _src)
        {
            auto blob = e.encode();
            payload += blob.size();
            blobs.push_back(
                static_cast<std::vector<char>&&>(
                    blob));
        }

        std::size_t hdr_sz = _with_hdr
            ? D_BINARY_HEADER_SIZE : 0;

        std::vector<char> out(hdr_sz + payload);

        if (_with_hdr)
        {
            auto h = make_header<_Container>(
                _src.size(), payload);

            std::memcpy(out.data(),
                        &h, D_BINARY_HEADER_SIZE);
        }

        char* cursor = out.data() + hdr_sz;

        for (const auto& blob : blobs)
        {
            std::memcpy(cursor,
                        blob.data(), blob.size());
            cursor += blob.size();
        }

        return out;
    }

NS_END  // internal


// =============================================================================
// IV.  Unified Encode Interface
// =============================================================================

// binary_encode (with header)
template<typename _Container>
inline typename std::enable_if<
    traits::is_binary_encodable_v<_Container>,
    std::vector<char>
>::type
binary_encode(const _Container& _src)
{
    constexpr auto s =
        traits::container_encode_strategy_v<
            _Container>;

    if constexpr (
        s == traits::DBinaryEncodeStrategy::native)
    {
        // native encode() returns raw payload;
        // wrap with header
        auto payload = _src.encode();
        auto h = make_header<_Container>(
            _src.size(), payload.size());

        std::vector<char> out(
            D_BINARY_HEADER_SIZE + payload.size());

        std::memcpy(out.data(),
                    &h, D_BINARY_HEADER_SIZE);
        std::memcpy(out.data() +
                        D_BINARY_HEADER_SIZE,
                    payload.data(),
                    payload.size());

        return out;
    }
    else if constexpr (
        s == traits::DBinaryEncodeStrategy::bulk)
    {
        return internal::encode_bulk(_src, true);
    }
    else if constexpr (
        s == traits::DBinaryEncodeStrategy::element)
    {
        if constexpr (
            traits::has_trivially_copyable_elements_v<
                _Container>)
        {
            return internal::encode_element_trivial(
                _src, true);
        }
        else
        {
            return internal::encode_element_method(
                _src, true);
        }
    }
    else
    {
        return {};
    }
}

// binary_encode_raw (no header)
template<typename _Container>
inline typename std::enable_if<
    traits::is_binary_encodable_v<_Container>,
    std::vector<char>
>::type
binary_encode_raw(const _Container& _src)
{
    constexpr auto s =
        traits::container_encode_strategy_v<
            _Container>;

    if constexpr (
        s == traits::DBinaryEncodeStrategy::native)
    {
        return _src.encode();
    }
    else if constexpr (
        s == traits::DBinaryEncodeStrategy::bulk)
    {
        return internal::encode_bulk(_src, false);
    }
    else if constexpr (
        s == traits::DBinaryEncodeStrategy::element)
    {
        if constexpr (
            traits::has_trivially_copyable_elements_v<
                _Container>)
        {
            return internal::encode_element_trivial(
                _src, false);
        }
        else
        {
            return internal::encode_element_method(
                _src, false);
        }
    }
    else
    {
        return {};
    }
}


// =============================================================================
// V.   Decode Paths
// =============================================================================

NS_INTERNAL

    // --- bulk: contiguous memcpy ---

    template<typename _Container>
    _Container
    decode_bulk(const char*  _data,
                std::size_t  _payload)
    {
        using elem = typename _Container::value_type;

        std::size_t n = _payload / sizeof(elem);

        _Container result;
        result.resize(n);

        std::memcpy(result.data(),
                     _data,
                     n * sizeof(elem));

        return result;
    }

    // --- element: per-element trivially copyable ---

    template<typename _Container>
    _Container
    decode_element_trivial(const char*  _data,
                           std::size_t  _payload)
    {
        using elem = typename _Container::value_type;

        std::size_t n = _payload / sizeof(elem);

        _Container result;
        const char* cursor = _data;

        for (std::size_t i = 0; i < n; ++i)
        {
            elem e;
            std::memcpy(&e, cursor, sizeof(elem));
            cursor += sizeof(elem);

            result.push_back(
                static_cast<elem&&>(e));
        }

        return result;
    }

    // --- element: per-element decode() ---

    template<typename _Container>
    _Container
    decode_element_method(const char*   _data,
                          std::size_t   _payload,
                          std::uint32_t _elem_sz)
    {
        using elem = typename _Container::value_type;

        _Container result;

        if (_elem_sz == 0)
        {
            return result;
        }

        std::size_t n = _payload / _elem_sz;
        const char* cursor = _data;

        for (std::size_t i = 0; i < n; ++i)
        {
            result.push_back(
                elem::decode(cursor, _elem_sz));
            cursor += _elem_sz;
        }

        return result;
    }

NS_END  // internal


// =============================================================================
// VI.  Unified Decode Interface
// =============================================================================

// binary_decode (with header)
template<typename _Container>
inline typename std::enable_if<
    traits::is_binary_decodable_v<_Container>,
    _Container
>::type
binary_decode(const char*  _data,
              std::size_t  _size)
{
    binary_header hdr;

    if (!read_header(_data, _size, hdr))
    {
        return _Container{};
    }

    const char*   payload    =
        _data + D_BINARY_HEADER_SIZE;
    std::size_t   payload_sz =
        static_cast<std::size_t>(hdr.payload_size);
    std::uint32_t elem_sz    = hdr.element_size;

    constexpr auto s =
        traits::container_decode_strategy_v<
            _Container>;

    if constexpr (
        s == traits::DBinaryDecodeStrategy::native)
    {
        return _Container::decode(
            payload, payload_sz);
    }
    else if constexpr (
        s == traits::DBinaryDecodeStrategy::bulk)
    {
        return internal::decode_bulk<_Container>(
            payload, payload_sz);
    }
    else if constexpr (
        s == traits::DBinaryDecodeStrategy::element)
    {
        if constexpr (
            traits::has_trivially_copyable_elements_v<
                _Container>)
        {
            return internal::decode_element_trivial<
                _Container>(payload, payload_sz);
        }
        else
        {
            return internal::decode_element_method<
                _Container>(
                    payload, payload_sz, elem_sz);
        }
    }
    else
    {
        return _Container{};
    }
}

// binary_decode (vector<char> overload)
template<typename _Container>
inline typename std::enable_if<
    traits::is_binary_decodable_v<_Container>,
    _Container
>::type
binary_decode(const std::vector<char>& _blob)
{
    return binary_decode<_Container>(
        _blob.data(), _blob.size());
}

// binary_decode_raw (headerless)
template<typename _Container>
inline typename std::enable_if<
    traits::is_binary_decodable_v<_Container>,
    _Container
>::type
binary_decode_raw(const char*  _data,
                  std::size_t  _size)
{
    constexpr auto s =
        traits::container_decode_strategy_v<
            _Container>;

    if constexpr (
        s == traits::DBinaryDecodeStrategy::native)
    {
        return _Container::decode(_data, _size);
    }
    else if constexpr (
        s == traits::DBinaryDecodeStrategy::bulk)
    {
        return internal::decode_bulk<_Container>(
            _data, _size);
    }
    else if constexpr (
        s == traits::DBinaryDecodeStrategy::element)
    {
        if constexpr (
            traits::has_trivially_copyable_elements_v<
                _Container>)
        {
            return internal::decode_element_trivial<
                _Container>(_data, _size);
        }
        else
        {
            return _Container{};
        }
    }
    else
    {
        return _Container{};
    }
}


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_BINARY_
