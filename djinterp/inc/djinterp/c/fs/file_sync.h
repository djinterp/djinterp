/*******************************************************************************
* djinterp [c]                                                       file_sync.h
*
* Forcing data to durable storage.
*   Three calls, three DIFFERENT promises, and confusing them is how programs
* lose data they were told was written:
*     d_file_flush_stream  stdio's buffer -> the kernel. Survives a process
*                          crash; does NOT survive a power cut.
*     d_file_sync_fd       the kernel -> the device. Survives a power cut.
*     d_file_sync_stream   both, in that order. The one you usually want.
*   fflush is not a weaker fsync; it is a different layer. A program that
* flushes and believes it has persisted is one power cut from finding out.
*
*
* path:      /inc/djinterp/c/fs/file_sync.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  DURABILITY
    ----------
    1.  Kernel -> device
    2.  Stdio -> kernel
*/

#ifndef DJINTERP_C_FS_FILE_SYNC_H
#define DJINTERP_C_FS_FILE_SYNC_H 1

// std
#include <stdio.h>  // FILE
// djinterp
#include "./file_common.h"                    // fs foundation, D_EXTERN_C_*
#include "../../config/c/fs/cfg_file_sync.h"  // module configuration


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  DURABILITY
//==============================================================================


// 1.1    Kernel -> device
//------------------------------------------------------------------------------
/**
 * @brief Forces a descriptor's data to durable storage.
 *
 * @note On macOS plain fsync does not flush the drive's own cache, so
 *       D_CFG_FILE_SYNC_FULL (on by default) uses F_FULLFSYNC there;
 *       D_FILE_SYNC_REACHES_PLATTER says which promise this build makes.
 * @note This syncs the file, not its directory entry: to persist a new
 *       file's name, sync the containing directory too.
 *
 * @param[in] _fd  an open descriptor.
 * @return 0, or -1 on failure with errno set, in which case the data is not
 *         durable; ENOSYS on the ISO C backend.
 */
int d_file_sync_fd(int _fd);
/**
 * @brief Forces a stream's data to durable storage: stdio's buffer into the
 *        kernel, then the kernel's copy onto the device.
 *
 * @param[in] _stream  an open stream.
 * @return 0, or -1 on failure with errno set; ENOSYS on the ISO C backend.
 */
int d_file_sync_stream(FILE* _stream);

// 1.2    Stdio -> kernel
//------------------------------------------------------------------------------
/**
 * @brief Hands a stream's buffered bytes to the kernel.
 *
 * @warning Not a weaker fsync: the data survives the process dying but not
 *          the machine losing power. Use d_file_sync_stream for durability.
 *
 * @param[in] _stream  an open output stream, or `NULL` to flush every output
 *                     stream, per ISO C.
 * @return 0, or EOF on failure with errno set.
 */
int d_file_flush_stream(FILE* _stream);


D_EXTERN_C_END


#endif  // DJINTERP_C_FS_FILE_SYNC_H
