/*******************************************************************************
* djinterp [c]                                                       file_open.h
*
* Opening, reopening and closing FILE* streams.
*   Every path this subframework opens goes through here, so the decisions
* that have to be made uniformly -- how a mode string is interpreted, whether
* a stream is inherited across exec, what other processes may do to the file
* meanwhile -- are made once and inherited by file_read, file_write and
* anything else that needs a stream.
*   This module owns streams only. Raw descriptors are file_desc.h.
*
*
* path:      /inc/djinterp/c/fs/file_open.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  STREAMS
    -------
    1.  Stream opening
    2.  Stream closing
*/

#ifndef DJINTERP_C_FS_FILE_OPEN_H
#define DJINTERP_C_FS_FILE_OPEN_H 1

// std
#include <stdio.h>  // FILE
// djinterp
#include "./file_common.h"                    // fs foundation, D_EXTERN_C_*
#include "../../config/c/fs/cfg_file_open.h"  // module configuration
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  STREAMS
//==============================================================================
// Each open applies the build's policy to the mode: a 'b' when
// D_CFG_FILE_OPEN_BINARY_DEFAULT is set and the caller named neither 'b' nor
// 't', and the platform's close-on-exec flag when D_CFG_FILE_OPEN_CLOEXEC is
// set. An explicit choice in the caller's mode is never overruled.


// 1.1    Stream opening
//------------------------------------------------------------------------------
/**
 * @brief Opens a file as a stream (portable fopen).
 *
 * @param[in] _filename  the path to open.
 * @param[in] _mode      an fopen mode ("r", "w", "a", ...), optionally with
 *                       'b' and '+'.
 * @return the stream, or `NULL` on failure with errno set.
 * @post The caller owns the stream and closes it with d_file_close_stream.
 */
FILE* d_file_open_stream(const char* _filename,
                         const char* _mode);
/**
 * @brief Opens a file as a stream, reporting through the return value (C11
 *        Annex K fopen_s equivalent).
 *
 * @param[out] _stream    receives the stream; cleared to `NULL` before
 *                        anything else can fail, so a caller that ignores the
 *                        result never holds an indeterminate pointer.
 * @param[in]  _filename  the path to open.
 * @param[in]  _mode      an fopen mode.
 * @return 0, or a non-zero error code: EINVAL for a bad argument, otherwise
 *         the platform's errno.
 */
int   d_file_open_stream_s(FILE**      _stream,
                           const char* _filename,
                           const char* _mode);
/**
 * @brief Reopens an existing stream on a new file or in a new mode (portable
 *        freopen).
 *
 * @warning Per the C contract, a failed reopen closes `_stream` regardless;
 *          the caller must not use it again.
 *
 * @param[in]     _filename  the path to open, or `NULL` to change the mode of
 *                           `_stream` in place.
 * @param[in]     _mode      the new mode.
 * @param[in,out] _stream    the stream to reopen.
 * @return the reopened stream, or `NULL` on failure.
 */
FILE* d_file_reopen_stream(const char* _filename,
                           const char* _mode,
                           FILE*       _stream);
/**
 * @brief Reopens an existing stream, reporting through the return value (C11
 *        Annex K freopen_s equivalent).
 *
 * @warning As with d_file_reopen_stream, a failed reopen closes `_stream`.
 *
 * @param[out]    _newstream  receives the reopened stream, or `NULL`.
 * @param[in]     _filename   the path to open, or `NULL` to change the mode
 *                            of `_stream` in place.
 * @param[in]     _mode       the new mode.
 * @param[in,out] _stream     the stream to reopen.
 * @return 0, or a non-zero error code: EINVAL for a bad argument, otherwise
 *         the platform's errno.
 */
int   d_file_reopen_stream_s(FILE**      _newstream,
                             const char* _filename,
                             const char* _mode,
                             FILE*       _stream);
/**
 * @brief Associates a stream with an already open descriptor (POSIX fdopen
 *        equivalent).
 *
 * @warning The descriptor is adopted, not duplicated: closing the stream
 *          closes `_fd`, and closing `_fd` out from under the stream is
 *          undefined.
 *
 * @param[in] _fd    an open descriptor.
 * @param[in] _mode  a mode compatible with how `_fd` was opened.
 * @return the stream, or `NULL` on failure with errno set; ENOSYS on the ISO
 *         C backend, which has no descriptors.
 * @post The stream owns `_fd`.
 */
FILE* d_file_open_stream_fd(int         _fd,
                            const char* _mode);

// 1.2    Stream closing
//------------------------------------------------------------------------------
/**
 * @brief Closes a stream opened by this module.
 *
 * @warning A buffered write that could not be flushed is reported here and
 *          nowhere else; a caller that ignores the result can lose data it
 *          believes it wrote.
 *
 * @param[in] _stream  the stream to close.
 * @return 0, or EOF on failure with errno set.
 * @post `_stream` is closed whatever the result, and must not be used again.
 */
int   d_file_close_stream(FILE* _stream);


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_FS_FILE_OPEN_H
