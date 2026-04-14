/******************************************************************************
* djinterp [container]                                               pool.hpp
*
* Memory pool resource for the djinterp container framework.
*   A pool is a block-structured, fixed-slot-size memory resource that
* provides O(1) allocation and (policy-dependent) O(1) deallocation of
* uniform-sized elements.  Pools are the engine behind pool_allocator
* and can serve as the underlying resource for any node-based or
* slot-based container.
*
*   The design is policy-based along three orthogonal axes:
*
*     RELEASE POLICY — what happens when a slot is returned:
*       monotonic_release_policy     — individual release is a no-op;
*                                      all slots reclaimed on reset()
*       free_list_release_policy     — freed slots are threaded into an
*                                      intrusive free list for O(1) reuse
*       generational_release_policy  — slots are grouped by generation;
*                                      entire generations can be swept
*
*     BLOCK POLICY — how the pool acquires backing memory:
*       contiguous_block_policy      — single growable allocation; may
*                                      invalidate pointers on growth
*       chunked_block_policy         — linked list of fixed-size blocks;
*                                      pointers are stable across growth
*
*     GROWTH POLICY — how much new memory to acquire when exhausted:
*       (reuses buffer.hpp growth policies: fixed, linear, exponential,
*        page-aligned)
*
* THREAD SAFETY:
*   pool_resource is NOT thread-safe.  External synchronization or a
*   threadsafe wrapper (using the lock policies from threadsafe.hpp)
*   is required for concurrent access.
*
* DEPENDENCIES:
*   djinterp.hpp   — namespace macros, clean_t
*   buffer.hpp     — growth policies
*   dmemory.h      — d_memset (optional, for debug zeroing)
*
* TABLE OF CONTENTS
* =================
* I.    Release Strategy Enum
* II.   Block Layout Enum
* III.  Release Policies
* IV.   Block Policies
* V.    pool_block (internal)
* VI.   pool_resource
* VII.  Policy Selection
* VIII. Default Aliases
*
*
* path:      /inc/container/pool/pool.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.03.30
******************************************************************************/

#ifndef DJINTERP_CONTAINER_POOL_
#define DJINTERP_CONTAINER_POOL_ 1

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include "../djinterp.hpp"
#include "../buffer.hpp"


NS_DJINTERP
NS_CONTAINER


// =============================================================================
// I.   Release Strategy Enum
// =============================================================================

// pool_release
//   enum: classifies how a pool handles individual slot
// deallocation.
enum class pool_release
{
    // individual release is a no-op — all memory is
    // reclaimed only when the pool is reset or destroyed
    monotonic,

    // freed slots are pushed onto an intrusive free list
    // and recycled on subsequent acquire() calls
    free_list,

    // slots are tagged with a generation counter;
    // sweep(gen) reclaims all slots from that generation
    generational
};


// =============================================================================
// II.  Block Layout Enum
// =============================================================================

// pool_block_layout
//   enum: classifies how a pool acquires and organizes
// its backing storage.
enum class pool_block_layout
{
    // single contiguous allocation — fast iteration,
    // but pointers may be invalidated on growth
    contiguous,

    // linked list of fixed-size blocks — pointers are
    // stable across growth, ideal for node-based
    // containers
    chunked
};


// =============================================================================
// III. Release Policies
// =============================================================================
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
    static constexpr pool_release strategy =
        pool_release::monotonic;
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


// =============================================================================
// IV.  Block Policies
// =============================================================================
// Block policies describe how backing memory is organized.
// They expose compile-time constants that pool_resource
// uses to select its internal storage strategy.

// contiguous_block_policy
//   struct: the pool uses a single growable allocation.
// All slots are contiguous in memory, enabling fast
// linear iteration.  Growth may invalidate all pointers.
struct contiguous_block_policy
{
    static constexpr pool_block_layout layout =
        pool_block_layout::contiguous;
    static constexpr bool pointer_stable = false;
};

// chunked_block_policy
//   struct: the pool uses a linked list of fixed-size
// blocks.  Each block holds _SlotsPerBlock slots.
// Pointers to existing slots remain valid across growth.
template<std::size_t _SlotsPerBlock = 256>
struct chunked_block_policy
{
    static constexpr pool_block_layout layout =
        pool_block_layout::chunked;
    static constexpr bool        pointer_stable  = true;
    static constexpr std::size_t slots_per_block = _SlotsPerBlock;

    static_assert(_SlotsPerBlock > 0,
        "chunked_block_policy: _SlotsPerBlock must be "
        "greater than zero.");
};


// =============================================================================
// V.   pool_block (internal)
// =============================================================================

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
        create(std::size_t _count,
               std::size_t _slot_size)
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


// =============================================================================
// VI.  pool_resource
// =============================================================================
// The core pool class.  Owns backing storage, manages a
// free list (when the release policy permits), and provides
// O(1) acquire/release for fixed-size slots.
//
// The pool does not construct or destroy objects — it deals
// in raw aligned storage only.  Object lifecycle is the
// responsibility of the allocator or the user.
//
// Template parameters:
//   _Type          — element type (determines slot size)
//   _ReleasePolicy — how individual release is handled
//   _BlockPolicy   — contiguous vs. chunked storage
//   _GrowthPolicy  — how much to grow when exhausted

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
    pool_resource(size_type _initial_capacity)
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
    // destructors on live elements — the user or
    // allocator is responsible for element lifecycle.
    ~pool_resource()
    {
        release_all_blocks();
    }

    // non-copyable
    pool_resource(const pool_resource&)            = delete;
    pool_resource& operator=(const pool_resource&) = delete;

    // movable
    pool_resource(pool_resource&& _other) noexcept
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
    operator=(pool_resource&& _other) noexcept
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
        if constexpr (_ReleasePolicy::supports_individual_release)
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
        if constexpr (!_GrowthPolicy::can_grow)
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
    //   monotonic   — no-op (slot reclaimed on reset)
    //   free_list   — slot is pushed onto the free list
    //   generational — slot pushed onto free list
    void
    release(void* _ptr) noexcept
    {
        if (!_ptr)
        {
            return;
        }

        if constexpr (_ReleasePolicy::supports_individual_release)
        {
            auto* node = static_cast<internal::free_node*>(_ptr);
            node->next  = m_free_head;
            m_free_head = node;
        }

        // monotonic: no-op — slot memory is not reclaimed
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
// PRIVATE — contiguous block layout
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

    struct no_generation_state
    {};

    using generation_state = std::conditional_t<
        _ReleasePolicy::supports_generational_sweep,
        typename _ReleasePolicy::generation_type,
        no_generation_state
    >;

    // --------------------------------------------------------
    //  init_block_state
    // --------------------------------------------------------

    void
    init_block_state() noexcept
    {
        if constexpr (_BlockPolicy::layout ==
                      pool_block_layout::contiguous)
        {
            m_blocks.data        = nullptr;
            m_blocks.bump_offset = 0;
        }
        else
        {
            m_blocks.head        = nullptr;
            m_blocks.current     = nullptr;
            m_blocks.last        = nullptr;
            m_blocks.block_count = 0;
            m_blocks.bump_offset = 0;
        }

        if constexpr (_ReleasePolicy::supports_generational_sweep)
        {
            m_generation = 0;
        }

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
            // rewrite free list pointers — they are
            // now in the new allocation
            if constexpr (_ReleasePolicy::supports_individual_release)
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
        if constexpr (_BlockPolicy::layout ==
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
        if constexpr (_BlockPolicy::layout ==
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

    // generational counter — only present when needed.
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


// =============================================================================
// VII. Policy Selection
// =============================================================================
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
using select_block_policy_t =
    typename select_block_policy<_Layout>::type;


// =============================================================================
// VIII. Default Aliases
// =============================================================================
// Sensible defaults for common use cases.

// default_pool_resource
//   alias: free-list release, chunked blocks (pointer-
// stable), exponential growth.  The safest general-
// purpose default.
template<typename _Type>
using default_pool_resource =
    pool_resource<_Type,
                  free_list_release_policy,
                  chunked_block_policy<>,
                  exponential_growth_policy>;

// monotonic_pool_resource
//   alias: monotonic release, chunked blocks, exponential
// growth.  Fastest acquire — no free list overhead.
// All memory released on reset/destruction.
template<typename _Type>
using monotonic_pool_resource =
    pool_resource<_Type,
                  monotonic_release_policy,
                  chunked_block_policy<>,
                  exponential_growth_policy>;

// flat_pool_resource
//   alias: free-list release, contiguous block, exponential
// growth.  Enables linear iteration over slots but may
// invalidate pointers on growth.
template<typename _Type>
using flat_pool_resource =
    pool_resource<_Type,
                  free_list_release_policy,
                  contiguous_block_policy,
                  exponential_growth_policy>;


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_POOL_
