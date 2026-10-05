/*******************************************************************************
* djinterp [c]                                                       file_desc.h
*
* Descriptor lifecycle -- getting one, copying one, giving one back.
*   This module owns descriptors, not the bytes that move through them; those
* are file_io. So a program handed a descriptor by its parent, which only ever
* reads it, links file_io and never this.
*   Every descriptor here is close-on-exec by default (D_CFG_FILE_DESC_CLOEXEC)
* -- including the one d_file_dup_fd returns, which POSIX would otherwise hand
* back with the flag silently cleared.
*
*
* path:      /inc/djinterp/c/fs/file_desc.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DESCRIPTORS
    -----------
    1.  Acquisition
    2.  Duplication
    3.  Release
*/

#ifndef DJINTERP_C_FS_FILE_DESC_H
#define DJINTERP_C_FS_FILE_DESC_H 1

// std
#include <stdio.h>  // FILE
// djinterp
#include "./file_common.h"                    // fs foundation, D_EXTERN_C_*
#include "../../config/c/fs/cfg_file_desc.h"  // module configuration
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  DESCRIPTORS
//==============================================================================
// Every function here reports failure as -1 with errno set. The ISO C backend
// has no descriptors, so on it every one of them fails with ENOSYS.


// 1.1    Acquisition
//------------------------------------------------------------------------------
/**
 * @brief Opens a file and returns a descriptor (POSIX open equivalent).
 *
 * @note    Unless the build turns it off, the descriptor is close-on-exec,
 *          and on Windows binary unless `_flags` asks for O_TEXT.
 * @warning Pass the creation mode whenever `_flags` contains O_CREAT. A mode
 *          of 0 is taken for a forgotten argument and replaced with
 *          D_CFG_FILE_DESC_CREATE_MODE.
 *
 * @param[in] _path   the path to open.
 * @param[in] _flags  O_RDONLY, O_WRONLY or O_RDWR, optionally OR'd with
 *                    O_CREAT, O_TRUNC, O_APPEND, O_EXCL and the like.
 * @param[in] ...     the creation mode, as an int; read only when `_flags`
 *                    contains O_CREAT.
 * @return a descriptor, or -1 on failure with errno set.
 * @post The caller owns the descriptor and releases it with d_file_close_fd.
 */
int d_file_open(const char* _path,
                int         _flags,
                ...);
/**
 * @brief Returns the descriptor a stream is built on.
 *
 * @warning The descriptor is borrowed: the stream still owns it, closing it
 *          out from under the stream is undefined, and it dies with the
 *          stream. Duplicate it with d_file_dup_fd to keep one.
 *
 * @param[in] _stream  an open stream.
 * @return the descriptor, or -1 on failure with errno set.
 */
int d_file_descriptor_stream(FILE* _stream);

// 1.2    Duplication
//------------------------------------------------------------------------------
/**
 * @brief Duplicates a descriptor onto the lowest free number.
 *
 * @note The copy shares the original's file offset and status flags. It is
 *       close-on-exec when D_CFG_FILE_DESC_DUP_CLOEXEC is set, where plain
 *       dup would clear the flag.
 *
 * @param[in] _fd  an open descriptor.
 * @return the new descriptor, or -1 on failure with errno set.
 * @post The caller owns the new descriptor and releases it with
 *       d_file_close_fd.
 */
int d_file_dup_fd(int _fd);
/**
 * @brief Duplicates a descriptor onto a chosen number, closing whatever was
 *        open there.
 *
 * @note Deliberately not close-on-exec: installing a descriptor on 0, 1 or 2
 *       for a child to inherit is the usual reason to call this. Duplicating
 *       a valid descriptor onto itself is a no-op, not an error.
 *
 * @param[in] _fd   an open descriptor.
 * @param[in] _fd2  the descriptor number to install it onto.
 * @return `_fd2`, or -1 on failure with errno set.
 */
int d_file_dup2_fd(int _fd,
                   int _fd2);

// 1.3    Release
//------------------------------------------------------------------------------
/**
 * @brief Closes a descriptor.
 *
 * @note An interrupted close is not retried: on Linux the descriptor is
 *       already gone, and a retry could close one another thread just opened
 *       onto the same number.
 *
 * @param[in] _fd  the descriptor to close.
 * @return 0, or -1 on failure with errno set.
 * @post `_fd` is released whatever the result, and must not be used again.
 */
int d_file_close_fd(int _fd);


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_FS_FILE_DESC_H
