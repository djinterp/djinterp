/******************************************************************************
* djinterp [c]                                                     file_pipe.h
*
* Process pipes.
*   d_popen runs its argument through a SHELL -- /bin/sh -c on POSIX, cmd.exe
* /c on Windows. That means every shell metacharacter in the string is
* interpreted, so building the command from anything a user influenced is a
* command-injection vulnerability. No configuration here prevents that; the
* only fix is not to call it. Use posix_spawn / CreateProcess with an argument
* vector when the command is not a literal.
*   The whole API is compiled out when D_INTERNAL_FILE_HAS_PIPES is 0 -- guard
* with D_FILE_PIPE_IS_AVAILABLE.
*
* path:      /inc/djinterp/c/fs/file_pipe.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.15
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    PIPES
      -----
      1.  d_popen           (run a command; read or write its stdio)
      2.  d_pclose          (close and reap; returns the raw WAIT STATUS)
      3.  d_pipe_exit_code  (decode that status into the command's exit code)

   POSIX pclose returns a wait status, not an exit code: `exit 3` yields 768.
   d_pipe_exit_code decodes it -- N for a normal exit, 128+N for death by
   signal N, -1 for a failed reap -- so callers never see the raw encoding. On
   Windows _pclose already returns the code, so the call is the identity
   there. The decode lives in this module because <sys/wait.h> is an OS header
   and the C++ layer may not read one.
*/

#ifndef DJINTERP_FILE_PIPE_
#define DJINTERP_FILE_PIPE_ 1

// djinterp
#include "./file_common.h"
#include "../../config/c/fs/cfg_file_pipe.h"


#if (D_INTERNAL_FILE_HAS_PIPES == 1)

D_EXTERN_C_BEGIN


// I.    Pipes
FILE* d_popen(const char* _command,
              const char* _mode);
int   d_pclose(FILE* _stream);

int   d_pipe_exit_code(int _status);


D_EXTERN_C_END

#endif  // D_INTERNAL_FILE_HAS_PIPES


#endif  // DJINTERP_FILE_PIPE_
