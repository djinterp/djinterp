/******************************************************************************
* djinterp [container]                                       text_buffer.hpp
*
* Concrete text buffer for the djinterp container framework.
*   A text buffer is a growable, staged accumulator for character data.
* It satisfies the canonical text contract defined by
* container_text_traits.hpp:
*
*   TEXT:    std::string to_text() const
*   STREAM: std::size_t stream_to(char* _buf,
*                                  std::size_t _cap) const
*
*   and additionally provides to_string(), operator<<(ostream),
* c_str(), and the full container iteration protocol (begin/end,
* value_type, data, size, capacity, etc.).
*
*   The buffer always maintains a null terminator after the last
* written byte.  The null is not counted in size() but is always
* present in storage, making c_str() a const O(1) operation.
*
*   Text-specific operations include string and character append,
* line-oriented append, formatted append (printf-style), and
* delimiter-based joining.
*
*   Growth and cursor policies are inherited from buffer.hpp via
* buffer_base.  The default configuration is exponential growth
* with a write-only cursor, which suits the typical "accumulate
* then consume" text workflow.
*
* TRAIT SATISFACTION:
*   has_to_text_method_v    — yes (to_text)
*   has_stream_to_method_v  — yes (stream_to)
*   has_to_string_method_v  — yes (to_string)
*   is_ostream_insertable_v — yes (operator<<)
*   has_data_accessor_v     — yes (data)
*   has_size_accessor_v     — yes (max_size)
*   has_capacity_accessor_v — yes (capacity)
*   is_iterable_container_v — yes (begin/end)
*   has_push_back_v         — yes (push_back)
*   has_clear_v             — yes (clear)
*
*   container_text_strategy_v  → DTextStrategy::native
*   container_stream_strategy_v→ DStreamStrategy::native
*
* TABLE OF CONTENTS
* =================
* I.      text_buffer Class
*         I.a   Type Aliases and CRTP Contract
*         I.b   Construction and Destruction
*         I.c   Move and Copy
*         I.d   Text Append Operations
*         I.e   Formatted Append
*         I.f   Canonical Text Contract
*         I.g   String Extraction
*         I.h   Container Protocol
*         I.i   Ostream Integration
* II.     Factory Functions
*
*
* path:      /inc/container/text_buffer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.29
******************************************************************************/

#ifndef DJINTERP_CONTAINER_TEXT_BUFFER_
#define DJINTERP_CONTAINER_TEXT_BUFFER_ 1

#include <algorithm>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ostream>
#include <iterator>
#include <limits>
#include <new>
#include <string>
#include <type_traits>
#include <utility>
#include "..\djinterp.hpp"
#include "buffer.hpp"

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <string_view>
#endif

// D_ATTRIBUTE_FORMAT_PRINTF
//   attribute: enables compiler format-string checking
// for printf-style functions.  _fmt_idx is the 1-based
// index of the format string parameter; _va_idx is the
// 1-based index of the first variadic argument.
#if defined(__GNUC__) || defined(__clang__)
    #define D_ATTRIBUTE_FORMAT_PRINTF(_fmt_idx, _va_idx) \
        __attribute__((format(printf, _fmt_idx, _va_idx)))
#else
    #define D_ATTRIBUTE_FORMAT_PRINTF(_fmt_idx, _va_idx)
#endif


NS_DJINTERP
NS_CONTAINER

// =============================================================================
// I.   text_buffer Class
// =============================================================================

template<typename _GrowthPolicy = default_growth_policy,
         typename _CursorPolicy = write_only_cursor_policy>
class text_buffer
    : public buffer_base<
          text_buffer<_GrowthPolicy, _CursorPolicy>,
          _GrowthPolicy,
          _CursorPolicy>
{
private:
    using base_type = buffer_base<
        text_buffer<_GrowthPolicy, _CursorPolicy>,
        _GrowthPolicy,
        _CursorPolicy>;

    // buffer_base accesses storage()/capacity()/grow()
    friend base_type;

public:
    // =========================================================================
    // I.a  Type Aliases and CRTP Contract
    // =========================================================================

    using value_type      = char;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = char&;
    using const_reference = const char&;
    using pointer         = char*;
    using const_pointer   = const char*;
    using iterator        = char*;
    using const_iterator  = const char*;
    using reverse_iterator =
        std::reverse_iterator<iterator>;
    using const_reverse_iterator =
        std::reverse_iterator<const_iterator>;


    // =========================================================================
    // I.b  Construction and Destruction
    // =========================================================================

    // default constructor
    //   creates an empty text buffer with no allocation.
    // The first append triggers the growth policy.
    text_buffer() noexcept
        : base_type()
        , m_data(nullptr)
        , m_capacity(0)
    {
    }

    // capacity constructor
    //   creates an empty text buffer pre-allocated to hold
    // at least _initial_capacity bytes of text content
    // (plus the null terminator).
    explicit
    text_buffer(size_type _initial_capacity)
        : base_type()
        , m_data(nullptr)
        , m_capacity(0)
    {
        if (_initial_capacity > 0)
        {
            grow(_initial_capacity);
            m_data[0] = '\0';
        }
    }

    // string constructor
    //   creates a text buffer initialized with a copy of
    // the provided C string.
    explicit
    text_buffer(const char* _str)
        : base_type()
        , m_data(nullptr)
        , m_capacity(0)
    {
        if (_str)
        {
            size_type len = std::strlen(_str);

            append(_str, len);
        }
    }

    // string + length constructor
    //   creates a text buffer initialized with _len bytes
    // copied from _str.  _str need not be null-terminated.
    text_buffer(const char* _str,
                size_type   _len)
        : base_type()
        , m_data(nullptr)
        , m_capacity(0)
    {
        if ( (_str) && (_len > 0) )
        {
            append(_str, _len);
        }
    }

    // std::string constructor
    //   creates a text buffer initialized from a string.
    explicit
    text_buffer(const std::string& _str)
        : base_type()
        , m_data(nullptr)
        , m_capacity(0)
    {
        if (!_str.empty())
        {
            append(_str.data(), _str.size());
        }
    }

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // string_view constructor
    //   creates a text buffer initialized from a
    // string_view.
    explicit
    text_buffer(std::string_view _sv)
        : base_type()
        , m_data(nullptr)
        , m_capacity(0)
    {
        if (!_sv.empty())
        {
            append(_sv.data(), _sv.size());
        }
    }
#endif  // C++17

    // destructor
    ~text_buffer() noexcept
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
    text_buffer(text_buffer&& _other) noexcept
        : base_type(static_cast<base_type&&>(_other))
        , m_data(_other.m_data)
        , m_capacity(_other.m_capacity)
    {
        _other.m_data     = nullptr;
        _other.m_capacity = 0;
    }

    // move assignment
    text_buffer& operator=(text_buffer&& _other) noexcept
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
    text_buffer(const text_buffer& _other)
        : base_type()
        , m_data(nullptr)
        , m_capacity(0)
    {
        if (_other.size() > 0)
        {
            size_type sz = _other.size();

            grow(sz);

            std::memcpy(m_data,
                        _other.content_begin_(),
                        sz);

            // fresh cursors: content at [0, sz)
            _CursorPolicy::reset(this->m_cursors);
            _CursorPolicy::advance_write(
                this->m_cursors, sz);

            m_data[sz] = '\0';
        }
    }

    // copy assignment
    text_buffer& operator=(const text_buffer& _other)
    {
        if (this != &_other)
        {
            this->reset();

            if (_other.size() > 0)
            {
                size_type sz = _other.size();

                if (this->capacity() < sz)
                {
                    grow(sz);
                }

                std::memcpy(m_data,
                            _other.content_begin_(),
                            sz);

                _CursorPolicy::reset(this->m_cursors);
                _CursorPolicy::advance_write(
                    this->m_cursors, sz);

                m_data[sz] = '\0';
            }
            else if (m_data)
            {
                m_data[0] = '\0';
            }
        }

        return *this;
    }


    // =========================================================================
    // I.d  Text Append Operations
    // =========================================================================
    // All append methods maintain the null terminator
    // invariant: storage()[write_position()] == '\0'
    // after every operation.  The null byte is not
    // counted in size().

    // append (buffer + length)
    //   appends _len bytes from _str.  _str need not be
    // null-terminated.  Returns the number of bytes
    // actually appended.
    //
    // capacity() already reserves one byte for the null
    // terminator, so ensure_writable(_len) is sufficient.
    size_type
    append(const char* _str,
           size_type   _len) noexcept
    {
        if ( (!_str) || (_len == 0) )
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

        std::memcpy(this->write_head(), _str, _len);

        this->commit(_len);

        // maintain null terminator
        storage()[this->write_position()] = '\0';

        return _len;
    }

    // append (C string)
    //   appends a null-terminated C string.  Returns the
    // number of bytes appended (excluding the source's
    // null terminator).
    size_type
    append(const char* _str) noexcept
    {
        if (!_str)
        {
            return 0;
        }

        return append(_str, std::strlen(_str));
    }

    // append (std::string)
    //   appends the contents of a std::string.
    size_type
    append(const std::string& _str) noexcept
    {
        return append(_str.data(), _str.size());
    }

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // append (string_view)
    //   appends the contents of a string_view.
    size_type
    append(std::string_view _sv) noexcept
    {
        return append(_sv.data(), _sv.size());
    }
#endif  // C++17

    // append (single character)
    //   appends a single character.  Returns 1 on
    // success, 0 on failure.
    size_type
    append(char _ch) noexcept
    {
        return append(&_ch, 1);
    }

    // append (repeated character)
    //   appends _count copies of _ch.  Returns the
    // number of characters actually appended.
    size_type
    append(char      _ch,
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

        std::memset(this->write_head(), _ch, _count);

        this->commit(_count);

        storage()[this->write_position()] = '\0';

        return _count;
    }

    // append_line (buffer + length)
    //   appends _len bytes from _str followed by a
    // newline character.
    size_type
    append_line(const char* _str,
                size_type   _len) noexcept
    {
        size_type written = append(_str, _len);

        written += append('\n');

        return written;
    }

    // append_line (C string)
    //   appends a null-terminated string followed by a
    // newline.
    size_type
    append_line(const char* _str) noexcept
    {
        if (!_str)
        {
            return append('\n');
        }

        return append_line(_str, std::strlen(_str));
    }

    // append_line (std::string)
    //   appends a string followed by a newline.
    size_type
    append_line(const std::string& _str) noexcept
    {
        return append_line(_str.data(), _str.size());
    }

    // append_line (no argument)
    //   appends a bare newline.
    size_type
    append_line() noexcept
    {
        return append('\n');
    }

    // push_back
    //   appends a single character.  Satisfies the
    // Container push_back requirement detected by
    // has_push_back_v.
    void push_back(char _ch)
    {
        append(_ch);

        return;
    }


    // =========================================================================
    // I.e  Formatted Append
    // =========================================================================

    // appendf
    //   appends formatted text using printf-style format
    // specifiers.  Returns the number of characters
    // appended, or 0 on failure.
    //
    //   Uses a two-pass approach: first measures the
    // required length via vsnprintf with a null buffer,
    // then writes into the buffer after ensuring capacity.
    D_ATTRIBUTE_FORMAT_PRINTF(2, 3)
    size_type
    appendf(const char* _fmt, ...) noexcept
    {
        if (!_fmt)
        {
            return 0;
        }

        va_list args;
        va_list args_copy;

        va_start(args, _fmt);
        va_copy(args_copy, args);

        // measure required length
        int needed = std::vsnprintf(
            nullptr, 0, _fmt, args);

        va_end(args);

        if (needed <= 0)
        {
            va_end(args_copy);

            return 0;
        }

        size_type len =
            static_cast<size_type>(needed);

        // ensure space for content (null byte is
        // already reserved by capacity())
        if (!this->ensure_writable(len))
        {
            va_end(args_copy);

            return 0;
        }

        // write directly into the buffer
        std::vsnprintf(
            this->write_head(),
            len + 1,
            _fmt,
            args_copy);

        va_end(args_copy);

        this->commit(len);

        // null is already written by vsnprintf,
        // but ensure the invariant explicitly
        storage()[this->write_position()] = '\0';

        return len;
    }


    // =========================================================================
    // I.f  Canonical Text Contract
    // =========================================================================
    // These methods satisfy the detection predicates in
    // container_text_traits.hpp, ensuring:
    //   container_text_strategy_v  → DTextStrategy::native
    //   container_stream_strategy_v→ DStreamStrategy::native

    // to_text
    //   returns the buffer contents as a std::string.
    // Satisfies has_to_text_method_v.
    std::string to_text() const
    {
        if (this->size() == 0)
        {
            return std::string();
        }

        return std::string(
            content_begin_(), this->size());
    }

    // stream_to
    //   writes as many bytes as fit into _buf (up to
    // _cap bytes) and returns the number of bytes
    // written.  Does not advance any internal cursor.
    // Satisfies has_stream_to_method_v.
    std::size_t
    stream_to(char*       _buf,
              std::size_t _cap) const noexcept
    {
        if ( (!_buf) || (_cap == 0) )
        {
            return 0;
        }

        std::size_t n =
            std::min(this->size(), _cap);

        std::memcpy(_buf, content_begin_(), n);

        return n;
    }

    // to_string
    //   returns the buffer contents as a std::string.
    // Satisfies has_to_string_method_v.  Identical to
    // to_text() but provided for API symmetry with
    // containers that expose to_string().
    std::string to_string() const
    {
        return to_text();
    }


    // =========================================================================
    // I.g  String Extraction
    // =========================================================================

    // c_str
    //   returns a const pointer to a null-terminated
    // string covering the meaningful content region.
    // O(1) because the null terminator is always
    // maintained at write_position().
    //
    // Write-only cursor: returns the start of storage.
    // Dual cursor: returns storage + read_position;
    //   the null at write_position terminates the
    //   unconsumed region.
    const char* c_str() const noexcept
    {
        if (!m_data)
        {
            return "";
        }

        return content_begin_();
    }

    // str
    //   returns the buffer contents as a std::string.
    // Convenience alias for to_text().
    std::string str() const
    {
        return to_text();
    }

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    // view
    //   returns a non-owning string_view of the
    // meaningful content.  The view is invalidated by
    // any operation that may reallocate the buffer.
    std::string_view view() const noexcept
    {
        return std::string_view(
            content_begin_(), this->size());
    }
#endif  // C++17

    // substr
    //   returns a substring from position _pos (relative
    // to the start of meaningful content) with length
    // _len.  If _len extends past the end, it is clamped.
    std::string
    substr(size_type _pos,
           size_type _len = std::string::npos) const
    {
        if (_pos >= this->size())
        {
            return std::string();
        }

        size_type remaining =
            this->size() - _pos;

        if (_len > remaining)
        {
            _len = remaining;
        }

        return std::string(
            content_begin_() + _pos, _len);
    }

    // line_count
    //   returns the number of newline characters in
    // the meaningful content region.
    size_type line_count() const noexcept
    {
        const char* p   = content_begin_();
        size_type   sz  = this->size();
        size_type   cnt = 0;

        for (size_type i = 0; i < sz; ++i)
        {
            if (p[i] == '\n')
            {
                ++cnt;
            }
        }

        return cnt;
    }


    // =========================================================================
    // I.h  Container Protocol
    // =========================================================================
    // Provides the structural interface detected by
    // container_traits.hpp: iterators, positional
    // access, size, capacity, data.
    //
    // All accessors operate over the meaningful content
    // region, which differs by cursor model:
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
            ? m_data + this->write_position()
            : nullptr;
    }

    const_iterator begin() const noexcept
    {
        return content_begin_();
    }

    const_iterator end() const noexcept
    {
        return m_data
            ? m_data + this->write_position()
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
    // Indices are relative to the start of meaningful
    // content (i.e. operator[](0) is the first
    // unconsumed character in dual-cursor mode).

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
                "text_buffer::at: index out of range");
        }

        return content_begin_mut_()[_pos];
    }

    const_reference at(size_type _pos) const
    {
        if (_pos >= this->size())
        {
            throw std::out_of_range(
                "text_buffer::at: index out of range");
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
        return m_data[this->write_position() - 1];
    }

    const_reference back() const noexcept
    {
        return m_data[this->write_position() - 1];
    }

    // --- data access ---
    // data() returns a pointer to the start of the
    // meaningful content region, consistent with
    // begin() and size().

    const char* data() const noexcept
    {
        return content_begin_();
    }

    char* data() noexcept
    {
        return content_begin_mut_();
    }

    size_type capacity() const noexcept
    {
        // report usable capacity (excluding the
        // reserved null terminator byte)
        return (m_capacity > 0)
            ? (m_capacity - 1)
            : 0;
    }

    size_type max_size() const noexcept
    {
        return std::numeric_limits<size_type>::max()
            - 1;
    }

    // clear
    //   resets the buffer to empty and zeroes storage.
    // Satisfies has_clear_v.
    void clear() noexcept
    {
        base_type::clear();

        if (m_data)
        {
            m_data[0] = '\0';
        }

        return;
    }

    // swap
    //   exchanges contents with another text_buffer.
    void swap(text_buffer& _other) noexcept
    {
        if (this == &_other)
        {
            return;
        }

        // swap cursor state
        auto tmp_cursors = this->m_cursors;
        this->m_cursors  = _other.m_cursors;
        _other.m_cursors = tmp_cursors;

        // swap storage
        char* tmp_data       = m_data;
        size_type tmp_cap    = m_capacity;

        m_data               = _other.m_data;
        m_capacity           = _other.m_capacity;

        _other.m_data        = tmp_data;
        _other.m_capacity    = tmp_cap;

        return;
    }


    // =========================================================================
    // I.i  Ostream Integration
    // =========================================================================
    // Satisfies is_ostream_insertable_v via a friend
    // operator<<.

    template<typename _GP, typename _CP>
    friend std::ostream&
    operator<<(std::ostream&            _os,
               const text_buffer<_GP,
                                 _CP>& _buf);


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
    // These compute the start of the meaningful content
    // region, which differs by cursor model:
    //   write_only: m_data
    //   dual:       m_data + read_pos

    const char* content_begin_() const noexcept
    {
        if (!m_data)
        {
            return nullptr;
        }

        if constexpr (_CursorPolicy::has_read_cursor)
        {
            return m_data + this->m_cursors.read_pos;
        }
        else
        {
            return m_data;
        }
    }

    char* content_begin_mut_() noexcept
    {
        if (!m_data)
        {
            return nullptr;
        }

        if constexpr (_CursorPolicy::has_read_cursor)
        {
            return m_data + this->m_cursors.read_pos;
        }
        else
        {
            return m_data;
        }
    }

    // grow
    //   reallocates so that usable capacity is at least
    // _new_usable bytes.  Raw allocation is _new_usable + 1
    // to reserve space for the null terminator.
    // Preserves existing content and null terminator.
    // Returns true on success.
    bool grow(size_type _new_usable) noexcept
    {
        // already have enough usable capacity
        size_type current_usable =
            (m_capacity > 0) ? (m_capacity - 1) : 0;

        if (_new_usable <= current_usable)
        {
            return true;
        }

        // +1 for the null terminator byte
        size_type alloc_size = _new_usable + 1;

        // overflow check
        if (_new_usable >=
            std::numeric_limits<size_type>::max())
        {
            return false;
        }

        char* new_data = nullptr;

        try
        {
            new_data = new char[alloc_size];
        }
        catch (const std::bad_alloc&)
        {
            return false;
        }

        // copy existing content + null
        if (m_data)
        {
            size_type copy_len =
                this->write_position() + 1;

            if (copy_len > alloc_size)
            {
                copy_len = alloc_size;
            }

            std::memcpy(new_data, m_data, copy_len);

            delete[] m_data;
        }
        else
        {
            new_data[0] = '\0';
        }

        m_data     = new_data;
        m_capacity = alloc_size;

        return true;
    }

    // --- private capacity helper ---

    // raw_capacity
    //   returns the true allocation size (including the
    // reserved null terminator byte).  Used internally;
    // capacity() reports the usable size.
    size_type raw_capacity() const noexcept
    {
        return m_capacity;
    }

    // --- private data members ---
    char*     m_data;
    size_type m_capacity;
};


// --- ostream operator (out-of-class definition) ---

template<typename _GP, typename _CP>
std::ostream&
operator<<(std::ostream&                    _os,
           const text_buffer<_GP, _CP>&     _buf)
{
    if (_buf.size() > 0)
    {
        _os.write(_buf.data(),
                  static_cast<std::streamsize>(
                      _buf.size()));
    }

    return _os;
}


// =============================================================================
// II.  Factory Functions
// =============================================================================

// make_text_buffer
//   creates a text_buffer with default policies and the
// specified initial capacity.
inline text_buffer<>
make_text_buffer(std::size_t _capacity = 0)
{
    return text_buffer<>(_capacity);
}

// make_text_buffer (from C string)
//   creates a text_buffer initialized with a copy of
// the provided string.
inline text_buffer<>
make_text_buffer(const char* _str)
{
    return text_buffer<>(_str);
}

// make_text_buffer (from std::string)
//   creates a text_buffer initialized from a string.
inline text_buffer<>
make_text_buffer(const std::string& _str)
{
    return text_buffer<>(_str);
}

// make_fixed_text_buffer
//   creates a text_buffer with fixed (non-growable)
// storage of the specified capacity.
inline text_buffer<fixed_growth_policy>
make_fixed_text_buffer(std::size_t _capacity)
{
    return text_buffer<fixed_growth_policy>(_capacity);
}


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TEXT_BUFFER_
