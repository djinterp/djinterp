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
*   The status and size queries come in both widths, d_file_stat32 beside
* d_file_stat64, and the unsuffixed names follow D_FILE_WIDTH (file_common.h
* has the rules). A 32-bit query fails with EOVERFLOW rather than report a
* size, a time or an inode number its type cannot hold. The predicates ask
* the 64-bit queries, so a file too large for d_stat32_t still exists, and
* d_file_mode, which has no width, answers what a path IS at either one.
*   A build with neither POSIX nor Windows has nothing to ask -- ISO C cannot
* describe a file -- so there every query fails with ENOSYS and every
* predicate answers no.
*
*
* path:      /inc/djinterp/c/filesys/file_stat.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.18
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  METADATA
    --------
    1.  Status
    2.  Mode
    3.  Permissions
    4.  Size
    5.  Predicates
2.  DEFAULT WIDTH
    -------------
    1.  Unsuffixed names
*/

#ifndef DJINTERP_C_FILESYS_FILE_STAT_H
#define DJINTERP_C_FILESYS_FILE_STAT_H 1

// std
#include <stdio.h>   // FILE
// djinterp
#include "./file_common.h"                         // foundation, d_stat32_t,
                                                   // d_stat64_t
#include "../../config/c/filesys/cfg_file_stat.h"  // module configuration
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // int32_t, int64_t, uint32_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  METADATA
//==============================================================================
// The queries report failure as -1 with errno set. The predicates fold every
// failure into "no". A path that is not there -- ENOENT, or ENOTDIR for a
// component on the way that is not a directory -- is an answer to the
// predicates and to d_file_mode, so they raise no notice for it; every other
// failure, and any failure of the status queries, is reported. On Windows a
// trailing separator means what it means on POSIX: "dir\\" names a directory
// and "file.txt\\" fails with ENOTDIR.


// 1.1    Status
//------------------------------------------------------------------------------
#if D_FILE_64_IS_AVAILABLE
/**
 * @brief Retrieves the status of a path, following symbolic links.
 *
 * @note With D_CFG_FILE_STAT_FOLLOW_SYMLINKS set to 0 this stops at a link
 *       instead, like d_file_stat_nofollow64 -- for an archiver or backup
 *       tool that must not traverse.
 *
 * @param[in]  _path  the path to query.
 * @param[out] _buf   receives the status; fully overwritten, with every field
 *                    this platform cannot answer set to 0.
 * @return 0, or -1 on failure with errno set.
 */
int     d_file_stat64(const char*        _path,
                      struct d_stat64_t* _buf);
/**
 * @brief Retrieves the status of a path without following a symbolic link,
 *        so the result describes the link itself.
 *
 * @note On Windows a symbolic link or a junction is described as POSIX
 *       describes a link: S_IFLNK, with the link's own times and link count,
 *       and a size of 0. Any other reparse point is the file or directory it
 *       appears to be.
 *
 * @param[in]  _path  the path to query.
 * @param[out] _buf   receives the status.
 * @return 0, or -1 on failure with errno set.
 */
int     d_file_stat_nofollow64(const char*        _path,
                               struct d_stat64_t* _buf);
/**
 * @brief Retrieves the status of an open descriptor.
 *
 * @note The only query here with no TOCTOU hazard: a descriptor names one
 *       open file for as long as it is held. When a decision must be about
 *       the file you will actually use, open it first and ask this.
 * @note It answers field for field as d_file_stat64 answers for the same
 *       file, creation time included.
 *
 * @param[in]  _fd   an open descriptor.
 * @param[out] _buf  receives the status.
 * @return 0, or -1 on failure with errno set; ENOSYS on the ISO C backend.
 */
int     d_file_stat_fd64(int                _fd,
                         struct d_stat64_t* _buf);
#endif  // D_FILE_64_IS_AVAILABLE
/**
 * @brief d_file_stat64, into the 32-bit layout.
 *
 * @param[in]  _path  the path to query.
 * @param[out] _buf   receives the status; zeroed on failure.
 * @return 0, or -1 on failure with errno set; EOVERFLOW when a size, time,
 *         device or inode number does not fit d_stat32_t.
 */
int     d_file_stat32(const char*        _path,
                      struct d_stat32_t* _buf);
/**
 * @brief d_file_stat_nofollow64, into the 32-bit layout.
 *
 * @param[in]  _path  the path to query.
 * @param[out] _buf   receives the status; zeroed on failure.
 * @return 0, or -1 on failure with errno set; EOVERFLOW when a value does
 *         not fit d_stat32_t.
 */
int     d_file_stat_nofollow32(const char*        _path,
                               struct d_stat32_t* _buf);
/**
 * @brief d_file_stat_fd64, into the 32-bit layout.
 *
 * @param[in]  _fd   an open descriptor.
 * @param[out] _buf  receives the status; zeroed on failure.
 * @return 0, or -1 on failure with errno set; EOVERFLOW when a value does
 *         not fit d_stat32_t, ENOSYS on the ISO C backend.
 */
int     d_file_stat_fd32(int                _fd,
                         struct d_stat32_t* _buf);

// 1.2    Mode
//------------------------------------------------------------------------------
/**
 * @brief Retrieves only the mode of a path -- its type bits and permission
 *        bits -- following symbolic links as d_file_stat does.
 *
 * @note It asks what a path is, and "nothing" is one of the answers: a path
 *       that is not there fails with ENOENT or ENOTDIR but raises no notice.
 *
 * @note The mode is 32-bit at either width, so this has no twin and never
 *       fails for a value d_stat32_t cannot hold: it is how "what is this
 *       path" stays answerable for a file too large, or too far from 1970,
 *       for d_stat32_t. (Where the C library's own time_t is 32-bit, a time
 *       past 2038 still fails the underlying stat with EOVERFLOW.)
 *
 * @param[in]  _path  the path to query.
 * @param[out] _mode  receives the mode, for S_ISREG and its kin.
 * @return 0, or -1 on failure with errno set.
 */
int     d_file_mode(const char* _path,
                    uint32_t*   _mode);
/**
 * @brief d_file_mode without following a final symbolic link, so that a link
 *        reports as a link -- on Windows a junction too, as
 *        d_file_stat_nofollow64 describes them.
 *
 * @param[in]  _path  the path to query.
 * @param[out] _mode  receives the mode.
 * @return 0, or -1 on failure with errno set.
 */
int     d_file_mode_nofollow(const char* _path,
                             uint32_t*   _mode);

// 1.3    Permissions
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
 * @return 0 when the access is permitted, or -1 otherwise with errno set:
 *         EINVAL for a mode with any other bit.
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

// 1.4    Size
//------------------------------------------------------------------------------
#if D_FILE_64_IS_AVAILABLE
/**
 * @brief Reports the size of a file in bytes, as the filesystem states it.
 *
 * @note A /proc or /sys entry states 0 though reading it yields data; to
 *       learn how many bytes a read will produce, use d_file_read_all.
 *
 * @param[in] _path  the path to measure.
 * @return the size in bytes, or -1 on failure with errno set.
 */
int64_t d_file_size64(const char* _path);
/**
 * @brief Reports the size of the file behind a stream, without moving the
 *        stream's position.
 *
 * @note What the stream still buffers is not in the file yet and is not
 *       counted; fflush a stream you have written to first.
 *
 * @param[in] _stream  an open stream.
 * @return the size in bytes, or -1 on failure with errno set; ENOSYS on the
 *         ISO C backend.
 */
int64_t d_file_size_stream64(FILE* _stream);
#endif  // D_FILE_64_IS_AVAILABLE
/**
 * @brief d_file_size64, as a 32-bit count.
 *
 * @param[in] _path  the path to measure.
 * @return the size in bytes, or -1 on failure with errno set; EOVERFLOW past
 *         2 GiB - 1.
 */
int32_t d_file_size32(const char* _path);
/**
 * @brief d_file_size_stream64, as a 32-bit count.
 *
 * @param[in] _stream  an open stream.
 * @return the size in bytes, or -1 on failure with errno set; EOVERFLOW past
 *         2 GiB - 1, ENOSYS on the ISO C backend.
 */
int32_t d_file_size_stream32(FILE* _stream);

// 1.5    Predicates
//------------------------------------------------------------------------------
// predicates -- one stat each, asked as d_file_stat64 asks at every width, so
// they follow symbolic links as it does and no file is too large to exist;
// the answer is already stale when it returns. Each answers non-zero for yes
// and 0 for no, for a NULL path, or for any failure.
int     d_file_exists(const char* _path);
int     d_file_is_regular(const char* _path);
int     d_dir_exists(const char* _path);


//==============================================================================
// 2.  DEFAULT WIDTH
//==============================================================================


// 2.1    Unsuffixed names
//------------------------------------------------------------------------------
// d_file_stat
//   alias: the call's twin at D_FILE_WIDTH, so it fills a d_stat_t
// (file_common.h, 3.2); d_file_stat_nofollow, d_file_stat_fd, d_file_size and
// d_file_size_stream after it are the same alias for their calls, the sizes
// returning the matching type.
#if (D_FILE_WIDTH == 64)
    #define d_file_stat          d_file_stat64
    #define d_file_stat_nofollow d_file_stat_nofollow64
    #define d_file_stat_fd       d_file_stat_fd64
    #define d_file_size          d_file_size64
    #define d_file_size_stream   d_file_size_stream64
#else
    #define d_file_stat          d_file_stat32
    #define d_file_stat_nofollow d_file_stat_nofollow32
    #define d_file_stat_fd       d_file_stat_fd32
    #define d_file_size          d_file_size32
    #define d_file_size_stream   d_file_size_stream32
#endif  // D_FILE_WIDTH == 64


D_EXTERN_C_END


#endif  // DJINTERP_C_FILESYS_FILE_STAT_H
