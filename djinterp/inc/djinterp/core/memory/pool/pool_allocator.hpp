/******************************************************************************
* djinterp [memory]                                         pool_allocator.hpp
*
* STL-conforming allocator backed by a pool_resource.
*   pool_allocator<T> satisfies the C++ named requirements for Allocator
* (Table [allocator.requirements] in the standard), enabling its use as
* a drop-in replacement for std::allocator in any standard or djinterp
* container.
*
*   The allocator is a lightweight handle — it stores a non-owning
* pointer to a pool_resource and delegates all storage operations to it.
* Copying a pool_allocator copies the handle, not the pool.
*
*   Rebind is supported: pool_allocator<T> can be rebound to
* pool_allocator<U>, producing a new allocator backed by a different
* pool_resource<U> (provided externally) or by the same pool_resource
* when the slot sizes are compatible.
*
*   Propagation semantics follow the PMR model:
*     - propagate_on_container_copy_assignment:  false
*     - propagate_on_container_move_assignment:  false
*     - propagate_on_container_swap:             false
*     - select_on_container_copy_construction:   returns *this
*   This means containers using pool_allocator will use the same pool
*   across copies and moves unless explicitly rebound.
*
* DEPENDENCIES:
*   pool.hpp       — pool_resource
*   djinterp.hpp   — namespace macros
*
* TABLE OF CONTENTS
* =================
* I.    pool_allocator
* II.   Equality Operators
* III.  Convenience Aliases
*
*
* path:      /inc/djinterp/core/memory/pool/pool_allocator.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.03.30
******************************************************************************/

#ifndef DJINTERP_MEMORY_POOL_ALLOCATOR_
#define DJINTERP_MEMORY_POOL_ALLOCATOR_ 1

// std
#include <cstddef>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
// djinterp
#include "../djinterp.hpp"
#include "./pool.hpp"


NS_DJINTERP


// =============================================================================
// I.   pool_allocator
// =============================================================================

// pool_allocator
//   class: a stateful allocator that draws storage from an
// external pool_resource.  Satisfies the C++ Allocator
// named requirements.
//
// Template parameters:
//   _Type            — the element type
//   _ReleasePolicy   — forwarded to pool_resource
//   _BlockPolicy     — forwarded to pool_resource
//   _GrowthPolicy    — forwarded to pool_resource
template<typename _Type,
         typename _ReleasePolicy = free_list_release_policy,
         typename _BlockPolicy   = chunked_block_policy<>,
         typename _GrowthPolicy  = exponential_growth_policy>
class pool_allocator
{
public:
    // --- standard allocator type aliases ---

    using value_type      = _Type;
    using pointer         = _Type*;
    using const_pointer   = const _Type*;
    using reference       = _Type&;
    using const_reference = const _Type&;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;

    // --- pool type ---

    using pool_type = pool_resource<_Type,
                                    _ReleasePolicy,
                                    _BlockPolicy,
                                    _GrowthPolicy>;

    // --- propagation traits ---
    // PMR-style: the allocator is bound to a specific pool
    // and does not propagate on container copy/move/swap.
    // The user rebinds explicitly when a different pool
    // is desired.

    using propagate_on_container_copy_assignment = std::false_type;
    using propagate_on_container_move_assignment = std::false_type;
    using propagate_on_container_swap            = std::false_type;
    using is_always_equal                        = std::false_type;

    // --- rebind ---
    // Produces a pool_allocator for a different type,
    // preserving the policy parameters.  The rebound
    // allocator must be constructed with its own
    // pool_resource<_Other, ...>.

    template<typename _Other>
    struct rebind
    {
        using other = pool_allocator<_Other,
                                     _ReleasePolicy,
                                     _BlockPolicy,
                                     _GrowthPolicy>;
    };

    // --------------------------------------------------------
    //  construction
    // --------------------------------------------------------

    // pool_allocator (from pool reference)
    //   binds the allocator to an existing pool resource.
    explicit
    pool_allocator(
        pool_type& _pool
    ) noexcept
        : m_pool(&_pool)
    {}

    // pool_allocator (copy)
    //   copies the pool binding.  Both allocators share
    // the same pool.
    pool_allocator(
        const pool_allocator& _other
    ) noexcept
        : m_pool(_other.m_pool)
    {}

    // pool_allocator (rebind copy)
    //   converting constructor for rebind.  Accepts a
    // pool_allocator of a different type but compatible
    // policies.  The pool pointer is NOT transferable
    // across types — the caller must supply a correctly
    // typed pool.
    //
    //   This constructor exists to satisfy the Allocator
    // requirements.  In practice, rebound allocators are
    // constructed with explicit pool references.
    template<typename _Other>
    pool_allocator(
        const pool_allocator<_Other,
                             _ReleasePolicy,
                             _BlockPolicy,
                             _GrowthPolicy>& /*_other*/) noexcept
        : m_pool(nullptr)
    {
        // NOTE: rebound allocators cannot share a pool
        // with a different value_type.  The caller must
        // assign a pool via set_pool() or construct
        // with an explicit pool reference.
        //
        // This is intentional — pool_resource is typed,
        // and sharing a pool_resource<A> with a
        // pool_allocator<B> is not safe.
    }

    // pool_allocator (assignment)
    pool_allocator&
    operator=(const pool_allocator& _other) noexcept
    {
        m_pool = _other.m_pool;

        return *this;
    }

    // --------------------------------------------------------
    //  pool access
    // --------------------------------------------------------

    // resource
    //   returns a pointer to the underlying pool resource.
    pool_type*
    resource() const noexcept
    {
        return m_pool;
    }

    // set_resource
    //   rebinds this allocator to a different pool.
    void
    set_resource(pool_type& _pool) noexcept
    {
        m_pool = &_pool;

        return;
    }

    // --------------------------------------------------------
    //  allocate / deallocate
    // --------------------------------------------------------

    // allocate
    //   allocates storage for _n objects of type _Type.
    // For pool-backed allocation, _n should be 1 (pools
    // allocate one slot at a time).  Falls back to
    // ::operator new for _n > 1.
    pointer
    allocate(
        size_type _n
    )
    {
        if (_n == 1 && m_pool)
        {
            void* slot = m_pool->acquire();

            if (!slot)
            {
                throw std::bad_alloc();
            }

            return static_cast<pointer>(slot);
        }

        // bulk allocation — bypass pool
        return static_cast<pointer>(
            ::operator new(_n * sizeof(_Type)));
    }

    // deallocate
    //   returns storage for _n objects.  For _n == 1,
    // delegates to the pool.  For _n > 1, delegates to
    // ::operator delete (matching allocate).
    void
    deallocate(
        pointer   _p,
        size_type _n
    ) noexcept
    {
        if (!_p)
        {
            return;
        }

        if (_n == 1 && m_pool)
        {
            m_pool->release(static_cast<void*>(_p));
        }
        else
        {
            ::operator delete(static_cast<void*>(_p));
        }

        return;
    }

    // --------------------------------------------------------
    //  construct / destroy (C++17 deprecation-safe)
    // --------------------------------------------------------
    // These are provided for pre-C++20 compatibility.
    // C++20 allocator_traits handles construction without
    // requiring these members.

    template<typename _U,
             typename... _Args>
    void
    construct(
        _U*        _p,
        _Args&&... _args
    )
    {
        ::new (static_cast<void*>(_p))
            _U(static_cast<_Args&&>(_args)...);

        return;
    }

    template<typename _U>
    void
    destroy(
        _U* _p
    ) noexcept
    {
        _p->~_U();

        return;
    }

    // --------------------------------------------------------
    //  capacity
    // --------------------------------------------------------

    // max_size
    //   returns the maximum number of objects that can
    // theoretically be allocated.
    size_type
    max_size() const noexcept
    {
        return std::numeric_limits<size_type>::max()
             / sizeof(_Type);
    }

    // --------------------------------------------------------
    //  select_on_container_copy_construction
    // --------------------------------------------------------
    // Returns a copy of this allocator bound to the same
    // pool.  The copied container will share the pool with
    // the original.

    pool_allocator
    select_on_container_copy_construction() const noexcept
    {
        return *this;
    }

private:
    pool_type* m_pool;

    // grant access to other pool_allocator instantiations
    // for the rebind converting constructor.
    template<typename,
             typename,
             typename,
             typename>
    friend class pool_allocator;
};


// =============================================================================
// II.  Equality Operators
// =============================================================================
// Two pool allocators are equal if and only if they
// reference the same pool_resource instance.

template<typename _T1,
         typename _T2,
         typename _RP,
         typename _BP,
         typename _GP>
bool
operator==(const pool_allocator<_T1, _RP, _BP, _GP>& _a,
           const pool_allocator<_T2, _RP, _BP, _GP>& _b) noexcept
{
    // different value types cannot share a typed pool,
    // so they are never equal unless both are null.
    if constexpr (std::is_same_v<_T1, _T2>)
    {
        return (_a.resource() == _b.resource());
    }
    else
    {
        return ( (!_a.resource()) && (!_b.resource()) );
    }
}

template<typename _T1,
         typename _T2,
         typename _RP,
         typename _BP,
         typename _GP>
bool
operator!=(const pool_allocator<_T1, _RP, _BP, _GP>& _a,
           const pool_allocator<_T2, _RP, _BP, _GP>& _b) noexcept
{
    return !(_a == _b);
}


// =============================================================================
// III. Convenience Aliases
// =============================================================================

// default_pool_allocator
//   alias: pool allocator with default policies (free-list
// release, chunked blocks, exponential growth).
template<typename _Type>
using default_pool_allocator =
    pool_allocator<_Type,
                   free_list_release_policy,
                   chunked_block_policy<>,
                   exponential_growth_policy>;

// monotonic_pool_allocator
//   alias: pool allocator with monotonic release.
// Fastest allocation — no free list overhead.
template<typename _Type>
using monotonic_pool_allocator =
    pool_allocator<_Type,
                   monotonic_release_policy,
                   chunked_block_policy<>,
                   exponential_growth_policy>;


NS_END  // djinterp


#endif  // DJINTERP_MEMORY_POOL_ALLOCATOR_