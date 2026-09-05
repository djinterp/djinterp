/******************************************************************************
* djinterp [c]                                                     file_temp.h
*
* Temporary files.
*   Two shapes, and only one of them is safe:
*     d_tmpfile / d_mkstemp   choose the name AND open it, atomically. No
*                             window. Use these.
*     d_tmpnam_s              hands you a name to open later. Between the two,
*                             anyone with write access to that directory can
*                             create it first -- classically as a symlink to
*                             something you have permission to destroy.
*   d_tmpnam_s exists for compatibility and is gated behind
* D_CFG_FILE_TEMP_ALLOW_TMPNAM so a codebase can prove it has no uses left.
*
* path:      /inc/djinterp/c/fs/file_temp.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.15
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    ATOMIC CREATION  (safe)
      -----------------------
      1.  d_tmpfile      (anonymous; deleted on close)
      2.  d_tmpfile_s    (as above, Annex K error reporting)
      3.  d_mkstemp      (named, from a template; opened atomically)

II.   NAME GENERATION  (racy -- see the header note)
      ---------------------------------------------
      1.  d_tmpnam_s

III.  LOCATION
      --------
      1.  d_tempdir
*/

#ifndef DJINTERP_FILE_TEMP_
#define DJINTERP_FILE_TEMP_ 1

// djinterp
#include "./file_common.h"
#include "../../config/c/fs/cfg_file_temp.h"


D_EXTERN_C_BEGIN


// I.    Atomic creation
FILE* d_tmpfile(void);
int   d_tmpfile_s(FILE** _stream);
int   d_mkstemp(char* _template);

// II.   Name generation
#if (D_INTERNAL_FILE_TEMP_TMPNAM == 1)
int   d_tmpnam_s(char*  _s,
                 size_t _maxsize);
#endif

// III.  Location
char* d_tempdir(char*  _buf,
                size_t _bufsize);


D_EXTERN_C_END


#endif  // DJINTERP_FILE_TEMP_
