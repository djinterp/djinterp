/******************************************************************************
* djinterp [container]                                        arena_tree.hpp
*
* Arena-Allocated Tree Container:
*   A thin wrapper over arena<> that adds single-root ownership and
* tree-specific semantics.  The arena provides storage, free-list
* recycling, topology operations, identity tracking, and traversal.
* This class adds:
*
*   - Root tracking (create_root, root, has_root, is_root)
*   - Allocate-and-link helpers (add_child, add_sibling)
*   - Recursive subtree removal with deallocation
*   - Tree-level clear that resets root
*
*   For a forest (multiple roots), use arena<> directly.
*
* DEPENDENCIES:
*   arena.hpp   — generalized arena container
*
* TABLE OF CONTENTS
* =================
* I.    arena_tree
* II.   Convenience Aliases
*
*
* path:      /inc/container/arena/arena_tree.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.07
******************************************************************************/

#ifndef DJINTERP_CONTAINER_ARENA_TREE_
#define DJINTERP_CONTAINER_ARENA_TREE_ 1

#include "../../../djinterp.hpp"
#include "../arena.hpp"


NS_DJINTERP


// =============================================================================
// I.   arena_tree
// =============================================================================

// arena_tree
//   class: a single-rooted tree backed by an arena.
// Inherits all arena operations and adds root management.
//
// Template parameters:
//   _Payload     — the domain data stored per node
//   _LinkPolicy  — determines link topology (default: full)
//   _Allocator   — STL allocator for the backing storage

template<typename _Payload,
         typename _LinkPolicy = full_nary_link_policy,
         typename _Allocator  = std::allocator<
             arena_node<_Payload, _LinkPolicy>>>
class arena_tree : public arena<_Payload, _LinkPolicy, _Allocator>
{
private:
    using base_type = arena<_Payload, _LinkPolicy, _Allocator>;

public:
    using typename base_type::payload_type;
    using typename base_type::link_policy;
    using typename base_type::node_type;
    using typename base_type::allocator_type;
    using typename base_type::size_type;


    // -----------------------------------------------------------------
    //  constructors
    // -----------------------------------------------------------------

    // arena_tree (default)
    //   constructor: creates an empty tree with no root.
    arena_tree()
        : base_type()
        , m_root(null_node)
    {
    }

    // arena_tree (with capacity)
    explicit
    arena_tree(size_type _reserve)
        : base_type(_reserve)
        , m_root(null_node)
    {
    }

    // arena_tree (allocator-extended)
    explicit
    arena_tree(const allocator_type& _alloc)
        : base_type(_alloc)
        , m_root(null_node)
    {
    }

    // arena_tree (capacity + allocator)
    arena_tree(size_type             _reserve,
               const allocator_type& _alloc)
        : base_type(_reserve, _alloc)
        , m_root(null_node)
    {
    }

    // rule-of-five: default.
    arena_tree(const arena_tree&)            = default;
    arena_tree(arena_tree&&) noexcept        = default;
    arena_tree& operator=(const arena_tree&) = default;
    arena_tree& operator=(arena_tree&&) noexcept = default;
    ~arena_tree()                            = default;


    // =================================================================
    //  root access
    // =================================================================

    // root
    //   method: returns the root node_id.
    node_id
    root() const noexcept
    {
        return m_root;
    }

    // has_root
    //   method: returns true if a root exists.
    bool
    has_root() const noexcept
    {
        return (m_root != null_node);
    }

    // set_root
    //   method: explicitly sets the root.  Use with care.
    void
    set_root(node_id _id) noexcept
    {
        m_root = _id;

        return;
    }

    // is_root
    //   method: returns true if _id is the current root.
    bool
    is_root(node_id _id) const noexcept
    {
        return (_id == m_root);
    }


    // =================================================================
    //  allocate-and-link helpers
    // =================================================================
    // These combine allocation with immediate linking,
    // which is the common case for tree construction.

    // create_root
    //   method: allocates a root node.  Asserts that no
    // root exists.
    node_id
    create_root(const _Payload& _data)
    {
        assert(m_root == null_node);

        m_root = base_type::allocate(_data);

        return m_root;
    }

    // create_root (move)
    node_id
    create_root(_Payload&& _data)
    {
        assert(m_root == null_node);

        m_root = base_type::allocate(
            static_cast<_Payload&&>(_data));

        return m_root;
    }

    // create_root (explicit stable_id)
    node_id
    create_root(std::uint64_t   _stable_id,
                const _Payload& _data)
    {
        assert(m_root == null_node);

        m_root = base_type::allocate(_stable_id, _data);

        return m_root;
    }

    // create_root (explicit stable_id, move)
    node_id
    create_root(std::uint64_t _stable_id,
                _Payload&&    _data)
    {
        assert(m_root == null_node);

        m_root = base_type::allocate(
            _stable_id,
            static_cast<_Payload&&>(_data));

        return m_root;
    }

    // add_child
    //   method: allocates a new node and appends it as the
    // last child of _parent.  Returns the new node_id.
    node_id
    add_child(node_id         _parent,
              const _Payload& _data)
    {
        node_id child = base_type::allocate(_data);
        base_type::append_child(_parent, child);

        return child;
    }

    // add_child (move)
    node_id
    add_child(node_id    _parent,
              _Payload&& _data)
    {
        node_id child = base_type::allocate(
            static_cast<_Payload&&>(_data));
        base_type::append_child(_parent, child);

        return child;
    }

    // add_child (explicit stable_id)
    node_id
    add_child(node_id         _parent,
              std::uint64_t   _stable_id,
              const _Payload& _data)
    {
        node_id child = base_type::allocate(
            _stable_id, _data);
        base_type::append_child(_parent, child);

        return child;
    }

    // add_child_first
    //   method: allocates and prepends as the first child.
    node_id
    add_child_first(node_id         _parent,
                    const _Payload& _data)
    {
        node_id child = base_type::allocate(_data);
        base_type::prepend_child(_parent, child);

        return child;
    }

    // add_sibling_after
    //   method: allocates and inserts after _sibling.
    node_id
    add_sibling_after(node_id         _sibling,
                      const _Payload& _data)
    {
        node_id child = base_type::allocate(_data);
        base_type::insert_after(_sibling, child);

        return child;
    }


    // =================================================================
    //  removal
    // =================================================================

    // remove
    //   method: detaches and deallocates a single node.
    // Does NOT remove children — they become orphaned.
    void
    remove(node_id _id)
    {
        base_type::detach(_id);
        base_type::deallocate(_id);

        if (_id == m_root)
        {
            m_root = null_node;
        }

        return;
    }

    // remove_subtree
    //   method: detaches _id and recursively deallocates
    // _id and all its descendants.
    void
    remove_subtree(node_id _id)
    {
        static_assert(
            ( link_policy::has_first_child &&
              link_policy::has_next_sibling ),
            "remove_subtree: requires first_child and "
            "next_sibling links.");

        if (_id == null_node)
        {
            return;
        }

        base_type::detach(_id);

        // collect all descendants via BFS, then deallocate
        std::vector<node_id> to_destroy;
        base_type::collect_subtree(_id, to_destroy);

        for (std::size_t i = 0;
             i < to_destroy.size();
             ++i)
        {
            base_type::deallocate(to_destroy[i]);
        }

        if (_id == m_root)
        {
            m_root = null_node;
        }

        return;
    }


    // =================================================================
    //  bulk operations
    // =================================================================

    // clear
    //   method: destroys all nodes and resets the tree.
    void
    clear()
    {
        base_type::clear();
        m_root = null_node;

        return;
    }

    // swap
    //   method: exchanges contents with another arena_tree.
    void
    swap(arena_tree& _other) noexcept
    {
        base_type::swap(_other);

        node_id t      = m_root;
        m_root         = _other.m_root;
        _other.m_root  = t;

        return;
    }


    // =================================================================
    //  comparison
    // =================================================================

    friend bool
    operator==(const arena_tree& _a,
               const arena_tree& _b) noexcept
    {
        return ( _a.m_root == _b.m_root &&
                 static_cast<const base_type&>(_a) ==
                 static_cast<const base_type&>(_b) );
    }

    friend bool
    operator!=(const arena_tree& _a,
               const arena_tree& _b) noexcept
    {
        return !(_a == _b);
    }


private:
    node_id m_root;
};


// =============================================================================
// II.  Convenience Aliases
// =============================================================================

// nary_tree
//   alias: arena tree with full n-ary links.  O(1)
// everything.  The default general-purpose tree.
template<typename _Payload>
using nary_tree = arena_tree<_Payload, full_nary_link_policy>;

// lcrs_tree
//   alias: minimal 2-link arena tree.  Smallest node
// footprint.
template<typename _Payload>
using lcrs_tree = arena_tree<_Payload, lcrs_link_policy>;

// parented_tree
//   alias: 3-link arena tree with parent back-pointer.
template<typename _Payload>
using parented_tree = arena_tree<_Payload,
                                 parented_lcrs_link_policy>;

// binary_tree
//   alias: arena tree with left/right/parent links.
template<typename _Payload>
using binary_tree = arena_tree<_Payload,
                               parented_binary_link_policy>;


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_ARENA_TREE_
