/******************************************************************************
* djinterp [core]                                                  file_pipe.h
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
* path:      \inc\djinterp\c\fs\file_pipe.h
* link:      TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.07.15
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    PIPES
      -----
      1.  d_popen    (run a command; read or write its stdio)
      2.  d_pclose   (close and reap; returns the EXIT STATUS)
*/

#ifndef DJINTERP_FILE_PIPE_
#define DJINTERP_FILE_PIPE_ 1

#include "./file_common.h"
#include "../../config/c/fs/cfg_file_pipe.h"


#if (D_INTERNAL_FILE_HAS_PIPES == 1)

D_EXTERN_C_BEGIN


// I.    Pipes
FILE* d_popen(const char* _command,
              const char* _mode);
int   d_pclose(FILE* _stream);


D_EXTERN_C_END

#endif  // D_INTERNAL_FILE_HAS_PIPES


#endif  // DJINTERP_FILE_PIPE_
