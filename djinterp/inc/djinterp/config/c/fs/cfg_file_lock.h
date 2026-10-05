/*******************************************************************************
* djinterp [config]                                              cfg_file_lock.h
*
* Build-time configuration for c/fs/file_lock.h -- advisory file locking.
*
*   ADVISORY. Every lock here is a convention between programs that agree to
* check. Nothing stops a process that does not ask from writing the file
* anyway. If you need a lock that cannot be ignored, this module cannot give
* you one and neither can POSIX.
*
*   targets:  c/fs/file_lock.h, c/fs/file_lock.c -> D_INTERNAL_FILE_LOCK_BACKEND
*   requires: cfg_file_common.h
*
*
* path:      /inc/djinterp/config/c/fs/cfg_file_lock.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.30
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Backend selection
         1.  D_CFG_FILE_LOCK_BACKEND_AUTO
         2.  D_CFG_FILE_LOCK_BACKEND_FLOCK
         3.  D_CFG_FILE_LOCK_BACKEND_FCNTL
         4.  D_CFG_FILE_LOCK_BACKEND
2.  VALIDATION
    ----------
    1.  Knob validation
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_FILE_LOCK_BACKEND
4.  QUERIES
    -------
    1.  Public query macros
         1.  D_FILE_LOCK_IS_PER_DESCRIPTION
*/

#ifndef DJINTERP_CONFIG_C_FS_CFG_FILE_LOCK_H
#define DJINTERP_CONFIG_C_FS_CFG_FILE_LOCK_H 1

// djinterp
#include "cfg_file_common.h"  // shared fs knobs, D_CFG_IS_BOOL


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Backend selection
//------------------------------------------------------------------------------
//   Selection model, and it decides SEMANTICS, not just which symbol gets
// called. The two POSIX locking APIs are not two spellings of one idea; they
// disagree about what a lock is attached to, and code written for one is
// silently wrong on the other.

// 1.1.1
// D_CFG_FILE_LOCK_BACKEND_AUTO
//   constant: pick whichever the platform offers, preferring flock.
#define D_CFG_FILE_LOCK_BACKEND_AUTO   0

// 1.1.2
// D_CFG_FILE_LOCK_BACKEND_FLOCK
//   constant: BSD flock(2). The lock belongs to the OPEN FILE DESCRIPTION --
// so it survives dup(), is shared by a fork()ed child, and is released only
// when the last descriptor referring to that open is closed.
//   Notably NOT per-process: two opens of the same file in one process take
// two independent locks and can deadlock against each other.
//   Historically unreliable over NFS, where it may be silently local-only --
// which is to say, no lock at all with respect to the other machine.
#define D_CFG_FILE_LOCK_BACKEND_FLOCK  1

// 1.1.3
// D_CFG_FILE_LOCK_BACKEND_FCNTL
//   constant: POSIX fcntl(F_SETLK). The lock belongs to the PROCESS, and it
// carries the worst footgun in POSIX: closing ANY descriptor to that file
// drops every lock the process holds on it. A library that opens the file to
// read a byte and closes it has just released your lock, from another
// function, with no diagnostic.
//   In exchange it works over NFS, and it is the only option where flock is
// absent (Solaris, some embedded libcs).
#define D_CFG_FILE_LOCK_BACKEND_FCNTL  2

// 1.1.4
// D_CFG_FILE_LOCK_BACKEND
//   knob: which locking semantics this build has. AUTO prefers flock, whose
// footguns are the less surprising pair.
//   Pin FCNTL if your locks must hold across NFS, and then never close a
// second descriptor to a locked file.
#ifndef D_CFG_FILE_LOCK_BACKEND
    #define D_CFG_FILE_LOCK_BACKEND D_CFG_FILE_LOCK_BACKEND_AUTO
#endif  // D_CFG_FILE_LOCK_BACKEND

#if !D_CFG_IS_INT_LITERAL(D_CFG_FILE_LOCK_BACKEND)
    #error "D_CFG_FILE_LOCK_BACKEND must name one of its values; a misspelled name would read as 0"
#endif


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
#if ( (D_CFG_NORM(D_CFG_FILE_LOCK_BACKEND) < D_CFG_FILE_LOCK_BACKEND_AUTO) ||  \
      (D_CFG_NORM(D_CFG_FILE_LOCK_BACKEND) > D_CFG_FILE_LOCK_BACKEND_FCNTL) )
    #error "D_CFG_FILE_LOCK_BACKEND must be one of D_CFG_FILE_LOCK_BACKEND_*"
#endif

#if ( (D_CFG_NORM(D_CFG_FILE_LOCK_BACKEND) ==                                  \
       D_CFG_FILE_LOCK_BACKEND_FLOCK) &&                                       \
      (D_CFG_IS_OFF(D_CFG_FILE_HAS_FLOCK)) )
    #error "D_CFG_FILE_LOCK_BACKEND == FLOCK but D_CFG_FILE_HAS_FLOCK is 0"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================


// 3.1    Effective values
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_FILE_LOCK_BACKEND
//   resolved: the resolved backend -- never AUTO.
#if (D_CFG_NORM(D_CFG_FILE_LOCK_BACKEND) != D_CFG_FILE_LOCK_BACKEND_AUTO)
    #define D_INTERNAL_FILE_LOCK_BACKEND D_CFG_NORM(D_CFG_FILE_LOCK_BACKEND)
#elif D_CFG_IS_ON(D_CFG_FILE_HAS_WIN32)
    #define D_INTERNAL_FILE_LOCK_BACKEND D_CFG_FILE_LOCK_BACKEND_FCNTL
#elif D_CFG_IS_ON(D_CFG_FILE_HAS_FLOCK)
    #define D_INTERNAL_FILE_LOCK_BACKEND D_CFG_FILE_LOCK_BACKEND_FLOCK
#else
    #define D_INTERNAL_FILE_LOCK_BACKEND D_CFG_FILE_LOCK_BACKEND_FCNTL
#endif


//==============================================================================
// 4.  QUERIES
//==============================================================================


// 4.1    Public query macros
//------------------------------------------------------------------------------
// 4.1.1
// D_FILE_LOCK_IS_PER_DESCRIPTION
//   query: 1 when a lock belongs to the open file description (flock), 0 when
// it belongs to the process (fcntl) and any close drops it. Safe in #if.
#define D_FILE_LOCK_IS_PER_DESCRIPTION                                         \
    (D_INTERNAL_FILE_LOCK_BACKEND == D_CFG_FILE_LOCK_BACKEND_FLOCK)


#endif  // DJINTERP_CONFIG_C_FS_CFG_FILE_LOCK_H
