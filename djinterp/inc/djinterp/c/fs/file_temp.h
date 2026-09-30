/*******************************************************************************
* djinterp [c]                                                       file_temp.h
*
* Temporary files.
*   Two shapes, and only one of them is safe:
*     d_file_temp_stream, d_file_temp_create
*         create the file and open it in one step, so there is no window. Use
*         these.
*     d_file_temp_name
*         hands you a name to open later. Between the two, anyone with write
*         access to that directory can create it first -- classically as a
*         symlink to something you have permission to destroy.
*   d_file_temp_name exists for compatibility and is gated behind
* D_CFG_FILE_TEMP_ALLOW_TMPNAM so a codebase can prove it has no uses left.
*
*
* path:      /inc/djinterp/c/fs/file_temp.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TEMPORARY FILES
    ---------------
    1.  Atomic creation
    2.  Name generation
    3.  Location
*/

#ifndef DJINTERP_C_FS_FILE_TEMP_H
#define DJINTERP_C_FS_FILE_TEMP_H 1

// std
#include <stddef.h>  // size_t
#include <stdio.h>   // FILE
// djinterp
#include "./file_common.h"                    // fs foundation, D_EXTERN_C_*
#include "../../config/c/fs/cfg_file_temp.h"  // D_INTERNAL_FILE_TEMP_TMPNAM


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TEMPORARY FILES
//==============================================================================


// 1.1    Atomic creation
//------------------------------------------------------------------------------
/**
 * @brief Creates an anonymous temporary file, removed when it is closed or
 *        the process exits.
 *
 * @return an open read/write stream, or `NULL` on failure with errno set.
 * @post The caller owns the stream; closing it removes the file.
 */
FILE* d_file_temp_stream(void);
/**
 * @brief Creates an anonymous temporary file, reporting through the return
 *        value (C11 Annex K shape).
 *
 * @param[out] _stream  receives the stream, or `NULL` on failure; cleared
 *                      before anything else happens.
 * @return 0, or a non-zero error code -- EINVAL for a `NULL` out-parameter.
 */
int   d_file_temp_stream_s(FILE** _stream);
/**
 * @brief Creates and opens a uniquely named temporary file from a template,
 *        in one step, so no other process can claim the name first.
 *
 * @note The file is created with D_CFG_FILE_TEMP_MODE (0600 by default) and
 *       is not removed for you.
 *
 * @param[in,out] _template  a writable path ending in exactly six 'X'
 *                           characters, which are replaced in place with the
 *                           chosen suffix; a string literal will crash.
 * @return an open descriptor, or -1 on failure with errno set; ENOSYS on the
 *         ISO C backend.
 * @post The caller owns the descriptor and the file.
 */
int   d_file_temp_create(char* _template);

// 1.2    Name generation
//------------------------------------------------------------------------------
#if (D_INTERNAL_FILE_TEMP_TMPNAM == 1)
/**
 * @brief Generates a filename that does not currently exist.
 *
 * @warning Racy by construction: between this call and the open, anyone who
 *          can write to the directory can create the name, classically as a
 *          symlink to a file you may destroy. Use d_file_temp_create. This is
 *          compiled only when D_CFG_FILE_TEMP_ALLOW_TMPNAM is 1.
 *
 * @param[out] _s        receives the name.
 * @param[in]  _maxsize  size of `_s`, in bytes.
 * @return 0, or a non-zero error code: EINVAL for a bad argument, ERANGE when
 *         the name does not fit, EEXIST when no free name was found, and EIO
 *         when the temporary directory is unavailable.
 */
int   d_file_temp_name(char*  _s,
                       size_t _maxsize);
#endif

// 1.3    Location
//------------------------------------------------------------------------------
/**
 * @brief Reports the directory temporary files should go in.
 *
 * @note It consults TMPDIR, TMP and TEMP, then the platform's default --
 *       unless D_CFG_FILE_TEMP_HONOUR_ENV is 0, which is what a set-uid
 *       program wants.
 *
 * @param[out] _buf      receives the path, without a trailing separator.
 * @param[in]  _bufsize  size of `_buf`, in bytes.
 * @return `_buf`, or `NULL` on failure with errno set.
 */
char* d_dir_temp(char*  _buf,
                 size_t _bufsize);


D_EXTERN_C_END


#endif  // DJINTERP_C_FS_FILE_TEMP_H
