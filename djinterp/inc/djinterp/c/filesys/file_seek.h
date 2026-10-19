/*******************************************************************************
* djinterp [c]                                                       file_seek.h
*
* Positioning and truncation, at either width.
*   Exists as its own module because "where am I in this file" and "how big is
* this file" are one concern: both are the file's extent, and both are where a
* 32-bit off_t silently corrupts data on a large file.
*   d_file_tell_stream64 and d_file_seek_stream64 are 64-bit on every target,
* including a 32-bit Windows build where ftell() would wrap at 2 GiB and
* report a plausible wrong answer. Their 32-bit twins move the same 64-bit
* position, so a 32-bit build can still seek past 2 GiB; only
* d_file_tell_stream32 can fail there, with EOVERFLOW, when the position it
* must report does not fit. The unsuffixed names follow D_FILE_WIDTH, and
* file_common.h has the rules every twin keeps.
*
*
* path:      /inc/djinterp/c/filesys/file_seek.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.13
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  POSITION
    --------
    1.  Positioning
    2.  Truncation
2.  DEFAULT WIDTH
    -------------
    1.  Unsuffixed names
*/

#ifndef DJINTERP_C_FILESYS_FILE_SEEK_H
#define DJINTERP_C_FILESYS_FILE_SEEK_H 1

// std
#include <stdio.h>  // FILE, SEEK_SET, SEEK_CUR, SEEK_END
// djinterp
#include "./file_common.h"                         // foundation, d_off32_t,
                                                   // d_off64_t
#include "../../config/c/filesys/cfg_file_seek.h"  // module configuration


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  POSITION
//==============================================================================


// 1.1    Positioning
//------------------------------------------------------------------------------
#if D_FILE_64_IS_AVAILABLE
/**
 * @brief Sets a stream's position, with a 64-bit offset on every target.
 *
 * @param[in] _stream  an open stream.
 * @param[in] _offset  the offset, interpreted per `_whence`.
 * @param[in] _whence  SEEK_SET, SEEK_CUR or SEEK_END.
 * @return 0, or -1 on failure with errno set; EOVERFLOW where the build has
 *         no 64-bit seek and `_offset` does not fit a long.
 */
int       d_file_seek_stream64(FILE*     _stream,
                               d_off64_t _offset,
                               int       _whence);
/**
 * @brief Reports a stream's position, 64-bit on every target.
 *
 * @param[in] _stream  an open stream.
 * @return the current offset, or -1 on failure with errno set.
 */
d_off64_t d_file_tell_stream64(FILE* _stream);
#endif  // D_FILE_64_IS_AVAILABLE
/**
 * @brief Sets a stream's position with a 32-bit offset. The position itself
 *        is not limited: SEEK_CUR and SEEK_END may land past 2 GiB.
 *
 * @param[in] _stream  an open stream.
 * @param[in] _offset  the offset, interpreted per `_whence`.
 * @param[in] _whence  SEEK_SET, SEEK_CUR or SEEK_END.
 * @return 0, or -1 on failure with errno set.
 */
int       d_file_seek_stream32(FILE*     _stream,
                               d_off32_t _offset,
                               int       _whence);
/**
 * @brief Reports a stream's position as a 32-bit offset.
 *
 * @param[in] _stream  an open stream.
 * @return the current offset, or -1 on failure with errno set; EOVERFLOW
 *         when the position is past 2 GiB - 1.
 */
d_off32_t d_file_tell_stream32(FILE* _stream);
/**
 * @brief Returns a stream to its start and clears its error flag -- unlike
 *        ISO C's rewind, reporting whether it could.
 *
 * @param[in] _stream  an open stream.
 * @return 0, or -1 on failure with errno set; a stream that refuses is
 *         usually a pipe.
 */
int       d_file_rewind_stream(FILE* _stream);

// 1.2    Truncation
//------------------------------------------------------------------------------
#if D_FILE_64_IS_AVAILABLE
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
int       d_file_truncate_fd64(int       _fd,
                               d_off64_t _length);
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
int       d_file_truncate_stream64(FILE*     _stream,
                                   d_off64_t _length);
#endif  // D_FILE_64_IS_AVAILABLE
/**
 * @brief d_file_truncate_fd64 with a 32-bit length.
 *
 * @param[in] _fd      a descriptor open for writing.
 * @param[in] _length  the new length in bytes; not negative.
 * @return 0, or -1 on failure with errno set; ENOSYS on the ISO C backend.
 */
int       d_file_truncate_fd32(int       _fd,
                               d_off32_t _length);
/**
 * @brief d_file_truncate_stream64 with a 32-bit length.
 *
 * @param[in] _stream  a stream open for writing.
 * @param[in] _length  the new length in bytes; not negative.
 * @return 0, or -1 on failure with errno set; ENOSYS on the ISO C backend.
 */
int       d_file_truncate_stream32(FILE*     _stream,
                                   d_off32_t _length);


//==============================================================================
// 2.  DEFAULT WIDTH
//==============================================================================


// 2.1    Unsuffixed names
//------------------------------------------------------------------------------
// d_file_seek_stream
//   alias: the call's twin at D_FILE_WIDTH, so it takes a d_off_t
// (file_common.h, 3.2); d_file_tell_stream, d_file_truncate_fd and
// d_file_truncate_stream after it are the same alias for their calls.
#if (D_FILE_WIDTH == 64)
    #define d_file_seek_stream     d_file_seek_stream64
    #define d_file_tell_stream     d_file_tell_stream64
    #define d_file_truncate_fd     d_file_truncate_fd64
    #define d_file_truncate_stream d_file_truncate_stream64
#else
    #define d_file_seek_stream     d_file_seek_stream32
    #define d_file_tell_stream     d_file_tell_stream32
    #define d_file_truncate_fd     d_file_truncate_fd32
    #define d_file_truncate_stream d_file_truncate_stream32
#endif  // D_FILE_WIDTH == 64


D_EXTERN_C_END


#endif  // DJINTERP_C_FILESYS_FILE_SEEK_H
