/*******************************************************************************
* djinterp [c]                                                      mem_source.h
*
* Upstream byte sources -- the shared C core (tier 0):
*   The protocol every allocator in this subframework obtains its backing
* storage through, and the built-in implementations of it. A source is two
* pointers -- a vtable and a context -- and answers exactly one question: give
* me N bytes aligned to A, and take them back later.
*
*   THIS IS THE CONTROL POINT. An arena, a pool and a slab all differ in how
* they SUBDIVIDE memory; none of them differs in where memory comes from,
* because all three take a d_mem_source. A caller that wants every byte to
* come from a static array passes the buffer source; one that wants to prove a
* subsystem allocates nothing passes the null source; one that wants to know
* what a subsystem costs wraps its real source in the counting source and
* reads the numbers off afterwards. No allocator in the subframework needs to
* know which of those happened.
*
* THE CONTRACT (all four clauses are load-bearing):
*   1. release() is SIZED AND ALIGNED. The caller must hand back the same
*      byte count and the same alignment it was given. This is not politeness:
*      the system source decides between malloc and an over-aligned path by
*      looking at the alignment, and the buffer source recognises the most
*      recent allocation by looking at the size. A source is entitled to
*      corrupt itself if the caller lies.
*   2. allocate(0) returns NULL and is not an error. Zero bytes is a
*      degenerate request, not a malformed one, and every caller in this
*      subframework already tests the result.
*   3. alignment is a POWER OF TWO within [D_MEM_ALIGN_MIN, D_MEM_ALIGN_MAX].
*      Zero means "the configured default". Anything else is refused rather
*      than rounded, because a silently rounded alignment is a corruption the
*      caller will be blamed for.
*   4. a source is STATELESS OR OWNS ITS STATE. The vtable is shared; the
*      context is the instance. Copying a d_mem_source copies a reference to
*      the context, never the context -- so two copies of a buffer source
*      share one buffer, which is the intent.
*
* PORTABLE ACROSS:
*   C99, C11, C17, C23  /  C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/djinterp/c/memory/mem_source.h
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    PROTOCOL
      --------
      a. struct d_mem_source_vtable
      b. struct d_mem_source
II.   BUILT-IN SOURCE STATE
      ---------------------
      a. struct d_mem_buffer_source
      b. struct d_mem_counting_source
III.  SOURCE CONSTRUCTION
      -------------------
      a. d_mem_source_none / _default / _system / _null
      b. d_mem_source_buffer_init / _buffer
      c. d_mem_source_counting_init / _counting
IV.   SOURCE OPERATIONS
      -----------------
      a. d_mem_source_is_valid / _resolve / _name
      b. d_mem_source_allocate / _allocate_ex
      c. d_mem_source_release
      d. d_mem_source_reallocate / _reallocate_ex
      e. d_mem_source_max_size
V.    BUFFER SOURCE QUERIES
      ---------------------
      a. d_mem_buffer_source_used / _remaining / _reset
VI.   COUNTING SOURCE QUERIES
      -----------------------
      a. d_mem_counting_source_stats
*/

#ifndef DJINTERP_C_MEMORY_MEM_SOURCE_H
#define DJINTERP_C_MEMORY_MEM_SOURCE_H 1

// djinterp
#include "./mem_common.h"
#include "../../config/core/memory/cfg_mem_source.h"


D_EXTERN_C_BEGIN


///////////////////////////////////////////////////////////////////////////////
///                        I.   PROTOCOL                                    ///
///////////////////////////////////////////////////////////////////////////////

// d_mem_source_vtable
//   struct: the operation table of a byte source. One instance per source
// KIND, shared by every source of that kind; the per-instance state lives in
// the context pointer beside it.
//   Three of the five slots are optional, gated by cfg_mem_source.h. An
// absent slot is not a missing capability -- d_mem_source_reallocate emulates
// reallocation with allocate/copy/release, and an absent max_size reports
// "unbounded" -- so a narrower protocol costs a copy or a question, never a
// behaviour.
//   NOTE FOR C++ IMPLEMENTERS: this declaration sits inside an extern "C"
// block, so these pointer types carry C language linkage. A C++ source should
// give its slot functions extern "C" linkage too. Every mainstream compiler
// accepts the mismatch; the standard does not require it to.
struct d_mem_source_vtable
{
    // allocate
    //   returns _bytes bytes aligned to _align, or NULL. A request of zero
    // bytes returns NULL and is not an error.
    void*      (*allocate)(void*      _context,
                           d_mem_size _bytes,
                           d_mem_size _align);

    // release
    //   returns a block previously produced by allocate. _bytes and _align
    // MUST match the values that produced it.
    void       (*release)(void*      _context,
                          void*      _ptr,
                          d_mem_size _bytes,
                          d_mem_size _align);

#if (D_INTERNAL_MEM_SOURCE_REALLOC == 1)
    // reallocate
    //   grows or shrinks a block in place where it can, moving it otherwise.
    // May be NULL even when the slot exists, in which case the emulation
    // applies -- so a source opts in per instance, not per build.
    void*      (*reallocate)(void*      _context,
                             void*      _ptr,
                             d_mem_size _old_bytes,
                             d_mem_size _new_bytes,
                             d_mem_size _align);
#endif

#if (D_INTERNAL_MEM_SOURCE_MAX_SIZE == 1)
    // max_size
    //   the largest single request this source could satisfy. May be NULL,
    // meaning unbounded.
    d_mem_size (*max_size)(const void* _context);
#endif

#if (D_INTERNAL_MEM_SOURCE_NAMED == 1)
    // name
    //   a stable, static name for diagnostics. May be NULL.
    const char* name;
#endif
};

// d_mem_source
//   struct: a byte source -- an operation table plus the instance state it
// operates on. Two pointers, copied by value, referring to shared state.
//   A ZEROED d_mem_source IS THE "UNSET" SOURCE, not a broken one. Every
// configuration struct in this subframework can therefore be zero-initialised
// and still mean something sensible: d_mem_source_resolve turns an unset
// source into the configured default.
struct d_mem_source
{
    const struct d_mem_source_vtable* vtable;
    void*                             context;
};


///////////////////////////////////////////////////////////////////////////////
///                  II.   BUILT-IN SOURCE STATE                            ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_MEM_SOURCE_BUFFER == 1)

// d_mem_buffer_source
//   struct: the state of a source that vends from memory the caller already
// owns. Monotonic by construction -- it has no free list, because a source
// that hands whole regions to an arena or a pool is asked for a handful of
// large blocks over its lifetime, and a free list for a handful of blocks is
// a data structure that costs more than it saves.
//   It does honour a release of the MOST RECENT block, which is the case that
// actually arises: an allocator that asks for a region, finds it cannot use
// it, and hands it straight back. That is one comparison, not a data
// structure.
struct d_mem_buffer_source
{
    char*      base;         // the caller's buffer
    d_mem_size capacity;     // its length in bytes
    d_mem_size used;         // bytes handed out so far
    d_mem_size last_offset;  // offset of the most recent block, for pop-back
    d_mem_size last_size;    // its size, or 0 when there is no popable block
};

#endif  // D_INTERNAL_MEM_SOURCE_BUFFER

#if (D_INTERNAL_MEM_SOURCE_COUNTER == 1)

// d_mem_counting_source
//   struct: the state of a source that forwards to another and accounts for
// what passes through. A DECORATOR: it is a source, it holds a source, and
// anything that takes a source takes it without knowing.
//   This is how a caller answers "what does this subsystem cost" without
// instrumenting the subsystem: wrap the source it was going to be given,
// hand it the wrapper, read the block afterwards.
struct d_mem_counting_source
{
    struct d_mem_source upstream;
    struct d_mem_stats  stats;
};

#endif  // D_INTERNAL_MEM_SOURCE_COUNTER


// I.    source construction
struct d_mem_source d_mem_source_none(void);
struct d_mem_source d_mem_source_default(void);
#if (D_INTERNAL_MEM_SOURCE_SYSTEM == 1)
struct d_mem_source d_mem_source_system(void);
#endif
#if (D_INTERNAL_MEM_SOURCE_NULL == 1)
struct d_mem_source d_mem_source_null(void);
#endif
#if (D_INTERNAL_MEM_SOURCE_BUFFER == 1)
enum d_mem_status   d_mem_buffer_source_init(struct d_mem_buffer_source* _state,
                                             void*                       _buffer,
                                             d_mem_size                  _bytes);
struct d_mem_source d_mem_source_buffer(struct d_mem_buffer_source* _state);
#endif
#if (D_INTERNAL_MEM_SOURCE_COUNTER == 1)
enum d_mem_status   d_mem_counting_source_init(struct d_mem_counting_source* _state,
                                               struct d_mem_source           _upstream);
struct d_mem_source d_mem_source_counting(struct d_mem_counting_source* _state);
#endif

// II.   source operations
bool                d_mem_source_is_valid(const struct d_mem_source* _source);
struct d_mem_source d_mem_source_resolve(const struct d_mem_source* _source);
const char*         d_mem_source_name(const struct d_mem_source* _source);
void*               d_mem_source_allocate(const struct d_mem_source* _source,
                                          d_mem_size                 _bytes,
                                          d_mem_size                 _align);
enum d_mem_status   d_mem_source_allocate_ex(const struct d_mem_source* _source,
                                             d_mem_size                 _bytes,
                                             d_mem_size                 _align,
                                             struct d_mem_block*        _out);
void                d_mem_source_release(const struct d_mem_source* _source,
                                         void*                      _ptr,
                                         d_mem_size                 _bytes,
                                         d_mem_size                 _align);
void                d_mem_source_release_block(const struct d_mem_source* _source,
                                               struct d_mem_block*        _block,
                                               d_mem_size                 _align);
void*               d_mem_source_reallocate(const struct d_mem_source* _source,
                                            void*                      _ptr,
                                            d_mem_size                 _old_bytes,
                                            d_mem_size                 _new_bytes,
                                            d_mem_size                 _align);
enum d_mem_status   d_mem_source_reallocate_ex(const struct d_mem_source* _source,
                                               d_mem_size                 _new_bytes,
                                               d_mem_size                 _align,
                                               struct d_mem_block*        _block);
d_mem_size          d_mem_source_max_size(const struct d_mem_source* _source);

// III.  buffer source queries
#if (D_INTERNAL_MEM_SOURCE_BUFFER == 1)
d_mem_size          d_mem_buffer_source_used(const struct d_mem_buffer_source* _state);
d_mem_size          d_mem_buffer_source_remaining(const struct d_mem_buffer_source* _state);
void                d_mem_buffer_source_reset(struct d_mem_buffer_source* _state);
#endif

// IV.   counting source queries
#if (D_INTERNAL_MEM_SOURCE_COUNTER == 1)
enum d_mem_status   d_mem_counting_source_stats(const struct d_mem_counting_source* _state,
                                                struct d_mem_stats*                 _out);
#endif


///////////////////////////////////////////////////////////////////////////////
///                     V.   LAYOUT ASSERTIONS                              ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_MEM_ASSERT_LAYOUT == 1)

D_STATIC_ASSERT(offsetof(struct d_mem_source, vtable) == 0,
                "d_mem_source.vtable must lead the struct");

D_STATIC_ASSERT(sizeof(struct d_mem_source) ==
                    (2 * sizeof(void*)),
                "d_mem_source must be exactly two pointers");

D_STATIC_ASSERT(offsetof(struct d_mem_source_vtable, allocate) == 0,
                "d_mem_source_vtable.allocate must lead the vtable");

#endif  // D_INTERNAL_MEM_ASSERT_LAYOUT

#if (D_INTERNAL_MEM_ASSERT_SIZES == 1) && (D_INTERNAL_MEM_SOURCE_BUFFER == 1)

D_STATIC_ASSERT(sizeof(struct d_mem_buffer_source) == 40,
                "d_mem_buffer_source layout drift (LP64, native counter)");

#endif  // D_INTERNAL_MEM_ASSERT_SIZES


D_EXTERN_C_END


#endif  // DJINTERP_C_MEMORY_MEM_SOURCE_H
