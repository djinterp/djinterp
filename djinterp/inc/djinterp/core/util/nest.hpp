/******************************************************************************
* djinterp [util]                                                     nest.hpp
*
* The C++ face of nest.h: an n-ary node's shape, deduced from the node type
* where it can be and declared where it cannot, with iterators over the result.
*
*
* THE SHAPE IS DEDUCED, NOT DECLARED
* ==================================
*   nest.h has to be told a node's shape, because C cannot look at a struct.
* C++ can. `node_traits` detects `.first_child` / `.next_sibling`, `.children`
* / `.count`, and `.parent`, and reports which shape the node has -- so a node
* type that already looks like a tree needs no descriptor at all:
*
*       struct node { node* parent; node* first_child; node* next_sibling; };
*       static_assert(is_child_sibling_node<node>::value, "");
*
*   DETECTION IS A DEFAULT, NOT A REQUIREMENT. A node whose members are named
* differently declares its shape with child_sibling_shape<...> or
* child_array_shape<...> and everything downstream is unchanged. The deduction
* saves the common case from writing what the compiler can already see; it
* never becomes the only way to say it.
*
*
* WHAT THE ITERATORS BUY
* ======================
*   nest.h offers D_NEST_FOR_EACH_CHILD, and it is quadratic on a sibling
* chain because a macro cannot hold a cursor. An iterator can. `child_range`
* walks a chain in linear total time on the sibling shape and subscripts on the
* array shape, from the same call site, so a caller need not know which shape
* it has to get the right complexity.
*
*   THEY ARE INPUT ITERATORS, deliberately. A forward iterator must guarantee
* that two copies advance independently over the same sequence, which holds
* here, but multi-pass over a structure whose acyclicity is unverified is a
* promise this cannot keep -- so the weaker category is the honest one and
* range-for, which is all most callers want, works either way.
*
*
* CYCLES ARE STILL BOUNDED AND NOT DETECTED
* =========================================
*   `walk_limit` is carried through unchanged from the C view. Detecting a
* cycle needs a visited set on every traversal; bounding one needs a counter.
* The bound is what is offered, and a structure whose acyclicity is an
* invariant elsewhere sets the limit to zero and pays nothing.
*
*
* path:      /inc/djinterp/core/util/nest.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.09.05
******************************************************************************/

#ifndef DJINTERP_UTIL_NEST_
#define DJINTERP_UTIL_NEST_ 1

// std
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <utility>
// djinterp
#include "../../djinterp.hpp"
#include "../../c/util/nest.h"
#include "../meta/kv.hpp"


NS_DJINTERP


// I.     member detection

NS_INTERNAL

    // nest_parent_t / nest_first_child_t / nest_next_sibling_t
    // nest_prev_sibling_t / nest_children_t / nest_count_t
    //   type: the type of each conventionally named link member, if present.
    // Alias templates so a missing member is a substitution failure in the
    // immediate context rather than a hard error; see kv.hpp on why a member
    // typedef would not do.
    template<typename _Node>
    using nest_parent_t = decltype(std::declval<const _Node&>().parent);

    template<typename _Node>
    using nest_first_child_t =
        decltype(std::declval<const _Node&>().first_child);

    template<typename _Node>
    using nest_next_sibling_t =
        decltype(std::declval<const _Node&>().next_sibling);

    template<typename _Node>
    using nest_prev_sibling_t =
        decltype(std::declval<const _Node&>().prev_sibling);

    template<typename _Node>
    using nest_children_t = decltype(std::declval<const _Node&>().children);

    template<typename _Node>
    using nest_count_t = decltype(std::declval<const _Node&>().count);

NS_END  // internal

// D_NEST_DEFINE_HAS_MEMBER
//   macro: generate a `has_<name>` trait over the matching detector alias.
// The six traits below differ only in which member they look for, so the
// detection idiom is spelled once rather than six times.
#define D_NEST_DEFINE_HAS_MEMBER(trait_name, detector_name)                    \
    template<typename _Node,                                                   \
             typename _Enable = void>                                          \
    struct trait_name : std::false_type                                        \
    {};                                                                        \
                                                                               \
    template<typename _Node>                                                   \
    struct trait_name<_Node,                                                   \
                      kv_void_t<internal::detector_name<_Node>>>               \
        : std::true_type                                                       \
    {}

// has_nest_parent
//   trait: whether the node keeps an upward link named `parent`.
D_NEST_DEFINE_HAS_MEMBER(has_nest_parent, nest_parent_t);

// has_nest_first_child
//   trait: whether the node keeps a `first_child` link.
D_NEST_DEFINE_HAS_MEMBER(has_nest_first_child, nest_first_child_t);

// has_nest_next_sibling
//   trait: whether the node keeps a `next_sibling` link.
D_NEST_DEFINE_HAS_MEMBER(has_nest_next_sibling, nest_next_sibling_t);

// has_nest_prev_sibling
//   trait: whether the sibling chain is doubly linked.
D_NEST_DEFINE_HAS_MEMBER(has_nest_prev_sibling, nest_prev_sibling_t);

// has_nest_children
//   trait: whether the node keeps a `children` array member.
D_NEST_DEFINE_HAS_MEMBER(has_nest_children, nest_children_t);

// has_nest_count
//   trait: whether the node keeps a `count` beside its child array.
D_NEST_DEFINE_HAS_MEMBER(has_nest_count, nest_count_t);


// II.    shape traits

// is_child_sibling_node
//   trait: whether the node reaches its children through a first-child link
// and a sibling chain -- the classic n-ary shape, arbitrary arity at two links
// a node.
template<typename _Node>
struct is_child_sibling_node
{
    static D_CONSTEXPR bool value =
        ( has_nest_first_child<_Node>::value &&
          has_nest_next_sibling<_Node>::value );
};

// is_child_array_node
//   trait: whether the node reaches its children through a contiguous array
// and a count -- `d_file_tree_node`'s shape.
template<typename _Node>
struct is_child_array_node
{
    static D_CONSTEXPR bool value =
        ( has_nest_children<_Node>::value &&
          has_nest_count<_Node>::value );
};

// is_nest_node
//   trait: whether the node has a shape this module can walk at all.
template<typename _Node>
struct is_nest_node
{
    static D_CONSTEXPR bool value =
        ( is_child_sibling_node<_Node>::value ||
          is_child_array_node<_Node>::value );
};

// child_sibling_shape
//   struct: the sibling shape stated outright, for a node whose members are
// named something other than the convention. Detection is a default, never the
// only way to say it.
template<typename _Node,
         typename _FirstChild,
         typename _NextSibling,
         typename _Parent = void>
struct child_sibling_shape
{
    using node_type         = _Node;
    using first_child_field = _FirstChild;
    using next_sibling_field = _NextSibling;
    using parent_field      = _Parent;

    static D_CONSTEXPR bool is_array   = false;
    static D_CONSTEXPR bool has_parent =
        (!std::is_same<_Parent, void>::value);
};

// child_array_shape
//   struct: the array shape stated outright.
template<typename _Node,
         typename _Children,
         typename _Count,
         typename _Parent = void>
struct child_array_shape
{
    using node_type     = _Node;
    using children_field = _Children;
    using count_field   = _Count;
    using parent_field  = _Parent;

    static D_CONSTEXPR bool is_array   = true;
    static D_CONSTEXPR bool has_parent =
        (!std::is_same<_Parent, void>::value);
};

// node_traits
//   trait: everything deducible about a node type's shape from the node type
// alone. The four booleans are what the walkers below dispatch on.
template<typename _Node>
struct node_traits
{
    using node_type = _Node;

    static D_CONSTEXPR bool is_sibling_shape =
        is_child_sibling_node<_Node>::value;
    static D_CONSTEXPR bool is_array_shape   =
        is_child_array_node<_Node>::value;
    static D_CONSTEXPR bool has_parent       =
        has_nest_parent<_Node>::value;
    static D_CONSTEXPR bool is_doubly_linked =
        has_nest_prev_sibling<_Node>::value;
    static D_CONSTEXPR bool is_walkable      =
        is_nest_node<_Node>::value;
    static D_CONSTEXPR bool is_addressable   =
        is_kv_addressable<_Node>::value;
};


// III.   navigation
//   Tag dispatch on the deduced shape. The overloads are separate functions
// rather than one branching function, so a sibling-shaped node never emits the
// array path and the compiler folds each call to a member access.

NS_INTERNAL

    // sibling_shape_tag / array_shape_tag
    //   type: the dispatch tags the shape traits resolve to.
    struct sibling_shape_tag
    {};

    struct array_shape_tag
    {};

    // shape_tag_of
    //   trait: the tag for a node type, preferring the sibling shape when a
    // node somehow satisfies both.
    template<typename _Node,
             bool _IsSibling = is_child_sibling_node<_Node>::value>
    struct shape_tag_of
    {
        using type = sibling_shape_tag;
    };

    template<typename _Node>
    struct shape_tag_of<_Node, false>
    {
        using type = array_shape_tag;
    };

NS_END  // internal

// nest_parent
//   function: the node's parent. Enabled only for a node type that keeps one,
// so asking a parentless node is a compile error rather than a null.
template<typename _Node>
D_INLINE
typename std::enable_if<has_nest_parent<_Node>::value, const _Node*>::type
nest_parent(
    const _Node* _node
) D_NOEXCEPT
{
    // a null node has no parent
    if (!_node)
    {
        return NULL;
    }

    return _node->parent;
}

// nest_first_child
//   function: the node's first child, in the sibling shape.
template<typename _Node>
D_INLINE
typename std::enable_if<is_child_sibling_node<_Node>::value,
                        const _Node*>::type
nest_first_child(
    const _Node* _node
) D_NOEXCEPT
{
    // a null node has no children
    if (!_node)
    {
        return NULL;
    }

    return _node->first_child;
}

// nest_next_sibling
//   function: the next node in the sibling chain.
template<typename _Node>
D_INLINE
typename std::enable_if<is_child_sibling_node<_Node>::value,
                        const _Node*>::type
nest_next_sibling(
    const _Node* _node
) D_NOEXCEPT
{
    // a null node has no sibling
    if (!_node)
    {
        return NULL;
    }

    return _node->next_sibling;
}


// IV.    child access

NS_INTERNAL

    // nest_child_count_helper
    //   function: the sibling-shape count, walked and bounded.
    template<typename _Node>
    D_INLINE std::size_t
    nest_child_count_helper(
        const _Node* _node,
        std::size_t  _limit,
        sibling_shape_tag
    ) D_NOEXCEPT
    {
        const _Node* child;
        std::size_t  count;

        count = 0u;
        child = nest_first_child(_node);

        // walk the chain, stopping at the budget when there is one
        while (child)
        {
            ++count;

            if ( (_limit != 0u) &&
                 (count >= _limit) )
            {
                break;
            }

            child = nest_next_sibling(child);
        }

        return count;
    }

    // nest_child_count_helper
    //   function: the array-shape count, read from the member beside it.
    template<typename _Node>
    D_INLINE std::size_t
    nest_child_count_helper(
        const _Node* _node,
        std::size_t  _limit,
        array_shape_tag
    ) D_NOEXCEPT
    {
        (void)_limit;

        // a null node has no children
        if (!_node)
        {
            return 0u;
        }

        return static_cast<std::size_t>(_node->count);
    }

    // nest_child_at_helper
    //   function: the sibling-shape subscript, walked and bounded.
    template<typename _Node>
    D_INLINE const _Node*
    nest_child_at_helper(
        const _Node* _node,
        std::size_t  _index,
        std::size_t  _limit,
        sibling_shape_tag
    ) D_NOEXCEPT
    {
        const _Node* child;
        std::size_t  step;

        step  = 0u;
        child = nest_first_child(_node);

        // walk to the index, stopping at the budget
        while (child)
        {
            if (step == _index)
            {
                return child;
            }

            ++step;

            if ( (_limit != 0u) &&
                 (step >= _limit) )
            {
                break;
            }

            child = nest_next_sibling(child);
        }

        return NULL;
    }

    // nest_child_at_helper
    //   function: the array-shape subscript.
    template<typename _Node>
    D_INLINE const _Node*
    nest_child_at_helper(
        const _Node* _node,
        std::size_t  _index,
        std::size_t  _limit,
        array_shape_tag
    ) D_NOEXCEPT
    {
        (void)_limit;

        // a null node or an index past the count names no child
        if ( (!_node) ||
             (_index >= static_cast<std::size_t>(_node->count)) )
        {
            return NULL;
        }

        return _node->children[_index];
    }

NS_END  // internal

// nest_child_count
//   function: how many children the node has, in either shape. `_limit` bounds
// a chain walk and is ignored by the array shape, which reads its count.
template<typename _Node>
D_INLINE
typename std::enable_if<is_nest_node<_Node>::value, std::size_t>::type
nest_child_count(
    const _Node* _node,
    std::size_t  _limit = 0u
) D_NOEXCEPT
{
    return internal::nest_child_count_helper(
        _node,
        _limit,
        typename internal::shape_tag_of<_Node>::type());
}

// nest_child_at
//   function: the child at an index, in either shape.
template<typename _Node>
D_INLINE
typename std::enable_if<is_nest_node<_Node>::value, const _Node*>::type
nest_child_at(
    const _Node* _node,
    std::size_t  _index,
    std::size_t  _limit = 0u
) D_NOEXCEPT
{
    return internal::nest_child_at_helper(
        _node,
        _index,
        _limit,
        typename internal::shape_tag_of<_Node>::type());
}

// nest_is_leaf
//   function: whether the node has no children.
template<typename _Node>
D_INLINE
typename std::enable_if<is_nest_node<_Node>::value, bool>::type
nest_is_leaf(
    const _Node* _node
) D_NOEXCEPT
{
    return (nest_child_at(_node, 0u) == NULL);
}


// V.     iterators

// child_iterator
//   class: walks a node's children in linear TOTAL time on either shape, by
// holding a cursor a macro could not.
//   AN INPUT ITERATOR, deliberately. A forward iterator promises multi-pass
// over a stable sequence, and over a structure whose acyclicity is unverified
// that is a promise this cannot keep. Range-for asks only for this category.
template<typename _Node>
class child_iterator
{
public:
    using iterator_category = std::input_iterator_tag;
    using value_type        = _Node;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const _Node*;
    using reference         = const _Node&;

    child_iterator() D_NOEXCEPT
        : m_parent(NULL),
          m_current(NULL),
          m_index(0u),
          m_limit(0u)
    {}

    child_iterator(
        const _Node* _parent,
        std::size_t  _limit
    ) D_NOEXCEPT
        : m_parent(_parent),
          m_current(nest_child_at(_parent, 0u, _limit)),
          m_index(0u),
          m_limit(_limit)
    {}

    reference operator*() const D_NOEXCEPT
    {
        return *m_current;
    }

    pointer operator->() const D_NOEXCEPT
    {
        return m_current;
    }

    // operator++
    //   function: advance. THE SIBLING SHAPE STEPS FROM THE CURSOR, which is
    // what makes a full walk linear rather than quadratic; the array shape
    // subscripts, which was linear either way.
    child_iterator& operator++() D_NOEXCEPT
    {
        ++m_index;

        // the budget bounds a chain that loops rather than detecting it
        if ( (m_limit != 0u) &&
             (m_index >= m_limit) )
        {
            m_current = NULL;

            return *this;
        }

        m_current = internal::nest_child_at_helper(
            m_parent,
            m_index,
            m_limit,
            typename internal::shape_tag_of<_Node>::type());

        return *this;
    }

    bool operator==(const child_iterator& _other) const D_NOEXCEPT
    {
        return (m_current == _other.m_current);
    }

    bool operator!=(const child_iterator& _other) const D_NOEXCEPT
    {
        return (m_current != _other.m_current);
    }

private:
    const _Node* m_parent;
    const _Node* m_current;
    std::size_t  m_index;
    std::size_t  m_limit;
};

// sibling_child_iterator
//   class: the sibling-shape walk that steps rather than subscripts, for the
// case where the shape is known and the chain is long.
template<typename _Node>
class sibling_child_iterator
{
public:
    using iterator_category = std::input_iterator_tag;
    using value_type        = _Node;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const _Node*;
    using reference         = const _Node&;

    sibling_child_iterator() D_NOEXCEPT
        : m_current(NULL),
          m_steps(0u),
          m_limit(0u)
    {}

    sibling_child_iterator(
        const _Node* _parent,
        std::size_t  _limit
    ) D_NOEXCEPT
        : m_current(nest_first_child(_parent)),
          m_steps(0u),
          m_limit(_limit)
    {}

    reference operator*() const D_NOEXCEPT
    {
        return *m_current;
    }

    pointer operator->() const D_NOEXCEPT
    {
        return m_current;
    }

    sibling_child_iterator& operator++() D_NOEXCEPT
    {
        ++m_steps;

        // the budget bounds a chain that loops rather than detecting it
        if ( (m_limit != 0u) &&
             (m_steps >= m_limit) )
        {
            m_current = NULL;

            return *this;
        }

        m_current = nest_next_sibling(m_current);

        return *this;
    }

    bool operator!=(const sibling_child_iterator& _other) const D_NOEXCEPT
    {
        return (m_current != _other.m_current);
    }

private:
    const _Node* m_current;
    std::size_t  m_steps;
    std::size_t  m_limit;
};

// ancestor_iterator
//   class: walks from a node's parent to its root.
template<typename _Node>
class ancestor_iterator
{
public:
    using iterator_category = std::input_iterator_tag;
    using value_type        = _Node;
    using difference_type   = std::ptrdiff_t;
    using pointer           = const _Node*;
    using reference         = const _Node&;

    ancestor_iterator() D_NOEXCEPT
        : m_current(NULL),
          m_steps(0u),
          m_limit(0u)
    {}

    ancestor_iterator(
        const _Node* _node,
        std::size_t  _limit
    ) D_NOEXCEPT
        : m_current(nest_parent(_node)),
          m_steps(0u),
          m_limit(_limit)
    {}

    reference operator*() const D_NOEXCEPT
    {
        return *m_current;
    }

    pointer operator->() const D_NOEXCEPT
    {
        return m_current;
    }

    ancestor_iterator& operator++() D_NOEXCEPT
    {
        ++m_steps;

        // a parent chain can loop too, and is bounded the same way
        if ( (m_limit != 0u) &&
             (m_steps >= m_limit) )
        {
            m_current = NULL;

            return *this;
        }

        m_current = nest_parent(m_current);

        return *this;
    }

    bool operator!=(const ancestor_iterator& _other) const D_NOEXCEPT
    {
        return (m_current != _other.m_current);
    }

private:
    const _Node* m_current;
    std::size_t  m_steps;
    std::size_t  m_limit;
};

// child_range
//   class: a range over a node's children, so range-for serves without a
// macro. Shape-agnostic: the iterator picks the right walk.
template<typename _Node>
class child_range
{
public:
    using iterator = child_iterator<_Node>;

    child_range(
        const _Node* _node,
        std::size_t  _limit
    ) D_NOEXCEPT
        : m_node(_node),
          m_limit(_limit)
    {}

    iterator begin() const D_NOEXCEPT
    {
        return iterator(m_node, m_limit);
    }

    iterator end() const D_NOEXCEPT
    {
        return iterator();
    }

private:
    const _Node* m_node;
    std::size_t  m_limit;
};

// sibling_range
//   class: a range that steps the chain, for the sibling shape only. Linear
// total rather than quadratic, at the cost of not working on an array node --
// which the enable_if on its element type makes a compile error rather than an
// empty loop.
template<typename _Node>
class sibling_range
{
public:
    using iterator = sibling_child_iterator<_Node>;

    static_assert(is_child_sibling_node<_Node>::value,
                  "nest: a sibling_range needs a first_child/next_sibling "
                  "node; an array node has no chain to step.");

    sibling_range(
        const _Node* _node,
        std::size_t  _limit
    ) D_NOEXCEPT
        : m_node(_node),
          m_limit(_limit)
    {}

    iterator begin() const D_NOEXCEPT
    {
        return iterator(m_node, m_limit);
    }

    iterator end() const D_NOEXCEPT
    {
        return iterator();
    }

private:
    const _Node* m_node;
    std::size_t  m_limit;
};

// ancestor_range
//   class: a range from a node's parent to its root.
template<typename _Node>
class ancestor_range
{
public:
    using iterator = ancestor_iterator<_Node>;

    ancestor_range(
        const _Node* _node,
        std::size_t  _limit
    ) D_NOEXCEPT
        : m_node(_node),
          m_limit(_limit)
    {}

    iterator begin() const D_NOEXCEPT
    {
        return iterator(m_node, m_limit);
    }

    iterator end() const D_NOEXCEPT
    {
        return iterator();
    }

private:
    const _Node* m_node;
    std::size_t  m_limit;
};

// children
//   function: the child range of a node.
template<typename _Node>
D_INLINE
typename std::enable_if<is_nest_node<_Node>::value,
                        child_range<_Node>>::type
children(
    const _Node* _node,
    std::size_t  _limit = 0u
) D_NOEXCEPT
{
    return child_range<_Node>(_node, _limit);
}

// siblings
//   function: the stepping child range of a sibling-shaped node.
template<typename _Node>
D_INLINE
typename std::enable_if<is_child_sibling_node<_Node>::value,
                        sibling_range<_Node>>::type
siblings(
    const _Node* _node,
    std::size_t  _limit = 0u
) D_NOEXCEPT
{
    return sibling_range<_Node>(_node, _limit);
}

// ancestors
//   function: the ancestor range of a node.
template<typename _Node>
D_INLINE
typename std::enable_if<has_nest_parent<_Node>::value,
                        ancestor_range<_Node>>::type
ancestors(
    const _Node* _node,
    std::size_t  _limit = 0u
) D_NOEXCEPT
{
    return ancestor_range<_Node>(_node, _limit);
}


// VI.    the runtime descriptor and view

// nest_desc
//   class: the runtime layout. PRIVATELY INHERITS THE C STRUCT AND ADDS NO
// DATA MEMBER, so its layout is the C struct's.
class nest_desc : private ::d_nest_desc
{
public:
    nest_desc() D_NOEXCEPT
        : ::d_nest_desc()
    {}

    explicit nest_desc(
        const ::d_nest_desc& _desc
    ) D_NOEXCEPT
        : ::d_nest_desc(_desc)
    {}

    std::uint32_t node_size() const D_NOEXCEPT
    {
        return ::d_nest_desc::node_size;
    }

    const ::d_nest_desc& c_ref() const D_NOEXCEPT
    {
        return *this;
    }
};

// nest_view
//   class: a descriptor bound to an arena, for walking bytes whose node type
// is not visible where the walk happens -- a mapped file, a node reached
// through a `void*`.
class nest_view : private ::d_nest_view
{
public:
    nest_view() D_NOEXCEPT
        : ::d_nest_view()
    {}

    nest_view(
        const nest_desc& _desc,
        const void*      _arena       = NULL,
        std::size_t      _arena_count = 0u,
        std::uint32_t    _arena_stride = 0u,
        std::size_t      _walk_limit  = 0u
    ) D_NOEXCEPT
    {
        static_cast< ::d_nest_view&>(*this) =
            ::d_nest_view_make(&_desc.c_ref(),
                               _arena,
                               _arena_count,
                               _arena_stride,
                               _walk_limit);
    }

    bool valid() const D_NOEXCEPT
    {
        return ::d_nest_view_is_valid(this);
    }

    const void* parent(const void* _node) const D_NOEXCEPT
    {
        return ::d_nest_parent(this, _node);
    }

    const void* first_child(const void* _node) const D_NOEXCEPT
    {
        return ::d_nest_first_child(this, _node);
    }

    const void* next_sibling(const void* _node) const D_NOEXCEPT
    {
        return ::d_nest_next_sibling(this, _node);
    }

    const void* payload(const void* _node) const D_NOEXCEPT
    {
        return ::d_nest_payload(this, _node);
    }

    std::size_t child_count(const void* _node) const D_NOEXCEPT
    {
        return ::d_nest_child_count(this, _node);
    }

    const void* child_at(
        const void* _node,
        std::size_t _index
    ) const D_NOEXCEPT
    {
        return ::d_nest_child_at(this, _node, _index);
    }

    const ::d_nest_view& c_ref() const D_NOEXCEPT
    {
        return *this;
    }
};


// VII.   lowering

// D_NEST_DECLARE_CHILD_SIBLING
//   macro: DECLARE a nest_desc for a sibling-shaped node, plus the C
// descriptor it wraps.
//   A MACRO because every offset in it comes from offsetof, which needs member
// names and cannot be written by a template -- the same limit kv.hpp's
// D_KV_MEMBER_FIELD works around, here for five members at once.
//   A DECLARATION rather than an expression, for two reasons. A descriptor is
// pure layout and wants to be one static const per node type rather than one
// per call site. And the expression form would need a compound literal, which
// is C and not C++ -- it compiles under most compilers and is a -Wpedantic
// diagnostic in a header meant to be clean under it.
#define D_NEST_DECLARE_CHILD_SIBLING(name, node, payload_member,               \
                                     parent_member, first_child_member,        \
                                     next_sibling_member, kind, node_flags)    \
    static const ::d_nest_desc name##_c_desc =                                 \
        D_NEST_DESC_CHILD_SIBLING(node,                                        \
                                  payload_member,                              \
                                  0,                                           \
                                  parent_member,                               \
                                  first_child_member,                          \
                                  next_sibling_member,                         \
                                  kind,                                        \
                                  node_flags);                                 \
    static const ::djinterp::nest_desc name(name##_c_desc)

// D_NEST_DECLARE_CHILD_ARRAY
//   macro: declare a nest_desc for an array-shaped node.
#define D_NEST_DECLARE_CHILD_ARRAY(name, node, payload_member,                 \
                                   parent_member, array_member,                \
                                   count_member, slot_type, kind,              \
                                   node_flags)                                 \
    static const ::d_nest_desc name##_c_desc =                                 \
        D_NEST_DESC_CHILD_ARRAY(node,                                          \
                                payload_member,                                \
                                0,                                             \
                                parent_member,                                 \
                                array_member,                                  \
                                count_member,                                  \
                                slot_type,                                     \
                                kind,                                          \
                                node_flags);                                   \
    static const ::djinterp::nest_desc name(name##_c_desc)


// VIII.  layout assertions

static_assert((sizeof(nest_desc) == sizeof(::d_nest_desc)),
              "nest_desc must add no storage to d_nest_desc.");
static_assert((sizeof(nest_view) == sizeof(::d_nest_view)),
              "nest_view must add no storage to d_nest_view.");
static_assert(std::is_standard_layout<nest_desc>::value,
              "nest_desc must be standard-layout for the base to sit at "
              "offset zero.");
static_assert(std::is_standard_layout<nest_view>::value,
              "nest_view must be standard-layout for the base to sit at "
              "offset zero.");

//   THE SLOT NUMBERS ARE SHARED. A C++ caller reaching a slot by name and a C
// caller reaching it by number must arrive at the same link.
static_assert((D_NEST_SLOT_COUNT == 5u),
              "nest: the slot set has changed and the C++ face has not.");


NS_END  // djinterp


#endif  // DJINTERP_UTIL_NEST_
