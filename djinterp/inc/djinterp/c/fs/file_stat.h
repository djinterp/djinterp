/*******************************************************************************
* djinterp [c]                                                       file_stat.h
*
* File metadata -- what the filesystem knows about a path.
*   Every query here is a syscall. The convenience predicates look free and
* are not: d_file_exists + d_file_is_regular + d_dir_exists on one path is
* THREE stat calls, and each can disagree with the next if the file changes
* in between. Call d_file_stat once and read the fields when you have more
* than one question.
*   Every predicate is also a TOCTOU hazard by construction:
* d_file_is_regular(p) followed by d_file_open_stream(p) is two decisions about
* a path that may name two different files. Where it matters, open first and
* ask d_file_stat_fd about the descriptor -- that one cannot be swapped
* underneath you.
*
*
* path:      /inc/djinterp/c/fs/file_stat.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  METADATA
    --------
    1.  Status
    2.  Permissions
    3.  Size
    4.  Predicates
*/

#ifndef DJINTERP_C_FS_FILE_STAT_H
#define DJINTERP_C_FS_FILE_STAT_H 1

// std
#include <stdint.h>  // int64_t, uint32_t
#include <stdio.h>   // FILE
// djinterp
#include "./file_common.h"                    // fs foundation, d_stat_t
#include "../../config/c/fs/cfg_file_stat.h"  // module configuration


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  METADATA
//==============================================================================
// The queries report failure as -1 with errno set. The predicates fold every
// failure into "no".


// 1.1    Status
//------------------------------------------------------------------------------
/**
 * @brief Retrieves the status of a path, following symbolic links.
 *
 * @note With D_CFG_FILE_STAT_FOLLOW_SYMLINKS set to 0 this stops at a link
 *       instead, like d_file_stat_nofollow -- for an archiver or backup tool
 *       that must not traverse.
 *
 * @param[in]  _path  the path to query.
 * @param[out] _buf   receives the status; fully overwritten, with every field
 *                    this platform cannot answer set to 0.
 * @return 0, or -1 on failure with errno set.
 */
int     d_file_stat(const char*      _path,
                    struct d_stat_t* _buf);
/**
 * @brief Retrieves the status of a path without following a symbolic link,
 *        so the result describes the link itself.
 *
 * @warning The Windows CRT cannot describe a link, so there this behaves
 *          exactly like d_file_stat. Use d_file_is_symlink when the
 *          distinction matters.
 *
 * @param[in]  _path  the path to query.
 * @param[out] _buf   receives the status.
 * @return 0, or -1 on failure with errno set.
 */
int     d_file_stat_nofollow(const char*      _path,
                             struct d_stat_t* _buf);
/**
 * @brief Retrieves the status of an open descriptor.
 *
 * @note The only query here with no TOCTOU hazard: a descriptor names one
 *       open file for as long as it is held. When a decision must be about
 *       the file you will actually use, open it first and ask this.
 *
 * @param[in]  _fd   an open descriptor.
 * @param[out] _buf  receives the status.
 * @return 0, or -1 on failure with errno set; ENOSYS on the ISO C backend.
 */
int     d_file_stat_fd(int              _fd,
                       struct d_stat_t* _buf);

// 1.2    Permissions
//------------------------------------------------------------------------------
/**
 * @brief Asks whether the current user may do something to a path.
 *
 * @warning The answer describes permission bits at one instant, not a
 *          promise: the file can change before the open, and a set-uid
 *          program is checked as its real user here but its effective user
 *          by open. Use it for a better error message, never for a security
 *          decision.
 *
 * @param[in] _path  the path to test.
 * @param[in] _mode  F_OK, or R_OK, W_OK and X_OK OR'd together; Windows has no
 *                   execute permission and ignores X_OK.
 * @return 0 when the access is permitted, or -1 otherwise with errno set.
 */
int     d_file_access(const char* _path,
                      int         _mode);
/**
 * @brief Sets the permission bits of a path.
 *
 * @warning On Windows only the write bit exists: the mode becomes the
 *          read-only attribute and every other bit is discarded, without an
 *          error.
 *
 * @param[in] _path  the path to modify.
 * @param[in] _mode  permission bits (S_IRUSR, S_IWUSR, ...).
 * @return 0, or -1 on failure with errno set.
 */
int     d_file_chmod(const char* _path,
                     uint32_t    _mode);

// 1.3    Size
//------------------------------------------------------------------------------
/**
 * @brief Reports the size of a file in bytes, as the filesystem states it.
 *
 * @note A /proc or /sys entry states 0 though reading it yields data; to
 *       learn how many bytes a read will produce, use d_file_read_all.
 *
 * @param[in] _path  the path to measure.
 * @return the size in bytes, or -1 on failure with errno set.
 */
int64_t d_file_size(const char* _path);
/**
 * @brief Reports the size of the file behind a stream, without moving the
 *        stream's position.
 *
 * @param[in] _stream  an open stream.
 * @return the size in bytes, or -1 on failure with errno set; ENOSYS on the
 *         ISO C backend.
 */
int64_t d_file_size_stream(FILE* _stream);

// 1.4    Predicates
//------------------------------------------------------------------------------
// predicates -- one d_file_stat call each, so they follow symbolic links as
// it does, and the answer is already stale when it returns. Each answers
// non-zero for yes and 0 for no, for a NULL path, or for any failure.
int     d_file_exists(const char* _path);
int     d_file_is_regular(const char* _path);
int     d_dir_exists(const char* _path);


D_EXTERN_C_END


#endif  // DJINTERP_C_FS_FILE_STAT_H
