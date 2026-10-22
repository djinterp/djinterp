/*******************************************************************************
* djinterp [c]                                                    t_file_gates.c
*
*   Not a program: a probe ci/check_filesys_platforms.sh preprocesses, once
* per target, to read what c/filesys's configuration resolves to there. After
* preprocessing, each D_FILE_GATE line holds a gate's name, quoted so no
* header's macro can rewrite it, and its value as the preprocessor leaves it,
* for the script to evaluate:
*     backend    D_INTERNAL_FILE_BACKEND: 1 ISO C, 2 POSIX, 3 native
*     posix      D_CFG_FILE_HAS_POSIX
*     win32      D_CFG_FILE_HAS_WIN32
*     realpath   D_INTERNAL_FILE_HAS_REALPATH
*     symlinks   D_INTERNAL_FILE_HAS_SYMLINKS
*     pipes      D_INTERNAL_FILE_HAS_PIPES
*     pread      D_INTERNAL_FILE_HAS_PREAD
*     tmpfile    D_CFG_FILE_HAS_TMPFILE
*     clock      D_CFG_FILE_HAS_CLOCK
*     flock      D_CFG_FILE_HAS_FLOCK
*     lock       D_INTERNAL_FILE_LOCK_BACKEND: 1 flock, 2 fcntl, 3 Win32
*     nsec       D_CFG_FILE_HAS_STAT_NSEC: which spelling of the nanoseconds
*     birth      D_INTERNAL_FILE_STAT_BIRTHTIME
*     dtype      D_CFG_FILE_HAS_DIRENT_TYPE
*     syncfull   D_INTERNAL_FILE_SYNC_FULL
*     space      D_FILE_SPACE_IS_AVAILABLE
*     width      D_FILE_WIDTH
*     wide       D_INTERNAL_FILE_WIDE: paths read as UTF-8, handed to Windows
*                as UTF-16
*   By hand, from the repo root, for the target's compiler:
*     cc -E -Iinc tests/djinterp/c/filesys/t_file_gates.c | grep D_FILE_GATE
*
*
* path:      /tests/djinterp/c/filesys/t_file_gates.c
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.19
*                                                            revised: 2026.10.19
*******************************************************************************/
// djinterp
#include "../../../../inc/djinterp/c/filesys/file_common.h"
#include "../../../../inc/djinterp/config/c/filesys/cfg_file_dir.h"
#include "../../../../inc/djinterp/config/c/filesys/cfg_file_lock.h"
#include "../../../../inc/djinterp/config/c/filesys/cfg_file_space.h"
#include "../../../../inc/djinterp/config/c/filesys/cfg_file_stat.h"
#include "../../../../inc/djinterp/config/c/filesys/cfg_file_sync.h"


D_FILE_GATE "backend"   D_INTERNAL_FILE_BACKEND
D_FILE_GATE "posix"     D_CFG_FILE_HAS_POSIX
D_FILE_GATE "win32"     D_CFG_FILE_HAS_WIN32
D_FILE_GATE "realpath"  D_INTERNAL_FILE_HAS_REALPATH
D_FILE_GATE "symlinks"  D_INTERNAL_FILE_HAS_SYMLINKS
D_FILE_GATE "pipes"     D_INTERNAL_FILE_HAS_PIPES
D_FILE_GATE "pread"     D_INTERNAL_FILE_HAS_PREAD
D_FILE_GATE "tmpfile"   D_CFG_FILE_HAS_TMPFILE
D_FILE_GATE "clock"     D_CFG_FILE_HAS_CLOCK
D_FILE_GATE "flock"     D_CFG_FILE_HAS_FLOCK
D_FILE_GATE "lock"      D_INTERNAL_FILE_LOCK_BACKEND
D_FILE_GATE "nsec"      D_CFG_FILE_HAS_STAT_NSEC
D_FILE_GATE "birth"     D_INTERNAL_FILE_STAT_BIRTHTIME
D_FILE_GATE "dtype"     D_CFG_FILE_HAS_DIRENT_TYPE
D_FILE_GATE "syncfull"  D_INTERNAL_FILE_SYNC_FULL
D_FILE_GATE "space"     D_FILE_SPACE_IS_AVAILABLE
D_FILE_GATE "width"     D_FILE_WIDTH
D_FILE_GATE "wide"      D_INTERNAL_FILE_WIDE
