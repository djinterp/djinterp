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
*   The positional calls come in both widths, d_file_pread_fd32 beside
* d_file_pread_fd64; the unsuffixed names follow D_FILE_WIDTH (file_common.h
* has the rules). The offset is the only thing that differs, and a 32-bit one
* is widened, never refused.
*
*
* path:      /inc/djinterp/c/filesys/file_io.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.18
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
2.  DEFAULT WIDTH
    -------------
    1.  Unsuffixed names
*/

#ifndef DJINTERP_C_FILESYS_FILE_IO_H
#define DJINTERP_C_FILESYS_FILE_IO_H 1

// std
#include <stddef.h>  // size_t
#include <stdio.h>   // FILE
// djinterp
#include "./file_common.h"                       // foundation, ssize_t,
                                                 // d_off32_t, d_off64_t
#include "./file_open.h"                         // streams: the whole-file
                                                 // helpers are built on them
#include "../../config/c/filesys/cfg_file_io.h"  // module configuration


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TRANSFER
//==============================================================================
// The descriptor functions fail with ENOSYS on the ISO C backend, which has
// no descriptors; the whole-file functions work on every backend. A
// descriptor call moves at most one chunk (D_CFG_FILE_READ_CHUNK_SIZE,
// D_CFG_FILE_WRITE_CHUNK_SIZE), and never more than one platform call can
// carry -- INT_MAX bytes on Windows; a zero-length call needs no buffer.


// 1.1    Descriptor reads
//------------------------------------------------------------------------------
/**
 * @brief Reads from a descriptor (POSIX read equivalent).
 *
 * @note Like read(2) it may return fewer bytes than requested without that
 *       being an error; d_file_read_full_fd writes the loop for you.
 *
 * @param[in]  _fd     an open descriptor.
 * @param[out] _buf    receives the bytes; holds at least `_count`, and may be
 *                     `NULL` when `_count` is 0.
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
 * @warning On failure some bytes may already have been read into `_buf`, and
 *          from a pipe or a socket they are gone from the source.
 *
 * @param[in]  _fd     an open descriptor.
 * @param[out] _buf    receives the bytes; holds at least `_count`, and may be
 *                     `NULL` when `_count` is 0.
 * @param[in]  _count  the number of bytes wanted.
 * @return `_count`, fewer if end of file came first, or -1 on failure with
 *         errno set.
 */
ssize_t d_file_read_full_fd(int    _fd,
                            void*  _buf,
                            size_t _count);
#if D_FILE_64_IS_AVAILABLE
/**
 * @brief Reads from a fixed offset without moving the descriptor's position.
 *
 * @warning Atomic only where the platform has pread. Elsewhere -- Windows
 *          among them -- it is emulated by seek, read and seek back, and two
 *          threads sharing the descriptor can interleave;
 *          D_FILE_READ_PREAD_IS_ATOMIC says which.
 *
 * @param[in]  _fd      an open, seekable descriptor.
 * @param[out] _buf     receives the bytes; holds at least `_count`, and may be
 *                      `NULL` when `_count` is 0.
 * @param[in]  _count   the most bytes to read.
 * @param[in]  _offset  the absolute offset to read from; not negative.
 * @return the number of bytes read, 0 at end of file, or -1 on failure with
 *         errno set.
 */
ssize_t d_file_pread_fd64(int       _fd,
                          void*     _buf,
                          size_t    _count,
                          d_off64_t _offset);
#endif  // D_FILE_64_IS_AVAILABLE
/**
 * @brief d_file_pread_fd64 with a 32-bit offset.
 *
 * @param[in]  _fd      an open, seekable descriptor.
 * @param[out] _buf     receives the bytes; holds at least `_count`.
 * @param[in]  _count   the most bytes to read.
 * @param[in]  _offset  the absolute offset to read from; not negative.
 * @return the number of bytes read, 0 at end of file, or -1 on failure with
 *         errno set.
 */
ssize_t d_file_pread_fd32(int       _fd,
                          void*     _buf,
                          size_t    _count,
                          d_off32_t _offset);

// 1.2    Descriptor writes
//------------------------------------------------------------------------------
/**
 * @brief Writes to a descriptor (POSIX write equivalent).
 *
 * @note Like write(2) it may accept fewer bytes than offered without that
 *       being an error; d_file_write_full_fd writes the loop for you.
 *
 * @param[in] _fd     an open descriptor.
 * @param[in] _buf    the bytes to write; holds at least `_count`, and may be
 *                    `NULL` when `_count` is 0.
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
 * @param[in] _buf    the bytes to write; holds at least `_count`, and may be
 *                    `NULL` when `_count` is 0.
 * @param[in] _count  the number of bytes to write.
 * @return `_count`, or -1 on failure with errno set.
 */
ssize_t d_file_write_full_fd(int         _fd,
                             const void* _buf,
                             size_t      _count);
#if D_FILE_64_IS_AVAILABLE
/**
 * @brief Writes at a fixed offset without moving the descriptor's position.
 *
 * @warning Atomic only where the platform has pwrite; elsewhere -- Windows
 *          among them -- it is emulated by seek, write and seek back, and is
 *          not. On Linux a descriptor opened with O_APPEND appends whatever
 *          the offset, as the kernel documents.
 *
 * @param[in] _fd      an open, seekable descriptor.
 * @param[in] _buf     the bytes to write; holds at least `_count`, and may be
 *                     `NULL` when `_count` is 0.
 * @param[in] _count   the number of bytes to offer.
 * @param[in] _offset  the absolute offset to write at; not negative.
 * @return the number of bytes written, or -1 on failure with errno set.
 */
ssize_t d_file_pwrite_fd64(int         _fd,
                           const void* _buf,
                           size_t      _count,
                           d_off64_t   _offset);
#endif  // D_FILE_64_IS_AVAILABLE
/**
 * @brief d_file_pwrite_fd64 with a 32-bit offset.
 *
 * @param[in] _fd      an open, seekable descriptor.
 * @param[in] _buf     the bytes to write; holds at least `_count`.
 * @param[in] _count   the number of bytes to offer.
 * @param[in] _offset  the absolute offset to write at; not negative.
 * @return the number of bytes written, or -1 on failure with errno set.
 */
ssize_t d_file_pwrite_fd32(int         _fd,
                           const void* _buf,
                           size_t      _count,
                           d_off32_t   _offset);

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
 *         errno set: EFBIG past the read ceiling (D_CFG_FILE_READ_MAX_SIZE,
 *         or the allocation ceiling less the terminator's byte).
 * @post The caller owns the buffer and releases it with the configured
 *       deallocator (D_CFG_FILE_FREE).
 */
void*   d_file_read_all(const char* _path,
                        size_t*     _size);
/**
 * @brief Reads an open stream from its current position to its end, into one
 *        fresh allocation.
 *
 * @note The stream is read from where it is, in its own mode: a text-mode
 *       stream reads translated, so the size can be smaller than the
 *       file's.
 * @note A measurable source costs one allocation, and is read to the length
 *       it had when it was measured. One that cannot be measured -- a pipe,
 *       a /proc entry -- is read by growing the buffer, or, when
 *       D_CFG_FILE_READ_GROW_UNSIZED is 0, refused with ESPIPE unless it is
 *       empty.
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
 *       truncated -- before a byte is read, when it can be measured, and
 *       otherwise once it fills the buffer and has more. With
 *       D_CFG_FILE_READ_NUL_TERMINATE on, the terminator takes one byte of
 *       `_bufsize`.
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
 *       D_CFG_FILE_WRITE_SYNC on, success means the device has the data and,
 *       on POSIX, the directory entry that names it, not just the kernel; a
 *       directory that cannot be synced fails the call, although the file is
 *       written.
 * @note A file this creates gets D_CFG_FILE_WRITE_CREATE_MODE, under the
 *       umask; one that exists keeps its permission bits, rewritten or
 *       replaced. A replacement is a new file all the same: a symbolic link
 *       at the path is replaced rather than followed, and hard links,
 *       ownership and ACLs do not carry over (cfg_file_io.h has the list).
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
 *       overwrite each other. Atomic replacement does not apply. A file this
 *       creates gets D_CFG_FILE_WRITE_CREATE_MODE, under the umask, and
 *       D_CFG_FILE_WRITE_SYNC syncs as for d_file_write_all.
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


//==============================================================================
// 2.  DEFAULT WIDTH
//==============================================================================


// 2.1    Unsuffixed names
//------------------------------------------------------------------------------
// d_file_pread_fd
//   alias: the call's twin at D_FILE_WIDTH, so it takes a d_off_t
// (file_common.h, 3.2); d_file_pwrite_fd after it is the same alias for its
// call.
#if (D_FILE_WIDTH == 64)
    #define d_file_pread_fd  d_file_pread_fd64
    #define d_file_pwrite_fd d_file_pwrite_fd64
#else
    #define d_file_pread_fd  d_file_pread_fd32
    #define d_file_pwrite_fd d_file_pwrite_fd32
#endif  // D_FILE_WIDTH == 64


D_EXTERN_C_END


#endif  // DJINTERP_C_FILESYS_FILE_IO_H
