/******************************************************************************
* djinterp [container]                                           nary_tree.hpp
*
* N-ary tree container:
*   A heap-allocated, owning n-ary tree using the LCRS (left-child /
* right-sibling) topology with auxiliary parent, last_child, and
* prev_sibling links.  The five-pointer node layout enables O(1)
* append, prepend, detach, insert_after, and insert_before - the full
* set of operations advertised by the framework's `o1_*_nary_tree`
* concepts.
*
*   Inherits `tree_container` for entry-point storage, size tracking,
* allocator management, and Rule-of-Five behavior.  This file adds
* the n-ary topology, hierarchical access (children / parent / depth),
* a forward LCRS pre-order iterator, and the standard subtree
* mutators.
*
* CLASSIFICATION (axes 1–8):
*   1. lifetime       - mutable_storage (default; const& gives the
*                       immutable view for free)
*   2. iteration      - forward (begin/end/cbegin/cend over LCRS
*                       pre-order)
*   3. ordering       - ordered (children preserve insertion order;
*                       no key_compare, no hasher)
*   4. bounds         - unbounded (max_size sourced from allocator;
*                       size_interval / depth_interval intentionally
*                       absent)
*   5. multiplicity   - multi (duplicates allowed; no key_type)
*   6. structure      - hierarchical (node_type, depth_type, root,
*                       parent, children, depth - the headline axis)
*   7. storage        - dynamic (allocator_type, heap-allocated nodes)
*   8. thread safety  - policy-driven via _LockPolicy; exposed as
*                       lock_policy (re-exported from base)
*
* TEMPLATE PARAMETERS:
*   _ValueType        - user-facing element type
*   _Allocator        - node allocator (default std::allocator<node_type>)
*   _LockPolicy       - threading policy (default void = no locking)
*   _OwnershipPolicy  - entry point ownership (default
*                       unique_owning_policy - move-only RAII)
*
* TABLE OF CONTENTS
* =================
* I.    N-ary Tree Node
* II.   Sibling Range View
* III.  LCRS Pre-Order Iterator
* IV.   N-ary Tree
* V.    Convenience Aliases
*
*
* path:      /inc/djinterp/core/container/tree/nary/nary_tree_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2025.05.22
******************************************************************************/

#ifndef DJINTERP_CONTAINER_NARY_TREE_
#define DJINTERP_CONTAINER_NARY_TREE_ 1

#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include "../../../djinterp.hpp"
#include "../tree_container.hpp"


NS_DJINTERP

    // =========================================================================
    // I.   N-ARY TREE NODE
    // =========================================================================

    // nary_tree_node
    //   class: LCRS-based n-ary tree node carrying parent,
    // first_child, last_child, next_sibling, and prev_sibling
    // links.  The five-pointer layout enables O(1) append,
    // prepend, detach, insert_after, and insert_before.  Method
    // accessors are provided so node_traits can detect each link
    // via the `has_<x>_method` family.
    template<typename _ValueType>
    class nary_tree_node
    {
    public:
        using value_type = _ValueType;
        using node_type  = nary_tree_node;

        // -----------------------------------------------------------------
        // constructors
        // -----------------------------------------------------------------

        D_CONSTEXPR
        nary_tree_node()
            : m_data{},
              m_parent(nullptr),
              m_first_child(nullptr),
              m_last_child(nullptr),
              m_next_sibling(nullptr),
              m_prev_sibling(nullptr)
        {}

        D_CONSTEXPR explicit
        nary_tree_node(
            const value_type& _value
        )
            : m_data(_value),
              m_parent(nullptr),
              m_first_child(nullptr),
              m_last_child(nullptr),
              m_next_sibling(nullptr),
              m_prev_sibling(nullptr)
        {}

        D_CONSTEXPR explicit
        nary_tree_node(
            value_type&& _value
        )
            : m_data(std::move(_value)),
              m_parent(nullptr),
              m_first_child(nullptr),
              m_last_child(nullptr),
              m_next_sibling(nullptr),
              m_prev_sibling(nullptr)
        {}

        // copy/move are deleted at the node level - the tree owns
        // node lifetime and link integrity, so silent copies of a
        // single node would tear the topology apart
        nary_tree_node(const nary_tree_node&)            = delete;
        nary_tree_node(nary_tree_node&&)                 = delete;
        nary_tree_node& operator=(const nary_tree_node&) = delete;
        nary_tree_node& operator=(nary_tree_node&&)      = delete;

        ~nary_tree_node() = default;


        // -----------------------------------------------------------------
        // data access
        // -----------------------------------------------------------------

        D_CONSTEXPR value_type&
        data() noexcept
        {
            return m_data;
        }

        D_CONSTEXPR const value_type&
        data() const noexcept
        {
            return m_data;
        }


        // -----------------------------------------------------------------
        // link access (method form - probed by node_traits)
        // -----------------------------------------------------------------

        D_CONSTEXPR node_type*
        parent() noexcept
        {
            return m_parent;
        }

        D_CONSTEXPR const node_type*
        parent() const noexcept
        {
            return m_parent;
        }

        D_CONSTEXPR node_type*
        first_child() noexcept
        {
            return m_first_child;
        }

        D_CONSTEXPR const node_type*
        first_child() const noexcept
        {
            return m_first_child;
        }

        D_CONSTEXPR node_type*
        last_child() noexcept
        {
            return m_last_child;
        }

        D_CONSTEXPR const node_type*
        last_child() const noexcept
        {
            return m_last_child;
        }

        D_CONSTEXPR node_type*
        next_sibling() noexcept
        {
            return m_next_sibling;
        }

        D_CONSTEXPR const node_type*
        next_sibling() const noexcept
        {
            return m_next_sibling;
        }

        D_CONSTEXPR node_type*
        prev_sibling() noexcept
        {
            return m_prev_sibling;
        }

        D_CONSTEXPR const node_type*
        prev_sibling() const noexcept
        {
            return m_prev_sibling;
        }


        // -----------------------------------------------------------------
        // structural queries
        // -----------------------------------------------------------------

        D_CONSTEXPR bool
        is_leaf() const noexcept
        {
            return (m_first_child == nullptr);
        }

        D_CONSTEXPR bool
        is_root() const noexcept
        {
            return (m_parent == nullptr);
        }

        // child_count
        //   counts the number of direct children by walking the
        // first_child / next_sibling chain.  O(child_count); the
        // tree class provides the same answer in O(1) when the
        // count is needed often, by tracking a per-parent counter
        // externally if profiling demands it
        D_CONSTEXPR std::size_t
        child_count() const noexcept
        {
            std::size_t n;
            const node_type* c;

            n = 0;
            c = m_first_child;

            while (c != nullptr)
            {
                ++n;
                c = c->m_next_sibling;
            }

            return n;
        }


        // -----------------------------------------------------------------
        // friend declaration - only the tree mutates link fields
        // -----------------------------------------------------------------

        template<typename, typename, typename, typename>
        friend class nary_tree;

    private:
        value_type m_data;
        node_type* m_parent;
        node_type* m_first_child;
        node_type* m_last_child;
        node_type* m_next_sibling;
        node_type* m_prev_sibling;
    };


    // =========================================================================
    // II.  SIBLING RANGE VIEW
    // =========================================================================
    //   children(node) returns one of these - a forward range over
    // the LCRS sibling chain starting at node->first_child.  Holding
    // the view by value is fine; it stores a single pointer.

    NS_INTERNAL

        // sibling_iterator
        //   class: forward iterator over the next_sibling chain
        // beginning at a given node.  The end sentinel is the
        // default-constructed iterator (m_current == nullptr).
        template<typename _NodeType>
        class sibling_iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type        = typename std::remove_const<
                _NodeType>::type::value_type;
            using difference_type   = std::ptrdiff_t;
            using reference         = typename std::conditional<
                std::is_const<_NodeType>::value,
                const value_type&,
                value_type&>::type;
            using pointer           = typename std::conditional<
                std::is_const<_NodeType>::value,
                const value_type*,
                value_type*>::type;

            D_CONSTEXPR
            sibling_iterator() noexcept
                : m_current(nullptr)
            {}

            D_CONSTEXPR explicit
            sibling_iterator(
                _NodeType* _start
            ) noexcept
                : m_current(_start)
            {}

            D_CONSTEXPR reference
            operator*() const noexcept
            {
                return m_current->data();
            }

            D_CONSTEXPR pointer
            operator->() const noexcept
            {
                return &m_current->data();
            }

            // node
            //   returns the underlying node pointer, useful for
            // structural operations on the visited child
            D_CONSTEXPR _NodeType*
            node() const noexcept
            {
                return m_current;
            }

            D_CONSTEXPR sibling_iterator&
            operator++() noexcept
            {
                m_current = m_current->next_sibling();

                return *this;
            }

            D_CONSTEXPR sibling_iterator
            operator++(int) noexcept
            {
                sibling_iterator tmp;

                tmp       = *this;
                m_current = m_current->next_sibling();

                return tmp;
            }

            D_CONSTEXPR friend bool
            operator==(
                const sibling_iterator& _a,
                const sibling_iterator& _b
            ) noexcept
            {
                return (_a.m_current == _b.m_current);
            }

            D_CONSTEXPR friend bool
            operator!=(
                const sibling_iterator& _a,
                const sibling_iterator& _b
            ) noexcept
            {
                return !(_a == _b);
            }

        private:
            _NodeType* m_current;
        };

    NS_END  // internal


    // sibling_range
    //   class: lightweight forward range over an LCRS sibling
    // chain.  Holds a single node pointer and exposes
    // begin/end/empty.  Returned by nary_tree::children(node).
    template<typename _NodeType>
    class sibling_range
    {
    public:
        using node_type     = _NodeType;
        using value_type    = typename std::remove_const<
            _NodeType>::type::value_type;
        using iterator      = internal::sibling_iterator<_NodeType>;
        using const_iterator =
            internal::sibling_iterator<const _NodeType>;

        D_CONSTEXPR
        sibling_range() noexcept
            : m_first(nullptr)
        {}

        D_CONSTEXPR explicit
        sibling_range(
            _NodeType* _first
        ) noexcept
            : m_first(_first)
        {}

        D_CONSTEXPR iterator
        begin() const noexcept
        {
            return iterator(m_first);
        }

        D_CONSTEXPR iterator
        end() const noexcept
        {
            return iterator();
        }

        D_CONSTEXPR const_iterator
        cbegin() const noexcept
        {
            return const_iterator(m_first);
        }

        D_CONSTEXPR const_iterator
        cend() const noexcept
        {
            return const_iterator();
        }

        D_CONSTEXPR bool
        empty() const noexcept
        {
            return (m_first == nullptr);
        }

    private:
        _NodeType* m_first;
    };


    // =========================================================================
    // III. LCRS PRE-ORDER ITERATOR
    // =========================================================================
    //   Forward iterator that performs depth-first pre-order
    // traversal over an LCRS subtree.  State is a stack of pending
    // sibling pointers - at most one per ancestor - so memory is
    // bounded by the tree depth, not the node count.
    //
    //   Subtree-anchored: the iterator never advances past the
    // siblings of the start node, so iterating from any node yields
    // exactly that subtree.  This matches the conventional
    // begin()/end() contract where end() is the sentinel reached
    // after the last node in the subtree.

    NS_INTERNAL

        // nary_pre_order_iterator
        //   class: stack-based forward pre-order iterator over an
        // LCRS subtree rooted at a given node.
        template<typename _NodeType>
        class nary_pre_order_iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type        = typename std::remove_const<
                _NodeType>::type::value_type;
            using difference_type   = std::ptrdiff_t;
            using reference         = typename std::conditional<
                std::is_const<_NodeType>::value,
                const value_type&,
                value_type&>::type;
            using pointer           = typename std::conditional<
                std::is_const<_NodeType>::value,
                const value_type*,
                value_type*>::type;

            // -----------------------------------------------------------------
            // constructors
            // -----------------------------------------------------------------

            D_CONSTEXPR
            nary_pre_order_iterator() noexcept
                : m_current(nullptr),
                  m_subtree_root(nullptr),
                  m_pending()
            {}

            D_CONSTEXPR explicit
            nary_pre_order_iterator(
                _NodeType* _root
            )
                : m_current(_root),
                  m_subtree_root(_root),
                  m_pending()
            {}

            // -----------------------------------------------------------------
            // element access
            // -----------------------------------------------------------------

            D_CONSTEXPR reference
            operator*() const noexcept
            {
                return m_current->data();
            }

            D_CONSTEXPR pointer
            operator->() const noexcept
            {
                return &m_current->data();
            }

            // node
            //   returns the underlying node pointer (advanced uses
            // such as detach mid-traversal must invalidate the
            // iterator afterwards - same contract as STL
            // associative containers)
            D_CONSTEXPR _NodeType*
            node() const noexcept
            {
                return m_current;
            }


            // -----------------------------------------------------------------
            // traversal
            // -----------------------------------------------------------------

            nary_pre_order_iterator&
            operator++()
            {
                advance();

                return *this;
            }

            nary_pre_order_iterator
            operator++(int)
            {
                nary_pre_order_iterator tmp;

                tmp = *this;
                advance();

                return tmp;
            }


            // -----------------------------------------------------------------
            // comparison
            // -----------------------------------------------------------------

            D_CONSTEXPR friend bool
            operator==(
                const nary_pre_order_iterator& _a,
                const nary_pre_order_iterator& _b
            ) noexcept
            {
                return (_a.m_current == _b.m_current);
            }

            D_CONSTEXPR friend bool
            operator!=(
                const nary_pre_order_iterator& _a,
                const nary_pre_order_iterator& _b
            ) noexcept
            {
                return !(_a == _b);
            }

        private:
            // advance
            //   moves m_current to the next node in pre-order.
            // Three rules, applied in order:
            //   1. descend into first_child if present (after
            //      remembering the next_sibling unless we are at
            //      the subtree root, whose siblings lie outside
            //      our scope),
            //   2. otherwise step laterally to next_sibling,
            //   3. otherwise pop a remembered sibling from the
            //      stack - or, if empty, signal end of traversal.
            void
            advance()
            {
                _NodeType* child;

                if (m_current == nullptr)
                {
                    return;
                }

                child = m_current->first_child();

                // rule 1: descend
                if (child != nullptr)
                {
                    if ( (m_current != m_subtree_root) &&
                         (m_current->next_sibling() != nullptr) )
                    {
                        m_pending.push_back(m_current->next_sibling());
                    }

                    m_current = child;

                    return;
                }

                // rule 2: lateral step (only within the subtree)
                if ( (m_current != m_subtree_root) &&
                     (m_current->next_sibling() != nullptr) )
                {
                    m_current = m_current->next_sibling();

                    return;
                }

                // rule 3: unwind via the pending stack
                if (!m_pending.empty())
                {
                    m_current = m_pending.back();
                    m_pending.pop_back();

                    return;
                }

                m_current = nullptr;

                return;
            }

            _NodeType*              m_current;
            _NodeType*              m_subtree_root;
            std::vector<_NodeType*> m_pending;
        };

    NS_END  // internal


    // =========================================================================
    // IV.  N-ARY TREE
    // =========================================================================

    // nary_tree
    //   class: heap-allocated, owning n-ary tree using the LCRS
    // topology with auxiliary parent / last_child / prev_sibling
    // links.  Inherits tree_container for entry-point storage,
    // size tracking, allocator management, and Rule-of-Five
    // behavior; layers hierarchical access, forward iteration, and
    // O(1) subtree mutation on top.
    template<typename _ValueType,
             typename _Allocator       =
                 std::allocator<nary_tree_node<_ValueType>>,
             typename _LockPolicy      = void,
             typename _OwnershipPolicy = unique_owning_policy>
    class nary_tree
        : public tree_container<_ValueType,
                                nary_tree_node<_ValueType>,
                                _Allocator,
                                _LockPolicy,
                                _OwnershipPolicy>
    {
    private:
        using base = tree_container<_ValueType,
                                    nary_tree_node<_ValueType>,
                                    _Allocator,
                                    _LockPolicy,
                                    _OwnershipPolicy>;

        using alloc_traits = std::allocator_traits<_Allocator>;

    public:
        // -----------------------------------------------------------------
        // re-exported aliases (axes 1, 6, 7, 8)
        // -----------------------------------------------------------------

        using typename base::value_type;
        using typename base::node_type;
        using typename base::allocator_type;
        using typename base::lock_policy;
        using typename base::ownership_policy;
        using typename base::size_type;
        using typename base::difference_type;
        using typename base::depth_type;
        using typename base::reference;
        using typename base::const_reference;
        using typename base::pointer;
        using typename base::const_pointer;

        // -----------------------------------------------------------------
        // iterator aliases (axis 2 - forward)
        // -----------------------------------------------------------------

        using iterator =
            internal::nary_pre_order_iterator<node_type>;
        using const_iterator =
            internal::nary_pre_order_iterator<const node_type>;

        using sibling_range_type       = sibling_range<node_type>;
        using const_sibling_range_type = sibling_range<const node_type>;


        // -----------------------------------------------------------------
        // constructors - forward to base
        // -----------------------------------------------------------------

        using base::base;


        // -----------------------------------------------------------------
        // move semantics
        // -----------------------------------------------------------------
        //
        //   nary_tree must declare its move operations EXPLICITLY.
        // The user-declared destructor below suppresses implicit
        // move generation per the Rule of Five, and the default
        // unique_owning_policy deletes the implicit copy
        // operations - leaving the type both non-copyable and
        // non-movable if no explicit move is provided.  That
        // breaks every consumer that returns an nary_tree by
        // value (test_tree, factory functions, return-by-value
        // builders, etc.).
        //
        //   The implementation forwards to the base's move
        // operations.  The base manages entry_point + size +
        // allocator and is presumed to implement move correctly
        // for those members; the derived class therefore needs
        // only to wire the forwarding through.
        //
        //   Move-from state: the source's entry pointer is
        // released (transferred to *this) and its size is reset
        // to zero.  The source remains a valid empty nary_tree
        // whose destructor's destroy_all() walks zero nodes -
        // no double-free, no undefined behaviour.

        nary_tree(
            nary_tree&& _other
        ) noexcept
            : base(static_cast<base&&>(_other))
        {}

        nary_tree&
        operator=(
            nary_tree&& _other
        ) noexcept
        {
            if (this != &_other)
            {
                // tear down our own contents first; destroy_all
                // is a no-op if we were already empty.
                destroy_all();

                // hand the base's move-assignment the source's
                // entry/size/allocator state.
                base::operator=(static_cast<base&&>(_other));
            }

            return *this;
        }


        // -----------------------------------------------------------------
        // copy semantics - explicitly deleted
        // -----------------------------------------------------------------
        //
        //   With the default unique_owning_policy a tree owns
        // the entire node graph; copying would require a deep
        // walk plus per-node clone, which is not the framework's
        // intent.  The implicit copy operations were already
        // deleted by the policy; we make the deletion explicit
        // so the diagnostic at a misuse site reads
        // "use of deleted function" rather than the longer
        // "implicitly deleted because of policy".
        //
        //   Note: this declaration applies only to the default
        // unique_owning_policy instantiation in spirit.  Other
        // policies (shared, non-owning) may legitimately want
        // copyable trees; if you need that, instantiate
        // nary_tree with the appropriate policy and provide a
        // policy-aware copy via a separate clone() method
        // rather than reaching for operator=.

        nary_tree(const nary_tree&)            = delete;
        nary_tree& operator=(const nary_tree&) = delete;


        // -----------------------------------------------------------------
        // destructor - sweeps the entire owned graph
        // -----------------------------------------------------------------

        ~nary_tree()
        {
            destroy_all();
        }


        // -----------------------------------------------------------------
        // hierarchy access (axis 6)
        // -----------------------------------------------------------------

        // root
        //   inherited as base::root() - returns the entry node or
        // nullptr; tree_container provides both mutable and const
        // overloads.
        using base::root;
        using base::has_root;

        // parent
        //   returns the parent of _node, or nullptr if _node is
        // the root or null.
        D_CONSTEXPR node_type*
        parent(
            node_type* _node
        ) const noexcept
        {
            return ( (_node != nullptr) ? _node->parent()
                                        : nullptr );
        }

        D_CONSTEXPR const node_type*
        parent(
            const node_type* _node
        ) const noexcept
        {
            return ( (_node != nullptr) ? _node->parent()
                                        : nullptr );
        }

        // children
        //   returns a forward range over _node's direct children.
        // An empty range is returned for null or leaf nodes.
        D_CONSTEXPR sibling_range_type
        children(
            node_type* _node
        ) noexcept
        {
            return sibling_range_type(
                (_node != nullptr) ? _node->first_child()
                                   : nullptr);
        }

        D_CONSTEXPR const_sibling_range_type
        children(
            const node_type* _node
        ) const noexcept
        {
            return const_sibling_range_type(
                (_node != nullptr) ? _node->first_child()
                                   : nullptr);
        }

        // children
        //   no-arg overload: children of the root.  Allows the
        // tree itself to be queried for its top-level fan-out
        // without first dereferencing root().
        D_CONSTEXPR sibling_range_type
        children() noexcept
        {
            return children(base::entry_point());
        }

        D_CONSTEXPR const_sibling_range_type
        children() const noexcept
        {
            return const_sibling_range_type(
                (base::entry_point() != nullptr)
                    ? base::entry_point()->first_child()
                    : nullptr);
        }

        // depth
        //   returns the depth of _node measured from the root
        // (root depth == 0).  Walks the parent chain - O(depth).
        depth_type
        depth(
            const node_type* _node
        ) const noexcept
        {
            depth_type       d;
            const node_type* p;

            if (_node == nullptr)
            {
                return depth_type{};
            }

            d = depth_type{};
            p = _node->parent();

            while (p != nullptr)
            {
                ++d;
                p = p->parent();
            }

            return d;
        }


        // -----------------------------------------------------------------
        // iteration (axis 2 - forward, pre-order)
        // -----------------------------------------------------------------

        D_CONSTEXPR iterator
        begin() noexcept
        {
            return iterator(base::entry_point());
        }

        D_CONSTEXPR iterator
        end() noexcept
        {
            return iterator();
        }

        D_CONSTEXPR const_iterator
        begin() const noexcept
        {
            return const_iterator(base::entry_point());
        }

        D_CONSTEXPR const_iterator
        end() const noexcept
        {
            return const_iterator();
        }

        D_CONSTEXPR const_iterator
        cbegin() const noexcept
        {
            return const_iterator(base::entry_point());
        }

        D_CONSTEXPR const_iterator
        cend() const noexcept
        {
            return const_iterator();
        }


        // -----------------------------------------------------------------
        // capacity (axis 4 - unbounded; bound is allocator's max)
        // -----------------------------------------------------------------

        using base::empty;
        using base::size;
        using base::max_size;


        // -----------------------------------------------------------------
        // mutation: root creation
        // -----------------------------------------------------------------

        // emplace_root
        //   constructs a root node from _args and installs it.
        // Any prior tree is discarded via clear().  Returns the
        // new root pointer.  The size counter is reset to 1.
        template<typename... _Args>
        node_type*
        emplace_root(
            _Args&&... _args
        )
        {
            node_type* n;

            clear();
            n = make_node(std::forward<_Args>(_args)...);

            base::set_entry(n);
            base::set_size(1);

            return n;
        }


        // -----------------------------------------------------------------
        // mutation: child insertion (O(1) - axis-aligned)
        // -----------------------------------------------------------------

        // append_child
        //   constructs a new node from _args and links it as
        // _parent's last child.  O(1).  Returns the new node.
        template<typename... _Args>
        node_type*
        append_child(
            node_type* _parent,
            _Args&&...  _args
        )
        {
            node_type* n;

            n = make_node(std::forward<_Args>(_args)...);
            link_as_last_child(_parent, n);
            base::increment_size();

            return n;
        }

        // prepend_child
        //   constructs a new node from _args and links it as
        // _parent's first child.  O(1).  Returns the new node.
        template<typename... _Args>
        node_type*
        prepend_child(
            node_type* _parent,
            _Args&&...  _args
        )
        {
            node_type* n;

            n = make_node(std::forward<_Args>(_args)...);
            link_as_first_child(_parent, n);
            base::increment_size();

            return n;
        }


        // -----------------------------------------------------------------
        // mutation: sibling insertion (O(1) - axis-aligned)
        // -----------------------------------------------------------------

        // insert_after
        //   constructs a new node from _args and links it as
        // _sibling's immediate next sibling.  _sibling must not
        // be the root.  O(1).  Returns the new node.
        template<typename... _Args>
        node_type*
        insert_after(
            node_type* _sibling,
            _Args&&...  _args
        )
        {
            node_type* n;

            n = make_node(std::forward<_Args>(_args)...);
            link_as_next_sibling(_sibling, n);
            base::increment_size();

            return n;
        }

        // insert_before
        //   constructs a new node from _args and links it as
        // _sibling's immediate previous sibling.  _sibling must
        // not be the root.  O(1).  Returns the new node.
        template<typename... _Args>
        node_type*
        insert_before(
            node_type* _sibling,
            _Args&&...  _args
        )
        {
            node_type* n;

            n = make_node(std::forward<_Args>(_args)...);
            link_as_prev_sibling(_sibling, n);
            base::increment_size();

            return n;
        }


        // -----------------------------------------------------------------
        // mutation: structural moves
        // -----------------------------------------------------------------

        // detach
        //   unlinks _node from its parent and siblings, leaving
        // its own subtree intact.  Caller takes ownership of the
        // returned pointer.  O(1).  Detaching the root yields the
        // entry pointer and resets the tree to empty.
        node_type*
        detach(
            node_type* _node
        )
        {
            if (_node == nullptr)
            {
                return nullptr;
            }

            // Special case: detaching the root collapses the
            // tree.  release_entry already zeros the size
            // counter, so no separate accounting is needed.
            if (_node == base::entry_point())
            {
                return base::release_entry();
            }

            unlink_node(_node);
            base::set_size(base::size() - subtree_size(_node));

            return _node;
        }

        // remove_subtree
        //   detaches _node and destroys it together with all of
        // its descendants.  Equivalent to detach() followed by a
        // sweep through every reachable descendant.
        void
        remove_subtree(
            node_type* _node
        )
        {
            node_type* detached;

            if (_node == nullptr)
            {
                return;
            }

            detached = detach(_node);
            destroy_subtree(detached);

            return;
        }

        // move_subtree
        //   re-parents _node onto _new_parent as the new last
        // child.  Validates against cycles (re-parenting onto
        // self or any descendant).  O(depth) for the cycle check,
        // O(1) for the relink.
        bool
        move_subtree(
            node_type* _node,
            node_type* _new_parent
        )
        {
            const node_type* p;

            if ( (_node       == nullptr) ||
                 (_new_parent == nullptr) ||
                 (_node       == _new_parent) )
            {
                return false;
            }

            // refuse if _new_parent lies in the moving subtree
            p = _new_parent;

            while (p != nullptr)
            {
                if (p == _node)
                {
                    return false;
                }

                p = p->parent();
            }

            // root moves are not supported through this path
            if (_node == base::entry_point())
            {
                return false;
            }

            unlink_node(_node);
            link_as_last_child(_new_parent, _node);

            return true;
        }


        // -----------------------------------------------------------------
        // bulk operations
        // -----------------------------------------------------------------

        // clear
        //   destroys every node in the tree and resets the entry
        // and size to empty.  Always safe; no-op on an empty tree.
        void
        clear() noexcept
        {
            destroy_all();

            return;
        }


    private:
        // -----------------------------------------------------------------
        // node construction / destruction
        // -----------------------------------------------------------------

        template<typename... _Args>
        node_type*
        make_node(
            _Args&&... _args
        )
        {
            allocator_type a = base::get_allocator();
            node_type*     n = alloc_traits::allocate(a, 1);

            alloc_traits::construct(a,
                                    n,
                                    std::forward<_Args>(_args)...);

            return n;
        }

        void
        destroy_node(
            node_type* _node
        ) noexcept
        {
            allocator_type a;

            if (_node == nullptr)
            {
                return;
            }

            a = base::get_allocator();
            alloc_traits::destroy(a, _node);
            alloc_traits::deallocate(a, _node, 1);

            return;
        }

        // destroy_subtree
        //   iteratively destroys _node and all descendants.  Uses
        // an explicit work stack to keep stack depth bounded by
        // the heap, not the tree depth, which matters for
        // pathologically deep trees.
        void
        destroy_subtree(
            node_type* _node
        ) noexcept
        {
            std::vector<node_type*> stack;
            node_type*              n;
            node_type*              c;
            node_type*              next;

            if (_node == nullptr)
            {
                return;
            }

            stack.push_back(_node);

            while (!stack.empty())
            {
                n = stack.back();
                stack.pop_back();

                // push children before destroying the parent
                c = n->first_child();

                while (c != nullptr)
                {
                    next = c->next_sibling();
                    stack.push_back(c);
                    c = next;
                }

                destroy_node(n);
            }

            return;
        }

        // destroy_all
        //   destroys every node owned by this tree.  Two paths:
        //     - unique_owning_policy: release the head from the
        //       unique_ptr (which does NOT destroy it) and sweep
        //       the entire graph through the allocator.  This is
        //       the only ownership mode that cleans descendants
        //       reliably.
        //     - any other policy: defer to base::clear().  For
        //       non_owning the caller is responsible for node
        //       lifetime.  For shared_owning the head's refcount
        //       is decremented; if we held the last reference the
        //       head is destroyed but raw-pointer descendants
        //       leak - that combination is not recommended for
        //       this node design and the user should pick
        //       unique_owning_policy if they want automatic
        //       cleanup.
        void
        destroy_all() noexcept
        {
            node_type* r;

            if constexpr (
                std::is_same<_OwnershipPolicy,
                             unique_owning_policy>::value)
            {
                r = base::entry_point();

                if (r != nullptr)
                {
                    base::release_entry();
                    destroy_subtree(r);
                }

                base::set_size(0);
            }
            else
            {
                base::clear();
            }

            return;
        }


        // -----------------------------------------------------------------
        // link helpers (private; preserve all five LCRS pointers)
        // -----------------------------------------------------------------

        static void
        link_as_first_child(
            node_type* _parent,
            node_type* _child
        ) noexcept
        {
            node_type* old_first;

            old_first = _parent->m_first_child;

            _child->m_parent       = _parent;
            _child->m_prev_sibling = nullptr;
            _child->m_next_sibling = old_first;

            if (old_first != nullptr)
            {
                old_first->m_prev_sibling = _child;
            }
            else
            {
                _parent->m_last_child = _child;
            }

            _parent->m_first_child = _child;

            return;
        }

        static void
        link_as_last_child(
            node_type* _parent,
            node_type* _child
        ) noexcept
        {
            node_type* old_last;

            old_last = _parent->m_last_child;

            _child->m_parent       = _parent;
            _child->m_prev_sibling = old_last;
            _child->m_next_sibling = nullptr;

            if (old_last != nullptr)
            {
                old_last->m_next_sibling = _child;
            }
            else
            {
                _parent->m_first_child = _child;
            }

            _parent->m_last_child = _child;

            return;
        }

        static void
        link_as_next_sibling(
            node_type* _anchor,
            node_type* _new_node
        ) noexcept
        {
            node_type* old_next;
            node_type* p;

            old_next = _anchor->m_next_sibling;
            p        = _anchor->m_parent;

            _new_node->m_parent       = p;
            _new_node->m_prev_sibling = _anchor;
            _new_node->m_next_sibling = old_next;

            _anchor->m_next_sibling = _new_node;

            if (old_next != nullptr)
            {
                old_next->m_prev_sibling = _new_node;
            }
            else if (p != nullptr)
            {
                p->m_last_child = _new_node;
            }

            return;
        }

        static void
        link_as_prev_sibling(
            node_type* _anchor,
            node_type* _new_node
        ) noexcept
        {
            node_type* old_prev;
            node_type* p;

            old_prev = _anchor->m_prev_sibling;
            p        = _anchor->m_parent;

            _new_node->m_parent       = p;
            _new_node->m_prev_sibling = old_prev;
            _new_node->m_next_sibling = _anchor;

            _anchor->m_prev_sibling = _new_node;

            if (old_prev != nullptr)
            {
                old_prev->m_next_sibling = _new_node;
            }
            else if (p != nullptr)
            {
                p->m_first_child = _new_node;
            }

            return;
        }

        // unlink_node
        //   removes _node from its current parent / sibling
        // links, leaving its own subtree intact.  All four
        // affected pointers (parent's first/last and the two
        // siblings' next/prev) are repaired.  O(1).
        static void
        unlink_node(
            node_type* _node
        ) noexcept
        {
            node_type* p;
            node_type* prev;
            node_type* next;

            p    = _node->m_parent;
            prev = _node->m_prev_sibling;
            next = _node->m_next_sibling;

            if (prev != nullptr)
            {
                prev->m_next_sibling = next;
            }
            else if (p != nullptr)
            {
                p->m_first_child = next;
            }

            if (next != nullptr)
            {
                next->m_prev_sibling = prev;
            }
            else if (p != nullptr)
            {
                p->m_last_child = prev;
            }

            _node->m_parent       = nullptr;
            _node->m_prev_sibling = nullptr;
            _node->m_next_sibling = nullptr;

            return;
        }

        // subtree_size
        //   counts the number of nodes in _node's subtree
        // (inclusive).  O(subtree_size).  Used by detach() to
        // keep the tree's size counter consistent.
        static size_type
        subtree_size(
            const node_type* _node
        ) noexcept
        {
            std::vector<const node_type*> stack;
            const node_type*              n;
            const node_type*              c;
            size_type                     n_count;

            if (_node == nullptr)
            {
                return 0;
            }

            n_count = 0;
            stack.push_back(_node);

            while (!stack.empty())
            {
                n = stack.back();
                stack.pop_back();
                ++n_count;

                c = n->first_child();

                while (c != nullptr)
                {
                    stack.push_back(c);
                    c = c->next_sibling();
                }
            }

            return n_count;
        }
    };


    // =========================================================================
    // V.   CONVENIENCE ALIASES
    // =========================================================================

    // owning_nary_tree
    //   alias: nary_tree with unique_owning_policy.  Move-only
    // RAII semantics.  This is the same as the default - provided
    // for symmetry with node_container's family of aliases.
    template<typename _ValueType,
             typename _Allocator  =
                 std::allocator<nary_tree_node<_ValueType>>,
             typename _LockPolicy = void>
    using owning_nary_tree =
        nary_tree<_ValueType,
                  _Allocator,
                  _LockPolicy,
                  unique_owning_policy>;

    // shared_nary_tree
    //   alias: nary_tree with shared_owning_policy.  Copyable;
    // multiple trees may share refcounted ownership of a graph.
    template<typename _ValueType,
             typename _Allocator  =
                 std::allocator<nary_tree_node<_ValueType>>,
             typename _LockPolicy = void>
    using shared_nary_tree =
        nary_tree<_ValueType,
                  _Allocator,
                  _LockPolicy,
                  shared_owning_policy>;

    // non_owning_nary_tree
    //   alias: nary_tree with non_owning_policy.  The container
    // does not destroy nodes; useful when nodes live in an
    // external arena or pool.
    template<typename _ValueType,
             typename _Allocator  =
                 std::allocator<nary_tree_node<_ValueType>>,
             typename _LockPolicy = void>
    using non_owning_nary_tree =
        nary_tree<_ValueType,
                  _Allocator,
                  _LockPolicy,
                  non_owning_policy>;


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_NARY_TREE_