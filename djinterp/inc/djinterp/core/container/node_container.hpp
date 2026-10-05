/*******************************************************************************
* djinterp [core]                                             node_container.hpp
*
* Node Container Foundation:
*   Foundational base class for all containers that manage their elements
* through node structures - trees, linked lists, graphs, skip lists, and
* any other topology where elements are accessed through self-referential
* node objects rather than contiguous storage.
*
*   node_container manages three things that every node-based container
* needs regardless of topology:
*
*     1. An ENTRY POINT into the node graph, stored according to an
*        ownership policy (raw pointer, unique_ptr, or shared_ptr).
*
*     2. A SIZE counter for the number of live nodes.
*
*     3. An ALLOCATOR for node construction/destruction.
*
*   The ownership policy is the central design axis.  It controls root/
* head/entry storage, destruction semantics, and Rule of Five behavior.
* Three policies are provided:
*
*     non_owning_policy    - raw pointer entry, no destruction (default)
*     unique_owning_policy - unique_ptr entry, move-only, RAII destruction
*     shared_owning_policy - shared_ptr entry, refcounted, copyable
*
*   Derived containers add topology:
*     tree_container  - inherits node_container, adds root()/has_root()
*     (future) list_container  - adds head()/tail()
*     (future) graph_container - adds entry set, adjacency
*
* TEMPLATE PARAMETERS:
*   ValueType        - the user-facing element type
*   NodeType         - the internal node structure
*   Allocator        - allocator for nodes (default: std::allocator<N>)
*   LockPolicy       - threading policy (default: void = no locking)
*   OwnershipPolicy - entry point ownership (default: non_owning_policy)
*
*
* path:      /inc/djinterp/core/container/node_container.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.11
*                                                            revised: 2026.10.02
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_NODE_CONTAINER_HPP
#define DJINTERP_CONTAINER_NODE_CONTAINER_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <memory>
#include <type_traits>
// djinterp
#include "../../djinterp.hpp"


NS_DJINTERP


// ===========================================================================
//  1.  OWNERSHIP POLICIES
// ===========================================================================
//   Static-interface structs that control how a node_container stores
// its entry point(s) into the node graph.  The policy interface is
// uniform so node_container can dispatch through it without branching
// on policy identity.
//
//   POLICY INTERFACE (all static):
//     root_storage_type<N>   - storage type for the entry point
//     owns                   - true if the container destroys nodes
//     copyable               - true if copy construction/assignment is valid
//     movable                - true if move construction/assignment is valid
//     make_null<N>()         - returns the null sentinel
//     is_null(storage)       - tests for null
//     get(storage)           - extracts mutable N*
//     get_const(storage)     - extracts const N*
//     reset(storage)         - destroys (if owning) and nullifies
//     adopt(storage, N*)     - takes ownership of a raw pointer
//     release(storage)       - relinquishes ownership, returns N*
//     move_from(src, dst)    - transfers between storages
//     clone(src)             - copies (only if copyable)


// ─── non_owning_policy ──────────────────────────────────────────────────────

// non_owning_policy
//   struct: entry point held as a raw pointer. The container does NOT destroy
// nodes. Caller or derived type manages lifetime. This is the
// backward-compatible default.
struct non_owning_policy
{
    static D_CONSTEXPR bool owns     = false;
    static D_CONSTEXPR bool copyable = true;
    static D_CONSTEXPR bool movable  = true;

    template<typename N>
    using root_storage_type = N*;

    template<typename N>
    static D_CONSTEXPR N*
    make_null() noexcept
    {
        return nullptr;
    }

    template<typename N>
    static D_CONSTEXPR bool
    is_null(
        N* _s
    ) noexcept
    {
        return (_s == nullptr);
    }

    template<typename N>
    static D_CONSTEXPR N*
    get(
        N* _s
    ) noexcept
    {
        return _s;
    }

    template<typename N>
    static D_CONSTEXPR const N*
    get_const(
        const N* _s
    ) noexcept
    {
        return _s;
    }

    template<typename N>
    static D_CONSTEXPR void
    reset(
        N*& _s
    ) noexcept
    {
        _s = nullptr;

        return;
    }

    template<typename N>
    static D_CONSTEXPR void
    adopt(
        N*& _s,
        N*  _node
    ) noexcept
    {
        _s = _node;

        return;
    }

    template<typename N>
    static D_CONSTEXPR N*
    release(
        N*& _s
    ) noexcept
    {
        N* tmp = _s;
        _s = nullptr;

        return tmp;
    }

    template<typename N>
    static D_CONSTEXPR void
    move_from(
        N*& _src,
        N*& _dst
    ) noexcept
    {
        _dst = _src;
        _src = nullptr;

        return;
    }

    template<typename N>
    static D_CONSTEXPR N*
    clone(
        N* _src
    )
    {
        return _src;
    }
};


// ─── unique_owning_policy ───────────────────────────────────────────────────

// unique_owning_policy
//   struct: entry point held via std::unique_ptr. The container exclusively
// owns the node graph. Move-only - copy is a compile-time error.
struct unique_owning_policy
{
    static D_CONSTEXPR bool owns     = true;
    static D_CONSTEXPR bool copyable = false;
    static D_CONSTEXPR bool movable  = true;

    template<typename N>
    using root_storage_type = std::unique_ptr<N>;

    template<typename N>
    static std::unique_ptr<N>
    make_null()
    {
        return std::unique_ptr<N>(nullptr);
    }

    template<typename N>
    static bool
    is_null(
        const std::unique_ptr<N>& _s
    ) noexcept
    {
        return (!_s);
    }

    template<typename N>
    static N*
    get(
        std::unique_ptr<N>& _s
    ) noexcept
    {
        return _s.get();
    }

    template<typename N>
    static const N*
    get_const(
        const std::unique_ptr<N>& _s
    ) noexcept
    {
        return _s.get();
    }

    template<typename N>
    static void
    reset(
        std::unique_ptr<N>& _s
    )
    {
        _s.reset();

        return;
    }

    template<typename N>
    static void
    adopt(
        std::unique_ptr<N>& _s,
        N*                  _node
    )
    {
        _s.reset(_node);

        return;
    }

    template<typename N>
    static void
    adopt(
        std::unique_ptr<N>& _s,
        std::unique_ptr<N>  _node
    )
    {
        _s = std::move(_node);

        return;
    }

    template<typename N>
    static N*
    release(
        std::unique_ptr<N>& _s
    )
    {
        return _s.release();
    }

    template<typename N>
    static void
    move_from(
        std::unique_ptr<N>& _src,
        std::unique_ptr<N>& _dst
    ) noexcept
    {
        _dst = std::move(_src);

        return;
    }

    template<typename N>
    static std::unique_ptr<N>
    clone(
        const std::unique_ptr<N>& /* _src */
    )
    {
        static_assert(
            sizeof(N) == 0,
            "unique_owning_policy does not support cloning. "
            "Provide a deep_copy function on the derived type, "
            "or use shared_owning_policy.");

        return std::unique_ptr<N>(nullptr);
    }
};


// ─── shared_owning_policy ───────────────────────────────────────────────────

// shared_owning_policy
//   struct: entry point held via std::shared_ptr. Multiple containers can
// share the same node graph with reference- counted lifetime management.
// Copyable and movable.
struct shared_owning_policy
{
    static D_CONSTEXPR bool owns     = true;
    static D_CONSTEXPR bool copyable = true;
    static D_CONSTEXPR bool movable  = true;

    template<typename N>
    using root_storage_type = std::shared_ptr<N>;

    template<typename N>
    static std::shared_ptr<N>
    make_null()
    {
        return std::shared_ptr<N>(nullptr);
    }

    template<typename N>
    static bool
    is_null(
        const std::shared_ptr<N>& _s
    ) noexcept
    {
        return (!_s);
    }

    template<typename N>
    static N*
    get(
        std::shared_ptr<N>& _s
    ) noexcept
    {
        return _s.get();
    }

    template<typename N>
    static const N*
    get_const(
        const std::shared_ptr<N>& _s
    ) noexcept
    {
        return _s.get();
    }

    template<typename N>
    static void
    reset(
        std::shared_ptr<N>& _s
    )
    {
        _s.reset();

        return;
    }

    template<typename N>
    static void
    adopt(
        std::shared_ptr<N>& _s,
        N*                  _node
    )
    {
        _s.reset(_node);

        return;
    }

    template<typename N>
    static void
    adopt(
        std::shared_ptr<N>& _s,
        std::shared_ptr<N>  _node
    )
    {
        _s = std::move(_node);

        return;
    }

    template<typename N>
    static N*
    release(
        std::shared_ptr<N>& _s
    )
    {
        N* raw = _s.get();
        _s.reset();

        return raw;
    }

    template<typename N>
    static void
    move_from(
        std::shared_ptr<N>& _src,
        std::shared_ptr<N>& _dst
    ) noexcept
    {
        _dst = std::move(_src);

        return;
    }

    template<typename N>
    static std::shared_ptr<N>
    clone(
        const std::shared_ptr<N>& _src
    )
    {
        return _src;
    }
};


// ===========================================================================
//  2.  NODE CONTAINER
// ===========================================================================
//   The foundational base for all node-based containers.  Manages a
// single entry point into the node graph, a size counter, and an
// allocator.  Derived containers add topology-specific semantics:
//
//   tree_container  -> root(), has_root()
//   list_container  -> head(), tail()
//   graph_container -> entries(), entry_count()
//
//   node_container itself exposes entry_point() / has_entry() as the
// topology-neutral interface.  Derived types alias these to their
// domain names (root, head, etc.).

// node_container
//   class: foundational base for all node-based containers.
template<typename ValueType,
         typename NodeType,
         typename Allocator        = std::allocator<NodeType>,
         typename LockPolicy       = void,
         typename OwnershipPolicy = non_owning_policy>
class node_container
{
private:
    using allocator_traits = std::allocator_traits<Allocator>;

public:
    // ── type aliases ────────────────────────────────────────────────
    using value_type       = ValueType;
    using node_type        = NodeType;
    using allocator_type   = Allocator;
    using lock_policy      = LockPolicy;
    using ownership_policy = OwnershipPolicy;
    using size_type        = std::size_t;
    using difference_type  = std::ptrdiff_t;
    using depth_type       = std::size_t;
    using reference        = value_type&;
    using const_reference  = const value_type&;
    // using pointer = typename allocator_pointer; using const_pointer =
    // typename allocator_const_pointer;

    // Root storage type determined by ownership policy
    using entry_storage = typename OwnershipPolicy::
        template root_storage_type<NodeType>;

    // ── compile-time ownership queries ──────────────────────────────
    static D_CONSTEXPR bool entry_owns   = OwnershipPolicy::owns;
    static D_CONSTEXPR bool is_copyable  = OwnershipPolicy::copyable;
    static D_CONSTEXPR bool is_movable   = OwnershipPolicy::movable;

    // -----------------------------------------------------------------
    // constructors / destructor / assignment
    // -----------------------------------------------------------------

    // node_container
    //   constructor: default. Creates an empty container.
    D_CONSTEXPR node_container() noexcept(
        noexcept(entry_storage{}))
        : m_entry(OwnershipPolicy::template make_null<NodeType>()),
          m_size(0),
          m_allocator()
    {}

    // node_container
    //   constructor: allocator-extended default.
    D_CONSTEXPR explicit node_container(
        const allocator_type& _alloc
    ) noexcept(noexcept(entry_storage{}))
        : m_entry(
                OwnershipPolicy::template make_null<NodeType>()),
            m_size(0),
            m_allocator(_alloc)
    {}

    // node_container
    //   constructor: copy.
    //   Only participates if the ownership policy supports copying.
    template<typename Dummy = void,
             std::enable_if_t<
                 OwnershipPolicy::copyable &&
                 std::is_void<Dummy>::value,
                 int> = 0>
    D_CONSTEXPR node_container(
        const node_container& _other
    )
        : m_entry(OwnershipPolicy::clone(_other.m_entry)),
          m_size(_other.m_size),
          m_allocator(
              allocator_select_on_container_copy_construction(
                  _other.m_allocator))
    {}

    // node_container
    //   constructor: move. Transfers entry point and size.
    D_CONSTEXPR node_container(
            node_container&& _other
        ) noexcept
            : m_entry(
                  OwnershipPolicy::template make_null<NodeType>()),
              m_size(_other.m_size),
              m_allocator(
                  static_cast<allocator_type&&>(_other.m_allocator))
        {
            OwnershipPolicy::move_from(_other.m_entry, m_entry);
            _other.m_size = 0;
        }

    // ~node_container
    //   destructor: if the ownership policy owns, destruction is handled by
    // the entry_storage destructor (unique_ptr/shared_ptr). If non-owning, the
    // raw pointer is abandoned.
    ~node_container() = default;

    // operator=
    //   assignment: copy. Only participates if copyable.
    template<typename Dummy = void,
             std::enable_if_t<
                 OwnershipPolicy::copyable &&
                 std::is_void<Dummy>::value,
                 int> = 0>
    D_CONSTEXPR node_container&
    operator=(
        const node_container& _other
    )
    {
        if (this != &_other)
        {
            m_entry = OwnershipPolicy::clone(_other.m_entry);
            m_size  = _other.m_size;

            if constexpr (allocator_traits::propagate_on_container_copy_assignment::value)
            {
                m_allocator = _other.m_allocator;
            }
        }

        return *this;
    }

    // operator=
    //   assignment: move. Transfers state.
    D_CONSTEXPR node_container&
    operator=(
        node_container&& _other
    ) noexcept
    {
        if (this != &_other)
        {
            OwnershipPolicy::reset(m_entry);
            OwnershipPolicy::move_from(_other.m_entry, m_entry);

            m_size = _other.m_size;

            if constexpr (allocator_traits::propagate_on_container_move_assignment::value)
            {
                m_allocator = static_cast<allocator_type&&>(
                    _other.m_allocator);
            }

            _other.m_size = 0;
        }

        return *this;
    }


    // -----------------------------------------------------------------
    // capacity
    // -----------------------------------------------------------------

    D_CONSTEXPR bool
    empty() const noexcept
    {
        return (m_size == 0);
    }

    D_CONSTEXPR size_type
    size() const noexcept
    {
        return m_size;
    }

    D_CONSTEXPR size_type
    max_size() const noexcept
    {
        return allocator_traits::max_size(m_allocator);
    }


    // -----------------------------------------------------------------
    // entry point access (topology-neutral)
    // -----------------------------------------------------------------

    // entry_point
    //   returns a mutable pointer to the entry node. Derived types alias this:
    // root() for trees, head() for lists.
    D_CONSTEXPR node_type*
    entry_point() noexcept
    {
        return OwnershipPolicy::get(m_entry);
    }

    // entry_point (const)
    D_CONSTEXPR const node_type*
    entry_point() const noexcept
    {
        return OwnershipPolicy::get_const(m_entry);
    }

    // has_entry
    //   returns true if the entry point is non-null.
    D_CONSTEXPR bool
    has_entry() const noexcept
    {
        return (!OwnershipPolicy::is_null(m_entry));
    }

    // entry_storage_ref
    //   returns a reference to the underlying storage. Advanced usage - prefer
    // the typed accessors.
    entry_storage&
    entry_storage_ref() noexcept
    {
        return m_entry;
    }

    const entry_storage&
    entry_storage_ref() const noexcept
    {
        return m_entry;
    }


    // -----------------------------------------------------------------
    // entry point mutation
    // -----------------------------------------------------------------

    // set_entry (raw pointer)
    //   for non-owning: sets the entry directly.
    //   for owning: takes ownership of the pointer.
    D_CONSTEXPR void
    set_entry(
        node_type* _node
    ) noexcept(!OwnershipPolicy::owns)
    {
        OwnershipPolicy::adopt(m_entry, _node);

        return;
    }

    // adopt_entry (unique_ptr)
    //   only available with unique_owning_policy.
    template<typename Dummy = OwnershipPolicy,
             std::enable_if_t<
                 std::is_same<Dummy,
                              unique_owning_policy>::value,
                 int> = 0>
    void
    adopt_entry(
        std::unique_ptr<node_type> _node
    )
    {
        OwnershipPolicy::adopt(m_entry, std::move(_node));

        return;
    }

    // share_entry (shared_ptr)
    //   only available with shared_owning_policy.
    template<typename Dummy = OwnershipPolicy,
             std::enable_if_t<
                 std::is_same<Dummy,
                              shared_owning_policy>::value,
                 int> = 0>
    void
    share_entry(
        std::shared_ptr<node_type> _node
    )
    {
        OwnershipPolicy::adopt(m_entry, std::move(_node));

        return;
    }

    // release_entry
    //   releases ownership and returns the raw pointer.
    node_type*
    release_entry()
    {
        node_type* released = OwnershipPolicy::release(m_entry);
        m_size = 0;

        return released;
    }


    // -----------------------------------------------------------------
    // size mutation
    // -----------------------------------------------------------------

    D_CONSTEXPR void
    set_size(
        size_type _new_size
    ) noexcept
    {
        m_size = _new_size;

        return;
    }

    D_CONSTEXPR void
    increment_size() noexcept
    {
        ++m_size;

        return;
    }

    D_CONSTEXPR void
    decrement_size() noexcept
    {
        --m_size;

        return;
    }


    // -----------------------------------------------------------------
    // clear
    // -----------------------------------------------------------------

    D_CONSTEXPR void
    clear() noexcept(!OwnershipPolicy::owns)
    {
        OwnershipPolicy::reset(m_entry);
        m_size = 0;

        return;
    }


    // -----------------------------------------------------------------
    // swap
    // -----------------------------------------------------------------

    D_CONSTEXPR void
    swap(
        node_container& _other
    ) noexcept
    {
        djinterp::constexpr_swap(m_entry, _other.m_entry);
        djinterp::constexpr_swap(m_size, _other.m_size);

        if constexpr (allocator_traits::propagate_on_container_swap::value)
        {
            djinterp::constexpr_swap(m_allocator,
                                      _other.m_allocator);
        }

        return;
    }


    // -----------------------------------------------------------------
    // allocator
    // -----------------------------------------------------------------

    D_CONSTEXPR allocator_type
    get_allocator() const noexcept
    {
        return m_allocator;
    }


    // -----------------------------------------------------------------
    // comparison
    // -----------------------------------------------------------------

    D_CONSTEXPR friend bool
    operator==(
        const node_container& _a,
        const node_container& _b
    ) noexcept
    {
        return ( _a.entry_point() == _b.entry_point() &&
                 _a.m_size        == _b.m_size );
    }

    D_CONSTEXPR friend bool
    operator!=(
        const node_container& _a,
        const node_container& _b
    ) noexcept
    {
        return !(_a == _b);
    }

private:
    entry_storage  m_entry;
    size_type      m_size;
    allocator_type m_allocator;
};


// ===========================================================================
//  3.  CONVENIENCE ALIASES
// ===========================================================================

// owning_node_container
//   alias: node_container with unique_owning_policy.
template<typename ValueType,
         typename NodeType,
         typename Allocator   = std::allocator<NodeType>,
         typename LockPolicy = void>
using owning_node_container =
    node_container<ValueType,
                   NodeType,
                   Allocator,
                   LockPolicy,
                   unique_owning_policy>;

// shared_node_container
//   alias: node_container with shared_owning_policy.
template<typename ValueType,
         typename NodeType,
         typename Allocator   = std::allocator<NodeType>,
         typename LockPolicy = void>
using shared_node_container =
    node_container<ValueType,
                   NodeType,
                   Allocator,
                   LockPolicy,
                   shared_owning_policy>;


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_NODE_CONTAINER_HPP
