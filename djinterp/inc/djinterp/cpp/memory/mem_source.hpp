/*******************************************************************************
* djinterp [cpp]                                                  mem_source.hpp
*
* C++ face of the upstream byte-source protocol.
*   `DMemorySource` DERIVES from dj_mem_source and adds no members, so it IS
* one and may be passed to any C entry point with no conversion. What it adds
* is that a default-constructed one is the well-defined UNSET source rather
* than two indeterminate pointers -- which is the single most likely way to
* misuse the C struct, and the only thing a C++ face can fix about it.
*
*   `DBufferSource` and `DCountingSource` are HOLDERS, not wrappers: each owns
* the state block the C protocol requires and hands out a source bound to it,
* so the state cannot outlive its source or the source its state. They are
* therefore exempt from the cost law by construction, and say so below.
*
* THE HEAPLESS ENTRY POINT:
*
*     static char storage[65536];
*     DBufferSource bytes(storage);
*     arena         scratch(bytes.source());
*
* Nothing in that program calls malloc, and no configuration knob is needed to
* arrange it.
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/djinterp/cpp/memory/mem_source.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    DMemorySource
      -------------
      a. construction / validity / naming
      b. allocate / release / max_size
      c. default_source / system / refusing
II.   DBufferSource
      -------------
III.  DCountingSource
      ---------------
IV.   THE COST LAW
*/

#ifndef DJINTERP_MEMORY_MEM_SOURCE_HPP
#define DJINTERP_MEMORY_MEM_SOURCE_HPP 1

// FLOOR, FOR NOW: below C++11 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "./mem_common.hpp"
#include "../../c/memory/mem_source.h"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                      I.   DMemorySource                                 ///
///////////////////////////////////////////////////////////////////////////////

// DMemorySource
//   class: a byte source. Derives from the C struct and adds NO members, so
// it IS a dj_mem_source and may be passed to any C entry point by slicing --
// which is not a hazard here but the mechanism, since the base carries all the
// state and the derived part carries none.
//   What it adds is that a default-constructed one is the well-defined UNSET
// source rather than two indeterminate pointers, which is the single most
// likely way to misuse the C struct.
class DMemorySource : public ::dj_mem_source
{
public:

    // DMemorySource (default)
    //   constructor: the unset source, which resolves to the configured
    // default wherever an allocator is handed one.
    D_INLINE
    DMemorySource()
        : ::dj_mem_source()
    {
        static_cast< ::dj_mem_source& >(*this) = ::d_mem_source_none();
    }

    // DMemorySource (converting)
    //   constructor: wraps a source produced by a C factory.
    D_INLINE
    DMemorySource(
        const ::dj_mem_source& _source
    )
        : ::dj_mem_source(_source)
    {}

    // is_valid
    //   query: true when this source can be drawn from.
    D_INLINE
    bool is_valid() const
    {
        return ::d_mem_source_is_valid(this);
    }

    // name
    //   query: a stable name for diagnostics. Never null.
    D_INLINE
    const char* name() const
    {
        return ::d_mem_source_name(this);
    }

    // allocate
    //   operation: obtains _bytes bytes at _align, or returns null.
    D_INLINE
    void* allocate(
        DMemSize _bytes,
        DMemSize _align = 0
    ) const
    {
        return ::d_mem_source_allocate(this, _bytes, _align);
    }

    // release
    //   operation: returns a block. _bytes and _align MUST be the values that
    // produced it -- see the contract in mem_source.h.
    D_INLINE
    void release(
        void*    _ptr,
        DMemSize _bytes,
        DMemSize _align = 0
    ) const
    {
        ::d_mem_source_release(this, _ptr, _bytes, _align);

        return;
    }

    // max_size
    //   query: the largest single request this source could satisfy.
    D_INLINE
    DMemSize max_size() const
    {
        return ::d_mem_source_max_size(this);
    }

    // default_source
    //   factory: the source an allocator takes when none is named.
    static D_INLINE
    DMemorySource default_source()
    {
        return DMemorySource(::d_mem_source_default());
    }

#if (D_INTERNAL_MEM_SOURCE_SYSTEM == 1)
    // system
    //   factory: the malloc-backed source.
    static D_INLINE
    DMemorySource system()
    {
        return DMemorySource(::d_mem_source_system());
    }
#endif

#if (D_INTERNAL_MEM_SOURCE_NULL == 1)
    // none_source
    //   factory: the source that refuses everything, which turns "this
    // subsystem does not allocate" from a claim into a test.
    static D_INLINE
    DMemorySource refusing()
    {
        return DMemorySource(::d_mem_source_null());
    }
#endif
};


#if (D_INTERNAL_MEM_SOURCE_BUFFER == 1)

// DBufferSource
//   class: a source that vends from memory the caller owns, with the state
// block and the source bundled so neither can outlive the other by accident.
//   THIS IS THE HEAPLESS ENTRY POINT: give it a static array, hand its
// source() to an arena or a pool, and nothing in the program calls malloc.
class DBufferSource
{
public:

    // DBufferSource (parameterized)
    //   constructor: prepares a source over _bytes bytes at _buffer. The
    // buffer is NOT copied and NOT owned; it must outlive this object.
    D_INLINE
    DBufferSource(
        void*    _buffer,
        DMemSize _bytes
    )
    {
        ::d_mem_buffer_source_init(&m_state, _buffer, _bytes);
    }

    // DBufferSource (array)
    //   constructor: prepares a source over a whole array, so the size cannot
    // disagree with the storage.
    template<typename Type,
             std::size_t Count>
    explicit D_INLINE
    DBufferSource(
        Type (&_array)[Count]
    )
    {
        ::d_mem_buffer_source_init(
            &m_state,
            static_cast<void*>(_array),
            static_cast<DMemSize>(sizeof(Type) * Count));
    }

    DBufferSource(
        const DBufferSource&
    ) = delete;
    DBufferSource& operator=(
        const DBufferSource&
    ) = delete;

    // source
    //   query: this buffer as a byte source.
    D_INLINE
    DMemorySource source()
    {
        return DMemorySource(::d_mem_source_buffer(&m_state));
    }

    // used / remaining
    //   query: bytes handed out, and bytes still unclaimed.
    D_INLINE
    DMemSize used() const
    {
        return ::d_mem_buffer_source_used(&m_state);
    }

    D_INLINE
    DMemSize remaining() const
    {
        return ::d_mem_buffer_source_remaining(&m_state);
    }

    // reset
    //   operation: reclaims every byte. INVALIDATES EVERYTHING this source
    // ever vended, so only call it once every allocator drawing from it has
    // been released.
    D_INLINE
    void reset()
    {
        ::d_mem_buffer_source_reset(&m_state);

        return;
    }

private:

    ::dj_mem_buffer_source m_state;
};

#endif  // D_INTERNAL_MEM_SOURCE_BUFFER


#if (D_INTERNAL_MEM_SOURCE_COUNTER == 1)

// DCountingSource
//   class: a decorator that forwards to another source and accounts for what
// passes through. The way to answer "what does this subsystem cost" without
// instrumenting the subsystem: wrap the source it was going to be given, hand
// it the wrapper, read the numbers afterwards.
class DCountingSource
{
public:

    // DCountingSource (parameterized)
    //   constructor: wraps _upstream, defaulting to the configured default.
    explicit D_INLINE
    DCountingSource(
        const ::dj_mem_source& _upstream = ::d_mem_source_default()
    )
    {
        ::d_mem_counting_source_init(&m_state, _upstream);
    }

    DCountingSource(
        const DCountingSource&
    ) = delete;
    DCountingSource& operator=(
        const DCountingSource&
    ) = delete;

    // source
    //   query: this decorator as a byte source.
    D_INLINE
    DMemorySource source()
    {
        return DMemorySource(::d_mem_source_counting(&m_state));
    }

    // stats
    //   query: what has passed through.
    D_INLINE
    DMemStats stats() const
    {
        DMemStats out;

        ::d_mem_counting_source_stats(&m_state, &out);

        return out;
    }

private:

    ::dj_mem_counting_source m_state;
};

#endif  // D_INTERNAL_MEM_SOURCE_COUNTER




///////////////////////////////////////////////////////////////////////////////
///                      IV.   THE COST LAW                                 ///
///////////////////////////////////////////////////////////////////////////////

D_STATIC_ASSERT(sizeof(DMemorySource) == sizeof(::dj_mem_source),
                "memory_source must add nothing to dj_mem_source");

D_STATIC_ASSERT(std::is_standard_layout<DMemorySource>::value,
                "memory_source must remain standard-layout");

//   NOTE the extra parentheses: a template argument list's comma is a MACRO
// argument separator, so is_base_of<A, B> reaches D_STATIC_ASSERT as two
// arguments and the assertion fails to compile with a message about the macro
// rather than about the layout.
D_STATIC_ASSERT((std::is_base_of< ::dj_mem_source, DMemorySource>::value),
                "memory_source must derive from the C kernel, not mirror it");


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_MEMORY_MEM_SOURCE_HPP
