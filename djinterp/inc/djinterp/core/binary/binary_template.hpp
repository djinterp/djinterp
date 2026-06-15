/******************************************************************************
* djinterp [binary]                                        binary_template.hpp
*
*   Convenient, high-performance binary serialization -- the byte-oriented
* sibling of text_template.  A binary_template is BUILT (not parsed from an
* in-band format string: arbitrary bytes cannot carry a delimiter) by appending
* literal byte runs and named, typed fields, and then rendered against a value
* source into a byte buffer.  As with text_template the layout is fixed up
* front, so rendering is a straight-line run of appends.  The model is the
* familiar one of struct.pack: a layout plus values in, bytes out.
*
*   In the vocabulary of template.hpp this is the template-source-sink schema
* specialized to bytes: the binary_template is the *template* t, a value source
* is the *source*, the byte buffer is the *sink*, and pack(t) is the
* source-transformer F_t = F-hat(t).  (Like text_template the types are
* heterogeneous -- template, source and sink differ -- so it does not go
* through template_system.)
*
*   Field kinds:
*     literal              -- a fixed byte run (magic numbers, constant headers)
*     u8 / u16 / u32 / u64 -- an unsigned integer of that width, little- or
*                             big-endian (for a signed value pass the two's-
*                             complement pattern, e.g. static_cast<uint64_t>(x))
*     raw                  -- a byte span, variable-length or padded/truncated
*                             to a fixed width
*
*   Source contract: any callable (std::string_view name) -> binary_value, or
* an initializer_list of {name, binary_value} bindings.  Integer values convert
* implicitly (binary_value v = 1234u); byte values are explicit
* (binary_value::bytes(...)).  A byte value holds a NON-OWNING view, so when it
* comes from a callable that view must outlive the render call (values placed
* directly in a binding list are fine -- they live for the whole call).
*
*   Requires C++17 (std::string_view); self-suppresses below it.
*
* path:      /inc/djinterp/binary/binary_template.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.14
******************************************************************************/

#ifndef DJINTERP_BINARY_TEMPLATE_
#define DJINTERP_BINARY_TEMPLATE_ 1

// std
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <utility>
#include <initializer_list>
// djinterp
#include "../djinterp.hpp"      // NS_*, D_NODISCARD, language gates


// std::string_view backs the field-name lookup; below C++17 this module
// contributes nothing rather than failing to compile.
#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP


// ===========================================================================
// I.   byte_order
// ===========================================================================

// byte_order
//   enum: the byte ordering of an integer field.
enum class byte_order
{
    little,
    big,
};


// ===========================================================================
// II.  binary_value
// ===========================================================================

// binary_value
//   class: a value bound to a binary_template field -- either an unsigned
// integer (encoded to the field's width and byte order) or a non-owning view
// over a byte span (copied verbatim).  Integers convert implicitly; spans are
// built with the explicit bytes() factories.
class binary_value
{
public:
    using byte_type = std::uint8_t;
    using size_type = std::size_t;

    // an integer value (implicit, so `binary_value v = 0x1234u;` works)
    binary_value(
        std::uint64_t _value
    ) noexcept
        : m_is_bytes(false),
          m_integer(_value),
          m_data(nullptr),
          m_size(0)
    {}

    // a non-owning byte span
    static binary_value
    bytes(
        const byte_type* _data,
        size_type        _size
    ) noexcept
    {
        binary_value _value(std::uint64_t(0));
        _value.m_is_bytes = true;
        _value.m_data     = _data;
        _value.m_size     = _size;

        return _value;
    }

    // a byte span over a string_view (its chars taken as raw bytes)
    static binary_value
    bytes(
        std::string_view _view
    ) noexcept
    {
        return bytes(reinterpret_cast<const byte_type*>(_view.data()),
                     _view.size());
    }

    D_NODISCARD bool
    is_bytes() const noexcept
    {
        return m_is_bytes;
    }

    D_NODISCARD std::uint64_t
    integer() const noexcept
    {
        return m_integer;
    }

    D_NODISCARD const byte_type*
    data() const noexcept
    {
        return m_data;
    }

    D_NODISCARD size_type
    size() const noexcept
    {
        return m_size;
    }

private:
    bool             m_is_bytes;
    std::uint64_t    m_integer;
    const byte_type* m_data;
    size_type        m_size;
};


// ===========================================================================
// III. binary_template
// ===========================================================================

// binary_template
//   class: a fixed binary layout -- a sequence of literal byte runs and named,
// typed fields -- built incrementally and rendered against a value source into
// a byte buffer.  Built rather than parsed, since binary data cannot carry an
// in-band placeholder syntax.  High-performance: the layout is fixed once, and
// rendering is a straight run of appends over a reserved buffer.
class binary_template
{
public:
    using byte_type   = std::uint8_t;
    using buffer_type = std::vector<byte_type>;
    using size_type   = std::size_t;

    binary_template() = default;

    // --- builders (each returns *this for chaining) ---

    // literal bytes from a brace-enclosed list
    binary_template&
    literal(
        std::initializer_list<byte_type> _bytes
    )
    {
        const size_type _offset = m_literals.size();
        for (byte_type _b : _bytes)
        {
            m_literals.push_back(_b);
        }
        m_push_literal(_offset, _bytes.size());

        return *this;
    }

    // literal bytes taken from a view (its chars as raw bytes)
    binary_template&
    literal(
        std::string_view _bytes
    )
    {
        const size_type _offset = m_literals.size();
        for (char _c : _bytes)
        {
            m_literals.push_back(byte_type(_c));
        }
        m_push_literal(_offset, _bytes.size());

        return *this;
    }

    // a 1-byte unsigned integer field
    binary_template&
    u8(
        std::string_view _name
    )
    {
        return m_push_integer(_name, 1, byte_order::little);
    }

    // a 2-byte unsigned integer field
    binary_template&
    u16(
        std::string_view _name,
        byte_order       _order = byte_order::little
    )
    {
        return m_push_integer(_name, 2, _order);
    }

    // a 4-byte unsigned integer field
    binary_template&
    u32(
        std::string_view _name,
        byte_order       _order = byte_order::little
    )
    {
        return m_push_integer(_name, 4, _order);
    }

    // an 8-byte unsigned integer field
    binary_template&
    u64(
        std::string_view _name,
        byte_order       _order = byte_order::little
    )
    {
        return m_push_integer(_name, 8, _order);
    }

    // a variable-length raw byte field
    binary_template&
    raw(
        std::string_view _name
    )
    {
        return m_push_raw(_name, 0);
    }

    // a fixed-width raw byte field (zero-padded or truncated to _width)
    binary_template&
    raw(
        std::string_view _name,
        size_type        _width
    )
    {
        return m_push_raw(_name, _width);
    }

    // --- rendering ---

    // render_to -- append the encoded layout into _out (no result allocation).
    // _source is callable (std::string_view name) -> binary_value.
    template<typename _Source>
    void
    render_to(
        buffer_type& _out,
        _Source&&    _source
    ) const
    {
        for (const segment& _seg : m_segments)
        {
            // literal: copy the recorded byte run
            if (_seg.m_kind == segment_kind::literal)
            {
                if (_seg.m_length > 0)
                {
                    _out.insert(_out.end(),
                                m_literals.begin() + _seg.m_offset,
                                m_literals.begin() + _seg.m_offset
                                                   + _seg.m_length);
                }
                continue;
            }

            const binary_value _value =
                _source(std::string_view(_seg.m_name));

            // integer: encode to width / order
            if (_seg.m_kind == segment_kind::integer)
            {
                m_write_integer(_out, _value.integer(),
                                _seg.m_width, _seg.m_big);
                continue;
            }

            // raw: copy the span, padded/truncated for a fixed-width field
            m_write_raw(_out, _value.data(), _value.size(), _seg.m_length);
        }

        return;
    }

    // render_to -- inline-bindings convenience
    void
    render_to(
        buffer_type&                                                       _out,
        std::initializer_list<std::pair<std::string_view, binary_value>> _bindings
    ) const
    {
        render_to(_out, [&_bindings](std::string_view _name) -> binary_value
        {
            for (const std::pair<std::string_view, binary_value>& _entry
                     : _bindings)
            {
                if (_entry.first == _name)
                {
                    return _entry.second;
                }
            }

            return binary_value(std::uint64_t(0));
        });

        return;
    }

    // render -- the encoded layout as a freshly allocated buffer
    template<typename _Source>
    D_NODISCARD buffer_type
    render(
        _Source&& _source
    ) const
    {
        buffer_type _out;
        _out.reserve(m_min_size);
        render_to(_out, static_cast<_Source&&>(_source));

        return _out;
    }

    // render -- inline-bindings convenience
    D_NODISCARD buffer_type
    render(
        std::initializer_list<std::pair<std::string_view, binary_value>> _bindings
    ) const
    {
        buffer_type _out;
        _out.reserve(m_min_size);
        render_to(_out, _bindings);

        return _out;
    }

    // operator() -- render as a value (the functor face of a binary_template)
    template<typename _Source>
    D_NODISCARD buffer_type
    operator()(
        _Source&& _source
    ) const
    {
        return render(static_cast<_Source&&>(_source));
    }

    D_NODISCARD buffer_type
    operator()(
        std::initializer_list<std::pair<std::string_view, binary_value>> _bindings
    ) const
    {
        return render(_bindings);
    }

    // --- introspection ---

    // size_hint -- bytes contributed by literals and fixed-width fields (the
    // exact output size when there are no variable-length raw fields)
    D_NODISCARD size_type
    size_hint() const
    {
        return m_min_size;
    }

    // fields -- the field names, in layout order
    D_NODISCARD std::vector<std::string_view>
    fields() const
    {
        std::vector<std::string_view> _result;
        for (const segment& _seg : m_segments)
        {
            if (_seg.m_kind != segment_kind::literal)
            {
                _result.push_back(std::string_view(_seg.m_name));
            }
        }

        return _result;
    }

    D_NODISCARD size_type
    field_count() const
    {
        return m_field_count;
    }

    D_NODISCARD size_type
    segment_count() const
    {
        return m_segments.size();
    }

    D_NODISCARD bool
    empty() const
    {
        return m_segments.empty();
    }

private:
    // segment_kind
    //   enum: which sort of piece a segment is.
    enum class segment_kind : std::uint8_t
    {
        literal,
        integer,
        raw,
    };

    // segment
    //   struct: one piece of the layout.  literal -> [m_offset, m_offset +
    // m_length) into m_literals.  integer -> field m_name of m_width bytes,
    // big-endian iff m_big.  raw -> field m_name, fixed width m_length (0 means
    // variable-length).
    struct segment
    {
        segment_kind m_kind   = segment_kind::literal;
        size_type    m_offset = 0;
        size_type    m_length = 0;
        std::string  m_name;
        std::uint8_t m_width  = 0;
        bool         m_big    = false;
    };

    // m_push_literal
    //   function: record a literal segment and account its bytes.
    void
    m_push_literal(
        size_type _offset,
        size_type _length
    )
    {
        segment _seg;
        _seg.m_kind   = segment_kind::literal;
        _seg.m_offset = _offset;
        _seg.m_length = _length;
        m_segments.push_back(static_cast<segment&&>(_seg));
        m_min_size += _length;

        return;
    }

    // m_push_integer
    //   function: record an integer field of the given width and order.
    binary_template&
    m_push_integer(
        std::string_view _name,
        std::uint8_t     _width,
        byte_order       _order
    )
    {
        segment _seg;
        _seg.m_kind  = segment_kind::integer;
        _seg.m_name  = std::string(_name);
        _seg.m_width = _width;
        _seg.m_big   = (_order == byte_order::big);
        m_segments.push_back(static_cast<segment&&>(_seg));
        m_min_size += _width;
        ++m_field_count;

        return *this;
    }

    // m_push_raw
    //   function: record a raw byte field; _width 0 means variable-length.
    binary_template&
    m_push_raw(
        std::string_view _name,
        size_type        _width
    )
    {
        segment _seg;
        _seg.m_kind   = segment_kind::raw;
        _seg.m_length = _width;
        _seg.m_name   = std::string(_name);
        m_segments.push_back(static_cast<segment&&>(_seg));
        m_min_size += _width;
        ++m_field_count;

        return *this;
    }

    // m_write_integer
    //   function: append _value as _width bytes, most- or least-significant
    // byte first per _big.
    static void
    m_write_integer(
        buffer_type&  _out,
        std::uint64_t _value,
        std::uint8_t  _width,
        bool          _big
    )
    {
        if (_big)
        {
            for (std::uint8_t _i = _width; _i-- > 0; )
            {
                _out.push_back(byte_type((_value >> (8u * _i)) & 0xFFu));
            }
        }
        else
        {
            for (std::uint8_t _i = 0; _i < _width; ++_i)
            {
                _out.push_back(byte_type((_value >> (8u * _i)) & 0xFFu));
            }
        }

        return;
    }

    // m_write_raw
    //   function: append a byte span; for a fixed-width field copy at most
    // _fixed bytes and zero-pad the remainder.
    static void
    m_write_raw(
        buffer_type&     _out,
        const byte_type* _data,
        size_type        _size,
        size_type        _fixed
    )
    {
        // variable-length: copy the whole span
        if (_fixed == 0)
        {
            if (_size > 0)
            {
                _out.insert(_out.end(), _data, _data + _size);
            }

            return;
        }

        // fixed-width: copy what fits, then zero-pad
        const size_type _copy = (_size < _fixed) ? _size : _fixed;
        if (_copy > 0)
        {
            _out.insert(_out.end(), _data, _data + _copy);
        }
        for (size_type _i = _copy; _i < _fixed; ++_i)
        {
            _out.push_back(byte_type(0));
        }

        return;
    }

    buffer_type          m_literals;
    std::vector<segment> m_segments;
    size_type            m_min_size    = 0;
    size_type            m_field_count = 0;
};


// ===========================================================================
// IV.  pack
// ===========================================================================

// pack
//   class: the binary serialization transformation as a functor -- it binds a
// binary_template (the *template* t) and maps a *source* (field values) to the
// produced byte buffer (the *sink*).  This is F_t = F-hat(t): build the layout
// once, then call it with as many sources as desired.
class pack
{
public:
    using byte_type     = binary_template::byte_type;
    using buffer_type   = binary_template::buffer_type;
    using template_type = binary_template;

    pack() = default;

    explicit pack(
        template_type _template
    )
        : m_template(static_cast<template_type&&>(_template))
    {}

    template<typename _Source>
    D_NODISCARD buffer_type
    operator()(
        _Source&& _source
    ) const
    {
        return m_template.render(static_cast<_Source&&>(_source));
    }

    D_NODISCARD buffer_type
    operator()(
        std::initializer_list<std::pair<std::string_view, binary_value>> _source
    ) const
    {
        return m_template.render(_source);
    }

    template<typename _Source>
    void
    operator()(
        buffer_type& _out,
        _Source&&    _source
    ) const
    {
        m_template.render_to(_out, static_cast<_Source&&>(_source));

        return;
    }

    D_NODISCARD const template_type&
    bound() const
    {
        return m_template;
    }

private:
    template_type m_template;
};


NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_BINARY_TEMPLATE_
