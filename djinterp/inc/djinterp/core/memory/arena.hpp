/*******************************************************************************
* djinterp [core]                                                      arena.hpp
*
* C++ face of the monotonic bump arena.
*   `arena` DERIVES from d_arena and adds no data members, so it IS one: it may
* be passed to any C entry point, its layout is the C layout, and its size is
* asserted equal below. What it adds is lifetime -- construction initializes,
* destruction releases -- and the two ergonomics that a C caller has to write
* by hand every time: a scope guard over mark/rewind, and typed allocation.
*
*   NOTHING HERE REIMPLEMENTS THE ALGORITHM. Every member forwards to arena.c.
* If a future edit finds itself computing an offset in this file, that is the
* signal that the kernel is missing something, not that the wrapper should
* grow a second implementation (AGENT_README.md section 10).
*
* THE SCOPE GUARD IS THE REASON TO PREFER THIS FACE:
*
*     void render_frame(arena& _scratch)
*     {
*         arena_scope guard(_scratch);      // mark
*         auto* verts = _scratch.create_array<vertex>(count);
*         ...
*     }                                     // rewind, on every path
*
* The C form is the same two calls, and the difference is that the rewind
* cannot be skipped by an early return or an exception.
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/djinterp/core/memory/arena.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    arena
      -----
      a. construction / move / destruction
      b. allocate / allocate_array / create / create_array / duplicate
      c. mark / rewind / reset / trim / release
      d. queries
II.   arena_scope
      -----------
      a. RAII mark and rewind
*/

#ifndef DJINTERP_MEMORY_ARENA_HPP
#define DJINTERP_MEMORY_ARENA_HPP 1

// std
#include <new>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "./mem_common.hpp"
#include "./mem_source.hpp"
#include "../../c/memory/arena.h"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                          I.   arena                                     ///
///////////////////////////////////////////////////////////////////////////////

// arena
//   class: a monotonic bump allocator. Derives from d_arena and adds no
// state; every operation forwards to the C kernel.
//   NOT THREAD-SAFE, exactly as the kernel is not -- the wrapper does not
// silently add a lock, because a caller who needs one should be able to see
// where it is.
class arena : public ::d_arena
{
public:

    using size_type = mem_size;

    // arena (default)
    //   constructor: an arena over the configured default source. Obtains no
    // memory -- the first region is taken on the first allocation.
    D_INLINE
    arena()
    {
        ::d_arena_init(this, nullptr);
    }

    // arena (configured)
    //   constructor: an arena differing from the defaults as the config says.
    explicit D_INLINE
    arena(
        const ::d_arena_config& _config
    )
    {
        ::d_arena_init(this, &_config);
    }

    // arena (sourced)
    //   constructor: an arena over a named source, with optional first-region
    // size and total budget. The common case that would otherwise need a
    // config struct at every call site.
    explicit D_INLINE
    arena(
        const ::d_mem_source& _source,
        size_type             _initial_bytes = 0,
        size_type             _max_bytes     = 0
    )
    {
        ::d_arena_config config = ::d_arena_config_default();

        config.source        = _source;
        config.initial_bytes = _initial_bytes;
        config.max_bytes     = _max_bytes;

        ::d_arena_init(this, &config);
    }

    // arena (over a caller's buffer)
    //   constructor: an arena over memory the caller owns, with NO upstream
    // source at all. The buffer must outlive the arena; nothing here can
    // check that, and nothing here will free it.
    D_INLINE
    arena(
        void*     _buffer,
        size_type _bytes
    )
    {
        ::d_arena_init_buffer(this, _buffer, _bytes);
    }

    // ~arena
    //   destructor: returns every region to the source. Runs NO destructors
    // on anything allocated from the arena -- the kernel deals in raw
    // storage, and so does this.
    D_INLINE
    ~arena()
    {
        ::d_arena_release(this);
    }

    arena(const arena&)            = delete;
    arena& operator=(const arena&) = delete;

    // arena (move)
    //   constructor: takes over another arena's regions, leaving it empty but
    // usable. A byte-wise steal, which is correct because the kernel struct
    // holds no self-pointers -- its regions point at each other, never back
    // at the arena.
    D_INLINE
    arena(
        arena&& _other
    ) noexcept
    {
        static_cast< ::d_arena& >(*this) = static_cast< ::d_arena& >(_other);
        ::d_arena_init(&_other, nullptr);
    }

    D_INLINE arena&
    operator=(
        arena&& _other
    ) noexcept
    {
        if (this != &_other)
        {
            ::d_arena_release(this);
            static_cast< ::d_arena& >(*this) =
                static_cast< ::d_arena& >(_other);
            ::d_arena_init(&_other, nullptr);
        }

        return *this;
    }

    // ---------------------------------------------------------------
    //  allocation
    // ---------------------------------------------------------------

    // allocate
    //   operation: obtains _bytes raw bytes at _align, or returns null.
    D_INLINE void*
    allocate(
        size_type _bytes,
        size_type _align = 0
    )
    {
        return ::d_arena_allocate(this, _bytes, _align);
    }

    // allocate_ex
    //   operation: obtains bytes and reports why a failure failed.
    D_INLINE mem_status
    allocate_ex(
        size_type  _bytes,
        size_type  _align,
        mem_block& _out
    )
    {
        return ::d_arena_allocate_ex(this, _bytes, _align, &_out);
    }

    // allocate_object
    //   operation: raw storage for one _Type, correctly sized and aligned.
    // NO CONSTRUCTOR RUNS -- use create for that.
    template<typename _Type>
    D_INLINE _Type*
    allocate_object()
    {
        return static_cast<_Type*>(
            ::d_arena_allocate(this,
                               size_of<_Type>::value,
                               align_of<_Type>::value));
    }

    // allocate_array
    //   operation: raw storage for _count objects of _Type. The product is
    // guarded against overflow by the kernel, which is the reason to route
    // through it rather than multiply here.
    template<typename _Type>
    D_INLINE _Type*
    allocate_array(
        size_type _count
    )
    {
        return static_cast<_Type*>(
            ::d_arena_allocate_array(this,
                                     _count,
                                     size_of<_Type>::value,
                                     align_of<_Type>::value));
    }

    // create
    //   operation: allocates and CONSTRUCTS one _Type from _args.
    //   THE OBJECT IS NEVER DESTROYED BY THE ARENA. An arena has no record of
    // what it handed out, so it cannot run destructors, and reset simply
    // forgets. Use it for types whose destruction is a no-op, or destroy them
    // yourself before rewinding. This is the arena's central trade and the
    // wrapper does not paper over it.
    template<typename _Type,
             typename... _Args>
    D_INLINE _Type*
    create(
        _Args&&... _args
    )
    {
        void* storage = ::d_arena_allocate(this,
                                           size_of<_Type>::value,
                                           align_of<_Type>::value);

        if (!storage)
        {
            return nullptr;
        }

        return new (storage) _Type(std::forward<_Args>(_args)...);
    }

    // create_array
    //   operation: allocates and default-constructs _count objects of _Type.
    // Subject to the same no-destruction caveat as create.
    template<typename _Type>
    D_INLINE _Type*
    create_array(
        size_type _count
    )
    {
        _Type*    array;
        size_type index;

        array = allocate_array<_Type>(_count);

        if (!array)
        {
            return nullptr;
        }

        for (index = 0; index < _count; ++index)
        {
            new (static_cast<void*>(array + index)) _Type();
        }

        return array;
    }

    // duplicate
    //   operation: copies a byte range into the arena.
    D_INLINE void*
    duplicate(
        const void* _source,
        size_type   _bytes,
        size_type   _align = 0
    )
    {
        return ::d_arena_duplicate(this, _source, _bytes, _align);
    }

    // duplicate_string
    //   operation: copies a null-terminated string into the arena,
    // terminator included.
    D_INLINE char*
    duplicate_string(
        const char* _text
    )
    {
        return ::d_arena_duplicate_string(this, _text);
    }

    // ---------------------------------------------------------------
    //  bulk reclamation
    // ---------------------------------------------------------------

    // reset
    //   operation: reclaims every byte, keeping the regions for reuse.
    D_INLINE void
    reset()
    {
        ::d_arena_reset(this);

        return;
    }

    // trim
    //   operation: releases every region after the first, then rewinds. The
    // middle ground for an arena that spiked once and will not again.
    D_INLINE void
    trim()
    {
        ::d_arena_trim(this);

        return;
    }

    // release
    //   operation: returns every region to the source.
    D_INLINE void
    release()
    {
        ::d_arena_release(this);

        return;
    }

#if (D_INTERNAL_ARENA_MARKS == 1)
    // mark
    //   query: the arena's current position, for a later rewind.
    D_INLINE ::d_arena_mark
    mark() const
    {
        return ::d_arena_mark_get(this);
    }

    // rewind
    //   operation: restores a previously marked position.
    D_INLINE mem_status
    rewind(
        ::d_arena_mark _mark
    )
    {
        return ::d_arena_rewind(this, _mark);
    }
#endif

    // ---------------------------------------------------------------
    //  queries
    // ---------------------------------------------------------------

    // used / capacity / remaining / region_count
    //   query: the arena's population, forwarded to the kernel.
    D_INLINE size_type used()         const { return ::d_arena_used(this); }
    D_INLINE size_type capacity()     const { return ::d_arena_capacity(this); }
    D_INLINE size_type remaining()    const { return ::d_arena_remaining(this); }
    D_INLINE size_type region_count() const
    {
        return ::d_arena_region_count(this);
    }

#if (D_INTERNAL_ARENA_OWNS == 1)
    // owns
    //   query: whether a pointer came from this arena. O(regions) -- a
    // diagnostic, not something a hot path should call.
    D_INLINE bool
    owns(
        const void* _ptr
    ) const
    {
        return ::d_arena_owns(this, _ptr);
    }
#endif

    // stats
    //   query: the accounting block, or a zeroed one where accounting is not
    // compiled.
    D_INLINE mem_stats
    stats() const
    {
        mem_stats out;

        ::d_arena_stats(this, &out);

        return out;
    }

#if (D_INTERNAL_ARENA_AS_SOURCE == 1)
    // as_source
    //   query: this arena presented AS a byte source, so a pool or another
    // arena can draw its blocks from it. The composition point.
    D_INLINE memory_source
    as_source()
    {
        return memory_source(::d_arena_as_source(this));
    }
#endif
};


///////////////////////////////////////////////////////////////////////////////
///                       II.   arena_scope                                 ///
///////////////////////////////////////////////////////////////////////////////

#if (D_INTERNAL_ARENA_MARKS == 1)

// arena_scope
//   class: takes a mark on construction and rewinds to it on destruction, so
// an arena becomes a stack allocator whose unwinding cannot be skipped by an
// early return, a break, or an exception.
//   Scopes NEST, because marks do: the inner one rewinds to where the outer
// one already was, and neither knows about the other.
class arena_scope
{
public:

    // arena_scope (parameterized)
    //   constructor: marks _arena's current position.
    explicit D_INLINE
    arena_scope(
        arena& _arena
    )
        : m_arena(&_arena),
          m_mark(_arena.mark())
    {}

    // ~arena_scope
    //   destructor: rewinds to the marked position, unless released.
    //   RUNS NO DESTRUCTORS on anything allocated inside the scope, for the
    // same reason arena::create does not: the arena has no record of what it
    // handed out. A scope over non-trivially-destructible objects is a
    // deliberate choice, not an oversight the guard will cover for.
    D_INLINE
    ~arena_scope()
    {
        if (m_arena)
        {
            m_arena->rewind(m_mark);
        }
    }

    arena_scope(const arena_scope&)            = delete;
    arena_scope& operator=(const arena_scope&) = delete;

    // dismiss
    //   operation: abandons the rewind, so everything allocated in the scope
    // survives it. For the case where a scope turns out to have produced the
    // result rather than scratch.
    D_INLINE void
    dismiss()
    {
        m_arena = nullptr;

        return;
    }

private:

    arena*         m_arena;
    ::d_arena_mark m_mark;
};

#endif  // D_INTERNAL_ARENA_MARKS


///////////////////////////////////////////////////////////////////////////////
///                      III.   THE COST LAW                                ///
///////////////////////////////////////////////////////////////////////////////

D_STATIC_ASSERT(sizeof(arena) == sizeof(::d_arena),
                "arena must add nothing to d_arena");

D_STATIC_ASSERT((std::is_base_of< ::d_arena, arena>::value),
                "arena must derive from the C kernel, not mirror it");

D_STATIC_ASSERT(std::is_standard_layout< ::d_arena>::value,
                "d_arena must remain standard-layout for the wrapper to be "
                "layout-compatible with it");


NS_END  // djinterp


#endif  // DJINTERP_MEMORY_ARENA_HPP
