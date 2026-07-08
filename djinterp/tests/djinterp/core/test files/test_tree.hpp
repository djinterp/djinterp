/******************************************************************************
* djinterp [test]                                                test_tree.hpp
*
*   The default, general-use test container.  test_tree is one concrete
* implementation of the test_container contract (test_container.hpp);
* using it is not compulsory - any user container meeting the minimum
* contract is acceptable - but it is the framework reference and is
* tuned to suit the common case.
*
*   SHAPE:
*   test_tree pairs a set of test kinds with a forest of root
*   test_objects:
*       test_tree = { kinds ; <forest of root test_objects> }
*   The forest is a single backing tree (default nary_tree<_Element>)
* whose ROOT NODE is the implied conjunctive root of the formal model:
* the sequential test roots (the queue) are its immediate children, and
* the whole run succeeds iff every root succeeds.  The conjunctive root
* is created on first use and is an ordinary node in the walk; the owning
* handler derives its status from its children during evaluation.
*
*   KINDS AND RANK:
*   The kinds are stored as any range of test_kind records (default
* std::vector<test_kind>; a test_kind_set<C> works equally, since the
* resolved-query free functions in test_kind.hpp accept either).  They
* are metadata held alongside the tree, never nodes in it.  Insertion is
* routed through those queries: append_child rejects a child under a
* registered leaf parent, and otherwise enforces rank monotonicity via
* can_be_child_of(kinds, child, parent).  Top-level roots (add_root) are
* unconstrained - the conjunctive root admits any sequence.  The
* _ValidateRank flag compiles the checks out entirely when false.
*
*   RUN SURFACE:
*   A sequential counting surface (count_passed / count_failed /
* count_skipped / count_pending, all_passed, any_failed) aggregates node
* status across the forest.  test_object carries no rank or identity of
* its own - the kind set supplies rank/leaf classification, and the
* handler supplies address and depth during the walk.
*
*   DESIGN:
*   test_tree delegates all storage to its backing forest, adding the
* kind metadata, the rank-checked build surface, and the run surface on
* top.  It is templated so callers may vary the element type, the forest
* storage, and the kind container, with sensible defaults.
*
*   This module supersedes the old test_tree overlay and folds in the
* detection that previously lived in the retired test_tree_traits.hpp /
* test_tree_concepts.hpp: only is_test_tree (recogniser for this class
* template) remains, restated here as a trait and a C++20 concept.
*
*   PORTABILITY:
*   C++11 minimum.  `_v` companions on C++14+.  Concepts on C++20.
*
*
* TABLE OF CONTENTS
* =================
* I.    TEST TREE
* II.   TEST-TREE DETECTION
* III.  CONVENIENCE ALIASES
*
*
* path:      /inc/djinterp/test/test_tree.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.17
******************************************************************************/

#ifndef DJINTERP_TEST_TREE_
#define DJINTERP_TEST_TREE_ 1

#ifndef __cplusplus
    #error "test_tree.hpp requires C++ compilation"
#endif

// std
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "../core/meta/type_traits.hpp"
#include "../core/container/tree/nary/nary_tree.hpp"
#include "./test_common.hpp"
#include "./test_kind.hpp"
#include "./test_container.hpp"


#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "test_tree.hpp requires C++11 or higher"
#endif


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   TEST TREE                                            ///
///////////////////////////////////////////////////////////////////////////////

// test_tree
//   class: the default, general-use test_container.  Pairs a set of
// test kinds with a forest of root test_objects held in a backing tree
// whose root node is the implied conjunctive root.  Delegates storage
// to the backing, adding kind metadata, a rank-checked build surface,
// and a run / counting surface.
//
// Template parameters:
//   _Element:       the test object element type (must satisfy the test
//                   object protocol, is_test_evaluable).
//   _Underlying:    the forest backing.  Must expose root(), size(),
//                   append_child(node_type*, value_type), and begin()/
//                   end().  Default: nary_tree<_Element>.
//   _KindContainer: storage for the test kinds - any range of test_kind
//                   records (a test_kind_set<C> also qualifies).
//                   Default: std::vector<test_kind>.
//   _ValidateRank:  compile-time flag.  When true (default), append_child
//                   enforces the leaf / rank rules; when false they are
//                   compiled out.
//
// Usage:
//   test_tree<basic_test> t;                 // default-backed
//   auto* r = t.add_root(make_test(0, true));
//   t.append_child(r, make_test(1, true));   // rank-checked
//   bool ok = t.all_passed();
template<typename _Element,
         typename _Underlying    = ::djinterp::nary_tree<_Element>,
         typename _KindContainer = ::std::vector<test_kind>,
         bool     _ValidateRank  = true>
class test_tree
{
    static_assert(
        is_test_evaluable<_Element>::value,
        "`_Element` must satisfy the test object protocol "
        "(is_test_evaluable).");

    static_assert(
        has_root_method<_Underlying>::value,
        "`_Underlying` must expose root() (forest backing).");

    static_assert(
        has_size_accessor<_Underlying>::value,
        "`_Underlying` must expose size() (forest backing).");

    static_assert(
        has_append_child_method<_Underlying>::value,
        "`_Underlying` must expose append_child(node_type*, "
        "value_type) (forest backing).");

    static_assert(
        has_begin_end<_Underlying>::value,
        "`_Underlying` must expose begin()/end() for the run / "
        "counting surface.");

public:
    // -----------------------------------------------------------------
    //  type aliases
    // -----------------------------------------------------------------
    using value_type                = _Element;
    using underlying_container_type = _Underlying;
    using kind_container_type       = _KindContainer;
    using node_type                 = typename _Underlying::node_type;
    using size_type                 = std::size_t;

    // compile-time rank-validation flag
    static constexpr bool validate_rank = _ValidateRank;


    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    // test_tree
    //   constructor: default.  Empty kind set, empty forest (the
    // conjunctive root is created on first add_root).
    test_tree()
        : m_kinds(),
          m_forest()
    {}

    // test_tree
    //   constructor: from a kind container (move).
    explicit test_tree(
        kind_container_type _kinds
    )
        : m_kinds(static_cast<kind_container_type&&>(_kinds)),
          m_forest()
    {}

    // test_tree
    //   constructor: from a kind container and a pre-built forest (move).
    test_tree(
        kind_container_type       _kinds,
        underlying_container_type _forest
    )
        : m_kinds(static_cast<kind_container_type&&>(_kinds)),
          m_forest(static_cast<underlying_container_type&&>(_forest))
    {}


    // -----------------------------------------------------------------
    //  kind access
    // -----------------------------------------------------------------

    // kinds
    //   returns a mutable reference to the kind container.
    kind_container_type&
    kinds() D_NOEXCEPT
    {
        return m_kinds;
    }

    // kinds (const)
    const kind_container_type&
    kinds() const D_NOEXCEPT
    {
        return m_kinds;
    }


    // -----------------------------------------------------------------
    //  underlying access
    // -----------------------------------------------------------------

    // underlying
    //   returns a mutable reference to the backing forest.
    underlying_container_type&
    underlying() D_NOEXCEPT
    {
        return m_forest;
    }

    // underlying (const)
    const underlying_container_type&
    underlying() const D_NOEXCEPT
    {
        return m_forest;
    }


    // -----------------------------------------------------------------
    //  forwarded capacity
    // -----------------------------------------------------------------

    // size
    size_type
    size() const D_NOEXCEPT
    {
        return m_forest.size();
    }

    // empty
    bool
    empty() const D_NOEXCEPT
    {
        return (m_forest.size() == 0);
    }


    // -----------------------------------------------------------------
    //  forwarded iteration (sequential walk of the forest)
    // -----------------------------------------------------------------
    //   Trailing return types deduce from `_Underlying`, which is in
    // scope here, since members declared lower in the class are not yet
    // visible in the trailing-return context.

    auto
    begin() -> decltype(std::declval<_Underlying&>().begin())
    {
        return m_forest.begin();
    }

    auto
    end() -> decltype(std::declval<_Underlying&>().end())
    {
        return m_forest.end();
    }

    auto
    begin() const -> decltype(std::declval<const _Underlying&>().begin())
    {
        return m_forest.begin();
    }

    auto
    end() const -> decltype(std::declval<const _Underlying&>().end())
    {
        return m_forest.end();
    }


    // -----------------------------------------------------------------
    //  forwarded root (the implied conjunctive root)
    // -----------------------------------------------------------------

    auto
    root() -> decltype(std::declval<_Underlying&>().root())
    {
        return m_forest.root();
    }

    auto
    root() const -> decltype(std::declval<const _Underlying&>().root())
    {
        return m_forest.root();
    }


    // -----------------------------------------------------------------
    //  forwarded clear
    // -----------------------------------------------------------------

    void
    clear()
    {
        m_forest.clear();

        return;
    }


    // -----------------------------------------------------------------
    //  rank-checked build surface
    // -----------------------------------------------------------------

    // add_root
    //   appends _child as a new top-level test root - a child of the
    // implied conjunctive root.  Top-level roots are unconstrained (the
    // conjunctive root admits any sequence), so this is NOT rank-checked.
    // Creates the conjunctive root on first use.  Returns the new node.
    node_type*
    add_root(
        value_type _child
    )
    {
        node_type* conjunctive = ensure_conjunctive_root();

        return m_forest.append_child(conjunctive, _child);
    }

    // append_child
    //   appends _child under _parent within a tree, enforcing the
    // structural rules through the kind set: a registered leaf parent
    // admits no children, and otherwise the child's resolved rank must
    // not exceed the parent's (can_be_child_of).  Returns nullptr if
    // _parent is null or the insertion is rejected.  When _ValidateRank
    // is false the checks are compiled out.
    node_type*
    append_child(
        node_type* _parent,
        value_type _child
    )
    {
        if (_parent == nullptr)
        {
            return nullptr;
        }

        if (!rank_admits(_parent->data(), _child))
        {
            return nullptr;
        }

        return m_forest.append_child(_parent, _child);
    }


    // -----------------------------------------------------------------
    //  run / counting surface
    // -----------------------------------------------------------------

    // count_by_status
    //   walks the forest and counts elements whose status() matches
    // _status (compared numerically, so any status representation works).
    template<typename _StatusType>
    size_type
    count_by_status(
        _StatusType _status
    ) const
    {
        size_type count = 0;

        for (auto it = m_forest.begin();
             it != m_forest.end();
             ++it)
        {
            if (static_cast<int>(it->status()) ==
                static_cast<int>(_status))
            {
                ++count;
            }
        }

        return count;
    }

    // count_passed
    size_type
    count_passed() const
    {
        return count_by_status(test_status::passed);
    }

    // count_failed
    size_type
    count_failed() const
    {
        return count_by_status(test_status::failed);
    }

    // count_skipped
    size_type
    count_skipped() const
    {
        return count_by_status(test_status::skipped);
    }

    // count_pending
    size_type
    count_pending() const
    {
        return count_by_status(test_status::pending);
    }

    // all_passed
    //   true iff no node is failed, pending, or error.
    bool
    all_passed() const
    {
        return ( (count_by_status(test_status::failed)  == 0) &&
                 (count_by_status(test_status::pending) == 0) &&
                 (count_by_status(test_status::error)   == 0) );
    }

    // any_failed
    //   true iff at least one node is failed or error.
    bool
    any_failed() const
    {
        return ( (count_by_status(test_status::failed) > 0) ||
                 (count_by_status(test_status::error)  > 0) );
    }


private:
    // -----------------------------------------------------------------
    //  internal: conjunctive root
    // -----------------------------------------------------------------

    // ensure_conjunctive_root
    //   creates the implied conjunctive root (a default-constructed
    // element) if the forest is empty, then returns it.
    node_type*
    ensure_conjunctive_root()
    {
        if (m_forest.root() == nullptr)
        {
            m_forest.emplace_root(value_type());
        }

        return m_forest.root();
    }


    // -----------------------------------------------------------------
    //  internal: rank / leaf admission (dispatch on _ValidateRank)
    // -----------------------------------------------------------------

    // rank_admits
    //   true iff _child may be attached under _parent.
    bool
    rank_admits(
        const value_type& _parent,
        const value_type& _child
    ) const
    {
        return rank_admits_dispatch(
            _parent,
            _child,
            std::integral_constant<bool, _ValidateRank>{});
    }

    // rank_admits_dispatch (validation enabled)
    bool
    rank_admits_dispatch(
        const value_type& _parent,
        const value_type& _child,
        std::true_type
    ) const
    {
        const test_kind* parent_kind =
            find_kind(m_kinds, _parent.type_id());

        // a registered leaf parent admits no children
        if ( (parent_kind != nullptr) &&
             (parent_kind->is_leaf) )
        {
            return false;
        }

        // otherwise enforce rank monotonicity
        return can_be_child_of(
            m_kinds,
            _child.type_id(),
            _parent.type_id());
    }

    // rank_admits_dispatch (validation disabled)
    bool
    rank_admits_dispatch(
        const value_type&,
        const value_type&,
        std::false_type
    ) const
    {
        return true;
    }


    kind_container_type       m_kinds;
    underlying_container_type m_forest;
};


///////////////////////////////////////////////////////////////////////////////
///                II.  TEST-TREE DETECTION                                  ///
///////////////////////////////////////////////////////////////////////////////

NS_INTERNAL

    // is_test_tree_instantiation
    //   trait: detects whether a type is an instantiation of the
    // test_tree class template.
    template<typename _Type>
    struct is_test_tree_instantiation : std::false_type
    {};

    template<typename    _Element,
             typename    _Underlying,
             typename    _KindContainer,
             bool        _ValidateRank>
    struct is_test_tree_instantiation<
        test_tree<_Element, _Underlying, _KindContainer, _ValidateRank>>
        : std::true_type
    {};

NS_END  // internal

// is_test_tree
//   trait: true iff _Type is an instantiation of test_tree.
template<typename _Type>
struct is_test_tree
    : internal::is_test_tree_instantiation<clean_t<_Type>>
{};

D_TYPE_TRAIT_VALUE_BOOL(is_test_tree)


#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// test_tree_type
//   concept: the type is an instantiation of test_tree; mirrors
// is_test_tree.
template<typename _Type>
concept test_tree_type =
    is_test_tree<clean_t<_Type>>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


///////////////////////////////////////////////////////////////////////////////
///                III. CONVENIENCE ALIASES                                  ///
///////////////////////////////////////////////////////////////////////////////

// Convenience aliases for concrete element / backing / kind-container
// combinations are defined in test_defaults.hpp, once basic_test and the
// framework's default kind set are available.


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_TREE_
