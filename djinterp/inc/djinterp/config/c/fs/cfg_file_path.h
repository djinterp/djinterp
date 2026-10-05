/*******************************************************************************
* djinterp [config]                                              cfg_file_path.h
*
* Build-time configuration for c/fs/file_path.h -- lexical path
* manipulation.
*
*   file_path is the one fs module that issues no system calls at all. It
* answers questions about what a path SAYS, never about what is on disk:
* d_path_dirname("/nonexistent/x") is "/nonexistent" whether or not that
* directory exists. Anything that has to look at the filesystem (getcwd,
* chdir, realpath) lives in file_dir instead.
*   That is why the knobs here are about SYNTAX -- which separators count,
* whether a drive letter is a root -- and not about platform capability.
*
*   targets:  c/fs/file_path.h, c/fs/file_path.c -> D_INTERNAL_FILE_PATH_*
*   requires: cfg_file_common.h
*
*
* path:      /inc/djinterp/config/c/fs/cfg_file_path.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.30
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Path syntax
         1.  D_CFG_FILE_PATH_SYNTAX_POSIX
         2.  D_CFG_FILE_PATH_SYNTAX_WINDOWS
         3.  D_CFG_FILE_PATH_SYNTAX
         4.  D_CFG_FILE_PATH_ACCEPT_ALT_SEP
         5.  D_CFG_FILE_PATH_OUT_SEP
    2.  Normalization
         1.  D_CFG_FILE_PATH_NORMALIZE_DOTDOT
         2.  D_CFG_FILE_PATH_COLLAPSE_SEPARATORS
         3.  D_CFG_FILE_PATH_STRIP_TRAILING_SEP
    3.  Join
         1.  D_CFG_FILE_PATH_JOIN_ABSOLUTE_WINS
2.  VALIDATION
    ----------
    1.  Knob validation
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_FILE_PATH_WINDOWS
         2.  D_INTERNAL_FILE_PATH_ALT_SEP
         3.  D_INTERNAL_FILE_PATH_HAS_DRIVE
         4.  D_INTERNAL_FILE_PATH_HAS_UNC
         5.  D_INTERNAL_FILE_PATH_OUT_SEP
4.  QUERIES
    -------
    1.  Public query macros
         1.  D_FILE_PATH_UNDERSTANDS_WINDOWS
         2.  D_FILE_PATH_OUT_SEP / D_FILE_PATH_OUT_SEP_STR
*/

#ifndef DJINTERP_CONFIG_C_FS_CFG_FILE_PATH_H
#define DJINTERP_CONFIG_C_FS_CFG_FILE_PATH_H 1

// djinterp
#include "cfg_file_common.h"  // shared fs knobs, D_CFG_IS_BOOL


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Path syntax
//------------------------------------------------------------------------------
//   Selection model, and the master knob of this module.
//
//   THIS IS ONE QUESTION, NOT FOUR. An earlier cut of this file had separate
// booleans for alt separators, drive letters and UNC, and they could be set
// into states that do not exist in reality: drive letters recognised while
// '\\' was not a separator, so "C:\\x" parsed as the drive-relative "C:"
// followed by a filename called "\\x". Those three are not independent
// aspects of a path -- they are consequences of WHICH SYNTAX you are reading.
// So there is one knob, and the rest derive from it.

// 1.1.1
// D_CFG_FILE_PATH_SYNTAX_POSIX
//   constant: '/' separates; no drive letters; no UNC; '\\' is an ordinary
// byte in a filename, because on POSIX it genuinely is one.
#define D_CFG_FILE_PATH_SYNTAX_POSIX    0

// 1.1.2
// D_CFG_FILE_PATH_SYNTAX_WINDOWS
//   constant: '\\' separates (and '/' too, see ACCEPT_ALT_SEP); "C:" is a
// drive root; "\\\\server\\share" is a UNC root.
#define D_CFG_FILE_PATH_SYNTAX_WINDOWS  1

// 1.1.3
// D_CFG_FILE_PATH_SYNTAX
//   knob: which path grammar this build parses. Defaults to the host's own.
//   Override it to read the OTHER platform's paths -- a cross-compiler
// resolving a Windows manifest, an archiver writing a ZIP, a build system
// consuming paths it did not create. That is a real use case and it is why
// this is a knob rather than a platform check: the question "what syntax is
// this path in" is not the same question as "what OS am I running on", and
// conflating them is what made the previous design incoherent.
#ifndef D_CFG_FILE_PATH_SYNTAX
    #if D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
        #define D_CFG_FILE_PATH_SYNTAX D_CFG_FILE_PATH_SYNTAX_WINDOWS
    #else
        #define D_CFG_FILE_PATH_SYNTAX D_CFG_FILE_PATH_SYNTAX_POSIX
    #endif
#endif  // D_CFG_FILE_PATH_SYNTAX

#if !D_CFG_IS_INT_LITERAL(D_CFG_FILE_PATH_SYNTAX)
    #error "D_CFG_FILE_PATH_SYNTAX must name one of its values; a misspelled name would read as 0"
#endif

// 1.1.4
// D_CFG_FILE_PATH_ACCEPT_ALT_SEP
//   knob: under WINDOWS syntax, accept '/' as a separator as well as '\\'.
//   On by default, because Win32 itself accepts both and every makefile,
// config file and URL a program is handed uses '/'. Meaningless under POSIX
// syntax, where '\\' is a filename byte -- accepting it as a separator would
// corrupt legitimate names -- so it is ignored there rather than obeyed.
#ifndef D_CFG_FILE_PATH_ACCEPT_ALT_SEP
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_PATH_ACCEPT_ALT_SEP D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_PATH_ACCEPT_ALT_SEP 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_PATH_ACCEPT_ALT_SEP

// 1.1.5
// D_CFG_FILE_PATH_OUT_SEP
//   knob: the character this module EMITS when it builds a path (value
// model). Defaults to the separator of the selected syntax -- not of the host
// -- so a Linux build reading and writing Windows paths round-trips them
// instead of quietly converting them to '/'.
//   Set it to '/' explicitly on Windows if you want output that is
// byte-identical across platforms: Win32 accepts '/' everywhere, and paths
// that end up diffed, hashed or committed are better off stable.
#ifndef D_CFG_FILE_PATH_OUT_SEP
    #if (D_CFG_NORM(D_CFG_FILE_PATH_SYNTAX) == D_CFG_FILE_PATH_SYNTAX_WINDOWS)
        #define D_CFG_FILE_PATH_OUT_SEP '\\'
    #else
        #define D_CFG_FILE_PATH_OUT_SEP '/'
    #endif
#endif  // D_CFG_FILE_PATH_OUT_SEP

// 1.2    Normalization
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_FILE_PATH_NORMALIZE_DOTDOT
//   knob: let d_path_normalize collapse "a/b/../c" to "a/c".
//   On by default, with a caveat that is the whole reason this is a knob:
// collapsing ".." is only equivalent to what the OS does when no component is a
// symlink. "/x/link/.." resolves to "/x" lexically, but to the parent of link's
// TARGET on disk. If that distinction matters, do not normalize -- call
// d_path_resolve (file_dir), which asks the filesystem instead of guessing.
#ifndef D_CFG_FILE_PATH_NORMALIZE_DOTDOT
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_PATH_NORMALIZE_DOTDOT D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_PATH_NORMALIZE_DOTDOT 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_PATH_NORMALIZE_DOTDOT

// 1.2.2
// D_CFG_FILE_PATH_COLLAPSE_SEPARATORS
//   knob: collapse runs of separators -- "a//b" becomes "a/b". On by
// default; POSIX says a leading "//" is implementation-defined, and this
// module preserves exactly two leading slashes for that reason.
#ifndef D_CFG_FILE_PATH_COLLAPSE_SEPARATORS
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_PATH_COLLAPSE_SEPARATORS D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_PATH_COLLAPSE_SEPARATORS 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_PATH_COLLAPSE_SEPARATORS

// 1.2.3
// D_CFG_FILE_PATH_STRIP_TRAILING_SEP
//   knob: drop a trailing separator -- "a/b/" becomes "a/b". On by default.
// A root is never stripped: "/" stays "/", and "C:\\" stays "C:\\", because
// the separator there is the path rather than punctuation on it.
#ifndef D_CFG_FILE_PATH_STRIP_TRAILING_SEP
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_PATH_STRIP_TRAILING_SEP D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_PATH_STRIP_TRAILING_SEP 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_PATH_STRIP_TRAILING_SEP

// 1.3    Join
//------------------------------------------------------------------------------
// 1.3.1
// D_CFG_FILE_PATH_JOIN_ABSOLUTE_WINS
//   knob: when the second component of a join is absolute, discard the
// first -- join("/a", "/b") is "/b", not "/a/b".
//   On by default: it is what every path library does (POSIX shells,
// Python's os.path.join, std::filesystem's operator/=), and the alternative
// silently produces "/a/b" for code that meant to override a base directory
// with an absolute one. Set 0 for strict textual concatenation.
#ifndef D_CFG_FILE_PATH_JOIN_ABSOLUTE_WINS
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_PATH_JOIN_ABSOLUTE_WINS D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_PATH_JOIN_ABSOLUTE_WINS 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_PATH_JOIN_ABSOLUTE_WINS


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
#if !D_CFG_IS_BOOL(D_CFG_FILE_PATH_ACCEPT_ALT_SEP)
    #error "D_CFG_FILE_PATH_ACCEPT_ALT_SEP must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_PATH_NORMALIZE_DOTDOT)
    #error "D_CFG_FILE_PATH_NORMALIZE_DOTDOT must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_PATH_COLLAPSE_SEPARATORS)
    #error "D_CFG_FILE_PATH_COLLAPSE_SEPARATORS must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_PATH_STRIP_TRAILING_SEP)
    #error "D_CFG_FILE_PATH_STRIP_TRAILING_SEP must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_PATH_JOIN_ABSOLUTE_WINS)
    #error "D_CFG_FILE_PATH_JOIN_ABSOLUTE_WINS must be literally 0 or 1"
#endif

// selection knob: must name a syntax that exists
#if ( (D_CFG_NORM(D_CFG_FILE_PATH_SYNTAX) < D_CFG_FILE_PATH_SYNTAX_POSIX) ||   \
      (D_CFG_NORM(D_CFG_FILE_PATH_SYNTAX) > D_CFG_FILE_PATH_SYNTAX_WINDOWS) )
    #error "D_CFG_FILE_PATH_SYNTAX must be one of D_CFG_FILE_PATH_SYNTAX_*"
#endif

// a Windows target that parses POSIX syntax cannot decide whether "C:\\x" is
// absolute, and every join and normalize downstream inherits the wrong answer.
// Reading foreign syntax is a legitimate request; being unable to read your
// OWN is not.
#if ( (D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)) &&                                   \
      (D_CFG_NORM(D_CFG_FILE_PATH_SYNTAX) != D_CFG_FILE_PATH_SYNTAX_WINDOWS) )
    #error "D_CFG_FILE_PATH_SYNTAX must be WINDOWS on a Windows target"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================


// 3.1    Effective values
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_FILE_PATH_WINDOWS
//   resolved: 1 when this build parses Windows path grammar. Every other
// derivation below hangs off this one, so there is exactly one place that
// decides it.
#if (D_CFG_NORM(D_CFG_FILE_PATH_SYNTAX) == D_CFG_FILE_PATH_SYNTAX_WINDOWS)
    #define D_INTERNAL_FILE_PATH_WINDOWS 1
#else
    #define D_INTERNAL_FILE_PATH_WINDOWS 0
#endif

// 3.1.2
// D_INTERNAL_FILE_PATH_ALT_SEP
//   resolved: 1 when a second separator character is recognised on input. Only
// meaningful under Windows syntax; forced 0 under POSIX syntax, where '\\'
// is a legal filename byte.
#if ( (D_INTERNAL_FILE_PATH_WINDOWS == 1) &&                                   \
      (D_CFG_IS_ON(D_CFG_FILE_PATH_ACCEPT_ALT_SEP)) )
    #define D_INTERNAL_FILE_PATH_ALT_SEP 1
#else
    #define D_INTERNAL_FILE_PATH_ALT_SEP 0
#endif

// 3.1.3
// D_INTERNAL_FILE_PATH_HAS_DRIVE
//   resolved: 1 when "C:" is parsed as a root. A consequence of the syntax, not
// a knob of its own.
#define D_INTERNAL_FILE_PATH_HAS_DRIVE  D_INTERNAL_FILE_PATH_WINDOWS

// 3.1.4
// D_INTERNAL_FILE_PATH_HAS_UNC
//   resolved: 1 when "\\\\server\\share" is parsed as a root. Likewise.
#define D_INTERNAL_FILE_PATH_HAS_UNC    D_INTERNAL_FILE_PATH_WINDOWS

// 3.1.5
// D_INTERNAL_FILE_PATH_OUT_SEP
//   resolved: the separator character emitted when constructing a path.
#define D_INTERNAL_FILE_PATH_OUT_SEP    D_CFG_FILE_PATH_OUT_SEP


//==============================================================================
// 4.  QUERIES
//==============================================================================


// 4.1    Public query macros
//------------------------------------------------------------------------------
// 4.1.1
// D_FILE_PATH_UNDERSTANDS_WINDOWS
//   query: 1 when this build parses Windows path syntax (drive letters, UNC),
// whatever platform it is running on. Safe in #if; the test suite uses it to
// decide which syntax to assert.
#define D_FILE_PATH_UNDERSTANDS_WINDOWS D_INTERNAL_FILE_PATH_WINDOWS

// 4.1.2
// D_FILE_PATH_OUT_SEP / D_FILE_PATH_OUT_SEP_STR
//   constant: the separator this build EMITS, in char and string form.
//   Distinct from D_FILE_PATH_SEP, which is the PLATFORM's separator and is
// what file_temp and file_dir want because they build paths for the running
// kernel to open. This one follows the GRAMMAR, which is what d_path_join and
// d_path_normalize write. The two agree on a native build and disagree the
// moment D_CFG_FILE_PATH_SYNTAX selects the other grammar -- so a caller
// predicting this module's output needs this one, and until now the only
// published answer was the other question's.
#if (D_INTERNAL_FILE_PATH_WINDOWS == 1)
    #define D_FILE_PATH_OUT_SEP          '\\'
    #define D_FILE_PATH_OUT_SEP_STR      "\\"
#else
    #define D_FILE_PATH_OUT_SEP          '/'
    #define D_FILE_PATH_OUT_SEP_STR      "/"
#endif


#endif  // DJINTERP_CONFIG_C_FS_CFG_FILE_PATH_H
