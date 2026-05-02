
/******************************************************************************
* djinterp [container]                                      nary_tree_node.hpp
*
* N-ary tree node:
*   LCRS-based n-ary tree node carrying parent, first_child,
* last_child, next_sibling, and prev_sibling links.  The five-pointer
* layout enables O(1) append, prepend, detach, insert_after, and
* insert_before - the full set of operations advertised by the
* framework's `o1_*_nary_tree` concepts.
*
*   Built on `linked_node<_Type, 5, nary_tree_node<_Type>*>`: the
* fixed-slot link array, constructors, and `data()` / `null_link()`
* utilities are reused from linked_node, while this header layers
* named link accessors and structural queries on top.
*
*   The link type is fixed to `nary_tree_node<_Type>*` rather than
* the linked_node default of `self*`.  Doing so causes resolve_self_t
* to leave the type alone, so `link_type` resolves to the actual
* derived class and callers never need a downcast to navigate.  The
* trade-off is that this node form is locked to raw-pointer linking;
* arena-backed (index-handle) and smart-pointer variants will live in
* their own headers when needed, mirroring the same layout.
*
* TEMPLATE PARAMETERS:
*   _Type - user-facing payload type.
*
* SLOT ASSIGNMENTS (stable across the framework):
*   0 - parent
*   1 - first_child
*   2 - last_child
*   3 - next_sibling
*   4 - prev_sibling
*
* PUBLIC INTERFACE:
*   - data()                      payload access (from linked_node)
*   - parent(), first_child(),    typed link reads, const-correct
*     last_child(), next_sibling(),
*     prev_sibling()
*   - is_leaf(), is_root()        structural queries
*   - child_count()               O(child_count) sibling walk
*   - null_link()                 sentinel value (from linked_node)
*
* PRIVATE INTERFACE (friended to nary_tree):
*   - parent_slot(), first_child_slot(),
*     last_child_slot(), next_sibling_slot(),
*     prev_sibling_slot()         mutable slot references for the
*                                 tree's link helpers
*
* 
* path:      /inc/djinterp/core/container/tree/nary/nary_tree_node.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.28
******************************************************************************/

#ifndef DJINTERP_CONTAINER_NARY_TREE_NODE_
#define DJINTERP_CONTAINER_NARY_TREE_NODE_ 1

// std
#include <cstddef>
// djinterp
#include "../../../djinterp.hpp"
#include "../../node/linked_node.hpp"


NS_DJINTERP

    // forward declaration so the base specialisation can name the
    // derived class as its link type
    template<typename _Type>
    class nary_tree_node;

    // forward declaration of nary_tree to friend it for slot
    // access; the four template parameters mirror nary_tree.hpp's
    // signature
    template<typename _ValueType,
             typename _Allocator,
             typename _LockPolicy,
             typename _OwnershipPolicy>
    class nary_tree;


    NS_INTERNAL

        // nary_link_index
        //   enum: stable slot indices for the LCRS topology.
        // Names match the public link accessors and are used
        // throughout nary_tree's link helpers.
        enum nary_link_index : std::size_t
        {
            nary_link_parent       = 0,
            nary_link_first_child  = 1,
            nary_link_last_child   = 2,
            nary_link_next_sibling = 3,
            nary_link_prev_sibling = 4,

            nary_link_count        = 5
        };

    NS_END  // internal


    // =========================================================================
    // I.   N-ARY TREE NODE
    // =========================================================================

    // nary_tree_node
    //   class: LCRS n-ary tree node built on linked_node<T, 5>.
    // Storage and constructors are reused from the base; this
    // wrapper adds typed, const-correct link accessors and the
    // structural queries the trait system probes for.
    template<typename _Type>
    class nary_tree_node
        : private linked_node<_Type,
                              internal::nary_link_count,
                              nary_tree_node<_Type>*>
    {
    private:
        using base = linked_node<_Type,
                                 internal::nary_link_count,
                                 nary_tree_node<_Type>*>;

        // tree is friended so its link helpers can write the
        // mutable slot references without leaking those into the
        // public API
        template<typename, typename, typename, typename>
        friend class nary_tree;

    public:
        // -----------------------------------------------------------------
        // re-exported aliases
        // -----------------------------------------------------------------

        using typename base::value_type;
        using typename base::edges_type;
        using typename base::size_type;

        using node_type        = nary_tree_node;
        using link_type        = nary_tree_node*;
        using const_link_type  = const nary_tree_node*;


        // -----------------------------------------------------------------
        // constructors / destructor / assignment
        // -----------------------------------------------------------------

        // forward all of linked_node's constructors (default,
        // value-copy, value-move, value+edges)
        using base::base;

        // copy and move are deleted at the node level - the tree
        // owns node lifetime and link integrity, so a silent copy
        // would tear the topology apart
        nary_tree_node(const nary_tree_node&)            = delete;
        nary_tree_node(nary_tree_node&&)                 = delete;
        nary_tree_node& operator=(const nary_tree_node&) = delete;
        nary_tree_node& operator=(nary_tree_node&&)      = delete;

        ~nary_tree_node() = default;


        // -----------------------------------------------------------------
        // payload access
        // -----------------------------------------------------------------

        using base::data;


        // -----------------------------------------------------------------
        // typed link accessors - read-only, const-correct
        // -----------------------------------------------------------------

        // parent
        //   returns the parent link, or null if this node is a
        // root.
        D_CONSTEXPR link_type
        parent() noexcept
        {
            return base::template get_node<internal::nary_link_parent>();
        }

        D_CONSTEXPR const_link_type
        parent() const noexcept
        {
            return base::template get_node<internal::nary_link_parent>();
        }

        // first_child
        //   returns the leftmost child link, or null if this node
        // is a leaf.
        D_CONSTEXPR link_type
        first_child() noexcept
        {
            return base::template get_node<
                internal::nary_link_first_child>();
        }

        D_CONSTEXPR const_link_type
        first_child() const noexcept
        {
            return base::template get_node<
                internal::nary_link_first_child>();
        }

        // last_child
        //   returns the rightmost child link.  Maintained eagerly
        // by nary_tree's link helpers so that O(1) append is
        // available.
        D_CONSTEXPR link_type
        last_child() noexcept
        {
            return base::template get_node<
                internal::nary_link_last_child>();
        }

        D_CONSTEXPR const_link_type
        last_child() const noexcept
        {
            return base::template get_node<
                internal::nary_link_last_child>();
        }

        // next_sibling
        //   returns the immediate right sibling, or null at the
        // tail of the sibling chain.
        D_CONSTEXPR link_type
        next_sibling() noexcept
        {
            return base::template get_node<
                internal::nary_link_next_sibling>();
        }

        D_CONSTEXPR const_link_type
        next_sibling() const noexcept
        {
            return base::template get_node<
                internal::nary_link_next_sibling>();
        }

        // prev_sibling
        //   returns the immediate left sibling, or null at the
        // head of the sibling chain.
        D_CONSTEXPR link_type
        prev_sibling() noexcept
        {
            return base::template get_node<
                internal::nary_link_prev_sibling>();
        }

        D_CONSTEXPR const_link_type
        prev_sibling() const noexcept
        {
            return base::template get_node<
                internal::nary_link_prev_sibling>();
        }


        // -----------------------------------------------------------------
        // structural queries
        // -----------------------------------------------------------------

        // is_leaf
        //   returns true when no children are linked.
        D_CONSTEXPR bool
        is_leaf() const noexcept
        {
            return (first_child() == nullptr);
        }

        // is_root
        //   returns true when no parent is linked.
        D_CONSTEXPR bool
        is_root() const noexcept
        {
            return (parent() == nullptr);
        }

        // child_count
        //   counts direct children by walking the sibling chain
        // from first_child.  O(child_count); for amortised O(1)
        // counts a derived class can cache the value externally.
        D_CONSTEXPR size_type
        child_count() const noexcept
        {
            size_type        n;
            const_link_type  c;

            n = 0;
            c = first_child();

            while (c != nullptr)
            {
                ++n;
                c = c->next_sibling();
            }

            return n;
        }


        // -----------------------------------------------------------------
        // utilities
        // -----------------------------------------------------------------

        using base::null_link;


    private:
        // -----------------------------------------------------------------
        // mutable slot references - only nary_tree (friend) uses
        // these to splice nodes in and out of the topology
        // -----------------------------------------------------------------

        D_CONSTEXPR link_type&
        parent_slot() noexcept
        {
            return base::template get_node<
                internal::nary_link_parent>();
        }

        D_CONSTEXPR link_type&
        first_child_slot() noexcept
        {
            return base::template get_node<
                internal::nary_link_first_child>();
        }

        D_CONSTEXPR link_type&
        last_child_slot() noexcept
        {
            return base::template get_node<
                internal::nary_link_last_child>();
        }

        D_CONSTEXPR link_type&
        next_sibling_slot() noexcept
        {
            return base::template get_node<
                internal::nary_link_next_sibling>();
        }

        D_CONSTEXPR link_type&
        prev_sibling_slot() noexcept
        {
            return base::template get_node<
                internal::nary_link_prev_sibling>();
        }
    };


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_NARY_TREE_NODE_