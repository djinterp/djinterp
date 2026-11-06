/*******************************************************************************
* djinterp [config]                                                cfg_ostream.h
*
*   Configuration for the output stream: which backend writes to the console,
* and the size of the buffers djinterp provides itself. The kernel is
* otherwise configuration-free.
*
*   targets:  c/util/ostream/ostream_console.h -> D_INTERNAL_OSTREAM_CONSOLE
*             cpp/util/output/output_stream.hpp -> D_CFG_OSTREAM_BUFFER_SIZE
*   requires: cfg_common.h (helpers, D_CFG_TESTING);
*             c/filesys/cfg_file_common.h (the resolved file backend)
*   The template engine's knob, D_CFG_OSTREAM_TEMPLATE_DEPTH, is text's:
* config/c/text/cfg_ostream_template.h.
*
*   THE CONSOLE FOLLOWS c/filesys UNLESS TOLD OTHERWISE. AUTO takes the backend
* c/filesys resolved -- NATIVE on Windows, POSIX on a POSIX system, STDC on
* anything else -- so a build that pins D_CFG_FILE_BACKEND pins the console
* with it, and the two cannot disagree by accident. A freestanding
* implementation (__STDC_HOSTED__ == 0) has no stdout to follow and resolves to
* NONE: no default console, and d_ostream_init_console reports
* D_OSTREAM_UNSUPPORTED.
*
*   A PINNED BACKEND THE TARGET CANNOT SUPPLY IS A BUILD ERROR, as in
* c/filesys: a silent downgrade ships a binary without the behaviour it was
* pinned for.
*
*
* path:      /inc/djinterp/config/c/util/ostream/cfg_ostream.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.07
*                                                            revised: 2026.11.05
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Console backend
         1.  D_CFG_OSTREAM_CONSOLE_AUTO
         2.  D_CFG_OSTREAM_CONSOLE_NONE
         3.  D_CFG_OSTREAM_CONSOLE_STDC
         4.  D_CFG_OSTREAM_CONSOLE_POSIX
         5.  D_CFG_OSTREAM_CONSOLE_NATIVE
         6.  D_CFG_OSTREAM_CONSOLE_BACKEND
    2.  Buffers
         1.  D_CFG_OSTREAM_BUFFER_SIZE
2.  VALIDATION
    ----------
    1.  Literals, ranges and the target
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_OSTREAM_CONSOLE
*/

#ifndef DJINTERP_CONFIG_C_UTIL_OSTREAM_CFG_OSTREAM_H
#define DJINTERP_CONFIG_C_UTIL_OSTREAM_CFG_OSTREAM_H 1

// djinterp
#include "../../../cfg_common.h"            // D_CFG_IS_ON, D_CFG_NORM,
                                            // D_CFG_IS_INT_LITERAL
#include "../../filesys/cfg_file_common.h"  // D_FILE_BACKEND_IS_*,
                                            // D_CFG_FILE_HAS_*


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Console backend
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_OSTREAM_CONSOLE_AUTO
//   constant: backend selector: follow c/filesys (the default).
#define D_CFG_OSTREAM_CONSOLE_AUTO    0

// 1.1.2
// D_CFG_OSTREAM_CONSOLE_NONE
//   constant: backend selector: no console; d_ostream_init_console refuses.
#define D_CFG_OSTREAM_CONSOLE_NONE    1

// 1.1.3
// D_CFG_OSTREAM_CONSOLE_STDC
//   constant: backend selector: ISO C stdio and nothing else.
#define D_CFG_OSTREAM_CONSOLE_STDC    2

// 1.1.4
// D_CFG_OSTREAM_CONSOLE_POSIX
//   constant: backend selector: stdio for the bytes, isatty for the terminal.
#define D_CFG_OSTREAM_CONSOLE_POSIX   3

// 1.1.5
// D_CFG_OSTREAM_CONSOLE_NATIVE
//   constant: backend selector: the Win32 console API.
#define D_CFG_OSTREAM_CONSOLE_NATIVE  4

// 1.1.6
// D_CFG_OSTREAM_CONSOLE_BACKEND
//   knob: which backend writes to the console; one of the selectors above.
#ifndef D_CFG_OSTREAM_CONSOLE_BACKEND
    #define D_CFG_OSTREAM_CONSOLE_BACKEND D_CFG_OSTREAM_CONSOLE_AUTO
#endif  // D_CFG_OSTREAM_CONSOLE_BACKEND

// 1.2    Buffers
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_OSTREAM_BUFFER_SIZE
//   knob: the bytes of a buffer djinterp provides a transport itself -- the C++
// DBufferedOutputStream's default -- where the caller does not choose. 4096,
// a page: large enough that a drain costs little per byte, small enough to
// live on a stack. At least 64.
#ifndef D_CFG_OSTREAM_BUFFER_SIZE
    #define D_CFG_OSTREAM_BUFFER_SIZE 4096
#endif  // D_CFG_OSTREAM_BUFFER_SIZE


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Literals, ranges and the target
//------------------------------------------------------------------------------
#if !D_CFG_IS_INT_LITERAL(D_CFG_OSTREAM_CONSOLE_BACKEND)
    #error "D_CFG_OSTREAM_CONSOLE_BACKEND must name one of its values"
#endif

#if ( (D_CFG_NORM(D_CFG_OSTREAM_CONSOLE_BACKEND) <                             \
       D_CFG_OSTREAM_CONSOLE_AUTO) ||                                          \
      (D_CFG_NORM(D_CFG_OSTREAM_CONSOLE_BACKEND) >                             \
       D_CFG_OSTREAM_CONSOLE_NATIVE) )
    #error "D_CFG_OSTREAM_CONSOLE_BACKEND must be a console backend selector"
#endif

#if ( (D_CFG_NORM(D_CFG_OSTREAM_CONSOLE_BACKEND) ==                            \
       D_CFG_OSTREAM_CONSOLE_POSIX) &&                                         \
      (D_CFG_IS_OFF(D_CFG_FILE_HAS_POSIX)) )
    #error "D_CFG_OSTREAM_CONSOLE_BACKEND is POSIX; the target has no POSIX"
#endif

#if ( (D_CFG_NORM(D_CFG_OSTREAM_CONSOLE_BACKEND) ==                            \
       D_CFG_OSTREAM_CONSOLE_NATIVE) &&                                        \
      (D_CFG_IS_OFF(D_CFG_FILE_HAS_WIN32)) )
    #error "D_CFG_OSTREAM_CONSOLE_BACKEND is NATIVE, which is Win32's"
#endif

#if ((D_CFG_OSTREAM_BUFFER_SIZE + 0) < 64)
    #error "D_CFG_OSTREAM_BUFFER_SIZE must be a number of bytes, at least 64"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================


// 3.1    Effective values
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_OSTREAM_CONSOLE
//   resolved: the backend in effect -- a D_CFG_OSTREAM_CONSOLE_* selector,
// never AUTO. Do not override it; set D_CFG_OSTREAM_CONSOLE_BACKEND instead.
#if (D_CFG_NORM(D_CFG_OSTREAM_CONSOLE_BACKEND) != D_CFG_OSTREAM_CONSOLE_AUTO)
    #define D_INTERNAL_OSTREAM_CONSOLE                                         \
        D_CFG_NORM(D_CFG_OSTREAM_CONSOLE_BACKEND)
#elif ( (defined(__STDC_HOSTED__)) &&                                          \
        (__STDC_HOSTED__ == 0) )
    #define D_INTERNAL_OSTREAM_CONSOLE D_CFG_OSTREAM_CONSOLE_NONE
#elif ( (D_FILE_BACKEND_IS_NATIVE) &&                                          \
        (D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) )
    #define D_INTERNAL_OSTREAM_CONSOLE D_CFG_OSTREAM_CONSOLE_NATIVE
#elif ( (D_FILE_BACKEND_IS_STDC) ||                                            \
        (D_CFG_IS_OFF(D_CFG_FILE_HAS_POSIX)) )
    #define D_INTERNAL_OSTREAM_CONSOLE D_CFG_OSTREAM_CONSOLE_STDC
#else
    #define D_INTERNAL_OSTREAM_CONSOLE D_CFG_OSTREAM_CONSOLE_POSIX
#endif


#endif  // DJINTERP_CONFIG_C_UTIL_OSTREAM_CFG_OSTREAM_H
