/******************************************************************************
* djinterp [c]                                                     file_stat.h
*
* File metadata -- what the filesystem knows about a path.
*   Every query here is a syscall. The convenience predicates look free and
* are not: d_file_exists + d_is_file + d_is_dir on one path is THREE stat
* calls, and each can disagree with the next if the file changes in between.
* Call d_stat once and read the fields when you have more than one question.
*
*   Every predicate is also a TOCTOU hazard by construction. d_is_file(p)
* followed by d_fopen(p) is two decisions about a path that may name two
* different files. Where it matters, open first and ask d_fstat about the
* descriptor -- that one cannot be swapped underneath you.
*
* path:      /inc/djinterp/c/fs/file_stat.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.15
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    STATUS
      ------
      1.  d_stat              (by path; follows symlinks)
      2.  d_lstat             (by path; never follows)
      3.  d_fstat             (by descriptor; TOCTOU-free)

II.   PERMISSIONS
      -----------
      1.  d_access            (can I do X to this path)
      2.  d_chmod             (set permission bits)

III.  SIZE
      ----
      1.  d_file_size         (by path)
      2.  d_file_size_stream  (by stream)

IV.   PREDICATES
      ----------
      1.  d_file_exists
      2.  d_is_file
      3.  d_is_dir
*/

#ifndef DJINTERP_FILE_STAT_
#define DJINTERP_FILE_STAT_ 1

// djinterp
#include "./file_common.h"
#include "../../config/c/fs/cfg_file_stat.h"


D_EXTERN_C_BEGIN


// I.    Status
int     d_stat(const char*      _path,
               struct d_stat_t* _buf);
int     d_lstat(const char*      _path,
                struct d_stat_t* _buf);
int     d_fstat(int              _fd,
                struct d_stat_t* _buf);

// II.   Permissions
int     d_access(const char* _path,
                 int         _mode);
int     d_chmod(const char* _path,
                uint32_t    _mode);

// III.  Size
int64_t d_file_size(const char* _path);
int64_t d_file_size_stream(FILE* _stream);

// IV.   Predicates
int     d_file_exists(const char* _path);
int     d_is_file(const char* _path);
int     d_is_dir(const char* _path);


D_EXTERN_C_END


#endif  // DJINTERP_FILE_STAT_
