/*******************************************************************************
* djinterp [core]                                             pool_allocator.hpp
*
* Standard-library allocator adapters.
*   `pool_allocator<T>` and `arena_allocator<T>` present the kernels as
* Allocator-conforming types, so std::vector, std::list, std::map and every
* other standard container can be given one.
*
*   THIS FILE IS PURE NOTATION, and is the clearest case in the subframework
* of the framework's phase rule (AGENT_README.md section 8): it computes
* NOTHING. Every allocate() call forwards to the kernel, and the adapter's
* whole content is the typedef set, the rebind machinery, and the comparison
* operators that the Allocator requirements demand. There is no C counterpart
* because there is no C notion of an Allocator to have one -- which is exactly
* the "C++-only notation over shared semantics" the framework permits.
*
* WHICH ADAPTER FOR WHICH CONTAINER:
*
*   pool_allocator   fixed-size nodes, so it fits the NODE-BASED containers --
*                    std::list, std::forward_list, std::set, std::map,
*                    std::unordered_* -- where nearly every allocation is one
*                    node and every node is the same size. This is the pairing
*                    that makes a pool worth having: a std::list<T> over a
*                    pool_allocator<T> allocates one block per few hundred
*                    nodes instead of one malloc per node.
*                    It draws from a `node_pool`, which sizes itself from the
*                    container's own node type on first use -- see that class
*                    for why the size cannot be known any earlier.
*
*   arena_allocator  variable-size allocations that are never individually
*                    freed, so it fits std::vector and std::basic_string in a
*                    phase-scoped workload. deallocate() is a NO-OP: the
*                    memory returns when the arena is reset or released, and
*                    a vector that grows repeatedly will leave its earlier
*                    buffers behind. That is the arena's trade, stated here so
*                    it is chosen rather than discovered.
*
* LIFETIME:
*   Both adapters hold a REFERENCE to their allocator, not a copy. The pool or
* arena must outlive every container that was given one -- and every copy of
* every iterator into it. Nothing here can check that.
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/djinterp/core/memory/pool_allocator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.08.13
*                                                            revised: 2026.09.21
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    pool_allocator<T>
      -----------------
      a. typedefs and rebind
      b. allocate / deallocate
      c. equality
II.   arena_allocator<T>
      ------------------
      a. typedefs and rebind
      b. allocate / deallocate (no-op)
      c. equality
*/

#ifndef DJINTERP_MEMORY_POOL_ALLOCATOR_HPP
#define DJINTERP_MEMORY_POOL_ALLOCATOR_HPP 1

// std
#include <cstddef>
#include <new>
// djinterp
#include "../../djinterp.hpp"
#include "./mem_common.hpp"
#include "./mem_source.hpp"
#include "./pool.hpp"
#include "./arena.hpp"


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///                   I.   pool_allocator<T>                                ///
///////////////////////////////////////////////////////////////////////////////

// node_pool
//   class: a pool that configures itself on FIRST USE, plus a fallback source
// for anything it cannot serve.
//
//   THE PROBLEM IT EXISTS TO SOLVE. A std::list<int, A> does not allocate
// ints. It allocates its own private node -- an int plus two pointers -- and
// rebinds A to that node type internally. The node type's NAME is
// implementation-private, so a caller cannot write it down, and a pool sized
// from sizeof(int) produces four-byte slots for twenty-four-byte nodes. There
// is no portable expression that names the thing the container will actually
// ask for.
//
//   But the REBOUND ALLOCATOR knows: inside the container, the allocator is
// pool_allocator<node_type>, and sizeof(node_type) is available there. So the
// size arrives at the first allocate() call and not before -- which is why
// this class defers configuration to that call rather than to its
// constructor.
//
//   THE FALLBACK IS NOT A CONVENIENCE. std::unordered_map allocates nodes
// through one rebound allocator and its bucket array through another, so a
// single pool cannot serve every request a container makes. Refusing the
// second size would make this adapter unusable with the unordered containers
// for a reason the caller could neither predict nor fix. Instead, a request
// the pool cannot serve goes to the upstream source, and release routes by
// asking the pool whether the pointer is its own. Degrade, never error.
class node_pool
{
public:

    // node_pool (parameterized)
    //   constructor: an unconfigured pool. Its slot geometry is fixed by the
    // first allocation, and its blocks and overflow both come from _source.
    explicit D_INLINE
    node_pool(
        const ::d_mem_source& _source         = ::d_mem_source_none(),
        ::d_pool_index        _slots_per_block = 0,
        ::d_pool_index        _max_slots       = 0
    ) noexcept
        : m_pool(),
          m_source(::d_mem_source_resolve(&_source)),
          m_slots_per_block(_slots_per_block),
          m_max_slots(_max_slots),
          m_ready(false)
    {}

    D_INLINE
    ~node_pool()
    {
        if (m_ready)
        {
            ::d_pool_release(&m_pool);
        }
    }

    node_pool(const node_pool&)            = delete;
    node_pool& operator=(const node_pool&) = delete;

    // acquire
    //   operation: storage for _bytes bytes at _align -- from the pool when
    // its slots fit, from the upstream source otherwise.
    D_INLINE void*
    acquire(
        mem_size _bytes,
        mem_size _align
    )
    {
        if (!m_ready)
        {
            configure(_bytes, _align);
        }

        if ( (m_ready) &&
             (_bytes <= ::d_pool_slot_size(&m_pool)) &&
             (_align <= ::d_pool_slot_align(&m_pool)) )
        {
            return ::d_pool_acquire(&m_pool);
        }

        return ::d_mem_source_allocate(&m_source, _bytes, _align);
    }

    // release
    //   operation: returns storage to wherever it came from. The pool is
    // asked whether the pointer is one of its slots, which is exact rather
    // than inferred from the size -- two different requests can share a size.
    D_INLINE void
    release(
        void*    _ptr,
        mem_size _bytes,
        mem_size _align
    ) noexcept
    {
        if ( (m_ready) &&
             (::d_pool_index_of(&m_pool, _ptr) != D_POOL_INDEX_NONE) )
        {
            ::d_pool_release_slot(&m_pool, _ptr);

            return;
        }

        ::d_mem_source_release(&m_source, _ptr, _bytes, _align);

        return;
    }

    // ---------------------------------------------------------------
    //  queries
    // ---------------------------------------------------------------

    // is_configured
    //   query: whether the slot geometry has been fixed yet. False until the
    // first allocation, which is the whole point of the class.
    D_INLINE bool
    is_configured() const noexcept
    {
        return m_ready;
    }

    D_INLINE ::d_pool_index
    size() const noexcept
    {
        return (m_ready ? ::d_pool_size(&m_pool) : 0);
    }

    D_INLINE ::d_pool_index
    capacity() const noexcept
    {
        return (m_ready ? ::d_pool_capacity(&m_pool) : 0);
    }

    D_INLINE mem_size
    slot_size() const noexcept
    {
        return (m_ready ? ::d_pool_slot_size(&m_pool) : 0);
    }

    D_INLINE mem_size
    block_count() const noexcept
    {
        return (m_ready ? ::d_pool_block_count(&m_pool) : 0);
    }

    D_INLINE mem_size
    bytes_allocated() const noexcept
    {
        return (m_ready ? ::d_pool_bytes_allocated(&m_pool) : 0);
    }

private:

    // configure
    //   helper: fixes the slot geometry from the first request seen.
    D_INLINE void
    configure(
        mem_size _bytes,
        mem_size _align
    )
    {
        ::d_pool_config config = ::d_pool_config_default(_bytes, _align);

        config.source          = m_source;
        config.slots_per_block = m_slots_per_block;
        config.max_slots       = m_max_slots;
        config.policy          = D_POOL_POLICY_FREE_LIST;

        m_ready = (::d_pool_init(&m_pool, &config) == D_MEM_OK);

        return;
    }

    ::d_pool       m_pool;
    ::d_mem_source m_source;
    ::d_pool_index m_slots_per_block;
    ::d_pool_index m_max_slots;
    bool           m_ready;
};


// pool_allocator
//   class: an Allocator over a node_pool, for node-based containers.
//   Every rebound copy shares the same node_pool, which is what makes this
// work: the container rebinds to its private node type, and THAT
// instantiation's sizeof is what configures the pool.
template<typename _Type>
class pool_allocator
{
public:

    using value_type      = _Type;
    using pointer         = _Type*;
    using const_pointer   = const _Type*;
    using reference       = _Type&;
    using const_reference = const _Type&;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    // propagate_on_container_move_assignment
    //   trait: move assignment carries the allocator, which is correct
    // because the allocator is a reference -- moving it moves the reference,
    // never the pool.
    using propagate_on_container_move_assignment = std::true_type;

    // is_always_equal
    //   trait: FALSE. Two pool_allocators are interchangeable only when they
    // name the same node_pool, so the containers must compare them. Declaring
    // this true is how memory from one pool ends up returned to another.
    using is_always_equal = std::false_type;

    // rebind
    //   trait: the C++11 spelling containers still reach for. Every rebound
    // allocator shares the SAME node_pool.
    template<typename _Other>
    struct rebind
    {
        using other = pool_allocator<_Other>;
    };

    // pool_allocator (parameterized)
    //   constructor: draws from _pool, which must outlive every container
    // holding this allocator and every copy of it.
    explicit D_INLINE
    pool_allocator(
        node_pool& _pool
    ) noexcept
        : m_pool(&_pool)
    {}

    // pool_allocator (rebinding copy)
    //   constructor: the conversion the container performs when it rebinds to
    // its node type -- and the point at which the real node size becomes
    // knowable.
    template<typename _Other>
    D_INLINE
    pool_allocator(
        const pool_allocator<_Other>& _other
    ) noexcept
        : m_pool(_other.resource())
    {}

    // allocate
    //   operation: storage for _count objects. A single object comes from the
    // pool; anything else goes to the pool's fallback source, since pool
    // slots are individually recycled and are not guaranteed adjacent.
    D_INLINE pointer
    allocate(
        size_type _count
    )
    {
        void* storage = m_pool->acquire(
            static_cast<mem_size>(_count * sizeof(_Type)),
            align_of<_Type>::value);

        if (!storage)
        {
            throw std::bad_alloc();
        }

        return static_cast<pointer>(storage);
    }

    // deallocate
    //   operation: returns the storage to whichever of the two it came from.
    D_INLINE void
    deallocate(
        pointer   _ptr,
        size_type _count
    ) noexcept
    {
        m_pool->release(static_cast<void*>(_ptr),
                        static_cast<mem_size>(_count * sizeof(_Type)),
                        align_of<_Type>::value);

        return;
    }

    // max_size
    //   query: the largest _count allocate could be asked for.
    D_INLINE size_type
    max_size() const noexcept
    {
        return (static_cast<size_type>(D_MEM_SIZE_MAX) / sizeof(_Type));
    }

    // resource
    //   query: the node_pool this allocator draws from, so a rebound copy can
    // share it.
    D_INLINE node_pool*
    resource() const noexcept
    {
        return m_pool;
    }

private:

    node_pool* m_pool;
};


// operator== / operator!=  (pool_allocator)
//   compare: two pool allocators are interchangeable exactly when they name
// the same node_pool. Anything looser would let a container deallocate into a
// pool that never vended the slot.
template<typename _Left,
         typename _Right>
D_INLINE bool
operator==(
    const pool_allocator<_Left>&  _left,
    const pool_allocator<_Right>& _right
) noexcept
{
    return (_left.resource() == _right.resource());
}

template<typename _Left,
         typename _Right>
D_INLINE bool
operator!=(
    const pool_allocator<_Left>&  _left,
    const pool_allocator<_Right>& _right
) noexcept
{
    return !(_left == _right);
}


///////////////////////////////////////////////////////////////////////////////
///                  II.   arena_allocator<T>                               ///
///////////////////////////////////////////////////////////////////////////////

// arena_allocator
//   class: an Allocator over an arena, for variable-size allocations in a
// phase-scoped workload.
//   DEALLOCATE IS A NO-OP. A std::vector over this allocator will leave every
// earlier buffer behind as it grows, and a container that is filled and
// cleared repeatedly will consume the arena without bound. That is not a
// defect being tolerated -- it is what an arena is -- and the right response
// is to reset the arena at the end of the phase, not to reach for a different
// allocator.
template<typename _Type>
class arena_allocator
{
public:

    using value_type      = _Type;
    using pointer         = _Type*;
    using const_pointer   = const _Type*;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal                        = std::false_type;

    template<typename _Other>
    struct rebind
    {
        using other = arena_allocator<_Other>;
    };

    // arena_allocator (parameterized)
    //   constructor: draws from _arena, which must outlive every container
    // holding this allocator.
    explicit D_INLINE
    arena_allocator(
        arena& _arena
    ) noexcept
        : m_arena(&_arena)
    {}

    // arena_allocator (rebinding copy)
    //   constructor: the conversion a container performs when it rebinds.
    template<typename _Other>
    D_INLINE
    arena_allocator(
        const arena_allocator<_Other>& _other
    ) noexcept
        : m_arena(_other.resource())
    {}

    // allocate
    //   operation: storage for _count objects, aligned for _Type. The product
    // is guarded against overflow by the kernel.
    D_INLINE pointer
    allocate(
        size_type _count
    )
    {
        void* storage = ::d_arena_allocate_array(
            m_arena,
            static_cast<mem_size>(_count),
            size_of<_Type>::value,
            align_of<_Type>::value);

        if (!storage)
        {
            throw std::bad_alloc();
        }

        return static_cast<pointer>(storage);
    }

    // deallocate
    //   operation: nothing. See the class comment -- an arena reclaims in
    // bulk, and pretending otherwise here would be the lie that makes the
    // trade invisible.
    D_INLINE void
    deallocate(
        pointer   _ptr,
        size_type _count
    ) noexcept
    {
        D_MEM_UNUSED(_ptr);
        D_MEM_UNUSED(_count);

        return;
    }

    // max_size
    //   query: the largest _count allocate could be asked for.
    D_INLINE size_type
    max_size() const noexcept
    {
        return (static_cast<size_type>(D_MEM_SIZE_MAX) / sizeof(_Type));
    }

    // resource
    //   query: the arena this allocator draws from.
    D_INLINE arena*
    resource() const noexcept
    {
        return m_arena;
    }

private:

    arena* m_arena;
};


// operator== / operator!=  (arena_allocator)
//   compare: interchangeable exactly when they name the same arena.
template<typename _Left,
         typename _Right>
D_INLINE bool
operator==(
    const arena_allocator<_Left>&  _left,
    const arena_allocator<_Right>& _right
) noexcept
{
    return (_left.resource() == _right.resource());
}

template<typename _Left,
         typename _Right>
D_INLINE bool
operator!=(
    const arena_allocator<_Left>&  _left,
    const arena_allocator<_Right>& _right
) noexcept
{
    return !(_left == _right);
}


///////////////////////////////////////////////////////////////////////////////
///                     III.   THE COST LAW                                 ///
///////////////////////////////////////////////////////////////////////////////
//   Both adapters are one pointer, and the assertions say so. An allocator
// that grew a second member would be copied into every rebound instantiation
// of every container using it, which is the one place in this subframework
// where a stray member multiplies.

D_STATIC_ASSERT(sizeof(pool_allocator<char>) == sizeof(void*),
                "pool_allocator must be exactly one pointer");
D_STATIC_ASSERT(sizeof(pool_allocator<double>) == sizeof(void*),
                "pool_allocator must be exactly one pointer, per instantiation");
D_STATIC_ASSERT(sizeof(arena_allocator<char>) == sizeof(void*),
                "arena_allocator must be exactly one pointer");

//   node_pool is DELIBERATELY EXEMPT from the cost law, and the exemption is
// worth stating rather than leaving to be noticed. Every other wrapper in this
// subframework derives from a C kernel and adds nothing, because everything it
// does the kernel already did. node_pool is not a wrapper: it holds a d_pool
// it has not configured yet, plus the source and geometry it will configure it
// WITH, plus a flag saying whether it has. Those members exist because the
// problem is C++-only -- a container's node type has no portable name, so its
// size cannot arrive until the container rebinds -- and a C caller, who always
// knows what it is allocating, has no use for any of it. Claiming the cost law
// here would mean either lying about the members or pushing deferred
// initialization into the kernel to serve a language feature the kernel does
// not have.
D_STATIC_ASSERT(sizeof(node_pool) > sizeof(::d_pool),
                "node_pool is a deferred-configuration holder, not a "
                "zero-cost wrapper; if this ever becomes equal, the deferral "
                "state has been lost");


NS_END  // djinterp


#endif  // DJINTERP_MEMORY_POOL_ALLOCATOR_HPP
