/*******************************************************************************
* djinterp [config]                                              cfg_file_open.h
*
* Build-time configuration for c/fs/file_open.h -- opening, reopening and
* closing FILE* streams.
*
*   file_read and file_write both depend on this module, since a whole-file
* operation has to open the file first; every path-handling decision (wide
* paths, long paths, share semantics) is therefore made here exactly once and
* inherited by both.
*
*   targets:  c/fs/file_open.h, c/fs/file_open.c -> D_INTERNAL_FILE_OPEN_*
*   requires: cfg_file_common.h
*
*
* path:      /inc/djinterp/config/c/fs/cfg_file_open.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.30
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Secure variants
         1.  D_CFG_FILE_OPEN_USE_FOPEN_S
    2.  Mode handling
         1.  D_CFG_FILE_OPEN_BINARY_DEFAULT
         2.  D_CFG_FILE_OPEN_CLOEXEC
    3.  Sharing (Windows)
         1.  D_CFG_FILE_OPEN_SHARE_DEFAULT
         2.  D_CFG_FILE_OPEN_SHARE_NONE
         3.  D_CFG_FILE_OPEN_SHARE_READ
         4.  D_CFG_FILE_OPEN_SHARE_ALL
         5.  D_CFG_FILE_OPEN_SHARE
    4.  Buffering
         1.  D_CFG_FILE_OPEN_BUFFER_SIZE
2.  VALIDATION
    ----------
    1.  Knob validation
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_FILE_OPEN_FOPEN_S
         2.  D_INTERNAL_FILE_OPEN_CLOEXEC_CHAR
         3.  D_INTERNAL_FILE_OPEN_DECORATE
         4.  D_INTERNAL_FILE_OPEN_MODE_MAX
         5.  D_INTERNAL_FILE_OPEN_SHARE_FLAG
         6.  D_INTERNAL_FILE_OPEN_SETVBUF
*/

#ifndef DJINTERP_CONFIG_C_FS_CFG_FILE_OPEN_H
#define DJINTERP_CONFIG_C_FS_CFG_FILE_OPEN_H 1

// djinterp
#include "cfg_file_common.h"  // shared fs knobs, D_CFG_IS_BOOL


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Secure variants
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_FILE_OPEN_USE_FOPEN_S
//   knob: route d_file_open_stream through Annex K's fopen_s where it exists.
// Follows the subframework's D_CFG_FILE_PREFER_ANNEX_K. Worth knowing before
// you leave this on: MSVC's fopen_s opens files with sharing denied, so a
// second opener -- including a tail -f, an editor, or your own test harness --
// is locked out. If that bites, set D_CFG_FILE_OPEN_SHARE instead of turning
// this off.
#ifndef D_CFG_FILE_OPEN_USE_FOPEN_S
    #define D_CFG_FILE_OPEN_USE_FOPEN_S D_CFG_FILE_PREFER_ANNEX_K
#endif  // D_CFG_FILE_OPEN_USE_FOPEN_S

// 1.2    Mode handling
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_FILE_OPEN_BINARY_DEFAULT
//   knob: append 'b' to any mode string that names neither 'b' nor 't', so
// a stream never silently translates line endings.
//   Off by default, because it is a real behaviour change: on Windows a text
// stream turns "\n" into "\r\n" on the way out and back on the way in, and a
// program written against that is entitled to it. Turn it on for a program
// that wants byte-for-byte identical files across platforms -- which is most
// programs that are not writing a .txt for Notepad.
#ifndef D_CFG_FILE_OPEN_BINARY_DEFAULT
    #define D_CFG_FILE_OPEN_BINARY_DEFAULT 0
#endif  // D_CFG_FILE_OPEN_BINARY_DEFAULT

// 1.2.2
// D_CFG_FILE_OPEN_CLOEXEC
//   knob: ask for close-on-exec in the mode string ('e' on glibc, 'N' on
// MSVC) so a stream is not inherited by a child process. Follows the
// subframework's D_CFG_FILE_CLOEXEC_DEFAULT.
//   Where the platform has no mode-string spelling this resolves to 0: the
// alternative is a second fcntl syscall per open, which is not a cost to
// impose silently.
#ifndef D_CFG_FILE_OPEN_CLOEXEC
    #define D_CFG_FILE_OPEN_CLOEXEC D_CFG_FILE_CLOEXEC_DEFAULT
#endif  // D_CFG_FILE_OPEN_CLOEXEC

// 1.3    Sharing (Windows)
//------------------------------------------------------------------------------
//   Selection model. POSIX has no equivalent -- an open file is shareable
// and locking is advisory -- so these are ignored there.

// 1.3.1
// D_CFG_FILE_OPEN_SHARE_DEFAULT
//   constant: whatever the CRT does on its own.
#define D_CFG_FILE_OPEN_SHARE_DEFAULT    0

// 1.3.2
// D_CFG_FILE_OPEN_SHARE_NONE
//   constant: deny all other access (_SH_DENYRW).
#define D_CFG_FILE_OPEN_SHARE_NONE       1

// 1.3.3
// D_CFG_FILE_OPEN_SHARE_READ
//   constant: permit concurrent readers, deny writers (_SH_DENYWR).
#define D_CFG_FILE_OPEN_SHARE_READ       2

// 1.3.4
// D_CFG_FILE_OPEN_SHARE_ALL
//   constant: permit everything (_SH_DENYNO) -- POSIX-like behaviour.
#define D_CFG_FILE_OPEN_SHARE_ALL        3

// 1.3.5
// D_CFG_FILE_OPEN_SHARE
//   knob: what other openers may do while this stream is open. Defaults to
// SHARE_DEFAULT, which changes nothing.
//   Set SHARE_ALL to make Windows behave like POSIX; that is what you want
// if your program is cross-platform and was written against POSIX sharing.
#ifndef D_CFG_FILE_OPEN_SHARE
    #define D_CFG_FILE_OPEN_SHARE D_CFG_FILE_OPEN_SHARE_DEFAULT
#endif  // D_CFG_FILE_OPEN_SHARE

#if !D_CFG_IS_INT_LITERAL(D_CFG_FILE_OPEN_SHARE)
    #error "D_CFG_FILE_OPEN_SHARE must name one of its values; a misspelled name would read as 0"
#endif

// 1.4    Buffering
//------------------------------------------------------------------------------
// D_CFG_FILE_OPEN_BUFFER_MODE
//   constant/brief: stdio buffering applied to a freshly opened stream
// (value model): 0 leaves the platform's choice alone, otherwise one of
// _IOFBF (full), _IOLBF (line) or _IONBF (none). Left alone by default --
// stdio's own heuristics are good, and overriding them costs an allocation
// per stream.
#ifndef D_CFG_FILE_OPEN_BUFFER_MODE
    #define D_CFG_FILE_OPEN_BUFFER_MODE 0
#endif  // D_CFG_FILE_OPEN_BUFFER_MODE

// 1.4.1
// D_CFG_FILE_OPEN_BUFFER_SIZE
//   knob: buffer size in bytes applied with the mode above (value model).
// Ignored when D_CFG_FILE_OPEN_BUFFER_MODE is 0.
#ifndef D_CFG_FILE_OPEN_BUFFER_SIZE
    #define D_CFG_FILE_OPEN_BUFFER_SIZE (64u * 1024u)
#endif  // D_CFG_FILE_OPEN_BUFFER_SIZE


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
#if !D_CFG_IS_BOOL(D_CFG_FILE_OPEN_USE_FOPEN_S)
    #error "D_CFG_FILE_OPEN_USE_FOPEN_S must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_OPEN_BINARY_DEFAULT)
    #error "D_CFG_FILE_OPEN_BINARY_DEFAULT must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_OPEN_CLOEXEC)
    #error "D_CFG_FILE_OPEN_CLOEXEC must be literally 0 or 1"
#endif
#if ( (D_CFG_NORM(D_CFG_FILE_OPEN_SHARE) < D_CFG_FILE_OPEN_SHARE_DEFAULT) ||   \
      (D_CFG_NORM(D_CFG_FILE_OPEN_SHARE) > D_CFG_FILE_OPEN_SHARE_ALL) )
    #error "D_CFG_FILE_OPEN_SHARE must be one of D_CFG_FILE_OPEN_SHARE_*"
#endif

// fopen_s and _fsopen are different entry points; asking for both is asking
// for one of the two settings to be silently ignored. Say so instead.
#if ( (D_CFG_IS_ON(D_CFG_FILE_OPEN_USE_FOPEN_S)) &&                            \
      (D_CFG_NORM(D_CFG_FILE_OPEN_SHARE) != D_CFG_FILE_OPEN_SHARE_DEFAULT) &&  \
      (D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) )
    #error "D_CFG_FILE_OPEN_SHARE requires D_CFG_FILE_OPEN_USE_FOPEN_S == 0"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================


// 3.1    Effective values
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_FILE_OPEN_FOPEN_S
//   resolved: 1 when d_file_open_stream actually calls fopen_s -- it must
// exist, be preferred subframework-wide, and be wanted by this module.
#if ( (D_INTERNAL_FILE_HAS_FOPEN_S == 1) &&                                    \
      (D_CFG_IS_ON(D_CFG_FILE_OPEN_USE_FOPEN_S)) )
    #define D_INTERNAL_FILE_OPEN_FOPEN_S 1
#else
    #define D_INTERNAL_FILE_OPEN_FOPEN_S 0
#endif

// 3.1.2
// D_INTERNAL_FILE_OPEN_CLOEXEC_CHAR
//   resolved: the mode-string character that requests close-on-exec on this
// target, or 0 when the target has no such spelling and the request must be
// dropped. glibc took 'e'; the MSVC CRT took 'N'; everyone else took neither.
#if D_CFG_IS_OFF(D_CFG_FILE_OPEN_CLOEXEC)
    #define D_INTERNAL_FILE_OPEN_CLOEXEC_CHAR 0
#elif defined(__GLIBC__)
    #define D_INTERNAL_FILE_OPEN_CLOEXEC_CHAR 'e'
#elif D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    #define D_INTERNAL_FILE_OPEN_CLOEXEC_CHAR 'N'
#else
    #define D_INTERNAL_FILE_OPEN_CLOEXEC_CHAR 0
#endif

// 3.1.3
// D_INTERNAL_FILE_OPEN_DECORATE
//   resolved: 1 when d_file_open_stream must rewrite the caller's mode string
// before passing it on. When 0 the mode goes through untouched and the rewrite
// buffer is not even declared.
#if ( (D_CFG_IS_ON(D_CFG_FILE_OPEN_BINARY_DEFAULT)) ||                         \
      (D_INTERNAL_FILE_OPEN_CLOEXEC_CHAR != 0) )
    #define D_INTERNAL_FILE_OPEN_DECORATE 1
#else
    #define D_INTERNAL_FILE_OPEN_DECORATE 0
#endif

// 3.1.4
// D_INTERNAL_FILE_OPEN_MODE_MAX
//   resolved: size of the rewritten-mode buffer. A mode string is a handful of
// characters ("rb+"); 16 leaves room for every decoration and keeps the
// buffer on the stack.
#define D_INTERNAL_FILE_OPEN_MODE_MAX 16

// 3.1.5
// D_INTERNAL_FILE_OPEN_SHARE_FLAG
//   resolved: the _fsopen sharing flag this build asks for, or 0 when sharing
// is left to the CRT / the platform has no such concept.
#if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) &&                                   \
      (D_CFG_NORM(D_CFG_FILE_OPEN_SHARE) != D_CFG_FILE_OPEN_SHARE_DEFAULT) )
    #if (D_CFG_NORM(D_CFG_FILE_OPEN_SHARE) == D_CFG_FILE_OPEN_SHARE_NONE)
        #define D_INTERNAL_FILE_OPEN_SHARE_FLAG _SH_DENYRW
    #elif (D_CFG_NORM(D_CFG_FILE_OPEN_SHARE) == D_CFG_FILE_OPEN_SHARE_READ)
        #define D_INTERNAL_FILE_OPEN_SHARE_FLAG _SH_DENYWR
    #else
        #define D_INTERNAL_FILE_OPEN_SHARE_FLAG _SH_DENYNO
    #endif
    #define D_INTERNAL_FILE_OPEN_USE_FSOPEN 1
#else
    #define D_INTERNAL_FILE_OPEN_USE_FSOPEN 0
#endif

// 3.1.6
// D_INTERNAL_FILE_OPEN_SETVBUF
//   resolved: 1 when a freshly opened stream gets an explicit buffering call.
#if (D_CFG_NORM(D_CFG_FILE_OPEN_BUFFER_MODE) != 0)
    #define D_INTERNAL_FILE_OPEN_SETVBUF 1
#else
    #define D_INTERNAL_FILE_OPEN_SETVBUF 0
#endif


#endif  // DJINTERP_CONFIG_C_FS_CFG_FILE_OPEN_H
