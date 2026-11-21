/*******************************************************************************
* djinterp [cpp]                                                    compress.cpp
*
*   The C++ facade's dispatch leaves -- three adapters, and nothing else.
*
*   WHAT THIS FILE USED TO BE:
*   Every backend: zlib, bzip2, liblzma, zstd, lz4, brotli, each with an
* encoder and a decoder, each behind its own #if.  All of it is gone.  It now
* lives once in compress_common.c, which the C fork compiles too, so a fix to
* the zstd path is a fix to both languages rather than to whichever one someone
* remembered.  That is the whole point of the re-basing: not fewer lines here,
* but ONE place where a compression result is computed.
*
*   WHAT AN ADAPTER DOES:
*   Exactly two things -- turn a byte_blob into a dj_pack_sink, and call the
* kernel.  Nothing here decides a default, selects a backend, or knows what a
* codec is.  If a change to this file would move an output byte, the change
* belongs in the kernel instead.
*
*   ONE PASS, NOT TWO:
*   The kernel's buffer form measures and then produces, which costs a full
* encode twice.  This file does not use it.  A byte_blob can grow, so the
* adapters bind a sink that appends directly and the encoder runs once -- which
* is what keeps the C++ facade from being slower than a native C++
* implementation would have been.  A wrapper that costs a pass over what it
* wraps is a defect, not a tradeoff.
*
*   THE AUTOLINK INCLUDE MUST STAY.  env_compress_link.h turns each detected
* library into a link request on MSVC, and it reacts to whichever env headers
* the translation unit has already included -- so it goes AFTER the facade, and
* only in an implementation unit.  compress_common.c carries the same include
* for the C fork.
*
*
* path:      /src/djinterp/cpp/util/compress/compress.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.05.23
*                                                            revised: 2026.11.18
*******************************************************************************/

#include "../../../../../inc/djinterp/cpp/util/compress/compress.hpp"  // own
// std
#include <algorithm>  // std::min
#include <vector>     // std::vector
// djinterp
#include "../../../../../inc/djinterp/env/util/compress/env_compress_link.h"


NS_DJINTERP

namespace internal
{

// blob_sink_write
//   function: the sink write that appends into a byte_blob.  Never refuses, so
// a short write -- which the kernel treats as a sink failure -- can only happen
// if std::string::append throws, and that propagates rather than returning.
//
//   This is the whole reason dj_pack_sink carries a context pointer.
// djinterp.h's fn_write does not, so it could not have driven a growable
// destination without global mutable state.
static
std::size_t blob_sink_write(
    void*       _context,
    const void* _data,
    std::size_t _size
)
{
    byte_blob* out = static_cast<byte_blob*>(_context);

    if (out == 0)
    {
        return 0;
    }

    out->append(static_cast<const char*>(_data), _size);

    return _size;
}

// bind_blob_sink
//   function: a dj_pack_sink that appends into _out.  Borrows _out, which must
// outlive the call.
static
dj_pack_sink bind_blob_sink(
    byte_blob* _out
)
{
    dj_pack_sink sink;

    sink.write   = &blob_sink_write;
    sink.context = _out;

    return sink;
}

/*
compress_buffer
  Compress [_in, _in + _n) with codec _id into _out.

  _out is CLEARED first.  The kernel appends, and a caller reusing a blob
across calls would otherwise silently concatenate -- which produces a stream
that is still valid for its first member and wrong for everything after it.

Parameter(s):
  _id:  the codec.
  _in:  the input bytes; may be null when _n is 0.
  _n:   how many.
  _opt: the tuning; UNSET knobs are resolved by the kernel.
  _out: receives the compressed stream.
Return:
  status_ok, or the kernel's status unchanged.
*/
status compress_buffer(
    DCodecId                _id,
    const char*             _in,
    std::size_t             _n,
    const compress_options& _opt,
    byte_blob&              _out
)
{
    _out.clear();

    return d_compress_to_sink(_id, _in, _n, &_opt,
                              bind_blob_sink(&_out), 0);
}

/*
decompress_buffer
  Decompress [_in, _in + _n) with codec _id into _out.

Parameter(s):
  _id:  the codec the stream is in.
  _in:  the compressed bytes.
  _n:   how many.
  _out: receives the decompressed bytes.
Return:
  status_ok, or the kernel's status unchanged.
*/
status decompress_buffer(
    DCodecId    _id,
    const char* _in,
    std::size_t _n,
    byte_blob&  _out
)
{
    _out.clear();

    return d_decompress_to_sink(_id, _in, _n, bind_blob_sink(&_out), 0);
}

/*
compress_gzip_buffer
  Compress [_in, _in + _n) as gzip into _out, writing _header's name, time
and OS byte.  _out is CLEARED first, as compress_buffer's is.

Parameter(s):
  _in:     the input bytes; may be null when _n is 0.
  _n:      how many.
  _opt:    the tuning; UNSET knobs are resolved by the kernel.
  _header: the name (no NUL), the time and the OS byte to write.
  _out:    receives the gzip stream.
Return:
  status_invalid_argument for a name holding a NUL; otherwise status_ok, or
the kernel's status unchanged.
*/
status compress_gzip_buffer(
    const char*             _in,
    std::size_t             _n,
    const compress_options& _opt,
    const gzip_header&      _header,
    byte_blob&              _out
)
{
    struct dj_gzip_header header = D_GZIP_HEADER_NONE;

    _out.clear();

    if (_header.name.find('\0') != std::string::npos)
    {
        return status_invalid_argument;
    }

    header.name.data   = _header.name.empty() ? NULL : _header.name.data();
    header.name.length = _header.name.size();
    header.mtime       = _header.mtime;
    header.os          = _header.os;

    return d_compress_gzip_to_sink(_in, _n, &_opt, &header,
                                   bind_blob_sink(&_out), 0);
}

/*
decompress_gzip_buffer
  Decompress a gzip stream's first member into _out, and read its header
into _header.  The name is read into a buffer that grows, by fours, until
it holds the name or reaches gzip_name_max; each growth reads the stream
again, which a name past 256 bytes alone costs.

Parameter(s):
  _in:     the gzip stream.
  _n:      how many bytes.
  _header: receives the name, the time, the OS byte, and whether the name
           was cut at gzip_name_max.
  _out:    receives the decompressed bytes.
Return:
  status_ok, or the kernel's status unchanged, with _header a default one.
*/
status decompress_gzip_buffer(
    const char*  _in,
    std::size_t  _n,
    gzip_header& _header,
    byte_blob&   _out
)
{
    std::vector<char> name(256u, '\0');

    for (;;)
    {
        struct dj_gzip_header header = D_GZIP_HEADER_NONE;
        status                s;

        _out.clear();
        s = d_decompress_gzip_to_sink(_in, _n, &header, &name[0],
                                      name.size(), bind_blob_sink(&_out), 0);

        if (s != status_ok)
        {
            _header = gzip_header();

            return s;
        }

        if ( (header.name_truncated != 0) &&
             (name.size() < gzip_name_max + 1u) )
        {
            name.resize(std::min<std::size_t>(name.size() * 4u,
                                              gzip_name_max + 1u));

            continue;
        }

        _header.name.assign((header.name.data) ? header.name.data : "",
                            header.name.length);
        _header.mtime          = header.mtime;
        _header.os             = header.os;
        _header.name_truncated = (header.name_truncated != 0);

        return s;
    }
}

/*
codec_available
  Whether this build can both compress and decompress with _id.

Parameter(s):
  _id: the codec to query.
Return:
  true when the codec is fully usable.
*/
bool codec_available(
    DCodecId _id
)
{
    return (d_codec_is_available(_id) != 0);
}

}  // namespace internal

NS_END  // djinterp
