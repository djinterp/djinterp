/*******************************************************************************
* djinterp [config]                                              cfg_file_sync.h
*
* Build-time configuration for c/filesys/file_sync.h -- forcing data to durable
* storage.
*
*   Every knob here trades speed for a promise about what survives a power
* cut. None of them is free, and the defaults are chosen so that a call that
* claims durability delivers it. They hold wherever c/filesys syncs: in
* d_file_sync_fd and d_file_sync_stream, and in file_io's durable writes
* (D_CFG_FILE_WRITE_SYNC).
*
*   targets:  c/filesys/file_sync.h, c/filesys/file_sync.c ->
*             D_INTERNAL_FILE_SYNC_*
*   requires: cfg_file_common.h
*
*
* path:      /inc/djinterp/config/c/filesys/cfg_file_sync.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.10.18
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Scope
         1.  D_CFG_FILE_SYNC_DATA_ONLY
    2.  Depth
         1.  D_CFG_FILE_SYNC_FULL
2.  VALIDATION
    ----------
    1.  Knob validation
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_FILE_SYNC_DATA_ONLY
         2.  D_INTERNAL_FILE_SYNC_APPLE
         3.  D_INTERNAL_FILE_SYNC_FULL
4.  QUERIES
    -------
    1.  Public query macros
         1.  D_FILE_SYNC_REACHES_PLATTER
*/

#ifndef DJINTERP_CONFIG_C_FILESYS_CFG_FILE_SYNC_H
#define DJINTERP_CONFIG_C_FILESYS_CFG_FILE_SYNC_H 1

// djinterp
#include "cfg_file_common.h"  // shared filesys knobs, D_CFG_IS_BOOL


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Scope
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_FILE_SYNC_DATA_ONLY
//   knob: use fdatasync(2) rather than fsync(2) -- flush the file's DATA but
// not the metadata that is not needed to read it back.
//   OFF by default, which is the conservative choice. fdatasync is genuinely
// faster (it can skip a second write to the inode for a timestamp nobody
// asked about), but "not needed to read it back" is a subtle promise: it
// still flushes a size change, because you cannot read data you cannot find,
// yet it may skip mtime. A program that syncs and then trusts mtime to detect
// its own writes will be wrong, rarely, after a crash.
//   Turn it on for a log or a database journal, where the data is the point
// and the timestamp is not. Linux only: elsewhere fsync is used whatever this
// says. A directory gets fsync too, since fdatasync need not wait for a new
// entry -- the thing a directory is synced for -- to reach the device.
#ifndef D_CFG_FILE_SYNC_DATA_ONLY
    #define D_CFG_FILE_SYNC_DATA_ONLY 0
#endif  // D_CFG_FILE_SYNC_DATA_ONLY

// 1.2    Depth
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_FILE_SYNC_FULL
//   knob: on Apple's systems -- macOS, and iOS with its relatives -- use
// fcntl(F_FULLFSYNC) instead of fsync(2).
//   ON, and this is not a tuning knob -- it is a correctness one. Apple's
// fsync() is documented to push data to the DRIVE, and then to stop: it does
// not ask the drive to flush its own write cache. So fsync() returns, the
// power fails, and the data is gone from a call whose entire purpose was to
// prevent that. F_FULLFSYNC is Apple's answer and is what SQLite and every
// serious database there use. A filesystem that refuses it -- most network
// ones do -- gets plain fsync, with a warning; an I/O error, or a full disk
// or quota, is reported instead, since fsync, asked afterwards, could report
// success for data the device has just lost.
//   It holds at any POSIX level the build asks for: Darwin hides the name
// F_FULLFSYNC from a build at one, but not the command.
//   It is slower, sometimes by an order of magnitude, because it is doing the
// thing you asked for. Set 0 only if you have decided that fsync-shaped
// performance matters more than the guarantee, and know that you have.
//   No effect on any other platform.
#ifndef D_CFG_FILE_SYNC_FULL
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_SYNC_FULL D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_SYNC_FULL 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_SYNC_FULL


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
#if !D_CFG_IS_BOOL(D_CFG_FILE_SYNC_DATA_ONLY)
    #error "D_CFG_FILE_SYNC_DATA_ONLY must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_SYNC_FULL)
    #error "D_CFG_FILE_SYNC_FULL must be literally 0 or 1"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================


// 3.1    Effective values
//------------------------------------------------------------------------------
// 3.1.1
// D_INTERNAL_FILE_SYNC_DATA_ONLY
//   resolved: 1 when d_file_sync_fd lowers to fdatasync: on Linux, with
// descriptors to sync.
#if ( (D_CFG_IS_ON(D_CFG_FILE_SYNC_DATA_ONLY)) &&                              \
      (D_INTERNAL_FILE_OS_LINUX == 1) &&                                       \
      (D_INTERNAL_FILE_BACKEND != D_CFG_FILE_BACKEND_STDC) )
    #define D_INTERNAL_FILE_SYNC_DATA_ONLY 1
#else
    #define D_INTERNAL_FILE_SYNC_DATA_ONLY 0
#endif

// 3.1.2
// D_INTERNAL_FILE_SYNC_APPLE
//   resolved: 1 on Apple's systems (D_INTERNAL_FILE_OS_DARWIN), whose fsync
// stops short of the drive's cache.
#if (D_INTERNAL_FILE_OS_DARWIN == 1)
    #define D_INTERNAL_FILE_SYNC_APPLE 1
#else
    #define D_INTERNAL_FILE_SYNC_APPLE 0
#endif

// 3.1.3
// D_INTERNAL_FILE_SYNC_FULL
//   resolved: 1 when d_file_sync_fd lowers to fcntl(F_FULLFSYNC): on Apple's
// systems, with descriptors to sync.
#if ( (D_CFG_IS_ON(D_CFG_FILE_SYNC_FULL)) &&                                   \
      (D_INTERNAL_FILE_SYNC_APPLE == 1) &&                                     \
      (D_INTERNAL_FILE_BACKEND != D_CFG_FILE_BACKEND_STDC) )
    #define D_INTERNAL_FILE_SYNC_FULL 1
#else
    #define D_INTERNAL_FILE_SYNC_FULL 0
#endif


//==============================================================================
// 4.  QUERIES
//==============================================================================


// 4.1    Public query macros
//------------------------------------------------------------------------------
// 4.1.1
// D_FILE_SYNC_REACHES_PLATTER
//   query: 1 when a successful d_file_sync_fd means the bytes are on the
// physical device; 0 when it only means the device has been handed them and may
// still be holding them in a volatile cache, and on the ISO C backend, which
// cannot sync at all. Safe in #if.
//   On Apple's systems this is D_CFG_FILE_SYNC_FULL -- for a filesystem that
// accepts F_FULLFSYNC; one that refuses it gets plain fsync, with a warning.
// Elsewhere the platform's own sync, fsync or FlushFileBuffers, already asks
// the device to empty its cache, so it is 1.
#if (D_INTERNAL_FILE_BACKEND == D_CFG_FILE_BACKEND_STDC)
    #define D_FILE_SYNC_REACHES_PLATTER 0
#elif (D_INTERNAL_FILE_SYNC_APPLE == 1)
    #define D_FILE_SYNC_REACHES_PLATTER D_INTERNAL_FILE_SYNC_FULL
#else
    #define D_FILE_SYNC_REACHES_PLATTER 1
#endif


#endif  // DJINTERP_CONFIG_C_FILESYS_CFG_FILE_SYNC_H
