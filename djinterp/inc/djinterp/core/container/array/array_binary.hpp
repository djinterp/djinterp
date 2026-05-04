/******************************************************************************
* djinterp [container]                                      array_binary.hpp
*
* Array-specific binary encoding/decoding module.
*   Specializes the generic container_binary.hpp infrastructure for
* contiguous array containers, exploiting data() pointer access for
* zero-copy encoding and direct-to-storage decoding.
*
*   Where container_binary.hpp dispatches through three strategy tiers
* (native → bulk → element), this module provides array-optimized
* paths that operate entirely through data() + size(), never touching
* iterators.
*
* ARRAY-SPECIFIC OPTIMIZATIONS:
*   - Direct memcpy via data() (no iterator indirection)
*   - Subarray encoding: encode a [offset, count) slice without
*     copying the elements first
*   - In-place decode: decode directly into an existing array's
*     data() buffer, no temporary allocation
*   - Circular buffer linearization: encode a circular buffer by
*     writing the logical order (head..tail) as a flat sequence
*   - Delta encoding: for sorted integer arrays, encode the
*     differences between adjacent elements for compact storage
*   - Chunked encode/decode: encode/decode in fixed-size chunks
*     for chunked arrays
*
* DESIGN:
*   Two layers following the array CRTP pattern:
*
*   array_binary_base<D>  — read-only encode operations
*     Encode to blob, encode subarray, encode raw (headerless),
*     delta encode (sorted), circular encode.
*
*   array_binary_mutable<D>  — decode-into-self operations
*     Decode from blob into existing storage, decode in-place
*     from raw payload, delta decode.
*
*   Free functions provide non-member interfaces for use with
*   any contiguous container without CRTP inheritance.
*
* DEPENDENCIES:
*   container_binary.hpp           — header format, make_container_header
*   array_container.hpp            — chunk_ref, circular_iterator
*   array_container_traits.hpp     — is_contiguous_array_v, etc.
*
* TABLE OF CONTENTS
* =================
* I.      array_binary_base (CRTP) — encode layer
* II.     array_binary_mutable (CRTP) — decode-into layer
* III.    Free-Function Array Encode
* IV.     Free-Function Array Decode
* V.      Free-Function Delta Encode/Decode
* VI.     Free-Function Circular Encode
*
*
* path:      \inc\container\array_binary.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                      date: 2026.03.29
******************************************************************************/

#ifndef DJINTERP_ARRAY_BINARY_
#define DJINTERP_ARRAY_BINARY_ 1

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>
#include "../../djinterp.hpp"
#include "../container_binary.hpp"
#include "array_container.hpp"
#include "array_container_traits.hpp"


NS_DJINTERP
NS_CONTAINER

// =============================================================================
// I.   array_binary_base (CRTP) — encode layer
// =============================================================================
// Read-only binary encoding operations.  All methods
// operate via data() + size() — no iteration dependency.

template<typename _Derived>
class array_binary_base
{
protected:
    constexpr array_binary_base()  = default;
    ~array_binary_base() = default;

private:
    constexpr const _Derived& self() const noexcept
    {
        return static_cast<
            const _Derived&>(*this);
    }

    using value_type =
        typename _Derived::value_type;

public:
    // --- full encode ---

    // encode
    //   encodes the entire array into a binary blob
    // with header.  For trivially copyable elements,
    // this is a single memcpy from data().
    template<typename V = value_type>
    typename std::enable_if<
        std::is_trivially_copyable_v<V>,
        std::vector<char>
    >::type
    encode() const
    {
        std::size_t n       = self().size();
        std::size_t payload = n * sizeof(V);
        std::size_t hdr_sz  =
            binary::D_BINARY_HEADER_SIZE;

        std::vector<char> out(hdr_sz + payload);

        auto h = make_container_header<_Derived>(
            n, payload);

        binary::write_header(
            out.data(), out.size(), h);

        std::memcpy(
            out.data() + hdr_sz,
            self().data(),
            payload);

        return out;
    }

    // encode_raw
    //   encodes the array without a header (raw
    // payload only).
    template<typename V = value_type>
    typename std::enable_if<
        std::is_trivially_copyable_v<V>,
        std::vector<char>
    >::type
    encode_raw() const
    {
        std::size_t payload =
            self().size() * sizeof(V);

        std::vector<char> out(payload);

        std::memcpy(
            out.data(),
            self().data(),
            payload);

        return out;
    }

    // --- subarray encode ---

    // encode_subarray
    //   encodes [_offset, _offset + _count) without
    // copying elements into a temporary first.
    template<typename V = value_type>
    typename std::enable_if<
        std::is_trivially_copyable_v<V>,
        std::vector<char>
    >::type
    encode_subarray(
        std::size_t _offset,
        std::size_t _count) const
    {
        std::size_t sz = self().size();

        std::size_t actual_offset =
            (_offset < sz) ? _offset : sz;

        std::size_t remaining =
            sz - actual_offset;

        std::size_t actual_count =
            (_count < remaining)
                ? _count : remaining;

        std::size_t payload =
            actual_count * sizeof(V);

        std::size_t hdr_sz =
            binary::D_BINARY_HEADER_SIZE;

        std::vector<char> out(hdr_sz + payload);

        auto h = make_container_header<_Derived>(
            actual_count, payload);

        binary::write_header(
            out.data(), out.size(), h);

        std::memcpy(
            out.data() + hdr_sz,
            self().data() + actual_offset,
            payload);

        return out;
    }

    // encode_subarray_raw (headerless)
    template<typename V = value_type>
    typename std::enable_if<
        std::is_trivially_copyable_v<V>,
        std::vector<char>
    >::type
    encode_subarray_raw(
        std::size_t _offset,
        std::size_t _count) const
    {
        std::size_t sz = self().size();

        std::size_t actual_offset =
            (_offset < sz) ? _offset : sz;

        std::size_t remaining =
            sz - actual_offset;

        std::size_t actual_count =
            (_count < remaining)
                ? _count : remaining;

        std::size_t payload =
            actual_count * sizeof(V);

        std::vector<char> out(payload);

        std::memcpy(
            out.data(),
            self().data() + actual_offset,
            payload);

        return out;
    }

    // --- delta encode (sorted arrays) ---

    // encode_delta
    //   for sorted integer arrays: encodes the
    // differences between adjacent elements.  The
    // first element is stored as-is; each subsequent
    // element is stored as (current - previous).
    //
    // This produces smaller payloads when the values
    // are close together (e.g. timestamps, indices).
    //
    // Requires arithmetic + trivially copyable.
    template<typename V = value_type>
    typename std::enable_if<
        std::is_trivially_copyable_v<V> &&
        std::is_arithmetic_v<V>,
        std::vector<char>
    >::type
    encode_delta() const
    {
        std::size_t n       = self().size();
        std::size_t payload = n * sizeof(V);
        std::size_t hdr_sz  =
            binary::D_BINARY_HEADER_SIZE;

        std::vector<char> out(hdr_sz + payload);

        auto h = make_container_header<_Derived>(
            n, payload);

        h.flags |=
            binary::binary_flags::compressed;

        binary::write_header(
            out.data(), out.size(), h);

        const V* p = self().data();
        char* cursor = out.data() + hdr_sz;

        if (n > 0)
        {
            // first element stored as-is
            std::memcpy(cursor, &p[0], sizeof(V));
            cursor += sizeof(V);

            // subsequent: store delta
            for (std::size_t i = 1; i < n; ++i)
            {
                V delta =
                    static_cast<V>(p[i] - p[i - 1]);

                std::memcpy(
                    cursor, &delta, sizeof(V));

                cursor += sizeof(V);
            }
        }

        return out;
    }

    // --- byte size query ---

    // encoded_size
    //   returns the exact byte count that encode()
    // would produce, without allocating.
    template<typename V = value_type>
    constexpr typename std::enable_if<
        std::is_trivially_copyable_v<V>,
        std::size_t
    >::type
    encoded_size() const noexcept
    {
        return binary::D_BINARY_HEADER_SIZE +
               (self().size() * sizeof(V));
    }

    // encoded_raw_size
    //   raw payload size (no header).
    template<typename V = value_type>
    constexpr typename std::enable_if<
        std::is_trivially_copyable_v<V>,
        std::size_t
    >::type
    encoded_raw_size() const noexcept
    {
        return self().size() * sizeof(V);
    }

    // --- encode into buffer ---

    // encode_into
    //   encodes directly into a caller-provided buffer.
    // Returns bytes written, or 0 on insufficient space.
    // No allocation.
    template<typename V = value_type>
    typename std::enable_if<
        std::is_trivially_copyable_v<V>,
        std::size_t
    >::type
    encode_into(char*       _buf,
                std::size_t _capacity) const noexcept
    {
        std::size_t n       = self().size();
        std::size_t payload = n * sizeof(V);
        std::size_t hdr_sz  =
            binary::D_BINARY_HEADER_SIZE;
        std::size_t total   = hdr_sz + payload;

        if (_capacity < total)
        {
            return 0;
        }

        auto h = make_container_header<_Derived>(
            n, payload);

        binary::write_header(_buf, _capacity, h);

        std::memcpy(
            _buf + hdr_sz,
            self().data(),
            payload);

        return total;
    }

    // encode_raw_into
    //   encodes raw payload (no header) into buffer.
    template<typename V = value_type>
    typename std::enable_if<
        std::is_trivially_copyable_v<V>,
        std::size_t
    >::type
    encode_raw_into(
        char*       _buf,
        std::size_t _capacity) const noexcept
    {
        std::size_t payload =
            self().size() * sizeof(V);

        if (_capacity < payload)
        {
            return 0;
        }

        std::memcpy(
            _buf,
            self().data(),
            payload);

        return payload;
    }
};


// =============================================================================
// II.  array_binary_mutable (CRTP) — decode-into layer
// =============================================================================
// Mutating operations that decode binary data into the
// container's existing storage.

template<typename _Derived>
class array_binary_mutable
    : public array_binary_base<_Derived>
{
protected:
    array_binary_mutable()  = default;
    ~array_binary_mutable() = default;

private:
    _Derived& self()
    {
        return static_cast<_Derived&>(*this);
    }

    const _Derived& self() const
    {
        return static_cast<
            const _Derived&>(*this);
    }

    using value_type =
        typename _Derived::value_type;

public:
    // --- decode from blob (with header) ---

    // decode_from
    //   decodes a binary blob (with header) into this
    // container, replacing its contents.  For dynamic
    // containers this resizes; for fixed containers
    // it reads up to capacity.
    // Returns the number of elements decoded.
    template<typename V = value_type>
    typename std::enable_if<
        std::is_trivially_copyable_v<V>,
        std::size_t
    >::type
    decode_from(const char*  _data,
                std::size_t  _size)
    {
        binary::binary_header hdr;

        if (!binary::read_header(
                _data, _size, hdr))
        {
            return 0;
        }

        const char* payload =
            _data +
            binary::D_BINARY_HEADER_SIZE;

        std::size_t payload_sz =
            static_cast<std::size_t>(
                hdr.payload_size);

        std::size_t count =
            static_cast<std::size_t>(
                hdr.count);

        return decode_raw_from(
            payload, payload_sz, count);
    }

    // decode_from (vector overload)
    template<typename V = value_type>
    typename std::enable_if<
        std::is_trivially_copyable_v<V>,
        std::size_t
    >::type
    decode_from(const std::vector<char>& _blob)
    {
        return decode_from(
            _blob.data(), _blob.size());
    }

    // --- decode from raw payload ---

    // decode_raw_from
    //   decodes a headerless raw payload into this
    // container.  _count is the number of elements
    // (if 0, inferred from _size / sizeof(V)).
    template<typename V = value_type>
    typename std::enable_if<
        std::is_trivially_copyable_v<V>,
        std::size_t
    >::type
    decode_raw_from(
        const char*  _data,
        std::size_t  _size,
        std::size_t  _count = 0)
    {
        if (_count == 0)
        {
            _count = _size / sizeof(V);
        }

        std::size_t bytes = _count * sizeof(V);

        if (bytes > _size)
        {
            _count = _size / sizeof(V);
            bytes  = _count * sizeof(V);
        }

        // try resize for dynamic containers
        resize_if_possible(_count);

        // clamp to actual capacity
        std::size_t actual =
            (_count < self().size())
                ? _count : self().size();

        std::memcpy(
            self().data(),
            _data,
            actual * sizeof(V));

        return actual;
    }

    // --- delta decode ---

    // decode_delta_from
    //   decodes a delta-encoded blob.  First element
    // is stored as-is; subsequent elements are
    // accumulated: elem[i] = elem[i-1] + delta[i].
    template<typename V = value_type>
    typename std::enable_if<
        std::is_trivially_copyable_v<V> &&
        std::is_arithmetic_v<V>,
        std::size_t
    >::type
    decode_delta_from(
        const char*  _data,
        std::size_t  _size)
    {
        binary::binary_header hdr;

        if (!binary::read_header(
                _data, _size, hdr))
        {
            return 0;
        }

        const char* payload =
            _data +
            binary::D_BINARY_HEADER_SIZE;

        std::size_t count =
            static_cast<std::size_t>(hdr.count);

        resize_if_possible(count);

        std::size_t actual =
            (count < self().size())
                ? count : self().size();

        V* p = self().data();

        if (actual > 0)
        {
            // first element
            std::memcpy(
                &p[0], payload, sizeof(V));

            const char* cursor =
                payload + sizeof(V);

            // accumulate deltas
            for (std::size_t i = 1;
                 i < actual; ++i)
            {
                V delta;

                std::memcpy(
                    &delta, cursor, sizeof(V));

                p[i] = static_cast<V>(
                    p[i - 1] + delta);

                cursor += sizeof(V);
            }
        }

        return actual;
    }

    // --- encode-into-buffer from existing data ---
    // (inherited from array_binary_base)

private:
    // resize_if_possible
    //   attempts to resize the container if it
    // supports resize().  SFINAE fall is a no-op
    // for fixed-size containers.

    template<typename D = _Derived>
    auto resize_if_possible(std::size_t _n)
        -> decltype(
            std::declval<D&>().resize(_n), void())
    {
        self().resize(_n);
    }

    void resize_if_possible(...) noexcept
    {
        // no-op for fixed-size containers
    }
};


// =============================================================================
// III. Free-Function Array Encode
// =============================================================================
// Non-member encode functions for any contiguous array.

// array_binary_encode
//   encodes a contiguous array with header via data()
// memcpy.  Single allocation, single copy.
template<typename _Container>
inline typename std::enable_if<
    ( traits::is_contiguous_array_v<_Container> &&
      traits::has_trivially_copyable_elements_v<
          _Container> ),
    std::vector<char>
>::type
array_binary_encode(const _Container& _src)
{
    using V = typename _Container::value_type;

    std::size_t n       = _src.size();
    std::size_t payload = n * sizeof(V);
    std::size_t hdr_sz  =
        binary::D_BINARY_HEADER_SIZE;

    std::vector<char> out(hdr_sz + payload);

    auto h = make_container_header<_Container>(
        n, payload);

    binary::write_header(
        out.data(), out.size(), h);

    std::memcpy(
        out.data() + hdr_sz,
        _src.data(),
        payload);

    return out;
}

// array_binary_encode_raw
//   raw payload only (no header).
template<typename _Container>
inline typename std::enable_if<
    ( traits::is_contiguous_array_v<_Container> &&
      traits::has_trivially_copyable_elements_v<
          _Container> ),
    std::vector<char>
>::type
array_binary_encode_raw(const _Container& _src)
{
    using V = typename _Container::value_type;

    std::size_t payload =
        _src.size() * sizeof(V);

    std::vector<char> out(payload);

    std::memcpy(
        out.data(),
        _src.data(),
        payload);

    return out;
}

// array_binary_encode_subarray
//   encodes [_offset, _offset + _count) with header.
template<typename _Container>
inline typename std::enable_if<
    ( traits::is_contiguous_array_v<_Container> &&
      traits::has_trivially_copyable_elements_v<
          _Container> ),
    std::vector<char>
>::type
array_binary_encode_subarray(
    const _Container& _src,
    std::size_t       _offset,
    std::size_t       _count)
{
    using V = typename _Container::value_type;

    std::size_t sz = _src.size();

    std::size_t actual_offset =
        (_offset < sz) ? _offset : sz;

    std::size_t remaining = sz - actual_offset;

    std::size_t actual_count =
        (_count < remaining)
            ? _count : remaining;

    std::size_t payload =
        actual_count * sizeof(V);

    std::size_t hdr_sz =
        binary::D_BINARY_HEADER_SIZE;

    std::vector<char> out(hdr_sz + payload);

    auto h = make_container_header<_Container>(
        actual_count, payload);

    binary::write_header(
        out.data(), out.size(), h);

    std::memcpy(
        out.data() + hdr_sz,
        _src.data() + actual_offset,
        payload);

    return out;
}

// array_binary_encode_into
//   encodes into a caller-provided buffer.
// Returns bytes written, or 0 on insufficient space.
template<typename _Container>
inline typename std::enable_if<
    ( traits::is_contiguous_array_v<_Container> &&
      traits::has_trivially_copyable_elements_v<
          _Container> ),
    std::size_t
>::type
array_binary_encode_into(
    const _Container& _src,
    char*             _buf,
    std::size_t       _capacity)
{
    using V = typename _Container::value_type;

    std::size_t n       = _src.size();
    std::size_t payload = n * sizeof(V);
    std::size_t hdr_sz  =
        binary::D_BINARY_HEADER_SIZE;
    std::size_t total   = hdr_sz + payload;

    if (_capacity < total)
    {
        return 0;
    }

    auto h = make_container_header<_Container>(
        n, payload);

    binary::write_header(_buf, _capacity, h);

    std::memcpy(
        _buf + hdr_sz,
        _src.data(),
        payload);

    return total;
}


// =============================================================================
// IV.  Free-Function Array Decode
// =============================================================================

// array_binary_decode
//   decodes a binary blob (with header) into a new
// container.
template<typename _Container>
inline typename std::enable_if<
    ( traits::is_contiguous_array_v<_Container> &&
      traits::has_trivially_copyable_elements_v<
          _Container> ),
    _Container
>::type
array_binary_decode(
    const char*  _data,
    std::size_t  _size)
{
    using V = typename _Container::value_type;

    binary::binary_header hdr;

    if (!binary::read_header(_data, _size, hdr))
    {
        return _Container{};
    }

    const char* payload =
        _data + binary::D_BINARY_HEADER_SIZE;

    std::size_t count =
        static_cast<std::size_t>(hdr.count);

    _Container result;

    // resize for dynamic containers (SFINAE)
    internal::try_resize(result, count);

    std::size_t actual =
        (count < result.size())
            ? count : result.size();

    std::memcpy(
        result.data(),
        payload,
        actual * sizeof(V));

    return result;
}

// array_binary_decode (vector overload)
template<typename _Container>
inline typename std::enable_if<
    ( traits::is_contiguous_array_v<_Container> &&
      traits::has_trivially_copyable_elements_v<
          _Container> ),
    _Container
>::type
array_binary_decode(
    const std::vector<char>& _blob)
{
    return array_binary_decode<_Container>(
        _blob.data(), _blob.size());
}

// array_binary_decode_raw
//   decodes a headerless raw payload.
template<typename _Container>
inline typename std::enable_if<
    ( traits::is_contiguous_array_v<_Container> &&
      traits::has_trivially_copyable_elements_v<
          _Container> ),
    _Container
>::type
array_binary_decode_raw(
    const char*  _data,
    std::size_t  _size)
{
    using V = typename _Container::value_type;

    std::size_t count = _size / sizeof(V);

    _Container result;

    internal::try_resize(result, count);

    std::size_t actual =
        (count < result.size())
            ? count : result.size();

    std::memcpy(
        result.data(),
        _data,
        actual * sizeof(V));

    return result;
}

// array_binary_decode_into
//   decodes directly into existing container storage.
// Returns element count decoded.
template<typename _Container>
inline typename std::enable_if<
    ( traits::is_contiguous_array_v<_Container> &&
      traits::has_trivially_copyable_elements_v<
          _Container> ),
    std::size_t
>::type
array_binary_decode_into(
    _Container&  _dst,
    const char*  _data,
    std::size_t  _size)
{
    using V = typename _Container::value_type;

    binary::binary_header hdr;

    if (!binary::read_header(_data, _size, hdr))
    {
        return 0;
    }

    const char* payload =
        _data + binary::D_BINARY_HEADER_SIZE;

    std::size_t count =
        static_cast<std::size_t>(hdr.count);

    internal::try_resize(_dst, count);

    std::size_t actual =
        (count < _dst.size())
            ? count : _dst.size();

    std::memcpy(
        _dst.data(),
        payload,
        actual * sizeof(V));

    return actual;
}


// =============================================================================
// V.   Free-Function Delta Encode/Decode
// =============================================================================
// For sorted integer arrays: stores differences between
// adjacent elements for compact representation.

// array_binary_delta_encode
template<typename _Container>
inline typename std::enable_if<
    ( traits::is_contiguous_array_v<_Container>  &&
      traits::has_trivially_copyable_elements_v<
          _Container>                            &&
      std::is_arithmetic_v<
          typename _Container::value_type> ),
    std::vector<char>
>::type
array_binary_delta_encode(
    const _Container& _src)
{
    using V = typename _Container::value_type;

    std::size_t n       = _src.size();
    std::size_t payload = n * sizeof(V);
    std::size_t hdr_sz  =
        binary::D_BINARY_HEADER_SIZE;

    std::vector<char> out(hdr_sz + payload);

    auto h = make_container_header<_Container>(
        n, payload);

    h.flags |=
        binary::binary_flags::compressed;

    binary::write_header(
        out.data(), out.size(), h);

    const V* p = _src.data();
    char* cursor = out.data() + hdr_sz;

    if (n > 0)
    {
        std::memcpy(cursor, &p[0], sizeof(V));
        cursor += sizeof(V);

        for (std::size_t i = 1; i < n; ++i)
        {
            V delta =
                static_cast<V>(p[i] - p[i - 1]);

            std::memcpy(
                cursor, &delta, sizeof(V));

            cursor += sizeof(V);
        }
    }

    return out;
}

// array_binary_delta_decode
template<typename _Container>
inline typename std::enable_if<
    ( traits::is_contiguous_array_v<_Container>  &&
      traits::has_trivially_copyable_elements_v<
          _Container>                            &&
      std::is_arithmetic_v<
          typename _Container::value_type> ),
    _Container
>::type
array_binary_delta_decode(
    const char*  _data,
    std::size_t  _size)
{
    using V = typename _Container::value_type;

    binary::binary_header hdr;

    if (!binary::read_header(_data, _size, hdr))
    {
        return _Container{};
    }

    const char* payload =
        _data + binary::D_BINARY_HEADER_SIZE;

    std::size_t count =
        static_cast<std::size_t>(hdr.count);

    _Container result;

    internal::try_resize(result, count);

    std::size_t actual =
        (count < result.size())
            ? count : result.size();

    V* p = result.data();

    if (actual > 0)
    {
        std::memcpy(
            &p[0], payload, sizeof(V));

        const char* cursor =
            payload + sizeof(V);

        for (std::size_t i = 1;
             i < actual; ++i)
        {
            V delta;

            std::memcpy(
                &delta, cursor, sizeof(V));

            p[i] = static_cast<V>(
                p[i - 1] + delta);

            cursor += sizeof(V);
        }
    }

    return result;
}


// =============================================================================
// VI.  Free-Function Circular Encode
// =============================================================================
// Encodes a circular buffer by writing elements in
// logical order (head → tail) as a flat sequence.
// The decode side produces a standard (non-circular)
// flat array.

// array_binary_circular_encode
//   SFINAE-gated on is_circular_buffer_v.
template<typename _Container>
inline typename std::enable_if<
    ( traits::is_contiguous_array_v<_Container> &&
      traits::is_circular_buffer_v<_Container>  &&
      traits::has_trivially_copyable_elements_v<
          _Container> ),
    std::vector<char>
>::type
array_binary_circular_encode(
    const _Container& _src)
{
    using V = typename _Container::value_type;

    std::size_t n       = _src.size();
    std::size_t payload = n * sizeof(V);
    std::size_t hdr_sz  =
        binary::D_BINARY_HEADER_SIZE;

    std::vector<char> out(hdr_sz + payload);

    auto h = make_container_header<_Container>(
        n, payload);

    binary::write_header(
        out.data(), out.size(), h);

    // write in logical order via head/capacity
    const V*    base = _src.data();
    std::size_t cap  = _src.capacity();
    std::size_t head = _src.head();
    char*       cursor = out.data() + hdr_sz;

    for (std::size_t i = 0; i < n; ++i)
    {
        std::size_t phys =
            (head + i) % cap;

        std::memcpy(
            cursor,
            &base[phys],
            sizeof(V));

        cursor += sizeof(V);
    }

    return out;
}


// =============================================================================
// internal: try_resize helper
// =============================================================================
// SFINAE helper used by decode functions.

NS_INTERNAL

    template<typename _C>
    auto try_resize(_C& _c, std::size_t _n)
        -> decltype(_c.resize(_n), void())
    {
        _c.resize(_n);
    }

    inline void try_resize(...) noexcept
    {
        // no-op for fixed-size containers
    }

NS_END  // internal


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_ARRAY_BINARY_
