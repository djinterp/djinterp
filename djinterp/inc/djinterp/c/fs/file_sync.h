/******************************************************************************
* djinterp [core]                                                  file_sync.h
*
* Forcing data to durable storage.
*   Three calls, three DIFFERENT promises, and confusing them is how programs
* lose data they were told was written:
*     d_fflush        stdio's buffer -> the kernel.  Survives a process crash.
*                     Does NOT survive a power cut.
*     d_fsync         the kernel -> the device.      Survives a power cut.
*     d_fsync_stream  both, in that order.           The one you usually want.
*   fflush is not a weaker fsync; it is a different layer. A program that
* flushes and believes it has persisted is one power cut from finding out.
*
* path:      \inc\djinterp\c\fs\file_sync.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.15
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    KERNEL -> DEVICE
      ----------------
      1.  d_fsync           (by descriptor)
      2.  d_fsync_stream    (by stream; flushes stdio first)

II.   STDIO -> KERNEL
      ---------------
      1.  d_fflush
*/

#ifndef DJINTERP_FILE_SYNC_
#define DJINTERP_FILE_SYNC_ 1

#include "./file_common.h"
#include "../../config/c/fs/cfg_file_sync.h"


D_EXTERN_C_BEGIN


// I.    Kernel -> device
int d_fsync(int _fd);
int d_fsync_stream(FILE* _stream);

// II.   Stdio -> kernel
int d_fflush(FILE* _stream);


D_EXTERN_C_END


#endif  // DJINTERP_FILE_SYNC_
