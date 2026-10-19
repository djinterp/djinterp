/*******************************************************************************
* djinterp [c]                                                           dfile.h
*
* Umbrella for the filesys subframework -- includes every c/filesys module.
*   CONVENIENCE, NOT ARCHITECTURE. The whole point of the split is that a
* program which only reads a file compiles and links file_common + file_open +
* file_io, and nothing else: no lock code, no directory walker, no shell.
* Including this header discards that -- you get every declaration and, once
* the linker is done, most of the objects.
*   Use it for a quick program or a test harness. In a library, include the
* two or three modules you actually call; the deps column in the filesys
* module map says which.
*   file_link.h and file_pipe.h are always safe to include: each publishes
* nothing on a platform without the capability. Guard USES with
* D_FILE_LINK_IS_AVAILABLE and D_FILE_PIPE_IS_AVAILABLE. What holds an
* offset, a size, a time, an inode number or a capacity comes in a 32-bit and
* a 64-bit twin, the unsuffixed name following D_FILE_WIDTH; the 64-bit twins
* are absent below the 64-bit floor, so guard a USE of one with
* D_FILE_64_IS_AVAILABLE (file_common.h has the rules).
*   The c/filesys sources build with -D_XOPEN_SOURCE=700 where the C library
* hides POSIX in a strict ISO mode, and with -D_FILE_OFFSET_BITS=64 (plus
* -D_TIME_BITS=64 from glibc 2.34) where off_t and time_t are otherwise
* 32-bit -- a 32-bit glibc host -- which they assert (the C guide's rule:
* feature-test macros come from the build). On Linux, file_ops.c and
* file_stat.c build with -D_GNU_SOURCE as well, as their banners ask: it
* brings copy_file_range and statx, and without it each compiles only its
* portable path.
*   THE NAME. dfile.h was the monolithic header this subframework was split
* from, and then a compatibility shim over this umbrella. The shim is gone,
* and the legacy names it carried with it: D_FILE_HAS_SYMLINKS is
* D_FILE_LINK_IS_AVAILABLE, D_FILE_HAS_PIPES is D_FILE_PIPE_IS_AVAILABLE, and
* D_FILE_PLATFORM_WINDOWS / _POSIX give way to the capability being asked
* about (D_FILE_PATH_UNDERSTANDS_WINDOWS, D_FILE_BACKEND_IS_NATIVE).
*
*
* path:      /inc/djinterp/c/filesys/dfile.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.13
*******************************************************************************/

#ifndef DJINTERP_C_FILESYS_DFILE_H
#define DJINTERP_C_FILESYS_DFILE_H 1

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


#endif  // DJINTERP_C_FILESYS_DFILE_H
