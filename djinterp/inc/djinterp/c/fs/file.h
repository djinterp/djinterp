/*******************************************************************************
* djinterp [c]                                                            file.h
*
* Umbrella for the djinterp fs subframework -- includes every c/fs module.
*   CONVENIENCE, NOT ARCHITECTURE. The whole point of the split is that a
* program which only reads a file compiles and links file_common + file_open +
* file_io, and nothing else: no lock code, no directory walker, no shell.
* Including this header discards that -- you get every declaration and, once
* the linker is done, most of the objects.
*   Use it for a quick program or a test harness. In a library, include the
* two or three modules you actually call; the deps column in the fs module map
* says which.
*   file_link.h and file_pipe.h are always safe to include: each publishes
* nothing on a platform without the capability. Guard USES with
* D_FILE_LINK_IS_AVAILABLE and D_FILE_PIPE_IS_AVAILABLE.
*
*
* path:      /inc/djinterp/c/fs/file.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/

#ifndef DJINTERP_C_FS_FILE_H
#define DJINTERP_C_FS_FILE_H 1

// djinterp
#include "./file_common.h"  // foundation: types, constants, notifications
#include "./file_open.h"    // streams: open, reopen, fdopen, close
#include "./file_desc.h"    // descriptors: open, dup, dup2, fileno, close
#include "./file_io.h"      // transfer: read, write, pread, pwrite, *_all
#include "./file_seek.h"    // position: seek, tell, rewind, truncate
#include "./file_sync.h"    // durability: fsync, fflush
#include "./file_lock.h"    // locking: advisory flock / fcntl locks
#include "./file_stat.h"    // metadata: stat, access, chmod, size, predicates
#include "./file_path.h"    // paths: lexical only, no system calls
#include "./file_dir.h"     // directories: mkdir, walk, getcwd, realpath
#include "./file_ops.h"     // operations: remove, rename, copy
#include "./file_temp.h"    // temporaries: tmpfile, mkstemp, temp directory
#include "./file_space.h"   // capacity: total, free, available
#include "./file_link.h"    // links: symlink; publishes nothing without them
#include "./file_pipe.h"    // pipes: popen; publishes nothing without them


#endif  // DJINTERP_C_FS_FILE_H
