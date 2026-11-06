/*******************************************************************************
* djinterp [cpp]                                               output_stream.hpp
*
*   The C++ face of the output stream in c/util/ostream/ostream.h:
* DOutputStream, one object that writes to the console, a file, a dj_string, a
* caller's array, several streams at once, or anything else with a sink;
* DOutputCursor, an output iterator over its window, so std::copy,
* std::format_to and fmt::format_to write into any of those directly; and
* DBufferedOutputStream, a DOutputStream that carries its own buffer.
*
*   C++98 AND UP, ADDING ONLY. Every member compiles at C++98. C++11 adds the
* moves and the explicit bool conversion, C++17 string_view; nothing above
* C++98 replaces anything below it, so the same source builds at every level.
* The data members are the same at every level -- the C stream and the
* destination's state -- so a member added above C++98 cannot change the
* layout (decision 3.5's review rule), and all the state is a C struct, the
* boundary 3.5 already names for mixing.
*
*   NOTHING HERE COSTS MORE THAN THE C. Every member is inline and forwards to
* the kernel; the writes are the kernel's inline paths. Failure is reported by
* value -- a bool return, with status() and last_errno() to say why -- and
* nothing throws (D3). There is no OS here: every platform decision was made
* once, in c/util and c/filesys. Non-copyable at every level, movable from
* C++11 (D4).
*
*   A MOVE RE-POINTS WHAT POINTED INTO THE OBJECT. The C stream may hold the
* address of the destination's state (a console's, a fixed array's) or of a
* buffer the object carries, and its window may point into either; a move
* points them at the new object's copies. A sink taken with as_sink(), a
* cursor, or the stream handed to a fan-out through get() hold this object's
* address too, and a move invalidates them as it would any pointer.
*
*   STANDARD-LIBRARY ADAPTERS ARE ELSEWHERE. A std::string, a std::ostream and
* a std::streambuf over a stream are in output_stream_std.hpp, so a unit that
* writes only to the console or a file includes no iostream.
*
*   NUMBERS FOR PEOPLE ARE TEXT'S. Integers are written here, in place and
* untouched by the locale. A double, and printf, are number formatting, which
* lives in text above the kernel (ruling 3): cpp/text/output_format.hpp adds
* write_double(os, value), operator<<(os, double) and
* write_formatted(os, format, ...).
* Without it, streaming a double does not compile: no integer overload is a
* better match than another.
*
*
* path:      /inc/djinterp/cpp/util/output/output_stream.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.11.05
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  CONSOLE CHANNEL
    ---------------
    1.  DConsoleChannel
2.  CURSOR
    ------
    1.  DOutputCursor
3.  OUTPUT STREAM
    -------------
    1.  DOutputStream
         1.  Construction and destruction
         2.  Transfer
         3.  Destinations
         4.  Output
         5.  Cursors
         6.  Flush and close
         7.  Observers
         8.  Implementation
    2.  DBufferedOutputStream
4.  CALLABLE SINK
    -------------
    1.  DOutputStreamSink
*/

#ifndef DJINTERP_UTIL_OUTPUT_OUTPUT_STREAM_HPP
#define DJINTERP_UTIL_OUTPUT_OUTPUT_STREAM_HPP 1

// std
#include <cstddef>     // std::size_t, std::ptrdiff_t
#include <cstdio>      // std::FILE
#include <cstring>     // std::strlen
#include <functional>  // std::less: the move's pointer test
#include <iterator>    // std::output_iterator_tag
// djinterp
#include "../../djinterp.hpp"                         // framework root
#include "../../../c/util/ostream/ostream.h"          // struct dj_ostream
#include "../../../c/util/ostream/ostream_console.h"  // d_ostream_init_console
#include "../../../c/util/ostream/ostream_file.h"     // d_ostream_open_file
#include "../../../config/c/util/ostream/cfg_ostream.h"
                                    // D_CFG_OSTREAM_BUFFER_SIZE
// re_std
#include "../../../../re_std/cpp/cstdint/cstdint.hpp"  // int32_t, INT64_MAX

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <string_view>  // std::string_view
#endif


NS_DJINTERP


//==============================================================================
// 1.  CONSOLE CHANNEL
//==============================================================================


// 1.1    DConsoleChannel
//------------------------------------------------------------------------------
// DConsoleChannel
//   struct: the console's channels, a struct-scoped enum (decision 3.4).
struct DConsoleChannel
{
    enum value
    {
        out = D_OSTREAM_CONSOLE_OUT,
        err = D_OSTREAM_CONSOLE_ERR
    };
};


//==============================================================================
// 2.  CURSOR
//==============================================================================


// 2.1    DOutputCursor
//------------------------------------------------------------------------------
// DOutputCursor
//   class: an output iterator over a stream's window (ostream.h 3.4). Writes
// through it are a compare and a store against two pointers it keeps by
// value, so in a loop they stay in registers; a full window takes the slow
// path and comes back with a fresh one. Like any output iterator, keep
// writing through the one an algorithm returns, and give it back to the
// stream -- give(), or DOutputStream::give -- before using the stream any
// other way.
class DOutputCursor
{
public:
    typedef std::output_iterator_tag iterator_category;
    typedef void                     value_type;
    typedef std::ptrdiff_t           difference_type;
    typedef void                     pointer;
    typedef void                     reference;

    /**
     * @brief A cursor over no stream, to be assigned one; writing through it
     *        does nothing.
     */
    DOutputCursor() D_NOEXCEPT
        : m_stream(D_NULLPTR),
          m_cursor(empty())
    {}

    /**
     * @brief Takes a stream's window.
     *
     * @param[in] _stream  the stream, which must outlive the cursor; null
     *                     gives a cursor over no stream.
     */
    explicit
    DOutputCursor(
        struct dj_ostream* _stream
    ) D_NOEXCEPT
        : m_stream(_stream),
          m_cursor((_stream != D_NULLPTR) ? d_ostream_cursor_take(_stream)
                                          : empty())
    {}

    // the output iterator's protocol: *it = c writes, ++ is the write's
    DOutputCursor& operator*() D_NOEXCEPT
    {
        return *this;
    }

    DOutputCursor& operator++() D_NOEXCEPT
    {
        return *this;
    }

    DOutputCursor& operator++(int) D_NOEXCEPT
    {
        return *this;
    }

    DOutputCursor& operator=(
        char _c
    ) D_NOEXCEPT
    {
        m_cursor = d_ostream_cursor_put(m_stream, m_cursor, _c);

        return *this;
    }

    /**
     * @brief Writes one character.
     *
     * @param[in] _c  the character.
     */
    void put(
        char _c
    ) D_NOEXCEPT
    {
        m_cursor = d_ostream_cursor_put(m_stream, m_cursor, _c);

        return;
    }

    /**
     * @brief Writes bytes.
     *
     * @param[in] _data  the bytes.
     * @param[in] _size  how many.
     */
    void write(
        const char* _data,
        std::size_t _size
    ) D_NOEXCEPT
    {
        m_cursor = d_ostream_cursor_write(m_stream, m_cursor, _data, _size);

        return;
    }

    /**
     * @brief Gives the cursor's writes back to the stream.
     *
     * @return true while the stream is healthy.
     */
    bool give() D_NOEXCEPT
    {
        return ( (m_stream != D_NULLPTR) &&
                 (d_ostream_cursor_give(m_stream, m_cursor) != 0) );
    }

    /**
     * @brief The stream the cursor was taken from.
     *
     * @return the stream.
     */
    D_NODISCARD struct dj_ostream* stream() const D_NOEXCEPT
    {
        return m_stream;
    }

private:
    // empty
    //   function: a cursor with no room, so every write takes the slow path,
    // which refuses it for want of a stream.
    static struct dj_ostream_cursor empty() D_NOEXCEPT
    {
        struct dj_ostream_cursor cursor;

        cursor.cur = D_NULLPTR;
        cursor.end = D_NULLPTR;

        return cursor;
    }

    struct dj_ostream*       m_stream;
    struct dj_ostream_cursor m_cursor;
};


//==============================================================================
// 3.  OUTPUT STREAM
//==============================================================================


// 3.1    DOutputStream
//------------------------------------------------------------------------------
// DOutputStream
//   class: owns a stream and its destination's state. Non-copyable at every
// level; movable from C++11. Unbuffered until set_buffer() lends it storage,
// or DBufferedOutputStream carries its own.
class DOutputStream
{
public:
    // 3.1.1  Construction and destruction
    //--------------------------------------------------------------------------
    /**
     * @brief Constructs a stream with no destination; is_valid() is false
     *        until one is attached.
     */
    DOutputStream() D_NOEXCEPT
        : m_default_buffer(D_NULLPTR),
          m_default_capacity(0)
    {
        reset();
    }

    /**
     * @brief Closes the stream: delivers the window, finishes the
     *        destination and releases what the stream owns.
     */
    ~DOutputStream()
    {
        (void)d_ostream_close(&m_stream);
    }

#if (D_MOVE_ENABLED == 1)
    // 3.1.2  Transfer
    //--------------------------------------------------------------------------
    /**
     * @brief Moves the stream into a new object, so exactly one closes it.
     *        C++11 and later, ADDITIVE.
     *
     * @note A buffer _other carries itself (a DBufferedOutputStream's) cannot
     *       come along into an object without one: its bytes are delivered
     *       first, and the stream arrives unbuffered.
     *
     * @param[in,out] _other  the stream to take; left with no destination.
     */
    DOutputStream(
        DOutputStream&& _other
    ) D_NOEXCEPT
        : m_default_buffer(D_NULLPTR),
          m_default_capacity(0)
    {
        take(_other, false);
    }

    /**
     * @brief Closes this stream, then takes _other's. C++11 and later,
     *        ADDITIVE.
     *
     * @note As for the move constructor; a buffer this object carries is
     *       lent to the stream it takes.
     *
     * @param[in,out] _other  the stream to take; left with no destination.
     * @return *this.
     */
    DOutputStream& operator=(
        DOutputStream&& _other
    ) D_NOEXCEPT
    {
        if (this != &_other)
        {
            (void)d_ostream_close(&m_stream);
            take(_other, false);
        }

        return *this;
    }
#endif  // D_MOVE_ENABLED

    // 3.1.3  Destinations
    //--------------------------------------------------------------------------
    //   Each closes the current destination first. On failure the stream has
    // none, and status() says why. A transport takes the buffer the object
    // carries, if it carries one.

    /**
     * @brief Writes to any destination: a sink, or a storage destination's
     *        table.
     *
     * @param[in] _sink   the destination.
     * @param[in] _ops    its operations, or null.
     * @param[in] _flags  D_OSTREAM_* flags.
     * @return true on success.
     */
    bool attach(
        struct dj_pack_sink          _sink,
        const struct dj_ostream_ops* _ops   = D_NULLPTR,
        int32_t                      _flags = 0
    ) D_NOEXCEPT
    {
        release();

        return settle(d_ostream_init(&m_stream, _sink, _ops, _flags));
    }

    /**
     * @brief Writes to stdout or stderr.
     *
     * @param[in] _channel  DConsoleChannel::out or ::err.
     * @param[in] _flags    D_OSTREAM_BINARY and D_OSTREAM_LINE_BUFFERED.
     * @return true on success.
     */
    bool attach_console(
        DConsoleChannel::value _channel = DConsoleChannel::out,
        int32_t                _flags   = 0
    ) D_NOEXCEPT
    {
        release();

        return settle(d_ostream_init_console(&m_stream,
                                             &m_state.console,
                                             static_cast<int32_t>(_channel),
                                             _flags));
    }

    /**
     * @brief Writes to a FILE* the caller opened.
     *
     * @param[in] _file  the file.
     * @param[in] _owns  true to have the stream close it.
     * @return true on success.
     */
    bool attach_file(
        std::FILE* _file,
        bool       _owns = false
    ) D_NOEXCEPT
    {
        release();

        return settle(d_ostream_init_file(&m_stream,
                                          _file,
                                          _owns ? D_OSTREAM_OWNS_CONTEXT : 0));
    }

    /**
     * @brief Opens a file in binary mode, owned by the stream.
     *
     * @param[in] _path    the file's path.
     * @param[in] _append  true to append; false to truncate.
     * @return true on success; on failure last_errno() has the open's errno.
     */
    bool open_file(
        const char* _path,
        bool        _append = false
    ) D_NOEXCEPT
    {
        release();

        return settle(d_ostream_open_file(&m_stream, _path, _append ? 1 : 0));
    }

    /**
     * @brief Appends to a dj_string, in place. Linking needs
     *        ostream_string.c.
     *
     * @param[in] _string  the target, which must outlive the stream.
     * @return true on success.
     */
    bool attach_string(
        struct dj_string* _string
    ) D_NOEXCEPT
    {
        release();

        return d_ostream_init_string(&m_stream, _string) != 0;
    }

    /**
     * @brief Writes into a caller's array, with snprintf's contract: what
     *        fits is kept, written() counts all of it.
     *
     * @param[out] _data       the array.
     * @param[in]  _capacity   its size in bytes.
     * @param[in]  _terminate  true to keep it NUL-terminated.
     * @return true on success.
     */
    bool attach_fixed(
        void*       _data,
        std::size_t _capacity,
        bool        _terminate = true
    ) D_NOEXCEPT
    {
        release();

        return d_ostream_init_fixed(&m_stream,
                                    &m_state.fixed,
                                    _data,
                                    _capacity,
                                    _terminate ? 1 : 0) != 0;
    }

    /**
     * @brief Counts what is written and keeps none of it: the measure pass.
     *
     * @return true on success.
     */
    bool attach_counter() D_NOEXCEPT
    {
        return attach_fixed(D_NULLPTR, 0, false);
    }

    /**
     * @brief Copies every write to each of a fan-out's targets.
     *
     * @param[in] _fanout  the targets and policy, which must outlive the
     *                     stream.
     * @return true on success.
     */
    bool attach_fanout(
        struct dj_ostream_fanout& _fanout
    ) D_NOEXCEPT
    {
        release();

        return settle(d_ostream_init_fanout(&m_stream, &_fanout));
    }

    /**
     * @brief Gives a transport a buffer, or takes it away.
     *
     * @param[in] _buffer    storage the caller keeps alive while it is used.
     * @param[in] _capacity  its size in bytes.
     * @return true on success; false for a storage destination, which has
     *         its own window.
     */
    bool set_buffer(
        void*       _buffer,
        std::size_t _capacity
    ) D_NOEXCEPT
    {
        return d_ostream_set_buffer(&m_stream, _buffer, _capacity) != 0;
    }

    // 3.1.4  Output
    //--------------------------------------------------------------------------
    //   Every write reports whether the stream is still healthy; the first
    // failure is kept, so checking once at the end is enough.

    /**
     * @brief Writes bytes.
     *
     * @param[in] _data  the bytes.
     * @param[in] _size  how many.
     * @return true when every byte was accepted.
     */
    bool write(
        const void* _data,
        std::size_t _size
    ) D_NOEXCEPT
    {
        return d_ostream_write(&m_stream, _data, _size) != 0;
    }

    /**
     * @brief Writes a NUL-terminated string, without its terminator.
     *
     * @param[in] _cstr  the string.
     * @return as write().
     */
    bool write(
        const char* _cstr
    ) D_NOEXCEPT
    {
        return d_ostream_write_cstr(&m_stream, _cstr) != 0;
    }

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    /**
     * @brief Writes a string_view. C++17 and later, ADDITIVE.
     *
     * @param[in] _text  the text.
     * @return as write().
     */
    bool write(
        std::string_view _text
    ) D_NOEXCEPT
    {
        return d_ostream_write(&m_stream, _text.data(), _text.size()) != 0;
    }
#endif  // C++17

    /**
     * @brief Writes anything with data() and size(): a std::string, a
     *        vector of char.
     *
     * @param[in] _text  the text.
     * @return as write().
     */
    template<typename Text>
    bool write_text(
        const Text& _text
    ) D_NOEXCEPT
    {
        return d_ostream_write(&m_stream, _text.data(), _text.size()) != 0;
    }

    /**
     * @brief Writes one character.
     *
     * @param[in] _c  the character.
     * @return as write().
     */
    bool put(
        char _c
    ) D_NOEXCEPT
    {
        return d_ostream_write_char(&m_stream, _c) != 0;
    }

    /**
     * @brief Writes one character _count times.
     *
     * @param[in] _c      the character.
     * @param[in] _count  how many times.
     * @return as write().
     */
    bool fill(
        char        _c,
        std::size_t _count
    ) D_NOEXCEPT
    {
        return d_ostream_fill(&m_stream, _c, _count) != 0;
    }

    // print: integers in decimal, converted in place; no locale reaches them
    bool print(
        int _value
    ) D_NOEXCEPT
    {
        return d_ostream_write_int(&m_stream, _value) != 0;
    }

    bool print(
        unsigned int _value
    ) D_NOEXCEPT
    {
        return d_ostream_write_uint(&m_stream, _value) != 0;
    }

    bool print(
        long _value
    ) D_NOEXCEPT
    {
        return d_ostream_write_long(&m_stream, _value) != 0;
    }

    bool print(
        unsigned long _value
    ) D_NOEXCEPT
    {
        return d_ostream_write_ulong(&m_stream, _value) != 0;
    }

#if ( (D_ENV_HAS_LONG_LONG) &&                                                 \
      (defined(INT64_MAX)) )
    D_LONG_LONG_DIAG_PUSH
    bool print(
        long long _value
    ) D_NOEXCEPT
    {
        return d_ostream_write_int64(&m_stream,
                                     static_cast<int64_t>(_value)) != 0;
    }

    bool print(
        unsigned long long _value
    ) D_NOEXCEPT
    {
        return d_ostream_write_uint64(&m_stream,
                                      static_cast<uint64_t>(_value)) != 0;
    }
    D_LONG_LONG_DIAG_POP
#endif  // long long

    /**
     * @brief Writes a size or an address-sized value in hexadecimal.
     *
     * @param[in] _value   the value.
     * @param[in] _digits  the width to pad to with zeros; 0 for none.
     * @return as write().
     */
    bool print_hex(
        std::size_t _value,
        int32_t     _digits = 0
    ) D_NOEXCEPT
    {
        return d_ostream_write_hex(&m_stream, _value, _digits) != 0;
    }

    // operator<<: the writes above, chained; check the stream once after
    DOutputStream& operator<<(
        const char* _cstr
    ) D_NOEXCEPT
    {
        (void)d_ostream_write_cstr(&m_stream, _cstr);

        return *this;
    }

    DOutputStream& operator<<(
        char _c
    ) D_NOEXCEPT
    {
        (void)d_ostream_write_char(&m_stream, _c);

        return *this;
    }

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    DOutputStream& operator<<(
        std::string_view _text
    ) D_NOEXCEPT
    {
        (void)write(_text);

        return *this;
    }
#endif  // C++17

    DOutputStream& operator<<(
        int _value
    ) D_NOEXCEPT
    {
        (void)print(_value);

        return *this;
    }

    DOutputStream& operator<<(
        unsigned int _value
    ) D_NOEXCEPT
    {
        (void)print(_value);

        return *this;
    }

    DOutputStream& operator<<(
        long _value
    ) D_NOEXCEPT
    {
        (void)print(_value);

        return *this;
    }

    DOutputStream& operator<<(
        unsigned long _value
    ) D_NOEXCEPT
    {
        (void)print(_value);

        return *this;
    }

#if ( (D_ENV_HAS_LONG_LONG) &&                                                 \
      (defined(INT64_MAX)) )
    D_LONG_LONG_DIAG_PUSH
    DOutputStream& operator<<(
        long long _value
    ) D_NOEXCEPT
    {
        (void)print(_value);

        return *this;
    }

    DOutputStream& operator<<(
        unsigned long long _value
    ) D_NOEXCEPT
    {
        (void)print(_value);

        return *this;
    }
    D_LONG_LONG_DIAG_POP
#endif  // long long

    // 3.1.5  Cursors
    //--------------------------------------------------------------------------
    /**
     * @brief Takes the window as an output iterator.
     *
     * @return the cursor; give it back before using the stream otherwise.
     */
    D_NODISCARD DOutputCursor cursor() D_NOEXCEPT
    {
        return DOutputCursor(&m_stream);
    }

    /**
     * @brief Gives a cursor's writes back.
     *
     * @param[in] _cursor  a cursor taken from this stream.
     * @return true while the stream is healthy.
     */
    bool give(
        DOutputCursor _cursor
    ) D_NOEXCEPT
    {
        return _cursor.give();
    }

    // 3.1.6  Flush and close
    //--------------------------------------------------------------------------
    /**
     * @brief Delivers or commits the window, then flushes the destination.
     *
     * @return true on success.
     */
    bool flush() D_NOEXCEPT
    {
        return d_ostream_flush(&m_stream) != 0;
    }

    /**
     * @brief Closes the stream now rather than at destruction.
     *
     * @return true when the stream never failed.
     */
    bool close() D_NOEXCEPT
    {
        return d_ostream_close(&m_stream) != 0;
    }

    // 3.1.7  Observers
    //--------------------------------------------------------------------------
    /**
     * @brief Whether the stream can be written: attached, healthy, open.
     *
     * @return true or false.
     */
    D_NODISCARD bool is_valid() const D_NOEXCEPT
    {
        return d_ostream_is_ok(&m_stream) != 0;
    }

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    /**
     * @brief is_valid(), for `if (out)`. From C++11, as an explicit
     *        conversion; below, is_valid() is the spelling (decision 3.2).
     *
     * @return true or false.
     */
    D_EXPLICIT_BOOL operator bool() const D_NOEXCEPT
    {
        return is_valid();
    }
#endif  // C++11

    // the first failure (enum dj_ostream_status) and its errno; bytes written
    D_NODISCARD int32_t status() const D_NOEXCEPT
    {
        return d_ostream_status(&m_stream);
    }

    D_NODISCARD int32_t last_errno() const D_NOEXCEPT
    {
        return d_ostream_last_errno(&m_stream);
    }

    D_NODISCARD std::size_t written() const D_NOEXCEPT
    {
        return d_ostream_written(&m_stream);
    }

    /**
     * @brief The bytes a fixed array holds, as of the last flush or close.
     *
     * @return the length; 0 for any other destination.
     */
    D_NODISCARD std::size_t fixed_length() const D_NOEXCEPT
    {
        return (m_stream.sink.context == &m_state.fixed)
                   ? d_ostream_fixed_length(&m_state.fixed)
                   : 0;
    }

    /**
     * @brief Whether a fixed array lost bytes that did not fit.
     *
     * @return true when it did; false for any other destination.
     */
    D_NODISCARD bool fixed_truncated() const D_NOEXCEPT
    {
        return ( (m_stream.sink.context == &m_state.fixed) &&
                 (d_ostream_fixed_truncated(&m_state.fixed) != 0) );
    }

    /**
     * @brief Asks the destination to describe itself.
     *
     * @param[out] _out  the description; every member 0 on failure.
     * @return true when the destination answered.
     */
    bool query(
        struct dj_ostream_info& _out
    ) const D_NOEXCEPT
    {
        return d_ostream_query(&m_stream, &_out) != 0;
    }

    /**
     * @brief Asks a console stream to have escape sequences interpreted.
     *
     * @return true when they will be; false for any other destination.
     */
    bool enable_vt() D_NOEXCEPT
    {
        return ( (m_stream.sink.context == &m_state.console) &&
                 (d_ostream_console_enable_vt(&m_state.console) != 0) );
    }

    // the C stream, for a C API or a fan-out
    D_NODISCARD struct dj_ostream* get() D_NOEXCEPT
    {
        return &m_stream;
    }

    D_NODISCARD const struct dj_ostream* get() const D_NOEXCEPT
    {
        return &m_stream;
    }

    /**
     * @brief A sink that writes through this stream, for a producer that
     *        takes a struct dj_pack_sink.
     *
     * @return the sink; valid while this object is neither moved nor
     *         destroyed.
     */
    D_NODISCARD struct dj_pack_sink as_sink() D_NOEXCEPT
    {
        return d_ostream_as_sink(&m_stream);
    }

protected:
#if (D_MOVE_ENABLED == 1)
    /**
     * @brief Moves the stream with the buffer _other carries, for a derived
     *        class that carries one too and re-points the stream at its own.
     *
     * @param[in,out] _other  the stream to take; left with no destination.
     * @param[in]     _carry  ignored; selects this constructor.
     */
    DOutputStream(
        DOutputStream&& _other,
        bool            _carry
    ) D_NOEXCEPT
        : m_default_buffer(D_NULLPTR),
          m_default_capacity(0)
    {
        (void)_carry;
        take(_other, true);
    }

    /**
     * @brief Closes this stream, then takes _other's with the buffer it
     *        carries, as the carrying constructor does.
     *
     * @param[in,out] _other  the stream to take; left with no destination.
     */
    void move_assign_carrying(
        DOutputStream& _other
    ) D_NOEXCEPT
    {
        if (this != &_other)
        {
            (void)d_ostream_close(&m_stream);
            take(_other, true);
        }

        return;
    }
#endif  // D_MOVE_ENABLED

    /**
     * @brief Lends the buffer a derived class carries to every transport
     *        attached from now on.
     *
     * @param[in] _buffer    the storage, alive as long as the object.
     * @param[in] _capacity  its size in bytes.
     */
    void use_default_buffer(
        unsigned char* _buffer,
        std::size_t    _capacity
    ) D_NOEXCEPT
    {
        m_default_buffer   = _buffer;
        m_default_capacity = _capacity;

        return;
    }

    /**
     * @brief Re-points every pointer of the stream that points into
     *        [_from, _from + _size) at the same offset from _to.
     *
     * @param[in] _from  the storage the pointers point into.
     * @param[in] _to    the storage they should point into.
     * @param[in] _size  its size in bytes.
     *
     * std::less, not <, compares pointers into unrelated objects: it is the
     * one comparison C++ gives a total order for.
     */
    void relocate(
        const void* _from,
        void*       _to,
        std::size_t _size
    ) D_NOEXCEPT
    {
        relocate_one(m_stream.cur, _from, _to, _size);
        relocate_one(m_stream.end, _from, _to, _size);
        relocate_one(m_stream.base, _from, _to, _size);
        relocate_one(m_stream.limit, _from, _to, _size);
        relocate_one(m_stream.buffer, _from, _to, _size);
        relocate_one(m_default_buffer, _from, _to, _size);

        unsigned char* context = static_cast<unsigned char*>(
            m_stream.sink.context);

        relocate_one(context, _from, _to, _size);
        m_stream.sink.context = context;

        return;
    }

private:
    // 3.1.8  Implementation
    //--------------------------------------------------------------------------
    // never copyable -- two owners would close one destination twice
    D_DELETED_FN(DOutputStream(const DOutputStream& _other))
    D_DELETED_FN(DOutputStream& operator=(const DOutputStream& _other))

    // relocate_one
    //   function: moves one pointer from _from's storage to _to's.
    static void relocate_one(
        unsigned char*& _pointer,
        const void*     _from,
        void*           _to,
        std::size_t     _size
    ) D_NOEXCEPT
    {
        const unsigned char* const from =
            static_cast<const unsigned char*>(_from);
        std::less<const unsigned char*> before;

        if ( (!before(_pointer, from)) &&
             (before(_pointer, from + _size)) )
        {
            _pointer = static_cast<unsigned char*>(_to) + (_pointer - from);
        }

        return;
    }

    // reset
    //   function: leaves the stream with no destination.
    void reset() D_NOEXCEPT
    {
        struct dj_pack_sink none;

        none.write   = D_NULLPTR;
        none.context = D_NULLPTR;

        (void)d_ostream_init(&m_stream, none, D_NULLPTR, 0);

        return;
    }

    // release
    //   function: closes the current destination, before another is attached.
    void release() D_NOEXCEPT
    {
        (void)d_ostream_close(&m_stream);

        return;
    }

    // settle
    //   function: after a transport is attached, lends it the buffer this
    // object carries, if any; passes the attach's answer through.
    bool settle(
        int32_t _attached
    ) D_NOEXCEPT
    {
        if ( (_attached != 0) &&
             (m_default_buffer != D_NULLPTR) )
        {
            (void)d_ostream_set_buffer(&m_stream,
                                       m_default_buffer,
                                       m_default_capacity);
        }

        return _attached != 0;
    }

#if (D_MOVE_ENABLED == 1)
    // take
    //   function: takes _other's stream and destination state, re-points what
    // pointed into _other at this object, and leaves _other with nothing.
    // Without _carry, a buffer _other carries is delivered first, since it
    // stays with _other; a buffer this object carries is then lent instead.
    void take(
        DOutputStream& _other,
        bool           _carry
    ) D_NOEXCEPT
    {
        if ( (!_carry) &&
             (_other.m_default_buffer != D_NULLPTR) &&
             (_other.m_stream.buffer == _other.m_default_buffer) )
        {
            (void)d_ostream_set_buffer(&_other.m_stream, D_NULLPTR, 0);
        }

        m_stream = _other.m_stream;
        m_state  = _other.m_state;

        if (_carry)
        {
            m_default_buffer   = _other.m_default_buffer;
            m_default_capacity = _other.m_default_capacity;
        }

        relocate(&_other, this, sizeof(DOutputStream));
        _other.reset();

        if ( (!_carry) &&
             (m_default_buffer != D_NULLPTR) )
        {
            (void)d_ostream_set_buffer(&m_stream,
                                       m_default_buffer,
                                       m_default_capacity);
        }

        return;
    }
#endif  // D_MOVE_ENABLED

    struct dj_ostream m_stream;

    // the destination's state, for the destinations that keep one here
    union
    {
        struct dj_ostream_console console;
        struct dj_ostream_fixed   fixed;
    } m_state;

    unsigned char* m_default_buffer;
    std::size_t    m_default_capacity;
};

// 3.2    DBufferedOutputStream
//------------------------------------------------------------------------------
// DBufferedOutputStream
//   class: a DOutputStream that carries a buffer of Size bytes and lends it to
// every transport it attaches, so the console, a file or a fan-out is written
// once per buffer rather than once per write. A storage destination has its
// own window and ignores it. Mixing its console output with printf's needs a
// flush() between them, as with any two buffered writers.
template<std::size_t Size = D_CFG_OSTREAM_BUFFER_SIZE>
class DBufferedOutputStream : public DOutputStream
{
public:
    DBufferedOutputStream() D_NOEXCEPT
        : DOutputStream()
    {
        use_default_buffer(m_storage, Size);
    }

    /**
     * @brief Closes the stream while the buffer it drains is still alive.
     */
    ~DBufferedOutputStream()
    {
        (void)close();
    }

#if (D_MOVE_ENABLED == 1)
    /**
     * @brief Moves the stream, its buffer's bytes with it. C++11 and later,
     *        ADDITIVE.
     *
     * @param[in,out] _other  the stream to take; left with no destination.
     */
    DBufferedOutputStream(
        DBufferedOutputStream&& _other
    ) D_NOEXCEPT
        : DOutputStream(static_cast<DOutputStream&&>(_other), true)
    {
        adopt_storage(_other);
    }

    /**
     * @brief Closes this stream, then takes _other's. C++11 and later,
     *        ADDITIVE.
     *
     * @param[in,out] _other  the stream to take; left with no destination.
     * @return *this.
     */
    DBufferedOutputStream& operator=(
        DBufferedOutputStream&& _other
    ) D_NOEXCEPT
    {
        if (this != &_other)
        {
            move_assign_carrying(_other);
            adopt_storage(_other);
        }

        return *this;
    }
#endif  // D_MOVE_ENABLED

private:
    D_DELETED_FN(DBufferedOutputStream(const DBufferedOutputStream& _other))
    D_DELETED_FN(DBufferedOutputStream& operator=(
                     const DBufferedOutputStream& _other))

#if (D_MOVE_ENABLED == 1)
    // adopt_storage
    //   function: copies the buffer's bytes and re-points the stream at this
    // object's buffer; the base has already taken everything else.
    void adopt_storage(
        DBufferedOutputStream& _other
    ) D_NOEXCEPT
    {
        for (std::size_t i = 0; i < Size; ++i)
        {
            m_storage[i] = _other.m_storage[i];
        }

        relocate(_other.m_storage, m_storage, Size);
        _other.use_default_buffer(_other.m_storage, Size);

        return;
    }
#endif  // D_MOVE_ENABLED

    unsigned char m_storage[Size];
};


//==============================================================================
// 4.  CALLABLE SINK
//==============================================================================


// 4.1    DOutputStreamSink
//------------------------------------------------------------------------------
// DOutputStreamSink
//   struct: a callable `(const char*, std::size_t)` over a stream -- the sink
// shape writer.hpp, printer.hpp and the document renderers take. Their sinks
// return nothing, so a failed write is kept by the stream instead: check ok(),
// or the stream's status, after the renderer has run.
struct DOutputStreamSink
{
    struct dj_ostream* stream;

    explicit
    DOutputStreamSink(
        struct dj_ostream* _stream
    ) D_NOEXCEPT
        : stream(_stream)
    {}

    explicit
    DOutputStreamSink(
        DOutputStream& _stream
    ) D_NOEXCEPT
        : stream(_stream.get())
    {}

    void operator()(
        const char* _data,
        std::size_t _size
    ) const D_NOEXCEPT
    {
        (void)d_ostream_write(stream, _data, _size);

        return;
    }

    /**
     * @brief Whether every write so far reached the stream: the stream's own
     *        health, which a failed write ends for good.
     *
     * @return true while the stream is healthy.
     */
    D_NODISCARD bool ok() const D_NOEXCEPT
    {
        return d_ostream_is_ok(stream) != 0;
    }
};


NS_END  // djinterp


#endif  // DJINTERP_UTIL_OUTPUT_OUTPUT_STREAM_HPP
