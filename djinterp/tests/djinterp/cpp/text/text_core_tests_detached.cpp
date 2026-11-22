/*******************************************************************************
* djinterp [cpp][text]                              text_core_tests_detached.cpp
*
*   The groups for detached.hpp: the owning handle the markup modules'
* factories return, and the adoption they share. The handle is tested over a
* facade of the suite's own, whose node counts how many of it are alive, so
* that what is freed, and when, is a check rather than a sanitizer's report.
*
*
* path:      /tests/djinterp/cpp/text/text_core_tests_detached.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.11.18
*                                                            revised: 2026.11.18
*******************************************************************************/
#include "text_core_tests.hpp"  // D_TESTS_TEXT_CHECK, the groups
// std
#include <memory>       // std::unique_ptr
#include <type_traits>  // std::is_*
#include <utility>      // std::move
#include <vector>       // std::vector
// djinterp
#include "../../../../inc/djinterp/cpp/text/detached.hpp"  // DDetached


#if D_ENV_LANG_IS_CPP11_OR_HIGHER

/*
DTestsCountedNode
  A node that counts how many of it are alive, with the children and parent
adoption needs.
*/
struct DTestsCountedNode
{
    static int s_live;

    int                                             value;
    std::vector<std::unique_ptr<DTestsCountedNode>> children;
    DTestsCountedNode*                              parent;

    DTestsCountedNode()
    :   value(0),
        children(),
        parent(nullptr)
    {
        ++s_live;
    }

    explicit
    DTestsCountedNode(
        int _value
    )
    :   value(_value),
        children(),
        parent(nullptr)
    {
        ++s_live;
    }

    ~DTestsCountedNode()
    {
        --s_live;
    }

    DTestsCountedNode(const DTestsCountedNode&)            = delete;
    DTestsCountedNode& operator=(const DTestsCountedNode&) = delete;
};

int DTestsCountedNode::s_live = 0;

/*
DTestsCounted
  A facade over a counted node, shaped as the markup facades are.
*/
class DTestsCounted
{
public:
    using node_type = DTestsCountedNode;

    DTestsCounted()
    :   m_node(nullptr)
    {}

    explicit
    DTestsCounted(
        node_type* _node
    )
    :   m_node(_node)
    {}

    bool valid() const
    {
        return (m_node != nullptr);
    }

    int value() const
    {
        return m_node->value;
    }

    node_type* backend_handle() const
    {
        return m_node;
    }

    // adopt
    //   adoption as the markup facades do it.
    DTestsCounted adopt(
        djinterp::DDetached<DTestsCounted>&& _child
    )
    {
        return DTestsCounted(
            djinterp::internal::detached_access::adopt(m_node, _child));
    }

protected:
    node_type* m_node;
};

/*
DTestsCountedLeaf
  A facade that converts to DTestsCounted, as a Less rule's does to a CSS
rule's.
*/
class DTestsCountedLeaf
:   public DTestsCounted
{
public:
    DTestsCountedLeaf()
    :   DTestsCounted()
    {}

    explicit
    DTestsCountedLeaf(
        node_type* _node
    )
    :   DTestsCounted(_node)
    {}

    int doubled() const
    {
        return (2 * m_node->value);
    }
};

/*
DTestsOther
  A facade over another node type, which no counted handle converts to.
*/
class DTestsOther
{
public:
    using node_type = int;

    DTestsOther()
    :   m_node(nullptr)
    {}

    explicit
    DTestsOther(
        node_type* _node
    )
    :   m_node(_node)
    {}

    node_type* backend_handle() const
    {
        return m_node;
    }

private:
    node_type* m_node;
};

// counted, counted_leaf
//   the handles under test.
typedef djinterp::DDetached<DTestsCounted>     counted;
typedef djinterp::DDetached<DTestsCountedLeaf> counted_leaf;


/*
d_tests_text_core_detached_own
  Tests the following:
  - a default handle is empty, gives an empty facade, and frees nothing
  - make makes one node, with its arguments; get, the arrow and
    backend_handle reach it
  - a handle frees its node when it goes, or on reset
  - a move takes the node and empties the source; a move assignment frees
    the node it replaces
  - a handle doesn't copy
*/
void
d_tests_text_core_detached_own(void)
{
    const int before = DTestsCountedNode::s_live;

    {
        counted none;

        D_TESTS_TEXT_CHECK(!none.valid() && !none &&
                           !none.get().valid() &&
                           (none.backend_handle() == nullptr));

        counted made = counted::make(7);

        D_TESTS_TEXT_CHECK(made.valid() && static_cast<bool>(made) &&
                           (DTestsCountedNode::s_live == before + 1));
        D_TESTS_TEXT_CHECK((made.get().value() == 7) &&
                           (made->value() == 7) &&
                           (made.backend_handle()->value == 7) &&
                           (made.backend_handle()->parent == nullptr));

        counted taken(std::move(made));

        D_TESTS_TEXT_CHECK(!made.valid() && taken.valid() &&
                           (taken->value() == 7) &&
                           (DTestsCountedNode::s_live == before + 1));

        counted other = counted::make(8);

        D_TESTS_TEXT_CHECK(DTestsCountedNode::s_live == before + 2);
        taken = std::move(other);
        D_TESTS_TEXT_CHECK(!other.valid() && (taken->value() == 8) &&
                           (DTestsCountedNode::s_live == before + 1));
        taken.reset();
        D_TESTS_TEXT_CHECK(!taken.valid() &&
                           (DTestsCountedNode::s_live == before));

        counted kept = counted::make();

        D_TESTS_TEXT_CHECK((kept->value() == 0) &&
                           (DTestsCountedNode::s_live == before + 1));
    }

    const bool moves_only =
        (!std::is_copy_constructible<counted>::value) &&
        (!std::is_copy_assignable<counted>::value) &&
        (std::is_nothrow_move_constructible<counted>::value) &&
        (std::is_nothrow_move_assignable<counted>::value);

    D_TESTS_TEXT_CHECK(DTestsCountedNode::s_live == before);
    D_TESTS_TEXT_CHECK(moves_only);

    return;
}


/*
d_tests_text_core_detached_convert
  Tests the following:
  - a handle converts, by move, to the handle of a facade its own facade
    converts to over the same node type, and the node goes with it
  - it doesn't convert the other way, nor to a facade over another node
    type, nor from an lvalue
  - is_detached_handle is true for handles only; is_detached_convertible
    says which facades a handle may become
*/
void
d_tests_text_core_detached_convert(void)
{
    using djinterp::is_detached_convertible;
    using djinterp::is_detached_handle;

    const int before = DTestsCountedNode::s_live;

    {
        counted_leaf leaf = counted_leaf::make(4);

        D_TESTS_TEXT_CHECK(leaf->doubled() == 8);

        counted up(std::move(leaf));

        D_TESTS_TEXT_CHECK(!leaf.valid() && (up->value() == 4) &&
                           (DTestsCountedNode::s_live == before + 1));

        counted made = counted_leaf::make(5);

        D_TESTS_TEXT_CHECK((made->value() == 5) &&
                           (DTestsCountedNode::s_live == before + 2));
    }

    D_TESTS_TEXT_CHECK(DTestsCountedNode::s_live == before);
    const bool up_only =
        (std::is_constructible<counted, counted_leaf&&>::value) &&
        (!std::is_constructible<counted_leaf, counted&&>::value) &&
        (!std::is_constructible<counted, counted_leaf&>::value) &&
        (!std::is_constructible<djinterp::DDetached<DTestsOther>,
                                counted&&>::value);
    const bool handles =
        (is_detached_handle<counted>::value) &&
        (is_detached_handle<counted_leaf>::value) &&
        (!is_detached_handle<DTestsCounted>::value) &&
        (!is_detached_handle<int>::value);
    const bool convertible =
        (is_detached_convertible<DTestsCountedLeaf, DTestsCounted>::value) &&
        (is_detached_convertible<DTestsCounted, DTestsCounted>::value) &&
        (!is_detached_convertible<DTestsCounted, DTestsCountedLeaf>::value) &&
        (!is_detached_convertible<DTestsOther, DTestsCounted>::value);

    D_TESTS_TEXT_CHECK(up_only);
    D_TESTS_TEXT_CHECK(handles);
    D_TESTS_TEXT_CHECK(convertible);

    return;
}


/*
d_tests_text_core_detached_adopt
  Tests the following:
  - adoption appends the node to the parent's children, in order, sets its
    parent, empties the handle, and gives a facade over the node; the
    parent frees it
  - a subtree built under a handle's node goes with it
  - refused, with an empty facade and the handle keeping its node: an
    empty parent, an empty handle, the handle's own node as parent, and a
    parent under the handle's node
*/
void
d_tests_text_core_detached_adopt(void)
{
    const int before = DTestsCountedNode::s_live;

    {
        counted       root  = counted::make(1);
        DTestsCounted top   = root.get();
        DTestsCounted first = top.adopt(counted::make(2));
        counted       sub   = counted::make(3);
        DTestsCounted leaf  = sub->adopt(counted::make(4));
        DTestsCounted placed;

        D_TESTS_TEXT_CHECK(first.valid() && (first.value() == 2) &&
                           (first.backend_handle()->parent ==
                            top.backend_handle()));
        D_TESTS_TEXT_CHECK(!DTestsCounted().adopt(std::move(sub)).valid() &&
                           sub.valid());
        D_TESTS_TEXT_CHECK(!top.adopt(counted()).valid());
        D_TESTS_TEXT_CHECK(!sub->adopt(std::move(sub)).valid() &&
                           sub.valid());
        D_TESTS_TEXT_CHECK(!leaf.adopt(std::move(sub)).valid() &&
                           sub.valid());
        D_TESTS_TEXT_CHECK(DTestsCountedNode::s_live == before + 4);
        placed = top.adopt(std::move(sub));
        D_TESTS_TEXT_CHECK(!sub.valid() && (placed.value() == 3) &&
                           (top.backend_handle()->children.size() == 2u) &&
                           (top.backend_handle()->children[1].get() ==
                            placed.backend_handle()) &&
                           (leaf.backend_handle()->parent ==
                            placed.backend_handle()));
        D_TESTS_TEXT_CHECK(!leaf.adopt(std::move(root)).valid() &&
                           root.valid());
    }

    D_TESTS_TEXT_CHECK(DTestsCountedNode::s_live == before);

    return;
}

#endif  // C++11
