/*******************************************************************************
* djinterp [c]                                                    ostream_file.h
*
*   A stream over a C library FILE*: one the caller opened and lends, or one
* opened here and owned. The FILE* is a transport: the bytes go through stdio,
* so the stream interleaves correctly with anything else writing the same
* FILE* once it is flushed.
*
*   NO OS HERE. Opening, writing and closing go through c/filesys
* (d_file_open_stream and d_file_close_stream; d_file_open, d_file_write_fd
* and d_file_close_fd), which made every platform decision once; this unit
* adds only the stream. A file opened here is opened in binary mode, so no C
* library rewrites a byte of it. A borrowed FILE*'s mode is the caller's, and
* its query calls it binary-safe only where no C library translates newlines.
*
*   APPENDING KEEPS RECORDS WHOLE. A file opened here to append is not a
* FILE*: it is a descriptor opened O_APPEND, written once per flush, so a
* record no larger than the stream's buffer, written and then flushed,
* reaches the file whole however many processes append to it.  stdio's own
* buffer would split a record larger than it into several writes.
*
*   GIVE IT A BUFFER. stdio already buffers, but every fwrite still takes the
* FILE's lock and copies; with a buffer (d_ostream_set_buffer) the stream
* calls fwrite once per buffer, and a write at least as large as the buffer
* reaches fwrite uncopied.
*
*
* path:      /inc/djinterp/c/util/ostream/ostream_file.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.11.17
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  FILE DESTINATIONS
    -----------------
    1.  A FILE* the caller opened
    2.  A file opened here
*/

#ifndef DJINTERP_C_UTIL_OSTREAM_OSTREAM_FILE_H
#define DJINTERP_C_UTIL_OSTREAM_OSTREAM_FILE_H 1

// std
#include <stdio.h>  // FILE
// djinterp
#include "../../djinterp.h"  // framework root
#include "./ostream.h"       // struct dj_ostream
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // int32_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  FILE DESTINATIONS
//==============================================================================


// 1.1    A FILE* the caller opened
//------------------------------------------------------------------------------
/**
 * @brief Makes a stream over a FILE* the caller opened.
 *
 * @param[out] _os     the stream.
 * @param[in]  _file   the file.
 * @param[in]  _flags  D_OSTREAM_OWNS_CONTEXT to have d_ostream_close close
 *                     the file, and the other D_OSTREAM_* flags as for any
 *                     stream.
 * @return as d_ostream_init(); 0 as well for a null _file.
 */
int32_t d_ostream_init_file(struct dj_ostream* _os,
                            FILE*              _file,
                            int32_t            _flags);

// 1.2    A file opened here
//------------------------------------------------------------------------------
/**
 * @brief Opens a file for writing, in binary mode, as a stream that owns it.
 *
 * @note To append, the file is opened on a descriptor, O_APPEND, and each
 *       flush is one write: a record no larger than the stream's buffer,
 *       written and then flushed, reaches a local file whole, whatever other
 *       processes append to it.  Give the stream a buffer
 *       (d_ostream_set_buffer) as large as the largest record.  On Windows
 *       this holds once c/filesys opens O_APPEND with FILE_APPEND_DATA alone;
 *       where c/filesys has no descriptors, appending goes through stdio and
 *       promises nothing.
 * @note When the open fails the stream is left D_OSTREAM_FAILED with the
 *       open's errno, readable through d_ostream_last_errno(); a null _path is
 *       D_OSTREAM_INVALID.
 *
 * @param[out] _os      the stream.
 * @param[in]  _path    the file's path.
 * @param[in]  _append  non-zero to append; zero to truncate.
 * @return 1, or 0 when the file could not be opened.
 */
int32_t d_ostream_open_file(struct dj_ostream* _os,
                            const char*        _path,
                            int32_t            _append);


D_EXTERN_C_END


#endif  // DJINTERP_C_UTIL_OSTREAM_OSTREAM_FILE_H
