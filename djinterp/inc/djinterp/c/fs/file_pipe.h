/*******************************************************************************
* djinterp [c]                                                       file_pipe.h
*
* Process pipes.
*   d_pipe_open runs its argument through a SHELL -- /bin/sh -c on POSIX,
* cmd.exe /c on Windows. That means every shell metacharacter in the string is
* interpreted, so building the command from anything a user influenced is a
* command-injection vulnerability. No configuration here prevents that; the
* only fix is not to call it. Use posix_spawn / CreateProcess with an argument
* vector when the command is not a literal.
*   The whole API is compiled out when D_INTERNAL_FILE_HAS_PIPES is 0 -- guard
* with D_FILE_PIPE_IS_AVAILABLE.
*
*
* path:      /inc/djinterp/c/fs/file_pipe.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  PIPES
    -----
    1.  Process pipes
*/

#ifndef DJINTERP_C_FS_FILE_PIPE_H
#define DJINTERP_C_FS_FILE_PIPE_H 1

// std
#include <stdio.h>  // FILE
// djinterp
#include "./file_common.h"                    // fs foundation, D_EXTERN_C_*
#include "../../config/c/fs/cfg_file_pipe.h"  // module configuration
// re_std
#include "../../../re_std/cstdint/dstdint.h"  // INT64_MAX: this header's floor

// 64-bit floor: this header needs a 64-bit integer type, which dstdint.h
// declares only where the build can spell one. Below it -- ISO strict
// C++98 on a 32-bit target -- the header compiles to nothing (the owner's
// ruling of 2026.10.03 on round 3's question 1, (a)).
#if defined(INT64_MAX)


#if (D_INTERNAL_FILE_HAS_PIPES == 1)

D_EXTERN_C_BEGIN


//==============================================================================
// 1.  PIPES
//==============================================================================
// POSIX pclose returns a wait status, not an exit code: `exit 3` yields 768.
// d_pipe_exit_code decodes it -- N for a normal exit, 128+N for death by
// signal N, -1 for a failed reap -- so callers never see the raw encoding. On
// Windows _pclose already returns the code, so the call is the identity there.
// The decode lives in this module because <sys/wait.h> is an OS header and the
// C++ layer may not read one.


// 1.1    Process pipes
//------------------------------------------------------------------------------
/**
 * @brief Runs a command through the system shell and connects a stream to
 *        its standard input or output.
 *
 * @warning The command goes through a shell, so every metacharacter is
 *          interpreted: a command built from anything a user influenced is a
 *          command-injection vulnerability, and no quoting survives both
 *          shells.
 *
 * @param[in] _command  the command line, passed to the shell.
 * @param[in] _mode     "r" to read the child's stdout, "w" to write its
 *                      stdin.
 * @return an open stream, or `NULL` on failure with errno set.
 * @post Close the stream with d_pipe_close -- never fclose, which does not
 *       reap the child.
 */
FILE* d_pipe_open(const char* _command,
                  const char* _mode);
/**
 * @brief Closes a pipe and waits for the child to finish.
 *
 * @warning The result is the child's status, not a success flag: 0 means the
 *          command ran and succeeded, other values report how it ended, and
 *          only -1 means the wait itself failed. d_pipe_exit_code decodes it.
 *
 * @param[in] _stream  a stream from d_pipe_open.
 * @return the platform's status -- a wait(2) status on POSIX, the exit code
 *         on Windows -- or -1 if the wait failed.
 * @post `_stream` is closed and must not be used again.
 */
int   d_pipe_close(FILE* _stream);
/**
 * @brief Decodes what d_pipe_close returned into the command's exit code, in
 *        the shell's vocabulary.
 *
 * @param[in] _status  the value d_pipe_close returned.
 * @return N for a normal exit, 128 + N for death by signal N, -1 when
 *         `_status` reports a failed wait, and any other status unchanged.
 */
int   d_pipe_exit_code(int _status);


D_EXTERN_C_END

#endif  // D_INTERNAL_FILE_HAS_PIPES


#endif  // defined(INT64_MAX)

#endif  // DJINTERP_C_FS_FILE_PIPE_H
