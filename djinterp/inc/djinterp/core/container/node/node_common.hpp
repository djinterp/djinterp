/******************************************************************************
* djinterp [container]                                         node_common.hpp
*
* Common node definitions and node structural traits:
*   This header is the single home for the framework's concrete node types
* AND the compile-time structural traits that inspect them.  It is the union
* of two former headers:
*
*     node.hpp        -> the node TYPES  (leaf_node, dynamic_node, tuple_node)
*     node_traits.hpp -> the node TRAITS (has_next, is_dynamic_node, ...)
*
*   Keeping the vertex/node types next to the traits that classify them makes
* this the natural place to express the monograph's node-level vocabulary --
* nodes as a vertex paired with an edge collection, and the classification of
* vertices by the arity of that collection (edgeless / fixed / n-ary /
* hybrid).  The container-level counterparts (ownership, entry points, graph
* shape) live in node_container_traits.hpp.
*
* PART A -- NODE TYPES
*   leaf_node<T>                     terminal, zero-link vertex (payload only)
*   dynamic_node<T, Alloc, Cont>     n-ary vertex, edges in a dynamic container
*   tuple_node<T, Edges...>          heterogeneous fixed vertex, edges in a tuple
*
* PART B -- NODE STRUCTURAL TRAITS (merged from node_traits.hpp)
*   Purely structural, SFINAE-based detection.  Every trait takes the NODE
* type as its primary parameter (never the container), and answers a
* concrete, falsifiable question about a node's interface.  No taxonomy or
* tag type is imposed on the node; policy is left to the consumer.
*
*     member/method/unified field access ....... has_next, has_left, ...
*     node handle form ......................... node_is_raw_pointer, ...
*     composite structural queries ............. is_binary_node, is_nary_node, ...
*     polymorphism / self-reference ............ node_is_polymorphic, node_is_self
*     topology detection ....................... has_edges, is_dynamic_node, ...
*     link utilities ........................... is_null_link, node_link_count
*
* PART C -- VERTEX CLASSIFICATION (monograph "Vertices")
*   DVertexArity + vertex_arity_of              edgeless / fixed / n-ary / hybrid
*   payload classification                      is_payloaded_vertex, ...
*
* NAMING CONVENTIONS:
*   djinterp::has_next<my_node>::value
*   djinterp::is_doubly_linked_v<my_node>
*   djinterp::vertex_arity_of_v<my_node>
*
*
* path:      /inc/djinterp/core/container/node/node_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.03.30
******************************************************************************/

/*
TABLE OF CONTENTS
=================
PART A -- NODE TYPES
  I.     leaf_node
  II.    dynamic_node
  III.   tuple_node
PART B -- NODE STRUCTURAL TRAITS
  IV.    Node Member Detection (field access)
  V.     Node Method Detection (callable form)
  VI.    Unified Access Detection (field OR method)
  VII.   Node Handle Form Detection
  VIII.  Composite Structural Queries
  IX.    Polymorphism Detection
  X.     Self-Referential Node Detection
  XI.    Topology Detection
  XII.   Link Utilities
PART C -- VERTEX CLASSIFICATION
  XIII.  Vertex Arity Regimes and Payload
*/

#ifndef DJINTERP_CONTAINER_NODE_COMMON_
#define DJINTERP_CONTAINER_NODE_COMMON_ 1

// std
#include <cstddef>
#include <cstdint>
#include <memory>
#include <tuple>
#include <type_traits>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../traits/container_traits.hpp"

// node_common requires C++11 or higher for decltype, SFINAE, and variadic
// templates.  Checked here, right after the core header supplies the feature
// macros, so the failure message is clear before the heavier trait includes.
#if !D_ENV_LANG_IS_CPP11_OR_HIGHER
    #error "node_common.hpp requires C++11 or higher"
#endif


NS_DJINTERP

// ###########################################################################
// #  PART A -- NODE TYPES
// ###########################################################################
//   A node is a VERTEX (the site carrying the payload) paired with an EDGE
// COLLECTION (its links to other nodes).  The three concrete types below
// realise the three principal arity regimes: leaf_node is edgeless,
// tuple_node is a fixed heterogeneous vertex, and dynamic_node is n-ary.

// ===========================================================================
// I.   leaf_node
// ===========================================================================

// leaf_node
//   class: Terminal/Zero-link node. Contains purely data.
//   Highly cache-efficient for variant-based tree leaves.
template<typename _Type>
class leaf_node
{
public:
    using self_type  = leaf_node;
    using value_type = _Type;

    static D_CONSTEXPR std::size_t num_links = 0;

    D_CONSTEXPR
    leaf_node()
        : m_data{}
    {}

    D_CONSTEXPR explicit
    leaf_node(const value_type& _val)
        : m_data(_val)
    {}

    D_CONSTEXPR explicit
    leaf_node(value_type&& _val)
        : m_data(static_cast<value_type&&>(_val))
    {}

    D_CONSTEXPR value_type& data()
    {
        return m_data;
    }

    D_CONSTEXPR const value_type& data() const
    {
        return m_data;
    }

private:
    value_type m_data;
};


// ===========================================================================
// II.  dynamic_node
// ===========================================================================

// dynamic_node
//   class: Dynamic Homogeneous node. Edges are stored in a standard
//   container. Supports std::allocator_arg_t for stateful arena injection.
template<typename _Type,
         typename _NodeAllocator = std::allocator<_Type>,
         template<typename, typename> class _ContainerType = std::vector>
class dynamic_node
{
public:
    using self_type      = dynamic_node;
    using value_type     = _Type;
    using allocator_type = _NodeAllocator;

    // Resolve self* to the actual type
    using link_type = resolve_self_t<self*, self_type>;

    // Rebind the allocator for the links
    using edge_allocator = typename std::allocator_traits<
        allocator_type>::template rebind_alloc<link_type>;

    // The final dynamically-sized edge container
    using container_type = _ContainerType<link_type, edge_allocator>;

    // -----------------------------------------------------------------
    // constructors (stateless / std::allocator)
    // -----------------------------------------------------------------

    D_CONSTEXPR
    dynamic_node()
        : m_data{},
          m_edges{}
    {}

    D_CONSTEXPR explicit
    dynamic_node(const value_type& _val)
        : m_data(_val),
          m_edges{}
    {}

    D_CONSTEXPR explicit
    dynamic_node(value_type&& _val)
        : m_data(static_cast<value_type&&>(_val)),
          m_edges{}
    {}

    // -----------------------------------------------------------------
    // allocator-extended constructors (stateful / arenas)
    // -----------------------------------------------------------------

    D_CONSTEXPR
    dynamic_node(std::allocator_arg_t,
                 const allocator_type& _alloc)
        : m_data{},
          m_edges(_alloc)
    {}

    D_CONSTEXPR
    dynamic_node(std::allocator_arg_t,
                 const allocator_type& _alloc,
                 const value_type&     _val)
        : m_data(_val),
          m_edges(_alloc)
    {}

    // -----------------------------------------------------------------
    // accessors
    // -----------------------------------------------------------------

    D_CONSTEXPR value_type& data()
    {
        return m_data;
    }

    D_CONSTEXPR const value_type& data() const
    {
        return m_data;
    }

    D_CONSTEXPR container_type& edges()
    {
        return m_edges;
    }

    D_CONSTEXPR const container_type& edges() const
    {
        return m_edges;
    }

    D_CONSTEXPR std::size_t edge_count() const
    {
        return m_edges.size();
    }

    D_CONSTEXPR bool is_leaf() const
    {
        return m_edges.empty();
    }

private:
    value_type     m_data;
    container_type m_edges;
};


// ===========================================================================
// III. tuple_node
// ===========================================================================

// tuple_node
//   class: Static Heterogeneous node. Edges are distinct types evaluated
//   at compile time. Applies resolve_self to every edge in the pack.
template<typename    _Type,
         typename... _Edges>
class tuple_node
{
public:
    using self_type  = tuple_node;
    using value_type = _Type;

    // Apply resolve_self recursively across the parameter pack
    using edge_tuple_type = std::tuple<resolve_self_t<_Edges, self_type>...>;

    static D_CONSTEXPR std::size_t edge_groups = sizeof...(_Edges);

    D_CONSTEXPR
    tuple_node()
        : m_data{},
          m_edges{}
    {}

    D_CONSTEXPR explicit
    tuple_node(const value_type& _val)
        : m_data(_val),
          m_edges{}
    {}

    D_CONSTEXPR explicit
    tuple_node(value_type&& _val)
        : m_data(static_cast<value_type&&>(_val)),
          m_edges{}
    {}

    D_CONSTEXPR value_type& data()
    {
        return m_data;
    }

    D_CONSTEXPR const value_type& data() const
    {
        return m_data;
    }

    template<std::size_t _Index>
    D_CONSTEXPR auto& get_edge_group()
    {
        static_assert(_Index < edge_groups,
                      "Edge index out of bounds.");
        return std::get<_Index>(m_edges);
    }

    template<std::size_t _Index>
    D_CONSTEXPR const auto& get_edge_group() const
    {
        static_assert(_Index < edge_groups,
                      "Edge index out of bounds.");
        return std::get<_Index>(m_edges);
    }

private:
    value_type      m_data;
    edge_tuple_type m_edges;
};


// ###########################################################################
// #  PART B -- NODE STRUCTURAL TRAITS  (merged from node_traits.hpp)
// ###########################################################################


// ===========================================================================
// IV.  NODE MEMBER DETECTION (field access)
// ===========================================================================
// Each trait probes for the existence of a specific data member on the
// node type _N. Detection is purely syntactic; no semantic verification
// is performed. Every trait evaluates to std::true_type or std::false_type.

// has_next
//   trait: evaluates to true if _N has a `next` data member.
template<typename _N,
         typename = void>
struct has_next : std::false_type
{};

template<typename _N>
struct has_next<_N, void_t<decltype(std::declval<_N>().next)>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_next_v
    //   variable template: value of has_next<_N>.
    template<typename _N>
    D_CONSTEXPR bool has_next_v = has_next<_N>::value;
#endif

// has_prev
//   trait: evaluates to true if _N has a `prev` data member.
template<typename _N,
         typename = void>
struct has_prev : std::false_type
{};

template<typename _N>
struct has_prev<_N,
    void_t<decltype(std::declval<_N>().prev)>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_prev_v
    //   variable template: value of has_prev<_N>.
    template<typename _N>
    D_CONSTEXPR bool has_prev_v = has_prev<_N>::value;
#endif

// has_parent
//   trait: evaluates to true if _N has a `parent` data member.
template<typename _N,
         typename = void>
struct has_parent : std::false_type
{};

template<typename _N>
struct has_parent<_N,
    void_t<decltype(std::declval<_N>().parent)>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_parent_v
    //   variable template: value of has_parent<_N>.
    template<typename _N>
    D_CONSTEXPR bool has_parent_v = has_parent<_N>::value;
#endif

// has_left
//   trait: evaluates to true if _N has a `left` data member.
template<typename _N,
         typename = void>
struct has_left : std::false_type
{};

template<typename _N>
struct has_left<_N,
    void_t<decltype(std::declval<_N>().left)>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_left_v
    //   variable template: value of has_left<_N>.
    template<typename _N>
    D_CONSTEXPR bool has_left_v = has_left<_N>::value;
#endif

// has_right
//   trait: evaluates to true if _N has a `right` data member.
template<typename _N,
         typename = void>
struct has_right : std::false_type
{};

template<typename _N>
struct has_right<_N,
    void_t<decltype(std::declval<_N>().right)>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_right_v
    //   variable template: value of has_right<_N>.
    template<typename _N>
    D_CONSTEXPR bool has_right_v = has_right<_N>::value;
#endif

// has_data
//   trait: evaluates to true if _N has a `data` data member.
template<typename _N,
         typename = void>
struct has_data : std::false_type
{};

template<typename _N>
struct has_data<_N,
    void_t<decltype(std::declval<_N>().data)>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_data_v
    //   variable template: value of has_data<_N>.
    template<typename _N>
    D_CONSTEXPR bool has_data_v = has_data<_N>::value;
#endif

// has_children_type
//   trait: evaluates to true if _N has a nested `children_type`
// type alias.
template<typename _N,
         typename = void>
struct has_children_type : std::false_type
{};

template<typename _N>
struct has_children_type<_N,
    void_t<typename clean_t<_N>::children_type>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_children_type_v
    //   variable template: value of has_children_type<_N>.
    template<typename _N>
    D_CONSTEXPR bool has_children_type_v =
        has_children_type<_N>::value;
#endif

// has_adjacency_type
//   trait: evaluates to true if _N has a nested `adjacency_type`
// type alias.
template<typename _N,
         typename = void>
struct has_adjacency_type : std::false_type
{};

template<typename _N>
struct has_adjacency_type<_N,
    void_t<typename clean_t<_N>::adjacency_type>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_adjacency_type_v
    //   variable template: value of has_adjacency_type<_N>.
    template<typename _N>
    D_CONSTEXPR bool has_adjacency_type_v =
        has_adjacency_type<_N>::value;
#endif


// ===========================================================================
// V.   NODE METHOD DETECTION (callable form)
// ===========================================================================
// Detects member functions by attempting a call expression. This is
// necessary for node types whose API is method-based (e.g. linked_node
// derivatives with overloaded const/non-const accessors).

// has_left_method
//   trait: evaluates to true if _N has a callable `left()` method.
template<typename _N,
         typename = void>
struct has_left_method : std::false_type
{};

template<typename _N>
struct has_left_method<_N,
    void_t<decltype(std::declval<_N>().left())>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_left_method_v
    //   variable template: value of has_left_method<_N>.
    template<typename _N>
    D_CONSTEXPR bool has_left_method_v = has_left_method<_N>::value;
#endif

// has_right_method
//   trait: evaluates to true if _N has a callable `right()` method.
template<typename _N,
         typename = void>
struct has_right_method : std::false_type
{};

template<typename _N>
struct has_right_method<_N,
    void_t<decltype(std::declval<_N>().right())>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_right_method_v
    //   variable template: value of has_right_method<_N>.
    template<typename _N>
    D_CONSTEXPR bool has_right_method_v = has_right_method<_N>::value;
#endif

// has_parent_method
//   trait: evaluates to true if _N has a callable `parent()` method.
template<typename _N,
         typename = void>
struct has_parent_method : std::false_type
{};

template<typename _N>
struct has_parent_method<_N,
    void_t<decltype(std::declval<_N>().parent())>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_parent_method_v
    //   variable template: value of has_parent_method<_N>.
    template<typename _N>
    D_CONSTEXPR bool has_parent_method_v = has_parent_method<_N>::value;
#endif

// has_next_method
//   trait: evaluates to true if _N has a callable `next()` method.
template<typename _N,
         typename = void>
struct has_next_method : std::false_type
{};

template<typename _N>
struct has_next_method<_N,
    void_t<decltype(std::declval<_N>().next())>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_next_method_v
    //   variable template: value of has_next_method<_N>.
    template<typename _N>
    D_CONSTEXPR bool has_next_method_v = has_next_method<_N>::value;
#endif

// has_prev_method
//   trait: evaluates to true if _N has a callable `prev()` method.
template<typename _N,
         typename = void>
struct has_prev_method : std::false_type
{};

template<typename _N>
struct has_prev_method<_N,
    void_t<decltype(std::declval<_N>().prev())>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_prev_method_v
    //   variable template: value of has_prev_method<_N>.
    template<typename _N>
    D_CONSTEXPR bool has_prev_method_v = has_prev_method<_N>::value;
#endif

// has_data_method
//   trait: evaluates to true if _N has a callable `data()` method.
//   NOTE: container_traits.hpp defines a `has_data_method` over the
// CONTAINER surface (const-qualified probe).  That trait is included via
// the chain above and lives in the same djinterp namespace; the node
// layer reuses it rather than redefining it, so no node-local duplicate
// is declared here.


// ===========================================================================
// VI.  UNIFIED ACCESS DETECTION (field OR method)
// ===========================================================================
// These accept either form — data members or methods — so composite
// traits work uniformly across POD-style nodes and method-based nodes
// (e.g. linked_node derivatives).

// has_data_access
//   trait: evaluates to true if _N has either a `data` member or
// a callable `data()` method.
template<typename _N>
struct has_data_access
{
    static constexpr bool value =
        ( has_data<_N>::value ||
          has_data_method<_N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_data_access_v
    //   variable template: value of has_data_access<_N>.
    template<typename _N>
    D_CONSTEXPR bool has_data_access_v = has_data_access<_N>::value;
#endif

// has_left_access
//   trait: evaluates to true if _N has either a `left` member or
// a callable `left()` method.
template<typename _N>
struct has_left_access
{
    static constexpr bool value =
        ( has_left<_N>::value ||
          has_left_method<_N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_left_access_v
    //   variable template: value of has_left_access<_N>.
    template<typename _N>
    D_CONSTEXPR bool has_left_access_v = has_left_access<_N>::value;
#endif

// has_right_access
//   trait: evaluates to true if _N has either a `right` member or
// a callable `right()` method.
template<typename _N>
struct has_right_access
{
    static constexpr bool value =
        ( has_right<_N>::value ||
          has_right_method<_N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_right_access_v
    //   variable template: value of has_right_access<_N>.
    template<typename _N>
    D_CONSTEXPR bool has_right_access_v = has_right_access<_N>::value;
#endif

// has_parent_access
//   trait: evaluates to true if _N has either a `parent` member or
// a callable `parent()` method.
template<typename _N>
struct has_parent_access
{
    static constexpr bool value =
        ( has_parent<_N>::value ||
          has_parent_method<_N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_parent_access_v
    //   variable template: value of has_parent_access<_N>.
    template<typename _N>
    D_CONSTEXPR bool has_parent_access_v = has_parent_access<_N>::value;
#endif

// has_next_access
//   trait: evaluates to true if _N has either a `next` member or
// a callable `next()` method.
template<typename _N>
struct has_next_access
{
    static constexpr bool value =
        ( has_next<_N>::value ||
          has_next_method<_N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_next_access_v
    //   variable template: value of has_next_access<_N>.
    template<typename _N>
    D_CONSTEXPR bool has_next_access_v = has_next_access<_N>::value;
#endif

// has_prev_access
//   trait: evaluates to true if _N has either a `prev` member or
// a callable `prev()` method.
template<typename _N>
struct has_prev_access
{
    static constexpr bool value =
        ( has_prev<_N>::value ||
          has_prev_method<_N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_prev_access_v
    //   variable template: value of has_prev_access<_N>.
    template<typename _N>
    D_CONSTEXPR bool has_prev_access_v = has_prev_access<_N>::value;
#endif


// ===========================================================================
// VII. NODE HANDLE FORM DETECTION
// ===========================================================================
// Classifies the structural form of a type _H used as a node
// handle — the thing a container or another node holds to refer
// to a node. This is the type of the `next`, `left`, `parent`,
// etc. members, or the container's own `node_type` alias.
//
// The handle may be a raw pointer, smart pointer, integral index,
// or a user-defined handle class (e.g. a slot-map key).  It is the
// realisation of Ref(N) -- the reference type through which the node
// recursion runs (monograph "Nodes").

NS_INTERNAL

    // is_unique_ptr
    //   trait: detects std::unique_ptr<...> (any deleter).
    template<typename _T>
    struct is_unique_ptr : std::false_type
    {};

    template<typename _T,
             typename _D>
    struct is_unique_ptr<std::unique_ptr<_T, _D>> : std::true_type
    {};

    // is_shared_ptr
    //   trait: detects std::shared_ptr<...>.
    template<typename _T>
    struct is_shared_ptr : std::false_type
    {};

    template<typename _T>
    struct is_shared_ptr<std::shared_ptr<_T>> : std::true_type
    {};

    // is_weak_ptr
    //   trait: detects std::weak_ptr<...>.
    template<typename _T>
    struct is_weak_ptr : std::false_type
    {};

    template<typename _T>
    struct is_weak_ptr<std::weak_ptr<_T>> : std::true_type
    {};

NS_END  // internal

// node_is_raw_pointer
//   trait: evaluates to true if _H is a raw pointer type.
template<typename _H>
struct node_is_raw_pointer
    : std::is_pointer<_H>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_raw_pointer_v
    //   variable template: value of node_is_raw_pointer<_H>.
    template<typename _H>
    D_CONSTEXPR bool node_is_raw_pointer_v =
        node_is_raw_pointer<_H>::value;
#endif

// node_is_unique_pointer
//   trait: evaluates to true if _H is a std::unique_ptr
// instantiation.
template<typename _H>
struct node_is_unique_pointer
    : internal::is_unique_ptr<_H>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_unique_pointer_v
    //   variable template: value of node_is_unique_pointer<_H>.
    template<typename _H>
    D_CONSTEXPR bool node_is_unique_pointer_v =
        node_is_unique_pointer<_H>::value;
#endif

// node_is_shared_pointer
//   trait: evaluates to true if _H is a std::shared_ptr
// instantiation.
template<typename _H>
struct node_is_shared_pointer
    : internal::is_shared_ptr<_H>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_shared_pointer_v
    //   variable template: value of node_is_shared_pointer<_H>.
    template<typename _H>
    D_CONSTEXPR bool node_is_shared_pointer_v =
        node_is_shared_pointer<_H>::value;
#endif

// node_is_weak_pointer
//   trait: evaluates to true if _H is a std::weak_ptr
// instantiation.
template<typename _H>
struct node_is_weak_pointer
    : internal::is_weak_ptr<_H>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_weak_pointer_v
    //   variable template: value of node_is_weak_pointer<_H>.
    template<typename _H>
    D_CONSTEXPR bool node_is_weak_pointer_v =
        node_is_weak_pointer<_H>::value;
#endif

// node_is_smart_pointer
//   trait: evaluates to true if _H is any of unique_ptr,
// shared_ptr, or weak_ptr.
template<typename _H>
struct node_is_smart_pointer
{
    static constexpr bool value =
        ( internal::is_unique_ptr<_H>::value ||
          internal::is_shared_ptr<_H>::value ||
          internal::is_weak_ptr<_H>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_smart_pointer_v
    //   variable template: value of node_is_smart_pointer<_H>.
    template<typename _H>
    D_CONSTEXPR bool node_is_smart_pointer_v =
        node_is_smart_pointer<_H>::value;
#endif

// node_is_any_pointer
//   trait: evaluates to true if _H is a raw pointer or any
// standard smart pointer.
template<typename _H>
struct node_is_any_pointer
{
    static constexpr bool value =
        ( std::is_pointer<_H>::value ||
          node_is_smart_pointer<_H>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_any_pointer_v
    //   variable template: value of node_is_any_pointer<_H>.
    template<typename _H>
    D_CONSTEXPR bool node_is_any_pointer_v =
        node_is_any_pointer<_H>::value;
#endif

// node_is_index
//   trait: evaluates to true if _H is a non-bool integral type,
// indicating index-based node access into a backing store.
template<typename _H>
struct node_is_index
{
    static constexpr bool value =
        ( std::is_integral<_H>::value &&
          !std::is_same<_H, bool>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_index_v
    //   variable template: value of node_is_index<_H>.
    template<typename _H>
    D_CONSTEXPR bool node_is_index_v = node_is_index<_H>::value;
#endif

// node_is_handle
//   trait: evaluates to true if _H is a class type with an
// `element_type` member alias (the convention for smart-pointer-
// like handle types, e.g. slot-map keys, arena handles).
// Standard smart pointers also satisfy this, but are better
// identified by the specific traits above.
template<typename _H,
         typename = void>
struct node_is_handle : std::false_type
{};

template<typename _H>
struct node_is_handle<_H,
    void_t<typename clean_t<_H>::element_type>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_handle_v
    //   variable template: value of node_is_handle<_H>.
    template<typename _H>
    D_CONSTEXPR bool node_is_handle_v = node_is_handle<_H>::value;
#endif


// ===========================================================================
// VIII. COMPOSITE STRUCTURAL QUERIES
// ===========================================================================
// Higher-level traits combining multiple member detections to answer
// common structural questions about a node type. Uses the unified
// access traits so that both field-based (POD) and method-based
// (linked_node-derived) nodes are detected correctly.

// is_singly_linked
//   trait: evaluates to true if _N has next access but not prev.
template<typename _N>
struct is_singly_linked
{
    static constexpr bool value =
        ( has_next_access<_N>::value &&
          !has_prev_access<_N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_singly_linked_v
    //   variable template: value of is_singly_linked<_N>.
    template<typename _N>
    D_CONSTEXPR bool is_singly_linked_v = is_singly_linked<_N>::value;
#endif

// is_doubly_linked
//   trait: evaluates to true if _N has both next and prev access.
template<typename _N>
struct is_doubly_linked
{
    static constexpr bool value =
        ( has_next_access<_N>::value &&
          has_prev_access<_N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_doubly_linked_v
    //   variable template: value of is_doubly_linked<_N>.
    template<typename _N>
    D_CONSTEXPR bool is_doubly_linked_v = is_doubly_linked<_N>::value;
#endif

// is_binary_node
//   trait: evaluates to true if _N has left and right access.
// Does not require parent.
template<typename _N>
struct is_binary_node
{
    static constexpr bool value =
        ( has_left_access<_N>::value &&
          has_right_access<_N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_binary_node_v
    //   variable template: value of is_binary_node<_N>.
    template<typename _N>
    D_CONSTEXPR bool is_binary_node_v = is_binary_node<_N>::value;
#endif

// is_parented
//   trait: evaluates to true if _N has parent access.
template<typename _N>
struct is_parented
{
    static constexpr bool value = has_parent_access<_N>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_parented_v
    //   variable template: value of is_parented<_N>.
    template<typename _N>
    D_CONSTEXPR bool is_parented_v = is_parented<_N>::value;
#endif

// is_parented_binary_node
//   trait: evaluates to true if _N has parent, left, and right access.
template<typename _N>
struct is_parented_binary_node
{
    static constexpr bool value =
        ( is_binary_node<_N>::value &&
          has_parent_access<_N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_parented_binary_node_v
    //   variable template: value of is_parented_binary_node<_N>.
    template<typename _N>
    D_CONSTEXPR bool is_parented_binary_node_v =
        is_parented_binary_node<_N>::value;
#endif

// is_nary_node
//   trait: evaluates to true if _N has parent access and a
// `children_type` nested type alias.
template<typename _N>
struct is_nary_node
{
    static constexpr bool value =
        ( has_parent_access<_N>::value &&
          has_children_type<_N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_nary_node_v
    //   variable template: value of is_nary_node<_N>.
    template<typename _N>
    D_CONSTEXPR bool is_nary_node_v = is_nary_node<_N>::value;
#endif

// is_graph_node
//   trait: evaluates to true if _N has an `adjacency_type`
// nested type alias.
template<typename _N>
struct is_graph_node
{
    static constexpr bool value = has_adjacency_type<_N>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_graph_node_v
    //   variable template: value of is_graph_node<_N>.
    template<typename _N>
    D_CONSTEXPR bool is_graph_node_v = is_graph_node<_N>::value;
#endif

// is_keyed_node
//   trait: evaluates to true if _N has both a `key_type` alias
// and data access. Common in associative-container nodes
// where the key and payload are separate concerns.
template<typename _N>
struct is_keyed_node
{
    static constexpr bool value =
        ( has_key_type<_N>::value &&
          has_data_access<_N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_keyed_node_v
    //   variable template: value of is_keyed_node<_N>.
    template<typename _N>
    D_CONSTEXPR bool is_keyed_node_v = is_keyed_node<_N>::value;
#endif

// is_leaf_capable
//   trait: evaluates to true if _N carries payload (has data access
// or `value_type`) but has no child links (no left, right,
// children_type, or adjacency_type). Structural indicator
// that _N can only be a leaf node.
template<typename _N>
struct is_leaf_capable
{
    static constexpr bool value =
        ( ( has_data_access<_N>::value ||
            has_value_type<_N>::value )          &&
          !has_left_access<_N>::value            &&
          !has_right_access<_N>::value           &&
          !has_children_type<_N>::value          &&
          !has_adjacency_type<_N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_leaf_capable_v
    //   variable template: value of is_leaf_capable<_N>.
    template<typename _N>
    D_CONSTEXPR bool is_leaf_capable_v = is_leaf_capable<_N>::value;
#endif


// ===========================================================================
// IX.  POLYMORPHISM DETECTION
// ===========================================================================

// node_is_polymorphic
//   trait: evaluates to true if _N has at least one virtual
// method (i.e. std::is_polymorphic). Indicates the node
// participates in a type hierarchy where different concrete
// node types (e.g. leaf vs. interior) share a common base.
template<typename _N>
struct node_is_polymorphic
{
    static constexpr bool value = std::is_polymorphic<_N>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_polymorphic_v
    //   variable template: value of node_is_polymorphic<_N>.
    template<typename _N>
    D_CONSTEXPR bool node_is_polymorphic_v =
        node_is_polymorphic<_N>::value;
#endif

// node_derives_from
//   trait: evaluates to true if _N derives from _Base.
// Two-parameter trait for verifying a node against a known
// base class (e.g. node_base, tree_node_base).
template<typename _N,
         typename _Base>
struct node_derives_from
{
    static constexpr bool value =
        ( std::is_base_of<_Base, _N>::value &&
          !std::is_same<_Base, _N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_derives_from_v
    //   variable template: value of node_derives_from<_N, _Base>.
    template<typename _N,
             typename _Base>
    D_CONSTEXPR bool node_derives_from_v =
        node_derives_from<_N, _Base>::value;
#endif


// ===========================================================================
// X.   SELF-REFERENTIAL NODE DETECTION
// ===========================================================================
// Integration with djinterp::self / djinterp::resolve_self.

// node_is_self
//   trait: evaluates to true if _N is the djinterp::self marker
// type, indicating the node type is a placeholder that resolves
// to the owning type via resolve_self.
template<typename _N>
struct node_is_self
{
    static constexpr bool value = djinterp::is_self<_N>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_self_v
    //   variable template: value of node_is_self<_N>.
    template<typename _N>
    D_CONSTEXPR bool node_is_self_v = node_is_self<_N>::value;
#endif


// ===========================================================================
// XI.  TOPOLOGY DETECTION
// ===========================================================================
// Probes for node structural types (dynamic containers, heterogeneous
// tuples) to power constexpr traversal logic.

// has_edges
//   trait: evaluates to true if _N has an `edges()` member.
template<typename _N,
         typename = void>
struct has_edges : std::false_type
{};

template<typename _N>
struct has_edges<_N,
    void_t<decltype(std::declval<_N>().edges())>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_edges_v
    //   variable template: value of has_edges<_N>.
    template<typename _N>
    D_CONSTEXPR bool has_edges_v = has_edges<_N>::value;
#endif

// is_dynamic_node
//   trait: evaluates to true if _N exposes an `edges()` accessor over
// its edge collection.  This is the signal the traversal policies use to
// decide a node's edges are iterable; note it does NOT by itself
// distinguish a dynamic (unbounded) edge container from a fixed-capacity
// one -- both expose edges().  The arity distinction is drawn in Part C
// by combining this with the compile-time link count.
template<typename _N>
struct is_dynamic_node : has_edges<_N>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_dynamic_node_v
    //   variable template: value of is_dynamic_node<_N>.
    template<typename _N>
    D_CONSTEXPR bool is_dynamic_node_v = is_dynamic_node<_N>::value;
#endif

// is_heterogeneous_node
//   trait: evaluates to true if _N exposes an `edge_tuple_type`
// alias.
template<typename _N,
         typename = void>
struct is_heterogeneous_node : std::false_type
{};

template<typename _N>
struct is_heterogeneous_node<_N,
    void_t<typename clean_t<_N>::edge_tuple_type>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_heterogeneous_node_v
    //   variable template: value of is_heterogeneous_node<_N>.
    template<typename _N>
    D_CONSTEXPR bool is_heterogeneous_node_v =
        is_heterogeneous_node<_N>::value;
#endif

// dynamic_link_extent
//   constant: represents an unbounded/dynamic link count.
D_STATIC_CONSTEXPR std::size_t dynamic_link_extent =
    static_cast<std::size_t>(-1);

NS_INTERNAL

    // has_num_links
    //   helper trait: true iff _N exposes a `num_links` static
    // member.  Used as the disambiguation predicate between
    // num_links-priority and edge_groups-fallback specializations
    // of node_link_count_helper below.
    //
    //   PRIOR ART: an earlier revision of this header attempted
    // to express the negation inline as
    //     !void_t<decltype(_N::num_links)*, int*>()
    // — that is malformed.  `void_t` is a TYPE alias resolving
    // to `void`, not a callable, and `void` cannot be negated
    // with `!`.  Negative-detection in SFINAE has to go through
    // a boolean trait first; that's what this helper provides.
    template<typename _N,
             typename = void>
    struct has_num_links : std::false_type
    {};

    template<typename _N>
    struct has_num_links<_N, void_t<decltype(clean_t<_N>::num_links)>>
        : std::true_type
    {};


    // node_link_count_helper
    //   trait: prioritized extraction of compile-time link count.
    // Priority: num_links > edge_groups > dynamic_link_extent.
    template<typename _N,
             typename = void,
             typename = void>
    struct node_link_count_helper
        : std::integral_constant<std::size_t, dynamic_link_extent>
    {};

    // node_link_count_helper — num_links present
    template<typename _N,
             typename _Dummy>
    struct node_link_count_helper<_N,
        void_t<decltype(clean_t<_N>::num_links)>,
        _Dummy>
        : std::integral_constant<std::size_t, clean_t<_N>::num_links>
    {};

    // node_link_count_helper — only edge_groups present
    //   (num_links takes priority via partial ordering)
    template<typename _N>
    struct node_link_count_helper<_N,
        typename std::enable_if<
            !has_num_links<_N>::value
        >::type,
        void_t<decltype(clean_t<_N>::edge_groups)>>
        : std::integral_constant<std::size_t, clean_t<_N>::edge_groups>
    {};

NS_END  // internal

// node_link_count
//   trait: extracts the compile-time link count of _N.
// Resolves to dynamic_link_extent for dynamically sized nodes.
// Prefers `num_links` over `edge_groups` when both are present.
template<typename _N,
         typename = void>
struct node_link_count
    : std::integral_constant<std::size_t, dynamic_link_extent>
{};

template<typename _N>
struct node_link_count<_N,
    void_t<decltype(clean_t<_N>::num_links)>>
    : std::integral_constant<std::size_t, clean_t<_N>::num_links>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_link_count_v
    //   variable template: value of node_link_count<_N>.
    template<typename _N>
    D_CONSTEXPR std::size_t node_link_count_v = node_link_count<_N>::value;
#endif

// node_edge_group_count
//   trait: extracts the compile-time edge group count from _N.
// Separate from node_link_count to avoid ambiguity when both
// num_links and edge_groups are present.
template<typename _N,
         typename = void>
struct node_edge_group_count
    : std::integral_constant<std::size_t, 0>
{};

template<typename _N>
struct node_edge_group_count<_N,
    void_t<decltype(clean_t<_N>::edge_groups)>>
    : std::integral_constant<std::size_t, clean_t<_N>::edge_groups>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_edge_group_count_v
    //   variable template: value of node_edge_group_count<_N>.
    template<typename _N>
    D_CONSTEXPR std::size_t node_edge_group_count_v =
        node_edge_group_count<_N>::value;
#endif


// ===========================================================================
// XII. LINK UTILITIES
// ===========================================================================

NS_INTERNAL

    // is_null_link_impl
    //   trait: dispatches null-link detection by handle form.

    // raw pointer form
    template<typename _L>
    D_CONSTEXPR auto is_null_link_test(const _L& _link, int)
        -> std::enable_if_t<std::is_pointer<_L>::value, bool>
    {
        return (_link == nullptr);
    }

    // smart pointer form (has operator bool)
    template<typename _L>
    D_CONSTEXPR auto is_null_link_test(const _L& _link, long)
        -> std::enable_if_t<
            node_is_smart_pointer<_L>::value, bool>
    {
        return (!_link);
    }

    // index form: sentinel is max value
    template<typename _L>
    D_CONSTEXPR auto is_null_link_test(const _L& _link, ...)
        -> std::enable_if_t<node_is_index<_L>::value, bool>
    {
        return (_link == static_cast<_L>(-1));
    }

NS_END  // internal

// is_null_link
//   function: tests whether a node link is null/sentinel.
// Works for raw pointers (nullptr), smart pointers (!ptr),
// and indices (== -1 sentinel).
template<typename _LinkType>
D_CONSTEXPR bool is_null_link(
    const _LinkType& _link
)
{
    return internal::is_null_link_test(_link, 0);
}


// ###########################################################################
// #  PART C -- VERTEX CLASSIFICATION  (monograph "Vertices")
// ###########################################################################


// ===========================================================================
// XIII. VERTEX ARITY REGIMES AND PAYLOAD
// ===========================================================================
//   The monograph classifies a vertex by the ARITY of its node -- the
// capacity of the edge collection K_N -- into four regimes:
//
//     edgeless  arity 0                 : a leaf (leaf node)
//     fixed     arity exactly k >= 1    : compile-time k (k=2 is binary)
//     n-ary     arity finite per node   : runtime, unbounded container
//     hybrid    k fixed + overflow      : a fixed block plus an n-ary tail
//
// and, orthogonally, by whether the vertex carries a payload P at all
// (payloaded) or is purely structural (payload-free, P = 1).
//
//   Classification here is structural and tag-free: it is composed from the
// topology detectors of Part B.  The decisive distinction between fixed and
// n-ary is the presence of a COMPILE-TIME link count: a fixed vertex reports
// a finite node_link_count (num_links / edge_groups), whereas an n-ary
// vertex exposes an edges() container with no compile-time bound.  This is
// why is_dynamic_node alone (which only detects edges()) is insufficient --
// linked_node exposes edges() over a fixed std::array yet is fixed, not
// n-ary.

// DVertexArity
//   enum: the arity regime of a node's vertex (monograph "Vertices").
enum class DVertexArity : std::uint8_t
{
    edgeless = 0,   // arity 0             — leaf node
    fixed    = 1,   // arity exactly k>=1  — fixed interior (k=2: binary)
    n_ary    = 2,   // arity per instance  — n-ary interior
    hybrid   = 3,   // k fixed + overflow  — hybrid interior
    unknown  = 4    // arity not structurally determinable
};

NS_INTERNAL

    // has_fixed_link_block
    //   helper: true iff _N declares a compile-time arity of at least one,
    // via a finite node_link_count (num_links) or a non-empty heterogeneous
    // edge_groups count.  This is the "fixed block" signal.
    template<typename _N>
    struct has_fixed_link_block
    {
        static constexpr bool value =
            ( ( node_link_count<_N>::value != dynamic_link_extent &&
                node_link_count<_N>::value != 0 )                    ||
              ( is_heterogeneous_node<_N>::value &&
                node_edge_group_count<_N>::value != 0 ) );
    };

    // has_unbounded_edges
    //   helper: true iff _N carries an edge collection with no compile-time
    // bound -- an edges() container that is not a fixed-count array, or an
    // n-ary / graph adjacency alias.  This is the "overflow / n-ary" signal.
    template<typename _N>
    struct has_unbounded_edges
    {
        static constexpr bool value =
            ( ( is_dynamic_node<_N>::value &&
                node_link_count<_N>::value == dynamic_link_extent ) ||
              is_nary_node<_N>::value                                ||
              is_graph_node<_N>::value );
    };

    // vertex_arity_impl
    //   trait: classifies the arity regime of _N by combining the fixed-block
    // and unbounded-edge signals with the small fixed-pointer topologies
    // (binary / singly / doubly linked) and the edgeless base case.
    template<typename _N>
    struct vertex_arity_impl
    {
        static constexpr bool fixed_block = has_fixed_link_block<_N>::value;
        static constexpr bool unbounded   = has_unbounded_edges<_N>::value;

        static constexpr DVertexArity value =
            // a fixed block AND an unbounded overflow -> hybrid
            ( fixed_block && unbounded )
                ? DVertexArity::hybrid

            // an unbounded edge collection -> n-ary
            : unbounded
                ? DVertexArity::n_ary

            // a compile-time k>=1, or a small fixed pointer topology -> fixed
            : ( fixed_block                    ||
                is_binary_node<_N>::value      ||
                is_singly_linked<_N>::value    ||
                is_doubly_linked<_N>::value )
                ? DVertexArity::fixed

            // an explicit zero link count, or a payload-only leaf -> edgeless
            : ( node_link_count<_N>::value == 0 ||
                is_leaf_capable<_N>::value )
                ? DVertexArity::edgeless

            : DVertexArity::unknown;
    };

NS_END  // internal

// vertex_arity_of
//   trait: the arity regime of _N's vertex.
template<typename _N>
struct vertex_arity_of
    : std::integral_constant<DVertexArity,
                             internal::vertex_arity_impl<_N>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // vertex_arity_of_v
    //   variable template: value of vertex_arity_of<_N>.
    template<typename _N>
    D_CONSTEXPR DVertexArity vertex_arity_of_v =
        vertex_arity_of<_N>::value;
#endif

// is_edgeless_node
//   trait: true if _N is an edgeless (leaf) vertex — arity 0.
template<typename _N>
struct is_edgeless_node
{
    static constexpr bool value =
        ( vertex_arity_of<_N>::value == DVertexArity::edgeless );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_edgeless_node_v
    //   variable template: value of is_edgeless_node<_N>.
    template<typename _N>
    D_CONSTEXPR bool is_edgeless_node_v = is_edgeless_node<_N>::value;
#endif

// is_fixed_arity_node
//   trait: true if _N is a fixed-arity interior vertex — arity k >= 1.
template<typename _N>
struct is_fixed_arity_node
{
    static constexpr bool value =
        ( vertex_arity_of<_N>::value == DVertexArity::fixed );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_fixed_arity_node_v
    //   variable template: value of is_fixed_arity_node<_N>.
    template<typename _N>
    D_CONSTEXPR bool is_fixed_arity_node_v =
        is_fixed_arity_node<_N>::value;
#endif

// is_nary_arity_node
//   trait: true if _N is an n-ary interior vertex — unbounded arity.
template<typename _N>
struct is_nary_arity_node
{
    static constexpr bool value =
        ( vertex_arity_of<_N>::value == DVertexArity::n_ary );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_nary_arity_node_v
    //   variable template: value of is_nary_arity_node<_N>.
    template<typename _N>
    D_CONSTEXPR bool is_nary_arity_node_v =
        is_nary_arity_node<_N>::value;
#endif

// is_hybrid_arity_node
//   trait: true if _N is a hybrid interior vertex — k fixed + overflow.
template<typename _N>
struct is_hybrid_arity_node
{
    static constexpr bool value =
        ( vertex_arity_of<_N>::value == DVertexArity::hybrid );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_hybrid_arity_node_v
    //   variable template: value of is_hybrid_arity_node<_N>.
    template<typename _N>
    D_CONSTEXPR bool is_hybrid_arity_node_v =
        is_hybrid_arity_node<_N>::value;
#endif

// is_interior_vertex
//   trait: true if _N bears at least one edge — fixed, n-ary, or hybrid.
// The complement of is_edgeless_node over the classified regimes.
template<typename _N>
struct is_interior_vertex
{
    static constexpr bool value =
        ( is_fixed_arity_node<_N>::value ||
          is_nary_arity_node<_N>::value  ||
          is_hybrid_arity_node<_N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_interior_vertex_v
    //   variable template: value of is_interior_vertex<_N>.
    template<typename _N>
    D_CONSTEXPR bool is_interior_vertex_v = is_interior_vertex<_N>::value;
#endif

// is_payloaded_vertex
//   trait: true if _N's vertex carries a payload P — it exposes data
// access or a `value_type`.  (monograph "Vertices": payloaded.)
template<typename _N>
struct is_payloaded_vertex
{
    static constexpr bool value =
        ( has_data_access<_N>::value ||
          has_value_type<_N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_payloaded_vertex_v
    //   variable template: value of is_payloaded_vertex<_N>.
    template<typename _N>
    D_CONSTEXPR bool is_payloaded_vertex_v = is_payloaded_vertex<_N>::value;
#endif

// is_payload_free_vertex
//   trait: true if _N's vertex is purely structural — it carries no datum
// beyond identity and edges (P = 1).  (monograph "Vertices": payload-free.)
template<typename _N>
struct is_payload_free_vertex
{
    static constexpr bool value = !is_payloaded_vertex<_N>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_payload_free_vertex_v
    //   variable template: value of is_payload_free_vertex<_N>.
    template<typename _N>
    D_CONSTEXPR bool is_payload_free_vertex_v =
        is_payload_free_vertex<_N>::value;
#endif


NS_END  // djinterp


#endif  // DJINTERP_CONTAINER_NODE_COMMON_
