/*******************************************************************************
* djinterp [c]                                                       file_lock.h
*
* Advisory file locking.
*   ADVISORY, and the word is load-bearing: a lock here is a convention among
* programs that agree to ask. A process that never calls d_file_lock_fd writes
* the file regardless, and nothing reports it. There is no portable mandatory
* locking; if that is what you need, this module cannot supply it.
*   The semantics depend on the backend, and they are not interchangeable --
* see cfg_file_lock.h. In short: flock's lock lives on the open file
* description, fcntl's lives on the process and is dropped when ANY descriptor
* to the file closes. Query with D_FILE_LOCK_IS_PER_DESCRIPTION.
*
*
* path:      /inc/djinterp/c/fs/file_lock.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  LOCKING
    -------
    1.  Advisory locks
*/

#ifndef DJINTERP_C_FS_FILE_LOCK_H
#define DJINTERP_C_FS_FILE_LOCK_H 1

// std
#include <stdio.h>  // FILE
// djinterp
#include "./file_common.h"                    // fs foundation, D_LOCK_*
#include "../../config/c/fs/cfg_file_lock.h"  // module configuration
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  LOCKING
//==============================================================================
// Operations come from file_common.h: D_LOCK_SH, D_LOCK_EX or D_LOCK_UN,
// optionally OR'd with D_LOCK_NB to fail rather than block. On the ISO C
// backend, which has no descriptors, both functions fail with ENOSYS.


// 1.1    Advisory locks
//------------------------------------------------------------------------------
/**
 * @brief Takes or releases an advisory lock on a descriptor.
 *
 * @note Only processes that ask are coordinated. The lock's lifetime follows
 *       the backend: with flock it belongs to the open file description,
 *       with fcntl to the process, and closing ANY descriptor to the file
 *       drops it. D_FILE_LOCK_IS_PER_DESCRIPTION says which.
 *
 * @param[in] _fd         an open descriptor.
 * @param[in] _operation  exactly one of D_LOCK_SH, D_LOCK_EX or D_LOCK_UN,
 *                        optionally OR'd with D_LOCK_NB.
 * @return 0, or -1 on failure with errno set. Under D_LOCK_NB a conflict
 *         returns -1 with errno EWOULDBLOCK, EAGAIN or EACCES -- an answer,
 *         not a fault.
 */
int d_file_lock_fd(int _fd,
                   int _operation);
/**
 * @brief Takes or releases an advisory lock through the stream that owns the
 *        file.
 *
 * @note It does not flush. Flush before releasing a lock that guards data you
 *       have written.
 *
 * @param[in] _stream     an open stream.
 * @param[in] _operation  as for d_file_lock_fd.
 * @return 0, or -1 on failure with errno set.
 */
int d_file_lock_stream(FILE* _stream,
                       int   _operation);


D_EXTERN_C_END


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_FS_FILE_LOCK_H
