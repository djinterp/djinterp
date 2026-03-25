/******************************************************************************
* djinterp [container]                                  container_binary.hpp
*
* Container binary encoding/decoding module.
*   Dispatches through container_binary_traits.hpp to encode containers
* into portable binary blobs and decode them back, using the foundational
* primitives from binary.hpp.
*
*   Canonical container contract:
*     WRITE:  std::vector<char> encode() const
*     READ:   static C decode(const char*, std::size_t)
*
*   Three tiers per direction (native → bulk → element), selected at
* compile time.
*
* USAGE:
*   std::vector<int> v = {1, 2, 3};
*   auto blob = container_binary_encode(v);       // with header
*   auto raw  = container_binary_encode_raw(v);   // headerless
*   auto v2   = container_binary_decode<
*                   std::vector<int>>(blob);       // from header
*
* TABLE OF CONTENTS
* =================
* I.      Container Header Builder
* II.     Encode Paths
* III.    Unified Encode Interface
* IV.     Decode Paths
* V.      Unified Decode Interface
*
*
* path:      /inc/container/container_binary.hpp
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
#include "../djinterp.hpp"
#include "../binary/binary.hpp"
#include "./meta/container_binary_traits.hpp"


NS_DJINTERP
NS_CONTAINER

// =============================================================================
// I.   Container Header Builder
// =============================================================================
// Builds a binary_header populated with container-specific
// metadata (type_info from elements, element_size for
// trivially copyable types).

template<typename _Container>
binary::binary_header
make_container_header(
    std::uint64_t _count,
    std::uint64_t _payload) noexcept
{
    using C = clean_t<_Container>;

    auto h = binary::make_header(_count, _payload);

    // type info from container's type_descriptor
    // or element type mapping
    if constexpr (
        traits::has_type_info_integration_v<C>)
    {
        h.flags |=
            binary::binary_flags::has_type_info;
    }

    if constexpr (
        traits::has_trivially_copyable_elements_v<C>)
    {
        h.flags |=
            binary::binary_flags::fixed_elements;

        h.element_size =
            static_cast<std::uint32_t>(
                sizeof(typename C::value_type));

        h.type_info =
            static_cast<d_type_info64>(
                binary::type_info_for_v<
                    typename C::value_type>);
    }

    return h;
}


// =============================================================================
// II.  Encode Paths
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
            ? binary::D_BINARY_HEADER_SIZE : 0;

        std::vector<char> out(hdr_sz + payload);

        if (_with_hdr)
        {
            auto h = make_container_header<
                _Container>(n, payload);

            binary::write_header(
                out.data(), out.size(), h);
        }

        std::memcpy(out.data() + hdr_sz,
                     _src.data(), payload);

        return out;
    }

    // --- element: per-element trivially copyable ---

    template<typename _Container>
    std::vector<char>
    encode_element_trivial(
        const _Container& _src,
        bool              _with_hdr)
    {
        using elem = typename _Container::value_type;

        std::size_t n       = _src.size();
        std::size_t payload = n * sizeof(elem);
        std::size_t hdr_sz  = _with_hdr
            ? binary::D_BINARY_HEADER_SIZE : 0;

        std::vector<char> out(hdr_sz + payload);

        if (_with_hdr)
        {
            auto h = make_container_header<
                _Container>(n, payload);

            binary::write_header(
                out.data(), out.size(), h);
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
    encode_element_method(
        const _Container& _src,
        bool              _with_hdr)
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
            ? binary::D_BINARY_HEADER_SIZE : 0;

        std::vector<char> out(hdr_sz + payload);

        if (_with_hdr)
        {
            auto h = make_container_header<
                _Container>(_src.size(), payload);

            binary::write_header(
                out.data(), out.size(), h);
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
// III. Unified Encode Interface
// =============================================================================

// container_binary_encode (with header)
template<typename _Container>
inline typename std::enable_if<
    traits::is_binary_encodable_v<_Container>,
    std::vector<char>
>::type
container_binary_encode(const _Container& _src)
{
    constexpr auto s =
        traits::container_encode_strategy_v<
            _Container>;

    if constexpr (
        s == traits::binary_encoding_strategy::
                 native)
    {
        // native encode() returns raw payload;
        // wrap with header
        auto payload = _src.encode();

        auto h = make_container_header<_Container>(
            _src.size(), payload.size());

        std::vector<char> out(
            binary::D_BINARY_HEADER_SIZE +
            payload.size());

        binary::write_header(
            out.data(), out.size(), h);

        std::memcpy(
            out.data() +
                binary::D_BINARY_HEADER_SIZE,
            payload.data(),
            payload.size());

        return out;
    }
    else if constexpr (
        s == traits::binary_encoding_strategy::bulk)
    {
        return internal::encode_bulk(_src, true);
    }
    else if constexpr (
        s == traits::binary_encoding_strategy::
                 element)
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

// container_binary_encode_raw (no header)
template<typename _Container>
inline typename std::enable_if<
    traits::is_binary_encodable_v<_Container>,
    std::vector<char>
>::type
container_binary_encode_raw(
    const _Container& _src)
{
    constexpr auto s =
        traits::container_encode_strategy_v<
            _Container>;

    if constexpr (
        s == traits::binary_encoding_strategy::
                 native)
    {
        return _src.encode();
    }
    else if constexpr (
        s == traits::binary_encoding_strategy::bulk)
    {
        return internal::encode_bulk(_src, false);
    }
    else if constexpr (
        s == traits::binary_encoding_strategy::
                 element)
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
// IV.  Decode Paths
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
    decode_element_trivial(
        const char*  _data,
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
    decode_element_method(
        const char*   _data,
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
// V.   Unified Decode Interface
// =============================================================================

// container_binary_decode (with header)
template<typename _Container>
inline typename std::enable_if<
    traits::is_binary_decodable_v<_Container>,
    _Container
>::type
container_binary_decode(
    const char*  _data,
    std::size_t  _size)
{
    binary::binary_header hdr;

    if (!binary::read_header(_data, _size, hdr))
    {
        return _Container{};
    }

    const char*   payload =
        _data + binary::D_BINARY_HEADER_SIZE;
    std::size_t   payload_sz =
        static_cast<std::size_t>(hdr.payload_size);
    std::uint32_t elem_sz = hdr.element_size;

    constexpr auto s =
        traits::container_decode_strategy_v<
            _Container>;

    if constexpr (
        s == traits::binary_decoding_strategy::
                 native)
    {
        return _Container::decode(
            payload, payload_sz);
    }
    else if constexpr (
        s == traits::binary_decoding_strategy::bulk)
    {
        return internal::decode_bulk<_Container>(
            payload, payload_sz);
    }
    else if constexpr (
        s == traits::binary_decoding_strategy::
                 element)
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

// container_binary_decode (vector<char> overload)
template<typename _Container>
inline typename std::enable_if<
    traits::is_binary_decodable_v<_Container>,
    _Container
>::type
container_binary_decode(
    const std::vector<char>& _blob)
{
    return container_binary_decode<_Container>(
        _blob.data(), _blob.size());
}

// container_binary_decode_raw (headerless)
template<typename _Container>
inline typename std::enable_if<
    traits::is_binary_decodable_v<_Container>,
    _Container
>::type
container_binary_decode_raw(
    const char*  _data,
    std::size_t  _size)
{
    constexpr auto s =
        traits::container_decode_strategy_v<
            _Container>;

    if constexpr (
        s == traits::binary_decoding_strategy::
                 native)
    {
        return _Container::decode(_data, _size);
    }
    else if constexpr (
        s == traits::binary_decoding_strategy::bulk)
    {
        return internal::decode_bulk<_Container>(
            _data, _size);
    }
    else if constexpr (
        s == traits::binary_decoding_strategy::
                 element)
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
