/*******************************************************************************
* djinterp [c]                                                         file_io.h
*
* Moving bytes -- through a descriptor, at an offset, or a whole file at once.
*   Read and write live together because they are one mechanism seen from two
* directions: the same chunking rule, the same short-transfer retry, the same
* open/size/close scaffolding behind the whole-file helpers. Splitting them
* would duplicate all of it to buy a separation nothing needs.
*   Descriptor LIFECYCLE is file_desc.h; this module only transfers through a
* descriptor the caller already holds. The whole-file helpers open by path and
* so depend on file_open.h.
*   Whether d_file_write_all replaces a file atomically and whether it waits
* for durable storage are build-time decisions -- see cfg_file_io.h, and
* D_FILE_WRITE_IS_ATOMIC / D_FILE_WRITE_IS_DURABLE to query them.
*
*
* path:      /inc/djinterp/c/fs/file_io.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TRANSFER
    --------
    1.  Descriptor reads
    2.  Descriptor writes
    3.  Whole-file reads
    4.  Whole-file writes
*/

#ifndef DJINTERP_C_FS_FILE_IO_H
#define DJINTERP_C_FS_FILE_IO_H 1

// std
#include <stddef.h>  // size_t
#include <stdio.h>   // FILE
// djinterp
#include "./file_common.h"                  // fs foundation, ssize_t, d_off_t
#include "./file_open.h"                    // streams, which the whole-file
                                            // helpers are built on
#include "../../config/c/fs/cfg_file_io.h"  // module configuration
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TRANSFER
//==============================================================================
// The descriptor functions fail with ENOSYS on the ISO C backend, which has
// no descriptors; the whole-file functions work on every backend.


// 1.1    Descriptor reads
//------------------------------------------------------------------------------
/**
 * @brief Reads from a descriptor (POSIX read equivalent).
 *
 * @note Like read(2) it may return fewer bytes than requested without that
 *       being an error; d_file_read_full_fd writes the loop for you.
 *
 * @param[in]  _fd     an open descriptor.
 * @param[out] _buf    receives the bytes; holds at least `_count`.
 * @param[in]  _count  the most bytes to read.
 * @return the number of bytes read, 0 at end of file, or -1 on failure with
 *         errno set.
 */
ssize_t d_file_read_fd(int    _fd,
                       void*  _buf,
                       size_t _count);
/**
 * @brief Reads until `_count` bytes have arrived or the source ends.
 *
 * @param[in]  _fd     an open descriptor.
 * @param[out] _buf    receives the bytes; holds at least `_count`.
 * @param[in]  _count  the number of bytes wanted.
 * @return `_count`, fewer if end of file came first, or -1 on failure with
 *         errno set.
 */
ssize_t d_file_read_full_fd(int    _fd,
                            void*  _buf,
                            size_t _count);
/**
 * @brief Reads from a fixed offset without moving the descriptor's position.
 *
 * @warning Atomic only where the platform has pread. Elsewhere it is emulated
 *          by seek, read and seek back, and two threads sharing the
 *          descriptor can interleave; D_FILE_READ_PREAD_IS_ATOMIC says which.
 *
 * @param[in]  _fd      an open, seekable descriptor.
 * @param[out] _buf     receives the bytes; holds at least `_count`.
 * @param[in]  _count   the most bytes to read.
 * @param[in]  _offset  the absolute offset to read from; not negative.
 * @return the number of bytes read, 0 at end of file, or -1 on failure with
 *         errno set.
 */
ssize_t d_file_pread_fd(int     _fd,
                        void*   _buf,
                        size_t  _count,
                        d_off_t _offset);

// 1.2    Descriptor writes
//------------------------------------------------------------------------------
/**
 * @brief Writes to a descriptor (POSIX write equivalent).
 *
 * @note Like write(2) it may accept fewer bytes than offered without that
 *       being an error; d_file_write_full_fd writes the loop for you.
 *
 * @param[in] _fd     an open descriptor.
 * @param[in] _buf    the bytes to write; holds at least `_count`.
 * @param[in] _count  the number of bytes to offer.
 * @return the number of bytes written, or -1 on failure with errno set.
 */
ssize_t d_file_write_fd(int         _fd,
                        const void* _buf,
                        size_t      _count);
/**
 * @brief Writes until every byte has been accepted.
 *
 * @warning On failure some bytes may already have been written. A
 *          destination that stops accepting bytes without an error fails
 *          with EIO rather than hanging the loop.
 *
 * @param[in] _fd     an open descriptor.
 * @param[in] _buf    the bytes to write; holds at least `_count`.
 * @param[in] _count  the number of bytes to write.
 * @return `_count`, or -1 on failure with errno set.
 */
ssize_t d_file_write_full_fd(int         _fd,
                             const void* _buf,
                             size_t      _count);
/**
 * @brief Writes at a fixed offset without moving the descriptor's position.
 *
 * @warning Atomic only where the platform has pwrite; elsewhere it is
 *          emulated by seek, write and seek back, and is not.
 *
 * @param[in] _fd      an open, seekable descriptor.
 * @param[in] _buf     the bytes to write; holds at least `_count`.
 * @param[in] _count   the number of bytes to offer.
 * @param[in] _offset  the absolute offset to write at; not negative.
 * @return the number of bytes written, or -1 on failure with errno set.
 */
ssize_t d_file_pwrite_fd(int         _fd,
                         const void* _buf,
                         size_t      _count,
                         d_off_t     _offset);

// 1.3    Whole-file reads
//------------------------------------------------------------------------------
/**
 * @brief Reads an entire file into one fresh allocation.
 *
 * @note The file is opened in binary mode, so the size matches the file's own
 *       on every platform; for translated text, open the stream yourself and
 *       call d_file_read_all_stream. A source that reports no size, such as a
 *       /proc entry, is read until end of file.
 *
 * @param[in]  _path  the file to read.
 * @param[out] _size  receives the number of payload bytes; may be `NULL`.
 * @return the buffer -- NUL-terminated past `*_size` when
 *         D_CFG_FILE_READ_NUL_TERMINATE is on -- or `NULL` on failure with
 *         errno set.
 * @post The caller owns the buffer and releases it with the configured
 *       deallocator (D_CFG_FILE_FREE).
 */
void*   d_file_read_all(const char* _path,
                        size_t*     _size);
/**
 * @brief Reads an open stream from its current position to its end, into one
 *        fresh allocation.
 *
 * @note A measurable source costs one allocation. One that cannot be measured
 *       -- a pipe, a /proc entry -- is read by growing the buffer, or refused
 *       with ESPIPE when D_CFG_FILE_READ_GROW_UNSIZED is 0.
 *
 * @param[in]  _stream  the stream to read.
 * @param[out] _size    receives the number of payload bytes; may be `NULL`.
 * @return the buffer, NUL-terminated as for d_file_read_all, or `NULL` on
 *         failure.
 * @post The caller owns the buffer and releases it with the configured
 *       deallocator (D_CFG_FILE_FREE).
 */
void*   d_file_read_all_stream(FILE*   _stream,
                               size_t* _size);
/**
 * @brief Reads an entire file into a buffer the caller already owns, without
 *        allocating.
 *
 * @note A file larger than the buffer fails with ERANGE rather than being
 *       truncated. With D_CFG_FILE_READ_NUL_TERMINATE on, the terminator
 *       takes one byte of `_bufsize`.
 *
 * @param[in]  _path     the file to read.
 * @param[out] _buf      receives the contents.
 * @param[in]  _bufsize  size of `_buf`, in bytes.
 * @param[out] _size     receives the number of payload bytes; may be `NULL`.
 * @return 0, or a non-zero errno-style code -- ERANGE when the file does not
 *         fit.
 */
int     d_file_read_all_into(const char* _path,
                             void*       _buf,
                             size_t      _bufsize,
                             size_t*     _size);

// 1.4    Whole-file writes
//------------------------------------------------------------------------------
/**
 * @brief Creates or replaces a file with the contents of a buffer.
 *
 * @note With D_CFG_FILE_WRITE_ATOMIC on, the bytes go to a sibling temporary
 *       that is renamed over the target, so no reader and no crash ever sees
 *       a partial file; off, the target is rewritten in place. With
 *       D_CFG_FILE_WRITE_SYNC on, success means the device has the data, not
 *       just the kernel.
 *
 * @param[in] _path  the file to write.
 * @param[in] _data  the bytes to write; may be `NULL` only when `_size` is 0.
 * @param[in] _size  the number of bytes to write.
 * @return 0, or -1 on failure with errno set.
 */
int     d_file_write_all(const char* _path,
                         const void* _data,
                         size_t      _size);
/**
 * @brief Appends a buffer to a file, creating it if it does not exist.
 *
 * @note On POSIX the move to the end and the write are one operation, so two
 *       processes appending to one file interleave records rather than
 *       overwrite each other. Atomic replacement does not apply.
 *
 * @param[in] _path  the file to append to.
 * @param[in] _data  the bytes to append; may be `NULL` only when `_size` is
 *                   0.
 * @param[in] _size  the number of bytes to append.
 * @return 0, or -1 on failure with errno set.
 */
int     d_file_append_all(const char* _path,
                          const void* _data,
                          size_t      _size);


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_FS_FILE_IO_H
