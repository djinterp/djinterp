/*******************************************************************************
* djinterp [c]                                                    ostream_file.c
*
*   The FILE* destination: a transport over fwrite and fflush, closed through
* c/filesys when the stream owns the file.
*
*
* path:      /src/djinterp/c/util/ostream/ostream_file.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.11.05
*******************************************************************************/
#include "../../../../../inc/djinterp/c/util/ostream/ostream_file.h"
// std
#include <errno.h>  // errno
#include <stdio.h>  // FILE, fwrite, fflush
// djinterp
#include "../../../../../inc/djinterp/c/util/ostream/ostream.h"
                                                // struct dj_ostream
#include "../../../../../inc/djinterp/c/util/sink_common.h"
                                                // struct dj_pack_sink
#include "../../../../../inc/djinterp/c/filesys/file_open.h"
                                // d_file_open_stream, d_file_close_stream
#include "../../../../../inc/djinterp/config/c/filesys/cfg_file_common.h"
                                // D_CFG_FILE_HAS_WIN32
// re_std
#include "../../../../../inc/re_std/c/cstdint/dstdint.h"  // int32_t


// D_INTERNAL_OSTREAM_FILE_TRANSLATES
//   constant: 1 where a FILE* in text mode rewrites newlines, which is where a
// borrowed FILE* cannot be called binary-safe.
#if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    #define D_INTERNAL_OSTREAM_FILE_TRANSLATES 1
#else
    #define D_INTERNAL_OSTREAM_FILE_TRANSLATES 0
#endif


/**
 * @brief The FILE* destination's write.
 *
 * @param[in] _context  a FILE*.
 * @param[in] _data     the bytes.
 * @param[in] _size     how many.
 * @return how many fwrite took; fewer than _size is a failure, and on POSIX
 *         leaves errno saying why.
 */
static size_t
d_internal_ostream_file_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    return fwrite(_data, (size_t)1u, _size, (FILE*)_context);
}

/**
 * @brief The FILE* destination's flush.
 *
 * @param[in] _context  a FILE*.
 * @return 1 or 0.
 */
static int32_t
d_internal_ostream_file_flush(
    void* _context
)
{
    return (fflush((FILE*)_context) == 0) ? 1 : 0;
}

/**
 * @brief Finishes a FILE* destination.
 *
 * @param[in] _context  a FILE*.
 * @param[in] _release  non-zero when the stream owns the file.
 * @return 1 or 0.
 *
 * Closing flushes too, so an owned file is only closed; a borrowed one is
 * only flushed.
 */
static int32_t
d_internal_ostream_file_close(
    void*   _context,
    int32_t _release
)
{
    if (_release != 0)
    {
        return (d_file_close_stream((FILE*)_context) == 0) ? 1 : 0;
    }

    return d_internal_ostream_file_flush(_context);
}

/**
 * @brief Describes a borrowed FILE*.
 *
 * @param[in]  _context  a FILE*.
 * @param[out] _out      the description; arrives zeroed.
 * @return 1.
 *
 * Binary-safe only where no C library translates, since the caller chose the
 * mode and this unit cannot ask.
 */
static int32_t
d_internal_ostream_file_query(
    void*                   _context,
    struct dj_ostream_info* _out
)
{
    (void)_context;

    _out->is_binary_safe = (D_INTERNAL_OSTREAM_FILE_TRANSLATES == 0) ? 1 : 0;

    return 1;
}

/**
 * @brief Describes a FILE* opened here, in binary mode.
 *
 * @param[in]  _context  a FILE*.
 * @param[out] _out      the description; arrives zeroed.
 * @return 1.
 */
static int32_t
d_internal_ostream_binary_file_query(
    void*                   _context,
    struct dj_ostream_info* _out
)
{
    (void)_context;

    _out->is_binary_safe = 1;

    return 1;
}

// d_internal_ostream_file_ops
//   table: the operations of a FILE* the caller opened.
static const struct dj_ostream_ops d_internal_ostream_file_ops =
{
    sizeof(struct dj_ostream_ops),
    NULL,
    &d_internal_ostream_file_flush,
    &d_internal_ostream_file_close,
    &d_internal_ostream_file_query
};

// d_internal_ostream_binary_file_ops
//   table: the operations of a FILE* opened here.
static const struct dj_ostream_ops d_internal_ostream_binary_file_ops =
{
    sizeof(struct dj_ostream_ops),
    NULL,
    &d_internal_ostream_file_flush,
    &d_internal_ostream_file_close,
    &d_internal_ostream_binary_file_query
};

/*
d_ostream_init_file
  A null FILE* reaches d_ostream_init as a null write, which refuses it.
*/
int32_t
d_ostream_init_file(
    struct dj_ostream* _os,
    FILE*              _file,
    int32_t            _flags
)
{
    struct dj_pack_sink sink;

    sink.write   = (_file != NULL) ? &d_internal_ostream_file_write : NULL;
    sink.context = _file;

    return d_ostream_init(_os, sink, &d_internal_ostream_file_ops, _flags);
}

/*
d_ostream_open_file
  A failed open is D_OSTREAM_FAILED, not D_OSTREAM_INVALID: the arguments were
fine and the file system refused, and the stream keeps the reason. The
refused stream is made by d_ostream_init first, so it is well-defined, and
its status is then overwritten with the open's.
*/
int32_t
d_ostream_open_file(
    struct dj_ostream* _os,
    const char*        _path,
    int32_t            _append
)
{
    if (_os == NULL)
    {
        return 0;
    }

    FILE* const file = (_path != NULL)
                           ? d_file_open_stream(_path,
                                                (_append != 0) ? "ab" : "wb")
                           : NULL;

    if (file == NULL)
    {
        const int           reason = errno;
        struct dj_pack_sink none;

        none.write   = NULL;
        none.context = NULL;

        (void)d_ostream_init(_os, none, NULL, 0);

        if (_path != NULL)
        {
            _os->status     = D_OSTREAM_FAILED;
            _os->last_errno = (int32_t)reason;
        }

        return 0;
    }

    struct dj_pack_sink sink;

    sink.write   = &d_internal_ostream_file_write;
    sink.context = file;

    return d_ostream_init(_os,
                          sink,
                          &d_internal_ostream_binary_file_ops,
                          D_OSTREAM_OWNS_CONTEXT);
}
