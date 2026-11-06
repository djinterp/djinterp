/*******************************************************************************
* djinterp [c]                                                 ostream_console.h
*
*   The console as a stream: stdout or stderr, written the way the platform
* needs.
*
*   THE BACKEND IS CHOSEN ONCE, in config/c/util/ostream/cfg_ostream.h, and
* follows
* c/filesys unless pinned:
*     POSIX   the bytes go through stdio, so they interleave with printf, and
*             isatty answers whether the channel is a terminal.
*     NATIVE  Windows. On a real console, text goes out as UTF-16 through
*             WriteConsoleW, an incomplete UTF-8 sequence carried over to the
*             next write, so no code page is involved and nothing has to call
*             SetConsoleOutputCP. Redirected text goes through the C library,
*             exactly as printf's would; binary goes through WriteFile,
*             untranslated. The C library's buffer is flushed before any direct
*             write, so an earlier printf is never overtaken.
*     STDC    ISO C stdio and nothing else: no terminal detection, and binary
*             output refused where the C library translates newlines.
*     NONE    no console (a freestanding build): the stream is refused with
*             D_OSTREAM_UNSUPPORTED.
*
*   THE STATE IS THE CALLER'S. struct dj_ostream_console holds what a backend
* needs between writes -- the handle, whether it is a device, the carried
* UTF-8 bytes -- in storage the caller owns, one per stream. Nothing is
* static, so two streams on one console share no hidden state; a stream
* still belongs to one thread at a time.
*
*   ESCAPE SEQUENCES ARE ASKED FOR, NOT ASSUMED. A stream's query says whether
* they are interpreted now; d_ostream_console_enable_vt asks Windows to start
* interpreting them, which changes the console's mode -- the reason a query
* never does it.
*
*
* path:      /inc/djinterp/c/util/ostream/ostream_console.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.11.05
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  TYPES
    -----
    1.  The console's state
         1.  dj_ostream_console_channel
         2.  dj_ostream_console
2.  OPERATIONS
    ----------
    1.  The console as a stream
*/

#ifndef DJINTERP_C_UTIL_OSTREAM_OSTREAM_CONSOLE_H
#define DJINTERP_C_UTIL_OSTREAM_OSTREAM_CONSOLE_H 1

// djinterp
#include "../../djinterp.h"  // framework root
#include "./ostream.h"       // struct dj_ostream
#include "../../../config/c/util/ostream/cfg_ostream.h"
                             // D_CFG_OSTREAM_CONSOLE_*
// re_std
#include "../../../../re_std/c/cstdint/dstdint.h"  // int32_t


D_EXTERN_C_BEGIN


//==============================================================================
// 1.  TYPES
//==============================================================================


// 1.1    The console's state
//------------------------------------------------------------------------------
// 1.1.1
// dj_ostream_console_channel
//   enum: which standard channel a console stream writes.
enum dj_ostream_console_channel
{
    D_OSTREAM_CONSOLE_OUT = 1,  // stdout
    D_OSTREAM_CONSOLE_ERR = 2   // stderr
};

// 1.1.2
// dj_ostream_console
//   struct: a console stream's state, in the caller's storage, one per
// stream, alive as long as the stream. Its members are the backend's.
struct dj_ostream_console
{
    void*         handle;         // the backend's handle for the channel
    void*         stdio;          // the C library's FILE* for the channel
    int32_t       channel;        // enum dj_ostream_console_channel
    int32_t       state;          // the backend's bits
    int32_t       pending_count;  // bytes held in pending
    unsigned char pending[4];     // an incomplete UTF-8 sequence, carried over
};


//==============================================================================
// 2.  OPERATIONS
//==============================================================================


// 2.1    The console as a stream
//------------------------------------------------------------------------------
/**
 * @brief Makes a stream over stdout or stderr.
 *
 * @note On a terminal the stream is line-buffered, which matters once it is
 *       given a buffer. When the channel cannot be opened -- a Windows GUI
 *       process has none -- the stream is left D_OSTREAM_FAILED; without a
 *       console backend, or for binary output the backend cannot keep
 *       untranslated, D_OSTREAM_UNSUPPORTED.
 *
 * @param[out] _os       the stream.
 * @param[out] _console  the stream's state, which must outlive it.
 * @param[in]  _channel  D_OSTREAM_CONSOLE_OUT or D_OSTREAM_CONSOLE_ERR.
 * @param[in]  _flags    D_OSTREAM_BINARY and D_OSTREAM_LINE_BUFFERED, or 0;
 *                       D_OSTREAM_OWNS_CONTEXT is ignored: nothing here is
 *                       released.
 * @return 1, or 0 when the stream was refused.
 */
int32_t d_ostream_init_console(struct dj_ostream*         _os,
                               struct dj_ostream_console* _console,
                               int32_t                    _channel,
                               int32_t                    _flags);
/**
 * @brief Asks for ANSI/VT escape sequences to be interpreted on a console.
 *
 * @note On Windows this sets ENABLE_VIRTUAL_TERMINAL_PROCESSING on the
 *       console, Windows 10 and later; elsewhere it changes nothing and
 *       answers whether the channel is a terminal that interprets them.
 *
 * @param[in,out] _console  the state of a console stream.
 * @return 1 when escape sequences will be interpreted, 0 otherwise.
 */
int32_t d_ostream_console_enable_vt(struct dj_ostream_console* _console);
// the backend this build uses: a D_CFG_OSTREAM_CONSOLE_* selector, never AUTO
int32_t d_ostream_console_backend(void);


D_EXTERN_C_END


#endif  // DJINTERP_C_UTIL_OSTREAM_OSTREAM_CONSOLE_H
