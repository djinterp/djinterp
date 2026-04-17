/******************************************************************************
* djinterp [container]                                              arena.hpp
*
* Generalized Arena Container:
*   A cache-friendly, index-based, policy-driven arena for contiguous
* storage of linked nodes.  The arena is parameterized on payload type
* and link policy, making topology, traversal, and subtree operations
* completely generic across tree shapes:
*
*   - LCRS (2 links/node), parented LCRS (3), full n-ary (5),
*     binary (2), parented binary (3), or any custom combination.
*
*   All topology operations (append, detach, move) are O(1) when the
* policy includes sufficient links.  Subtree collection is O(subtree).
*
*   Nodes carry identity fields (stable_id, version) for change
* tracking and cross-arena referencing (e.g. old-tree / new-tree
* diffing in a virtual DOM).
*
*   Deleted nodes are recycled via an intrusive free list threaded
* through the arena, providing O(1) allocation and deallocation
* without compaction.
*
* DESIGN PRINCIPLES:
*   - Zero virtual dispatch, zero RTTI, zero exceptions on the hot
*     path (allocate/release/navigate).
*   - UI-framework agnostic: suitable as the backbone for Qt model
*     trees, ncurses panels, GTK widget hierarchies, scene graphs,
*     ASTs, DOM-like document trees, etc.
*   - Pool-allocator compatible via template allocator parameter.
*
* COMPAT NOTES:
*   Baseline: C++17.
*   C++20: concept constraints available behind feature gate.
*   No std::optional, std::any, or std::variant required.
*   All nullable state uses node_id / null_node sentinel.
*
* TABLE OF CONTENTS
* =================
* I.    Node ID and Sentinel
* II.   Link Flag Constants
* III.  tree_link_policy (flag-driven policy generator)
* IV.   Preset Link Policies
* V.    arena_node
* VI.   arena
* VII.  Convenience Aliases
*
*
* path:      /inc/container/arena/arena.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2025.03.18
******************************************************************************/

#ifndef DJINTERP_CONTAINER_ARENA_
#define DJINTERP_CONTAINER_ARENA_ 1

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>
#include "../../djinterp.hpp"


// D_KEYWORD_ARENA
//   keyword: resolves to `arena`.
#ifndef D_KEYWORD_ARENA
    #define D_KEYWORD_ARENA             arena
#endif

// NS_ARENA
//   namespace: the arena subsystem namespace.
#ifndef NS_ARENA
    #define NS_ARENA                    D_NAMESPACE(D_KEYWORD_ARENA)
#endif


NS_DJINTERP


// =============================================================================
// I.   Node ID and Sentinel
// =============================================================================

// node_id
//   typedef: index into an arena's node array.  32-bit to halve
// pointer overhead on 64-bit targets and survive reallocation.
typedef std::uint32_t node_id;

// null_node
//   constant: sentinel value indicating "no node".
D_CONSTEXPR node_id null_node = ~node_id(0);


// =============================================================================
// II.  Link Flag Constants
// =============================================================================
// Bitmask flags specifying which navigational links a node
// carries.  Combine with bitwise OR to define a topology.
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

    // --- binary tree links ---
    static D_CONSTEXPR unsigned left          = 32u;
    static D_CONSTEXPR unsigned right         = 64u;
};


// =============================================================================
// III. tree_link_policy (flag-driven policy generator)
// =============================================================================
// Given a bitmask of tree_link flags, generates a policy
// struct with:
//   - num_links:  total number of link slots
//   - named index constants for each enabled link (null_node
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

    // --- index assignments ---
    // Each enabled link receives a sequential index.
    // Disabled links map to null_node.

private:
    static D_CONSTEXPR std::size_t count_before(
        unsigned _flag
    )
    {
        std::size_t n = 0;

        if ((_flag > tree_link::first_child)  && has_first_child)  { ++n; }
        if ((_flag > tree_link::next_sibling) && has_next_sibling) { ++n; }
        if ((_flag > tree_link::parent)       && has_parent)       { ++n; }
        if ((_flag > tree_link::prev_sibling) && has_prev_sibling) { ++n; }
        if ((_flag > tree_link::last_child)   && has_last_child)   { ++n; }
        if ((_flag > tree_link::left)         && has_left)         { ++n; }
        if ((_flag > tree_link::right)        && has_right)        { ++n; }

        return n;
    }

public:
    static D_CONSTEXPR std::size_t first_child_idx =
        has_first_child  ? count_before(tree_link::first_child)  : std::size_t(-1);
    static D_CONSTEXPR std::size_t next_sibling_idx =
        has_next_sibling ? count_before(tree_link::next_sibling) : std::size_t(-1);
    static D_CONSTEXPR std::size_t parent_idx =
        has_parent       ? count_before(tree_link::parent)       : std::size_t(-1);
    static D_CONSTEXPR std::size_t prev_sibling_idx =
        has_prev_sibling ? count_before(tree_link::prev_sibling) : std::size_t(-1);
    static D_CONSTEXPR std::size_t last_child_idx =
        has_last_child   ? count_before(tree_link::last_child)   : std::size_t(-1);
    static D_CONSTEXPR std::size_t left_idx =
        has_left         ? count_before(tree_link::left)         : std::size_t(-1);
    static D_CONSTEXPR std::size_t right_idx =
        has_right        ? count_before(tree_link::right)        : std::size_t(-1);

    // --- raw flags (for trait inspection) ---

    static D_CONSTEXPR unsigned flags = _Flags;
};


// =============================================================================
// IV.  Preset Link Policies
// =============================================================================

// lcrs_link_policy
//   policy: left-child/right-sibling — the minimal n-ary
// tree encoding.  Two links per node.
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
// O(1) last-child append, O(1) sibling removal, and
// O(1) detach.  The original arena.hpp layout.
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
// V.   arena_node
// =============================================================================
// A fixed-size node within an arena.  Combines policy-driven
// topology links, identity fields for change tracking, and the
// domain-specific payload.

template<typename _Payload,
         typename _LinkPolicy = full_nary_link_policy>
struct arena_node
{
    using payload_type = _Payload;
    using link_policy  = _LinkPolicy;

    static D_CONSTEXPR std::size_t num_links =
        _LinkPolicy::num_links;

    // --------------------------------------------------------
    //  topology
    // --------------------------------------------------------
    // Links stored in a fixed array whose layout is
    // determined entirely by the link policy.  All
    // initialized to null_node.
    node_id links[_LinkPolicy::num_links];

    // --------------------------------------------------------
    //  identity
    // --------------------------------------------------------
    std::uint64_t stable_id;
    std::uint32_t version;

    // --------------------------------------------------------
    //  lifecycle
    // --------------------------------------------------------
    node_id       free_next;   // intrusive free list link
    bool          alive;

    // --------------------------------------------------------
    //  payload
    // --------------------------------------------------------
    _Payload      data;

    // --------------------------------------------------------
    //  constructors
    // --------------------------------------------------------

    // arena_node (default)
    //   constructs an empty, unlinked, dead node.
    arena_node()
        : stable_id(0)
        , version(0)
        , free_next(null_node)
        , alive(false)
        , data()
    {
        for (std::size_t i = 0; i < num_links; ++i)
        {
            links[i] = null_node;
        }
    }

    // arena_node (with identity and payload)
    //   constructs an unlinked, alive node.
    explicit
    arena_node(std::uint64_t   _stable_id,
               const _Payload& _data)
        : stable_id(_stable_id)
        , version(1)
        , free_next(null_node)
        , alive(true)
        , data(_data)
    {
        for (std::size_t i = 0; i < num_links; ++i)
        {
            links[i] = null_node;
        }
    }

    // arena_node (with identity and payload — move)
    explicit
    arena_node(std::uint64_t _stable_id,
               _Payload&&    _data)
        : stable_id(_stable_id)
        , version(1)
        , free_next(null_node)
        , alive(true)
        , data(static_cast<_Payload&&>(_data))
    {
        for (std::size_t i = 0; i < num_links; ++i)
        {
            links[i] = null_node;
        }
    }

    // --------------------------------------------------------
    //  named link accessors (compile-time gated)
    // --------------------------------------------------------
    // Each accessor is only valid when the corresponding
    // flag is present in the policy.  Calling on a policy
    // without the flag is a static_assert failure.

    // get_link
    //   method: returns the link at compile-time index _I.
    template<std::size_t _I>
    D_CONSTEXPR node_id get_link() const
    {
        static_assert(_I < num_links,
                      "Link index out of range.");

        return links[_I];
    }

    // set_link
    //   method: sets the link at compile-time index _I.
    template<std::size_t _I>
    D_CONSTEXPR void set_link(node_id _target)
    {
        static_assert(_I < num_links,
                      "Link index out of range.");

        links[_I] = _target;

        return;
    }

    // --- convenience named accessors ---
    // These forward to the policy-determined index.
    // Disabled links cause static_assert failures.

    D_CONSTEXPR node_id first_child() const
    {
        static_assert(_LinkPolicy::has_first_child,
            "first_child: not in link policy.");
        return links[_LinkPolicy::first_child_idx];
    }

    D_CONSTEXPR node_id last_child() const
    {
        static_assert(_LinkPolicy::has_last_child,
            "last_child: not in link policy.");
        return links[_LinkPolicy::last_child_idx];
    }

    D_CONSTEXPR node_id next_sibling() const
    {
        static_assert(_LinkPolicy::has_next_sibling,
            "next_sibling: not in link policy.");
        return links[_LinkPolicy::next_sibling_idx];
    }

    D_CONSTEXPR node_id prev_sibling() const
    {
        static_assert(_LinkPolicy::has_prev_sibling,
            "prev_sibling: not in link policy.");
        return links[_LinkPolicy::prev_sibling_idx];
    }

    D_CONSTEXPR node_id parent() const
    {
        static_assert(_LinkPolicy::has_parent,
            "parent: not in link policy.");
        return links[_LinkPolicy::parent_idx];
    }

    D_CONSTEXPR node_id left() const
    {
        static_assert(_LinkPolicy::has_left,
            "left: not in link policy.");
        return links[_LinkPolicy::left_idx];
    }

    D_CONSTEXPR node_id right() const
    {
        static_assert(_LinkPolicy::has_right,
            "right: not in link policy.");
        return links[_LinkPolicy::right_idx];
    }

    // --------------------------------------------------------
    //  structural queries
    // --------------------------------------------------------

    // has_children
    //   method: returns true if this node has at least one
    // child.  Dispatches by policy.
    D_CONSTEXPR bool has_children() const
    {
        if constexpr (_LinkPolicy::has_first_child)
        {
            return (first_child() != null_node);
        }
        else if constexpr (_LinkPolicy::has_left)
        {
            return ( (left()  != null_node) ||
                     (right() != null_node) );
        }
        else
        {
            return false;
        }
    }

    // is_leaf
    //   method: returns true if this node has no children.
    D_CONSTEXPR bool is_leaf() const
    {
        return !has_children();
    }

    // reset_links
    //   method: sets all links to null_node.
    D_CONSTEXPR void reset_links()
    {
        for (std::size_t i = 0; i < num_links; ++i)
        {
            links[i] = null_node;
        }

        return;
    }
};


// =============================================================================
// VI.  arena
// =============================================================================
// The generalized arena container.  Manages a flat, contiguous
// array of arena_nodes with policy-driven topology operations,
// identity tracking, and free-list recycling.
//
// Template parameters:
//   _Payload     — the domain-specific data stored per node
//   _LinkPolicy  — determines the per-node link topology
//   _Allocator   — STL allocator for the backing vector

template<typename _Payload,
         typename _LinkPolicy  = full_nary_link_policy,
         typename _Allocator   = std::allocator<
             arena_node<_Payload, _LinkPolicy>>>
class arena
{
public:

    // -----------------------------------------------------------------
    //  type aliases
    // -----------------------------------------------------------------

    using payload_type    = _Payload;
    using link_policy     = _LinkPolicy;
    using node_type       = arena_node<_Payload, _LinkPolicy>;
    using allocator_type  = _Allocator;
    using size_type       = std::size_t;

    // -----------------------------------------------------------------
    //  storage
    // -----------------------------------------------------------------

private:
    using arena_storage = std::vector<node_type, _Allocator>;

    arena_storage   m_nodes;
    node_id         m_free_head;
    size_type       m_live_count;
    std::uint64_t   m_next_stable_id;

public:

    // -----------------------------------------------------------------
    //  constructors / destructor / assignment
    // -----------------------------------------------------------------

    // arena (default)
    //   constructor: creates an empty arena.
    arena()
        : m_nodes()
        , m_free_head(null_node)
        , m_live_count(0)
        , m_next_stable_id(1)
    {
    }

    // arena (with capacity)
    //   constructor: pre-allocates arena storage.
    explicit
    arena(size_type _reserve)
        : m_nodes()
        , m_free_head(null_node)
        , m_live_count(0)
        , m_next_stable_id(1)
    {
        m_nodes.reserve(_reserve);
    }

    // arena (allocator-extended)
    explicit
    arena(const allocator_type& _alloc)
        : m_nodes(_alloc)
        , m_free_head(null_node)
        , m_live_count(0)
        , m_next_stable_id(1)
    {
    }

    // arena (capacity + allocator)
    arena(size_type             _reserve,
          const allocator_type& _alloc)
        : m_nodes(_alloc)
        , m_free_head(null_node)
        , m_live_count(0)
        , m_next_stable_id(1)
    {
        m_nodes.reserve(_reserve);
    }

    // rule-of-five: default.
    arena(const arena&)            = default;
    arena(arena&&) noexcept        = default;
    arena& operator=(const arena&) = default;
    arena& operator=(arena&&) noexcept = default;
    ~arena()                       = default;


    // =================================================================
    //  capacity
    // =================================================================

    // size
    //   method: returns the number of live nodes.
    size_type
    size() const noexcept
    {
        return m_live_count;
    }

    // empty
    //   method: returns true if the arena has no live nodes.
    bool
    empty() const noexcept
    {
        return (m_live_count == 0);
    }

    // arena_size
    //   method: returns total slot count (live + dead).
    size_type
    arena_size() const noexcept
    {
        return m_nodes.size();
    }

    // capacity
    //   method: returns current vector capacity.
    size_type
    capacity() const noexcept
    {
        return m_nodes.capacity();
    }

    // reserve
    //   method: pre-allocates arena storage.
    void
    reserve(size_type _count)
    {
        m_nodes.reserve(_count);

        return;
    }


    // =================================================================
    //  element access
    // =================================================================

    // operator[]
    //   method: indexed access to the node (mutable).
    node_type&
    operator[](node_id _id)
    {
        return m_nodes[_id];
    }

    // operator[] (const)
    const node_type&
    operator[](node_id _id) const
    {
        return m_nodes[_id];
    }

    // node_at
    //   method: named accessor (mutable).
    node_type&
    node_at(node_id _id)
    {
        return m_nodes[_id];
    }

    // node_at (const)
    const node_type&
    node_at(node_id _id) const
    {
        return m_nodes[_id];
    }

    // data
    //   method: payload access (mutable).
    _Payload&
    data(node_id _id)
    {
        return m_nodes[_id].data;
    }

    // data (const)
    const _Payload&
    data(node_id _id) const
    {
        return m_nodes[_id].data;
    }

    // valid
    //   method: returns true if _id references a live node.
    bool
    valid(node_id _id) const noexcept
    {
        return ( (_id < static_cast<node_id>(m_nodes.size())) &&
                 m_nodes[_id].alive );
    }


    // =================================================================
    //  allocation
    // =================================================================

    // allocate
    //   method: acquires a slot from the free list or by
    // extending the arena.  Assigns the next stable_id
    // automatically.  Returns the new node_id.
    node_id
    allocate(const _Payload& _data)
    {
        node_id id = acquire_slot();

        node_type& n = m_nodes[id];
        n.stable_id  = m_next_stable_id++;
        n.version    = 1;
        n.data       = _data;

        return id;
    }

    // allocate (move)
    node_id
    allocate(_Payload&& _data)
    {
        node_id id = acquire_slot();

        node_type& n = m_nodes[id];
        n.stable_id  = m_next_stable_id++;
        n.version    = 1;
        n.data       = static_cast<_Payload&&>(_data);

        return id;
    }

    // allocate (explicit stable_id)
    //   method: allocates with a caller-provided stable_id.
    // Useful for deserialization and DOM reconstruction.
    node_id
    allocate(std::uint64_t   _stable_id,
             const _Payload& _data)
    {
        node_id id = acquire_slot();

        node_type& n = m_nodes[id];
        n.stable_id  = _stable_id;
        n.version    = 1;
        n.data       = _data;

        // keep the counter ahead of any externally
        // supplied id
        if (_stable_id >= m_next_stable_id)
        {
            m_next_stable_id = _stable_id + 1;
        }

        return id;
    }

    // allocate (explicit stable_id, move)
    node_id
    allocate(std::uint64_t _stable_id,
             _Payload&&    _data)
    {
        node_id id = acquire_slot();

        node_type& n = m_nodes[id];
        n.stable_id  = _stable_id;
        n.version    = 1;
        n.data       = static_cast<_Payload&&>(_data);

        if (_stable_id >= m_next_stable_id)
        {
            m_next_stable_id = _stable_id + 1;
        }

        return id;
    }

    // deallocate
    //   method: marks a node as dead and pushes it onto the
    // free list.  Does NOT unlink — caller must detach first.
    void
    deallocate(node_id _id)
    {
        node_type& n = m_nodes[_id];
        n.alive      = false;
        n.free_next  = m_free_head;
        n.reset_links();
        m_free_head  = _id;

        --m_live_count;

        return;
    }


    // =================================================================
    //  topology operations
    // =================================================================
    // All operations are O(1) when the link policy provides
    // sufficient links.  Operations that require missing links
    // fail at compile time via static_assert.

    // append_child
    //   method: links _child as the last child of _parent.
    void
    append_child(node_id _parent,
                 node_id _child)
    {
        static_assert(
            ( link_policy::has_first_child &&
              link_policy::has_next_sibling ),
            "append_child: requires first_child and "
            "next_sibling links.");

        node_type& pn = m_nodes[_parent];
        node_type& cn = m_nodes[_child];

        // set parent on child
        set_parent_if(cn, _parent);

        // clear child's sibling links
        set_next_sibling(cn, null_node);

        if (pn.first_child() == null_node)
        {
            // first child
            set_first_child(pn, _child);
            set_last_child_if(pn, _child);
            set_prev_sibling_if(cn, null_node);
        }
        else
        {
            // find or retrieve last child
            node_id last_id = find_last_child(_parent);

            node_type& ln = m_nodes[last_id];

            set_next_sibling(ln, _child);
            set_prev_sibling_if(cn, last_id);
            set_last_child_if(pn, _child);
        }

        return;
    }

    // prepend_child
    //   method: links _child as the first child of _parent.
    void
    prepend_child(node_id _parent,
                  node_id _child)
    {
        static_assert(
            ( link_policy::has_first_child &&
              link_policy::has_next_sibling ),
            "prepend_child: requires first_child and "
            "next_sibling links.");

        node_type& pn = m_nodes[_parent];
        node_type& cn = m_nodes[_child];

        set_parent_if(cn, _parent);
        set_prev_sibling_if(cn, null_node);

        if (pn.first_child() == null_node)
        {
            set_first_child(pn, _child);
            set_last_child_if(pn, _child);
            set_next_sibling(cn, null_node);
        }
        else
        {
            node_id old_first = pn.first_child();
            node_type& fn = m_nodes[old_first];

            set_prev_sibling_if(fn, _child);
            set_next_sibling(cn, old_first);
            set_first_child(pn, _child);
        }

        return;
    }

    // insert_after
    //   method: links _child as the next sibling of _after.
    void
    insert_after(node_id _after,
                 node_id _child)
    {
        static_assert(link_policy::has_next_sibling,
            "insert_after: requires next_sibling link.");

        node_type& an = m_nodes[_after];
        node_type& cn = m_nodes[_child];

        // inherit parent
        if constexpr (link_policy::has_parent)
        {
            set_parent_if(cn, an.parent());
        }

        set_prev_sibling_if(cn, _after);

        node_id old_next = an.next_sibling();
        set_next_sibling(cn, old_next);

        if (old_next != null_node)
        {
            set_prev_sibling_if(m_nodes[old_next], _child);
        }
        else
        {
            // _child is now the last child
            if constexpr ( link_policy::has_parent &&
                           link_policy::has_last_child )
            {
                node_id p = an.parent();

                if (p != null_node)
                {
                    set_last_child_if(m_nodes[p], _child);
                }
            }
        }

        set_next_sibling(an, _child);

        return;
    }

    // insert_before
    //   method: links _child as the previous sibling of
    // _before.  Requires prev_sibling link for O(1).
    void
    insert_before(node_id _before,
                  node_id _child)
    {
        static_assert(
            ( link_policy::has_next_sibling &&
              link_policy::has_prev_sibling ),
            "insert_before: requires next_sibling and "
            "prev_sibling links.");

        node_type& bn = m_nodes[_before];
        node_type& cn = m_nodes[_child];

        if constexpr (link_policy::has_parent)
        {
            set_parent_if(cn, bn.parent());
        }

        set_next_sibling(cn, _before);

        node_id old_prev = bn.prev_sibling();
        set_prev_sibling_if(cn, old_prev);

        if (old_prev != null_node)
        {
            set_next_sibling(m_nodes[old_prev], _child);
        }
        else
        {
            // _child is now the first child
            if constexpr (link_policy::has_parent)
            {
                node_id p = bn.parent();

                if (p != null_node)
                {
                    set_first_child(m_nodes[p], _child);
                }
            }
        }

        set_prev_sibling_if(bn, _child);

        return;
    }

    // detach
    //   method: unlinks _id from its parent and siblings
    // without deallocating.  The subtree rooted at _id
    // remains intact.
    void
    detach(node_id _id)
    {
        static_assert(
            ( link_policy::has_first_child &&
              link_policy::has_next_sibling ),
            "detach: requires first_child and "
            "next_sibling links.");

        node_type& n = m_nodes[_id];

        node_id parent_id = null_node;

        if constexpr (link_policy::has_parent)
        {
            parent_id = n.parent();
        }

        if constexpr (link_policy::has_prev_sibling)
        {
            // O(1) detach with prev_sibling
            node_id prev = n.prev_sibling();
            node_id next = n.next_sibling();

            if (prev != null_node)
            {
                set_next_sibling(m_nodes[prev], next);
            }
            else if (parent_id != null_node)
            {
                set_first_child(m_nodes[parent_id], next);
            }

            if (next != null_node)
            {
                set_prev_sibling_if(m_nodes[next], prev);
            }
            else if (parent_id != null_node)
            {
                set_last_child_if(m_nodes[parent_id], prev);
            }
        }
        else
        {
            // O(k) detach: walk sibling chain from parent
            if (parent_id != null_node)
            {
                detach_child_linear(parent_id, _id);
            }
        }

        // clear _id's external links
        set_next_sibling(n, null_node);
        set_prev_sibling_if(n, null_node);
        set_parent_if(n, null_node);

        return;
    }

    // move_subtree
    //   method: detaches _id and appends it as the last
    // child of _new_parent.  O(1) with full links.
    void
    move_subtree(node_id _id,
                 node_id _new_parent)
    {
        detach(_id);
        append_child(_new_parent, _id);

        return;
    }


    // =================================================================
    //  subtree collection
    // =================================================================

    // collect_subtree
    //   method: gathers all descendants of _root (including
    // _root) in breadth-first order.
    void
    collect_subtree(node_id               _root,
                    std::vector<node_id>& _out) const
    {
        static_assert(
            ( link_policy::has_first_child &&
              link_policy::has_next_sibling ),
            "collect_subtree: requires first_child and "
            "next_sibling links.");

        _out.push_back(_root);

        std::size_t scan = _out.size() - 1;

        while (scan < _out.size())
        {
            node_id child =
                m_nodes[_out[scan]].first_child();

            while (child != null_node)
            {
                _out.push_back(child);
                child = m_nodes[child].next_sibling();
            }

            ++scan;
        }

        return;
    }

    // collect_children
    //   method: gathers the immediate children of _parent.
    void
    collect_children(node_id               _parent,
                     std::vector<node_id>& _out) const
    {
        static_assert(
            ( link_policy::has_first_child &&
              link_policy::has_next_sibling ),
            "collect_children: requires first_child and "
            "next_sibling links.");

        node_id child = m_nodes[_parent].first_child();

        while (child != null_node)
        {
            _out.push_back(child);
            child = m_nodes[child].next_sibling();
        }

        return;
    }

    // collect_ancestors
    //   method: gathers all ancestors of _id (not including _id).
    void
    collect_ancestors(node_id               _id,
                      std::vector<node_id>& _out) const
    {
        static_assert(link_policy::has_parent,
            "collect_ancestors: requires parent link.");

        node_id current = m_nodes[_id].parent();

        while (current != null_node)
        {
            _out.push_back(current);
            current = m_nodes[current].parent();
        }

        return;
    }

    // depth
    //   method: returns the depth of _id (root = 0).
    std::size_t
    depth(node_id _id) const
    {
        static_assert(link_policy::has_parent,
            "depth: requires parent link.");

        std::size_t d       = 0;
        node_id     current = m_nodes[_id].parent();

        while (current != null_node)
        {
            ++d;
            current = m_nodes[current].parent();
        }

        return d;
    }

    // child_count
    //   method: returns the number of immediate children.
    std::size_t
    child_count(node_id _parent) const
    {
        static_assert(
            ( link_policy::has_first_child &&
              link_policy::has_next_sibling ),
            "child_count: requires first_child and "
            "next_sibling links.");

        std::size_t count = 0;
        node_id     child = m_nodes[_parent].first_child();

        while (child != null_node)
        {
            ++count;
            child = m_nodes[child].next_sibling();
        }

        return count;
    }


    // =================================================================
    //  traversal
    // =================================================================

    // visit_depth_first
    //   method: invokes _fn(node_id, depth) in pre-order.
    template<typename _Fn>
    void
    visit_depth_first(node_id _root,
                      _Fn     _fn) const
    {
        static_assert(
            ( link_policy::has_first_child &&
              link_policy::has_next_sibling ),
            "visit_depth_first: requires first_child and "
            "next_sibling links.");

        struct frame
        {
            node_id     id;
            std::size_t depth;
        };

        std::vector<frame> stack;
        stack.push_back({ _root, 0 });

        while (!stack.empty())
        {
            frame f = stack.back();
            stack.pop_back();

            _fn(f.id, f.depth);

            // push children in reverse for correct order
            std::vector<node_id> children;
            collect_children(f.id, children);

            for (std::size_t i = children.size();
                 i > 0;
                 --i)
            {
                stack.push_back({
                    children[i - 1],
                    f.depth + 1
                });
            }
        }

        return;
    }

    // visit_breadth_first
    //   method: invokes _fn(node_id, depth) in BFS order.
    template<typename _Fn>
    void
    visit_breadth_first(node_id _root,
                        _Fn     _fn) const
    {
        static_assert(
            ( link_policy::has_first_child &&
              link_policy::has_next_sibling ),
            "visit_breadth_first: requires first_child "
            "and next_sibling links.");

        struct frame
        {
            node_id     id;
            std::size_t depth;
        };

        std::vector<frame> queue;
        queue.push_back({ _root, 0 });

        std::size_t scan = 0;

        while (scan < queue.size())
        {
            frame f = queue[scan];
            ++scan;

            _fn(f.id, f.depth);

            node_id child =
                m_nodes[f.id].first_child();

            while (child != null_node)
            {
                queue.push_back({
                    child,
                    f.depth + 1
                });
                child = m_nodes[child].next_sibling();
            }
        }

        return;
    }


    // =================================================================
    //  identity / versioning
    // =================================================================

    // bump_version
    //   method: increments the version counter on _id.
    void
    bump_version(node_id _id)
    {
        ++m_nodes[_id].version;

        return;
    }

    // stable_id
    //   method: returns the stable_id of a node.
    std::uint64_t
    stable_id(node_id _id) const
    {
        return m_nodes[_id].stable_id;
    }

    // version
    //   method: returns the version counter of a node.
    std::uint32_t
    version(node_id _id) const
    {
        return m_nodes[_id].version;
    }

    // next_stable_id
    //   method: returns the next stable_id that will be
    // assigned.  Useful for serialization.
    std::uint64_t
    next_stable_id() const noexcept
    {
        return m_next_stable_id;
    }

    // set_next_stable_id
    //   method: overrides the counter.  Use with care
    // (deserialization).
    void
    set_next_stable_id(std::uint64_t _id) noexcept
    {
        m_next_stable_id = _id;

        return;
    }


    // =================================================================
    //  bulk operations
    // =================================================================

    // clear
    //   method: destroys all nodes and resets the arena.
    void
    clear()
    {
        m_nodes.clear();
        m_free_head      = null_node;
        m_live_count     = 0;
        m_next_stable_id = 1;

        return;
    }

    // swap
    //   method: exchanges contents with another arena.
    void
    swap(arena& _other) noexcept
    {
        m_nodes.swap(_other.m_nodes);

        { node_id t          = m_free_head;
          m_free_head        = _other.m_free_head;
          _other.m_free_head = t; }

        { size_type t          = m_live_count;
          m_live_count         = _other.m_live_count;
          _other.m_live_count  = t; }

        { std::uint64_t t            = m_next_stable_id;
          m_next_stable_id           = _other.m_next_stable_id;
          _other.m_next_stable_id    = t; }

        return;
    }


    // =================================================================
    //  allocator access
    // =================================================================

    allocator_type
    get_allocator() const noexcept
    {
        return m_nodes.get_allocator();
    }


    // =================================================================
    //  raw storage access (serialization, advanced use)
    // =================================================================

    const arena_storage&
    storage() const noexcept
    {
        return m_nodes;
    }

    arena_storage&
    storage() noexcept
    {
        return m_nodes;
    }


    // =================================================================
    //  comparison
    // =================================================================

    friend bool
    operator==(const arena& _a,
               const arena& _b) noexcept
    {
        return ( _a.m_live_count == _b.m_live_count &&
                 _a.m_nodes     == _b.m_nodes );
    }

    friend bool
    operator!=(const arena& _a,
               const arena& _b) noexcept
    {
        return !(_a == _b);
    }


    // =================================================================
    //  private helpers
    // =================================================================

private:

    // --- slot management ---

    // acquire_slot
    //   helper: obtains a slot from the free list or by
    // extending the vector.
    node_id
    acquire_slot()
    {
        node_id id;

        if (m_free_head != null_node)
        {
            // recycle
            id = m_free_head;
            m_free_head = m_nodes[id].free_next;

            node_type& n = m_nodes[id];
            n.reset_links();
            n.free_next = null_node;
            n.alive     = true;
            n.version   = 0;
        }
        else
        {
            // extend
            id = static_cast<node_id>(m_nodes.size());
            m_nodes.emplace_back();
            m_nodes[id].alive = true;
        }

        ++m_live_count;

        return id;
    }

    // --- conditional link setters ---
    // These write to the appropriate link slot via the
    // policy index.  The `_if` variants are no-ops when
    // the link is absent from the policy.

    static void
    set_first_child(node_type& _n,
                    node_id    _target) noexcept
    {
        _n.links[link_policy::first_child_idx] = _target;

        return;
    }

    static void
    set_next_sibling(node_type& _n,
                     node_id    _target) noexcept
    {
        _n.links[link_policy::next_sibling_idx] = _target;

        return;
    }

    static void
    set_last_child_if(node_type& _n,
                      node_id    _target) noexcept
    {
        if constexpr (link_policy::has_last_child)
        {
            _n.links[link_policy::last_child_idx] = _target;
        }

        return;
    }

    static void
    set_prev_sibling_if(node_type& _n,
                        node_id    _target) noexcept
    {
        if constexpr (link_policy::has_prev_sibling)
        {
            _n.links[link_policy::prev_sibling_idx] = _target;
        }

        return;
    }

    static void
    set_parent_if(node_type& _n,
                  node_id    _target) noexcept
    {
        if constexpr (link_policy::has_parent)
        {
            _n.links[link_policy::parent_idx] = _target;
        }

        return;
    }

    // --- find_last_child ---
    // Uses last_child link if available, otherwise walks.

    node_id
    find_last_child(node_id _parent) const noexcept
    {
        if constexpr (link_policy::has_last_child)
        {
            return m_nodes[_parent].last_child();
        }
        else
        {
            node_id cur = m_nodes[_parent].first_child();
            node_id last = cur;

            while (cur != null_node)
            {
                last = cur;
                cur = m_nodes[cur].next_sibling();
            }

            return last;
        }
    }

    // --- detach_child_linear ---
    // O(k) fallback when prev_sibling is absent.

    void
    detach_child_linear(node_id _parent_id,
                        node_id _child_id) noexcept
    {
        node_type& pn = m_nodes[_parent_id];
        node_id fc    = pn.first_child();
        node_id next  = m_nodes[_child_id].next_sibling();

        if (fc == _child_id)
        {
            set_first_child(pn, next);
        }
        else
        {
            // walk to find predecessor
            node_id prev = fc;

            while ( prev != null_node &&
                    m_nodes[prev].next_sibling() != _child_id )
            {
                prev = m_nodes[prev].next_sibling();
            }

            if (prev != null_node)
            {
                set_next_sibling(m_nodes[prev], next);
            }
        }

        // update last_child if applicable
        if constexpr (link_policy::has_last_child)
        {
            if (pn.last_child() == _child_id)
            {
                // find new last child
                node_id cur  = pn.first_child();
                node_id last = null_node;

                while (cur != null_node)
                {
                    last = cur;
                    cur  = m_nodes[cur].next_sibling();
                }

                set_last_child_if(pn, last);
            }
        }

        return;
    }
};


// =============================================================================
// VII. Convenience Aliases
// =============================================================================

// default_arena
//   alias: arena with full n-ary links.  Matches the
// original arena.hpp layout — O(1) everything.
template<typename _Payload>
using default_arena = arena<_Payload, full_nary_link_policy>;

// lcrs_arena
//   alias: minimal 2-link arena.  Smallest node size.
template<typename _Payload>
using lcrs_arena = arena<_Payload, lcrs_link_policy>;

// parented_arena
//   alias: 3-link arena with parent back-pointer.
template<typename _Payload>
using parented_arena = arena<_Payload, parented_lcrs_link_policy>;


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_ARENA_
