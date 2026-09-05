/******************************************************************************
* djinterp [c]                                                     file_desc.h
*
* Descriptor lifecycle -- getting one, copying one, giving one back.
*   This module owns descriptors, not the bytes that move through them; those
* are file_io. So a program handed a descriptor by its parent, which only ever
* reads it, links file_io and never this.
*   Every descriptor here is close-on-exec by default (D_CFG_FILE_DESC_CLOEXEC)
* -- including the one d_dup returns, which POSIX would otherwise hand back
* with the flag silently cleared.
*
* path:      /inc/djinterp/c/fs/file_desc.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.15
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    ACQUISITION
      -----------
      1.  d_open        (POSIX open equivalent)
      2.  d_fileno      (borrow the descriptor inside a stream)

II.   DUPLICATION
      -----------
      1.  d_dup         (lowest free descriptor)
      2.  d_dup2        (a chosen descriptor number)

III.  RELEASE
      -------
      1.  d_close
*/

#ifndef DJINTERP_FILE_DESC_
#define DJINTERP_FILE_DESC_ 1

// djinterp
#include "./file_common.h"
#include "../../config/c/fs/cfg_file_desc.h"


D_EXTERN_C_BEGIN


// I.    Acquisition
int d_open(const char* _path,
           int         _flags,
           ...);
int d_fileno(FILE* _stream);

// II.   Duplication
int d_dup(int _fd);
int d_dup2(int _fd,
           int _fd2);

// III.  Release
int d_close(int _fd);


D_EXTERN_C_END


#endif  // DJINTERP_FILE_DESC_
