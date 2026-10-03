/*******************************************************************************
* djinterp [core]                                                       pool.hpp
*
* Block-structured, fixed-slot memory resource with compile-time policy
* selection, its structural classification traits, and its C++20 concept faces.
*
*   pool_resource<T, Release, Block, Growth> vends fixed-size slots in O(1) via
* acquire()/release(), threading freed slots onto an intrusive free list when
* the release policy permits.  Three orthogonal, stateless policy axes shape it:
*
*     Release : monotonic | free_list | generational   (what release() does)
*     Block   : contiguous | chunked<N>                 (backing-store layout)
*     Growth  : buffer.hpp growth policies              (grow-on-exhaustion)
*
* Chunked backing keeps pointers stable across growth; contiguous trades that
* for linear iteration.  The resource deals in raw aligned storage only - it
* never constructs or destroys objects; lifetime is the caller's / allocator's.
* It is movable, non-copyable, and NOT thread-safe (wrap it externally).
*
*   The trait layer (is_pool_resource, the stability/release classifiers, the
* accounting and allocator predicates, the resource-type extractor, and the
* aggregate pool_class / pool_allocator_class) is purely structural - it
* duck-types the protocol and the policy constants on the clean_t form; no
* tagging, no base classes.  The concept layer is a thin C++20 face over those
* traits and appears only where concepts are available.
*
* DEPENDENCIES:
*   djinterp.hpp                     - namespace macros, language gate
*   meta/type_traits.hpp            - clean_t, void_t, D_TYPE_TRAIT_TRUE
*   container/buffer/buffer.hpp     - growth policies
*
*
* path:      /inc/djinterp/core/memory/pool/pool.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.30
*                                                            revised: 2026.09.21
*******************************************************************************/

#ifndef DJINTERP_MEMORY_POOL_POOL_HPP
#define DJINTERP_MEMORY_POOL_POOL_HPP 1

// std
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../../../re_std/type_traits/bool_constant.hpp"  // portable, C++11+
#include "../../container/buffer/buffer.hpp"
// pool classification traits (extracted from this file); pool.hpp re-includes
// them so its concept faces and every consumer of pool.hpp keep the full
// surface unchanged.
#include "./pool_traits.hpp"


NS_DJINTERP


// ===========================================================================
// I.   release strategy enum
// ===========================================================================

// pool_release
//   enum: classifies how a pool handles individual slot
// deallocation.
enum class pool_release
{
    // individual release is a no-op - all memory is
    // reclaimed only when the pool is reset or destroyed
    monotonic,

    // freed slots are pushed onto an intrusive free list
    // and recycled on subsequent acquire() calls
    free_list,

    // slots are tagged with a generation counter;
    // sweep(gen) reclaims all slots from that generation
    generational
};


// ===========================================================================
// II.  Block Layout Enum
// ===========================================================================

// pool_block_layout
//   enum: classifies how a pool acquires and organizes
// its backing storage.
enum class pool_block_layout
{
    // single contiguous allocation - fast iteration,
    // but pointers may be invalidated on growth
    contiguous,

    // linked list of fixed-size blocks - pointers are
    // stable across growth, ideal for node-based
    // containers
    chunked
};


// ===========================================================================
// III. Release Policies
// ===========================================================================
// Each release policy is a stateless struct exposing
// policy constants and (where needed) a free-list or
// generation header type.  Mirrors the lock-policy and
// growth-policy pattern: no virtual functions, no
// inheritance, fully constexpr where possible.

// monotonic_release_policy
//   struct: individual release is a no-op.  All slots
// are reclaimed in bulk via reset().  Optimal when
// allocation and deallocation follow a stack discipline
// or when the entire pool is short-lived.
struct monotonic_release_policy
{
    static constexpr pool_release strategy = pool_release::monotonic;
    static constexpr bool supports_individual_release = false;
    static constexpr bool supports_generational_sweep = false;
};

// free_list_release_policy
//   struct: freed slots are threaded into an intrusive
// singly-linked free list embedded in the unused slot
// memory.  acquire() pops from the free list before
// allocating a new slot.
struct free_list_release_policy
{
    static constexpr pool_release strategy =
        pool_release::free_list;
    static constexpr bool supports_individual_release = true;
    static constexpr bool supports_generational_sweep = false;
};

// generational_release_policy
//   struct: each slot is stamped with a generation id
// at acquisition time.  sweep(gen) reclaims all slots
// belonging to that generation in one pass.  Individual
// release is also supported (falls back to free list).
struct generational_release_policy
{
    static constexpr pool_release strategy =
        pool_release::generational;
    static constexpr bool supports_individual_release = true;
    static constexpr bool supports_generational_sweep = true;

    // generation_type
    //   type: unsigned counter identifying a generation.
    using generation_type = std::uint32_t;
};


// ===========================================================================
// IV.  Block Policies
// ===========================================================================
// Block policies describe how backing memory is organized.
// They expose compile-time constants that pool_resource
// uses to select its internal storage strategy.

// contiguous_block_policy
//   struct: the pool uses a single growable allocation.
// All slots are contiguous in memory, enabling fast
// linear iteration.  Growth may invalidate all pointers.
struct contiguous_block_policy
{
    static constexpr pool_block_layout layout = pool_block_layout::contiguous;
    static constexpr bool              pointer_stable      = false;
};

// chunked_block_policy
//   struct: the pool uses a linked list of fixed-size
// blocks.  Each block holds _SlotsPerBlock slots.
// Pointers to existing slots remain valid across growth.
template<std::size_t _SlotsPerBlock = 256>
struct chunked_block_policy
{
    static constexpr pool_block_layout layout = pool_block_layout::chunked;
    static constexpr bool              pointer_stable  = true;
    static constexpr std::size_t       slots_per_block = _SlotsPerBlock;

    static_assert(_SlotsPerBlock > 0,
        "chunked_block_policy: _SlotsPerBlock must be "
        "greater than zero.");
};


// ===========================================================================
// V.   pool_block (internal)
// ===========================================================================

NS_INTERNAL

    // free_node
    //   struct: intrusive free list link.  Stored inside
    // unused slot memory when a slot is returned to the
    // pool.  Requires slot_size >= sizeof(free_node*).
    struct free_node
    {
        free_node* next;
    };

    // pool_block
    //   struct: a single contiguous block of slot storage.
    // Used by the chunked block layout.  Each block
    // owns a raw allocation of _slots_per_block slots
    // and links to the next block.
    struct pool_block
    {
        pool_block*  next;
        std::size_t  slot_count;
        // raw slot storage follows (flexible member via
        // placement at end of allocation)

        // storage
        //   returns a pointer to the first byte of slot
        // storage.
        char*
        storage() noexcept
        {
            return reinterpret_cast<char*>(this + 1);
        }

        const char*
        storage() const noexcept
        {
            return reinterpret_cast<const char*>(this + 1);
        }

        // create
        //   allocates a new pool_block capable of holding
        // _count slots of _slot_size bytes each.
        static pool_block*
        create(
            std::size_t _count,
            std::size_t _slot_size
        )
        {
            std::size_t header = sizeof(pool_block);
            std::size_t body   = _count * _slot_size;
            void*       raw    = ::operator new(
                                     header + body);

            pool_block* blk = static_cast<pool_block*>(raw);
            blk->next       = nullptr;
            blk->slot_count = _count;

            return blk;
        }

        // destroy
        //   deallocates a pool_block.
        static void
        destroy(pool_block* _blk) noexcept
        {
            ::operator delete(
                static_cast<void*>(_blk));

            return;
        }
    };

    // slot_metrics
    //   struct: compile-time slot sizing for a given
    // element type.  The slot must be large enough to
    // hold either the element or a free_node pointer,
    // and aligned to the strictest of both.
    template<typename _Type>
    struct slot_metrics
    {
        static constexpr std::size_t type_size  = sizeof(_Type);
        static constexpr std::size_t type_align = alignof(_Type);
        static constexpr std::size_t link_size  = sizeof(free_node);
        static constexpr std::size_t link_align = alignof(free_node);

        // slot_align
        //   the alignment of each slot.
        static constexpr std::size_t slot_align =
            (type_align > link_align)
                ? type_align
                : link_align;

        // raw_size
        //   the unpadded maximum of type and link sizes.
        static constexpr std::size_t raw_size =
            (type_size > link_size)
                ? type_size
                : link_size;

        // slot_size
        //   the aligned slot size.  Rounded up to the
        // nearest multiple of slot_align.
        static constexpr std::size_t slot_size =
            ((raw_size + slot_align - 1) / slot_align)
                * slot_align;
    };

NS_END  // internal


// ===========================================================================
// VI.  pool_resource
// ===========================================================================
// The core pool class.  Owns backing storage, manages a
// free list (when the release policy permits), and provides
// O(1) acquire/release for fixed-size slots.
//
// The pool does not construct or destroy objects - it deals
// in raw aligned storage only.  Object lifecycle is the
// responsibility of the allocator or the user.
//
// Template parameters:
//   _Type          - element type (determines slot size)
//   _ReleasePolicy - how individual release is handled
//   _BlockPolicy   - contiguous vs. chunked storage
//   _GrowthPolicy  - how much to grow when exhausted

NS_INTERNAL

    // no_generation_state
    //   empty stand-in for a pool whose release policy is not generational.
    struct no_generation_state
    {};

    // generation_state_of
    //   defers the generation_type lookup.  std::conditional_t NAMES BOTH ARMS,
    // so writing
    //
    //     conditional_t<RP::supports_generational_sweep,
    //                   typename RP::generation_type,
    //                   no_generation_state>
    //
    // hard-requires RP::generation_type even when the flag is false -- and
    // monotonic_release_policy and free_list_release_policy do not have one.
    // That made default_pool_resource, monotonic_pool_resource and
    // flat_pool_resource ALL fail to instantiate: every pool in the header
    // except the generational one. A specialized helper only ever names the arm
    // it takes. (This is the fix the FIXME at the old site described.)
    template<typename _RP,
             bool = _RP::supports_generational_sweep>
    struct generation_state_of
    {
        using type = no_generation_state;
    };

    template<typename _RP>
    struct generation_state_of<_RP, true>
    {
        using type = typename _RP::generation_type;
    };

NS_END  // internal


template<typename _Type,
         typename _ReleasePolicy = free_list_release_policy,
         typename _BlockPolicy   = chunked_block_policy<>,
         typename _GrowthPolicy  = exponential_growth_policy>
class pool_resource
{
    // --- compile-time slot geometry ---
    using metrics = internal::slot_metrics<_Type>;

public:
    // --- policy types ---
    using release_policy = _ReleasePolicy;
    using block_policy   = _BlockPolicy;
    using growth_policy  = _GrowthPolicy;
    using value_type     = _Type;
    using size_type      = std::size_t;

    // --- policy constants ---
    static constexpr pool_release      release_strategy =
        _ReleasePolicy::strategy;
    static constexpr pool_block_layout  block_layout     =
        _BlockPolicy::layout;
    static constexpr bool pointer_stable =
        _BlockPolicy::pointer_stable;
    static constexpr bool supports_individual_release =
        _ReleasePolicy::supports_individual_release;
    static constexpr bool supports_generational_sweep =
        _ReleasePolicy::supports_generational_sweep;

    // --- slot geometry ---
    static constexpr size_type slot_size  = metrics::slot_size;
    static constexpr size_type slot_align = metrics::slot_align;

    // --------------------------------------------------------
    //  construction / destruction
    // --------------------------------------------------------

    // pool_resource (default)
    //   constructs an empty pool with no pre-allocated
    // storage.
    pool_resource() noexcept
        : m_free_head (nullptr),
          m_size      (0),
          m_capacity  (0)
    {
        init_block_state();
    }

    // pool_resource (with initial capacity)
    //   constructs a pool and pre-allocates storage for
    // at least _initial_capacity elements.
    explicit
    pool_resource(
        size_type _initial_capacity
    )
        : m_free_head (nullptr),
          m_size      (0),
          m_capacity  (0)
    {
        init_block_state();

        if (_initial_capacity > 0)
        {
            grow(_initial_capacity);
        }
    }

    // ~pool_resource
    //   releases all backing storage.  Does NOT call
    // destructors on live elements - the user or
    // allocator is responsible for element lifecycle.
    ~pool_resource()
    {
        release_all_blocks();
    }

    // non-copyable
    pool_resource(const pool_resource&)            = delete;
    pool_resource& operator=(const pool_resource&) = delete;

    // movable
    pool_resource(
        pool_resource&& _other
    ) noexcept
        : m_free_head (_other.m_free_head),
          m_size      (_other.m_size),
          m_capacity  (_other.m_capacity)
    {
        move_block_state(_other);
        _other.m_free_head = nullptr;
        _other.m_size      = 0;
        _other.m_capacity  = 0;
        _other.init_block_state();
    }

    pool_resource&
    operator=(
        pool_resource&& _other
    ) noexcept
    {
        if (this != &_other)
        {
            release_all_blocks();

            m_free_head = _other.m_free_head;
            m_size      = _other.m_size;
            m_capacity  = _other.m_capacity;
            move_block_state(_other);

            _other.m_free_head = nullptr;
            _other.m_size      = 0;
            _other.m_capacity  = 0;
            _other.init_block_state();
        }

        return *this;
    }

    // --------------------------------------------------------
    //  acquire / release
    // --------------------------------------------------------

    // acquire
    //   returns a pointer to an uninitialized slot of
    // slot_size bytes, suitably aligned for _Type.
    // Grows the pool if no free slots are available.
    // Returns nullptr on allocation failure.
    void*
    acquire()
    {
        // 1. try the free list (if release policy uses one)
        if (_ReleasePolicy::supports_individual_release)
        {
            if (m_free_head)
            {
                void* slot = static_cast<void*>(m_free_head);
                m_free_head = m_free_head->next;
                ++m_size;

                return slot;
            }
        }

        // 2. try the bump region in the current block
        void* slot = try_bump_acquire();

        if (slot)
        {
            ++m_size;

            return slot;
        }

        // 3. grow and retry
        if (!_GrowthPolicy::can_grow)
        {
            return nullptr;
        }
        else
        {
            size_type new_count = _GrowthPolicy::compute(
                m_capacity,
                m_capacity + 1);

            size_type added = new_count - m_capacity;

            if (added == 0)
            {
                added = 1;
            }

            if (!grow(added))
            {
                return nullptr;
            }

            slot = try_bump_acquire();

            if (slot)
            {
                ++m_size;
            }

            return slot;
        }
    }

    // release
    //   returns a previously acquired slot to the pool.
    // Behavior depends on the release policy:
    //   monotonic   - no-op (slot reclaimed on reset)
    //   free_list   - slot is pushed onto the free list
    //   generational - slot pushed onto free list
    void
    release(
        void* _ptr
    ) noexcept
    {
        if (!_ptr)
        {
            return;
        }

        if (_ReleasePolicy::supports_individual_release)
        {
            auto* node = static_cast<internal::free_node*>(_ptr);
            node->next  = m_free_head;
            m_free_head = node;
        }

        // monotonic: no-op - slot memory is not reclaimed
        // until reset() or destruction.

        --m_size;

        return;
    }

    // --------------------------------------------------------
    //  generational operations
    // --------------------------------------------------------

    // current_generation
    //   returns the current generation counter.
    // Only available with generational release policy.
    template<typename _RP = _ReleasePolicy>
    std::enable_if_t<_RP::supports_generational_sweep,
                     typename _RP::generation_type>
    current_generation() const noexcept
    {
        return m_generation;
    }

    // advance_generation
    //   increments the generation counter and returns
    // the new value.  Subsequent acquire() calls stamp
    // slots with this generation.
    template<typename _RP = _ReleasePolicy>
    std::enable_if_t<_RP::supports_generational_sweep,
                     typename _RP::generation_type>
    advance_generation() noexcept
    {
        return ++m_generation;
    }

    // --------------------------------------------------------
    //  capacity
    // --------------------------------------------------------

    // size
    //   returns the number of currently acquired (live)
    // slots.
    size_type
    size() const noexcept
    {
        return m_size;
    }

    // capacity
    //   returns the total number of slots (live + free
    // + bump region).
    size_type
    capacity() const noexcept
    {
        return m_capacity;
    }

    // empty
    //   returns true if no slots are currently acquired.
    bool
    empty() const noexcept
    {
        return (m_size == 0);
    }

    // reserve
    //   ensures the pool has capacity for at least _n
    // total slots.  Does nothing if capacity is already
    // sufficient.  Returns true on success.
    bool
    reserve(size_type _n)
    {
        if (_n <= m_capacity)
        {
            return true;
        }

        return grow(_n - m_capacity);
    }

    // --------------------------------------------------------
    //  bulk operations
    // --------------------------------------------------------

    // reset
    //   reclaims all slots without calling destructors.
    // The pool returns to empty state but retains its
    // backing storage for reuse.
    void
    reset() noexcept
    {
        m_free_head = nullptr;
        m_size      = 0;
        reset_bump_state();

        return;
    }

    // clear
    //   reclaims all slots and releases all backing
    // storage.  The pool returns to its default-
    // constructed state.
    void
    clear() noexcept
    {
        release_all_blocks();
        m_free_head = nullptr;
        m_size      = 0;
        m_capacity  = 0;
        init_block_state();

        return;
    }

    // --------------------------------------------------------
    //  slot geometry queries (constexpr)
    // --------------------------------------------------------

    // bytes_per_slot
    //   returns the size of each slot in bytes (including
    // alignment padding).
    static constexpr size_type
    bytes_per_slot() noexcept
    {
        return slot_size;
    }

    // alignment
    //   returns the alignment of each slot in bytes.
    static constexpr size_type
    alignment() noexcept
    {
        return slot_align;
    }

    // --------------------------------------------------------
    //  memory accounting
    // --------------------------------------------------------

    // bytes_allocated
    //   returns the total bytes of backing storage
    // currently allocated by this pool.
    size_type
    bytes_allocated() const noexcept
    {
        return m_capacity * slot_size + block_overhead();
    }

    // bytes_in_use
    //   returns the number of bytes occupied by live
    // (acquired) slots.
    size_type
    bytes_in_use() const noexcept
    {
        return m_size * slot_size;
    }

    // utilization
    //   returns the fraction of capacity currently in
    // use, as a value in [0.0, 1.0].
    double
    utilization() const noexcept
    {
        if (m_capacity == 0)
        {
            return 0.0;
        }

        return static_cast<double>(m_size)
             / static_cast<double>(m_capacity);
    }


// ============================================================
// PRIVATE - contiguous block layout
// ============================================================
private:

    // --- contiguous block state ---
    // Used when _BlockPolicy::layout == contiguous.
    // One growable allocation, bump pointer at the end.

    struct contiguous_state
    {
        char*       data;
        size_type   bump_offset;    // next uninitialized slot
    };

    // --- chunked block state ---
    // Used when _BlockPolicy::layout == chunked.
    // Linked list of fixed-size blocks, bump within the
    // most recent block.

    struct chunked_state
    {
        internal::pool_block*  head;
        internal::pool_block*  current;       // bump cursor block
        internal::pool_block*  last;          // true end of chain
        size_type              block_count;
        size_type              bump_offset;   // within current block
    };

    // --- unified state (only one is active) ---
    // Selected at compile time by the block policy.

    using block_state = std::conditional_t<
        _BlockPolicy::layout == pool_block_layout::contiguous,
        contiguous_state,
        chunked_state
    >;

    // --- generational state ---
    // Only present when the release policy is generational.

    // generation_state
    //   the generational bookkeeping, or an empty struct when the release
    // policy is not generational.  Deferred through internal::
    // generation_state_of so the non-taken arm is never named -- see the note
    // there.  Written with std::conditional_t, this line hard-required
    // generation_type from every policy and no ordinary pool would instantiate.
    using generation_state =
        typename internal::generation_state_of<_ReleasePolicy>::type;

    // --------------------------------------------------------
    //  init_block_state
    // --------------------------------------------------------

    // init_block_state_impl / init_generation
    //   TAG DISPATCH, not if-constexpr.  These two branches are LOAD-BEARING:
    // the untaken arm names members that exist only on the OTHER state struct
    // (contiguous_state has no .head; chunked_state has no .data; a
    // non-generational pool's generation_state is an EMPTY struct), so a plain
    // 'if' does not merely run dead code -- it fails to compile.
    //
    //   Tag dispatch gets the same guarantee without the C++17 keyword: an
    // overload of a class-template member that is never selected is never
    // instantiated, and so is never checked.  Available since C++11, which is
    // what lets this header build below C++17 again.
    void
    init_block_state_impl(
        re_std::bool_constant<true>   /*contiguous*/
    ) noexcept
    {
        m_blocks.data        = nullptr;
        m_blocks.bump_offset = 0;

        return;
    }

    void
    init_block_state_impl(
        re_std::bool_constant<false>  /*chunked*/
    ) noexcept
    {
        m_blocks.head        = nullptr;
        m_blocks.current     = nullptr;
        m_blocks.last        = nullptr;
        m_blocks.block_count = 0;
        m_blocks.bump_offset = 0;

        return;
    }

    void
    init_generation(
        re_std::bool_constant<true>   /*generational*/
    ) noexcept
    {
        m_generation = 0;

        return;
    }

    void
    init_generation(
        re_std::bool_constant<false>  /*not generational*/
    ) noexcept
    {
        return;
    }

    void
    init_block_state() noexcept
    {
        init_block_state_impl(
            re_std::bool_constant<
                _BlockPolicy::layout ==
                pool_block_layout::contiguous>());

        init_generation(
            re_std::bool_constant<
                _ReleasePolicy::supports_generational_sweep>());

        return;
    }

    // --------------------------------------------------------
    //  move_block_state
    // --------------------------------------------------------

    void
    move_block_state(pool_resource& _other) noexcept
    {
        m_blocks = _other.m_blocks;

        return;
    }

    // --------------------------------------------------------
    //  try_bump_acquire  (contiguous)
    // --------------------------------------------------------

    template<typename _BP = _BlockPolicy>
    std::enable_if_t<
        _BP::layout == pool_block_layout::contiguous,
        void*>
    try_bump_acquire() noexcept
    {
        if ( (!m_blocks.data) ||
             (m_blocks.bump_offset >= m_capacity) )
        {
            return nullptr;
        }

        void* slot = m_blocks.data
                   + (m_blocks.bump_offset * slot_size);
        ++m_blocks.bump_offset;

        return slot;
    }

    // --------------------------------------------------------
    //  try_bump_acquire  (chunked)
    // --------------------------------------------------------

    template<typename _BP = _BlockPolicy>
    std::enable_if_t<
        _BP::layout == pool_block_layout::chunked,
        void*>
    try_bump_acquire() noexcept
    {
        if (!m_blocks.current)
        {
            return nullptr;
        }

        // if current block is full, advance to next
        // existing block (available after reset)
        if (m_blocks.bump_offset >= m_blocks.current->slot_count)
        {
            if (!m_blocks.current->next)
            {
                return nullptr;
            }

            m_blocks.current     = m_blocks.current->next;
            m_blocks.bump_offset = 0;
        }

        void* slot = m_blocks.current->storage()
                   + (m_blocks.bump_offset * slot_size);
        ++m_blocks.bump_offset;

        return slot;
    }

    // --------------------------------------------------------
    //  grow  (contiguous)
    // --------------------------------------------------------

    template<typename _BP = _BlockPolicy>
    std::enable_if_t<
        _BP::layout == pool_block_layout::contiguous,
        bool>
    grow(size_type _additional)
    {
        size_type new_cap = m_capacity + _additional;
        size_type new_bytes = new_cap * slot_size;

        char* new_data = static_cast<char*>(
            ::operator new(new_bytes, std::nothrow));

        if (!new_data)
        {
            return false;
        }

        // copy existing live data
        if ( (m_blocks.data) && (m_blocks.bump_offset > 0) )
        {
            std::memcpy(new_data,
                        m_blocks.data,
                        m_blocks.bump_offset * slot_size);
        }

        if (m_blocks.data)
        {
            // rewrite free list pointers - they are
            // now in the new allocation
            if (_ReleasePolicy::supports_individual_release)
            {
                rebase_free_list(m_blocks.data, new_data);
            }

            ::operator delete(
                static_cast<void*>(m_blocks.data));
        }

        m_blocks.data = new_data;
        m_capacity    = new_cap;

        return true;
    }

    // --------------------------------------------------------
    //  grow  (chunked)
    // --------------------------------------------------------

    template<typename _BP = _BlockPolicy>
    std::enable_if_t<
        _BP::layout == pool_block_layout::chunked,
        bool>
    grow(size_type _additional)
    {
        // determine how many slots per new block
        size_type spb = _BlockPolicy::slots_per_block;

        while (_additional > 0)
        {
            size_type count = (spb < _additional)
                ? spb
                : _additional;

            auto* blk = internal::pool_block::create(
                            count, slot_size);

            if (!blk)
            {
                return false;
            }

            // link into the chain
            if (m_blocks.last)
            {
                m_blocks.last->next = blk;
            }
            else
            {
                m_blocks.head = blk;
            }

            m_blocks.last = blk;

            // if no current bump block, start here
            if (!m_blocks.current)
            {
                m_blocks.current     = blk;
                m_blocks.bump_offset = 0;
            }

            m_blocks.block_count++;
            m_capacity  += count;
            _additional -= count;
        }

        return true;
    }

    // --------------------------------------------------------
    //  reset_bump_state
    // --------------------------------------------------------

    void
    reset_bump_state() noexcept
    {
        if (_BlockPolicy::layout ==
                      pool_block_layout::contiguous)
        {
            m_blocks.bump_offset = 0;
        }
        else
        {
            // reset bump to beginning of first block
            m_blocks.current     = m_blocks.head;
            m_blocks.bump_offset = 0;
        }

        return;
    }

    // --------------------------------------------------------
    //  release_all_blocks  (contiguous)
    // --------------------------------------------------------

    template<typename _BP = _BlockPolicy>
    std::enable_if_t<
        _BP::layout == pool_block_layout::contiguous>
    release_all_blocks() noexcept
    {
        if (m_blocks.data)
        {
            ::operator delete(
                static_cast<void*>(m_blocks.data));
            m_blocks.data = nullptr;
        }

        m_blocks.bump_offset = 0;

        return;
    }

    // --------------------------------------------------------
    //  release_all_blocks  (chunked)
    // --------------------------------------------------------

    template<typename _BP = _BlockPolicy>
    std::enable_if_t<
        _BP::layout == pool_block_layout::chunked>
    release_all_blocks() noexcept
    {
        auto* blk = m_blocks.head;

        while (blk)
        {
            auto* next = blk->next;
            internal::pool_block::destroy(blk);
            blk = next;
        }

        m_blocks.head        = nullptr;
        m_blocks.current     = nullptr;
        m_blocks.last        = nullptr;
        m_blocks.block_count = 0;
        m_blocks.bump_offset = 0;

        return;
    }

    // --------------------------------------------------------
    //  rebase_free_list  (contiguous only)
    // --------------------------------------------------------
    //   When the contiguous block is reallocated, free list
    // pointers refer to the old allocation.  This rewrites
    // them to point into the new allocation.

    void
    rebase_free_list(
        char* _old_base,
        char* _new_base
    ) noexcept
    {
        internal::free_node** cursor = &m_free_head;

        while (*cursor)
        {
            char* old_addr =
                reinterpret_cast<char*>(*cursor);
            std::ptrdiff_t offset = old_addr - _old_base;
            char* new_addr = _new_base + offset;

            *cursor = reinterpret_cast<
                          internal::free_node*>(new_addr);
            cursor  = &((*cursor)->next);
        }

        return;
    }

    // --------------------------------------------------------
    //  block_overhead
    // --------------------------------------------------------

    size_type
    block_overhead() const noexcept
    {
        if (_BlockPolicy::layout ==
                      pool_block_layout::contiguous)
        {
            return 0;
        }
        else
        {
            return m_blocks.block_count * sizeof(internal::pool_block);
        }
    }

    // --------------------------------------------------------
    //  data members
    // --------------------------------------------------------

    internal::free_node*  m_free_head;
    size_type             m_size;
    size_type             m_capacity;
    block_state           m_blocks;

    // generational counter - only present when needed.
    // Uses [[no_unique_address]] to avoid overhead when
    // the generation state is an empty struct.
    // [[no_unique_address]] avoids overhead when
    // generation_state is empty (non-generational pools).
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    [[no_unique_address]] generation_state m_generation;
#else
    generation_state m_generation;
#endif
};


// ===========================================================================
// VII. Policy Selection
// ===========================================================================
// Compile-time selection of release and block policies by
// enum value.

template<pool_release _Strategy>
struct select_release_policy;

template<>
struct select_release_policy<pool_release::monotonic>
{
    using type = monotonic_release_policy;
};

template<>
struct select_release_policy<pool_release::free_list>
{
    using type = free_list_release_policy;
};

template<>
struct select_release_policy<pool_release::generational>
{
    using type = generational_release_policy;
};

template<pool_release _Strategy>
using select_release_policy_t =
    typename select_release_policy<_Strategy>::type;


template<pool_block_layout _Layout>
struct select_block_policy;

template<>
struct select_block_policy<pool_block_layout::contiguous>
{
    using type = contiguous_block_policy;
};

template<>
struct select_block_policy<pool_block_layout::chunked>
{
    using type = chunked_block_policy<>;
};

template<pool_block_layout _Layout>
using select_block_policy_t = typename select_block_policy<_Layout>::type;


// ===========================================================================
// VIII. Default Aliases
// ===========================================================================
// Sensible defaults for common use cases.

// default_pool_resource
//   alias: free-list release, chunked blocks (pointer-
// stable), exponential growth.  The safest general-
// purpose default.
template<typename _Type>
using default_pool_resource = pool_resource<_Type,
                                            free_list_release_policy,
                                            chunked_block_policy<>,
                                            exponential_growth_policy>;

// monotonic_pool_resource
//   alias: monotonic release, chunked blocks, exponential
// growth.  Fastest acquire - no free list overhead.
// All memory released on reset/destruction.
template<typename _Type>
using monotonic_pool_resource = pool_resource<_Type,
                                              monotonic_release_policy,
                                              chunked_block_policy<>,
                                              exponential_growth_policy>;

// flat_pool_resource
//   alias: free-list release, contiguous block, exponential
// growth.  Enables linear iteration over slots but may
// invalidate pointers on growth.
template<typename _Type>
using flat_pool_resource = pool_resource<_Type,
                                         free_list_release_policy,
                                         contiguous_block_policy,
                                         exponential_growth_policy>;


// ===========================================================================
// C++20 concept faces  (present only where concepts are available)
// ===========================================================================
#if defined(__cpp_concepts) && (__cpp_concepts >= 201907L)

// ===========================================================================
// XV.   Pool Resource Concepts
// ===========================================================================

// pool_resource
//   concept: constrains types satisfying the minimum pool resource protocol.
template<typename _Type>
concept pool_resource_c = is_pool_resource_v<clean_t<_Type>>;

// non_pool_resource
//   concept: constrains types that do not satisfy the pool resource protocol.
template<typename _Type>
concept non_pool_resource = !pool_resource_c<_Type>;

// acquiring_pool
//   concept: constrains pool-like types exposing acquire().
template<typename _Type>
concept acquiring_pool =
    has_acquire_v<clean_t<_Type>>;

// releasing_pool
//   concept: constrains pool-like types exposing release(void*).
template<typename _Type>
concept releasing_pool =
    has_release_v<clean_t<_Type>>;

// sized_pool
//   concept: constrains pool-like types exposing size().
template<typename _Type>
concept sized_pool = has_size_accessor_v<clean_t<_Type>>;

// typed_pool
//   concept: constrains pool-like types exposing value_type.
template<typename _Type>
concept typed_pool = has_value_type_v<clean_t<_Type>>;

// classified_pool
//   concept: constrains types recognized by the pool trait layer.
template<typename _Type>
concept classified_pool =
    ( pool_resource_c<_Type>                     ||
      is_pointer_stable_pool_v<clean_t<_Type>> ||
      supports_individual_release_v<clean_t<_Type>> ||
      has_generational_sweep_v<clean_t<_Type>> ||
      has_memory_accounting_v<clean_t<_Type>> );


// ===========================================================================
// XVI.  Pool Stability and Release Concepts
// ===========================================================================

// pointer_stable_pool
//   concept: constrains pools guaranteeing pointer stability across growth.
template<typename _Type>
concept pointer_stable_pool =
    is_pointer_stable_pool_v<clean_t<_Type>>;

// relocatable_pool
//   concept: constrains pools that do not guarantee pointer stability.
template<typename _Type>
concept relocatable_pool =
    ( pool_resource_c<_Type> &&
      !pointer_stable_pool<_Type> );

// individually_releasing_pool
//   concept: constrains pools supporting per-slot release.
template<typename _Type>
concept individually_releasing_pool =
    supports_individual_release_v<clean_t<_Type>>;

// monotonic_pool
//   concept: constrains pools reclaiming memory only on reset or destruction.
template<typename _Type>
concept monotonic_pool =
    is_monotonic_pool_v<clean_t<_Type>>;

// generational_pool
//   concept: constrains pools supporting generational reclamation.
template<typename _Type>
concept generational_pool =
    has_generational_sweep_v<clean_t<_Type>>;


// ===========================================================================
// XVII. Pool Capability Concepts
// ===========================================================================

// resettable_pool
//   concept: constrains pools exposing reset().
template<typename _Type>
concept resettable_pool =
    has_pool_reset_v<clean_t<_Type>>;

// reservable_pool
//   concept: constrains pools exposing reserve(size_t).
template<typename _Type>
concept reservable_pool =
    has_pool_reserve_v<clean_t<_Type>>;

// slot_sized_pool
//   concept: constrains pools exposing bytes_per_slot().
template<typename _Type>
concept slot_sized_pool =
    has_bytes_per_slot_v<clean_t<_Type>>;

// slot_aligned_pool
//   concept: constrains pools exposing alignment().
template<typename _Type>
concept slot_aligned_pool =
    has_slot_alignment_v<clean_t<_Type>>;

// generation_tracked_pool
//   concept: constrains pools exposing current_generation().
template<typename _Type>
concept generation_tracked_pool =
    has_current_generation_v<clean_t<_Type>>;

// generation_advancing_pool
//   concept: constrains pools exposing advance_generation().
template<typename _Type>
concept generation_advancing_pool =
    has_advance_generation_v<clean_t<_Type>>;

// memory_accounting_pool
//   concept: constrains pools exposing byte-level accounting.
template<typename _Type>
concept memory_accounting_pool =
    has_memory_accounting_v<clean_t<_Type>>;

// bytes_allocated_pool
//   concept: constrains pools exposing bytes_allocated().
template<typename _Type>
concept bytes_allocated_pool =
    has_bytes_allocated_v<clean_t<_Type>>;

// bytes_in_use_pool
//   concept: constrains pools exposing bytes_in_use().
template<typename _Type>
concept bytes_in_use_pool =
    has_bytes_in_use_v<clean_t<_Type>>;

// utilization_reporting_pool
//   concept: constrains pools exposing utilization().
template<typename _Type>
concept utilization_reporting_pool =
    has_utilization_v<clean_t<_Type>>;


// ===========================================================================
// XVIII.  Allocator and Container Concepts
// ===========================================================================

// pool_allocator
//   concept: constrains allocators backed by a pool resource.
template<typename _Type>
concept pool_allocator =
    is_pool_allocator_v<clean_t<_Type>>;

// non_pool_allocator
//   concept: constrains allocators not backed by a pool resource.
template<typename _Type>
concept non_pool_allocator =
    !pool_allocator<_Type>;

// resource_exposing_pool_allocator
//   concept: constrains pool allocators exposing resource().
template<typename _Type>
concept resource_exposing_pool_allocator =
    has_resource_method_v<clean_t<_Type>>;

// pool_backed_container
//   concept: constrains containers whose allocator is pool-backed.
template<typename _Type>
concept pool_backed_container =
    is_pool_backed_container_v<clean_t<_Type>>;

// non_pool_backed_container
//   concept: constrains containers whose allocator is not pool-backed.
template<typename _Type>
concept non_pool_backed_container =
    !pool_backed_container<_Type>;

// allocator_aware_pool_container
//   concept: constrains pool-backed containers exposing allocator_type.
template<typename _Type>
concept allocator_aware_pool_container =
    ( pool_backed_container<_Type> &&
      has_allocator_type_v<clean_t<_Type>> );


// ===========================================================================
// XIX.   Resource Extraction Concepts
// ===========================================================================

// pool_allocator_with_resource
//   concept: constrains pool allocators whose resource type can be extracted.
template<typename _Type>
concept pool_allocator_with_resource =
    ( pool_allocator<_Type> &&
      !std::is_void_v<pool_resource_type_t<clean_t<_Type>>> );

// stable_pool_allocator
//   concept: constrains pool allocators backed by pointer-stable pools.
template<typename _Type>
concept stable_pool_allocator =
    ( pool_allocator_with_resource<_Type> &&
      is_pointer_stable_pool_v<
          pool_resource_type_t<clean_t<_Type>>> );

// monotonic_pool_allocator
//   concept: constrains pool allocators backed by monotonic pools.
template<typename _Type>
concept monotonic_pool_allocator =
    ( pool_allocator_with_resource<_Type> &&
      is_monotonic_pool_v<
          pool_resource_type_t<clean_t<_Type>>> );

// generational_pool_allocator
//   concept: constrains pool allocators backed by generational pools.
template<typename _Type>
concept generational_pool_allocator =
    ( pool_allocator_with_resource<_Type> &&
      has_generational_sweep_v<
          pool_resource_type_t<clean_t<_Type>>> );

#endif  // __cpp_concepts


NS_END  // djinterp


#endif  // DJINTERP_MEMORY_POOL_POOL_HPP
