/******************************************************************************
* djinterp [core]                                                  file_open.h
*
* Opening, reopening and closing FILE* streams.
*   Every path this subframework opens goes through here, so the decisions
* that have to be made uniformly -- how a mode string is interpreted, whether
* a stream is inherited across exec, what other processes may do to the file
* meanwhile -- are made once and inherited by file_read, file_write and
* anything else that needs a stream.
*   This module owns streams only. Raw descriptors are file_desc.h.
*
* path:      \inc\djinterp\c\fs\file_open.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.15
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    STREAM OPENING
      --------------
      1.  d_fopen        (portable fopen)
      2.  d_fopen_s      (C11 Annex K fopen_s equivalent)
      3.  d_freopen      (portable freopen)
      4.  d_freopen_s    (C11 Annex K freopen_s equivalent)
      5.  d_fdopen       (POSIX fdopen equivalent)

II.   STREAM CLOSING
      --------------
      1.  d_fclose       (portable fclose)
*/

#ifndef DJINTERP_FILE_OPEN_
#define DJINTERP_FILE_OPEN_ 1

#include "./file_common.h"
#include "../../config/c/fs/cfg_file_open.h"


D_EXTERN_C_BEGIN


// I.    Stream opening
FILE* d_fopen(const char* _filename,
              const char* _mode);
int   d_fopen_s(FILE**      _stream,
                const char* _filename,
                const char* _mode);
FILE* d_freopen(const char* _filename,
                const char* _mode,
                FILE*       _stream);
int   d_freopen_s(FILE**      _newstream,
                  const char* _filename,
                  const char* _mode,
                  FILE*       _stream);
FILE* d_fdopen(int         _fd,
               const char* _mode);

// II.   Stream closing
int   d_fclose(FILE* _stream);


D_EXTERN_C_END


#endif  // DJINTERP_FILE_OPEN_
