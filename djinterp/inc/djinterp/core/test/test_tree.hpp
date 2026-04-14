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
* at compile time — when false, rank checks are compiled out
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
* III.  CONVENIENCE ALIASES
*
*
* path:      /inc/djinterp/test/test_tree.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.04.11
******************************************************************************/

#ifndef DJINTERP_TEST_TREE_
#define DJINTERP_TEST_TREE_ 1

#include <cstddef>
#include <type_traits>
#include "../djinterp.hpp"
#include "../meta/type_traits.hpp"
#include "./test_common.hpp"
#include "./test_object_traits.hpp"


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   UNDERLYING PROTOCOL DETECTION                       ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // has_root_method
    //   trait: true if _T exposes root().
    template<typename _T,
             typename = void>
    struct has_root_method : std::false_type
    {};

    template<typename _T>
    struct has_root_method<_T,
        D_VOID_T<decltype(std::declval<_T&>().root())>>
        : std::true_type
    {};

    // has_size_method
    //   trait: true if _T exposes size().
    template<typename _T,
             typename = void>
    struct has_size_method : std::false_type
    {};

    template<typename _T>
    struct has_size_method<_T,
        D_VOID_T<decltype(std::declval<const _T&>().size())>>
        : std::true_type
    {};

    // has_empty_method
    //   trait: true if _T exposes empty().
    template<typename _T,
             typename = void>
    struct has_empty_method : std::false_type
    {};

    template<typename _T>
    struct has_empty_method<_T,
        D_VOID_T<decltype(std::declval<const _T&>().empty())>>
        : std::true_type
    {};

    // has_clear_method
    //   trait: true if _T exposes clear().
    template<typename _T,
             typename = void>
    struct has_clear_method : std::false_type
    {};

    template<typename _T>
    struct has_clear_method<_T,
        D_VOID_T<decltype(std::declval<_T&>().clear())>>
        : std::true_type
    {};

    // has_begin_end
    //   trait: true if _T exposes begin() and end().
    template<typename _T,
             typename = void>
    struct has_begin_end : std::false_type
    {};

    template<typename _T>
    struct has_begin_end<_T,
        D_VOID_T<decltype(std::declval<_T&>().begin()),
                  decltype(std::declval<_T&>().end())>>
        : std::true_type
    {};

    // has_rank_method
    //   trait: true if _T exposes rank().
    template<typename _T,
             typename = void>
    struct has_rank_method : std::false_type
    {};

    template<typename _T>
    struct has_rank_method<_T,
        D_VOID_T<decltype(std::declval<const _T&>().rank())>>
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
        traits::is_test_evaluable<_Element>::value,
        "`_Element` must be convertible to bool (test "
        "object protocol).");

    static_assert(
        internal::has_root_method<_Underlying>::value,
        "`_Underlying` must expose root() (n-ary tree "
        "protocol).");

    static_assert(
        internal::has_size_method<_Underlying>::value,
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
    underlying() noexcept
    {
        return m_underlying;
    }

    // underlying (const)
    const _Underlying&
    underlying() const noexcept
    {
        return m_underlying;
    }

    // -----------------------------------------------------------------
    //  forwarded capacity
    // -----------------------------------------------------------------

    // size
    //   returns the number of nodes in the underlying tree.
    size_type
    size() const noexcept
    {
        return m_underlying.size();
    }

    // empty
    //   returns true if the underlying tree is empty.
    bool
    empty() const noexcept
    {
        return (m_underlying.size() == 0);
    }

    // -----------------------------------------------------------------
    //  forwarded root access
    // -----------------------------------------------------------------

    // root
    //   returns the root handle from the underlying tree.
    auto
    root() -> decltype(m_underlying.root())
    {
        return m_underlying.root();
    }

    // root (const)
    auto
    root() const -> decltype(m_underlying.root())
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
    // returns true — the check is compiled out entirely.
    //
    //   When the element type does not expose rank(), the
    // check is also compiled out (no constraint to enforce).
    template<typename _Parent,
             typename _Child>
    static D_CONSTEXPR bool
    validate_rank_invariant(
        const _Parent& _parent,
        const _Child&  _child
    ) noexcept
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

private:
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
    ) noexcept
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
    ) noexcept
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
///                III. CONVENIENCE ALIASES                                 ///
///////////////////////////////////////////////////////////////////////////////

// Convenience aliases are defined in test_defaults.hpp after
// the concrete element types and underlying containers are
// available.


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_TREE_
