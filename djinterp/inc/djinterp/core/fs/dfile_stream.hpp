/******************************************************************************
* djinterp [core]                                             dfile_stream.hpp
*
* Buffered (FILE*) file I/O for the djinterp C++ file toolkit.
*   This is the C++ counterpart to dfile_stream.h. It provides the stream
* class: a move-only RAII owner of a FILE* that opens on construction, closes
* on destruction, and exposes the buffered-I/O surface (positioning,
* truncation, flushing, durability). Whole-file convenience helpers and the
* temporary-file utilities are provided as free functions in djinterp::file.
*   Every method forwards inline to the extern "C" implementation, so using
* the class costs nothing over calling the C functions directly. The secure
* (_s) open variants of the C API are represented here by the RAII object
* itself: a stream that failed to open simply tests false.
*
* 
* path:      /inc/djinterp/cpp/io/file/dfile_stream.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.21
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    class stream
      -----------
      1.  construction / destruction (fopen, fdopen, tmpfile, adopt, move)
      2.  observers (is_open, get, descriptor, tell)
      3.  ownership (release, reset, reopen)
      4.  I/O (read, write, seek, truncate, flush, sync)

II.   WHOLE-FILE HELPERS
      ------------------
      1.  read_all      (read entire file into a buffer)
      2.  write_all     (write a buffer to a file, truncating)
      3.  append_all    (append a buffer to a file)

III.  TEMPORARY-FILE HELPERS
      ----------------------
      1.  make_temp      (create+open a temp file from a template)
      2.  temp_name      (generate a unique temp filename)
      3.  temp_directory (query the system temp directory)
*/

#ifndef DJINTERP_FILE_STREAM_
#define DJINTERP_FILE_STREAM_ 1

#include "../../../c/io/file/dfile_stream.h"
#include "../../../c/io/file/dfile_descriptor.h"  // for d_fileno
#include "./dfile_common.hpp"


NS_DJINTERP
D_NAMESPACE(file)

// I. class stream

// stream
//   type: move-only RAII wrapper owning a FILE*; opens on construction and
// closes on destruction.
class stream
{
public:
    // construction / destruction

    // construct an empty (closed) stream.
    D_INLINE stream() noexcept
        : m_stream(nullptr)
    {}

    // open _filename with mode _mode (see d_fopen).
    D_INLINE stream(const char* _filename, const char* _mode) noexcept
        : m_stream(d_fopen(_filename, _mode))
    {}

    // adopt an already-open FILE*; the stream takes ownership.
    D_INLINE explicit stream(FILE* _handle) noexcept
        : m_stream(_handle)
    {}

    stream(const stream&)            = delete;
    stream& operator=(const stream&) = delete;

    D_INLINE stream(stream&& _other) noexcept
        : m_stream(_other.m_stream)
    {
        _other.m_stream = nullptr;
    }

    D_INLINE stream& operator=(stream&& _other) noexcept
    {
        // guard against self-move before releasing our own handle
        if (this != &_other)
        {
            reset(_other.m_stream);
            _other.m_stream = nullptr;
        }

        return *this;
    }

    D_INLINE ~stream() noexcept
    {
        reset();
    }

    // wrap a file descriptor as a stream (see d_fdopen).
    D_INLINE static stream from_descriptor(int _fd, const char* _mode) noexcept
    {
        return stream(d_fdopen(_fd, _mode));
    }

    // create an anonymous temporary stream (see d_tmpfile).
    D_INLINE static stream temporary() noexcept
    {
        return stream(d_tmpfile());
    }

    // observers

    D_NO_DISCARD D_INLINE bool is_open() const noexcept
    {
        return m_stream != nullptr;
    }

    D_NO_DISCARD D_INLINE explicit operator bool() const noexcept
    {
        return m_stream != nullptr;
    }

    D_NO_DISCARD D_INLINE FILE* get() const noexcept
    {
        return m_stream;
    }

    // underlying file descriptor of the stream (see d_fileno).
    D_NO_DISCARD D_INLINE int descriptor() const noexcept
    {
        return d_fileno(m_stream);
    }

    // current stream position (see d_ftello).
    D_NO_DISCARD D_INLINE offset tell() const noexcept
    {
        return d_ftello(m_stream);
    }

    // ownership

    // relinquish ownership and return the raw handle.
    D_NO_DISCARD D_INLINE FILE* release() noexcept
    {
        FILE* handle;

        handle    = m_stream;
        m_stream  = nullptr;

        return handle;
    }

    // close any current handle and adopt _handle (default: none).
    D_INLINE void reset(FILE* _handle = nullptr) noexcept
    {
        // close the existing handle before replacing it
        if (m_stream)
        {
            fclose(m_stream);
        }

        m_stream = _handle;

        return;
    }

    // reopen this stream on a new file (see d_freopen).
    D_INLINE bool reopen(const char* _filename, const char* _mode) noexcept
    {
        m_stream = d_freopen(_filename, _mode, m_stream);

        return m_stream != nullptr;
    }

    // I/O

    // read up to _size bytes into _buf; returns the byte count read.
    D_INLINE size_t read(void*  _buf,
                         size_t _size) noexcept
    {
        return fread(_buf, 1, _size, m_stream);
    }

    // write _size bytes from _buf; returns the byte count written.
    D_INLINE size_t write(const void* _buf,
                          size_t      _size) noexcept
    {
        return fwrite(_buf, 1, _size, m_stream);
    }

    // reposition the stream (see d_fseeko).
    D_INLINE int seek(offset      _offset,
                      seek_origin _origin) noexcept
    {
        return d_fseeko(m_stream, _offset, to_int(_origin));
    }

    // truncate the stream to _length bytes (see d_ftruncate_stream).
    D_INLINE int truncate(offset _length) noexcept
    {
        return d_ftruncate_stream(m_stream, _length);
    }

    // flush buffered output to the OS (see d_fflush).
    D_INLINE int flush() noexcept
    {
        return d_fflush(m_stream);
    }

    // flush and force the OS to commit to disk (see d_fsync_stream).
    D_INLINE int sync() noexcept
    {
        return d_fsync_stream(m_stream);
    }

private:
    FILE* m_stream;
};


// II. whole-file helpers

// read_all
//   read the entire file at _path into a freshly allocated buffer, storing
// the byte count in *_size. The caller owns the returned buffer.
D_NO_DISCARD D_INLINE
void*
read_all(const char* _path,
         size_t*     _size)
{
    return d_fread_all(_path, _size);
}

// write_all
//   write _size bytes of _data to _path, truncating any existing contents.
D_INLINE
int
write_all(const char* _path,
          const void* _data,
          size_t      _size)
{
    return d_fwrite_all(_path, _data, _size);
}

// append_all
//   append _size bytes of _data to _path.
D_INLINE
int
append_all(const char* _path,
           const void* _data,
           size_t      _size)
{
    return d_fappend_all(_path, _data, _size);
}


// III. temporary-file helpers

// make_temp
//   create and open a unique temporary file from _template (see d_mkstemp);
// returns an open file descriptor, or -1 on failure.
D_NO_DISCARD D_INLINE
int
make_temp(char* _template)
{
    return d_mkstemp(_template);
}

// temp_name
//   generate a unique temporary filename into _s (see d_tmpnam_s).
D_INLINE
int
temp_name(char*  _s,
          size_t _maxsize)
{
    return d_tmpnam_s(_s, _maxsize);
}

// temp_directory
//   write the system temporary directory into _buf (see d_tempdir).
D_INLINE
char*
temp_directory(char*  _buf,
               size_t _bufsize)
{
    return d_tempdir(_buf, _bufsize);
}

NS_END  // file
NS_END  // djinterp


#endif  // DJINTERP_FILE_STREAM_
