/******************************************************************************
* djinterp [container]                                         byte_buffer.hpp
*
* Concrete byte buffer for the djinterp container framework.
*   A byte buffer is a growable, staged accumulator for raw binary data -
* the exact counterpart of text_buffer.hpp on the binary side.  Where a
* text buffer bridges buffer_base to the TEXT layer (to_text, c_str,
* ostream), a byte buffer bridges buffer_base to the SERIALIZATION layer
* (encode.hpp / decode.hpp).
*
*   Two identities, held at once (containers.tex, Serialization):
*
*     A CONTAINER OF BYTES.  Its value type is `byte`, and it satisfies
*       the intrinsic container axes - a runtime-lived, dynamically-stored,
*       read-write SEQUENCE (positions matter, duplicates allowed) whose
*       element is a flat leaf.  It exposes the full structural protocol:
*       begin/end, value_type, data, size, capacity, operator[], at,
*       front/back, iterators.
*
*     THE MEDIUM B*.  A byte buffer IS the "flat run of bits ... grouped
*       into bytes" a value is folded into and later read back from.  It is
*       therefore a valid ENCODE SINK (it exposes `push_back(byte)`, the sole
*       requirement encode.hpp places on a sink), so `encode_into(buf, v)`
*       and `encode_container_into(buf, c)` append directly into it; and it
*       yields a byte_reader (`reader()`) that drives the DECODER.
*
*   The buffer keeps NO null terminator (unlike text_buffer): binary data is
* opaque and self-measured by size(), so raw capacity and usable capacity
* coincide.  This keeps grow() and the accessors simpler than the text case.
*
*   Growth and cursor policies are inherited from buffer.hpp via buffer_base.
* The default configuration is exponential growth with a WRITE-ONLY cursor -
* the "encode, then hand off / extract" workflow, matching text_buffer's
* default.  A producer/consumer configuration (encode in, decode out) is the
* `byte_stream` alias below, or any instantiation with dual_cursor_policy;
* the read-cursor operations (get<T>, advance, ...) light up only then.
*
* SERIALIZATION CONTRACT:
*   put<T>(v)             - encode a leaf value into the buffer (this is a
*                           sink, so `encode_into(*this, v)` appends it).
*                           Transparently lifts to whole-container encoding
*                           when container_encode.hpp is also in scope, since
*                           it is a template call on `encode_into`.
*   get<T>()              - decode a leaf value from the read cursor,
*                           advancing it on success (dual-cursor only).
*   reader()              - a detached byte_reader over the readable region;
*                           the general decode entry (pair with the container
*                           decoder for nested shapes).
*   to_byte_string()      - the readable region as a byte_string (the model's
*                           B* as a value); `bytes()` is an alias.
*   stream_to(buf, cap)   - copy up to `cap` bytes out without consuming.
*   take()                - to_byte_string() followed by reset() (a drain).
*
* TRAIT SATISFACTION:
*   is_iterable_container_v      -  yes (begin/end, value_type)
*   has_data_accessor_v          -  yes (data)
*   has_size_accessor_v          -  yes (size)
*   has_capacity_accessor_v      -  yes (capacity)
*   has_push__v                  -  yes (push_)
*   has_clear_v                  -  yes (clear)
*   is_ostream_insertable_v      -  yes (operator<<, raw bytes)
*
*   value is a leaf: is_leaf_encodable_v<byte>       -> true
*                    is_leaf_decodable_v<byte>       -> true
*   whole buffer:    is_encodable_container_v        -> true
*                      (iterable container of a leaf; encodes as a
*                       length prefix followed by its bytes, exactly as a
*                       std::vector<byte> does - container_encode.hpp)
*
* DEPENDENCIES:
*   djinterp.hpp    - namespace macros, clean_t, feature macros
*   buffer.hpp      - buffer_base (CRTP), growth/cursor policies
*   encode.hpp      - byte, byte_string, encode_into (sink), enc length type
*   decode.hpp      - byte_reader, decode_result, decode<T>
*
*
* path:      /inc/djinterp/core/container/buffer/byte_buffer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.18
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.   byte_buffer Class
       a.   Type Aliases and CRTP Contract
       b.   Construction and Destruction
       c.   Move and Copy
       d.   Byte Append Operations
       e.   Serialization Contract (put / get / reader)
       f.   Byte Extraction
       g.   Container Protocol
       h.   Ostream Integration
II.  Factory Functions and Aliases
*/

#ifndef DJINTERP_CONTAINER_BYTE_BUFFER_
#define DJINTERP_CONTAINER_BYTE_BUFFER_ 1

// std
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <new>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "./buffer.hpp"
#include "../../binary/encode.hpp"   // byte, byte_string, encode_into, encode_length_type
#include "../../binary/decode.hpp"   // byte_reader, decode_result, decode<T>


NS_DJINTERP


// =============================================================================
// I.   byte_buffer Class
// =============================================================================
template<typename _GrowthPolicy = default_growth_policy,
         typename _CursorPolicy = write_only_cursor_policy>
class byte_buffer : public buffer_base<byte_buffer<_GrowthPolicy,
                                                    _CursorPolicy>,
                                       _GrowthPolicy,
                                       _CursorPolicy>
{
private:
    using base_type = buffer_base<byte_buffer<_GrowthPolicy, _CursorPolicy>,
                                  _GrowthPolicy,
                                  _CursorPolicy>;

    // buffer_base accesses storage()/capacity()/grow()
    friend base_type;

public:
    // =========================================================================
    // I.a  Type Aliases and CRTP Contract
    // =========================================================================
    // The element is `byte` (encode.hpp's octet, an unsigned char); storage
    // beneath is char (buffer_base's medium), reinterpreted at the boundary -
    // char and unsigned char alias each other, so this is well-defined.

    using value_type             = byte;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using reference              = byte&;
    using const_reference        = const byte&;
    using pointer                = byte*;
    using const_pointer          = const byte*;
    using iterator               = byte*;
    using const_iterator         = const byte*;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;


    // =========================================================================
    // I.b  Construction and Destruction
    // =========================================================================

    // default constructor
    //   creates an empty byte buffer with no allocation.
    // The first append triggers the growth policy.
    byte_buffer() noexcept
        : base_type(),
          m_data(nullptr),
          m_capacity(0)
    {}

    // capacity constructor
    //   creates an empty byte buffer pre-allocated to hold
    // at least _initial_capacity bytes.
    explicit
    byte_buffer(
        size_type _initial_capacity
    )
        : base_type(),
          m_data(nullptr),
          m_capacity(0)
    {
        if (_initial_capacity > 0)
        {
            grow(_initial_capacity);
        }
    }

    // raw-range constructor
    //   creates a byte buffer initialized with _len bytes
    // copied from _data.
    byte_buffer(
        const byte* _data,
        size_type   _len
    )
        : base_type(),
          m_data(nullptr),
          m_capacity(0)
    {
        if ( (_data) && (_len > 0) )
        {
            append(_data, _len);
        }
    }

    // byte_string constructor
    //   creates a byte buffer initialized from a byte_string
    // (the medium B* as a value).
    explicit
    byte_buffer(
        const byte_string& _bytes
    )
        : base_type(),
          m_data(nullptr),
          m_capacity(0)
    {
        if (!_bytes.empty())
        {
            append(_bytes.data(), _bytes.size());
        }
    }

    // destructor
    ~byte_buffer() noexcept
    {
        if (m_data)
        {
            delete[] m_data;
            m_data = nullptr;
        }
    }


    // =========================================================================
    // I.c  Move and Copy
    // =========================================================================

    // move constructor
    byte_buffer(
        byte_buffer&& _other
    ) noexcept
        : base_type(static_cast<base_type&&>(_other)),
          m_data(_other.m_data),
          m_capacity(_other.m_capacity)
    {
        _other.m_data     = nullptr;
        _other.m_capacity = 0;
    }

    // move assignment
    byte_buffer& operator=(byte_buffer&& _other) noexcept
    {
        if (this != &_other)
        {
            // free existing storage
            if (m_data)
            {
                delete[] m_data;
            }

            // move base cursor state
            base_type::operator=(
                static_cast<base_type&&>(_other));

            // take ownership
            m_data     = _other.m_data;
            m_capacity = _other.m_capacity;

            _other.m_data     = nullptr;
            _other.m_capacity = 0;
        }

        return *this;
    }

    // copy constructor
    //   deep-copies the readable region of _other to the
    // front of fresh storage (a copy compacts, dropping any
    // already-consumed prefix in dual-cursor mode).
    byte_buffer(const byte_buffer& _other)
        : base_type(),
          m_data(nullptr),
          m_capacity(0)
    {
        if (_other.size() > 0)
        {
            size_type _sz = _other.size();

            grow(_sz);

            std::memcpy(m_data, _other.data(), _sz);

            // fresh cursors: content at [0, sz)
            _CursorPolicy::reset(this->m_cursors);
            _CursorPolicy::advance_write(this->m_cursors, _sz);
        }
    }

    // copy assignment
    byte_buffer& operator=(const byte_buffer& _other)
    {
        if (this != &_other)
        {
            this->reset();

            if (_other.size() > 0)
            {
                size_type _sz = _other.size();

                if (this->capacity() < _sz)
                {
                    grow(_sz);
                }

                std::memcpy(m_data, _other.data(), _sz);

                _CursorPolicy::reset(this->m_cursors);
                _CursorPolicy::advance_write(this->m_cursors, _sz);
            }
        }

        return *this;
    }


    // =========================================================================
    // I.d  Byte Append Operations
    // =========================================================================
    // All appends grow the buffer if necessary and permitted,
    // and return the number of bytes actually appended (which
    // may be short of the request on a fixed, full buffer).

    // append (raw range)
    //   appends _len bytes from _data.
    size_type
    append(const byte* _data,
           size_type   _len) noexcept
    {
        if ( (!_data) || (_len == 0) )
        {
            return 0;
        }

        if (!this->ensure_writable(_len))
        {
            // partial write: fit what we can
            _len = this->writable();

            if (_len == 0)
            {
                return 0;
            }
        }

        std::memcpy(this->write_head(), _data, _len);

        this->commit(_len);

        return _len;
    }

    // append (byte_string)
    //   appends the contents of a byte_string.
    size_type
    append(const byte_string& _bytes) noexcept
    {
        return append(_bytes.data(), _bytes.size());
    }

    // append (single byte)
    //   appends one byte.  Returns 1 on success, 0 on failure.
    size_type
    append(byte _b) noexcept
    {
        return append(&_b, 1);
    }

    // append (another byte_buffer)
    //   appends the readable region of _other.  Safe against
    // self-append (a snapshot is taken when _other is *this).
    size_type
    append(const byte_buffer& _other)
    {
        if (&_other == this)
        {
            byte_string _snapshot = _other.to_byte_string();

            return append(_snapshot.data(), _snapshot.size());
        }

        return append(_other.data(), _other.size());
    }

    // append_fill
    //   appends _count copies of _b.  Returns the number of
    // bytes actually appended.
    size_type
    append_fill(byte      _b,
                size_type _count) noexcept
    {
        if (_count == 0)
        {
            return 0;
        }

        if (!this->ensure_writable(_count))
        {
            _count = this->writable();

            if (_count == 0)
            {
                return 0;
            }
        }

        std::memset(this->write_head(),
                    static_cast<int>(_b),
                    _count);

        this->commit(_count);

        return _count;
    }

    // push_back
    //   appends one byte.  This is the customization point
    // encode.hpp's sink probe looks for (has_push_back_byte),
    // so a byte_buffer is a valid ENCODE SINK.
    void push_back(byte _b) noexcept
    {
        append(&_b, 1);

        return;
    }

    // push_
    //   appends one byte.  Satisfies the container push_
    // requirement detected by has_push__v.
    void push_(byte _b)
    {
        append(&_b, 1);

        return;
    }


    // =========================================================================
    // I.e  Serialization Contract (put / get / reader)
    // =========================================================================
    // The bridge to the binary layer.  put<T>/get<T> are the
    // convenience for LEAF values (the built-in scalar leaves
    // and any type with a member encode_into / static decode);
    // whole-container serialization composes on top through the
    // sink (encode_container_into) and reader() entry points.

    // put
    //   encodes a leaf value into the buffer and returns *this
    // for chaining.  Because the buffer is a sink, this is
    // exactly encode.hpp's enc_tau writing into it; it lifts to
    // container encoding automatically when container_encode.hpp
    // is in scope, since the call resolves encode_into anew at
    // instantiation.
    template<typename _Type>
    byte_buffer&
    put(const _Type& _value)
    {
        encode_into(*this, _value);

        return *this;
    }

    // reader
    //   returns a detached byte_reader over the readable region.
    // It does NOT advance this buffer's cursor; the buffer must
    // outlive the reader and must not reallocate (append/grow)
    // while the reader is in use.  This is the general decode
    // entry - pair it with the container decoder for nested
    // shapes.
    byte_reader reader() const noexcept
    {
        return byte_reader(this->data(), this->size());
    }

    // get
    //   decodes one leaf value of _Type from the read cursor.
    // On SUCCESS the read cursor advances past the consumed
    // bytes; on failure the cursor is left unchanged so the
    // caller may inspect or retry.  Available only under a
    // cursor policy with a read cursor (dual_cursor_policy).
    template<typename _Type,
             typename _CP = _CursorPolicy>
    std::enable_if_t<_CP::has_read_cursor,
                     decode_result<clean_t<_Type>>>
    get()
    {
        byte_reader _rd(this->data(), this->size());

        const size_type _before = _rd.remaining();

        decode_result<clean_t<_Type>> _result =
            decode<_Type>(_rd);

        if (_result.ok)
        {
            const size_type _consumed = _before - _rd.remaining();

            this->advance(_consumed);
        }

        return _result;
    }


    // =========================================================================
    // I.f  Byte Extraction
    // =========================================================================

    // to_byte_string
    //   returns the readable region as a byte_string - the
    // model's medium B* materialized as a value.
    byte_string to_byte_string() const
    {
        if (this->size() == 0)
        {
            return byte_string();
        }

        const byte* _p = this->data();

        return byte_string(_p, _p + this->size());
    }

    // bytes
    //   alias for to_byte_string().
    byte_string bytes() const
    {
        return to_byte_string();
    }

    // stream_to
    //   copies as many bytes as fit into _buf (up to _cap
    // bytes) and returns the number copied.  Does not advance
    // any cursor.
    std::size_t
    stream_to(byte*       _buf,
              std::size_t _cap) const noexcept
    {
        if ( (!_buf) || (_cap == 0) )
        {
            return 0;
        }

        std::size_t _n = std::min(this->size(), _cap);

        std::memcpy(_buf, this->data(), _n);

        return _n;
    }

    // take
    //   returns the readable region as a byte_string and then
    // resets the buffer to empty (storage retained) - a drain,
    // fitting the buffer's write-then-consume temporality.
    // This is a copy followed by reset(), not a cheap move: the
    // underlying storage is a raw byte array, not a byte_string.
    byte_string take()
    {
        byte_string _out = to_byte_string();

        this->reset();

        return _out;
    }


    // =========================================================================
    // I.g  Container Protocol
    // =========================================================================
    // The structural interface detected by container_traits.hpp:
    // iterators, positional access, size, capacity, data.  All
    // accessors operate over the readable content region, which
    // differs by cursor model:
    //   write_only: [0, write_pos)
    //   dual:       [read_pos, write_pos)

    // --- iteration ---

    iterator begin() noexcept
    {
        return content_begin_mut_();
    }

    iterator end() noexcept
    {
        return m_data
            ? reinterpret_cast<byte*>(m_data) + this->write_position()
            : nullptr;
    }

    const_iterator begin() const noexcept
    {
        return content_begin_();
    }

    const_iterator end() const noexcept
    {
        return m_data
            ? reinterpret_cast<const byte*>(m_data) + this->write_position()
            : nullptr;
    }

    const_iterator cbegin() const noexcept
    {
        return begin();
    }

    const_iterator cend() const noexcept
    {
        return end();
    }

    // --- reverse iteration ---

    reverse_iterator rbegin() noexcept
    {
        return reverse_iterator(end());
    }

    reverse_iterator rend() noexcept
    {
        return reverse_iterator(begin());
    }

    const_reverse_iterator rbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }

    const_reverse_iterator rend() const noexcept
    {
        return const_reverse_iterator(begin());
    }

    const_reverse_iterator crbegin() const noexcept
    {
        return rbegin();
    }

    const_reverse_iterator crend() const noexcept
    {
        return rend();
    }

    // --- positional access ---
    // Indices are relative to the start of the readable region
    // (operator[](0) is the first unconsumed byte in dual mode).

    reference operator[](size_type _pos) noexcept
    {
        return content_begin_mut_()[_pos];
    }

    const_reference
    operator[](size_type _pos) const noexcept
    {
        return content_begin_()[_pos];
    }

    reference at(size_type _pos)
    {
        if (_pos >= this->size())
        {
            throw std::out_of_range(
                "byte_buffer::at: index out of range");
        }

        return content_begin_mut_()[_pos];
    }

    const_reference at(size_type _pos) const
    {
        if (_pos >= this->size())
        {
            throw std::out_of_range(
                "byte_buffer::at: index out of range");
        }

        return content_begin_()[_pos];
    }

    reference front() noexcept
    {
        return *content_begin_mut_();
    }

    const_reference front() const noexcept
    {
        return *content_begin_();
    }

    reference back() noexcept
    {
        return reinterpret_cast<byte*>(
            m_data)[this->write_position() - 1];
    }

    const_reference back() const noexcept
    {
        return reinterpret_cast<const byte*>(
            m_data)[this->write_position() - 1];
    }

    // --- data access ---
    // data() returns a pointer to the start of the readable
    // content region, consistent with begin() and size().

    const byte* data() const noexcept
    {
        return content_begin_();
    }

    byte* data() noexcept
    {
        return content_begin_mut_();
    }

    // capacity
    //   usable capacity in bytes.  For a byte buffer there is
    // no reserved terminator, so this is the full allocation.
    // Public: also the CRTP capacity() buffer_base queries.
    size_type capacity() const noexcept
    {
        return m_capacity;
    }

    size_type max_size() const noexcept
    {
        return std::numeric_limits<size_type>::max();
    }

    // clear
    //   resets the buffer to empty and zeroes storage.
    // Satisfies has_clear_v.
    void clear() noexcept
    {
        base_type::clear();

        return;
    }

    // swap
    //   exchanges contents with another byte_buffer.
    void swap(byte_buffer& _other) noexcept
    {
        if (this == &_other)
        {
            return;
        }

        // swap cursor state
        auto      _tmp_cursors = this->m_cursors;
        this->m_cursors        = _other.m_cursors;
        _other.m_cursors       = _tmp_cursors;

        // swap storage
        char*     _tmp_data = m_data;
        size_type _tmp_cap  = m_capacity;

        m_data            = _other.m_data;
        m_capacity        = _other.m_capacity;

        _other.m_data     = _tmp_data;
        _other.m_capacity = _tmp_cap;

        return;
    }


    // =========================================================================
    // I.h  Ostream Integration
    // =========================================================================
    // Satisfies is_ostream_insertable_v via a friend operator<<.
    // The readable region is written as RAW BYTES; intended for
    // binary ostreams (a text stream will see arbitrary octets).

    template<typename _GP, typename _CP>
    friend std::ostream&
    operator<<(std::ostream&                _os,
               const byte_buffer<_GP, _CP>& _buf);


private:
    // --- CRTP contract (accessed by buffer_base) ---

    char* storage() noexcept
    {
        return m_data;
    }

    const char* storage() const noexcept
    {
        return m_data;
    }

    // --- content region helpers ---
    // Start of the readable content region, as a byte pointer.
    //   write_only: m_data
    //   dual:       m_data + read_pos

    const byte* content_begin_() const noexcept
    {
        if (!m_data)
        {
            return nullptr;
        }

        if constexpr (_CursorPolicy::has_read_cursor)
        {
            return reinterpret_cast<const byte*>(
                m_data + this->m_cursors.read_pos);
        }
        else
        {
            return reinterpret_cast<const byte*>(m_data);
        }
    }

    byte* content_begin_mut_() noexcept
    {
        if (!m_data)
        {
            return nullptr;
        }

        if constexpr (_CursorPolicy::has_read_cursor)
        {
            return reinterpret_cast<byte*>(
                m_data + this->m_cursors.read_pos);
        }
        else
        {
            return reinterpret_cast<byte*>(m_data);
        }
    }

    // grow
    //   reallocates so that usable capacity is at least
    // _new_capacity bytes.  No terminator is reserved (binary
    // data is self-measured), so the allocation is exactly
    // _new_capacity.  Preserves the written region [0, write_pos)
    // so both cursors remain valid.  Returns true on success.
    bool grow(size_type _new_capacity) noexcept
    {
        // already have enough
        if (_new_capacity <= m_capacity)
        {
            return true;
        }

        char* _new_data = nullptr;

        try
        {
            _new_data = new char[_new_capacity];
        }
        catch (const std::bad_alloc&)
        {
            return false;
        }

        // copy the written high-water region, preserving the
        // meaning of both the read and write cursors
        if (m_data)
        {
            size_type _copy_len = this->write_position();

            if (_copy_len > _new_capacity)
            {
                _copy_len = _new_capacity;
            }

            std::memcpy(_new_data, m_data, _copy_len);

            delete[] m_data;
        }

        m_data     = _new_data;
        m_capacity = _new_capacity;

        return true;
    }

    // --- private data members ---
    char*     m_data;
    size_type m_capacity;
};


// --- ostream operator (out-of-class definition) ---

template<typename _GP, typename _CP>
std::ostream&
operator<<(std::ostream&                    _os,
           const byte_buffer<_GP, _CP>&     _buf)
{
    if (_buf.size() > 0)
    {
        _os.write(
            reinterpret_cast<const char*>(_buf.data()),
            static_cast<std::streamsize>(_buf.size()));
    }

    return _os;
}


// =============================================================================
// II.  Factory Functions and Aliases
// =============================================================================

// byte_stream
//   alias: a byte_buffer configured for the producer/consumer
// (encode-in, decode-out) workflow - exponential growth with a
// dual cursor, so get<T>()/advance()/peek() and the incremental
// read protocol are available.
using byte_stream = byte_buffer<default_growth_policy,
                                dual_cursor_policy>;

// make_byte_buffer
//   creates a byte_buffer with default policies and the
// specified initial capacity.
inline byte_buffer<>
make_byte_buffer(std::size_t _capacity = 0)
{
    return byte_buffer<>(_capacity);
}

// make_byte_buffer (from a byte_string)
//   creates a byte_buffer initialized with a copy of _bytes.
inline byte_buffer<>
make_byte_buffer(const byte_string& _bytes)
{
    return byte_buffer<>(_bytes);
}

// make_byte_buffer (from a raw range)
//   creates a byte_buffer initialized with _len bytes from
// _data.
inline byte_buffer<>
make_byte_buffer(const byte* _data,
                 std::size_t _len)
{
    return byte_buffer<>(_data, _len);
}

// make_fixed_byte_buffer
//   creates a byte_buffer with fixed (non-growable) storage of
// the specified capacity.
inline byte_buffer<fixed_growth_policy>
make_fixed_byte_buffer(std::size_t _capacity)
{
    return byte_buffer<fixed_growth_policy>(_capacity);
}


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_BYTE_BUFFER_
