/******************************************************************************
* djinterp [container]                                        arena_tree.hpp
*
* Arena-Allocated Tree Container:
*   A generalized, policy-driven, arena-backed tree container.  Nodes
* are stored contiguously in a flat vector (or pool-backed vector) and
* reference each other by index, not pointer.  This yields:
*
*   - O(1) node allocation (amortized, via push_back or free list)
*   - O(1) bulk deallocation (clear the arena)
*   - excellent cache locality for traversal
*   - index stability across growth (indices never change)
*   - trivial serialization (indices survive save/load)
*
*   The tree topology is fully configurable via a link policy that
* determines which navigational links each node carries.  Presets
* are provided for common layouts:
*
*   lcrs_link_policy           — left-child/right-sibling (minimal)
*   parented_lcrs_link_policy  — + parent back-pointer
*   full_nary_link_policy      — + prev_sibling + last_child
*   binary_link_policy         — left/right (binary tree)
*   parented_binary_link_policy— left/right/parent
*
*   Custom policies are trivial to create: define a struct with
* `num_links` and named index constants.
*
* DESIGN PRINCIPLES:
*   - Zero virtual dispatch, zero RTTI, zero exceptions in the
*     hot path (acquire/release/navigate).
*   - UI-framework agnostic: suitable as the backbone for Qt
*     model trees, ncurses panels, GTK widget hierarchies,
*     scene graphs, ASTs, DOM-like document trees, etc.
*   - Pool-allocator compatible: template on any STL-conforming
*     allocator, including pool_allocator from this framework.
*
* DEPENDENCIES:
*   djinterp.hpp       — namespace macros, D_CONSTEXPR, clean_t
*   type_traits.hpp    — resolve_self_t (optional, for link type)
*
* COMPAT NOTES:
*   Baseline: C++17.
*   C++20: concept constraints available behind feature gate.
*   No std::optional, std::any, or std::variant required.
*   All nullable state uses index_type npos sentinel.
*
* TABLE OF CONTENTS
* =================
* I.    Link Flag Constants
* II.   tree_link_policy (flag-driven policy generator)
* III.  Preset Link Policies
* IV.   arena_node
* V.    arena_tree
* VI.   Convenience Aliases
*
*
* path:      /inc/container/arena/arena_tree.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.07
******************************************************************************/

#ifndef DJINTERP_CONTAINER_ARENA_TREE_
#define DJINTERP_CONTAINER_ARENA_TREE_ 1

#include <array>
#include <cassert>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <vector>
#include "../djinterp.hpp"
#include "../type_traits.hpp"


NS_DJINTERP
NS_CONTAINER


// =============================================================================
// I.   Link Flag Constants
// =============================================================================
// Bitmask flags specifying which navigational links a node carries.
// Combine with bitwise OR to define a topology.
//
//   constexpr unsigned my_layout =
//       tree_link::first_child  |
//       tree_link::next_sibling |
//       tree_link::parent;

// tree_link
//   struct: named constants for link flag bits.  Using a
// struct of constants rather than an enum to allow free
// bitwise combination without casts.
struct tree_link
{
    static D_CONSTEXPR unsigned first_child   = 1u;
    static D_CONSTEXPR unsigned next_sibling  = 2u;
    static D_CONSTEXPR unsigned parent        = 4u;
    static D_CONSTEXPR unsigned prev_sibling  = 8u;
    static D_CONSTEXPR unsigned last_child    = 16u;

    // --- binary tree links (mutually exclusive with
    //     first_child/next_sibling in typical use) ---
    static D_CONSTEXPR unsigned left           = 32u;
    static D_CONSTEXPR unsigned right          = 64u;
};


// =============================================================================
// II.  tree_link_policy (flag-driven policy generator)
// =============================================================================
// Given a bitmask of tree_link flags, generates a policy
// struct with:
//   - num_links:  total number of link slots
//   - named index constants for each enabled link (npos
//     for disabled links)
//
// The index assignment is deterministic: enabled links
// receive sequential indices in flag-bit order (lowest
// flag bit = index 0).

template<unsigned _Flags>
struct tree_link_policy
{
    // --- flag queries ---

    static D_CONSTEXPR bool has_first_child  =
        ((_Flags & tree_link::first_child)  != 0);
    static D_CONSTEXPR bool has_next_sibling =
        ((_Flags & tree_link::next_sibling) != 0);
    static D_CONSTEXPR bool has_parent       =
        ((_Flags & tree_link::parent)       != 0);
    static D_CONSTEXPR bool has_prev_sibling =
        ((_Flags & tree_link::prev_sibling) != 0);
    static D_CONSTEXPR bool has_last_child   =
        ((_Flags & tree_link::last_child)   != 0);
    static D_CONSTEXPR bool has_left         =
        ((_Flags & tree_link::left)         != 0);
    static D_CONSTEXPR bool has_right        =
        ((_Flags & tree_link::right)        != 0);

    // --- link count ---

    static D_CONSTEXPR std::size_t num_links =
        ( (has_first_child  ? 1u : 0u) +
          (has_next_sibling ? 1u : 0u) +
          (has_parent       ? 1u : 0u) +
          (has_prev_sibling ? 1u : 0u) +
          (has_last_child   ? 1u : 0u) +
          (has_left         ? 1u : 0u) +
          (has_right        ? 1u : 0u) );

    // --- sentinel ---

    static D_CONSTEXPR std::size_t npos =
        static_cast<std::size_t>(-1);

    // --- index assignments ---
    // Each enabled link receives a sequential index.
    // Disabled links map to npos.  The ordering is
    // deterministic: first_child < next_sibling <
    // parent < prev_sibling < last_child < left < right.

private:
    // count_before: number of enabled links with flag
    // bits strictly below the given flag.
    static D_CONSTEXPR std::size_t count_before(
        unsigned _flag
    )
    {
        std::size_t n = 0;

        if ((_flag > tree_link::first_child) &&
            has_first_child)
        {
            ++n;
        }

        if ((_flag > tree_link::next_sibling) &&
            has_next_sibling)
        {
            ++n;
        }

        if ((_flag > tree_link::parent) &&
            has_parent)
        {
            ++n;
        }

        if ((_flag > tree_link::prev_sibling) &&
            has_prev_sibling)
        {
            ++n;
        }

        if ((_flag > tree_link::last_child) &&
            has_last_child)
        {
            ++n;
        }

        if ((_flag > tree_link::left) &&
            has_left)
        {
            ++n;
        }

        if ((_flag > tree_link::right) &&
            has_right)
        {
            ++n;
        }

        return n;
    }

public:
    static D_CONSTEXPR std::size_t first_child_idx =
        has_first_child
            ? count_before(tree_link::first_child)
            : npos;

    static D_CONSTEXPR std::size_t next_sibling_idx =
        has_next_sibling
            ? count_before(tree_link::next_sibling)
            : npos;

    static D_CONSTEXPR std::size_t parent_idx =
        has_parent
            ? count_before(tree_link::parent)
            : npos;

    static D_CONSTEXPR std::size_t prev_sibling_idx =
        has_prev_sibling
            ? count_before(tree_link::prev_sibling)
            : npos;

    static D_CONSTEXPR std::size_t last_child_idx =
        has_last_child
            ? count_before(tree_link::last_child)
            : npos;

    static D_CONSTEXPR std::size_t left_idx =
        has_left
            ? count_before(tree_link::left)
            : npos;

    static D_CONSTEXPR std::size_t right_idx =
        has_right
            ? count_before(tree_link::right)
            : npos;

    // --- raw flag value (for trait inspection) ---

    static D_CONSTEXPR unsigned flags = _Flags;
};


// =============================================================================
// III. Preset Link Policies
// =============================================================================

// lcrs_link_policy
//   policy: left-child/right-sibling — the minimal n-ary
// tree encoding.  Two links per node regardless of child
// count.
using lcrs_link_policy = tree_link_policy<
    tree_link::first_child |
    tree_link::next_sibling>;

// parented_lcrs_link_policy
//   policy: LCRS with parent back-pointer.  Enables
// bottom-up traversal (e.g. event bubbling in a DOM).
using parented_lcrs_link_policy = tree_link_policy<
    tree_link::first_child  |
    tree_link::next_sibling |
    tree_link::parent>;

// full_nary_link_policy
//   policy: all five n-ary navigational links.  Enables
// O(1) last-child append and O(1) sibling removal.
// The most capable n-ary layout at the cost of 5 indices
// per node.
using full_nary_link_policy = tree_link_policy<
    tree_link::first_child  |
    tree_link::next_sibling |
    tree_link::parent       |
    tree_link::prev_sibling |
    tree_link::last_child>;

// binary_link_policy
//   policy: classic binary tree — left and right children.
using binary_link_policy = tree_link_policy<
    tree_link::left |
    tree_link::right>;

// parented_binary_link_policy
//   policy: binary tree with parent pointer.
using parented_binary_link_policy = tree_link_policy<
    tree_link::left   |
    tree_link::right  |
    tree_link::parent>;


// =============================================================================
// IV.  arena_node
// =============================================================================
// A fixed-size node stored in the arena.  Contains a value
// payload and an array of link indices whose layout is
// determined entirely by the link policy.
//
// Nodes also carry an `alive` flag for tombstone-based
// deletion and an index-based free list link for recycling.

template<typename    _ValueType,
         typename    _LinkPolicy,
         typename    _IndexType = std::size_t>
struct arena_node
{
    using value_type  = _ValueType;
    using link_policy = _LinkPolicy;
    using index_type  = _IndexType;
    using links_type  = std::array<index_type,
                                   _LinkPolicy::num_links>;

    static D_CONSTEXPR index_type npos =
        static_cast<index_type>(-1);

    static D_CONSTEXPR std::size_t num_links =
        _LinkPolicy::num_links;

    // --- data ---

    value_type  data;
    links_type  links;
    index_type  free_next;   // intrusive free list link
    bool        alive;

    // --- construction ---

    D_CONSTEXPR
    arena_node()
        : data{}
        , links{}
        , free_next(npos)
        , alive(false)
    {
        // zero-initialize all links to npos
        for (std::size_t i = 0; i < num_links; ++i)
        {
            links[i] = npos;
        }
    }

    D_CONSTEXPR explicit
    arena_node(const value_type& _val)
        : data(_val)
        , links{}
        , free_next(npos)
        , alive(true)
    {
        for (std::size_t i = 0; i < num_links; ++i)
        {
            links[i] = npos;
        }
    }

    D_CONSTEXPR explicit
    arena_node(value_type&& _val)
        : data(static_cast<value_type&&>(_val))
        , links{}
        , free_next(npos)
        , alive(true)
    {
        for (std::size_t i = 0; i < num_links; ++i)
        {
            links[i] = npos;
        }
    }

    // --- link accessors (compile-time indexed) ---

    template<std::size_t _LinkIdx>
    D_CONSTEXPR index_type link() const
    {
        static_assert(_LinkIdx < num_links,
                      "Link index out of range.");

        return links[_LinkIdx];
    }

    template<std::size_t _LinkIdx>
    D_CONSTEXPR void set_link(index_type _target)
    {
        static_assert(_LinkIdx < num_links,
                      "Link index out of range.");

        links[_LinkIdx] = _target;

        return;
    }

    // --- link accessors (runtime indexed) ---

    D_CONSTEXPR index_type link(std::size_t _idx) const
    {
        return links[_idx];
    }

    D_CONSTEXPR void set_link(std::size_t _idx,
                              index_type  _target)
    {
        links[_idx] = _target;

        return;
    }
};


// =============================================================================
// V.   arena_tree
// =============================================================================
// The tree container itself.  Manages a flat arena of
// arena_node instances, a free list for index recycling,
// and provides the full navigation and mutation API.
//
// Template parameters:
//   _ValueType   — the payload type stored in each node
//   _LinkPolicy  — determines the per-node link topology
//   _Allocator   — STL allocator for the arena vector
//                  (default: std::allocator; pass
//                  pool_allocator for pool-backed trees)

template<typename _ValueType,
         typename _LinkPolicy = parented_lcrs_link_policy,
         typename _Allocator  = std::allocator<
             arena_node<_ValueType, _LinkPolicy>>>
class arena_tree
{
public:

    // -----------------------------------------------------------------
    //  type aliases
    // -----------------------------------------------------------------

    using value_type      = _ValueType;
    using link_policy     = _LinkPolicy;
    using node_type       = arena_node<_ValueType, _LinkPolicy>;
    using allocator_type  = _Allocator;
    using index_type      = typename node_type::index_type;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = value_type&;
    using const_reference = const value_type&;

    // the sentinel index
    static D_CONSTEXPR index_type npos = node_type::npos;

    // -----------------------------------------------------------------
    //  the arena storage
    // -----------------------------------------------------------------

private:
    using arena_type = std::vector<node_type, _Allocator>;

    arena_type m_arena;
    index_type m_root;
    index_type m_free_head;
    size_type  m_live_count;

public:

    // -----------------------------------------------------------------
    //  constructors / destructor / assignment
    // -----------------------------------------------------------------

    // arena_tree
    //   constructor: default. Creates an empty tree.
    D_CONSTEXPR
    arena_tree()
        : m_arena()
        , m_root(npos)
        , m_free_head(npos)
        , m_live_count(0)
    {
    }

    // arena_tree
    //   constructor: with initial arena capacity.
    D_CONSTEXPR explicit
    arena_tree(size_type _reserve)
        : m_arena()
        , m_root(npos)
        , m_free_head(npos)
        , m_live_count(0)
    {
        m_arena.reserve(_reserve);
    }

    // arena_tree
    //   constructor: allocator-extended default.
    D_CONSTEXPR explicit
    arena_tree(const allocator_type& _alloc)
        : m_arena(_alloc)
        , m_root(npos)
        , m_free_head(npos)
        , m_live_count(0)
    {
    }

    // arena_tree
    //   constructor: capacity + allocator.
    D_CONSTEXPR
    arena_tree(size_type             _reserve,
               const allocator_type& _alloc)
        : m_arena(_alloc)
        , m_root(npos)
        , m_free_head(npos)
        , m_live_count(0)
    {
        m_arena.reserve(_reserve);
    }

    // rule-of-five: default.  The arena vector handles
    // deep copy/move.  No manual resource management.
    arena_tree(const arena_tree&)            = default;
    arena_tree(arena_tree&&) noexcept        = default;
    arena_tree& operator=(const arena_tree&) = default;
    arena_tree& operator=(arena_tree&&) noexcept = default;
    ~arena_tree()                            = default;


    // =================================================================
    //  capacity
    // =================================================================

    // empty
    //   method: returns true if the tree has no live nodes.
    D_CONSTEXPR bool
    empty() const noexcept
    {
        return (m_live_count == 0);
    }

    // size
    //   method: returns the number of live nodes.
    D_CONSTEXPR size_type
    size() const noexcept
    {
        return m_live_count;
    }

    // arena_size
    //   method: returns the total number of slots
    // (including tombstones) in the arena.
    D_CONSTEXPR size_type
    arena_size() const noexcept
    {
        return m_arena.size();
    }

    // capacity
    //   method: returns the current arena capacity.
    D_CONSTEXPR size_type
    capacity() const noexcept
    {
        return m_arena.capacity();
    }

    // reserve
    //   method: pre-allocates arena storage.
    D_CONSTEXPR void
    reserve(size_type _cap)
    {
        m_arena.reserve(_cap);

        return;
    }


    // =================================================================
    //  root access
    // =================================================================

    // root
    //   method: returns the index of the root node.
    D_CONSTEXPR index_type
    root() const noexcept
    {
        return m_root;
    }

    // has_root
    //   method: returns true if a root node exists.
    D_CONSTEXPR bool
    has_root() const noexcept
    {
        return (m_root != npos);
    }

    // set_root
    //   method: explicitly sets the root index.
    D_CONSTEXPR void
    set_root(index_type _id) noexcept
    {
        m_root = _id;

        return;
    }


    // =================================================================
    //  node access
    // =================================================================

    // valid
    //   method: returns true if _id references a live node.
    D_CONSTEXPR bool
    valid(index_type _id) const noexcept
    {
        return ( (_id < m_arena.size()) &&
                 m_arena[_id].alive );
    }

    // node_at
    //   method: returns a mutable reference to the node.
    D_CONSTEXPR node_type&
    node_at(index_type _id)
    {
        return m_arena[_id];
    }

    // node_at (const)
    //   method: returns a const reference to the node.
    D_CONSTEXPR const node_type&
    node_at(index_type _id) const
    {
        return m_arena[_id];
    }

    // data
    //   method: returns a mutable reference to the value
    // stored in the node at _id.
    D_CONSTEXPR reference
    data(index_type _id)
    {
        return m_arena[_id].data;
    }

    // data (const)
    //   method: returns a const reference to the value.
    D_CONSTEXPR const_reference
    data(index_type _id) const
    {
        return m_arena[_id].data;
    }

    // operator[]
    //   method: shorthand for data(_id).
    D_CONSTEXPR reference
    operator[](index_type _id)
    {
        return m_arena[_id].data;
    }

    // operator[] (const)
    D_CONSTEXPR const_reference
    operator[](index_type _id) const
    {
        return m_arena[_id].data;
    }


    // =================================================================
    //  navigation
    // =================================================================
    // Each navigator returns npos if the link is not
    // present in the policy or the target is null.
    // Methods are conditionally compiled: calling
    // first_child() on a binary_link_policy is a
    // compile-time error.

    // first_child
    //   method: returns the index of the first child.
    D_CONSTEXPR index_type
    first_child(index_type _id) const noexcept
    {
        static_assert(link_policy::has_first_child,
            "first_child: link not present in policy.");

        return m_arena[_id].links[
            link_policy::first_child_idx];
    }

    // last_child
    //   method: returns the index of the last child.
    D_CONSTEXPR index_type
    last_child(index_type _id) const noexcept
    {
        static_assert(link_policy::has_last_child,
            "last_child: link not present in policy.");

        return m_arena[_id].links[
            link_policy::last_child_idx];
    }

    // next_sibling
    //   method: returns the index of the next sibling.
    D_CONSTEXPR index_type
    next_sibling(index_type _id) const noexcept
    {
        static_assert(link_policy::has_next_sibling,
            "next_sibling: link not present in policy.");

        return m_arena[_id].links[
            link_policy::next_sibling_idx];
    }

    // prev_sibling
    //   method: returns the index of the previous sibling.
    D_CONSTEXPR index_type
    prev_sibling(index_type _id) const noexcept
    {
        static_assert(link_policy::has_prev_sibling,
            "prev_sibling: link not present in policy.");

        return m_arena[_id].links[
            link_policy::prev_sibling_idx];
    }

    // parent
    //   method: returns the index of the parent.
    D_CONSTEXPR index_type
    parent(index_type _id) const noexcept
    {
        static_assert(link_policy::has_parent,
            "parent: link not present in policy.");

        return m_arena[_id].links[
            link_policy::parent_idx];
    }

    // left
    //   method: returns the index of the left child
    // (binary tree policy).
    D_CONSTEXPR index_type
    left(index_type _id) const noexcept
    {
        static_assert(link_policy::has_left,
            "left: link not present in policy.");

        return m_arena[_id].links[
            link_policy::left_idx];
    }

    // right
    //   method: returns the index of the right child
    // (binary tree policy).
    D_CONSTEXPR index_type
    right(index_type _id) const noexcept
    {
        static_assert(link_policy::has_right,
            "right: link not present in policy.");

        return m_arena[_id].links[
            link_policy::right_idx];
    }

    // is_leaf
    //   method: returns true if the node has no children.
    // Dispatches to the appropriate link based on policy.
    D_CONSTEXPR bool
    is_leaf(index_type _id) const noexcept
    {
        if constexpr (link_policy::has_first_child)
        {
            return (first_child(_id) == npos);
        }
        else if constexpr (link_policy::has_left)
        {
            return ( (left(_id)  == npos) &&
                     (right(_id) == npos) );
        }
        else
        {
            // no child links in policy — every node
            // is trivially a leaf
            return true;
        }
    }

    // is_root
    //   method: returns true if _id is the root.
    D_CONSTEXPR bool
    is_root(index_type _id) const noexcept
    {
        return (_id == m_root);
    }

    // child_count
    //   method: counts children by walking the sibling
    // chain.  O(k) where k is the number of children.
    // Requires first_child + next_sibling links.
    D_CONSTEXPR size_type
    child_count(index_type _id) const noexcept
    {
        static_assert(
            ( link_policy::has_first_child &&
              link_policy::has_next_sibling ),
            "child_count: requires first_child and "
            "next_sibling links.");

        size_type  count = 0;
        index_type child = first_child(_id);

        while (child != npos)
        {
            ++count;
            child = next_sibling(child);
        }

        return count;
    }

    // depth
    //   method: returns the depth of a node (distance
    // from root).  Requires parent link.  O(d).
    D_CONSTEXPR size_type
    depth(index_type _id) const noexcept
    {
        static_assert(link_policy::has_parent,
            "depth: requires parent link.");

        size_type d = 0;

        while (_id != npos && _id != m_root)
        {
            _id = parent(_id);
            ++d;
        }

        return d;
    }


    // =================================================================
    //  node allocation
    // =================================================================

    // acquire_slot
    //   method: obtains a slot index from the free list or
    // by extending the arena.  The returned node is marked
    // alive with all links set to npos.
    D_CONSTEXPR index_type
    acquire_slot()
    {
        index_type id;

        // recycle from free list if available
        if (m_free_head != npos)
        {
            id = m_free_head;
            m_free_head = m_arena[id].free_next;

            // re-initialize the recycled node
            node_type& node = m_arena[id];
            node.data       = value_type{};
            node.free_next  = npos;
            node.alive      = true;

            for (std::size_t i = 0;
                 i < node_type::num_links;
                 ++i)
            {
                node.links[i] = npos;
            }
        }
        else
        {
            // extend the arena
            id = static_cast<index_type>(m_arena.size());
            m_arena.emplace_back();
            m_arena[id].alive = true;
        }

        ++m_live_count;

        return id;
    }

    // release_slot
    //   method: marks a node as dead and pushes its index
    // onto the free list.  Does NOT unlink the node from
    // its neighbours — the caller must do that first.
    D_CONSTEXPR void
    release_slot(index_type _id)
    {
        node_type& node = m_arena[_id];
        node.alive      = false;
        node.free_next  = m_free_head;
        m_free_head     = _id;

        --m_live_count;

        return;
    }


    // =================================================================
    //  mutation — insertion
    // =================================================================

    // create_root
    //   method: creates the root node with the given value.
    // Returns the root index.  Asserts that no root exists.
    D_CONSTEXPR index_type
    create_root(const value_type& _val)
    {
        assert(m_root == npos);

        index_type id     = acquire_slot();
        m_arena[id].data  = _val;
        m_root            = id;

        return id;
    }

    // create_root (move)
    D_CONSTEXPR index_type
    create_root(value_type&& _val)
    {
        assert(m_root == npos);

        index_type id     = acquire_slot();
        m_arena[id].data  = static_cast<value_type&&>(_val);
        m_root            = id;

        return id;
    }

    // append_child
    //   method: inserts a new child as the last child of
    // _parent_id.  Returns the new child's index.
    //
    // Link requirements:
    //   - first_child, next_sibling (minimum)
    //   - last_child (for O(1) append; otherwise O(k))
    //   - parent, prev_sibling (updated if present)
    D_CONSTEXPR index_type
    append_child(index_type        _parent_id,
                 const value_type& _val)
    {
        static_assert(
            ( link_policy::has_first_child &&
              link_policy::has_next_sibling ),
            "append_child: requires first_child and "
            "next_sibling links.");

        index_type child_id    = acquire_slot();
        m_arena[child_id].data = _val;

        link_child_as_last(_parent_id, child_id);

        return child_id;
    }

    // append_child (move)
    D_CONSTEXPR index_type
    append_child(index_type   _parent_id,
                 value_type&& _val)
    {
        static_assert(
            ( link_policy::has_first_child &&
              link_policy::has_next_sibling ),
            "append_child: requires first_child and "
            "next_sibling links.");

        index_type child_id    = acquire_slot();
        m_arena[child_id].data =
            static_cast<value_type&&>(_val);

        link_child_as_last(_parent_id, child_id);

        return child_id;
    }

    // prepend_child
    //   method: inserts a new child as the first child of
    // _parent_id.  Returns the new child's index.
    D_CONSTEXPR index_type
    prepend_child(index_type        _parent_id,
                  const value_type& _val)
    {
        static_assert(
            ( link_policy::has_first_child &&
              link_policy::has_next_sibling ),
            "prepend_child: requires first_child and "
            "next_sibling links.");

        index_type child_id    = acquire_slot();
        m_arena[child_id].data = _val;

        link_child_as_first(_parent_id, child_id);

        return child_id;
    }

    // prepend_child (move)
    D_CONSTEXPR index_type
    prepend_child(index_type   _parent_id,
                  value_type&& _val)
    {
        static_assert(
            ( link_policy::has_first_child &&
              link_policy::has_next_sibling ),
            "prepend_child: requires first_child and "
            "next_sibling links.");

        index_type child_id    = acquire_slot();
        m_arena[child_id].data =
            static_cast<value_type&&>(_val);

        link_child_as_first(_parent_id, child_id);

        return child_id;
    }

    // insert_after
    //   method: inserts a new sibling immediately after
    // _sibling_id.  Returns the new node's index.
    D_CONSTEXPR index_type
    insert_after(index_type        _sibling_id,
                 const value_type& _val)
    {
        static_assert(link_policy::has_next_sibling,
            "insert_after: requires next_sibling link.");

        index_type new_id    = acquire_slot();
        m_arena[new_id].data = _val;

        link_after(_sibling_id, new_id);

        return new_id;
    }

    // insert_after (move)
    D_CONSTEXPR index_type
    insert_after(index_type   _sibling_id,
                 value_type&& _val)
    {
        static_assert(link_policy::has_next_sibling,
            "insert_after: requires next_sibling link.");

        index_type new_id    = acquire_slot();
        m_arena[new_id].data =
            static_cast<value_type&&>(_val);

        link_after(_sibling_id, new_id);

        return new_id;
    }


    // =================================================================
    //  mutation — removal
    // =================================================================

    // unlink
    //   method: disconnects _id from its parent and
    // siblings without destroying it.  The subtree rooted
    // at _id remains intact.  Returns the index of the
    // former parent (or npos).
    D_CONSTEXPR index_type
    unlink(index_type _id)
    {
        static_assert(
            ( link_policy::has_first_child  &&
              link_policy::has_next_sibling ),
            "unlink: requires first_child and "
            "next_sibling links.");

        index_type parent_id = npos;

        // retrieve parent if available
        if constexpr (link_policy::has_parent)
        {
            parent_id = parent(_id);
        }

        // stitch siblings around _id
        if constexpr (link_policy::has_prev_sibling)
        {
            index_type prev = prev_sibling(_id);
            index_type next = next_sibling(_id);

            if (prev != npos)
            {
                set_next_sibling(prev, next);
            }
            else if (parent_id != npos)
            {
                // _id was the first child
                set_first_child(parent_id, next);
            }

            if (next != npos)
            {
                set_prev_sibling(next, prev);
            }
            else if (parent_id != npos)
            {
                // _id was the last child
                if constexpr (link_policy::has_last_child)
                {
                    set_last_child(parent_id, prev);
                }
            }
        }
        else
        {
            // without prev_sibling, we must walk from
            // parent's first child to find predecessor
            if (parent_id != npos)
            {
                unlink_child_linear(parent_id, _id);
            }
        }

        // clear _id's navigational links to siblings/parent
        set_next_sibling_if(_id, npos);
        set_prev_sibling_if(_id, npos);
        set_parent_if(_id, npos);

        return parent_id;
    }

    // remove
    //   method: unlinks _id from the tree and releases its
    // slot.  Does NOT recursively remove children — the
    // caller is responsible for walking the subtree if
    // needed.
    D_CONSTEXPR void
    remove(index_type _id)
    {
        unlink(_id);
        release_slot(_id);

        // if we removed the root, clear it
        if (_id == m_root)
        {
            m_root = npos;
        }

        return;
    }

    // remove_subtree
    //   method: recursively removes _id and all
    // descendants.  Requires first_child + next_sibling.
    D_CONSTEXPR void
    remove_subtree(index_type _id)
    {
        static_assert(
            ( link_policy::has_first_child &&
              link_policy::has_next_sibling ),
            "remove_subtree: requires first_child and "
            "next_sibling links.");

        if (_id == npos)
        {
            return;
        }

        // depth-first destruction via iterative stack
        // (avoid recursion to stay constexpr-safe and
        // stack-overflow-safe)
        index_type stack[64];
        std::size_t sp = 0;

        // unlink from parent first
        unlink(_id);

        stack[sp++] = _id;

        while (sp > 0)
        {
            index_type cur = stack[--sp];

            // push all children
            index_type child = first_child(cur);

            while (child != npos)
            {
                index_type next = next_sibling(child);

                if (sp < 64)
                {
                    stack[sp++] = child;
                }
                else
                {
                    // overflow fallback: recurse
                    remove_subtree_recursive(child);
                }

                child = next;
            }

            release_slot(cur);
        }

        // if we removed the root, clear it
        if (_id == m_root)
        {
            m_root = npos;
        }

        return;
    }


    // =================================================================
    //  mutation — reparenting
    // =================================================================

    // reparent
    //   method: moves the subtree rooted at _id to become
    // the last child of _new_parent_id.
    D_CONSTEXPR void
    reparent(index_type _id,
             index_type _new_parent_id)
    {
        unlink(_id);
        link_child_as_last(_new_parent_id, _id);

        return;
    }


    // =================================================================
    //  bulk operations
    // =================================================================

    // clear
    //   method: destroys all nodes and resets the tree
    // to empty state.
    D_CONSTEXPR void
    clear()
    {
        m_arena.clear();
        m_root       = npos;
        m_free_head  = npos;
        m_live_count = 0;

        return;
    }

    // swap
    //   method: exchanges contents with another arena_tree.
    D_CONSTEXPR void
    swap(arena_tree& _other) noexcept
    {
        m_arena.swap(_other.m_arena);

        { index_type t   = m_root;
          m_root         = _other.m_root;
          _other.m_root  = t; }

        { index_type t       = m_free_head;
          m_free_head        = _other.m_free_head;
          _other.m_free_head = t; }

        { size_type t          = m_live_count;
          m_live_count         = _other.m_live_count;
          _other.m_live_count  = t; }

        return;
    }


    // =================================================================
    //  allocator access
    // =================================================================

    // get_allocator
    //   method: returns a copy of the arena's allocator.
    D_CONSTEXPR allocator_type
    get_allocator() const noexcept
    {
        return m_arena.get_allocator();
    }


    // =================================================================
    //  raw arena access (for advanced use / serialization)
    // =================================================================

    // arena
    //   method: returns a const reference to the
    // underlying arena vector.
    D_CONSTEXPR const arena_type&
    arena() const noexcept
    {
        return m_arena;
    }

    // arena (mutable)
    //   method: returns a mutable reference.  Use with
    // care — modifying nodes directly can break
    // invariants.
    D_CONSTEXPR arena_type&
    arena() noexcept
    {
        return m_arena;
    }


    // =================================================================
    //  comparison
    // =================================================================

    D_CONSTEXPR friend bool
    operator==(const arena_tree& _a,
               const arena_tree& _b) noexcept
    {
        return ( _a.m_root       == _b.m_root       &&
                 _a.m_live_count == _b.m_live_count  &&
                 _a.m_arena      == _b.m_arena );
    }

    D_CONSTEXPR friend bool
    operator!=(const arena_tree& _a,
               const arena_tree& _b) noexcept
    {
        return !(_a == _b);
    }


    // =================================================================
    //  private link helpers
    // =================================================================

private:

    // --- conditional setters ---
    // These use if constexpr to set a link only if the
    // policy includes it.  No-ops otherwise.

    D_CONSTEXPR void
    set_first_child(index_type _node,
                    index_type _target) noexcept
    {
        m_arena[_node].links[
            link_policy::first_child_idx] = _target;

        return;
    }

    D_CONSTEXPR void
    set_last_child_if(index_type _node,
                      index_type _target) noexcept
    {
        if constexpr (link_policy::has_last_child)
        {
            m_arena[_node].links[
                link_policy::last_child_idx] = _target;
        }

        return;
    }

    D_CONSTEXPR void
    set_next_sibling(index_type _node,
                     index_type _target) noexcept
    {
        m_arena[_node].links[
            link_policy::next_sibling_idx] = _target;

        return;
    }

    D_CONSTEXPR void
    set_next_sibling_if(index_type _node,
                        index_type _target) noexcept
    {
        if constexpr (link_policy::has_next_sibling)
        {
            set_next_sibling(_node, _target);
        }

        return;
    }

    D_CONSTEXPR void
    set_prev_sibling_if(index_type _node,
                        index_type _target) noexcept
    {
        if constexpr (link_policy::has_prev_sibling)
        {
            m_arena[_node].links[
                link_policy::prev_sibling_idx] = _target;
        }

        return;
    }

    D_CONSTEXPR void
    set_parent_if(index_type _node,
                  index_type _target) noexcept
    {
        if constexpr (link_policy::has_parent)
        {
            m_arena[_node].links[
                link_policy::parent_idx] = _target;
        }

        return;
    }

    // --- structural linkers ---

    // link_child_as_last
    //   helper: wires _child_id as the last child of
    // _parent_id, maintaining all available links.
    D_CONSTEXPR void
    link_child_as_last(index_type _parent_id,
                       index_type _child_id) noexcept
    {
        // set parent link on child
        set_parent_if(_child_id, _parent_id);

        // if parent has no children yet
        if (first_child(_parent_id) == npos)
        {
            set_first_child(_parent_id, _child_id);
            set_last_child_if(_parent_id, _child_id);
        }
        else
        {
            // find or retrieve last child
            index_type last = npos;

            if constexpr (link_policy::has_last_child)
            {
                last = last_child(_parent_id);
            }
            else
            {
                // walk the sibling chain
                last = first_child(_parent_id);

                while (next_sibling(last) != npos)
                {
                    last = next_sibling(last);
                }
            }

            // link after last
            set_next_sibling(last, _child_id);
            set_prev_sibling_if(_child_id, last);
            set_last_child_if(_parent_id, _child_id);
        }

        return;
    }

    // link_child_as_first
    //   helper: wires _child_id as the first child of
    // _parent_id.
    D_CONSTEXPR void
    link_child_as_first(index_type _parent_id,
                        index_type _child_id) noexcept
    {
        set_parent_if(_child_id, _parent_id);

        index_type old_first = first_child(_parent_id);

        set_first_child(_parent_id, _child_id);
        set_next_sibling(_child_id, old_first);
        set_prev_sibling_if(_child_id, npos);

        if (old_first != npos)
        {
            set_prev_sibling_if(old_first, _child_id);
        }
        else
        {
            // was empty — child is also last
            set_last_child_if(_parent_id, _child_id);
        }

        return;
    }

    // link_after
    //   helper: wires _new_id immediately after _sibling_id
    // in the sibling chain.
    D_CONSTEXPR void
    link_after(index_type _sibling_id,
               index_type _new_id) noexcept
    {
        index_type old_next = next_sibling(_sibling_id);

        set_next_sibling(_sibling_id, _new_id);
        set_prev_sibling_if(_new_id, _sibling_id);
        set_next_sibling(_new_id, old_next);

        if (old_next != npos)
        {
            set_prev_sibling_if(old_next, _new_id);
        }
        else
        {
            // _new_id is now the last child — update
            // parent's last_child if we have both links
            if constexpr ( link_policy::has_parent &&
                           link_policy::has_last_child )
            {
                index_type p = parent(_sibling_id);

                if (p != npos)
                {
                    set_last_child_if(p, _new_id);
                }
            }
        }

        // inherit parent
        if constexpr (link_policy::has_parent)
        {
            set_parent_if(_new_id, parent(_sibling_id));
        }

        return;
    }

    // unlink_child_linear
    //   helper: O(k) unlink when prev_sibling is not
    // available.  Walks parent's child chain to find
    // predecessor.
    D_CONSTEXPR void
    unlink_child_linear(index_type _parent_id,
                        index_type _child_id) noexcept
    {
        index_type fc   = first_child(_parent_id);
        index_type next = next_sibling(_child_id);

        if (fc == _child_id)
        {
            // removing the first child
            set_first_child(_parent_id, next);
        }
        else
        {
            // walk to find predecessor
            index_type prev = fc;

            while (prev != npos &&
                   next_sibling(prev) != _child_id)
            {
                prev = next_sibling(prev);
            }

            if (prev != npos)
            {
                set_next_sibling(prev, next);
            }
        }

        // update last_child if applicable
        if constexpr (link_policy::has_last_child)
        {
            if (last_child(_parent_id) == _child_id)
            {
                // find new last child (walk from first)
                index_type cur  = first_child(_parent_id);
                index_type last = npos;

                while (cur != npos)
                {
                    last = cur;
                    cur  = next_sibling(cur);
                }

                set_last_child_if(_parent_id, last);
            }
        }

        return;
    }

    // remove_subtree_recursive
    //   helper: fallback for stack overflow in
    // remove_subtree.
    D_CONSTEXPR void
    remove_subtree_recursive(index_type _id)
    {
        if (_id == npos)
        {
            return;
        }

        index_type child = first_child(_id);

        while (child != npos)
        {
            index_type next = next_sibling(child);
            remove_subtree_recursive(child);
            child = next;
        }

        release_slot(_id);

        return;
    }
};


// =============================================================================
// VI.  Convenience Aliases
// =============================================================================

// nary_tree
//   alias: arena tree with parented LCRS layout.  The
// default general-purpose n-ary tree.
template<typename _ValueType,
         typename _Allocator = std::allocator<
             arena_node<_ValueType,
                        parented_lcrs_link_policy>>>
using nary_tree = arena_tree<_ValueType,
                             parented_lcrs_link_policy,
                             _Allocator>;

// full_nary_tree
//   alias: arena tree with all five n-ary links.
// O(1) append, O(1) unlink.
template<typename _ValueType,
         typename _Allocator = std::allocator<
             arena_node<_ValueType,
                        full_nary_link_policy>>>
using full_nary_tree = arena_tree<_ValueType,
                                  full_nary_link_policy,
                                  _Allocator>;

// binary_tree
//   alias: arena tree with binary left/right/parent.
template<typename _ValueType,
         typename _Allocator = std::allocator<
             arena_node<_ValueType,
                        parented_binary_link_policy>>>
using binary_tree = arena_tree<_ValueType,
                               parented_binary_link_policy,
                               _Allocator>;


NS_END  // container
NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_ARENA_TREE_
