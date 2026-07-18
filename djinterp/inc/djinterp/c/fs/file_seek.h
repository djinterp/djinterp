/******************************************************************************
* djinterp [core]                                                  file_seek.h
*
* 64-bit positioning and truncation.
*   Exists as its own module because "where am I in this file" and "how big is
* this file" are one concern: both are the file's extent, and both are where a
* 32-bit off_t silently corrupts data on a large file.
*   d_ftello/d_fseeko are 64-bit on every target, including a 32-bit Windows
* build where ftell() would wrap at 2 GiB and report a plausible wrong answer.
*
* path:      \inc\djinterp\c\fs\file_seek.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.15
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    POSITIONING
      -----------
      1.  d_fseeko            (POSIX fseeko / Win32 _fseeki64)
      2.  d_ftello            (POSIX ftello / Win32 _ftelli64)
      3.  d_rewind            (seek to 0 and clear the error flag)

II.   TRUNCATION
      ----------
      1.  d_ftruncate         (by descriptor)
      2.  d_ftruncate_stream  (by stream; flushes first)
*/

#ifndef DJINTERP_FILE_SEEK_
#define DJINTERP_FILE_SEEK_ 1

#include "./file_common.h"
#include "../../config/c/fs/cfg_file_seek.h"


D_EXTERN_C_BEGIN


// I.    Positioning
int     d_fseeko(FILE*   _stream,
                 d_off_t _offset,
                 int     _whence);
d_off_t d_ftello(FILE* _stream);
int     d_rewind(FILE* _stream);

// II.   Truncation
int     d_ftruncate(int     _fd,
                    d_off_t _length);
int     d_ftruncate_stream(FILE*   _stream,
                           d_off_t _length);


D_EXTERN_C_END


#endif  // DJINTERP_FILE_SEEK_
