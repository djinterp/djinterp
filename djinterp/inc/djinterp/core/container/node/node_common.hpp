/*******************************************************************************
* djinterp [core]                                                node_common.hpp
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
*   dynamic_node<T, Alloc, Cont> n-ary vertex, edges in a dynamic container
*   tuple_node<T, Edges...> heterogeneous fixed vertex, edges in a tuple
*
* PART B -- NODE STRUCTURAL TRAITS (merged from node_traits.hpp)
*   Purely structural, SFINAE-based detection.  Every trait takes the NODE
* type as its primary parameter (never the container), and answers a
* concrete, falsifiable question about a node's interface.  No taxonomy or
* tag type is imposed on the node; policy is left to the consumer.
*
*     member/method/unified field access ....... has_next, has_left, ...
*     node handle form ......................... node_is_raw_pointer, ...
*     composite structural queries ............. is_binary_node, is_nary_node,
*   ...
*     polymorphism / self-reference ............ node_is_polymorphic,
*   node_is_self
*     topology detection ....................... has_edges, is_dynamic_node,
*   ...
*     link utilities ........................... is_null_link, node_link_count
*
* PART C -- VERTEX CLASSIFICATION (monograph "Vertices")
*   DVertexArity + vertex_arity_of edgeless / fixed / n-ary / hybrid
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
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.03.30
*                                                            revised: 2026.10.02
*******************************************************************************/

/*
TABLE OF CONTENTS
=================

      PART A -- NODE TYPES

I.    leaf_node
      ---------

II.   dynamic_node
      ------------

III.  tuple_node
      ----------

      PART B -- NODE STRUCTURAL TRAITS

IV.   Node Member Detection (field access)
      ------------------------------------

V.    Node Method Detection (callable form)
      -------------------------------------

VI.   Unified Access Detection (field OR method)
      ------------------------------------------

VII.  Node Handle Form Detection
      --------------------------

VIII. Composite Structural Queries
      ----------------------------

IX.   Polymorphism Detection
      ----------------------

X.    Self-Referential Node Detection
      -------------------------------

XI.   Topology Detection
      ------------------

XII.  Link Utilities
      --------------

      PART C -- VERTEX CLASSIFICATION

XIII. Vertex Arity Regimes and Payload
      --------------------------------
*/

#ifndef DJINTERP_CONTAINER_NODE_NODE_COMMON_HPP
#define DJINTERP_CONTAINER_NODE_NODE_COMMON_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <memory>
#include <tuple>
#include <type_traits>
#include <vector>
// djinterp
#include "../../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "../traits/container_traits.hpp"
// re_std
#include "../../../../re_std/cstdint/cstdint.hpp"  // re_std::uint8_t

// node_common requires C++11 or higher for decltype, SFINAE, and variadic
// templates. Checked here, right after the core header supplies the feature
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
template<typename Type>
class leaf_node
{
public:
    using self_type  = leaf_node;
    using value_type = Type;

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
template<typename Type,
         typename NodeAllocator = std::allocator<Type>,
         template<typename, typename> class ContainerType = std::vector>
class dynamic_node
{
public:
    using self_type      = dynamic_node;
    using value_type     = Type;
    using allocator_type = NodeAllocator;

    // Resolve self* to the actual type
    using link_type = resolve_self_t<self*, self_type>;

    // Rebind the allocator for the links
    using edge_allocator = typename std::allocator_traits<
        allocator_type>::template rebind_alloc<link_type>;

    // The final dynamically-sized edge container
    using container_type = ContainerType<link_type, edge_allocator>;

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
template<typename    Type,
         typename... Edges>
class tuple_node
{
public:
    using self_type  = tuple_node;
    using value_type = Type;

    // Apply resolve_self recursively across the parameter pack
    using edge_tuple_type = std::tuple<resolve_self_t<Edges, self_type>...>;

    static D_CONSTEXPR std::size_t edge_groups = sizeof...(Edges);

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

    template<std::size_t Index>
    D_CONSTEXPR auto& get_edge_group()
    {
        static_assert(Index < edge_groups,
                      "Edge index out of bounds.");
        return std::get<Index>(m_edges);
    }

    template<std::size_t Index>
    D_CONSTEXPR const auto& get_edge_group() const
    {
        static_assert(Index < edge_groups,
                      "Edge index out of bounds.");
        return std::get<Index>(m_edges);
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
// node type N. Detection is purely syntactic; no semantic verification
// is performed. Every trait evaluates to std::true_type or std::false_type.

// has_next
//   trait: evaluates to true if N has a `next` data member.
template<typename N,
         typename = void>
struct has_next : std::false_type
{};

// has_next<N, void_t<decltype(std::declval<N>().next)>>
//   trait: the `void_t<decltype(std::declval<N>().next)>` case; it reports
// true.
template<typename N>
struct has_next<N, void_t<decltype(std::declval<N>().next)>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_next_v
    //   variable template: value of has_next<N>.
    template<typename N>
    D_CONSTEXPR bool has_next_v = has_next<N>::value;
#endif

// has_prev
//   trait: evaluates to true if N has a `prev` data member.
template<typename N,
         typename = void>
struct has_prev : std::false_type
{};

// has_prev<N, void_t<decltype(std::declval<N>().prev)>>
//   trait: the `void_t<decltype(std::declval<N>().prev)>` case; it reports
// true.
template<typename N>
struct has_prev<N,
    void_t<decltype(std::declval<N>().prev)>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_prev_v
    //   variable template: value of has_prev<N>.
    template<typename N>
    D_CONSTEXPR bool has_prev_v = has_prev<N>::value;
#endif

// has_parent
//   trait: evaluates to true if N has a `parent` data member.
template<typename N,
         typename = void>
struct has_parent : std::false_type
{};

// has_parent<N, void_t<decltype(std::declval<N>().parent)>>
//   trait: the `void_t<decltype(std::declval<N>().parent)>` case; it reports
// true.
template<typename N>
struct has_parent<N,
    void_t<decltype(std::declval<N>().parent)>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_parent_v
    //   variable template: value of has_parent<N>.
    template<typename N>
    D_CONSTEXPR bool has_parent_v = has_parent<N>::value;
#endif

// has_left
//   trait: evaluates to true if N has a `left` data member.
template<typename N,
         typename = void>
struct has_left : std::false_type
{};

// has_left<N, void_t<decltype(std::declval<N>().left)>>
//   trait: the `void_t<decltype(std::declval<N>().left)>` case; it reports
// true.
template<typename N>
struct has_left<N,
    void_t<decltype(std::declval<N>().left)>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_left_v
    //   variable template: value of has_left<N>.
    template<typename N>
    D_CONSTEXPR bool has_left_v = has_left<N>::value;
#endif

// has_right
//   trait: evaluates to true if N has a `right` data member.
template<typename N,
         typename = void>
struct has_right : std::false_type
{};

// has_right<N, void_t<decltype(std::declval<N>().right)>>
//   trait: the `void_t<decltype(std::declval<N>().right)>` case; it reports
// true.
template<typename N>
struct has_right<N,
    void_t<decltype(std::declval<N>().right)>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_right_v
    //   variable template: value of has_right<N>.
    template<typename N>
    D_CONSTEXPR bool has_right_v = has_right<N>::value;
#endif

// has_data
//   trait: evaluates to true if N has a `data` data member.
template<typename N,
         typename = void>
struct has_data : std::false_type
{};

// has_data<N, void_t<decltype(std::declval<N>().data)>>
//   trait: the `void_t<decltype(std::declval<N>().data)>` case; it reports
// true.
template<typename N>
struct has_data<N,
    void_t<decltype(std::declval<N>().data)>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_data_v
    //   variable template: value of has_data<N>.
    template<typename N>
    D_CONSTEXPR bool has_data_v = has_data<N>::value;
#endif

// has_children_type
//   trait: evaluates to true if N has a nested `children_type` type alias.
template<typename N,
         typename = void>
struct has_children_type : std::false_type
{};

// has_children_type<N, void_t<typename clean_t<N>
//   trait: the `void_t<typename clean_t<N` case; it reports true.
template<typename N>
struct has_children_type<N,
    void_t<typename clean_t<N>::children_type>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_children_type_v
    //   variable template: value of has_children_type<N>.
    template<typename N>
    D_CONSTEXPR bool has_children_type_v =
        has_children_type<N>::value;
#endif

// has_adjacency_type
//   trait: evaluates to true if N has a nested `adjacency_type` type alias.
template<typename N,
         typename = void>
struct has_adjacency_type : std::false_type
{};

// has_adjacency_type<N, void_t<typename clean_t<N>
//   trait: the `void_t<typename clean_t<N` case; it reports true.
template<typename N>
struct has_adjacency_type<N,
    void_t<typename clean_t<N>::adjacency_type>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_adjacency_type_v
    //   variable template: value of has_adjacency_type<N>.
    template<typename N>
    D_CONSTEXPR bool has_adjacency_type_v =
        has_adjacency_type<N>::value;
#endif


// ===========================================================================
// V.   NODE METHOD DETECTION (callable form)
// ===========================================================================
// Detects member functions by attempting a call expression. This is
// necessary for node types whose API is method-based (e.g. linked_node
// derivatives with overloaded const/non-const accessors).

// has_left_method
//   trait: evaluates to true if N has a callable `left()` method.
template<typename N,
         typename = void>
struct has_left_method : std::false_type
{};

// has_left_method<N, void_t<decltype(std::declval<N>().left())>>
//   trait: the `void_t<decltype(std::declval<N>().left())>` case; it reports
// true.
template<typename N>
struct has_left_method<N,
    void_t<decltype(std::declval<N>().left())>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_left_method_v
    //   variable template: value of has_left_method<N>.
    template<typename N>
    D_CONSTEXPR bool has_left_method_v = has_left_method<N>::value;
#endif

// has_right_method
//   trait: evaluates to true if N has a callable `right()` method.
template<typename N,
         typename = void>
struct has_right_method : std::false_type
{};

// has_right_method<N, void_t<decltype(std::declval<N>().right())>>
//   trait: the `void_t<decltype(std::declval<N>().right())>` case; it reports
// true.
template<typename N>
struct has_right_method<N,
    void_t<decltype(std::declval<N>().right())>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_right_method_v
    //   variable template: value of has_right_method<N>.
    template<typename N>
    D_CONSTEXPR bool has_right_method_v = has_right_method<N>::value;
#endif

// has_parent_method
//   trait: evaluates to true if N has a callable `parent()` method.
template<typename N,
         typename = void>
struct has_parent_method : std::false_type
{};

// has_parent_method<N, void_t<decltype(std::declval<N>().parent())>>
//   trait: the `void_t<decltype(std::declval<N>().parent())>` case; it
// reports true.
template<typename N>
struct has_parent_method<N,
    void_t<decltype(std::declval<N>().parent())>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_parent_method_v
    //   variable template: value of has_parent_method<N>.
    template<typename N>
    D_CONSTEXPR bool has_parent_method_v = has_parent_method<N>::value;
#endif

// has_next_method
//   trait: evaluates to true if N has a callable `next()` method.
template<typename N,
         typename = void>
struct has_next_method : std::false_type
{};

// has_next_method<N, void_t<decltype(std::declval<N>().next())>>
//   trait: the `void_t<decltype(std::declval<N>().next())>` case; it reports
// true.
template<typename N>
struct has_next_method<N,
    void_t<decltype(std::declval<N>().next())>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_next_method_v
    //   variable template: value of has_next_method<N>.
    template<typename N>
    D_CONSTEXPR bool has_next_method_v = has_next_method<N>::value;
#endif

// has_prev_method
//   trait: evaluates to true if N has a callable `prev()` method.
template<typename N,
         typename = void>
struct has_prev_method : std::false_type
{};

// has_prev_method<N, void_t<decltype(std::declval<N>().prev())>>
//   trait: the `void_t<decltype(std::declval<N>().prev())>` case; it reports
// true.
template<typename N>
struct has_prev_method<N,
    void_t<decltype(std::declval<N>().prev())>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_prev_method_v
    //   variable template: value of has_prev_method<N>.
    template<typename N>
    D_CONSTEXPR bool has_prev_method_v = has_prev_method<N>::value;
#endif

// has_data_method
//   trait: evaluates to true if N has a callable `data()` method.
//   NOTE: container_traits.hpp defines a `has_data_method` over the CONTAINER
// surface (const-qualified probe). That trait is included via the chain above
// and lives in the same djinterp namespace; the node layer reuses it rather
// than redefining it, so no node-local duplicate is declared here.


// ===========================================================================
// VI.  UNIFIED ACCESS DETECTION (field OR method)
// ===========================================================================
// These accept either form — data members or methods — so composite
// traits work uniformly across POD-style nodes and method-based nodes
// (e.g. linked_node derivatives).

// has_data_access
//   trait: evaluates to true if N has either a `data` member or a callable
// `data()` method.
template<typename N>
struct has_data_access
{
    static constexpr bool value =
        ( has_data<N>::value ||
          has_data_method<N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_data_access_v
    //   variable template: value of has_data_access<N>.
    template<typename N>
    D_CONSTEXPR bool has_data_access_v = has_data_access<N>::value;
#endif

// has_left_access
//   trait: evaluates to true if N has either a `left` member or a callable
// `left()` method.
template<typename N>
struct has_left_access
{
    static constexpr bool value =
        ( has_left<N>::value ||
          has_left_method<N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_left_access_v
    //   variable template: value of has_left_access<N>.
    template<typename N>
    D_CONSTEXPR bool has_left_access_v = has_left_access<N>::value;
#endif

// has_right_access
//   trait: evaluates to true if N has either a `right` member or a callable
// `right()` method.
template<typename N>
struct has_right_access
{
    static constexpr bool value =
        ( has_right<N>::value ||
          has_right_method<N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_right_access_v
    //   variable template: value of has_right_access<N>.
    template<typename N>
    D_CONSTEXPR bool has_right_access_v = has_right_access<N>::value;
#endif

// has_parent_access
//   trait: evaluates to true if N has either a `parent` member or a callable
// `parent()` method.
template<typename N>
struct has_parent_access
{
    static constexpr bool value =
        ( has_parent<N>::value ||
          has_parent_method<N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_parent_access_v
    //   variable template: value of has_parent_access<N>.
    template<typename N>
    D_CONSTEXPR bool has_parent_access_v = has_parent_access<N>::value;
#endif

// has_next_access
//   trait: evaluates to true if N has either a `next` member or a callable
// `next()` method.
template<typename N>
struct has_next_access
{
    static constexpr bool value =
        ( has_next<N>::value ||
          has_next_method<N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_next_access_v
    //   variable template: value of has_next_access<N>.
    template<typename N>
    D_CONSTEXPR bool has_next_access_v = has_next_access<N>::value;
#endif

// has_prev_access
//   trait: evaluates to true if N has either a `prev` member or a callable
// `prev()` method.
template<typename N>
struct has_prev_access
{
    static constexpr bool value =
        ( has_prev<N>::value ||
          has_prev_method<N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_prev_access_v
    //   variable template: value of has_prev_access<N>.
    template<typename N>
    D_CONSTEXPR bool has_prev_access_v = has_prev_access<N>::value;
#endif


// ===========================================================================
// VII. NODE HANDLE FORM DETECTION
// ===========================================================================
// Classifies the structural form of a type H used as a node
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
    template<typename T>
    struct is_unique_ptr : std::false_type
    {};

    // is_unique_ptr<std::unique_ptr<T, D>>
    //   trait: the `std::unique_ptr<T, D>` case; it reports true.
    template<typename T,
             typename D>
    struct is_unique_ptr<std::unique_ptr<T, D>> : std::true_type
    {};

    // is_shared_ptr
    //   trait: detects std::shared_ptr<...>.
    template<typename T>
    struct is_shared_ptr : std::false_type
    {};

    // is_shared_ptr<std::shared_ptr<T>>
    //   trait: the `std::shared_ptr<T>` case; it reports true.
    template<typename T>
    struct is_shared_ptr<std::shared_ptr<T>> : std::true_type
    {};

    // is_weak_ptr
    //   trait: detects std::weak_ptr<...>.
    template<typename T>
    struct is_weak_ptr : std::false_type
    {};

    // is_weak_ptr<std::weak_ptr<T>>
    //   trait: the `std::weak_ptr<T>` case; it reports true.
    template<typename T>
    struct is_weak_ptr<std::weak_ptr<T>> : std::true_type
    {};

NS_END  // internal

// node_is_raw_pointer
//   trait: evaluates to true if H is a raw pointer type.
template<typename H>
struct node_is_raw_pointer
    : std::is_pointer<H>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_raw_pointer_v
    //   variable template: value of node_is_raw_pointer<H>.
    template<typename H>
    D_CONSTEXPR bool node_is_raw_pointer_v =
        node_is_raw_pointer<H>::value;
#endif

// node_is_unique_pointer
//   trait: evaluates to true if H is a std::unique_ptr instantiation.
template<typename H>
struct node_is_unique_pointer
    : internal::is_unique_ptr<H>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_unique_pointer_v
    //   variable template: value of node_is_unique_pointer<H>.
    template<typename H>
    D_CONSTEXPR bool node_is_unique_pointer_v =
        node_is_unique_pointer<H>::value;
#endif

// node_is_shared_pointer
//   trait: evaluates to true if H is a std::shared_ptr instantiation.
template<typename H>
struct node_is_shared_pointer
    : internal::is_shared_ptr<H>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_shared_pointer_v
    //   variable template: value of node_is_shared_pointer<H>.
    template<typename H>
    D_CONSTEXPR bool node_is_shared_pointer_v =
        node_is_shared_pointer<H>::value;
#endif

// node_is_weak_pointer
//   trait: evaluates to true if H is a std::weak_ptr instantiation.
template<typename H>
struct node_is_weak_pointer
    : internal::is_weak_ptr<H>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_weak_pointer_v
    //   variable template: value of node_is_weak_pointer<H>.
    template<typename H>
    D_CONSTEXPR bool node_is_weak_pointer_v =
        node_is_weak_pointer<H>::value;
#endif

// node_is_smart_pointer
//   trait: evaluates to true if H is any of unique_ptr, shared_ptr, or
// weak_ptr.
template<typename H>
struct node_is_smart_pointer
{
    static constexpr bool value =
        ( internal::is_unique_ptr<H>::value ||
          internal::is_shared_ptr<H>::value ||
          internal::is_weak_ptr<H>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_smart_pointer_v
    //   variable template: value of node_is_smart_pointer<H>.
    template<typename H>
    D_CONSTEXPR bool node_is_smart_pointer_v =
        node_is_smart_pointer<H>::value;
#endif

// node_is_any_pointer
//   trait: evaluates to true if H is a raw pointer or any standard smart
// pointer.
template<typename H>
struct node_is_any_pointer
{
    static constexpr bool value =
        ( std::is_pointer<H>::value ||
          node_is_smart_pointer<H>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_any_pointer_v
    //   variable template: value of node_is_any_pointer<H>.
    template<typename H>
    D_CONSTEXPR bool node_is_any_pointer_v =
        node_is_any_pointer<H>::value;
#endif

// node_is_index
//   trait: evaluates to true if H is a non-bool integral type, indicating
// index-based node access into a backing store.
template<typename H>
struct node_is_index
{
    static constexpr bool value =
        ( std::is_integral<H>::value &&
          !std::is_same<H, bool>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_index_v
    //   variable template: value of node_is_index<H>.
    template<typename H>
    D_CONSTEXPR bool node_is_index_v = node_is_index<H>::value;
#endif

// node_is_handle
//   trait: evaluates to true if H is a class type with an `element_type`
// member alias (the convention for smart-pointer- like handle types, e.g.
// slot-map keys, arena handles).
// Standard smart pointers also satisfy this, but are better identified by the
// specific traits above.
template<typename H,
         typename = void>
struct node_is_handle : std::false_type
{};

// node_is_handle<H, void_t<typename clean_t<H>
//   trait: the `void_t<typename clean_t<H` case; it reports true.
template<typename H>
struct node_is_handle<H,
    void_t<typename clean_t<H>::element_type>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_handle_v
    //   variable template: value of node_is_handle<H>.
    template<typename H>
    D_CONSTEXPR bool node_is_handle_v = node_is_handle<H>::value;
#endif


// ===========================================================================
// VIII. COMPOSITE STRUCTURAL QUERIES
// ===========================================================================
// Higher-level traits combining multiple member detections to answer
// common structural questions about a node type. Uses the unified
// access traits so that both field-based (POD) and method-based
// (linked_node-derived) nodes are detected correctly.

// is_singly_linked
//   trait: evaluates to true if N has next access but not prev.
template<typename N>
struct is_singly_linked
{
    static constexpr bool value =
        ( has_next_access<N>::value &&
          !has_prev_access<N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_singly_linked_v
    //   variable template: value of is_singly_linked<N>.
    template<typename N>
    D_CONSTEXPR bool is_singly_linked_v = is_singly_linked<N>::value;
#endif

// is_doubly_linked
//   trait: evaluates to true if N has both next and prev access.
template<typename N>
struct is_doubly_linked
{
    static constexpr bool value =
        ( has_next_access<N>::value &&
          has_prev_access<N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_doubly_linked_v
    //   variable template: value of is_doubly_linked<N>.
    template<typename N>
    D_CONSTEXPR bool is_doubly_linked_v = is_doubly_linked<N>::value;
#endif

// is_binary_node
//   trait: evaluates to true if N has left and right access. Does not require
// parent.
template<typename N>
struct is_binary_node
{
    static constexpr bool value =
        ( has_left_access<N>::value &&
          has_right_access<N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_binary_node_v
    //   variable template: value of is_binary_node<N>.
    template<typename N>
    D_CONSTEXPR bool is_binary_node_v = is_binary_node<N>::value;
#endif

// is_parented
//   trait: evaluates to true if N has parent access.
template<typename N>
struct is_parented
{
    static constexpr bool value = has_parent_access<N>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_parented_v
    //   variable template: value of is_parented<N>.
    template<typename N>
    D_CONSTEXPR bool is_parented_v = is_parented<N>::value;
#endif

// is_parented_binary_node
//   trait: evaluates to true if N has parent, left, and right access.
template<typename N>
struct is_parented_binary_node
{
    static constexpr bool value =
        ( is_binary_node<N>::value &&
          has_parent_access<N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_parented_binary_node_v
    //   variable template: value of is_parented_binary_node<N>.
    template<typename N>
    D_CONSTEXPR bool is_parented_binary_node_v =
        is_parented_binary_node<N>::value;
#endif

// is_nary_node
//   trait: evaluates to true if N has parent access and a `children_type`
// nested type alias.
template<typename N>
struct is_nary_node
{
    static constexpr bool value =
        ( has_parent_access<N>::value &&
          has_children_type<N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_nary_node_v
    //   variable template: value of is_nary_node<N>.
    template<typename N>
    D_CONSTEXPR bool is_nary_node_v = is_nary_node<N>::value;
#endif

// is_graph_node
//   trait: evaluates to true if N has an `adjacency_type` nested type alias.
template<typename N>
struct is_graph_node
{
    static constexpr bool value = has_adjacency_type<N>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_graph_node_v
    //   variable template: value of is_graph_node<N>.
    template<typename N>
    D_CONSTEXPR bool is_graph_node_v = is_graph_node<N>::value;
#endif

// is_keyed_node
//   trait: evaluates to true if N has both a `key_type` alias and data
// access. Common in associative-container nodes where the key and payload are
// separate concerns.
template<typename N>
struct is_keyed_node
{
    static constexpr bool value =
        ( has_key_type<N>::value &&
          has_data_access<N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_keyed_node_v
    //   variable template: value of is_keyed_node<N>.
    template<typename N>
    D_CONSTEXPR bool is_keyed_node_v = is_keyed_node<N>::value;
#endif

// is_leaf_capable
//   trait: evaluates to true if N carries payload (has data access or
// `value_type`) but has no child links (no left, right, children_type, or
// adjacency_type). Structural indicator that N can only be a leaf node.
template<typename N>
struct is_leaf_capable
{
    static constexpr bool value =
        ( ( has_data_access<N>::value ||
            has_value_type<N>::value )          &&
          !has_left_access<N>::value            &&
          !has_right_access<N>::value           &&
          !has_children_type<N>::value          &&
          !has_adjacency_type<N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_leaf_capable_v
    //   variable template: value of is_leaf_capable<N>.
    template<typename N>
    D_CONSTEXPR bool is_leaf_capable_v = is_leaf_capable<N>::value;
#endif


// ===========================================================================
// IX.  POLYMORPHISM DETECTION
// ===========================================================================

// node_is_polymorphic
//   trait: evaluates to true if N has at least one virtual method (i.e.
// std::is_polymorphic). Indicates the node participates in a type hierarchy
// where different concrete node types (e.g. leaf vs. interior) share a common
// base.
template<typename N>
struct node_is_polymorphic
{
    static constexpr bool value = std::is_polymorphic<N>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_polymorphic_v
    //   variable template: value of node_is_polymorphic<N>.
    template<typename N>
    D_CONSTEXPR bool node_is_polymorphic_v =
        node_is_polymorphic<N>::value;
#endif

// node_derives_from
//   trait: evaluates to true if N derives from Base. Two-parameter trait for
// verifying a node against a known base class (e.g. node_base,
// tree_node_base).
template<typename N,
         typename Base>
struct node_derives_from
{
    static constexpr bool value =
        ( std::is_base_of<Base, N>::value &&
          !std::is_same<Base, N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_derives_from_v
    //   variable template: value of node_derives_from<N, Base>.
    template<typename N,
             typename Base>
    D_CONSTEXPR bool node_derives_from_v =
        node_derives_from<N, Base>::value;
#endif


// ===========================================================================
// X.   SELF-REFERENTIAL NODE DETECTION
// ===========================================================================
// Integration with djinterp::self / djinterp::resolve_self.

// node_is_self
//   trait: evaluates to true if N is the djinterp::self marker
// type, indicating the node type is a placeholder that resolves to the owning
// type via resolve_self.
template<typename N>
struct node_is_self
{
    static constexpr bool value = djinterp::is_self<N>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_is_self_v
    //   variable template: value of node_is_self<N>.
    template<typename N>
    D_CONSTEXPR bool node_is_self_v = node_is_self<N>::value;
#endif


// ===========================================================================
// XI.  TOPOLOGY DETECTION
// ===========================================================================
// Probes for node structural types (dynamic containers, heterogeneous
// tuples) to power constexpr traversal logic.

// has_edges
//   trait: evaluates to true if N has an `edges()` member.
template<typename N,
         typename = void>
struct has_edges : std::false_type
{};

// has_edges<N, void_t<decltype(std::declval<N>().edges())>>
//   trait: the `void_t<decltype(std::declval<N>().edges())>` case; it reports
// true.
template<typename N>
struct has_edges<N,
    void_t<decltype(std::declval<N>().edges())>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // has_edges_v
    //   variable template: value of has_edges<N>.
    template<typename N>
    D_CONSTEXPR bool has_edges_v = has_edges<N>::value;
#endif

// is_dynamic_node
//   trait: evaluates to true if N exposes an `edges()` accessor over its edge
// collection. This is the signal the traversal policies use to decide a node's
// edges are iterable; note it does NOT by itself distinguish a dynamic
// (unbounded) edge container from a fixed-capacity one -- both expose edges().
// The arity distinction is drawn in Part C by combining this with the
// compile-time link count.
template<typename N>
struct is_dynamic_node : has_edges<N>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_dynamic_node_v
    //   variable template: value of is_dynamic_node<N>.
    template<typename N>
    D_CONSTEXPR bool is_dynamic_node_v = is_dynamic_node<N>::value;
#endif

// is_heterogeneous_node
//   trait: evaluates to true if N exposes an `edge_tuple_type` alias.
template<typename N,
         typename = void>
struct is_heterogeneous_node : std::false_type
{};

// is_heterogeneous_node<N, void_t<typename clean_t<N>
//   trait: the `void_t<typename clean_t<N` case; it reports true.
template<typename N>
struct is_heterogeneous_node<N,
    void_t<typename clean_t<N>::edge_tuple_type>>
    : std::true_type
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_heterogeneous_node_v
    //   variable template: value of is_heterogeneous_node<N>.
    template<typename N>
    D_CONSTEXPR bool is_heterogeneous_node_v =
        is_heterogeneous_node<N>::value;
#endif

// dynamic_link_extent
//   constant: represents an unbounded/dynamic link count.
D_STATIC_CONSTEXPR std::size_t dynamic_link_extent =
    static_cast<std::size_t>(-1);

NS_INTERNAL

    // has_num_links
    //   helper trait: true iff N exposes a `num_links` static member. Used as
    // the disambiguation predicate between num_links-priority and
    // edge_groups-fallback specializations of node_link_count_helper below.
    //
    //   PRIOR ART: an earlier revision of this header attempted
    // to express the negation inline as
    //     !void_t<decltype(N::num_links)*, int*>() — that is malformed.
    // `void_t` is a TYPE alias resolving to `void`, not a callable, and `void`
    // cannot be negated with `!`. Negative-detection in SFINAE has to go
    // through a boolean trait first; that's what this helper provides.
    template<typename N,
             typename = void>
    struct has_num_links : std::false_type
    {};

    // has_num_links<N, void_t<decltype(clean_t<N>
    //   trait: the `void_t<decltype(clean_t<N` case; it reports true.
    template<typename N>
    struct has_num_links<N, void_t<decltype(clean_t<N>::num_links)>>
        : std::true_type
    {};


    // node_link_count_helper
    //   trait: prioritized extraction of compile-time link count. Priority:
    // num_links > edge_groups > dynamic_link_extent.
    template<typename N,
             typename = void,
             typename = void>
    struct node_link_count_helper
        : std::integral_constant<std::size_t, dynamic_link_extent>
    {};

    // node_link_count_helper — num_links present node_link_count_helper<N,
    // void_t<decltype(clean_t<N>
    //   trait: the `void_t<decltype(clean_t<N` case; it reports `clean_t<N`.
    template<typename N,
             typename Dummy>
    struct node_link_count_helper<N,
        void_t<decltype(clean_t<N>::num_links)>,
        Dummy>
        : std::integral_constant<std::size_t, clean_t<N>::num_links>
    {};

    // node_link_count_helper — only edge_groups present
    //   (num_links takes priority via partial ordering)
    // node_link_count_helper<N, typename std::enable_if< !has_num_links<N>
    //   trait: the `typename std::enable_if< !has_num_links<N` case; it
    // reports `clean_t<N`.
    template<typename N>
    struct node_link_count_helper<N,
        typename std::enable_if<
            !has_num_links<N>::value
        >::type,
        void_t<decltype(clean_t<N>::edge_groups)>>
        : std::integral_constant<std::size_t, clean_t<N>::edge_groups>
    {};

NS_END  // internal

// node_link_count
//   trait: extracts the compile-time link count of N. Resolves to
// dynamic_link_extent for dynamically sized nodes. Prefers `num_links` over
// `edge_groups` when both are present.
template<typename N,
         typename = void>
struct node_link_count
    : std::integral_constant<std::size_t, dynamic_link_extent>
{};

// node_link_count<N, void_t<decltype(clean_t<N>
//   trait: the `void_t<decltype(clean_t<N` case; it reports `clean_t<N`.
template<typename N>
struct node_link_count<N,
    void_t<decltype(clean_t<N>::num_links)>>
    : std::integral_constant<std::size_t, clean_t<N>::num_links>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_link_count_v
    //   variable template: value of node_link_count<N>.
    template<typename N>
    D_CONSTEXPR std::size_t node_link_count_v = node_link_count<N>::value;
#endif

// node_edge_group_count
//   trait: extracts the compile-time edge group count from N.
// Separate from node_link_count to avoid ambiguity when both num_links and
// edge_groups are present.
template<typename N,
         typename = void>
struct node_edge_group_count
    : std::integral_constant<std::size_t, 0>
{};

// node_edge_group_count<N, void_t<decltype(clean_t<N>
//   trait: the `void_t<decltype(clean_t<N` case; it reports `clean_t<N`.
template<typename N>
struct node_edge_group_count<N,
    void_t<decltype(clean_t<N>::edge_groups)>>
    : std::integral_constant<std::size_t, clean_t<N>::edge_groups>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // node_edge_group_count_v
    //   variable template: value of node_edge_group_count<N>.
    template<typename N>
    D_CONSTEXPR std::size_t node_edge_group_count_v =
        node_edge_group_count<N>::value;
#endif


// ===========================================================================
// XII. LINK UTILITIES
// ===========================================================================

NS_INTERNAL

    // is_null_link_impl
    //   trait: dispatches null-link detection by handle form.

    // raw pointer form
    template<typename L>
    D_CONSTEXPR auto is_null_link_test(const L& _link, int)
        -> std::enable_if_t<std::is_pointer<L>::value, bool>
    {
        return (_link == nullptr);
    }

    // smart pointer form (has operator bool)
    template<typename L>
    D_CONSTEXPR auto is_null_link_test(const L& _link, long)
        -> std::enable_if_t<
            node_is_smart_pointer<L>::value, bool>
    {
        return (!_link);
    }

    // index form: sentinel is max value
    template<typename L>
    D_CONSTEXPR auto is_null_link_test(const L& _link, ...)
        -> std::enable_if_t<node_is_index<L>::value, bool>
    {
        return (_link == static_cast<L>(-1));
    }

NS_END  // internal

// is_null_link
//   function: tests whether a node link is null/sentinel. Works for raw
// pointers (nullptr), smart pointers (!ptr), and indices (== -1 sentinel).
template<typename LinkType>
D_CONSTEXPR bool is_null_link(
    const LinkType& _link
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
enum class DVertexArity : re_std::uint8_t
{
    edgeless = 0,   // arity 0             — leaf node
    fixed    = 1,   // arity exactly k>=1  — fixed interior (k=2: binary)
    n_ary    = 2,   // arity per instance  — n-ary interior
    hybrid   = 3,   // k fixed + overflow  — hybrid interior
    unknown  = 4    // arity not structurally determinable
};

NS_INTERNAL

    // has_fixed_link_block
    //   helper: true iff N declares a compile-time arity of at least one, via
    // a finite node_link_count (num_links) or a non-empty heterogeneous
    // edge_groups count. This is the "fixed block" signal.
    template<typename N>
    struct has_fixed_link_block
    {
        static constexpr bool value =
            ( ( node_link_count<N>::value != dynamic_link_extent &&
                node_link_count<N>::value != 0 )                    ||
              ( is_heterogeneous_node<N>::value &&
                node_edge_group_count<N>::value != 0 ) );
    };

    // has_unbounded_edges
    //   helper: true iff N carries an edge collection with no compile-time
    // bound -- an edges() container that is not a fixed-count array, or an
    // n-ary / graph adjacency alias. This is the "overflow / n-ary" signal.
    template<typename N>
    struct has_unbounded_edges
    {
        static constexpr bool value =
            ( ( is_dynamic_node<N>::value &&
                node_link_count<N>::value == dynamic_link_extent ) ||
              is_nary_node<N>::value                                ||
              is_graph_node<N>::value );
    };

    // vertex_arity_impl
    //   trait: classifies the arity regime of N by combining the fixed-block
    // and unbounded-edge signals with the small fixed-pointer topologies
    // (binary / singly / doubly linked) and the edgeless base case.
    template<typename N>
    struct vertex_arity_impl
    {
        static constexpr bool fixed_block = has_fixed_link_block<N>::value;
        static constexpr bool unbounded   = has_unbounded_edges<N>::value;

        static constexpr DVertexArity value =
            // a fixed block AND an unbounded overflow -> hybrid
            ( fixed_block && unbounded )
                ? DVertexArity::hybrid

            // an unbounded edge collection -> n-ary
            : unbounded
                ? DVertexArity::n_ary

            // a compile-time k>=1, or a small fixed pointer topology -> fixed
            : ( fixed_block                    ||
                is_binary_node<N>::value      ||
                is_singly_linked<N>::value    ||
                is_doubly_linked<N>::value )
                ? DVertexArity::fixed

            // an explicit zero link count, or a payload-only leaf -> edgeless
            : ( node_link_count<N>::value == 0 ||
                is_leaf_capable<N>::value )
                ? DVertexArity::edgeless

            : DVertexArity::unknown;
    };

NS_END  // internal

// vertex_arity_of
//   trait: the arity regime of N's vertex.
template<typename N>
struct vertex_arity_of
    : std::integral_constant<DVertexArity,
                             internal::vertex_arity_impl<N>::value>
{};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // vertex_arity_of_v
    //   variable template: value of vertex_arity_of<N>.
    template<typename N>
    D_CONSTEXPR DVertexArity vertex_arity_of_v =
        vertex_arity_of<N>::value;
#endif

// is_edgeless_node
//   trait: true if N is an edgeless (leaf) vertex — arity 0.
template<typename N>
struct is_edgeless_node
{
    static constexpr bool value =
        ( vertex_arity_of<N>::value == DVertexArity::edgeless );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_edgeless_node_v
    //   variable template: value of is_edgeless_node<N>.
    template<typename N>
    D_CONSTEXPR bool is_edgeless_node_v = is_edgeless_node<N>::value;
#endif

// is_fixed_arity_node
//   trait: true if N is a fixed-arity interior vertex — arity k >= 1.
template<typename N>
struct is_fixed_arity_node
{
    static constexpr bool value =
        ( vertex_arity_of<N>::value == DVertexArity::fixed );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_fixed_arity_node_v
    //   variable template: value of is_fixed_arity_node<N>.
    template<typename N>
    D_CONSTEXPR bool is_fixed_arity_node_v =
        is_fixed_arity_node<N>::value;
#endif

// is_nary_arity_node
//   trait: true if N is an n-ary interior vertex — unbounded arity.
template<typename N>
struct is_nary_arity_node
{
    static constexpr bool value =
        ( vertex_arity_of<N>::value == DVertexArity::n_ary );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_nary_arity_node_v
    //   variable template: value of is_nary_arity_node<N>.
    template<typename N>
    D_CONSTEXPR bool is_nary_arity_node_v =
        is_nary_arity_node<N>::value;
#endif

// is_hybrid_arity_node
//   trait: true if N is a hybrid interior vertex — k fixed + overflow.
template<typename N>
struct is_hybrid_arity_node
{
    static constexpr bool value =
        ( vertex_arity_of<N>::value == DVertexArity::hybrid );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_hybrid_arity_node_v
    //   variable template: value of is_hybrid_arity_node<N>.
    template<typename N>
    D_CONSTEXPR bool is_hybrid_arity_node_v =
        is_hybrid_arity_node<N>::value;
#endif

// is_interior_vertex
//   trait: true if N bears at least one edge — fixed, n-ary, or hybrid. The
// complement of is_edgeless_node over the classified regimes.
template<typename N>
struct is_interior_vertex
{
    static constexpr bool value =
        ( is_fixed_arity_node<N>::value ||
          is_nary_arity_node<N>::value  ||
          is_hybrid_arity_node<N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_interior_vertex_v
    //   variable template: value of is_interior_vertex<N>.
    template<typename N>
    D_CONSTEXPR bool is_interior_vertex_v = is_interior_vertex<N>::value;
#endif

// is_payloaded_vertex
//   trait: true if N's vertex carries a payload P — it exposes data access or
// a `value_type`. (monograph "Vertices": payloaded.)
template<typename N>
struct is_payloaded_vertex
{
    static constexpr bool value =
        ( has_data_access<N>::value ||
          has_value_type<N>::value );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_payloaded_vertex_v
    //   variable template: value of is_payloaded_vertex<N>.
    template<typename N>
    D_CONSTEXPR bool is_payloaded_vertex_v = is_payloaded_vertex<N>::value;
#endif

// is_payload_free_vertex
//   trait: true if N's vertex is purely structural — it carries no datum
// beyond identity and edges (P = 1). (monograph "Vertices": payload-free.)
template<typename N>
struct is_payload_free_vertex
{
    static constexpr bool value = !is_payloaded_vertex<N>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
    // is_payload_free_vertex_v
    //   variable template: value of is_payload_free_vertex<N>.
    template<typename N>
    D_CONSTEXPR bool is_payload_free_vertex_v =
        is_payload_free_vertex<N>::value;
#endif


NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_NODE_NODE_COMMON_HPP
