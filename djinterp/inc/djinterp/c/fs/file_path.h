/******************************************************************************
* djinterp [core]                                                  file_path.h
*
* Lexical path manipulation -- what a path SAYS, never what is on disk.
*   Nothing in this module issues a system call. d_dirname("/nowhere/x") is
* "/nowhere" whether or not that directory exists, and d_path_normalize does
* not resolve symlinks because it cannot see them. That is a feature: it makes
* the whole module testable on any machine with no filesystem, no permissions
* and no temp directory, and it makes it the natural bottom for a C++ path
* type whose lexical operations are specified the same way.
*   The counterpart -- the operations that must ask the filesystem -- are
* d_getcwd / d_chdir / d_realpath in file_dir.h.
*
* path:      \inc\djinterp\c\fs\file_path.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.15
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    DECOMPOSITION
      -------------
      1.  d_dirname           (directory component)
      2.  d_basename          (final component)
      3.  d_get_extension     (extension, pointing into the input)
      4.  d_path_stem         (final component without its extension)

II.   COMPOSITION
      -----------
      1.  d_path_join         (join two components)

III.  INSPECTION
      ----------
      1.  d_path_is_absolute  (does the path name a root)
      2.  d_path_root_length  (length of the root prefix, 0 if relative)

IV.   CANONICALIZATION
      -----------------
      1.  d_path_normalize    (lexical: collapse . .. and separators)
*/

#ifndef DJINTERP_FILE_PATH_
#define DJINTERP_FILE_PATH_ 1

#include "./file_common.h"
#include "../../config/c/fs/cfg_file_path.h"


D_EXTERN_C_BEGIN


// I.    Decomposition
char*       d_dirname(const char* _path,
                      char*       _buf,
                      size_t      _bufsize);
char*       d_basename(const char* _path,
                       char*       _buf,
                       size_t      _bufsize);
const char* d_get_extension(const char* _path);
char*       d_path_stem(const char* _path,
                        char*       _buf,
                        size_t      _bufsize);

// II.   Composition
char*       d_path_join(char*       _buf,
                        size_t      _bufsize,
                        const char* _path1,
                        const char* _path2);

// III.  Inspection
int         d_path_is_absolute(const char* _path);
size_t      d_path_root_length(const char* _path);

// IV.   Canonicalization
char*       d_path_normalize(const char* _path,
                             char*       _buf,
                             size_t      _bufsize);


D_EXTERN_C_END


#endif  // DJINTERP_FILE_PATH_
