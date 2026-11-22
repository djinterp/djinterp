/*******************************************************************************
* djinterp [c][utility]                                           ostream_file.c
*
*   The FILE* destination: a transport over fwrite and fflush, closed through
* c/filesys when the stream owns the file.  And the append destination: a
* descriptor opened to append, written with one system call per flush, so
* that several processes appending to one file never split each other's
* records.
*
*
* path:      /src/djinterp/c/util/ostream/ostream_file.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.11.17
*******************************************************************************/
#include "../../../../../inc/djinterp/c/util/ostream/ostream_file.h"
// std
#include <errno.h>   // errno, EINTR, EIO, ENOMEM, ENOSYS
#include <stdio.h>   // FILE, fwrite, fflush
#include <stdlib.h>  // malloc, free
// djinterp
#include "../../../../../inc/djinterp/c/util/ostream/ostream.h"
                                                // struct dj_ostream
#include "../../../../../inc/djinterp/c/util/sink_common.h"
                                                // struct dj_pack_sink
#include "../../../../../inc/djinterp/c/filesys/file_open.h"
                                // d_file_open_stream, d_file_close_stream
#include "../../../../../inc/djinterp/c/filesys/file_desc.h"
                                // d_file_open, d_file_close_fd, O_*
#include "../../../../../inc/djinterp/c/filesys/file_io.h"
                                // d_file_write_fd, ssize_t
#include "../../../../../inc/djinterp/config/c/filesys/cfg_file_io.h"
                                // D_CFG_FILE_WRITE_CREATE_MODE
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

// D_INTERNAL_OSTREAM_FILE_APPENDS
//   constant: 1 where c/filesys opens descriptors, and so the append
// destination exists; 0 under the ISO C backend, where appending goes through
// stdio and carries no promise about other appenders.
#if defined(O_APPEND)
    #define D_INTERNAL_OSTREAM_FILE_APPENDS 1
#else
    #define D_INTERNAL_OSTREAM_FILE_APPENDS 0
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

/**
 * @brief Leaves _os a well-defined stream that refused to open.
 *
 * @param[out] _os      the stream.
 * @param[in]  _path    the path asked for: null leaves D_OSTREAM_INVALID.
 * @param[in]  _reason  the open's errno, kept with D_OSTREAM_FAILED.
 * @return 0.
 */
static int32_t
d_internal_ostream_open_failed(
    struct dj_ostream* _os,
    const char*        _path,
    int                _reason
)
{
    struct dj_pack_sink none;

    none.write   = NULL;
    none.context = NULL;

    (void)d_ostream_init(_os, none, NULL, 0);

    if (_path != NULL)
    {
        _os->status     = D_OSTREAM_FAILED;
        _os->last_errno = (int32_t)_reason;
    }

    return 0;
}

#if (D_INTERNAL_OSTREAM_FILE_APPENDS == 1)

// dj_internal_ostream_append
//   struct: the append destination's context, a descriptor opened to append.
struct dj_internal_ostream_append
{
    int fd;
};

/**
 * @brief The append destination's write: one system call for the whole span.
 *
 * @param[in] _context  a struct dj_internal_ostream_append.
 * @param[in] _data     the bytes; a flush hands over the stream's whole buffer.
 * @param[in] _size     how many.
 * @return how many were written; fewer than _size is a failure, with errno
 *         saying why.
 *
 * The system appends each call whole: POSIX's O_APPEND, and on Windows a
 * handle with FILE_APPEND_DATA alone.  So the loop runs again only for a
 * short write, which a full disk or a signal can cause, or for a call a signal
 * interrupted before it wrote anything; either can leave a record split.
 */
static size_t
d_internal_ostream_append_write(
    void*       _context,
    const void* _data,
    size_t      _size
)
{
    const struct dj_internal_ostream_append* const append =
        (const struct dj_internal_ostream_append*)_context;
    const unsigned char* const bytes = (const unsigned char*)_data;
    size_t                     done  = 0u;

    while (done < _size)
    {
        const ssize_t wrote = d_file_write_fd(append->fd,
                                              bytes + done,
                                              _size - done);

        if (wrote > 0)
        {
            done += (size_t)wrote;
        }
        else if ( (wrote < 0) &&
                  (errno == EINTR) )
        {
            continue;
        }
        else
        {
            // nothing written and no error: say why the stream stopped
            if (wrote == 0)
            {
                errno = EIO;
            }

            break;
        }
    }

    return done;
}

/**
 * @brief The append destination's flush: nothing is held here.
 *
 * @param[in] _context  unused.
 * @return 1.
 */
static int32_t
d_internal_ostream_append_flush(
    void* _context
)
{
    (void)_context;

    return 1;
}

/**
 * @brief Finishes the append destination, which the stream always owns.
 *
 * @param[in] _context  a struct dj_internal_ostream_append.
 * @param[in] _release  non-zero when the stream owns it; always, here.
 * @return 1 or 0.
 */
static int32_t
d_internal_ostream_append_close(
    void*   _context,
    int32_t _release
)
{
    struct dj_internal_ostream_append* const append =
        (struct dj_internal_ostream_append*)_context;
    int32_t                                  result = 1;

    if (_release != 0)
    {
        result = (d_file_close_fd(append->fd) == 0) ? 1 : 0;
        free(append);
    }

    return result;
}

// d_internal_ostream_append_ops
//   table: the operations of a file opened here to append.
static const struct dj_ostream_ops d_internal_ostream_append_ops =
{
    sizeof(struct dj_ostream_ops),
    NULL,
    &d_internal_ostream_append_flush,
    &d_internal_ostream_append_close,
    &d_internal_ostream_binary_file_query
};

/**
 * @brief Opens _path to append, as the append destination.
 *
 * @param[out] _os    the stream.
 * @param[in]  _path  the file's path; not null.
 * @return 1; 0 when the open failed, with errno set and _os untouched.
 *
 * Created with D_CFG_FILE_WRITE_CREATE_MODE, as c/filesys creates every file
 * it writes.  ENOSYS means the build's c/filesys has no descriptors, and the
 * caller falls back to stdio.
 */
static int32_t
d_internal_ostream_open_append(
    struct dj_ostream* _os,
    const char*        _path
)
{
    struct dj_internal_ostream_append* append;
    struct dj_pack_sink                sink;
    const int                          fd =
        d_file_open(_path,
                    O_WRONLY | O_CREAT | O_APPEND,
                    (int)D_CFG_FILE_WRITE_CREATE_MODE);

    if (fd < 0)
    {
        return 0;
    }

    append = (struct dj_internal_ostream_append*)malloc(sizeof(*append));

    if (append == NULL)
    {
        (void)d_file_close_fd(fd);
        errno = ENOMEM;

        return 0;
    }

    append->fd   = fd;
    sink.write   = &d_internal_ostream_append_write;
    sink.context = append;

    return d_ostream_init(_os,
                          sink,
                          &d_internal_ostream_append_ops,
                          D_OSTREAM_OWNS_CONTEXT);
}

#endif  // D_INTERNAL_OSTREAM_FILE_APPENDS == 1


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
  To append, the append destination; where c/filesys has no descriptors, or
to truncate, a FILE* opened here.  A failed open is D_OSTREAM_FAILED, not
D_OSTREAM_INVALID: the arguments were fine and the file system refused, and
the stream keeps the reason. The refused stream is made by d_ostream_init
first, so it is well-defined, and its status is then overwritten with the
open's.
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

#if (D_INTERNAL_OSTREAM_FILE_APPENDS == 1)
    if ( (_append != 0) &&
         (_path != NULL) )
    {
        if (d_internal_ostream_open_append(_os, _path) != 0)
        {
            return 1;
        }

        // no descriptors in this build: stdio's append, below
        if (errno != ENOSYS)
        {
            return d_internal_ostream_open_failed(_os, _path, errno);
        }
    }
#endif

    FILE* const file = (_path != NULL)
                           ? d_file_open_stream(_path,
                                                (_append != 0) ? "ab" : "wb")
                           : NULL;

    if (file == NULL)
    {
        return d_internal_ostream_open_failed(_os, _path, errno);
    }

    struct dj_pack_sink sink;

    sink.write   = &d_internal_ostream_file_write;
    sink.context = file;

    return d_ostream_init(_os,
                          sink,
                          &d_internal_ostream_binary_file_ops,
                          D_OSTREAM_OWNS_CONTEXT);
}
