/*******************************************************************************
* djinterp [cpp]                                           output_stream_std.hpp
*
*   The standard library meets the output stream, both ways, at every level
* from C++98: a stream can write to a std::string or a std::ostream, and a
* std::ostream can write to a stream (DOutputStreambuf), so `os << x` reaches
* the console, a file, a dj_string or a document destination.
*
*   THE STREAMBUF'S PUT AREA IS THE STREAM'S WINDOW. DOutputStreambuf keeps no
* buffer: its put area is the window the stream lends, so `os << 'c'` is
* std::streambuf's own inline sputc into the destination's storage, and
* overflow, xsputn and sync hand the window back and take it again. While a
* std::ostream over it is in use, write to the stream through it alone, or
* flush it first (`os.flush()`, std::flush), which hands the window back.
*
*   NOTHING THROWS ACROSS THE C BOUNDARY. The kernel calls the std::string and
* std::ostream writers through function pointers, from C frames an exception
* cannot be thrown through. So each catches everything -- std::bad_alloc, an
* ostream whose exceptions() mask is set -- and reports a short write
* instead, which the stream records as D_OSTREAM_FAILED. They have C language
* linkage, because linkage is part of a function's type and the slots' types
* are declared in C; at least one compiler (Oracle's) holds that line.
*
*   A std::string IS A TRANSPORT, NOT A WINDOW. Writing in place into a
* std::string means resizing it first, which zero-fills what it adds, so each
* byte would be written twice anyway -- once by resize, once by the stream --
* and the string would be unsafe to read between flushes. Appending from the
* stream's buffer costs the same and leaves the string the caller's: give the
* stream a buffer (DBufferedOutputStream) and the string grows once per buffer.
*
*   LOCALE. iostreams format numbers with the stream's locale, which is the
* program's global one unless told otherwise. For output a machine will read,
* imbue std::locale::classic() on the std::ostream, or write numbers through
* DOutputStream::print, which no locale reaches.
*
*
* path:      /inc/djinterp/cpp/util/output/output_stream_std.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.11.05
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  WRITERS (internal)
    ------------------
    1.  std::string and std::ostream
2.  SINKS
    -----
    1.  make_sink, attach_std_string, attach_std_ostream
    2.  operator<< for std::string
3.  STREAM BUFFER
    -------------
    1.  DOutputStreambuf
*/

#ifndef DJINTERP_UTIL_OUTPUT_OUTPUT_STREAM_STD_HPP
#define DJINTERP_UTIL_OUTPUT_OUTPUT_STREAM_STD_HPP 1

// std
#include <cstddef>    // std::size_t
#include <limits>     // std::numeric_limits
#include <ostream>    // std::ostream
#include <streambuf>  // std::streambuf
#include <string>     // std::string
// djinterp
#include "../../djinterp.hpp"   // framework root
#include "./output_stream.hpp"  // DOutputStream, struct dj_ostream
// re_std
#include "../../../../re_std/cpp/cstdint/cstdint.hpp"  // int32_t


NS_DJINTERP


//==============================================================================
// 1.  WRITERS (internal)
//==============================================================================


// 1.1    std::string and std::ostream
//------------------------------------------------------------------------------
NS_INTERNAL

    extern "C"
    {
        /**
         * @brief Appends to a std::string; a throw becomes a short write.
         *
         * @param[in] _context  the std::string.
         * @param[in] _data     the bytes.
         * @param[in] _size     how many.
         * @return _size, or 0 when the append threw.
         */
        inline std::size_t d_internal_cxx_string_write(
            void*       _context,
            const void* _data,
            std::size_t _size
        )
        {
            try
            {
                static_cast<std::string*>(_context)->append(
                    static_cast<const char*>(_data),
                    _size);
            }
            catch (...)
            {
                return 0;
            }

            return _size;
        }

        /**
         * @brief Writes to a std::ostream, in pieces its streamsize can count;
         *        a failed or throwing stream becomes a short write.
         *
         * @param[in] _context  the std::ostream.
         * @param[in] _data     the bytes.
         * @param[in] _size     how many.
         * @return the bytes the std::ostream took.
         */
        inline std::size_t d_internal_cxx_ostream_write(
            void*       _context,
            const void* _data,
            std::size_t _size
        )
        {
            try
            {
                std::ostream* const out   =
                    static_cast<std::ostream*>(_context);
                const char* const   bytes = static_cast<const char*>(_data);
                const std::size_t   most  = static_cast<std::size_t>(
                    (std::numeric_limits<std::streamsize>::max)());
                std::size_t         at    = 0;

                while (at < _size)
                {
                    const std::size_t chunk = ((_size - at) > most)
                                                  ? most
                                                  : (_size - at);

                    out->write(bytes + at, static_cast<std::streamsize>(chunk));

                    if (out->fail())
                    {
                        return at;
                    }

                    at += chunk;
                }
            }
            catch (...)
            {
                return 0;
            }

            return _size;
        }

        /**
         * @brief Flushes a std::ostream.
         *
         * @param[in] _context  the std::ostream.
         * @return 1 or 0.
         */
        inline int32_t d_internal_cxx_ostream_flush(
            void* _context
        )
        {
            try
            {
                std::ostream* const out = static_cast<std::ostream*>(_context);

                out->flush();

                return out->fail() ? 0 : 1;
            }
            catch (...)
            {
                return 0;
            }
        }
    }

    /**
     * @brief The std::ostream destination's operations.
     *
     * @return the table; one object, function-local, shared by every unit.
     */
    inline const struct dj_ostream_ops* ostream_ops()
    {
        static const struct dj_ostream_ops ops =
        {
            sizeof(struct dj_ostream_ops),
            D_NULLPTR,
            &d_internal_cxx_ostream_flush,
            D_NULLPTR,
            D_NULLPTR
        };

        return &ops;
    }

NS_END  // internal


//==============================================================================
// 2.  SINKS
//==============================================================================


// 2.1    make_sink, attach_std_string, attach_std_ostream
//------------------------------------------------------------------------------
/**
 * @brief A sink that appends to a std::string.
 *
 * @param[in] _target  the string, which must outlive the sink.
 * @return the sink.
 */
inline struct dj_pack_sink make_sink(
    std::string& _target
)
{
    struct dj_pack_sink sink;

    sink.write   = &internal::d_internal_cxx_string_write;
    sink.context = &_target;

    return sink;
}

/**
 * @brief A sink that writes to a std::ostream.
 *
 * @param[in] _target  the stream, which must outlive the sink.
 * @return the sink.
 */
inline struct dj_pack_sink make_sink(
    std::ostream& _target
)
{
    struct dj_pack_sink sink;

    sink.write   = &internal::d_internal_cxx_ostream_write;
    sink.context = &_target;

    return sink;
}

/**
 * @brief Points a stream at a std::string, appending.
 *
 * @param[in,out] _stream  the stream.
 * @param[in]     _target  the string, which must outlive it.
 * @return true on success.
 */
inline bool attach_std_string(
    DOutputStream& _stream,
    std::string&   _target
)
{
    return _stream.attach(make_sink(_target));
}

/**
 * @brief Points a stream at a std::ostream, flushing through to it.
 *
 * @param[in,out] _stream  the stream.
 * @param[in]     _target  the std::ostream, which must outlive it.
 * @return true on success.
 */
inline bool attach_std_ostream(
    DOutputStream& _stream,
    std::ostream&  _target
)
{
    return _stream.attach(make_sink(_target), internal::ostream_ops());
}

// 2.2    operator<< for std::string
//------------------------------------------------------------------------------
/**
 * @brief Writes a std::string's bytes, embedded NULs included.
 *
 * @param[in,out] _stream  the stream.
 * @param[in]     _text    the string.
 * @return _stream.
 */
inline DOutputStream& operator<<(
    DOutputStream&     _stream,
    const std::string& _text
)
{
    (void)_stream.write(_text.data(), _text.size());

    return _stream;
}


//==============================================================================
// 3.  STREAM BUFFER
//==============================================================================


// 3.1    DOutputStreambuf
//------------------------------------------------------------------------------
// DOutputStreambuf
//   class: a std::streambuf whose put area is a stream's window, so
// `std::ostream os(&buf);` writes wherever the stream does, in place. The
// window is handed back on overflow, xsputn, sync and destruction; until
// then the stream's own pointers lag behind the put area.
class DOutputStreambuf : public std::streambuf
{
public:
    /**
     * @brief Takes a stream's window as the put area.
     *
     * @param[in,out] _stream  the stream, which must outlive the buffer.
     */
    explicit
    DOutputStreambuf(
        struct dj_ostream* _stream
    )
        : m_stream(_stream)
    {
        take();
    }

    /**
     * @brief Takes a DOutputStream's window as the put area.
     *
     * @param[in,out] _stream  the stream, which must outlive the buffer.
     */
    explicit
    DOutputStreambuf(
        DOutputStream& _stream
    )
        : m_stream(_stream.get())
    {
        take();
    }

    /**
     * @brief Hands the window back, so the stream holds every byte.
     */
    virtual ~DOutputStreambuf()
    {
        (void)give();
    }

protected:
    /**
     * @brief Writes one character the put area had no room for.
     *
     * @param[in] _c  the character, or end-of-file as a no-op.
     * @return _c, or end-of-file when the stream refused it.
     */
    virtual int_type overflow(
        int_type _c
    )
    {
        if (traits_type::eq_int_type(_c, traits_type::eof()))
        {
            return traits_type::not_eof(_c);
        }

        const bool ok = ( (give() != 0) &&
                          (d_ostream_write_char(m_stream,
                                                traits_type::to_char_type(_c))
                             != 0) );

        take();

        return ok ? _c : traits_type::eof();
    }

    /**
     * @brief Writes a run of characters, all or none.
     *
     * @param[in] _data  the characters.
     * @param[in] _size  how many.
     * @return _size, or 0 when the stream refused them.
     */
    virtual std::streamsize xsputn(
        const char*     _data,
        std::streamsize _size
    )
    {
        if (_size <= 0)
        {
            return 0;
        }

        const bool ok = ( (give() != 0) &&
                          (d_ostream_write(m_stream,
                                           _data,
                                           static_cast<std::size_t>(_size))
                             != 0) );

        take();

        return ok ? _size : 0;
    }

    /**
     * @brief Hands the window back and flushes the stream.
     *
     * @return 0, or -1 when the stream failed.
     */
    virtual int sync()
    {
        const bool ok = ( (give() != 0) &&
                          (d_ostream_flush(m_stream) != 0) );

        take();

        return ok ? 0 : -1;
    }

private:
    // give
    //   function: hands the put area back to the stream as its writes.
    int32_t give()
    {
        struct dj_ostream_cursor cursor;

        cursor.cur = reinterpret_cast<unsigned char*>(pptr());
        cursor.end = reinterpret_cast<unsigned char*>(epptr());

        return d_ostream_cursor_give(m_stream, cursor);
    }

    // take
    //   function: takes the stream's window as the put area.
    void take()
    {
        const struct dj_ostream_cursor cursor = d_ostream_cursor_take(m_stream);

        setp(reinterpret_cast<char*>(cursor.cur),
             reinterpret_cast<char*>(cursor.end));

        return;
    }

    D_DELETED_FN(DOutputStreambuf(const DOutputStreambuf& _other))
    D_DELETED_FN(DOutputStreambuf& operator=(const DOutputStreambuf& _other))

    struct dj_ostream* m_stream;
};


NS_END  // djinterp


#endif  // DJINTERP_UTIL_OUTPUT_OUTPUT_STREAM_STD_HPP
