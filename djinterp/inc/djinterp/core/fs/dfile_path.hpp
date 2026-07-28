/******************************************************************************
* djinterp [core]                                               dfile_path.hpp
*
* Path and working-directory utilities for the djinterp C++ toolkit.
*   This is the C++ counterpart to dfile_path.h. It exposes the path-string
* and working-directory operations as free functions in the djinterp::file::
* path sub-namespace, so calls read as file::path::join, file::path::dirname,
* and so on. Every function forwards inline to the extern "C" API; the
* filesystem-touching calls (getcwd, chdir, realpath) sit here alongside the
* pure string operations because they are path-centric.
*
* 
* path:      /inc/djinterp/cpp/io/file/dfile_path.hpp
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.12.21
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    WORKING DIRECTORY
      -----------------
      1.  getcwd / chdir / realpath

II.   PATH DECOMPOSITION
      ------------------
      1.  dirname / basename / extension

III.  PATH COMPOSITION AND TESTS
      --------------------------
      1.  join / normalize / is_absolute
*/

#ifndef DJINTERP_FILE_PATH_
#define DJINTERP_FILE_PATH_ 1

#include "../../../c/io/file/dfile_path.h"
#include "./dfile_common.hpp"


NS_DJINTERP
D_NAMESPACE(file)
D_NAMESPACE(path)

// I. working directory

// getcwd
//   write the current working directory into _buf (see d_getcwd).
D_INLINE
char*
getcwd(char*  _buf,
       size_t _size)
{
    return d_getcwd(_buf, _size);
}

// chdir
//   change the current working directory to _path (see d_chdir).
D_INLINE
int
chdir(const char* _path)
{
    return d_chdir(_path);
}

// realpath
//   resolve _path to a canonical absolute path in _resolved (see d_realpath).
D_INLINE
char*
realpath(const char* _path,
         char*       _resolved)
{
    return d_realpath(_path, _resolved);
}


// II. path decomposition

// dirname
//   write the directory portion of _path into _buf (see d_dirname).
D_INLINE
char*
dirname(const char* _path,
        char*       _buf,
        size_t      _bufsize)
{
    return d_dirname(_path, _buf, _bufsize);
}

// basename
//   write the final component of _path into _buf (see d_basename).
D_INLINE
char*
basename(const char* _path,
         char*       _buf,
         size_t      _bufsize)
{
    return d_basename(_path, _buf, _bufsize);
}

// extension
//   pointer to the filename extension within _path (see d_get_extension).
D_NO_DISCARD D_INLINE
const char*
extension(const char* _path)
{
    return d_get_extension(_path);
}


// III. path composition and tests

// join
//   join _path1 and _path2 with a separator into _buf (see d_path_join).
D_INLINE
char*
join(char*       _buf,
     size_t      _bufsize,
     const char* _path1,
     const char* _path2)
{
    return d_path_join(_buf, _bufsize, _path1, _path2);
}

// normalize
//   collapse "." / ".." and redundant separators of _path into _buf (see
// d_path_normalize).
D_INLINE
char*
normalize(const char* _path,
          char*       _buf,
          size_t      _bufsize)
{
    return d_path_normalize(_path, _buf, _bufsize);
}

// is_absolute
//   true if _path is an absolute path (see d_path_is_absolute).
D_NO_DISCARD D_INLINE
bool
is_absolute(const char* _path)
{
    return d_path_is_absolute(_path) != 0;
}

NS_END  // path
NS_END  // file
NS_END  // djinterp


#endif  // DJINTERP_FILE_PATH_
