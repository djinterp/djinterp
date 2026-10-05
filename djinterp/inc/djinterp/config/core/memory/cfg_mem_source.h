/*******************************************************************************
* djinterp [config]                                             cfg_mem_source.h
*
*   Configuration for the upstream byte-source protocol -- the layer every
* allocator in the memory subframework obtains its backing storage through.
* Selects which built-in sources are compiled, which platform API satisfies an
* over-aligned request, which source a caller gets when it asks for "the
* default", and which optional slots the vtable carries.
*
*   THE VTABLE IS A STRUCT, SO ITS SLOTS ARE BYTES. D_CFG_MEM_SOURCE_REALLOC
* and D_CFG_MEM_SOURCE_MAX_SIZE remove function pointers from
* d_mem_source_vtable, not merely calls to them. One vtable is shared by every
* instance of a source, so the saving is small in absolute terms -- but the
* protocol is the thing every other module is written against, and a narrower
* protocol is a narrower thing to implement when a caller supplies its own
* source. That, rather than the bytes, is why the slots are optional.
*
*   targets:  core/memory/mem_source.h  ->  D_INTERNAL_MEM_SOURCE_SYSTEM,
*             D_INTERNAL_MEM_SOURCE_BUFFER, D_INTERNAL_MEM_SOURCE_NULL,
*             D_INTERNAL_MEM_SOURCE_COUNTER, D_INTERNAL_MEM_SOURCE_REALLOC,
*             D_INTERNAL_MEM_SOURCE_MAX_SIZE, D_INTERNAL_MEM_SOURCE_NAMED,
*             D_INTERNAL_MEM_SOURCE_ALIGNED_API, D_INTERNAL_MEM_SOURCE_DEFAULT
*   requires: cfg_mem_common.h (vocabulary knobs, presets); cfg_common.h
*
*
* path:      /inc/djinterp/config/core/memory/cfg_mem_source.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.30
*******************************************************************************/

#ifndef DJINTERP_CONFIG_CORE_MEMORY_CFG_MEM_SOURCE_H
#define DJINTERP_CONFIG_CORE_MEMORY_CFG_MEM_SOURCE_H 1

// (0) subframework root first: it owns the aggregate, the presets, and the
//     vocabulary knobs whose values the defaults below read.
#include "./cfg_mem_common.h"


// ---------------------------------------------------------------------------
//  contents
// ---------------------------------------------------------------------------
//    1.  which built-in sources exist
//    2.  optional vtable slots
//    3.  over-aligned allocation API
//    4.  the default source
//    5.  validation
//    6.  derived values
// ---------------------------------------------------------------------------


// ===========================================================================
//  1.  WHICH BUILT-IN SOURCES EXIST
// ===========================================================================

// D_CFG_MEM_SOURCE_SYSTEM
//   brief: 1 compiles the malloc-backed source. Defaults ON, and off is a
// real configuration rather than a curiosity: a freestanding build has no
// malloc, and compiling a source that calls one it does not have is a link
// error at the end of a long build rather than a compile error at the top.
#ifndef D_CFG_MEM_SOURCE_SYSTEM
#   if D_CFG_IS_ON(D_CFG_MEM_PRESET_MINIMAL)
#       define D_CFG_MEM_SOURCE_SYSTEM 0
#   else
#       define D_CFG_MEM_SOURCE_SYSTEM 1
#   endif
#endif

// D_CFG_MEM_SOURCE_BUFFER
//   brief: 1 compiles the caller-supplied-buffer source, which vends from a
// block the caller already owns and never calls an upstream at all. This is
// the source that makes the whole subframework usable with no heap: an arena
// over a buffer source is a complete allocator whose every byte came from the
// caller's static array. Defaults ON in every profile, including MINIMAL.
#ifndef D_CFG_MEM_SOURCE_BUFFER
#   define D_CFG_MEM_SOURCE_BUFFER 1
#endif

// D_CFG_MEM_SOURCE_NULL
//   brief: 1 compiles the source that always refuses. Its value is negative
// proof: point an allocator at it and any path that reaches for memory fails
// visibly, so a "this subsystem allocates nothing after startup" claim becomes
// a test rather than a comment.
#ifndef D_CFG_MEM_SOURCE_NULL
#   define D_CFG_MEM_SOURCE_NULL 1
#endif

// D_CFG_MEM_SOURCE_COUNTER
//   brief: 1 compiles the counting decorator -- a source that forwards to
// another and accounts for what passes through. Follows D_CFG_MEM_STATS,
// since it reports through the same struct and a build with no accounting has
// nothing to report into.
#ifndef D_CFG_MEM_SOURCE_COUNTER
#   ifdef D_CFG_MEM_ALL
#       define D_CFG_MEM_SOURCE_COUNTER D_CFG_MEM_ALL
#   elif D_CFG_IS_ON(D_CFG_MEM_STATS)
#       define D_CFG_MEM_SOURCE_COUNTER 1
#   else
#       define D_CFG_MEM_SOURCE_COUNTER 0
#   endif
#endif


// ===========================================================================
//  2.  OPTIONAL VTABLE SLOTS
// ===========================================================================

// D_CFG_MEM_SOURCE_REALLOC
//   brief: 1 gives d_mem_source_vtable a reallocate slot. Defaults ON because
// the contiguous arena and the contiguous pool both grow by reallocation, and
// a source that can grow in place (system malloc's realloc can) saves a copy
// of everything already allocated.
//   Turning it OFF does not remove the capability -- d_mem_source_reallocate
// emulates it as allocate + copy + release -- so this trades a function
// pointer and an implementer's obligation for a copy. A caller writing its own
// source over a mapping that cannot grow in place should take that trade.
#ifndef D_CFG_MEM_SOURCE_REALLOC
#   define D_CFG_MEM_SOURCE_REALLOC 1
#endif

// D_CFG_MEM_SOURCE_MAX_SIZE
//   brief: 1 gives d_mem_source_vtable a max_size slot, so an allocator can
// ask what its upstream could possibly satisfy and cap a growth step instead
// of requesting something that will certainly fail. Defaults ON; a source
// that leaves the slot null reports "unbounded", so the slot's presence costs
// a pointer and its absence costs nothing but the question.
#ifndef D_CFG_MEM_SOURCE_MAX_SIZE
#   define D_CFG_MEM_SOURCE_MAX_SIZE 1
#endif

// D_CFG_MEM_SOURCE_NAMED
//   brief: 1 gives d_mem_source_vtable a name slot, so a diagnostic can say
// WHICH source refused. Defaults to D_CFG_MEM_STATS's setting: a build that
// is not accounting is not reporting either.
#ifndef D_CFG_MEM_SOURCE_NAMED
#   ifdef D_CFG_MEM_ALL
#       define D_CFG_MEM_SOURCE_NAMED D_CFG_MEM_ALL
#   elif D_CFG_IS_ON(D_CFG_MEM_STATS)
#       define D_CFG_MEM_SOURCE_NAMED 1
#   else
#       define D_CFG_MEM_SOURCE_NAMED 0
#   endif
#endif


// ===========================================================================
//  3.  OVER-ALIGNED ALLOCATION API
// ===========================================================================
//   malloc guarantees alignment suitable for any fundamental type and nothing
// more. A request above that has four possible answers, and which one is
// available is a property of the platform's library rather than of its
// language level -- so this resolves by DETECTION and the knob exists to force
// a path for a bisect, not because a user is expected to choose.

// D_CFG_MEM_SOURCE_ALIGN_AUTO / _C11 / _POSIX / _WIN32 / _MANUAL
//   brief: the five admissible values of D_CFG_MEM_SOURCE_ALIGNED_API.
//     AUTO   - detect, preferring C11 aligned_alloc, then posix_memalign,
//              then _aligned_malloc, then MANUAL
//     C11    - aligned_alloc / free
//     POSIX  - posix_memalign / free
//     WIN32  - _aligned_malloc / _aligned_free
//     MANUAL - over-allocate and store the raw pointer behind the payload;
//              works everywhere, costs one pointer plus up to align-1 bytes
//              per over-aligned block
#define D_CFG_MEM_SOURCE_ALIGN_AUTO     0
#define D_CFG_MEM_SOURCE_ALIGN_C11      1
#define D_CFG_MEM_SOURCE_ALIGN_POSIX    2
#define D_CFG_MEM_SOURCE_ALIGN_WIN32    3
#define D_CFG_MEM_SOURCE_ALIGN_MANUAL   4

// D_CFG_MEM_SOURCE_ALIGNED_API
//   brief: which of the five paths satisfies an over-aligned request. Note
// that NONE of them is used for a request at or below the platform's
// fundamental alignment: that takes plain malloc, because the manual path's
// header and the C11 path's size-rounding are both pure cost when malloc
// already answers correctly.
#ifndef D_CFG_MEM_SOURCE_ALIGNED_API
#   define D_CFG_MEM_SOURCE_ALIGNED_API D_CFG_MEM_SOURCE_ALIGN_AUTO
#endif

#if !D_CFG_IS_INT_LITERAL(D_CFG_MEM_SOURCE_ALIGNED_API)
    #error "D_CFG_MEM_SOURCE_ALIGNED_API must name one of its values; a misspelled name would read as 0"
#endif


// ===========================================================================
//  4.  THE DEFAULT SOURCE
// ===========================================================================

// D_CFG_MEM_SOURCE_DEFAULT_SYSTEM / _NULL
//   brief: the two admissible values of D_CFG_MEM_SOURCE_DEFAULT.
#define D_CFG_MEM_SOURCE_DEFAULT_SYSTEM 0
#define D_CFG_MEM_SOURCE_DEFAULT_NULL   1

// D_CFG_MEM_SOURCE_DEFAULT
//   brief: what d_mem_source_default() returns -- the source an allocator
// takes when its configuration names none. SYSTEM where malloc is compiled in,
// NULL otherwise, which is what makes a freestanding build fail loudly at the
// first unconfigured allocator rather than quietly at link time.
#ifndef D_CFG_MEM_SOURCE_DEFAULT
#   if D_CFG_IS_ON(D_CFG_MEM_SOURCE_SYSTEM)
#       define D_CFG_MEM_SOURCE_DEFAULT D_CFG_MEM_SOURCE_DEFAULT_SYSTEM
#   else
#       define D_CFG_MEM_SOURCE_DEFAULT D_CFG_MEM_SOURCE_DEFAULT_NULL
#   endif
#endif

#if !D_CFG_IS_INT_LITERAL(D_CFG_MEM_SOURCE_DEFAULT)
    #error "D_CFG_MEM_SOURCE_DEFAULT must name one of its values; a misspelled name would read as 0"
#endif


// ===========================================================================
//  5.  VALIDATION
// ===========================================================================

#if !D_CFG_IS_ON(D_CFG_MEM_SOURCE_SYSTEM) &&                                  \
    !D_CFG_IS_OFF(D_CFG_MEM_SOURCE_SYSTEM)
#   error "D_CFG_MEM_SOURCE_SYSTEM must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_MEM_SOURCE_BUFFER) &&                                  \
    !D_CFG_IS_OFF(D_CFG_MEM_SOURCE_BUFFER)
#   error "D_CFG_MEM_SOURCE_BUFFER must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_MEM_SOURCE_NULL) && !D_CFG_IS_OFF(D_CFG_MEM_SOURCE_NULL)
#   error "D_CFG_MEM_SOURCE_NULL must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_MEM_SOURCE_COUNTER) &&                                 \
    !D_CFG_IS_OFF(D_CFG_MEM_SOURCE_COUNTER)
#   error "D_CFG_MEM_SOURCE_COUNTER must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_MEM_SOURCE_REALLOC) &&                                 \
    !D_CFG_IS_OFF(D_CFG_MEM_SOURCE_REALLOC)
#   error "D_CFG_MEM_SOURCE_REALLOC must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_MEM_SOURCE_MAX_SIZE) &&                                \
    !D_CFG_IS_OFF(D_CFG_MEM_SOURCE_MAX_SIZE)
#   error "D_CFG_MEM_SOURCE_MAX_SIZE must be 0 or 1"
#endif
#if !D_CFG_IS_ON(D_CFG_MEM_SOURCE_NAMED) &&                                   \
    !D_CFG_IS_OFF(D_CFG_MEM_SOURCE_NAMED)
#   error "D_CFG_MEM_SOURCE_NAMED must be 0 or 1"
#endif
#if ( (D_CFG_NORM(D_CFG_MEM_SOURCE_ALIGNED_API) <                             \
       D_CFG_MEM_SOURCE_ALIGN_AUTO) ||                                        \
      (D_CFG_NORM(D_CFG_MEM_SOURCE_ALIGNED_API) >                             \
       D_CFG_MEM_SOURCE_ALIGN_MANUAL) )
#   error "D_CFG_MEM_SOURCE_ALIGNED_API must be one of the five ALIGN_ values"
#endif
#if ( (D_CFG_NORM(D_CFG_MEM_SOURCE_DEFAULT) !=                                \
       D_CFG_MEM_SOURCE_DEFAULT_SYSTEM) &&                                    \
      (D_CFG_NORM(D_CFG_MEM_SOURCE_DEFAULT) !=                                \
       D_CFG_MEM_SOURCE_DEFAULT_NULL) )
#   error "D_CFG_MEM_SOURCE_DEFAULT must be _DEFAULT_SYSTEM or _DEFAULT_NULL"
#endif
#if ( (D_CFG_NORM(D_CFG_MEM_SOURCE_DEFAULT) ==                                \
       D_CFG_MEM_SOURCE_DEFAULT_SYSTEM) &&                                    \
      D_CFG_IS_OFF(D_CFG_MEM_SOURCE_SYSTEM) )
#   error "D_CFG_MEM_SOURCE_DEFAULT names the system source, which is off"
#endif


// ===========================================================================
//  6.  DERIVED VALUES
// ===========================================================================

// D_INTERNAL_MEM_SOURCE_SYSTEM / _BUFFER / _NULL / _COUNTER
//   brief: 1 when the corresponding built-in source should be compiled. Read
// by mem_source.h and mem_source.c.
#define D_INTERNAL_MEM_SOURCE_SYSTEM   D_CFG_NORM(D_CFG_MEM_SOURCE_SYSTEM)
#define D_INTERNAL_MEM_SOURCE_BUFFER   D_CFG_NORM(D_CFG_MEM_SOURCE_BUFFER)
#define D_INTERNAL_MEM_SOURCE_NULL     D_CFG_NORM(D_CFG_MEM_SOURCE_NULL)
#define D_INTERNAL_MEM_SOURCE_COUNTER  D_CFG_NORM(D_CFG_MEM_SOURCE_COUNTER)

// D_INTERNAL_MEM_SOURCE_REALLOC / _MAX_SIZE / _NAMED
//   brief: 1 when d_mem_source_vtable should carry the corresponding slot.
// These are the FIELD knobs of this file: they change the protocol's shape,
// so every source in the process must agree on them.
#define D_INTERNAL_MEM_SOURCE_REALLOC  D_CFG_NORM(D_CFG_MEM_SOURCE_REALLOC)
#define D_INTERNAL_MEM_SOURCE_MAX_SIZE D_CFG_NORM(D_CFG_MEM_SOURCE_MAX_SIZE)
#define D_INTERNAL_MEM_SOURCE_NAMED    D_CFG_NORM(D_CFG_MEM_SOURCE_NAMED)

// D_INTERNAL_MEM_SOURCE_ALIGNED_API
//   brief: the resolved over-alignment path, never AUTO. The detection order
// prefers the standard spelling, then the POSIX one, then the Microsoft one,
// and falls back to the portable manual path -- which is always correct, so
// the cascade can end without an #error and the tier law holds.
//   posix_memalign is probed through _POSIX_C_SOURCE rather than __unix__: the
// question is whether the LIBRARY has the function, and a version macro is the
// only portable statement of that (goals section 5, detect features).
#if (D_CFG_NORM(D_CFG_MEM_SOURCE_ALIGNED_API) != D_CFG_MEM_SOURCE_ALIGN_AUTO)
#   define D_INTERNAL_MEM_SOURCE_ALIGNED_API                                  \
        D_CFG_NORM(D_CFG_MEM_SOURCE_ALIGNED_API)
#elif defined(_MSC_VER)
#   define D_INTERNAL_MEM_SOURCE_ALIGNED_API D_CFG_MEM_SOURCE_ALIGN_WIN32
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) &&           \
      !defined(__APPLE__)
#   define D_INTERNAL_MEM_SOURCE_ALIGNED_API D_CFG_MEM_SOURCE_ALIGN_C11
#elif defined(_POSIX_C_SOURCE) && (_POSIX_C_SOURCE >= 200112L)
#   define D_INTERNAL_MEM_SOURCE_ALIGNED_API D_CFG_MEM_SOURCE_ALIGN_POSIX
#else
#   define D_INTERNAL_MEM_SOURCE_ALIGNED_API D_CFG_MEM_SOURCE_ALIGN_MANUAL
#endif

// D_INTERNAL_MEM_SOURCE_DEFAULT
//   brief: which source d_mem_source_default() returns.
#define D_INTERNAL_MEM_SOURCE_DEFAULT  D_CFG_NORM(D_CFG_MEM_SOURCE_DEFAULT)


#endif  // DJINTERP_CONFIG_CORE_MEMORY_CFG_MEM_SOURCE_H
