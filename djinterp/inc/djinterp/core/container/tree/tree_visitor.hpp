/******************************************************************************
* djinterp [container]                                       tree_visitor.hpp
*
* Tree Visitor Module:
*   Provides comprehensive, version-portable visitor infrastructure for
* tree containers. Integrates with tree_container (root, node_type),
* tree_iterator (traversal tags, tree_cursor), and tree_container_traits
* (topology detection) to deliver traversal-order-parameterized visitation,
* depth-aware callbacks, accumulating (fold) visitors, filtered visitation,
* and binary-tree-specific in-order traversal.
*
*   DISPATCH STRATEGY:
*   All traversal logic structurally adapts to the node shape at compile
* time. Binary nodes (left/right) and n-ary nodes (first_child/next_sibling
* or children()) are dispatched via SFINAE (C++11) or if constexpr (C++17).
* No tag types, no RTTI, no virtual overhead in the traversal core.
*
*   PORTABILITY:
*   - C++11  : tree_visitor_base, tree_visit free functions (stack/queue),
*              depth_visitor, accumulating_visitor, filtered_visitor
*   - C++14  : generic lambda support in visit_tree_with
*   - C++17  : if constexpr dispatch, visit_tree (lambda overload sets),
*              structured bindings in depth-aware callbacks
*   - C++20  : concept-constrained tree_visitor_for, tree_visitable
*
*   TRAVERSAL ORDERS:
*   Uses tags from tree_iterator.hpp:
*     pre_order, post_order, level_order, leaf_order
*   Adds:
*     in_order  (binary trees only)
*
* path:      /inc/container/tree_visitor.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.08
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    CONFIGURATION & NODE SHAPE DETECTION
      -------------------------------------
      i.    feature gates
      ii.   node shape traits (internal)
            a. has_left_child_method
            b. has_right_child_method
            c. has_first_child_method
            d. has_next_sibling_method
            e. has_parent_method
            f. has_children_method
            g. has_child_count_method
            h. has_is_leaf_method
            i. has_data_method
            j. is_binary_node_shape
            k. is_nary_node_shape

II.   TRAVERSAL TAGS
      ---------------
      i.    in_order (binary-only addition)
      ii.   traversal_tag traits (internal)

III.  TREE VISITOR BASE (C++11+)
      ----------------------------
      i.    tree_visitor_base
      ii.   tree_node_visitor (CRTP)

IV.   TRAVERSAL CORE (C++11+)
      -------------------------
      i.    tree_visit  (pre_order)
      ii.   tree_visit  (in_order, binary only)
      iii.  tree_visit  (post_order)
      iv.   tree_visit  (level_order)
      v.    tree_visit  (leaf_order)
      vi.   tree_visit  (container overloads)

V.    DEPTH-AWARE VISITATION (C++11+)
      ---------------------------------
      i.    tree_visit_with_depth

VI.   ACCUMULATING VISITOR (C++11+)
      --------------------------------
      i.    tree_accumulate

VII.  FILTERED VISITATION (C++11+)
      -------------------------------
      i.    tree_visit_if

VIII. VISITOR ADAPTERS (C++14+)
      ---------------------------
      i.    tree_for_each (lambda convenience)

IX.   LAMBDA VISITOR (C++17+)
      --------------------------
      i.    tree_visit overloads with overloaded{} support

X.    CONCEPT-CONSTRAINED VISITATION (C++20+)
      ------------------------------------------
      i.    tree_node_visitor_for (concept)
      ii.   tree_visitable (concept)
      iii.  depth_callback (concept)
*/

#ifndef DJINTERP_CONTAINER_TREE_VISITOR_
#define DJINTERP_CONTAINER_TREE_VISITOR_ 1

#include <type_traits>
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "tree_container.hpp"
#include "tree_container_traits.hpp"
#include "tree_iterator.hpp"

#if D_ENV_LANG_IS_CPP11_OR_HIGHER
    #include <utility>
    #include <functional>
#endif

#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #include <queue>
#endif


NS_DJINTERP


///////////////////////////////////////////////////////////////////////////////
///        I.    CONFIGURATION & NODE SHAPE DETECTION                       ///
///////////////////////////////////////////////////////////////////////////////

// i.   feature gates
//////////////////////////////////////////

// D_TREE_VISITOR_HAS_IF_CONSTEXPR
//   macro: 1 if if-constexpr dispatch is available (C++17+).
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
    #define D_TREE_VISITOR_HAS_IF_CONSTEXPR 1
#else
    #define D_TREE_VISITOR_HAS_IF_CONSTEXPR 0
#endif

// D_TREE_VISITOR_HAS_CONCEPTS
//   macro: 1 if concepts are available (C++20+).
#if D_ENV_LANG_IS_CPP20_OR_HIGHER
    #define D_TREE_VISITOR_HAS_CONCEPTS 1
#else
    #define D_TREE_VISITOR_HAS_CONCEPTS 0
#endif


// ii.  node shape traits (internal)
//////////////////////////////////////////

NS_INTERNAL

    // has_left_child_method
    //   trait: detects _Node::left() accessor.
    template<typename _Node,
             typename = void>
    struct has_left_child_method : std::false_type
    {};

    template<typename _Node>
    struct has_left_child_method<_Node, D_VOID_T<
        decltype(std::declval<const _Node>().left())
    >> : std::true_type
    {};

    // has_right_child_method
    //   trait: detects _Node::right() accessor.
    template<typename _Node,
             typename = void>
    struct has_right_child_method : std::false_type
    {};

    template<typename _Node>
    struct has_right_child_method<_Node, D_VOID_T<
        decltype(std::declval<const _Node>().right())
    >> : std::true_type
    {};

    // has_first_child_method
    //   trait: detects _Node::first_child() accessor.
    template<typename _Node,
             typename = void>
    struct has_first_child_method : std::false_type
    {};

    template<typename _Node>
    struct has_first_child_method<_Node, D_VOID_T<
        decltype(std::declval<const _Node>().first_child())
    >> : std::true_type
    {};

    // has_next_sibling_method
    //   trait: detects _Node::next_sibling() accessor.
    template<typename _Node,
             typename = void>
    struct has_next_sibling_method : std::false_type
    {};

    template<typename _Node>
    struct has_next_sibling_method<_Node, D_VOID_T<
        decltype(std::declval<const _Node>().next_sibling())
    >> : std::true_type
    {};

    // has_parent_method
    //   trait: detects _Node::parent() accessor.
    template<typename _Node,
             typename = void>
    struct has_parent_method : std::false_type
    {};

    template<typename _Node>
    struct has_parent_method<_Node, D_VOID_T<
        decltype(std::declval<const _Node>().parent())
    >> : std::true_type
    {};

    // has_children_method
    //   trait: detects _Node::children() accessor (range-returning).
    template<typename _Node,
             typename = void>
    struct has_children_method : std::false_type
    {};

    template<typename _Node>
    struct has_children_method<_Node, D_VOID_T<
        decltype(std::begin(std::declval<const _Node>().children())),
        decltype(std::end(std::declval<const _Node>().children()))
    >> : std::true_type
    {};

    // has_child_count_method
    //   trait: detects _Node::child_count() accessor.
    template<typename _Node,
             typename = void>
    struct has_child_count_method : std::false_type
    {};

    template<typename _Node>
    struct has_child_count_method<_Node, D_VOID_T<
        decltype(std::declval<const _Node>().child_count())
    >> : std::true_type
    {};

    // has_is_leaf_method
    //   trait: detects _Node::is_leaf() query.
    template<typename _Node,
             typename = void>
    struct has_is_leaf_method : std::false_type
    {};

    template<typename _Node>
    struct has_is_leaf_method<_Node, D_VOID_T<
        decltype(std::declval<const _Node>().is_leaf())
    >> : std::true_type
    {};

    // has_data_method
    //   trait: detects _Node::data() accessor.
    template<typename _Node,
             typename = void>
    struct has_data_method : std::false_type
    {};

    template<typename _Node>
    struct has_data_method<_Node, D_VOID_T<
        decltype(std::declval<const _Node>().data())
    >> : std::true_type
    {};

    // is_binary_node_shape
    //   trait: true if node has left() and right() — binary topology.
    template<typename _Node>
    struct is_binary_node_shape
    {
        static constexpr bool value =
            ( has_left_child_method<_Node>::value &&
              has_right_child_method<_Node>::value );
    };

    // is_nary_node_shape
    //   trait: true if node has first_child()/next_sibling() or children()
    // — n-ary topology.
    template<typename _Node>
    struct is_nary_node_shape
    {
        static constexpr bool value =
            ( (has_first_child_method<_Node>::value &&
               has_next_sibling_method<_Node>::value) ||
              has_children_method<_Node>::value );
    };

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename _Node>
    constexpr bool is_binary_node_shape_v =
        is_binary_node_shape<_Node>::value;

    template<typename _Node>
    constexpr bool is_nary_node_shape_v =
        is_nary_node_shape<_Node>::value;

#endif

    // =====================================================================
    // node_is_leaf_check
    //   function: compile-time adaptive leaf test. Uses is_leaf() when
    // available; otherwise infers from child pointers.
    // =====================================================================

#if D_TREE_VISITOR_HAS_IF_CONSTEXPR

    template<typename _Node>
    D_CONSTEXPR_INLINE bool
    node_is_leaf(
        const _Node* _node
    ) noexcept
    {
        if (!_node)
        {
            return true;
        }

        if constexpr (has_is_leaf_method<_Node>::value)
        {
            return _node->is_leaf();
        }
        else if constexpr (is_binary_node_shape<_Node>::value)
        {
            return ( (_node->left()  == nullptr) &&
                     (_node->right() == nullptr) );
        }
        else if constexpr (has_first_child_method<_Node>::value)
        {
            return (_node->first_child() == nullptr);
        }
        else if constexpr (has_children_method<_Node>::value)
        {
            return (std::begin(_node->children()) ==
                    std::end(_node->children()));
        }
        else
        {
            return true;
        }
    }

#else  // C++11/14 — SFINAE overloads

    // node_is_leaf — has is_leaf() method
    template<typename _Node>
    D_CONSTEXPR_INLINE
    typename std::enable_if<has_is_leaf_method<_Node>::value, bool>::type
    node_is_leaf(
        const _Node* _node
    ) noexcept
    {
        if (!_node)
        {
            return true;
        }

        return _node->is_leaf();
    }

    // node_is_leaf — binary fallback
    template<typename _Node>
    D_CONSTEXPR_INLINE
    typename std::enable_if<
        ( !has_is_leaf_method<_Node>::value &&
          is_binary_node_shape<_Node>::value ), bool>::type
    node_is_leaf(
        const _Node* _node
    ) noexcept
    {
        if (!_node)
        {
            return true;
        }

        return ( (_node->left()  == nullptr) &&
                 (_node->right() == nullptr) );
    }

    // node_is_leaf — n-ary first_child fallback
    template<typename _Node>
    D_CONSTEXPR_INLINE
    typename std::enable_if<
        ( !has_is_leaf_method<_Node>::value    &&
          !is_binary_node_shape<_Node>::value  &&
          has_first_child_method<_Node>::value ), bool>::type
    node_is_leaf(
        const _Node* _node
    ) noexcept
    {
        if (!_node)
        {
            return true;
        }

        return (_node->first_child() == nullptr);
    }

#endif  // D_TREE_VISITOR_HAS_IF_CONSTEXPR

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                  II.   TRAVERSAL TAGS                                   ///
///////////////////////////////////////////////////////////////////////////////

// in_order
//   tag: tag type for in-order tree traversal (Left, Root, Right).
// Only valid for binary trees; a static_assert fires otherwise.
struct in_order
{};

NS_INTERNAL

    // is_traversal_tag
    //   trait: evaluates to true for recognised traversal tag types.
    template<typename _Tag>
    struct is_traversal_tag : std::false_type
    {};

    template<> struct is_traversal_tag<pre_order>   : std::true_type {};
    template<> struct is_traversal_tag<in_order>    : std::true_type {};
    template<> struct is_traversal_tag<post_order>  : std::true_type {};
    template<> struct is_traversal_tag<level_order> : std::true_type {};
    template<> struct is_traversal_tag<leaf_order>  : std::true_type {};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    template<typename _Tag>
    constexpr bool is_traversal_tag_v = is_traversal_tag<_Tag>::value;
#endif

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///             III.  TREE VISITOR BASE (C++11+)                            ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// tree_visitor_base
//   class: abstract base for tree node visitors. Parameterized on the
// node type and return type. Provides a single virtual visit() entry
// point invoked once per node during traversal.
template<typename _Node,
         typename _ReturnType = void>
class tree_visitor_base
{
public:
    using node_type   = _Node;
    using return_type = _ReturnType;

    virtual ~tree_visitor_base()
    {}

    virtual _ReturnType visit(_Node& _node) = 0;
    virtual _ReturnType visit(const _Node& _node) = 0;
};

// tree_node_visitor
//   class: CRTP base for zero-overhead tree visitors. _Derived
// implements visit() overloads and the traversal engine calls
// apply() which statically dispatches.
//
// Usage:
//   class print_visitor
//       : public tree_node_visitor<print_visitor, my_node>
//   {
//   public:
//       void visit(const my_node& n) { std::cout << n.data(); }
//   };
template<typename _Derived,
         typename _Node>
class tree_node_visitor
{
public:
    template<typename _N>
    auto apply(_N& _node)
        -> decltype(static_cast<_Derived*>(this)->visit(_node))
    {
        return static_cast<_Derived*>(this)->visit(_node);
    }

    template<typename _N>
    auto apply(const _N& _node)
        -> decltype(static_cast<_Derived*>(this)->visit(_node))
    {
        return static_cast<_Derived*>(this)->visit(_node);
    }

    template<typename _N>
    auto apply(const _N& _node) const
        -> decltype(static_cast<const _Derived*>(this)->visit(_node))
    {
        return static_cast<const _Derived*>(this)->visit(_node);
    }
};

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///              IV.   TRAVERSAL CORE (C++11+)                             ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_CPP11_OR_HIGHER

// =========================================================================
// IV.i  PRE-ORDER TRAVERSAL
// =========================================================================
//   Iterative stack-based. Adapts to binary vs. n-ary at compile time.

NS_INTERNAL

    // pre_order_visit_binary
    //   function: pre-order DFS over binary nodes (left/right).
    template<typename _Node,
             typename _Visitor>
    inline void
    pre_order_visit_binary(
        _Node*    _root,
        _Visitor& _visitor
    )
    {
        if (!_root)
        {
            return;
        }

        // manual stack using a simple vector to avoid <stack> dependency
        _Node* stack[256];
        std::size_t top = 0;
        stack[top++]    = _root;

        while (top > 0)
        {
            _Node* current = stack[--top];
            _visitor.visit(*current);

            // push right first so left is processed first
            if (current->right())
            {
                stack[top++] = current->right();
            }

            if (current->left())
            {
                stack[top++] = current->left();
            }
        }

        return;
    }

    // pre_order_visit_nary_sibling
    //   function: pre-order DFS over n-ary nodes (first_child/next_sibling).
    template<typename _Node,
             typename _Visitor>
    inline void
    pre_order_visit_nary_sibling(
        _Node*    _root,
        _Visitor& _visitor
    )
    {
        if (!_root)
        {
            return;
        }

        _Node* stack[256];
        std::size_t top = 0;
        stack[top++]    = _root;

        while (top > 0)
        {
            _Node* current = stack[--top];
            _visitor.visit(*current);

            // push siblings in reverse so leftmost child is visited first;
            // collect children then push in reverse
            _Node* children[64];
            std::size_t child_count = 0;
            _Node* child = current->first_child();

            while (child)
            {
                children[child_count++] = child;
                child                   = child->next_sibling();
            }

            // push in reverse order
            for (std::size_t i = child_count; i > 0; --i)
            {
                stack[top++] = children[i - 1];
            }
        }

        return;
    }

NS_END  // internal

// tree_visit<pre_order>
//   function: visits every node of a tree rooted at _root in pre-order.
// Dispatches to binary or n-ary traversal based on node shape.
template<typename _Order,
         typename _Node,
         typename _Visitor>
inline
typename std::enable_if<std::is_same<_Order, pre_order>::value>::type
tree_visit(
    _Node*    _root,
    _Visitor& _visitor
)
{
#if D_TREE_VISITOR_HAS_IF_CONSTEXPR
    if constexpr (internal::is_binary_node_shape<_Node>::value)
    {
        internal::pre_order_visit_binary(_root,
                                         _visitor);
    }
    else
    {
        internal::pre_order_visit_nary_sibling(_root,
                                               _visitor);
    }
#else
    // C++11/14 — tag dispatch via overloads
    tree_visit_pre_order_dispatch(_root,
                                  _visitor,
                                  std::integral_constant<bool,
                                      internal::is_binary_node_shape<_Node>::value>{});
#endif

    return;
}

#if (!D_TREE_VISITOR_HAS_IF_CONSTEXPR)

    // tree_visit_pre_order_dispatch — binary
    template<typename _Node,
             typename _Visitor>
    inline void
    tree_visit_pre_order_dispatch(
        _Node*          _root,
        _Visitor&       _visitor,
        std::true_type  /* is_binary */
    )
    {
        internal::pre_order_visit_binary(_root,
                                         _visitor);

        return;
    }

    // tree_visit_pre_order_dispatch — n-ary
    template<typename _Node,
             typename _Visitor>
    inline void
    tree_visit_pre_order_dispatch(
        _Node*          _root,
        _Visitor&       _visitor,
        std::false_type /* is_binary */
    )
    {
        internal::pre_order_visit_nary_sibling(_root,
                                               _visitor);

        return;
    }

#endif  // !D_TREE_VISITOR_HAS_IF_CONSTEXPR


// =========================================================================
// IV.ii  IN-ORDER TRAVERSAL (binary only)
// =========================================================================

NS_INTERNAL

    // in_order_visit_binary
    //   function: iterative in-order traversal for binary nodes.
    template<typename _Node,
             typename _Visitor>
    inline void
    in_order_visit_binary(
        _Node*    _root,
        _Visitor& _visitor
    )
    {
        _Node* stack[256];
        std::size_t top = 0;
        _Node* current  = _root;

        while ( (current != nullptr) ||
                (top > 0) )
        {
            // descend left
            while (current != nullptr)
            {
                stack[top++] = current;
                current      = current->left();
            }

            current = stack[--top];
            _visitor.visit(*current);
            current = current->right();
        }

        return;
    }

NS_END  // internal

// tree_visit<in_order>
//   function: visits every node in in-order (Left, Root, Right).
// Static-asserts that the node type is binary.
template<typename _Order,
         typename _Node,
         typename _Visitor>
inline
typename std::enable_if<std::is_same<_Order, in_order>::value>::type
tree_visit(
    _Node*    _root,
    _Visitor& _visitor
)
{
    static_assert(internal::is_binary_node_shape<_Node>::value,
                  "in_order traversal requires a binary node type "
                  "(must expose left() and right()).");

    internal::in_order_visit_binary(_root,
                                    _visitor);

    return;
}


// =========================================================================
// IV.iii  POST-ORDER TRAVERSAL
// =========================================================================

NS_INTERNAL

    // post_order_visit_binary
    //   function: iterative post-order for binary nodes using two stacks.
    template<typename _Node,
             typename _Visitor>
    inline void
    post_order_visit_binary(
        _Node*    _root,
        _Visitor& _visitor
    )
    {
        if (!_root)
        {
            return;
        }

        _Node* stack1[256];
        _Node* stack2[256];
        std::size_t top1 = 0;
        std::size_t top2 = 0;

        stack1[top1++] = _root;

        while (top1 > 0)
        {
            _Node* current  = stack1[--top1];
            stack2[top2++] = current;

            if (current->left())
            {
                stack1[top1++] = current->left();
            }

            if (current->right())
            {
                stack1[top1++] = current->right();
            }
        }

        // visit in reverse of the second stack
        while (top2 > 0)
        {
            _visitor.visit(*(stack2[--top2]));
        }

        return;
    }

    // post_order_visit_nary_sibling
    //   function: iterative post-order for n-ary nodes.
    template<typename _Node,
             typename _Visitor>
    inline void
    post_order_visit_nary_sibling(
        _Node*    _root,
        _Visitor& _visitor
    )
    {
        if (!_root)
        {
            return;
        }

        _Node* stack1[256];
        _Node* stack2[256];
        std::size_t top1 = 0;
        std::size_t top2 = 0;

        stack1[top1++] = _root;

        while (top1 > 0)
        {
            _Node* current = stack1[--top1];
            stack2[top2++] = current;

            // push children left-to-right (they'll be reversed in stack2)
            _Node* child = current->first_child();

            while (child)
            {
                stack1[top1++] = child;
                child          = child->next_sibling();
            }
        }

        while (top2 > 0)
        {
            _visitor.visit(*(stack2[--top2]));
        }

        return;
    }

NS_END  // internal

// tree_visit<post_order>
//   function: visits every node in post-order (children before parent).
template<typename _Order,
         typename _Node,
         typename _Visitor>
inline
typename std::enable_if<std::is_same<_Order, post_order>::value>::type
tree_visit(
    _Node*    _root,
    _Visitor& _visitor
)
{
#if D_TREE_VISITOR_HAS_IF_CONSTEXPR
    if constexpr (internal::is_binary_node_shape<_Node>::value)
    {
        internal::post_order_visit_binary(_root,
                                          _visitor);
    }
    else
    {
        internal::post_order_visit_nary_sibling(_root,
                                                _visitor);
    }
#else
    tree_visit_post_order_dispatch(_root,
                                   _visitor,
                                   std::integral_constant<bool,
                                       internal::is_binary_node_shape<_Node>::value>{});
#endif

    return;
}

#if (!D_TREE_VISITOR_HAS_IF_CONSTEXPR)

    template<typename _Node,
             typename _Visitor>
    inline void
    tree_visit_post_order_dispatch(
        _Node*          _root,
        _Visitor&       _visitor,
        std::true_type  /* is_binary */
    )
    {
        internal::post_order_visit_binary(_root,
                                          _visitor);

        return;
    }

    template<typename _Node,
             typename _Visitor>
    inline void
    tree_visit_post_order_dispatch(
        _Node*          _root,
        _Visitor&       _visitor,
        std::false_type /* is_binary */
    )
    {
        internal::post_order_visit_nary_sibling(_root,
                                                _visitor);

        return;
    }

#endif  // !D_TREE_VISITOR_HAS_IF_CONSTEXPR


// =========================================================================
// IV.iv  LEVEL-ORDER TRAVERSAL
// =========================================================================

NS_INTERNAL

    // level_order_visit_impl
    //   function: BFS traversal using a circular buffer as queue.
    // Adapts child enumeration to binary or n-ary at compile time.
    template<typename _Node,
             typename _Visitor>
    inline void
    level_order_visit_impl(
        _Node*    _root,
        _Visitor& _visitor
    )
    {
        if (!_root)
        {
            return;
        }

        // simple circular queue
        static constexpr std::size_t D_QUEUE_CAP = 512;
        _Node* queue[D_QUEUE_CAP];
        std::size_t head = 0;
        std::size_t tail = 0;
        std::size_t count = 0;

        queue[tail] = _root;
        tail        = (tail + 1) % D_QUEUE_CAP;
        ++count;

        while (count > 0)
        {
            _Node* current = queue[head];
            head           = (head + 1) % D_QUEUE_CAP;
            --count;

            _visitor.visit(*current);

            // enqueue children
#if D_TREE_VISITOR_HAS_IF_CONSTEXPR
            if constexpr (is_binary_node_shape<_Node>::value)
            {
                if (current->left())
                {
                    queue[tail] = current->left();
                    tail        = (tail + 1) % D_QUEUE_CAP;
                    ++count;
                }

                if (current->right())
                {
                    queue[tail] = current->right();
                    tail        = (tail + 1) % D_QUEUE_CAP;
                    ++count;
                }
            }
            else if constexpr (has_first_child_method<_Node>::value)
            {
                _Node* child = current->first_child();

                while (child)
                {
                    queue[tail] = child;
                    tail        = (tail + 1) % D_QUEUE_CAP;
                    ++count;
                    child = child->next_sibling();
                }
            }
#else
            level_order_enqueue_children(current,
                                         queue,
                                         tail,
                                         count,
                                         std::integral_constant<bool,
                                             is_binary_node_shape<_Node>::value>{});
#endif
        }

        return;
    }

#if (!D_TREE_VISITOR_HAS_IF_CONSTEXPR)

    // level_order_enqueue_children — binary
    template<typename _Node,
             std::size_t _Cap>
    inline void
    level_order_enqueue_children(
        _Node*          _current,
        _Node*          (&_queue)[_Cap],
        std::size_t&    _tail,
        std::size_t&    _count,
        std::true_type  /* is_binary */
    )
    {
        if (_current->left())
        {
            _queue[_tail] = _current->left();
            _tail         = (_tail + 1) % _Cap;
            ++_count;
        }

        if (_current->right())
        {
            _queue[_tail] = _current->right();
            _tail         = (_tail + 1) % _Cap;
            ++_count;
        }

        return;
    }

    // level_order_enqueue_children — n-ary
    template<typename _Node,
             std::size_t _Cap>
    inline void
    level_order_enqueue_children(
        _Node*          _current,
        _Node*          (&_queue)[_Cap],
        std::size_t&    _tail,
        std::size_t&    _count,
        std::false_type /* is_binary */
    )
    {
        _Node* child = _current->first_child();

        while (child)
        {
            _queue[_tail] = child;
            _tail         = (_tail + 1) % _Cap;
            ++_count;
            child = child->next_sibling();
        }

        return;
    }

#endif  // !D_TREE_VISITOR_HAS_IF_CONSTEXPR

NS_END  // internal

// tree_visit<level_order>
//   function: visits every node in breadth-first / level order.
template<typename _Order,
         typename _Node,
         typename _Visitor>
inline
typename std::enable_if<std::is_same<_Order, level_order>::value>::type
tree_visit(
    _Node*    _root,
    _Visitor& _visitor
)
{
    internal::level_order_visit_impl(_root,
                                     _visitor);

    return;
}


// =========================================================================
// IV.v  LEAF-ORDER TRAVERSAL
// =========================================================================

NS_INTERNAL

    // leaf_order_visit_impl
    //   function: pre-order scan that only invokes the visitor on leaf
    // nodes, skipping internal nodes entirely.
    template<typename _Node,
             typename _Visitor>
    inline void
    leaf_order_visit_impl(
        _Node*    _root,
        _Visitor& _visitor
    )
    {
        if (!_root)
        {
            return;
        }

        _Node* stack[256];
        std::size_t top = 0;
        stack[top++]    = _root;

        while (top > 0)
        {
            _Node* current = stack[--top];

            if (node_is_leaf(current))
            {
                _visitor.visit(*current);
            }
            else
            {
                // enqueue children for continued descent
#if D_TREE_VISITOR_HAS_IF_CONSTEXPR
                if constexpr (is_binary_node_shape<_Node>::value)
                {
                    if (current->right())
                    {
                        stack[top++] = current->right();
                    }

                    if (current->left())
                    {
                        stack[top++] = current->left();
                    }
                }
                else if constexpr (has_first_child_method<_Node>::value)
                {
                    _Node* children[64];
                    std::size_t cc = 0;
                    _Node* ch = current->first_child();

                    while (ch)
                    {
                        children[cc++] = ch;
                        ch             = ch->next_sibling();
                    }

                    for (std::size_t i = cc; i > 0; --i)
                    {
                        stack[top++] = children[i - 1];
                    }
                }
#else
                leaf_order_push_children(current,
                                         stack,
                                         top,
                                         std::integral_constant<bool,
                                             is_binary_node_shape<_Node>::value>{});
#endif
            }
        }

        return;
    }

#if (!D_TREE_VISITOR_HAS_IF_CONSTEXPR)

    template<typename _Node>
    inline void
    leaf_order_push_children(
        _Node*          _current,
        _Node**         _stack,
        std::size_t&    _top,
        std::true_type  /* is_binary */
    )
    {
        if (_current->right())
        {
            _stack[_top++] = _current->right();
        }

        if (_current->left())
        {
            _stack[_top++] = _current->left();
        }

        return;
    }

    template<typename _Node>
    inline void
    leaf_order_push_children(
        _Node*          _current,
        _Node**         _stack,
        std::size_t&    _top,
        std::false_type /* is_binary */
    )
    {
        _Node* children[64];
        std::size_t cc = 0;
        _Node* ch      = _current->first_child();

        while (ch)
        {
            children[cc++] = ch;
            ch             = ch->next_sibling();
        }

        for (std::size_t i = cc; i > 0; --i)
        {
            _stack[_top++] = children[i - 1];
        }

        return;
    }

#endif  // !D_TREE_VISITOR_HAS_IF_CONSTEXPR

NS_END  // internal

// tree_visit<leaf_order>
//   function: visits only leaf nodes in pre-order scan sequence.
template<typename _Order,
         typename _Node,
         typename _Visitor>
inline
typename std::enable_if<std::is_same<_Order, leaf_order>::value>::type
tree_visit(
    _Node*    _root,
    _Visitor& _visitor
)
{
    internal::leaf_order_visit_impl(_root,
                                    _visitor);

    return;
}


// =========================================================================
// IV.vi  CONTAINER OVERLOADS
// =========================================================================
// Convenience overloads that accept a tree_container and extract root().

// tree_visit (container overload)
//   function: traverses a tree_container by extracting its root node.
template<typename _Order,
         typename _ValueType,
         typename _NodeType,
         typename _Allocator,
         typename _LockPolicy,
         typename _Visitor>
inline void
tree_visit(
    tree_container<_ValueType, _NodeType, _Allocator, _LockPolicy>& _tree,
    _Visitor& _visitor
)
{
    tree_visit<_Order>(_tree.root(),
                       _visitor);

    return;
}

// tree_visit (const container overload)
//   function: const version for read-only visitation.
template<typename _Order,
         typename _ValueType,
         typename _NodeType,
         typename _Allocator,
         typename _LockPolicy,
         typename _Visitor>
inline void
tree_visit(
    const tree_container<_ValueType, _NodeType, _Allocator, _LockPolicy>& _tree,
    _Visitor& _visitor
)
{
    tree_visit<_Order>(const_cast<_NodeType*>(_tree.root()),
                       _visitor);

    return;
}


///////////////////////////////////////////////////////////////////////////////
///           V.    DEPTH-AWARE VISITATION (C++11+)                         ///
///////////////////////////////////////////////////////////////////////////////

// tree_visit_with_depth
//   function: pre-order traversal that invokes _fn(node, depth) for each
// node, passing the current depth as a size_t. Adapts to binary/n-ary.
template<typename _Node,
         typename _Fn>
inline void
tree_visit_with_depth(
    _Node* _root,
    _Fn&   _fn
)
{
    if (!_root)
    {
        return;
    }

    // stack entries: (node, depth)
    struct stack_entry
    {
        _Node*      node;
        std::size_t depth;
    };

    stack_entry stack[256];
    std::size_t top = 0;
    stack[top++]    = { _root, 0 };

    while (top > 0)
    {
        stack_entry entry = stack[--top];
        _fn(*(entry.node), entry.depth);

        std::size_t child_depth = entry.depth + 1;

#if D_TREE_VISITOR_HAS_IF_CONSTEXPR
        if constexpr (internal::is_binary_node_shape<_Node>::value)
        {
            if (entry.node->right())
            {
                stack[top++] = { entry.node->right(), child_depth };
            }

            if (entry.node->left())
            {
                stack[top++] = { entry.node->left(), child_depth };
            }
        }
        else if constexpr (internal::has_first_child_method<_Node>::value)
        {
            _Node* children[64];
            std::size_t cc = 0;
            _Node* ch      = entry.node->first_child();

            while (ch)
            {
                children[cc++] = ch;
                ch             = ch->next_sibling();
            }

            for (std::size_t i = cc; i > 0; --i)
            {
                stack[top++] = { children[i - 1], child_depth };
            }
        }
#else
        depth_push_children(entry.node,
                            child_depth,
                            stack,
                            top,
                            std::integral_constant<bool,
                                internal::is_binary_node_shape<_Node>::value>{});
#endif
    }

    return;
}

#if (!D_TREE_VISITOR_HAS_IF_CONSTEXPR)

    template<typename _Node,
             typename _Entry>
    inline void
    depth_push_children(
        _Node*          _current,
        std::size_t     _child_depth,
        _Entry*         _stack,
        std::size_t&    _top,
        std::true_type  /* is_binary */
    )
    {
        if (_current->right())
        {
            _stack[_top++] = { _current->right(), _child_depth };
        }

        if (_current->left())
        {
            _stack[_top++] = { _current->left(), _child_depth };
        }

        return;
    }

    template<typename _Node,
             typename _Entry>
    inline void
    depth_push_children(
        _Node*          _current,
        std::size_t     _child_depth,
        _Entry*         _stack,
        std::size_t&    _top,
        std::false_type /* is_binary */
    )
    {
        _Node* children[64];
        std::size_t cc = 0;
        _Node* ch      = _current->first_child();

        while (ch)
        {
            children[cc++] = ch;
            ch             = ch->next_sibling();
        }

        for (std::size_t i = cc; i > 0; --i)
        {
            _stack[_top++] = { children[i - 1], _child_depth };
        }

        return;
    }

#endif  // !D_TREE_VISITOR_HAS_IF_CONSTEXPR

// tree_visit_with_depth (container overload)
//   function: depth-aware visitation starting from a tree_container's root.
template<typename _ValueType,
         typename _NodeType,
         typename _Allocator,
         typename _LockPolicy,
         typename _Fn>
inline void
tree_visit_with_depth(
    tree_container<_ValueType, _NodeType, _Allocator, _LockPolicy>& _tree,
    _Fn& _fn
)
{
    tree_visit_with_depth(_tree.root(),
                          _fn);

    return;
}


///////////////////////////////////////////////////////////////////////////////
///            VI.   ACCUMULATING VISITOR (C++11+)                          ///
///////////////////////////////////////////////////////////////////////////////

// tree_accumulate
//   function: folds a tree into a single value. Visits nodes in the
// specified traversal order, calling _fn(accumulator, node) at each
// step and returning the final accumulated value.
//
// Usage:
//   int sum = tree_accumulate<pre_order>(
//       root, 0,
//       [](int acc, const my_node& n) { return acc + n.data(); });
template<typename _Order,
         typename _Node,
         typename _Accumulator,
         typename _Fn>
inline _Accumulator
tree_accumulate(
    _Node*             _root,
    _Accumulator       _init,
    _Fn&               _fn
)
{
    // wrap the callable into a visitor-compatible object
    struct accumulating_visitor
    {
        _Accumulator  acc;
        _Fn&          fn;

        void visit(_Node& _node)
        {
            acc = fn(acc, _node);

            return;
        }
    };

    accumulating_visitor v{ _init, _fn };
    tree_visit<_Order>(_root,
                       v);

    return v.acc;
}

// tree_accumulate (const node variant)
//   function: const-correct accumulate for read-only node access.
template<typename _Order,
         typename _Node,
         typename _Accumulator,
         typename _Fn>
inline _Accumulator
tree_accumulate(
    const _Node*       _root,
    _Accumulator       _init,
    _Fn&               _fn
)
{
    return tree_accumulate<_Order>(const_cast<_Node*>(_root),
                                   _init,
                                   _fn);
}

// tree_accumulate (container overload)
//   function: accumulate starting from a tree_container's root.
template<typename _Order,
         typename _ValueType,
         typename _NodeType,
         typename _Allocator,
         typename _LockPolicy,
         typename _Accumulator,
         typename _Fn>
inline _Accumulator
tree_accumulate(
    tree_container<_ValueType, _NodeType, _Allocator, _LockPolicy>& _tree,
    _Accumulator _init,
    _Fn&         _fn
)
{
    return tree_accumulate<_Order>(_tree.root(),
                                   _init,
                                   _fn);
}


///////////////////////////////////////////////////////////////////////////////
///           VII.  FILTERED VISITATION (C++11+)                            ///
///////////////////////////////////////////////////////////////////////////////

// tree_visit_if
//   function: visits only nodes for which _pred(node) returns true.
// Traverses in the given order, testing each node against the predicate
// before forwarding to the visitor.
template<typename _Order,
         typename _Node,
         typename _Visitor,
         typename _Predicate>
inline void
tree_visit_if(
    _Node*      _root,
    _Visitor&   _visitor,
    _Predicate& _pred
)
{
    struct filtering_visitor
    {
        _Visitor&   inner;
        _Predicate& pred;

        void visit(_Node& _node)
        {
            if (pred(_node))
            {
                inner.visit(_node);
            }

            return;
        }
    };

    filtering_visitor fv{ _visitor, _pred };
    tree_visit<_Order>(_root,
                       fv);

    return;
}

// tree_visit_if (container overload)
template<typename _Order,
         typename _ValueType,
         typename _NodeType,
         typename _Allocator,
         typename _LockPolicy,
         typename _Visitor,
         typename _Predicate>
inline void
tree_visit_if(
    tree_container<_ValueType, _NodeType, _Allocator, _LockPolicy>& _tree,
    _Visitor&   _visitor,
    _Predicate& _pred
)
{
    tree_visit_if<_Order>(_tree.root(),
                          _visitor,
                          _pred);

    return;
}


///////////////////////////////////////////////////////////////////////////////
///            VIII. VISITOR ADAPTERS (C++14+)                              ///
///////////////////////////////////////////////////////////////////////////////

#if D_ENV_LANG_IS_CPP14_OR_HIGHER

// tree_for_each
//   function: convenience adapter that wraps a callable (lambda, function
// pointer, functor) into a visitor and applies it in the given traversal
// order. The callable receives a reference to each node.
//
// Usage:
//   tree_for_each<pre_order>(root, [](auto& node) {
//       std::cout << node.data() << "\n";
//   });
template<typename _Order,
         typename _Node,
         typename _Fn>
inline void
tree_for_each(
    _Node* _root,
    _Fn&&  _fn
)
{
    struct callable_visitor
    {
        _Fn fn;

        void visit(_Node& _node)
        {
            fn(_node);

            return;
        }
    };

    callable_visitor v{ std::forward<_Fn>(_fn) };
    tree_visit<_Order>(_root,
                       v);

    return;
}

// tree_for_each (container overload)
template<typename _Order,
         typename _ValueType,
         typename _NodeType,
         typename _Allocator,
         typename _LockPolicy,
         typename _Fn>
inline void
tree_for_each(
    tree_container<_ValueType, _NodeType, _Allocator, _LockPolicy>& _tree,
    _Fn&& _fn
)
{
    tree_for_each<_Order>(_tree.root(),
                          std::forward<_Fn>(_fn));

    return;
}

// tree_count_if
//   function: counts nodes satisfying a predicate in the given order.
template<typename _Order,
         typename _Node,
         typename _Predicate>
inline std::size_t
tree_count_if(
    _Node*     _root,
    _Predicate _pred
)
{
    struct counting_visitor
    {
        _Predicate  pred;
        std::size_t count;

        void visit(_Node& _node)
        {
            if (pred(_node))
            {
                ++count;
            }

            return;
        }
    };

    counting_visitor v{ _pred, 0 };
    tree_visit<_Order>(_root,
                       v);

    return v.count;
}

// tree_find_if
//   function: finds the first node (in traversal order) satisfying a
// predicate. Returns nullptr if no match.
template<typename _Order,
         typename _Node,
         typename _Predicate>
inline _Node*
tree_find_if(
    _Node*     _root,
    _Predicate _pred
)
{
    struct search_visitor
    {
        _Predicate pred;
        _Node*     result;

        void visit(_Node& _node)
        {
            // first match wins (subsequent calls are no-ops)
            if ( (!result) &&
                 pred(_node) )
            {
                result = &_node;
            }

            return;
        }
    };

    search_visitor v{ _pred, nullptr };
    tree_visit<_Order>(_root,
                       v);

    return v.result;
}

#endif  // D_ENV_LANG_IS_CPP14_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///             IX.   CURSOR-BASED VISITATION (C++11+)                      ///
///////////////////////////////////////////////////////////////////////////////

// tree_visit_cursor
//   function: walks a tree using a tree_cursor, calling _fn(cursor) at
// each position. Provides imperative control — the callback receives
// the full cursor (not just the node) and can inspect topology.
template<typename _Node,
         typename _Fn>
inline void
tree_visit_cursor(
    _Node* _root,
    _Fn&   _fn
)
{
    if (!_root)
    {
        return;
    }

    tree_cursor<_Node> cursor(_root);

    struct cursor_stack_entry
    {
        _Node*      node;
        bool        visited;
    };

    cursor_stack_entry stack[256];
    std::size_t top = 0;
    stack[top++]    = { _root, false };

    while (top > 0)
    {
        cursor_stack_entry& entry = stack[top - 1];

        if (!entry.visited)
        {
            entry.visited = true;
            tree_cursor<_Node> pos(entry.node);
            _fn(pos);

            // push children
#if D_TREE_VISITOR_HAS_IF_CONSTEXPR
            if constexpr (internal::is_binary_node_shape<_Node>::value)
            {
                if (entry.node->right())
                {
                    stack[top++] = { entry.node->right(), false };
                }

                if (entry.node->left())
                {
                    stack[top++] = { entry.node->left(), false };
                }
            }
            else if constexpr (internal::has_first_child_method<_Node>::value)
            {
                _Node* children[64];
                std::size_t cc = 0;
                _Node* ch      = entry.node->first_child();

                while (ch)
                {
                    children[cc++] = ch;
                    ch             = ch->next_sibling();
                }

                for (std::size_t i = cc; i > 0; --i)
                {
                    stack[top++] = { children[i - 1], false };
                }
            }
#endif
            // pop the current (we already processed it)
            // children are now on top and will be visited next
        }
        else
        {
            --top;
        }
    }

    return;
}

#endif  // D_ENV_LANG_IS_CPP11_OR_HIGHER


///////////////////////////////////////////////////////////////////////////////
///          X.    CONCEPT-CONSTRAINED VISITATION (C++20+)                  ///
///////////////////////////////////////////////////////////////////////////////

#if D_TREE_VISITOR_HAS_CONCEPTS

// tree_node_visitor_for
//   concept: constrains a visitor type that can visit a specific node type.
template<typename _Visitor,
         typename _Node>
concept tree_node_visitor_for = requires(_Visitor& _v, _Node& _n)
{
    _v.visit(_n);
};

// tree_visitable
//   concept: constrains types that expose root() returning a pointer to
// a node type, making them suitable targets for tree_visit.
template<typename _T>
concept tree_visitable = requires(const _T& _t)
{
    { _t.root() } -> std::convertible_to<const typename _T::node_type*>;
};

// depth_callback
//   concept: constrains callables that accept (node, depth).
template<typename _Fn,
         typename _Node>
concept depth_callback = requires(_Fn& _f, _Node& _n, std::size_t _d)
{
    _f(_n, _d);
};

// constrained_tree_visit
//   function: concept-constrained tree visitation. Provides clear
// compile errors when the visitor or container is incompatible.
template<typename _Order,
         tree_visitable _Tree,
         tree_node_visitor_for<typename _Tree::node_type> _Visitor>
inline void
constrained_tree_visit(
    _Tree&    _tree,
    _Visitor& _visitor
)
{
    tree_visit<_Order>(_tree.root(),
                       _visitor);

    return;
}

// constrained_tree_visit_with_depth
//   function: concept-constrained depth-aware visitation.
template<tree_visitable                           _Tree,
         depth_callback<typename _Tree::node_type> _Fn>
inline void
constrained_tree_visit_with_depth(
    _Tree& _tree,
    _Fn&   _fn
)
{
    tree_visit_with_depth(_tree.root(),
                          _fn);

    return;
}

#endif  // D_TREE_VISITOR_HAS_CONCEPTS


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_TREE_VISITOR_