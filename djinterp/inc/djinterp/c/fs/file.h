/******************************************************************************
* djinterp [c]                                                          file.h
*
* Umbrella for the djinterp fs subframework -- includes every c/fs module.
*
*   CONVENIENCE, NOT ARCHITECTURE. The whole point of the split is that a
* program which only reads a file compiles and links file_common + file_open +
* file_io, and nothing else: no lock code, no directory walker, no shell.
* Including this header discards that -- you get every declaration and, once
* the linker is done, most of the objects.
*   Use it for a quick program or a test harness. In a library, include the
* two or three modules you actually call; the deps column in the fs module map
* says which.
*
* path:      /inc/djinterp/c/fs/file.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.15
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    FOUNDATION      file_common   types, constants, notifications
II.   STREAMS         file_open     fopen / fclose / freopen / fdopen
III.  DESCRIPTORS     file_desc     open / close / dup / dup2 / fileno
IV.   TRANSFER        file_io       read / write / pread / pwrite / *_all
V.    POSITION        file_seek     fseeko / ftello / ftruncate
VI.   DURABILITY      file_sync     fsync / fflush
VII.  LOCKING         file_lock     flock                       (advisory)
VIII. METADATA        file_stat     stat / access / chmod / size / predicates
IX.   PATHS           file_path     lexical only; zero syscalls
X.    DIRECTORIES     file_dir      mkdir / opendir / getcwd / realpath
XI.   OPERATIONS      file_ops      remove / rename / copy
XII.  TEMPORARIES     file_temp     tmpfile / mkstemp / tempdir
XIII. CAPACITY        file_space    capacity / free / available
XIV.  LINKS           file_link     symlink            (gated; may be absent)
XV.   PIPES           file_pipe     popen              (gated; may be absent)
*/

#ifndef DJINTERP_FILE_
#define DJINTERP_FILE_ 1

// djinterp
// I.    Foundation
#include "./file_common.h"
// II.   Streams
#include "./file_open.h"
// III.  Descriptors
#include "./file_desc.h"
// IV.   Transfer
#include "./file_io.h"
// V.    Position
#include "./file_seek.h"
// VI.   Durability
#include "./file_sync.h"
// VII.  Locking
#include "./file_lock.h"
// VIII. Metadata
#include "./file_stat.h"
// IX.   Paths
#include "./file_path.h"
// X.    Directories
#include "./file_dir.h"
// XI.   Operations
#include "./file_ops.h"
// XII.  Temporaries
#include "./file_temp.h"
// XIII. Capacity
#include "./file_space.h"
// XIV.  Links
//   The header is always safe to include; it publishes nothing when the
// platform has no symbolic links. Guard USES with D_FILE_LINK_IS_AVAILABLE.
#include "./file_link.h"
// XV.   Pipes
//   As above; guard uses with D_FILE_PIPE_IS_AVAILABLE.
#include "./file_pipe.h"

#endif  // DJINTERP_FILE_
