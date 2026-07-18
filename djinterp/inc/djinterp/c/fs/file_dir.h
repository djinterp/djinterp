/******************************************************************************
* djinterp [core]                                                   file_dir.h
*
* Directories -- creating, removing, walking -- and the path operations that
* must ask the filesystem.
*   The split from file_path is by cost, not by name: d_dirname is lexical and
* free, d_realpath is a syscall that resolves symlinks and can fail. Both are
* "path" work; only one of them needs a disk.
*   struct d_dir_t is opaque and owns the entry d_readdir returns, so that
* pointer stays valid until the next d_readdir on the same handle -- and dies
* with d_closedir.
*
* path:      \inc\djinterp\c\fs\file_dir.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.15
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CREATION
      --------
      1.  d_mkdir       (one directory)
      2.  d_mkdir_p     (a directory and its missing parents)

II.   REMOVAL
      -------
      1.  d_rmdir       (an empty directory)

III.  TRAVERSAL
      ---------
      1.  d_opendir
      2.  d_readdir
      3.  d_rewinddir
      4.  d_closedir

IV.   WORKING DIRECTORY & RESOLUTION
      ------------------------------
      1.  d_getcwd
      2.  d_chdir
      3.  d_realpath    (resolves symlinks; unlike d_path_normalize)
*/

#ifndef DJINTERP_FILE_DIR_
#define DJINTERP_FILE_DIR_ 1

#include "./file_common.h"
#include "./file_path.h"
#include "../../config/c/fs/cfg_file_dir.h"


D_EXTERN_C_BEGIN


// I.    Creation
int                d_mkdir(const char* _path,
                           uint32_t    _mode);
int                d_mkdir_p(const char* _path,
                             uint32_t    _mode);

// II.   Removal
int                d_rmdir(const char* _path);

// III.  Traversal
struct d_dir_t*    d_opendir(const char* _path);
struct d_dirent_t* d_readdir(struct d_dir_t* _dir);
int                d_rewinddir(struct d_dir_t* _dir);
int                d_closedir(struct d_dir_t* _dir);

// IV.   Working directory & resolution
char*              d_getcwd(char*  _buf,
                            size_t _size);
int                d_chdir(const char* _path);
char*              d_realpath(const char* _path,
                              char*       _resolved);


D_EXTERN_C_END


#endif  // DJINTERP_FILE_DIR_
