/******************************************************************************
* djinterp [test]                                              test_tree.hpp
*
* N-ary tree data structure for the C++ test framework.
*   Provides a rank-ordered tree where each node carries a name, a
* rank, and an outcome status. The tree enforces the invariant that
* every child's rank is less than or equal to its parent's rank,
* allowing users to define their own node hierarchy via rank values.
*
*   Structure is fully intrusive: each node holds parent,
* first_child, last_child, and next_sibling pointers. Appending a
* child is O(1). Sibling iteration is a forward walk via
* next_sibling. There is no container abstraction or allocation per
* link — just raw pointers.
*
*   test_node is a plain struct. Users inherit from it to add
* callable, duration, tags, or any domain-specific fields. The tree
* is templated on the node type and constructs instances via default
* constructor + field assignment.
*
*   The pool container is templated so that any djinterp container
* (or standard container) satisfying the pool protocol can be used
* in place of std::vector.  The pool protocol requires:
*   - push_back(element)
*   - back() -> reference
*   - size() -> integral
*   - clear()
*
* COMPONENTS:
*   djinterp::test::test_node    - base tree node
*   djinterp::test::test_tree    - owning tree container (template)
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/cpp/test/test_tree.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.14
******************************************************************************/

#ifndef DJINTERP_TEST_TREE_
#define DJINTERP_TEST_TREE_ 1

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "test_common.hpp"


NS_DJINTERP
NS_TEST


// =========================================================================
// 0.   POOL PROTOCOL DETECTION
// =========================================================================
//   Minimal SFINAE checks for the pool container protocol.
// These are intentionally self-contained — they do not depend
// on the container_traits.hpp header so that the test framework
// can remain standalone while still accepting djinterp
// containers.

NS_INTERNAL

    // pool_has_push_back
    //   trait: true if _Pool supports push_back(element).
    template<typename _Pool,
             typename = void>
    struct pool_has_push_back : std::false_type
    {};

    template<typename _Pool>
    struct pool_has_push_back<_Pool,
        djinterp::void_t<decltype(
            std::declval<_Pool&>().push_back(
                std::declval<typename _Pool::value_type>()))>>
        : std::true_type
    {};

    // pool_has_back
    //   trait: true if _Pool supports back().
    template<typename _Pool,
             typename = void>
    struct pool_has_back : std::false_type
    {};

    template<typename _Pool>
    struct pool_has_back<_Pool,
        djinterp::void_t<decltype(
            std::declval<_Pool&>().back())>>
        : std::true_type
    {};

    // pool_has_size
    //   trait: true if _Pool supports size().
    template<typename _Pool,
             typename = void>
    struct pool_has_size : std::false_type
    {};

    template<typename _Pool>
    struct pool_has_size<_Pool,
        djinterp::void_t<decltype(
            std::declval<const _Pool&>().size())>>
        : std::true_type
    {};

    // pool_has_clear
    //   trait: true if _Pool supports clear().
    template<typename _Pool,
             typename = void>
    struct pool_has_clear : std::false_type
    {};

    template<typename _Pool>
    struct pool_has_clear<_Pool,
        djinterp::void_t<decltype(
            std::declval<_Pool&>().clear())>>
        : std::true_type
    {};

    // output_has_push_back
    //   trait: true if _Output supports push_back(element).
    template<typename _Output,
             typename = void>
    struct output_has_push_back : std::false_type
    {};

    template<typename _Output>
    struct output_has_push_back<_Output,
        djinterp::void_t<decltype(
            std::declval<_Output&>().push_back(
                std::declval<typename _Output::value_type>()))>>
        : std::true_type
    {};

    // output_has_reserve
    //   trait: true if _Output supports reserve(n).
    template<typename _Output,
             typename = void>
    struct output_has_reserve : std::false_type
    {};

    template<typename _Output>
    struct output_has_reserve<_Output,
        djinterp::void_t<decltype(
            std::declval<_Output&>().reserve(
                std::declval<std::size_t>()))>>
        : std::true_type
    {};

    // conditional_reserve
    //   helper: calls reserve if supported, no-op otherwise.
    template<typename _Output>
    inline void
    conditional_reserve
    (
        _Output&    _out,
        std::size_t _n,
        std::true_type
    )
    {
        _out.reserve(_n);

        return;
    }

    template<typename _Output>
    inline void
    conditional_reserve
    (
        _Output&,
        std::size_t,
        std::false_type
    )
    {
        return;
    }

NS_END  // internal


// =========================================================================
// I.   TEST NODE
// =========================================================================

// test_node
//   struct: base tree node with intrusive sibling/child links.
//
// Structure:
//   parent       → non-owning back-pointer (nullptr for root)
//   first_child  → head of child linked list
//   last_child   → tail of child linked list (O(1) append)
//   next_sibling → next sibling in parent's child list
//
// This struct is intentionally minimal. Users inherit from it
// to add callable, duration, tags, metadata, or any domain-
// specific fields.
//
// Usage:
//   struct my_node : test_node
//   {
//       fn_test callable;
//       double  duration_ms = 0.0;
//   };
//
//   test_tree<my_node> tree;
struct test_node
{
    // ---- identity ----

    // unique identifier assigned by the tree's id generator
    test_id       id;

    // human-readable name
    std::string   name;

    // rank for tree ordering invariant (child <= parent)
    test_rank     rank;

    // ---- outcome ----

    // outcome status (initially D_TEST_STATUS_UNKNOWN)
    test_status   status;

    // human-readable message
    std::string   message;

    // ---- structure ----

    // true if this node is a leaf (no children)
    bool          is_leaf;

    // number of direct children (cached)
    std::size_t   child_count;

    // non-owning back-pointer to parent (nullptr for root)
    test_node* parent;

    // head of child linked list (nullptr if leaf)
    test_node* first_child;

    // tail of child linked list (nullptr if leaf); enables
    // O(1) append
    test_node* last_child;

    // next sibling in parent's child list (nullptr if last)
    test_node* next_sibling;


    // ---- constructors ----

    test_node()
        : id(D_TEST_ID_INVALID)
        , name()
        , rank(0)
        , status(D_TEST_STATUS_UNKNOWN)
        , message()
        , is_leaf(true)
        , child_count(0)
        , parent(nullptr)
        , first_child(nullptr)
        , last_child(nullptr)
        , next_sibling(nullptr)
    {
    };

    // ---- queries ----

    // depth
    //   returns the depth of this node (distance from root).
    std::size_t depth() const
    {
        std::size_t d = 0;
        const test_node* p = parent;

        while (p)
        {
            ++d;
            p = p->parent;
        }

        return d;
    };

    // subtree_size
    //   returns the total number of nodes in the subtree
    // rooted at this node (including this node).
    std::size_t subtree_size() const
    {
        std::size_t count = 1;
        const test_node* child = first_child;

        while (child)
        {
            count += child->subtree_size();
            child = child->next_sibling;
        }

        return count;
    };

    // subtree_depth
    //   returns the maximum depth of the subtree rooted at
    // this node (0 for a leaf).
    std::size_t subtree_depth() const
    {
        std::size_t max_child = 0;
        const test_node* child = first_child;

        while (child)
        {
            std::size_t cd = child->subtree_depth() + 1;

            if (cd > max_child)
            {
                max_child = cd;
            }

            child = child->next_sibling;
        }

        return max_child;
    };

    // all_passed
    //   returns true if this node and all descendants have
    // status D_TEST_STATUS_PASSED.
    bool all_passed() const
    {
        if (!is_passing(status))
        {
            return false;
        }

        const test_node* child = first_child;

        while (child)
        {
            if (!child->all_passed())
            {
                return false;
            }

            child = child->next_sibling;
        }

        return true;
    };

    // ---- search ----

    // find_by_id
    //   depth-first search for a node with the given id.
    test_node* find_by_id(test_id _id)
    {
        if (id == _id)
        {
            return this;
        }

        test_node* child = first_child;

        while (child)
        {
            test_node* found = child->find_by_id(_id);

            if (found)
            {
                return found;
            }

            child = child->next_sibling;
        }

        return nullptr;
    };

    // find_by_id (const)
    const test_node* find_by_id(test_id _id) const
    {
        if (id == _id)
        {
            return this;
        }

        const test_node* child = first_child;

        while (child)
        {
            const test_node* found = child->find_by_id(_id);

            if (found)
            {
                return found;
            }

            child = child->next_sibling;
        }

        return nullptr;
    };

    // find_by_name
    //   depth-first search for the first node with the
    // given name.
    test_node* find_by_name(const std::string& _name)
    {
        if (name == _name)
        {
            return this;
        }

        test_node* child = first_child;

        while (child)
        {
            test_node* found = child->find_by_name(_name);

            if (found)
            {
                return found;
            }

            child = child->next_sibling;
        }

        return nullptr;
    };

    // find_by_name (const)
    const test_node* find_by_name(
        const std::string& _name) const
    {
        if (name == _name)
        {
            return this;
        }

        const test_node* child = first_child;

        while (child)
        {
            const test_node* found =
                child->find_by_name(_name);

            if (found)
            {
                return found;
            }

            child = child->next_sibling;
        }

        return nullptr;
    };

    // find_if
    //   depth-first search for the first node matching the
    // predicate.
    template<typename _Predicate>
    test_node* find_if(_Predicate&& _pred)
    {
        if (_pred(*this))
        {
            return this;
        }

        test_node* child = first_child;

        while (child)
        {
            test_node* found = child->find_if(_pred);

            if (found)
            {
                return found;
            }

            child = child->next_sibling;
        }

        return nullptr;
    };

    // find_if (const)
    template<typename _Predicate>
    const test_node* find_if(_Predicate&& _pred) const
    {
        if (_pred(*this))
        {
            return this;
        }

        const test_node* child = first_child;

        while (child)
        {
            const test_node* found = child->find_if(_pred);

            if (found)
            {
                return found;
            }

            child = child->next_sibling;
        }

        return nullptr;
    };

    // ---- iteration ----

    // for_each
    //   depth-first pre-order traversal. The callable
    // receives (const test_node&, std::size_t depth).
    template<typename _Callable>
    void for_each(_Callable&& _fn,
        std::size_t _depth = 0) const
    {
        _fn(*this, _depth);

        const test_node* child = first_child;

        while (child)
        {
            child->for_each(_fn, _depth + 1);
            child = child->next_sibling;
        }
    };

    // for_each_mut
    //   mutable depth-first pre-order. The callable
    // receives (test_node&, std::size_t depth).
    template<typename _Callable>
    void for_each_mut(_Callable&& _fn,
        std::size_t _depth = 0)
    {
        _fn(*this, _depth);

        test_node* child = first_child;

        while (child)
        {
            child->for_each_mut(_fn, _depth + 1);
            child = child->next_sibling;
        }
    };

    // for_each_child
    //   invokes _fn for each direct child (not recursive).
    // The callable receives (const test_node&).
    template<typename _Callable>
    void for_each_child(_Callable&& _fn) const
    {
        const test_node* child = first_child;

        while (child)
        {
            _fn(*child);
            child = child->next_sibling;
        }
    };

    // for_each_child_mut
    //   mutable version; the callable receives (test_node&).
    template<typename _Callable>
    void for_each_child_mut(_Callable&& _fn)
    {
        test_node* child = first_child;

        while (child)
        {
            _fn(*child);
            child = child->next_sibling;
        }
    };

    // for_each_leaf
    //   invokes _fn for every leaf in the subtree.
    // The callable receives (const test_node&, std::size_t).
    template<typename _Callable>
    void for_each_leaf(_Callable&& _fn,
        std::size_t _depth = 0) const
    {
        if (is_leaf)
        {
            _fn(*this, _depth);

            return;
        }

        const test_node* child = first_child;

        while (child)
        {
            child->for_each_leaf(_fn, _depth + 1);
            child = child->next_sibling;
        }
    };

    // collect
    //   appends pointers to all nodes matching the predicate
    // into _out. Depth-first pre-order.
    //   _Output must support push_back(const test_node*).
    template<typename _Predicate,
             typename _Output>
    void collect(_Predicate&& _pred,
        _Output&    _out,
        std::size_t _depth = 0) const
    {
        static_assert(
            internal::output_has_push_back<_Output>::value,
            "collect output container must support "
            "push_back(value_type).");

        if (_pred(*this, _depth))
        {
            _out.push_back(this);
        }

        const test_node* child = first_child;

        while (child)
        {
            child->collect(_pred, _out, _depth + 1);
            child = child->next_sibling;
        }
    };

    // collect (std::vector overload for backward compat)
    //   preserves the original signature accepting
    // std::vector<const test_node*>&.
    template<typename _Predicate>
    void collect(_Predicate&& _pred,
        std::vector<const test_node*>& _out,
        std::size_t                     _depth = 0) const
    {
        if (_pred(*this, _depth))
        {
            _out.push_back(this);
        }

        const test_node* child = first_child;

        while (child)
        {
            child->collect(_pred, _out, _depth + 1);
            child = child->next_sibling;
        }
    };
};


// =========================================================================
// II.  TEST TREE
// =========================================================================

// test_tree
//   class: owning container for a rank-ordered n-ary tree.
// Templated on the node type and the pool container type.
// Owns all nodes in a flat pool with the tree structure
// maintained via parent/child/sibling pointers.
//
// Template parameters:
//   _Node: the node type (default: test_node). Must have all
//     the fields of test_node (either by being test_node or
//     inheriting from it) and be default-constructible.
//   _Pool: the container type for the node pool (default:
//     std::vector<std::unique_ptr<_Node>>). Must satisfy the
//     pool protocol: push_back, back, size, clear. Any
//     djinterp container with these capabilities may be used.
template<typename _Node = test_node,
         typename _Pool = std::vector<std::unique_ptr<_Node>>>
class test_tree
{
    static_assert(
        internal::pool_has_push_back<_Pool>::value,
        "_Pool container must support "
        "push_back(value_type).");

    static_assert(
        internal::pool_has_back<_Pool>::value,
        "_Pool container must support back().");

    static_assert(
        internal::pool_has_size<_Pool>::value,
        "_Pool container must support size().");

    static_assert(
        internal::pool_has_clear<_Pool>::value,
        "_Pool container must support clear().");

public:
    using node_type = _Node;
    using pool_type = _Pool;

    test_tree()
        : m_root(nullptr)
        , m_pool()
        , m_id_gen()
    {
    };

    // ---- root management ----

    // set_root
    //   creates the root node with the given name and rank.
    // Any existing tree is replaced.
    // returns: pointer to the new root.
    _Node* set_root(const std::string& _name,
        test_rank          _rank)
    {
        m_pool.clear();
        m_id_gen.reset();

        _Node* node = create_node();
        node->id = m_id_gen.next();
        node->name = _name;
        node->rank = _rank;
        node->is_leaf = false;

        m_root = node;

        return node;
    };

    // root
    //   returns a pointer to the root node, or nullptr.
    _Node* root()
    {
        return m_root;
    };

    // root (const)
    const _Node* root() const
    {
        return m_root;
    };

    // empty
    //   returns true if the tree has no root.
    bool empty() const
    {
        return (!m_root);
    };

    // ---- node creation ----

    // add_child
    //   creates a new interior (grouping) node and appends it
    // as a child of _parent. Enforces the rank invariant.
    // returns: pointer to the new child, or nullptr on failure.
    _Node* add_child(_Node* _parent,
        const std::string& _name,
        test_rank          _rank)
    {
        if ((!_parent) ||
            (_rank > _parent->rank))
        {
            return nullptr;
        }

        _Node* node = create_node();
        node->id = m_id_gen.next();
        node->name = _name;
        node->rank = _rank;
        node->is_leaf = false;
        node->parent = _parent;

        append_child(_parent, node);

        return node;
    };

    // add_leaf
    //   creates a new leaf node and appends it as a child of
    // _parent. Enforces the rank invariant.
    // returns: pointer to the new leaf, or nullptr on failure.
    _Node* add_leaf(_Node* _parent,
        const std::string& _name,
        test_rank          _rank)
    {
        if ((!_parent) ||
            (_rank > _parent->rank))
        {
            return nullptr;
        }

        _Node* node = create_node();
        node->id = m_id_gen.next();
        node->name = _name;
        node->rank = _rank;
        node->is_leaf = true;
        node->parent = _parent;

        append_child(_parent, node);

        return node;
    };

    // ---- queries ----

    // size
    //   returns the total number of nodes in the pool.
    std::size_t size() const
    {
        return m_pool.size();
    };

    // depth
    //   returns the maximum depth of the tree.
    std::size_t depth() const
    {
        if (!m_root)
        {
            return 0;
        }

        return m_root->subtree_depth();
    };

    // ---- search (delegates to root) ----

    _Node* find_by_id(test_id _id)
    {
        if (!m_root)
        {
            return nullptr;
        }

        return static_cast<_Node*>(
            m_root->find_by_id(_id));
    };

    const _Node* find_by_id(test_id _id) const
    {
        if (!m_root)
        {
            return nullptr;
        }

        return static_cast<const _Node*>(
            m_root->find_by_id(_id));
    };

    _Node* find_by_name(const std::string& _name)
    {
        if (!m_root)
        {
            return nullptr;
        }

        return static_cast<_Node*>(
            m_root->find_by_name(_name));
    };

    const _Node* find_by_name(
        const std::string& _name) const
    {
        if (!m_root)
        {
            return nullptr;
        }

        return static_cast<const _Node*>(
            m_root->find_by_name(_name));
    };

    template<typename _Predicate>
    _Node* find_if(_Predicate&& _pred)
    {
        if (!m_root)
        {
            return nullptr;
        }

        return static_cast<_Node*>(
            m_root->find_if(
                std::forward<_Predicate>(_pred)));
    };

    template<typename _Predicate>
    const _Node* find_if(_Predicate&& _pred) const
    {
        if (!m_root)
        {
            return nullptr;
        }

        return static_cast<const _Node*>(
            m_root->find_if(
                std::forward<_Predicate>(_pred)));
    };

    // ---- iteration (delegates to root) ----

    template<typename _Callable>
    void for_each(_Callable&& _fn) const
    {
        if (m_root)
        {
            m_root->for_each(
                std::forward<_Callable>(_fn));
        }
    };

    template<typename _Callable>
    void for_each_mut(_Callable&& _fn)
    {
        if (m_root)
        {
            m_root->for_each_mut(
                std::forward<_Callable>(_fn));
        }
    };

    template<typename _Callable>
    void for_each_leaf(_Callable&& _fn) const
    {
        if (m_root)
        {
            m_root->for_each_leaf(
                std::forward<_Callable>(_fn));
        }
    };

    // collect (default output: std::vector)
    //   collects pointers to nodes matching the predicate
    // into a std::vector.
    template<typename _Predicate>
    std::vector<const _Node*> collect(
        _Predicate&& _pred) const
    {
        std::vector<const test_node*> base_out;

        if (m_root)
        {
            m_root->collect(
                std::forward<_Predicate>(_pred),
                base_out);
        }

        // cast to derived pointers
        std::vector<const _Node*> out;
        out.reserve(base_out.size());

        for (const test_node* p : base_out)
        {
            out.push_back(
                static_cast<const _Node*>(p));
        }

        return out;
    };

    // collect_into
    //   collects pointers to nodes matching the predicate
    // into a user-supplied output container. _Output must
    // support push_back(const _Node*). Any djinterp
    // container with push_back capability may be used.
    template<typename _Predicate,
             typename _Output>
    void collect_into(
        _Predicate&& _pred,
        _Output&     _out) const
    {
        static_assert(
            internal::output_has_push_back<_Output>::value,
            "collect_into output container must support "
            "push_back(value_type).");

        if (!m_root)
        {
            return;
        }

        // collect base pointers first
        std::vector<const test_node*> base_out;
        m_root->collect(
            std::forward<_Predicate>(_pred),
            base_out);

        // conditionally reserve if output supports it
        internal::conditional_reserve(
            _out,
            base_out.size(),
            typename internal::output_has_reserve<
                _Output>());

        // cast and push to output container
        for (const test_node* p : base_out)
        {
            _out.push_back(
                static_cast<const _Node*>(p));
        }

        return;
    };

    // ---- id generator access ----

    test_id_generator& id_generator()
    {
        return m_id_gen;
    };

    const test_id_generator& id_generator() const
    {
        return m_id_gen;
    };

    // ---- pool access ----

    // pool
    //   returns a const reference to the underlying pool
    // container for inspection or integration with
    // container-aware algorithms.
    const _Pool& pool() const
    {
        return m_pool;
    };

    // ---- clear ----

    // clear
    //   destroys all nodes and resets the id generator.
    void clear()
    {
        m_root = nullptr;
        m_pool.clear();
        m_id_gen.reset();
    };

private:
    // append_child
    //   links _child into _parent's child list and updates
    // _parent's leaf status and child count.
    void append_child(_Node* _parent,
        _Node* _child)
    {
        _child->next_sibling = nullptr;

        if (!_parent->first_child)
        {
            _parent->first_child = _child;
            _parent->last_child = _child;
        }
        else
        {
            _parent->last_child->next_sibling = _child;
            _parent->last_child = _child;
        }

        ++_parent->child_count;
        _parent->is_leaf = false;
    };

    // create_node
    //   allocates a new node in the pool and returns a raw
    // pointer.
    _Node* create_node()
    {
        m_pool.push_back(
            std::unique_ptr<_Node>(new _Node()));

        return m_pool.back().get();
    };

    _Node*            m_root;
    _Pool             m_pool;
    test_id_generator m_id_gen;
};


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_TREE_
