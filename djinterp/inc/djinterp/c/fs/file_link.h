/******************************************************************************
* djinterp [c]                                                     file_link.h
*
* Symbolic links.
*   The whole API is compiled out when D_INTERNAL_FILE_HAS_SYMLINKS is 0, so
* guard your uses with D_FILE_LINK_IS_AVAILABLE.
*   That macro is a claim about the PLATFORM, not about your process. Windows
* has had symlinks since Vista and still refuses to create one without
* SeCreateSymbolicLinkPrivilege -- so d_symlink can compile, be available, and
* fail with EPERM for every ordinary user. Handle the runtime failure; do not
* infer it from the macro.
*
* path:      /inc/djinterp/c/fs/file_link.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.15
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    SYMBOLIC LINKS
      --------------
      1.  d_symlink      (create)
      2.  d_readlink     (read the target; does NOT resolve it)
      3.  d_is_symlink   (test)
*/

#ifndef DJINTERP_FILE_LINK_
#define DJINTERP_FILE_LINK_ 1

// djinterp
#include "./file_common.h"
#include "../../config/c/fs/cfg_file_link.h"


#if (D_INTERNAL_FILE_HAS_SYMLINKS == 1)

D_EXTERN_C_BEGIN


// I.    Symbolic links
int     d_symlink(const char* _target,
                  const char* _linkpath);
ssize_t d_readlink(const char* _path,
                   char*       _buf,
                   size_t      _bufsize);
int     d_is_symlink(const char* _path);


D_EXTERN_C_END

#endif  // D_INTERNAL_FILE_HAS_SYMLINKS


#endif  // DJINTERP_FILE_LINK_
