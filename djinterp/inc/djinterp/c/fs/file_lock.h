/******************************************************************************
* djinterp [c]                                                     file_lock.h
*
* Advisory file locking.
*   ADVISORY, and the word is load-bearing: a lock here is a convention among
* programs that agree to ask. A process that never calls d_flock writes the
* file regardless, and nothing reports it. There is no portable mandatory
* locking; if that is what you need, this module cannot supply it.
*
*   The semantics depend on the backend, and they are not interchangeable --
* see cfg_file_lock.h. In short: flock's lock lives on the open file
* description, fcntl's lives on the process and is dropped when ANY descriptor
* to the file closes. Query with D_FILE_LOCK_IS_PER_DESCRIPTION.
*
* path:      /inc/djinterp/c/fs/file_lock.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.15
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    LOCKING
      -------
      1.  d_flock          (by descriptor)
      2.  d_flock_stream   (by stream)

   Operations (from file_common.h): D_LOCK_SH, D_LOCK_EX, D_LOCK_UN,
   optionally OR'd with D_LOCK_NB to fail rather than block.
*/

#ifndef DJINTERP_FILE_LOCK_
#define DJINTERP_FILE_LOCK_ 1

// djinterp
#include "./file_common.h"
#include "../../config/c/fs/cfg_file_lock.h"


D_EXTERN_C_BEGIN


// I.    Locking
int d_flock(int _fd,
            int _operation);
int d_flock_stream(FILE* _stream,
                   int   _operation);


D_EXTERN_C_END


#endif  // DJINTERP_FILE_LOCK_
