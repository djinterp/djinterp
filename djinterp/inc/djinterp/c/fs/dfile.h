/******************************************************************************
* djinterp [c]                                                         dfile.h
*
* COMPATIBILITY SHIM. Do not include this in new code.
*
*   dfile has been split into the c/fs subframework: 15 modules, each with its
* own config, each independently linkable. This header exists so that code
* written against the old one keeps compiling while it migrates. It declares
* nothing of its own -- it includes c/fs/file.h, which is the umbrella over
* all of them.
*
*   WHAT YOU GIVE UP BY INCLUDING THIS. The umbrella pulls in every module, so
* a program that only reads a file still links the locking code, the directory
* walker and the shell pipe. Measured on the reference build: 22,408 bytes for
* file_common + file_open + file_io, against 39,024 for everything. Include the
* two or three modules you call instead; the fs module map lists the
* dependencies.
*
*   THE ONE SOURCE BREAK: d_stat_t's timestamp fields were renamed.
*     st_mtime  ->  st_modified
*     st_atime  ->  st_accessed
*     st_ctime  ->  st_changed    (+ st_created, which ctime never was)
*   This is not tidying. On glibc and macOS those three are MACROS, not member
* names -- `#define st_mtime st_mtim.tv_sec` -- so `uint64_t st_mtime;`
* expands to `uint64_t st_mtim.tv_sec;` and does not compile, and any code
* reading the field hits the same expansion. The macros only appear once
* _POSIX_C_SOURCE >= 200809L or _GNU_SOURCE is set, which is why the old
* header got away with it: the moment the fs subframework asked for a Linux
* fast path (copy_file_range, statx, posix_fadvise) it had to set _GNU_SOURCE,
* and the collision went live.
*   No shim can paper over it. An alias would have to be a macro, and a macro
* is exactly what breaks. Rename the field at your call sites; there is no
* other move. st_size, st_mode, st_nlink, st_uid, st_gid, st_dev and st_ino
* are unchanged.
*
* path:      /inc/djinterp/c/fs/dfile.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.15
******************************************************************************/

#ifndef DJINTERP_DFILE_
#define DJINTERP_DFILE_ 1

// djinterp
#include "./file.h"


// --- Legacy feature macros ---
//   The old dfile.h resolved these itself, in the header, which is what the
// config subframework exists to stop. They now come from cfg_file_common.h
// and are re-exported here under their old names.

// D_FILE_HAS_SYMLINKS
//   macro (legacy): 1 when the symbolic-link API exists.
//   Superseded by D_FILE_LINK_IS_AVAILABLE, which says the same thing and
// says which module it belongs to.
#ifndef D_FILE_HAS_SYMLINKS
#   define D_FILE_HAS_SYMLINKS D_INTERNAL_FILE_HAS_SYMLINKS
#endif

// D_FILE_HAS_PIPES
//   macro (legacy): 1 when the pipe API exists.
//   Superseded by D_FILE_PIPE_IS_AVAILABLE.
#ifndef D_FILE_HAS_PIPES
#   define D_FILE_HAS_PIPES D_INTERNAL_FILE_HAS_PIPES
#endif

// D_FILE_PLATFORM_WINDOWS
//   macro (legacy): 1 on a Windows target.
//   DO NOT USE IN NEW CODE, and it is worth being explicit about why rather
// than just deprecating it. Every place this macro appeared was a place
// asking the wrong question: "am I on Windows" instead of "does this build
// have the capability I need". Those came apart the moment the fs
// subframework let a Linux build parse Windows paths -- one knob answers
// "which path grammar", and the platform answers nothing. Ask the capability:
//     D_FILE_LINK_IS_AVAILABLE      not  "does Windows have symlinks"
//     D_FILE_PATH_UNDERSTANDS_WINDOWS    not  "am I on Windows"
//     D_FILE_BACKEND_IS_NATIVE      not  "is this Win32"
#ifndef D_FILE_PLATFORM_WINDOWS
#   define D_FILE_PLATFORM_WINDOWS D_CFG_FILE_HAS_WIN32
#endif

// D_FILE_PLATFORM_POSIX
//   macro (legacy): 1 on a POSIX target. See the note above.
#ifndef D_FILE_PLATFORM_POSIX
#   define D_FILE_PLATFORM_POSIX D_CFG_FILE_HAS_POSIX
#endif

//   D_FILE_PATH_MAX, D_LOCK_SH / _EX / _NB / _UN, d_off_t, d_stat_t,
// d_dirent_t and d_dir_t all keep their names and meanings -- they come
// through file_common.h unchanged, so nothing needs re-exporting here.


#endif  // DJINTERP_DFILE_
