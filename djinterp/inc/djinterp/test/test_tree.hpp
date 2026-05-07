/******************************************************************************
* djinterp [test]                                              test_tree.hpp
*
*   Test tree overlay container.  Wraps any container satisfying the
* n-ary tree protocol (as detected by nary_tree_traits.hpp) whose
* elements conform to the test object protocol (as detected by
* test_object_traits.hpp).
*
*   RANK INVARIANT:
*   By default, test_tree enforces the rank invariant: a child's
* rank must be less than or equal to its parent's rank.  This
* prevents structural violations (e.g. an assertion containing a
* module).  The _ValidateRank template parameter controls this
* at compile time - when false, rank checks are compiled out
* entirely with zero overhead.
*
*   OVERLAY DESIGN:
*   test_tree does not own storage.  It delegates entirely to its
* underlying container, adding a test-domain query surface on top:
* pass/fail counting, status aggregation, subtree filtering, and
* evaluation dispatch.  This places it on classification axis 9
* as an overlay container with underlying_container_type exposed.
*
*   UNDERLYING REQUIREMENTS:
*   The underlying container must be structurally recognized as an
* n-ary tree by the trait system.  Any child-access model is
* accepted: LCRS, container-children, edge-based, or hybrid.
* The element type stored in the underlying container must satisfy
* the test object protocol (boolean conversion + status accessor).
*
*   STRUCTURAL DETECTION:
*   test_tree itself exposes the container protocol members needed
* for the trait system to classify it: value_type, node_type,
* depth_type, underlying_container_type, begin/end (forwarded),
* size, root, children, parent.  The trait system will therefore
* classify it as hierarchical, overlay, and iterable at whatever
* level the underlying container supports.
*
*   PORTABILITY:
*   C++11 minimum.  Uses D_TEST_CONSTEXPR for relaxed constexpr
* paths on C++14+.  C++20 concepts are used when available;
* pre-C++20 falls back to static_assert validation.
*
*
* TABLE OF CONTENTS
* =================
* I.    UNDERLYING PROTOCOL DETECTION
* II.   TEST TREE
* III.  SUBTREE COMBINATION
* IV.   CONVENIENCE ALIASES
*
*
* path:      /inc/djinterp/test/test_tree.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.11
******************************************************************************/

#ifndef DJINTERP_TEST_TREE_
#define DJINTERP_TEST_TREE_ 1

// std
#include <cstddef>
#include <initializer_list>
#include <type_traits>
#include <utility>
// djinterp
#include "../core/djinterp.hpp"
#include "../core/meta/type_traits.hpp"
#include "./test_common.hpp"
#include "./test_object_traits.hpp"


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   UNDERLYING PROTOCOL DETECTION                       ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // has_root_method
    //   trait: true if _Type exposes root().
    template<typename _Type,
             typename = void>
    struct has_root_method : std::false_type
    {};

    template<typename _Type>
    struct has_root_method<_Type,
        void_t<decltype(std::declval<_Type&>().root())>>
        : std::true_type
    {};

    // has_size_accessor
    //   trait: true if _Type exposes size().
    template<typename _Type,
             typename = void>
    struct has_size_accessor : std::false_type
    {};

    template<typename _Type>
    struct has_size_accessor<_Type,
        void_t<decltype(std::declval<const _Type&>().size())>>
        : std::true_type
    {};

    // has_empty_method
    //   trait: true if _Type exposes empty().
    template<typename _Type,
             typename = void>
    struct has_empty_method : std::false_type
    {};

    template<typename _Type>
    struct has_empty_method<_Type,
        void_t<decltype(std::declval<const _Type&>().empty())>>
        : std::true_type
    {};

    // has_clear_method
    //   trait: true if _Type exposes clear().
    template<typename _Type,
             typename = void>
    struct has_clear_method : std::false_type
    {};

    template<typename _Type>
    struct has_clear_method<_Type,
        void_t<decltype(std::declval<_Type&>().clear())>>
        : std::true_type
    {};

    // has_begin_end
    //   trait: true if _Type exposes begin() and end().
    template<typename _Type,
             typename = void>
    struct has_begin_end : std::false_type
    {};

    template<typename _Type>
    struct has_begin_end<_Type,
        void_t<decltype(std::declval<_Type&>().begin()),
                  decltype(std::declval<_Type&>().end())>>
        : std::true_type
    {};

    // has_rank_method
    //   trait: true if _Type exposes rank().
    template<typename _Type,
             typename = void>
    struct has_rank_method : std::false_type
    {};

    template<typename _Type>
    struct has_rank_method<_Type,
        void_t<decltype(std::declval<const _Type&>().rank())>>
        : std::true_type
    {};

NS_END  // internal


///////////////////////////////////////////////////////////////////////////////
///                II.  TEST TREE                                           ///
///////////////////////////////////////////////////////////////////////////////

// test_tree
//   class: overlay container wrapping any n-ary tree whose
// elements satisfy the test object protocol.  Delegates all
// storage and structural operations to the underlying
// container, adding test-domain queries and evaluation
// dispatch.
//
//   _Element must satisfy the test object protocol: boolean
// conversion and status() accessor.  _Underlying must expose
// at minimum root() and size().
//
// Template parameters:
//   _Element:       the test object element type.
//   _Underlying:    any n-ary tree container holding _Element
//                   nodes.
//   _ValidateRank:  compile-time flag.  When true (default),
//                   add_child enforces the rank invariant
//                   (child.rank <= parent.rank).  When false,
//                   rank checks are compiled out entirely.
//
// Usage:
//   using my_tree = test_tree<basic_test, some_nary_tree<basic_test>>;
//   my_tree t;
//   // ... populate via underlying() ...
//   auto n = t.count_passed();
//
//   // disable rank validation:
//   using loose_tree = test_tree<basic_test, some_nary_tree<basic_test>, false>;
template<typename _Element,
         typename _Underlying,
         bool     _ValidateRank = true>
class test_tree
{
    static_assert(
        is_test_evaluable<_Element>::value,
        "`_Element` must be convertible to bool (test "
        "object protocol).");

    static_assert(
        internal::has_root_method<_Underlying>::value,
        "`_Underlying` must expose root() (n-ary tree "
        "protocol).");

    static_assert(
        internal::has_size_accessor<_Underlying>::value,
        "`_Underlying` must expose size() (n-ary tree "
        "protocol).");

public:
    // -----------------------------------------------------------------
    //  type aliases
    // -----------------------------------------------------------------
    using value_type                = _Element;
    using underlying_container_type = _Underlying;
    using size_type                 = std::size_t;
    using depth_type                = std::size_t;

    // compile-time rank validation flag
    static constexpr bool validate_rank = _ValidateRank;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // test_tree
    //   constructor: default.  Creates an empty overlay
    // wrapping a default-constructed underlying container.
    test_tree()
        : m_underlying()
    {}

    // test_tree
    //   constructor: from underlying.  Takes ownership of
    // an existing underlying container via move.
    explicit test_tree(
        _Underlying&& _tree
    )
        : m_underlying(static_cast<_Underlying&&>(_tree))
    {}

    // test_tree
    //   constructor: from underlying (copy).
    explicit test_tree(
            const _Underlying& _tree
        )
            : m_underlying(_tree)
    {}

    // -----------------------------------------------------------------
    //  underlying access
    // -----------------------------------------------------------------

    // underlying
    //   returns a mutable reference to the underlying
    // container for direct manipulation.
    _Underlying&
    underlying() D_NOEXCEPT
    {
        return m_underlying;
    }

    // underlying (const)
    const _Underlying&
    underlying() const D_NOEXCEPT
    {
        return m_underlying;
    }

    // -----------------------------------------------------------------
    //  forwarded capacity
    // -----------------------------------------------------------------

    // size
    //   returns the number of nodes in the underlying tree.
    size_type
    size() const D_NOEXCEPT
    {
        return m_underlying.size();
    }

    // empty
    //   returns true if the underlying tree is empty.
    bool
    empty() const D_NOEXCEPT
    {
        return (m_underlying.size() == 0);
    }

    // -----------------------------------------------------------------
    //  forwarded root access
    // -----------------------------------------------------------------

    // root
    //   returns the root handle from the underlying tree.
    //
    //   Trailing return types are parsed BEFORE the class body
    // is complete, so members declared later in the class
    // (here, `m_underlying` at the bottom of the class) are
    // not yet visible at this point.  An earlier form of this
    // declaration used `decltype(m_underlying.root())`, which
    // produced "use of undeclared identifier 'm_underlying'"
    // under clang / MSVC.  We side-step the issue entirely by
    // deducing the return type from the template parameter
    // `_Underlying`, which IS in scope in the trailing-return
    // context - the function body is unchanged.
    auto
    root() -> decltype(std::declval<_Underlying&>().root())
    {
        return m_underlying.root();
    }

    // root (const)
    auto
    root() const -> decltype(std::declval<const _Underlying&>().root())
    {
        return m_underlying.root();
    }

    // -----------------------------------------------------------------
    //  forwarded clear
    // -----------------------------------------------------------------

    // clear
    //   clears the underlying tree.
    void
    clear()
    {
        m_underlying.clear();

        return;
    }

    // -----------------------------------------------------------------
    //  rank-validated insertion
    // -----------------------------------------------------------------

    // validate_rank_invariant
    //   returns true if _child may be added under _parent
    // according to the rank invariant (child.rank <=
    // parent.rank).  When _ValidateRank is false, always
    // returns true - the check is compiled out entirely.
    //
    //   When the element type does not expose rank(), the
    // check is also compiled out (no constraint to enforce).
    template<typename _Parent,
             typename _Child>
    static D_CONSTEXPR bool
    validate_rank_invariant(
        const _Parent& _parent,
        const _Child&  _child
    ) D_NOEXCEPT
    {
#if D_ENV_LANG_IS_CPP17_OR_HIGHER
        if constexpr ( (_ValidateRank) &&
                       (internal::has_rank_method<_Parent>::value) &&
                       (internal::has_rank_method<_Child>::value) )
        {
            return (_child.rank() <= _parent.rank());
        }
        else
        {
            (void)_parent;
            (void)_child;

            return true;
        }
#else
        return validate_rank_dispatch(
            _parent,
            _child,
            std::integral_constant<bool,
                ( _ValidateRank &&
                  internal::has_rank_method<_Parent>::value &&
                  internal::has_rank_method<_Child>::value )>{});
#endif
    }

    // -----------------------------------------------------------------
    //  test-domain queries
    // -----------------------------------------------------------------

    // count_by_status
    //   traverses the underlying tree and counts elements
    // whose status() matches _status.  Requires the
    // underlying container to support pre-order iteration
    // via for_each or begin/end.
    template<typename _StatusType>
    size_type
    count_by_status(
        _StatusType _status
    ) const
    {
        size_type count = 0;

        count_by_status_impl(
            _status,
            count,
            typename internal::has_begin_end<
                const _Underlying>::type{});

        return count;
    }

    // count_passed
    //   returns the number of elements with status passed (0).
    size_type
    count_passed() const
    {
        return count_by_status(
            static_cast<test_status>(0));
    }

    // count_failed
    //   returns the number of elements with status failed (1).
    size_type
    count_failed() const
    {
        return count_by_status(
            static_cast<test_status>(1));
    }

    // count_skipped
    //   returns the number of elements with status skipped (2).
    size_type
    count_skipped() const
    {
        return count_by_status(
            static_cast<test_status>(2));
    }

    // count_pending
    //   returns the number of elements with status pending (3).
    size_type
    count_pending() const
    {
        return count_by_status(
            static_cast<test_status>(3));
    }

    // all_passed
    //   returns true if every element in the tree has status
    // passed.  An empty tree returns true (vacuous truth).
    bool
    all_passed() const
    {
        return (count_failed() == 0 &&
                count_pending() == 0 &&
                count_by_status(
                    static_cast<test_status>(4)) == 0);
    }

    // any_failed
    //   returns true if at least one element has status
    // failed or error.
    bool
    any_failed() const
    {
        return (count_failed() > 0 ||
                count_by_status(
                    static_cast<test_status>(4)) > 0);
    }

    // -----------------------------------------------------------------
    //  subtree composition
    // -----------------------------------------------------------------

    // graft
    //   Copies the entire structure of _src under _parent in this
    // tree.  _src is unchanged; this tree gains a deep copy of
    // every node in _src rooted as a child of _parent.
    //   Useful for assembling a multi-module test session out of
    // independently-built per-module subtrees: build each module's
    // tree with its own builder function, then graft each one
    // under a shared parent root in a master tree.
    //   The underlying container must support append_child and the
    // node protocol (data() / first_child() / next_sibling()) for
    // walking the source.  Any other underlying that exposes these
    // names will work too.
    //
    //   No-ops (returns nullptr) if either:
    //     - _parent is null, or
    //     - _src has no root (empty source tree).
    //
    //   Rank invariant: when _ValidateRank is true and both element
    // types expose rank(), grafting honors the same parent-vs-child
    // rank check as direct insertion.  The check is applied at the
    // graft seam (parent vs. _src.root()); deeper rank relationships
    // inside _src are presumed already valid because _src was
    // assembled under the same invariant.
    //
    // Parameters:
    //   _parent: pointer to the node in this tree under which _src's
    //            structure will be cloned.  Must belong to this
    //            tree's underlying container.
    //   _src:    source subtree to clone.  Read-only.
    //
    // Returns:
    //   pointer to the new node corresponding to _src's root in this
    //   tree, or nullptr if no graft was performed.
    template<typename _OtherUnderlying>
    auto
    graft(
        decltype(std::declval<_Underlying&>().root())  _parent,
        const test_tree<_Element, _OtherUnderlying>&   _src
    )
        -> decltype(std::declval<_Underlying&>().root())
    {
        return graft_impl(_parent, _src.underlying().root());
    }

    // append_subtree
    //   Convenience wrapper: grafts _src under this tree's existing
    // root.  Equivalent to `graft(this->root(), _src)`.
    //
    //   No-op (returns nullptr) if this tree has no root yet — call
    // emplace_root on the underlying first, or use combine_subtrees
    // (below) which constructs a fresh root from a label.
    template<typename _OtherUnderlying>
    auto
    append_subtree(
        const test_tree<_Element, _OtherUnderlying>& _src
    )
        -> decltype(std::declval<_Underlying&>().root())
    {
        return graft_impl(m_underlying.root(),
                          _src.underlying().root());
    }

    // merge_into
    //   Grafts every immediate child of _src's root under this
    // tree's root.  Differs from append_subtree() in that the
    // source's root node itself is dropped; only its descendants
    // are transplanted.
    //
    //   Useful when _src already has a wrapping module-kind root
    // and the caller wants its categories to appear directly under
    // this tree's root rather than under another nested module.
    //
    //   No-op if either tree has no root.
    template<typename _OtherUnderlying>
    void
    merge_into(
        const test_tree<_Element, _OtherUnderlying>& _src
    )
    {
        auto* dest_root = m_underlying.root();
        auto* src_root  = _src.underlying().root();

        if ((dest_root == nullptr) || (src_root == nullptr))
        {
            return;
        }

        for (auto* child = src_root->first_child();
             child != nullptr;
             child = child->next_sibling())
        {
            graft_impl(dest_root, child);
        }

        return;
    }

private:
    // -----------------------------------------------------------------
    //  internal: graft worker
    // -----------------------------------------------------------------

    // graft_impl
    //   Recursive worker for graft / append_subtree / merge_into.
    // Walks _src in pre-order and reconstructs the structure as a
    // child-chain rooted at _parent in this tree's underlying
    // container.
    //
    //   Templated on the source-node pointer type so the same
    // worker accepts both this tree's own node types and those of
    // a structurally-compatible foreign underlying.  Both must
    // expose data() / first_child() / next_sibling().
    template<typename _SrcNodePtr>
    auto
    graft_impl(
        decltype(std::declval<_Underlying&>().root()) _parent,
        _SrcNodePtr                                   _src
    )
        -> decltype(std::declval<_Underlying&>().root())
    {
        if ( (_parent == nullptr) ||
             (_src    == nullptr) )
        {
            return nullptr;
        }

        // copy the source root's value as a child of _parent
        auto* placed = m_underlying.append_child(_parent,
                                                 _src->data());

        // recurse over the source root's children
        for (auto* child = _src->first_child();
             child != nullptr;
             child = child->next_sibling())
        {
            graft_impl(placed, child);
        }

        return placed;
    }

    // -----------------------------------------------------------------
    //  internal: rank validation dispatch (pre-C++17)
    // -----------------------------------------------------------------

    // validate_rank_dispatch (enabled)
    template<typename _Parent,
             typename _Child>
    static D_CONSTEXPR bool
    validate_rank_dispatch(
        const _Parent& _parent,
        const _Child&  _child,
        std::true_type
    ) D_NOEXCEPT
    {
        return (_child.rank() <= _parent.rank());
    }

    // validate_rank_dispatch (disabled)
    template<typename _Parent,
             typename _Child>
    static D_CONSTEXPR bool
    validate_rank_dispatch(
        const _Parent&,
        const _Child&,
        std::false_type
    ) D_NOEXCEPT
    {
        return true;
    }

    // -----------------------------------------------------------------
    //  internal: status counting with begin/end
    // -----------------------------------------------------------------

    // count_by_status_impl (iterable)
    template<typename _StatusType>
    void
    count_by_status_impl(
        _StatusType _status,
        size_type&  _count,
        std::true_type
    ) const
    {
        for (auto it = m_underlying.begin();
             it != m_underlying.end();
             ++it)
        {
            if (static_cast<int>(it->status()) ==
                static_cast<int>(_status))
            {
                ++_count;
            }
        }

        return;
    }

    // count_by_status_impl (non-iterable fallback)
    //   when the underlying container does not expose
    // begin/end, counting is not supported and returns 0.
    template<typename _StatusType>
    void
    count_by_status_impl(
        _StatusType,
        size_type&,
        std::false_type
    ) const
    {
        return;
    }

    _Underlying m_underlying;
};


///////////////////////////////////////////////////////////////////////////////
///                III. SUBTREE COMBINATION                                 ///
///////////////////////////////////////////////////////////////////////////////

// combine_subtrees
//   Free factory: builds a fresh test_tree whose root is a
// freshly-constructed _RootValue and whose children are deep
// copies of every subtree in _sources, grafted under the new
// root in document order.
//
//   This is the high-level "many independent module trees -> one
// suite tree" composition primitive.  Each entry in _sources is
// expected to be a fully-built test_tree (typically returned by
// a per-module make_*_subtree() builder).  The returned tree
// owns its own storage, independent of any source.
//
// Usage:
//
//   array_test_tree suite = combine_subtrees<
//       array_test_tree>(
//           make_test_module("array suite"),
//           {
//               make_array_test_subtree(),
//               make_threadsafe_array_test_subtree()
//           });
//
//   handler.run(suite.underlying());
//
// Template parameters:
//   _Tree       : the output test_tree type (concrete
//                 instantiation expected; element + underlying
//                 are deduced from it).
//   _RootValue  : the type of the root node's value (typically
//                 the same as _Tree::value_type, but accepting
//                 any forwardable type widens convenience).
//
// Parameters:
//   _root_value : value placed at the root of the returned
//                 tree.  Forwarded into emplace_root.
//   _sources    : initializer list of source subtrees to graft
//                 (in order) under the new root.
template<typename _Tree, 
         typename _RootValue>
_Tree
combine_subtrees(
    _RootValue&&                 _root_value,
    std::initializer_list<_Tree> _sources
)
{
    _Tree result;

    result.underlying().emplace_root(
        std::forward<_RootValue>(_root_value));

    for (const auto& src : _sources)
    {
        result.append_subtree(src);
    }

    return result;
}


///////////////////////////////////////////////////////////////////////////////
///                IV.  CONVENIENCE ALIASES                                 ///
///////////////////////////////////////////////////////////////////////////////

// Convenience aliases are defined in test_defaults.hpp after
// the concrete element types and underlying containers are
// available.


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_TREE_