/*******************************************************************************
* djinterp [config]                                                cfg_file_io.h
*
* Build-time configuration for c/fs/file_io.h -- reading and writing bytes,
* whether through a descriptor, at an offset, or a whole file at a time.
*
*   Read and write share a module because they share their machinery: the same
* chunking rule, the same whole-file open/close scaffolding, the same
* short-transfer retry. They keep separate knobs because they are not the same
* tuning problem.
*
*   Two knobs here decide what your files look like after a crash --
* D_CFG_FILE_WRITE_ATOMIC and D_CFG_FILE_WRITE_SYNC. Neither is free; read
* their notes before flipping them.
*
*   targets:  c/fs/file_io.h, c/fs/file_io.c -> D_INTERNAL_FILE_*
*   requires: cfg_file_common.h
*
*
* path:      /inc/djinterp/config/c/fs/cfg_file_io.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.07.15
*                                                            revised: 2026.09.28
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  KNOBS
    -----
    1.  Read: transfer sizing
         1.  D_CFG_FILE_READ_CHUNK_SIZE
         2.  D_CFG_FILE_READ_MAX_SIZE
    2.  Read: whole-File reads
         1.  D_CFG_FILE_READ_NUL_TERMINATE
         2.  D_CFG_FILE_READ_GROW_UNSIZED
         3.  D_CFG_FILE_READ_GROW_INITIAL
         4.  D_CFG_FILE_READ_SHRINK_TO_FIT
    3.  Read: platform fast paths
         1.  D_CFG_FILE_READ_USE_PREAD
         2.  D_CFG_FILE_READ_SEQUENTIAL_HINT
    4.  Write: transfer sizing
         1.  D_CFG_FILE_WRITE_CHUNK_SIZE
    5.  Write: durability
         1.  D_CFG_FILE_WRITE_ATOMIC
         2.  D_CFG_FILE_WRITE_SYNC
    6.  Write: creation
         1.  D_CFG_FILE_WRITE_CREATE_MODE
         2.  D_CFG_FILE_WRITE_PREALLOCATE
    7.  Write: platform fast paths
         1.  D_CFG_FILE_WRITE_USE_PWRITE
2.  VALIDATION
    ----------
    1.  Knob validation
3.  RESOLVED VALUES
    ---------------
    1.  Effective values
         1.  D_INTERNAL_FILE_READ_CHUNK_SIZE
         2.  D_INTERNAL_FILE_READ_MAX_SIZE
         3.  D_INTERNAL_FILE_READ_GROW_INITIAL
         4.  D_INTERNAL_FILE_READ_HAS_PREAD
         5.  D_INTERNAL_FILE_READ_HINT
         6.  D_INTERNAL_FILE_READ_NUL_EXTRA
         7.  D_INTERNAL_FILE_WRITE_CHUNK_SIZE
         8.  D_INTERNAL_FILE_WRITE_MODE
         9.  D_INTERNAL_FILE_WRITE_HAS_PWRITE
         10. D_INTERNAL_FILE_WRITE_ATOMIC
         11. D_INTERNAL_FILE_WRITE_SYNC
         12. D_INTERNAL_FILE_WRITE_PREALLOC
         13. D_INTERNAL_FILE_WRITE_TEMP_SUFFIX
4.  QUERIES
    -------
    1.  Public query macros
         1.  D_FILE_READ_PREAD_IS_ATOMIC
         2.  D_FILE_WRITE_IS_ATOMIC
         3.  D_FILE_WRITE_IS_DURABLE
*/

#ifndef DJINTERP_CONFIG_C_FS_CFG_FILE_IO_H
#define DJINTERP_CONFIG_C_FS_CFG_FILE_IO_H 1

// djinterp
#include "cfg_file_common.h"  // shared fs knobs, D_CFG_IS_BOOL


//==============================================================================
// 1.  KNOBS
//==============================================================================


// 1.1    Read: transfer sizing
//------------------------------------------------------------------------------
// 1.1.1
// D_CFG_FILE_READ_CHUNK_SIZE
//   knob: the largest number of bytes handed to the platform in one call
// (value model). This is not only a tuning knob: Windows' _read takes an
// unsigned int, and a POSIX read is only required to handle SSIZE_MAX, so a
// size_t-sized request has to be broken up somewhere regardless. Doing it
// here means a caller may pass any size and get the loop for free.
//   Default 1 MiB: large enough that the per-call overhead vanishes, small
// enough to stay friendly to the page cache and to a 32-bit address space.
#ifndef D_CFG_FILE_READ_CHUNK_SIZE
    #define D_CFG_FILE_READ_CHUNK_SIZE (1024u * 1024u)
#endif  // D_CFG_FILE_READ_CHUNK_SIZE

// 1.1.2
// D_CFG_FILE_READ_MAX_SIZE
//   knob: refuse a whole-file read larger than this many bytes (value
// model). 0 means "no ceiling of its own" -- the D_CFG_FILE_MAX_ALLOC
// ceiling still applies, since the buffer has to be allocated. Set it when
// the read path should be tighter than the subframework's general limit,
// e.g. a config loader that should never see a file over a few MiB.
#ifndef D_CFG_FILE_READ_MAX_SIZE
    #define D_CFG_FILE_READ_MAX_SIZE 0
#endif  // D_CFG_FILE_READ_MAX_SIZE

// 1.2    Read: whole-File reads
//------------------------------------------------------------------------------
// 1.2.1
// D_CFG_FILE_READ_NUL_TERMINATE
//   knob: allocate one extra byte and NUL-terminate the buffer d_file_read_all
// returns, so the result may be handed straight to a string function. The
// terminator is not counted in the reported size. On by default: it is one
// byte, and the alternative is that every text caller reallocates to add it.
// Set 0 for a strictly binary program that wants the allocation to be exactly
// the file's length.
#ifndef D_CFG_FILE_READ_NUL_TERMINATE
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_READ_NUL_TERMINATE D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_READ_NUL_TERMINATE 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_READ_NUL_TERMINATE

// 1.2.2
// D_CFG_FILE_READ_GROW_UNSIZED
//   knob: let d_file_read_all handle sources whose length cannot be known in
// advance -- pipes, character devices, and every file under /proc and /sys,
// all of which stat as zero bytes and then produce data anyway -- by growing
// the buffer as it reads instead of trusting the reported size.
//   On by default: without it, reading /proc/self/status silently returns an
// empty buffer, which is the kind of bug that survives review. Set 0 for a
// program that only ever reads regular files and wants exactly one
// allocation per read.
#ifndef D_CFG_FILE_READ_GROW_UNSIZED
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_READ_GROW_UNSIZED D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_READ_GROW_UNSIZED 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_READ_GROW_UNSIZED

// 1.2.3
// D_CFG_FILE_READ_GROW_INITIAL
//   knob: first allocation for an unsized read, in bytes (value model).
// Doubles from there. Default 8 KiB: two pages, which covers essentially
// every /proc entry in one pass.
#ifndef D_CFG_FILE_READ_GROW_INITIAL
    #define D_CFG_FILE_READ_GROW_INITIAL (8u * 1024u)
#endif  // D_CFG_FILE_READ_GROW_INITIAL

// 1.2.4
// D_CFG_FILE_READ_SHRINK_TO_FIT
//   knob: after an unsized read, realloc the buffer down to the bytes
// actually read, so the caller is not handed a buffer with up to half its
// capacity wasted. On by default; the shrink is allowed to fail, in which
// case the oversized block is returned as-is rather than the read failing.
#ifndef D_CFG_FILE_READ_SHRINK_TO_FIT
    #ifdef D_CFG_FILE_ALL
        #define D_CFG_FILE_READ_SHRINK_TO_FIT D_CFG_FILE_ALL
    #else
        #define D_CFG_FILE_READ_SHRINK_TO_FIT 1
    #endif  // D_CFG_FILE_ALL
#endif  // D_CFG_FILE_READ_SHRINK_TO_FIT

// 1.3    Read: platform fast paths
//------------------------------------------------------------------------------
// 1.3.1
// D_CFG_FILE_READ_USE_PREAD
//   knob: implement d_file_pread_fd with the platform's positional read
// (pread / OVERLAPPED ReadFile) rather than seek-read-seek. Follows
// D_CFG_FILE_OPTIMIZE. This is a correctness knob as much as a speed one:
// the emulation is not atomic, so two threads sharing a descriptor can
// interleave and read each other's offsets.
#ifndef D_CFG_FILE_READ_USE_PREAD
    #define D_CFG_FILE_READ_USE_PREAD D_CFG_FILE_OPTIMIZE
#endif  // D_CFG_FILE_READ_USE_PREAD

// 1.3.2
// D_CFG_FILE_READ_SEQUENTIAL_HINT
//   knob: tell the kernel a whole-file read is sequential
// (POSIX_FADV_SEQUENTIAL), so it reads ahead more aggressively and drops the
// pages afterwards instead of evicting the caller's working set. Follows
// D_CFG_FILE_FADVISE; a no-op where the platform has no equivalent.
#ifndef D_CFG_FILE_READ_SEQUENTIAL_HINT
    #define D_CFG_FILE_READ_SEQUENTIAL_HINT D_CFG_FILE_FADVISE
#endif  // D_CFG_FILE_READ_SEQUENTIAL_HINT

// 1.4    Write: transfer sizing
//------------------------------------------------------------------------------
// 1.4.1
// D_CFG_FILE_WRITE_CHUNK_SIZE
//   knob: as D_CFG_FILE_READ_CHUNK_SIZE, for the write direction. Separate
// because the two are not the same tuning problem: a reader is usually
// bounded by readahead, a writer by the page cache and the device's queue.
// Default 1 MiB.
#ifndef D_CFG_FILE_WRITE_CHUNK_SIZE
    #define D_CFG_FILE_WRITE_CHUNK_SIZE (1024u * 1024u)
#endif  // D_CFG_FILE_WRITE_CHUNK_SIZE

// 1.5    Write: durability
//------------------------------------------------------------------------------
// 1.5.1
// D_CFG_FILE_WRITE_ATOMIC
//   knob: make d_file_write_all replace a file rather than rewrite it -- write
// to a sibling temporary, then rename over the target, so a reader either
// sees the whole old file or the whole new one and never a half-written one.
//   Off by default, because it is not a free upgrade:
//     - it needs write permission on the *directory*, not just the file;
//     - it breaks hard links and can reset ownership, ACLs and SELinux
//       labels, since the file is a new inode;
//     - it fails across filesystems if your temp is not a sibling (it is);
//     - rename is atomic in the directory, but a crash can still lose the
//       rename itself unless the directory is synced -- which is what
//       D_CFG_FILE_WRITE_SYNC adds.
//   Turn it on for config files, save files, and anything a second process
// may read while you write it. Leave it off for logs and scratch output.
#ifndef D_CFG_FILE_WRITE_ATOMIC
    #define D_CFG_FILE_WRITE_ATOMIC 0
#endif  // D_CFG_FILE_WRITE_ATOMIC

// 1.5.2
// D_CFG_FILE_WRITE_SYNC
//   knob: force written data to durable storage before d_file_write_all
// returns, instead of leaving it in the page cache.
//   Off by default because the cost is not marginal -- it is a device round
// trip, and it can be milliseconds on spinning media or a slow SSD, per
// call. Without it "the write succeeded" means the kernel accepted the
// bytes, not that they survive a power cut.
//   With D_CFG_FILE_WRITE_ATOMIC also on, the temporary is synced before the
// rename, which is the ordering that actually gives you the all-or-nothing
// guarantee people assume atomic replace provides on its own.
#ifndef D_CFG_FILE_WRITE_SYNC
    #define D_CFG_FILE_WRITE_SYNC 0
#endif  // D_CFG_FILE_WRITE_SYNC

// 1.6    Write: creation
//------------------------------------------------------------------------------
// 1.6.1
// D_CFG_FILE_WRITE_CREATE_MODE
//   knob: permission bits applied to a file this module creates (value
// model). Default 0644 (owner writes, everyone reads), which is what the
// shell's ">" produces and therefore what people expect. The process umask
// still applies on POSIX and can only take bits away.
//   Use 0600 for anything holding a secret: umask cannot be relied on to
// remove the group and other bits, because it is the user's setting, not
// yours.
#ifndef D_CFG_FILE_WRITE_CREATE_MODE
    #define D_CFG_FILE_WRITE_CREATE_MODE 0644
#endif  // D_CFG_FILE_WRITE_CREATE_MODE

// 1.6.2
// D_CFG_FILE_WRITE_PREALLOCATE
//   knob: reserve the file's extents up front when the size is known
// (posix_fallocate), instead of letting it grow one write at a time. Trades
// a syscall for less fragmentation and an early, honest ENOSPC rather than a
// half-written file. Follows D_CFG_FILE_OPTIMIZE; no-op where unsupported.
#ifndef D_CFG_FILE_WRITE_PREALLOCATE
    #define D_CFG_FILE_WRITE_PREALLOCATE D_CFG_FILE_OPTIMIZE
#endif  // D_CFG_FILE_WRITE_PREALLOCATE

// 1.7    Write: platform fast paths
//------------------------------------------------------------------------------
// 1.7.1
// D_CFG_FILE_WRITE_USE_PWRITE
//   knob: implement d_file_pwrite_fd with the platform's positional write
// rather than seek-write-seek. Follows D_CFG_FILE_OPTIMIZE. As with
// d_file_pread_fd this is a correctness knob: the emulation is not atomic
// against another thread using the same descriptor.
#ifndef D_CFG_FILE_WRITE_USE_PWRITE
    #define D_CFG_FILE_WRITE_USE_PWRITE D_CFG_FILE_OPTIMIZE
#endif  // D_CFG_FILE_WRITE_USE_PWRITE


//==============================================================================
// 2.  VALIDATION
//==============================================================================


// 2.1    Knob validation
//------------------------------------------------------------------------------
#if !D_CFG_IS_BOOL(D_CFG_FILE_READ_NUL_TERMINATE)
    #error "D_CFG_FILE_READ_NUL_TERMINATE must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_READ_GROW_UNSIZED)
    #error "D_CFG_FILE_READ_GROW_UNSIZED must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_READ_SHRINK_TO_FIT)
    #error "D_CFG_FILE_READ_SHRINK_TO_FIT must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_READ_USE_PREAD)
    #error "D_CFG_FILE_READ_USE_PREAD must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_READ_SEQUENTIAL_HINT)
    #error "D_CFG_FILE_READ_SEQUENTIAL_HINT must be literally 0 or 1"
#endif

// value knobs: a zero chunk is an infinite loop, and a zero initial grow is
// a buffer that never grows. Catch both here rather than at 3am.
#if (D_CFG_NORM(D_CFG_FILE_READ_CHUNK_SIZE) <= 0)
    #error "D_CFG_FILE_READ_CHUNK_SIZE must be greater than 0"
#endif
#if ( (D_CFG_IS_ON(D_CFG_FILE_READ_GROW_UNSIZED)) &&                           \
      (D_CFG_NORM(D_CFG_FILE_READ_GROW_INITIAL) <= 0) )
    #error "D_CFG_FILE_READ_GROW_INITIAL must be greater than 0"
#endif
#if ( (D_CFG_NORM(D_CFG_FILE_READ_MAX_SIZE) > 0) &&                            \
      (D_CFG_NORM(D_CFG_FILE_MAX_ALLOC) > 0) &&                                \
      (D_CFG_NORM(D_CFG_FILE_READ_MAX_SIZE) >                                  \
       D_CFG_NORM(D_CFG_FILE_MAX_ALLOC)) )
    #error "D_CFG_FILE_READ_MAX_SIZE exceeds D_CFG_FILE_MAX_ALLOC"
#endif

#if !D_CFG_IS_BOOL(D_CFG_FILE_WRITE_ATOMIC)
    #error "D_CFG_FILE_WRITE_ATOMIC must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_WRITE_SYNC)
    #error "D_CFG_FILE_WRITE_SYNC must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_WRITE_PREALLOCATE)
    #error "D_CFG_FILE_WRITE_PREALLOCATE must be literally 0 or 1"
#endif
#if !D_CFG_IS_BOOL(D_CFG_FILE_WRITE_USE_PWRITE)
    #error "D_CFG_FILE_WRITE_USE_PWRITE must be literally 0 or 1"
#endif
#if (D_CFG_NORM(D_CFG_FILE_WRITE_CHUNK_SIZE) <= 0)
    #error "D_CFG_FILE_WRITE_CHUNK_SIZE must be greater than 0"
#endif

// atomic replace needs a rename, which the ISO C backend does have -- but it
// also needs to build a sibling temporary name, and a target without a
// filesystem namespace to build it in cannot honour the request. Say so
// rather than quietly degrading to a plain rewrite, which is the exact
// failure the knob was set to prevent.
#if ( (D_CFG_IS_ON(D_CFG_FILE_WRITE_ATOMIC)) &&                                \
      (D_CFG_IS_OFF(D_CFG_FILE_HAS_POSIX)) &&                                  \
      (D_CFG_IS_OFF(D_CFG_FILE_HAS_WIN32)) )
    #error "D_CFG_FILE_WRITE_ATOMIC is unsupported on this target"
#endif


//==============================================================================
// 3.  RESOLVED VALUES
//==============================================================================


// 3.1    Effective values
//------------------------------------------------------------------------------
//   CONVENTION: every D_INTERNAL_* value below is a bare integer constant
// expression with no cast, so it stays legal inside #if. The modules apply
// the (size_t) themselves at the point of use. A cast here would compile in
// C and then fail the moment a module tested the same symbol with #if.

// 3.1.1
// D_INTERNAL_FILE_READ_CHUNK_SIZE
//   resolved: the per-call transfer ceiling the read loop uses.
#define D_INTERNAL_FILE_READ_CHUNK_SIZE D_CFG_FILE_READ_CHUNK_SIZE

// 3.1.2
// D_INTERNAL_FILE_READ_MAX_SIZE
//   resolved: the effective whole-file ceiling in bytes, or 0 for none.
// Resolves the module knob against the subframework's allocation ceiling by
// taking whichever is tighter, so a module knob can only ever narrow the limit.
#if (D_CFG_NORM(D_CFG_FILE_READ_MAX_SIZE) > 0)
    #define D_INTERNAL_FILE_READ_MAX_SIZE D_CFG_FILE_READ_MAX_SIZE
#elif (D_CFG_NORM(D_CFG_FILE_MAX_ALLOC) > 0)
    #define D_INTERNAL_FILE_READ_MAX_SIZE D_CFG_FILE_MAX_ALLOC
#else
    #define D_INTERNAL_FILE_READ_MAX_SIZE 0
#endif

// 3.1.3
// D_INTERNAL_FILE_READ_GROW_INITIAL
//   resolved: first allocation for an unsized read.
#define D_INTERNAL_FILE_READ_GROW_INITIAL D_CFG_FILE_READ_GROW_INITIAL

// 3.1.4
// D_INTERNAL_FILE_READ_HAS_PREAD
//   resolved: 1 when d_file_pread_fd lowers to a real positional read; 0 when
// it must be emulated with seek-read-seek and therefore is not atomic.
#if ( (D_CFG_IS_ON(D_CFG_FILE_READ_USE_PREAD)) &&                              \
      (D_INTERNAL_FILE_HAS_PREAD == 1) )
    #define D_INTERNAL_FILE_READ_HAS_PREAD 1
#else
    #define D_INTERNAL_FILE_READ_HAS_PREAD 0
#endif

// 3.1.5
// D_INTERNAL_FILE_READ_HINT
//   resolved: 1 when the whole-file path issues a sequential-access hint.
#if ( (D_CFG_IS_ON(D_CFG_FILE_READ_SEQUENTIAL_HINT)) &&                        \
      (D_INTERNAL_FILE_HAS_FADVISE == 1) )
    #define D_INTERNAL_FILE_READ_HINT 1
#else
    #define D_INTERNAL_FILE_READ_HINT 0
#endif

// 3.1.6
// D_INTERNAL_FILE_READ_NUL_EXTRA
//   resolved: extra bytes the whole-file path allocates for the terminator --
// 1 or 0. Used directly in the size arithmetic so the two code paths differ
// by a constant instead of by an #if.
#if D_CFG_IS_ON(D_CFG_FILE_READ_NUL_TERMINATE)
    #define D_INTERNAL_FILE_READ_NUL_EXTRA 1
#else
    #define D_INTERNAL_FILE_READ_NUL_EXTRA 0
#endif

//   CONVENTION: as in cfg_file_read.h, every D_INTERNAL_* value below is a
// bare integer constant expression -- no cast -- so it remains legal inside
// #if. Modules cast at the point of use.

// 3.1.7
// D_INTERNAL_FILE_WRITE_CHUNK_SIZE
//   resolved: the per-call transfer ceiling the write loop uses.
#define D_INTERNAL_FILE_WRITE_CHUNK_SIZE D_CFG_FILE_WRITE_CHUNK_SIZE

// 3.1.8
// D_INTERNAL_FILE_WRITE_MODE
//   resolved: the creation permission bits, as a value the platform's open will
// accept.
#define D_INTERNAL_FILE_WRITE_MODE D_CFG_FILE_WRITE_CREATE_MODE

// 3.1.9
// D_INTERNAL_FILE_WRITE_HAS_PWRITE
//   resolved: 1 when d_file_pwrite_fd lowers to a real positional write; 0 when
// it is emulated and therefore not atomic.
#if ( (D_CFG_IS_ON(D_CFG_FILE_WRITE_USE_PWRITE)) &&                            \
      (D_INTERNAL_FILE_HAS_PREAD == 1) )
    #define D_INTERNAL_FILE_WRITE_HAS_PWRITE 1
#else
    #define D_INTERNAL_FILE_WRITE_HAS_PWRITE 0
#endif

// 3.1.10
// D_INTERNAL_FILE_WRITE_ATOMIC
//   resolved: 1 when d_file_write_all replaces via a temporary and a rename.
#if D_CFG_IS_ON(D_CFG_FILE_WRITE_ATOMIC)
    #define D_INTERNAL_FILE_WRITE_ATOMIC 1
#else
    #define D_INTERNAL_FILE_WRITE_ATOMIC 0
#endif

// 3.1.11
// D_INTERNAL_FILE_WRITE_SYNC
//   resolved: 1 when the whole-file path forces data to durable storage.
// Requires descriptors, so the ISO C backend cannot honour it -- fflush
// pushes to the kernel, which is not the same promise and must not be
// mistaken for it.
#if ( (D_CFG_IS_ON(D_CFG_FILE_WRITE_SYNC)) &&                                  \
      (D_INTERNAL_FILE_BACKEND != D_CFG_FILE_BACKEND_STDC) )
    #define D_INTERNAL_FILE_WRITE_SYNC 1
#else
    #define D_INTERNAL_FILE_WRITE_SYNC 0
#endif

// 3.1.12
// D_INTERNAL_FILE_WRITE_PREALLOC
//   resolved: 1 when a known-size write reserves its extents first.
#if ( (D_CFG_IS_ON(D_CFG_FILE_WRITE_PREALLOCATE)) &&                           \
      (D_CFG_IS_ON(D_CFG_FILE_HAS_FALLOCATE)) )
    #define D_INTERNAL_FILE_WRITE_PREALLOC 1
#else
    #define D_INTERNAL_FILE_WRITE_PREALLOC 0
#endif

// 3.1.13
// D_INTERNAL_FILE_WRITE_TEMP_SUFFIX
//   resolved: suffix appended to the target path to name the temporary used by
// the atomic path. A sibling, so the rename never crosses a filesystem.
#ifndef D_INTERNAL_FILE_WRITE_TEMP_SUFFIX
    #define D_INTERNAL_FILE_WRITE_TEMP_SUFFIX ".djtmp"
#endif  // D_INTERNAL_FILE_WRITE_TEMP_SUFFIX


//==============================================================================
// 4.  QUERIES
//==============================================================================


// 4.1    Public query macros
//------------------------------------------------------------------------------
// 4.1.1
// D_FILE_READ_PREAD_IS_ATOMIC
//   query: 1 when d_file_pread_fd cannot interleave with another thread's use
// of the same descriptor, else 0. Safe in #if.
#define D_FILE_READ_PREAD_IS_ATOMIC D_INTERNAL_FILE_READ_HAS_PREAD

// 4.1.2
// D_FILE_WRITE_IS_ATOMIC
//   query: 1 when d_file_write_all replaces rather than rewrites, else 0.
#define D_FILE_WRITE_IS_ATOMIC   D_INTERNAL_FILE_WRITE_ATOMIC

// 4.1.3
// D_FILE_WRITE_IS_DURABLE
//   query: 1 when d_file_write_all returning 0 means the bytes are on the
// device, not merely accepted by the kernel.
#define D_FILE_WRITE_IS_DURABLE  D_INTERNAL_FILE_WRITE_SYNC


#endif  // DJINTERP_CONFIG_C_FS_CFG_FILE_IO_H
