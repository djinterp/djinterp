/*******************************************************************************
* djinterp [c]                                                       file_seek.h
*
* 64-bit positioning and truncation.
*   Exists as its own module because "where am I in this file" and "how big is
* this file" are one concern: both are the file's extent, and both are where a
* 32-bit off_t silently corrupts data on a large file.
*   d_file_tell_stream and d_file_seek_stream are 64-bit on every target,
* including a 32-bit Windows build where ftell() would wrap at 2 GiB and
* report a plausible wrong answer.
*
*
* path:      /inc/djinterp/c/fs/file_seek.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  POSITION
    --------
    1.  Positioning
    2.  Truncation
*/

#ifndef DJINTERP_C_FS_FILE_SEEK_H
#define DJINTERP_C_FS_FILE_SEEK_H 1

// std
#include <stdio.h>  // FILE, SEEK_SET, SEEK_CUR, SEEK_END
// djinterp
#include "./file_common.h"                    // fs foundation, d_off_t
#include "../../config/c/fs/cfg_file_seek.h"  // module configuration


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  POSITION
//==============================================================================


// 1.1    Positioning
//------------------------------------------------------------------------------
/**
 * @brief Sets a stream's position, with a 64-bit offset on every target.
 *
 * @param[in] _stream  an open stream.
 * @param[in] _offset  the offset, interpreted per `_whence`.
 * @param[in] _whence  SEEK_SET, SEEK_CUR or SEEK_END.
 * @return 0, or -1 on failure with errno set; EOVERFLOW where the build has
 *         no 64-bit seek and `_offset` does not fit a long.
 */
int     d_file_seek_stream(FILE*   _stream,
                           d_off_t _offset,
                           int     _whence);
/**
 * @brief Reports a stream's position, 64-bit on every target.
 *
 * @param[in] _stream  an open stream.
 * @return the current offset, or -1 on failure with errno set.
 */
d_off_t d_file_tell_stream(FILE* _stream);
/**
 * @brief Returns a stream to its start and clears its error flag -- unlike
 *        ISO C's rewind, reporting whether it could.
 *
 * @param[in] _stream  an open stream.
 * @return 0, or -1 on failure with errno set; a stream that refuses is
 *         usually a pipe.
 */
int     d_file_rewind_stream(FILE* _stream);

// 1.2    Truncation
//------------------------------------------------------------------------------
/**
 * @brief Sets a file's length through a descriptor.
 *
 * @note Shrinking discards the tail; extending zero-fills, sparsely where the
 *       filesystem allows. The file position is unchanged and may end up past
 *       the new end.
 *
 * @param[in] _fd      a descriptor open for writing.
 * @param[in] _length  the new length in bytes; not negative.
 * @return 0, or -1 on failure with errno set; ENOSYS on the ISO C backend.
 */
int     d_file_truncate_fd(int     _fd,
                           d_off_t _length);
/**
 * @brief Sets a file's length through the stream that owns it, flushing
 *        stdio's buffer first.
 *
 * @note Without the flush, bytes still buffered would be written after the
 *       truncation and undo it. D_CFG_FILE_SEEK_FLUSH_BEFORE_TRUNCATE turns
 *       it off for callers who flush themselves.
 *
 * @param[in] _stream  a stream open for writing.
 * @param[in] _length  the new length in bytes; not negative.
 * @return 0, or -1 on failure with errno set; ENOSYS on the ISO C backend.
 */
int     d_file_truncate_stream(FILE*   _stream,
                               d_off_t _length);


D_EXTERN_C_END


#endif  // DJINTERP_C_FS_FILE_SEEK_H
