/*******************************************************************************
* djinterp [core]                                               graph_traits.hpp
*
* Compile-time classification traits for graph-shaped containers.
*   Companion to `container_traits.hpp` and the rest of the
* `_container_traits.hpp` family under `meta/`.  Every detection here is
* purely structural SFINAE: types declare graphness through their
* members, not via tag types.
*
*   The traits cover six independent groups of detections:
*     1. Identification:   is the type a graph at all?
*     2. Direction:        directed / undirected / mixed / oriented
*     3. Multiplicity:     simple / multi / pseudograph
*     4. Topology:         tree / forest / dag / cyclic
*     5. Storage:          adjacency list / matrix / edge list /
*                          CSR / CSC / incidence / node-based /
*                          hyperedge / implicit / half-edge
*     6. Capabilities:     weighted / labeled / propertied / hyperedge
*
*   All detection operates on the `clean_t` (cv-ref-stripped) form of
* the type, follows the `static constexpr bool value` convention, and
* exposes a `_v` variable template alias on C++14 and higher.
*
* DETECTION PROBES (the structural protocol):
*   Methods:
*     - vertex_count() / edge_count() / order() / magnitude()
*     - add_vertex() / add_edge(...) / remove_edge(...)
*     - has_edge(u, v)  /  contains(v)
*     - adjacency(v)
*     - out_degree(v) / in_degree(v) / degree(v)
*     - vertex_property(v) / graph_property()
*   Type aliases:
*     - vertex_id_type            (any unsigned integer)
*     - edge_id_type              (any unsigned integer)
*     - vertex_descriptor_type    (typed wrapper)
*     - edge_descriptor_type      (typed wrapper)
*     - vertex_property_type      (vertex payload type)
*     - edge_property_type        (edge payload type)
*     - graph_property_type       (graph-level payload)
*     - adjacency_container_type  (per-vertex adjacency)
*     - vertex_container_type     (top-level vertex storage)
*     - adjacency_entry_type      (single adjacency record)
*   Static constants:
*     - direction          (graph_direction enum)
*     - multiplicity_kind  (edge_multiplicity enum)
*     - loops              (loop_policy enum)
*     - storage_strategy   (graph_storage enum)
*     - is_directed_graph / is_undirected_graph
*     - is_simple_graph / allows_parallel_edges / allows_self_loops
*     - is_weighted_graph / is_vertex_propertied
*
*
* path:      /inc/djinterp/core/container/graph/graph_traits.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.27
*                                                            revised: 2026.10.01
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Method Detection
      ----------------

II.   Type-alias Detection
      --------------------

III.  Static-constant Detection
      -------------------------

IV.   Identification (is_graph)
      -------------------------

V.    Direction Classification
      ------------------------

VI.   Multiplicity Classification
      ---------------------------

VII.  Topology Classification
      -----------------------

VIII. Storage Strategy Classification
      -------------------------------

IX.   Capability Classification (weighted / propertied / hyperedge)
      -------------------------------------------------------------

X.    Type Extractors
      ---------------

XI.   Combined Classification: graph_class<T>
      ---------------------------------------

XII.  Variable Templates (_v aliases)
      -------------------------------
*/

#ifndef DJINTERP_CONTAINER_GRAPH_GRAPH_TRAITS_HPP
#define DJINTERP_CONTAINER_GRAPH_GRAPH_TRAITS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <cstddef>
#include <type_traits>
#include <utility>
// djinterp
#include "../../../djinterp.hpp"
#include "../../meta/type_traits.hpp"
#include "graph_common.hpp"


NS_DJINTERP
NS_CONTAINER
NS_TRAITS


// ===========================================================================
// I.    METHOD DETECTION
// ===========================================================================

// has_vertex_count_method
//   trait: detects vertex_count() member function.
D_TYPE_TRAIT_TRUE(has_vertex_count_method,
    decltype(std::declval<const Type&>().vertex_count()))

// has_edge_count_method
//   trait: detects edge_count() member function.
D_TYPE_TRAIT_TRUE(has_edge_count_method,
    decltype(std::declval<const Type&>().edge_count()))

// has_order_method
//   trait: detects order() member function (graph theory: |V|).
D_TYPE_TRAIT_TRUE(has_order_method,
    decltype(std::declval<const Type&>().order()))

// has_magnitude_method
//   trait: detects magnitude() member function (graph theory: |E|).
D_TYPE_TRAIT_TRUE(has_magnitude_method,
    decltype(std::declval<const Type&>().magnitude()))

// has_add_vertex_method
//   trait: detects add_vertex() with no payload.
D_TYPE_TRAIT_TRUE(has_add_vertex_method,
    decltype(std::declval<Type&>().add_vertex()))

// has_add_edge_2_method
//   trait: detects 2-arg add_edge(u, v).
NS_INTERNAL

    template<typename Type, typename = void>
    struct has_add_edge_2_check : std::false_type
    {};

    template<typename Type>
    struct has_add_edge_2_check<Type, std::void_t<
        decltype(std::declval<Type&>().add_edge(
            std::declval<typename Type::vertex_descriptor_type>(),
            std::declval<typename Type::vertex_descriptor_type>()))>>
        : std::true_type
    {};

NS_END  // internal

template<typename Type>
struct has_add_edge_method : internal::has_add_edge_2_check<clean_t<Type>>
{};

// has_remove_edge_method
NS_INTERNAL

    template<typename Type, typename = void>
    struct has_remove_edge_check : std::false_type
    {};

    template<typename Type>
    struct has_remove_edge_check<Type, std::void_t<
        decltype(std::declval<Type&>().remove_edge(
            std::declval<typename Type::vertex_descriptor_type>(),
            std::declval<typename Type::vertex_descriptor_type>()))>>
        : std::true_type
    {};

NS_END  // internal

template<typename Type>
struct has_remove_edge_method : internal::has_remove_edge_check<clean_t<Type>>
{};

// has_has_edge_method
//   trait: detects has_edge(u, v) member.
NS_INTERNAL

    template<typename Type, typename = void>
    struct has_has_edge_check : std::false_type
    {};

    template<typename Type>
    struct has_has_edge_check<Type, std::void_t<
        decltype(std::declval<const Type&>().has_edge(
            std::declval<typename Type::vertex_descriptor_type>(),
            std::declval<typename Type::vertex_descriptor_type>()))>>
        : std::true_type
    {};

NS_END  // internal

template<typename Type>
struct has_has_edge_method : internal::has_has_edge_check<clean_t<Type>>
{};

// has_adjacency_method
//   trait: detects adjacency(v) member.
NS_INTERNAL

    template<typename Type, typename = void>
    struct has_adjacency_check : std::false_type
    {};

    template<typename Type>
    struct has_adjacency_check<Type, std::void_t<
        decltype(std::declval<const Type&>().adjacency(
            std::declval<typename Type::vertex_descriptor_type>()))>>
        : std::true_type
    {};

NS_END  // internal

template<typename Type>
struct has_adjacency_method : internal::has_adjacency_check<clean_t<Type>>
{};

D_TYPE_TRAIT_VALUE_BOOL(has_adjacency_method)

// has_out_degree_method
NS_INTERNAL

    template<typename Type, typename = void>
    struct has_out_degree_check : std::false_type
    {};

    template<typename Type>
    struct has_out_degree_check<Type, std::void_t<
        decltype(std::declval<const Type&>().out_degree(
            std::declval<typename Type::vertex_descriptor_type>()))>>
        : std::true_type
    {};

NS_END  // internal

template<typename Type>
struct has_out_degree_method : internal::has_out_degree_check<clean_t<Type>>
{};

// has_in_degree_method
NS_INTERNAL

    template<typename Type, typename = void>
    struct has_in_degree_check : std::false_type
    {};

    template<typename Type>
    struct has_in_degree_check<Type, std::void_t<
        decltype(std::declval<const Type&>().in_degree(
            std::declval<typename Type::vertex_descriptor_type>()))>>
        : std::true_type
    {};

NS_END  // internal

template<typename Type>
struct has_in_degree_method : internal::has_in_degree_check<clean_t<Type>>
{};

// has_graph_property_method
//   trait: detects graph_property() accessor.
D_TYPE_TRAIT_TRUE(has_graph_property_method,
    decltype(std::declval<const Type&>().graph_property()))


// ===========================================================================
// II.   TYPE-ALIAS DETECTION
// ===========================================================================

D_TYPE_TRAIT_TRUE(has_vertex_id_type_alias,
    typename Type::vertex_id_type)

D_TYPE_TRAIT_TRUE(has_edge_id_type_alias,
    typename Type::edge_id_type)

D_TYPE_TRAIT_TRUE(has_vertex_descriptor_type_alias,
    typename Type::vertex_descriptor_type)

D_TYPE_TRAIT_TRUE(has_edge_descriptor_type_alias,
    typename Type::edge_descriptor_type)

D_TYPE_TRAIT_TRUE(has_vertex_property_type_alias,
    typename Type::vertex_property_type)

D_TYPE_TRAIT_TRUE(has_edge_property_type_alias,
    typename Type::edge_property_type)

D_TYPE_TRAIT_TRUE(has_graph_property_type_alias,
    typename Type::graph_property_type)

D_TYPE_TRAIT_TRUE(has_adjacency_container_type_alias,
    typename Type::adjacency_container_type)

D_TYPE_TRAIT_TRUE(has_vertex_container_type_alias,
    typename Type::vertex_container_type)

D_TYPE_TRAIT_TRUE(has_adjacency_entry_type_alias,
    typename Type::adjacency_entry_type)


// ===========================================================================
// III.  STATIC-CONSTANT DETECTION
// ===========================================================================

D_TYPE_TRAIT_TRUE(has_direction_constant,
    decltype(Type::direction))

D_TYPE_TRAIT_TRUE(has_multiplicity_kind_constant,
    decltype(Type::multiplicity_kind))

D_TYPE_TRAIT_TRUE(has_loops_constant,
    decltype(Type::loops))

D_TYPE_TRAIT_TRUE(has_storage_strategy_constant,
    decltype(Type::storage_strategy))


// ===========================================================================
// IV.   IDENTIFICATION
// ===========================================================================

// is_graph
//   trait: true if Type satisfies the structural minimum to be a djinterp
// graph. Minimum = `vertex_id_type` alias + (some way to count vertices) +
// (some way to count edges) + (some way to walk neighbors).
template<typename Type>
struct is_graph
{
    using clean_type = clean_t<Type>;

    static D_CONSTEXPR bool value =
        ( has_vertex_id_type_alias_v<clean_type>          &&
          ( has_vertex_count_method_v<clean_type>  ||
            has_order_method_v<clean_type> )              &&
          ( has_edge_count_method_v<clean_type>    ||
            has_magnitude_method_v<clean_type> )          &&
          has_adjacency_method_v<clean_type> );
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type>
inline D_CONSTEXPR bool is_graph_v = is_graph<Type>::value;
#endif


// ===========================================================================
// V.    DIRECTION CLASSIFICATION
// ===========================================================================

NS_INTERNAL

    // direction_of_helper
    //   helper: extracts the `direction` static constant if present; falls
    // back to graph_direction::unspecified.
    template<typename Type, typename = void>
    struct direction_of_helper
    {
        static D_CONSTEXPR graph_direction value =
            graph_direction::unspecified;
    };

    template<typename Type>
    struct direction_of_helper<Type, std::void_t<decltype(Type::direction)>>
    {
        static D_CONSTEXPR graph_direction value = Type::direction;
    };

NS_END  // internal

// graph_direction_of
//   trait: returns the `graph_direction` of Type, or `unspecified`.
template<typename Type>
struct graph_direction_of
{
    static D_CONSTEXPR graph_direction value =
        internal::direction_of_helper<clean_t<Type>>::value;
};

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES
template<typename Type>
inline D_CONSTEXPR graph_direction graph_direction_of_v =
    graph_direction_of<Type>::value;
#endif

// is_directed_graph_t
//   trait: true if the graph carries any directed-edge semantics.
template<typename Type>
struct is_directed_graph_t
{
private:
    static D_CONSTEXPR graph_direction d = graph_direction_of<Type>::value;

public:
    static D_CONSTEXPR bool value =
        ( (d == graph_direction::directed)   ||
          (d == graph_direction::oriented)   ||
          (d == graph_direction::tournament) ||
          (d == graph_direction::bidirected) );
};

// is_undirected_graph_t
template<typename Type>
struct is_undirected_graph_t
{
    static D_CONSTEXPR bool value =
        (graph_direction_of<Type>::value == graph_direction::undirected);
};

// is_oriented_graph_t
template<typename Type>
struct is_oriented_graph_t
{
    static D_CONSTEXPR bool value =
        (graph_direction_of<Type>::value == graph_direction::oriented);
};

// is_tournament_graph_t
template<typename Type>
struct is_tournament_graph_t
{
    static D_CONSTEXPR bool value =
        (graph_direction_of<Type>::value == graph_direction::tournament);
};

// is_bidirected_graph_t
template<typename Type>
struct is_bidirected_graph_t
{
    static D_CONSTEXPR bool value =
        (graph_direction_of<Type>::value == graph_direction::bidirected);
};

// is_mixed_graph_t
template<typename Type>
struct is_mixed_graph_t
{
    static D_CONSTEXPR bool value =
        (graph_direction_of<Type>::value == graph_direction::mixed);
};


// ===========================================================================
// VI.   MULTIPLICITY CLASSIFICATION
// ===========================================================================

NS_INTERNAL

    template<typename Type, typename = void>
    struct multiplicity_of_helper
    {
        static D_CONSTEXPR edge_multiplicity value =
            edge_multiplicity::unspecified;
    };

    template<typename Type>
    struct multiplicity_of_helper<Type,
        std::void_t<decltype(Type::multiplicity_kind)>>
    {
        static D_CONSTEXPR edge_multiplicity value = Type::multiplicity_kind;
    };

NS_END  // internal

// graph_multiplicity_of
//   trait: returns the `edge_multiplicity` of Type.
template<typename Type>
struct graph_multiplicity_of
{
    static D_CONSTEXPR edge_multiplicity value =
        internal::multiplicity_of_helper<clean_t<Type>>::value;
};

// is_simple_graph_t
template<typename Type>
struct is_simple_graph_t
{
    static D_CONSTEXPR bool value =
        (graph_multiplicity_of<Type>::value == edge_multiplicity::simple);
};

// is_multigraph_t
template<typename Type>
struct is_multigraph_t
{
    static D_CONSTEXPR bool value =
        ( (graph_multiplicity_of<Type>::value == edge_multiplicity::multi) ||
          (graph_multiplicity_of<Type>::value == edge_multiplicity::bounded) );
};

// loops_of
//   trait: returns the `loop_policy` of Type.
NS_INTERNAL

    template<typename Type, typename = void>
    struct loops_of_helper
    {
        static D_CONSTEXPR loop_policy value = loop_policy::unknown;
    };

    template<typename Type>
    struct loops_of_helper<Type, std::void_t<decltype(Type::loops)>>
    {
        static D_CONSTEXPR loop_policy value = Type::loops;
    };

NS_END  // internal

template<typename Type>
struct loops_of
{
    static D_CONSTEXPR loop_policy value =
        internal::loops_of_helper<clean_t<Type>>::value;
};

// allows_self_loops_t
template<typename Type>
struct allows_self_loops_t
{
    static D_CONSTEXPR bool value =
        ( (loops_of<Type>::value == loop_policy::allow) ||
          (loops_of<Type>::value == loop_policy::require) );
};

// is_pseudograph_t
//   trait: undirected + simple + self-loops allowed.
template<typename Type>
struct is_pseudograph_t
{
    static D_CONSTEXPR bool value =
        ( is_undirected_graph_t<Type>::value &&
          is_simple_graph_t<Type>::value     &&
          allows_self_loops_t<Type>::value );
};


// ===========================================================================
// VII.  TOPOLOGY CLASSIFICATION
// ===========================================================================
// These traits are STRUCTURAL only -- they detect *declared* topology
// invariants (e.g. types that expose `acyclicity = graph_acyclicity::dag`
// as a static constant).  Run-time detection of acyclicity / connectivity
// requires actual algorithms and is out of scope for the trait system.

NS_INTERNAL

    template<typename Type, typename = void>
    struct acyclicity_of_helper
    {
        static D_CONSTEXPR graph_acyclicity value =
            graph_acyclicity::unknown;
    };

    template<typename Type>
    struct acyclicity_of_helper<Type,
        std::void_t<decltype(Type::acyclicity)>>
    {
        static D_CONSTEXPR graph_acyclicity value = Type::acyclicity;
    };

    template<typename Type, typename = void>
    struct connectivity_of_helper
    {
        static D_CONSTEXPR graph_connectivity value =
            graph_connectivity::unknown;
    };

    template<typename Type>
    struct connectivity_of_helper<Type,
        std::void_t<decltype(Type::connectivity)>>
    {
        static D_CONSTEXPR graph_connectivity value = Type::connectivity;
    };

    template<typename Type, typename = void>
    struct density_of_helper
    {
        static D_CONSTEXPR graph_density value = graph_density::unknown;
    };

    template<typename Type>
    struct density_of_helper<Type, std::void_t<decltype(Type::density)>>
    {
        static D_CONSTEXPR graph_density value = Type::density;
    };

    template<typename Type, typename = void>
    struct planarity_of_helper
    {
        static D_CONSTEXPR graph_planarity value =
            graph_planarity::unknown;
    };

    template<typename Type>
    struct planarity_of_helper<Type, std::void_t<decltype(Type::planarity)>>
    {
        static D_CONSTEXPR graph_planarity value = Type::planarity;
    };

NS_END  // internal

template<typename Type>
struct graph_acyclicity_of
{
    static D_CONSTEXPR graph_acyclicity value =
        internal::acyclicity_of_helper<clean_t<Type>>::value;
};

template<typename Type>
struct graph_connectivity_of
{
    static D_CONSTEXPR graph_connectivity value =
        internal::connectivity_of_helper<clean_t<Type>>::value;
};

template<typename Type>
struct graph_density_of
{
    static D_CONSTEXPR graph_density value =
        internal::density_of_helper<clean_t<Type>>::value;
};

template<typename Type>
struct graph_planarity_of
{
    static D_CONSTEXPR graph_planarity value =
        internal::planarity_of_helper<clean_t<Type>>::value;
};

// is_dag_t
template<typename Type>
struct is_dag_t
{
    static D_CONSTEXPR bool value =
        (graph_acyclicity_of<Type>::value == graph_acyclicity::dag);
};

// is_tree_t
template<typename Type>
struct is_tree_t
{
    static D_CONSTEXPR bool value =
        ( (graph_acyclicity_of<Type>::value == graph_acyclicity::tree) ||
          (graph_acyclicity_of<Type>::value == graph_acyclicity::rooted_tree) );
};

// is_forest_t
template<typename Type>
struct is_forest_t
{
    static D_CONSTEXPR bool value =
        (graph_acyclicity_of<Type>::value == graph_acyclicity::forest);
};


// ===========================================================================
// VIII. STORAGE STRATEGY CLASSIFICATION
// ===========================================================================

NS_INTERNAL

    template<typename Type, typename = void>
    struct storage_of_helper
    {
        static D_CONSTEXPR graph_storage value = graph_storage::unspecified;
    };

    template<typename Type>
    struct storage_of_helper<Type,
        std::void_t<decltype(Type::storage_strategy)>>
    {
        static D_CONSTEXPR graph_storage value = Type::storage_strategy;
    };

NS_END  // internal

template<typename Type>
struct graph_storage_of
{
    static D_CONSTEXPR graph_storage value =
        internal::storage_of_helper<clean_t<Type>>::value;
};

// is_adjacency_list_storage_t
template<typename Type>
struct is_adjacency_list_storage_t
{
    static D_CONSTEXPR bool value =
        (graph_storage_of<Type>::value == graph_storage::adjacency_list);
};

// is_adjacency_matrix_storage_t
template<typename Type>
struct is_adjacency_matrix_storage_t
{
    static D_CONSTEXPR bool value =
        (graph_storage_of<Type>::value == graph_storage::adjacency_matrix);
};

// is_edge_list_storage_t
template<typename Type>
struct is_edge_list_storage_t
{
    static D_CONSTEXPR bool value =
        (graph_storage_of<Type>::value == graph_storage::edge_list);
};

// is_csr_storage_t
template<typename Type>
struct is_csr_storage_t
{
    static D_CONSTEXPR bool value =
        (graph_storage_of<Type>::value == graph_storage::csr);
};

// is_csc_storage_t
template<typename Type>
struct is_csc_storage_t
{
    static D_CONSTEXPR bool value =
        (graph_storage_of<Type>::value == graph_storage::csc);
};

// is_incidence_matrix_storage_t
template<typename Type>
struct is_incidence_matrix_storage_t
{
    static D_CONSTEXPR bool value =
        (graph_storage_of<Type>::value == graph_storage::incidence_matrix);
};

// is_node_based_storage_t
template<typename Type>
struct is_node_based_storage_t
{
    static D_CONSTEXPR bool value =
        (graph_storage_of<Type>::value == graph_storage::node_based);
};

// is_hyperedge_storage_t
template<typename Type>
struct is_hyperedge_storage_t
{
    static D_CONSTEXPR bool value =
        (graph_storage_of<Type>::value == graph_storage::hyperedge_list);
};

// is_implicit_graph_storage_t
template<typename Type>
struct is_implicit_graph_storage_t
{
    static D_CONSTEXPR bool value =
        (graph_storage_of<Type>::value == graph_storage::implicit);
};


// ===========================================================================
// IX.   CAPABILITY CLASSIFICATION
// ===========================================================================

NS_INTERNAL

    template<typename Type, typename = void>
    struct edge_property_extract
    {
        using type = void;
    };

    template<typename Type>
    struct edge_property_extract<Type,
        std::void_t<typename Type::edge_property_type>>
    {
        using type = typename Type::edge_property_type;
    };

    template<typename Type, typename = void>
    struct vertex_property_extract
    {
        using type = void;
    };

    template<typename Type>
    struct vertex_property_extract<Type,
        std::void_t<typename Type::vertex_property_type>>
    {
        using type = typename Type::vertex_property_type;
    };

NS_END  // internal

// is_weighted_graph_t
//   trait: true if `Type::edge_property_type` is non-void and not
// `no_property`. This catches both arithmetic weights (double, int) and
// user-defined edge property structs.
template<typename Type>
struct is_weighted_graph_t
{
private:
    using clean_type = clean_t<Type>;
    using ep         = typename internal::edge_property_extract<clean_type>::type;

public:
    static D_CONSTEXPR bool value =
        ( !std::is_same<ep, void>::value &&
          !std::is_same<ep, no_property>::value );
};

// is_vertex_propertied_t
template<typename Type>
struct is_vertex_propertied_t
{
private:
    using clean_type = clean_t<Type>;
    using vp         = typename internal::vertex_property_extract<clean_type>::type;

public:
    static D_CONSTEXPR bool value =
        ( !std::is_same<vp, void>::value &&
          !std::is_same<vp, no_property>::value );
};

// is_propertied_graph_t
//   trait: true if either vertices or edges carry a non-trivial property.
template<typename Type>
struct is_propertied_graph_t
{
    static D_CONSTEXPR bool value =
        ( is_weighted_graph_t<Type>::value ||
          is_vertex_propertied_t<Type>::value );
};

// is_hypergraph_t
//   trait: true if the graph stores hyperedges (storage strategy is
// hyperedge_list).
template<typename Type>
struct is_hypergraph_t
{
    static D_CONSTEXPR bool value =
        is_hyperedge_storage_t<Type>::value;
};


// ===========================================================================
// X.    TYPE EXTRACTORS
// ===========================================================================

// vertex_id_type_of_t
template<typename Type>
using vertex_id_type_of_t = typename clean_t<Type>::vertex_id_type;

// edge_id_type_of_t
NS_INTERNAL

    template<typename Type, typename = void>
    struct edge_id_extract
    {
        using type = void;
    };

    template<typename Type>
    struct edge_id_extract<Type, std::void_t<typename Type::edge_id_type>>
    {
        using type = typename Type::edge_id_type;
    };

NS_END  // internal

template<typename Type>
using edge_id_type_of_t = typename internal::edge_id_extract<clean_t<Type>>::type;

// vertex_property_type_of_t
template<typename Type>
using vertex_property_type_of_t =
    typename internal::vertex_property_extract<clean_t<Type>>::type;

// edge_property_type_of_t
template<typename Type>
using edge_property_type_of_t =
    typename internal::edge_property_extract<clean_t<Type>>::type;

// adjacency_container_type_of_t
NS_INTERNAL

    template<typename Type, typename = void>
    struct adj_container_extract
    {
        using type = void;
    };

    template<typename Type>
    struct adj_container_extract<Type,
        std::void_t<typename Type::adjacency_container_type>>
    {
        using type = typename Type::adjacency_container_type;
    };

NS_END  // internal

template<typename Type>
using adjacency_container_type_of_t =
    typename internal::adj_container_extract<clean_t<Type>>::type;


// ===========================================================================
// XI.   COMBINED CLASSIFICATION: graph_class<T>
// ===========================================================================

// graph_class
//   struct: aggregate compile-time classification of a graph type across all
// axes detected by this header. Members are `static constexpr` so the entire
// struct can be queried at compile time and pattern-matched in `if constexpr`
// chains.
template<typename Type>
struct graph_class
{
    using clean_type = clean_t<Type>;

    // --- Identification ---
    static D_CONSTEXPR bool is_graph_type =
        is_graph<clean_type>::value;

    // --- Direction ---
    static D_CONSTEXPR graph_direction direction =
        graph_direction_of<clean_type>::value;

    static D_CONSTEXPR bool is_directed   = is_directed_graph_t<clean_type>::value;
    static D_CONSTEXPR bool is_undirected = is_undirected_graph_t<clean_type>::value;
    static D_CONSTEXPR bool is_mixed      = is_mixed_graph_t<clean_type>::value;
    static D_CONSTEXPR bool is_oriented   = is_oriented_graph_t<clean_type>::value;
    static D_CONSTEXPR bool is_tournament = is_tournament_graph_t<clean_type>::value;
    static D_CONSTEXPR bool is_bidirected = is_bidirected_graph_t<clean_type>::value;

    // --- Multiplicity / loops ---
    static D_CONSTEXPR edge_multiplicity multiplicity =
        graph_multiplicity_of<clean_type>::value;

    static D_CONSTEXPR bool is_simple        = is_simple_graph_t<clean_type>::value;
    static D_CONSTEXPR bool is_multigraph    = is_multigraph_t<clean_type>::value;

    static D_CONSTEXPR loop_policy loops =
        loops_of<clean_type>::value;
    static D_CONSTEXPR bool allows_self_loops = allows_self_loops_t<clean_type>::value;

    static D_CONSTEXPR bool is_pseudograph    = is_pseudograph_t<clean_type>::value;

    // --- Topology ---
    static D_CONSTEXPR graph_acyclicity acyclicity =
        graph_acyclicity_of<clean_type>::value;
    static D_CONSTEXPR graph_connectivity connectivity =
        graph_connectivity_of<clean_type>::value;
    static D_CONSTEXPR graph_density density =
        graph_density_of<clean_type>::value;
    static D_CONSTEXPR graph_planarity planarity =
        graph_planarity_of<clean_type>::value;

    static D_CONSTEXPR bool is_dag    = is_dag_t<clean_type>::value;
    static D_CONSTEXPR bool is_tree   = is_tree_t<clean_type>::value;
    static D_CONSTEXPR bool is_forest = is_forest_t<clean_type>::value;

    // --- Storage ---
    static D_CONSTEXPR graph_storage storage =
        graph_storage_of<clean_type>::value;

    static D_CONSTEXPR bool uses_adjacency_list   = is_adjacency_list_storage_t<clean_type>::value;
    static D_CONSTEXPR bool uses_adjacency_matrix = is_adjacency_matrix_storage_t<clean_type>::value;
    static D_CONSTEXPR bool uses_edge_list        = is_edge_list_storage_t<clean_type>::value;
    static D_CONSTEXPR bool uses_csr              = is_csr_storage_t<clean_type>::value;
    static D_CONSTEXPR bool uses_csc              = is_csc_storage_t<clean_type>::value;
    static D_CONSTEXPR bool uses_incidence_matrix = is_incidence_matrix_storage_t<clean_type>::value;
    static D_CONSTEXPR bool uses_node_based       = is_node_based_storage_t<clean_type>::value;
    static D_CONSTEXPR bool is_hypergraph         = is_hypergraph_t<clean_type>::value;
    static D_CONSTEXPR bool is_implicit           = is_implicit_graph_storage_t<clean_type>::value;

    // --- Capabilities ---
    static D_CONSTEXPR bool is_weighted          = is_weighted_graph_t<clean_type>::value;
    static D_CONSTEXPR bool is_vertex_propertied = is_vertex_propertied_t<clean_type>::value;
    static D_CONSTEXPR bool is_propertied        = is_propertied_graph_t<clean_type>::value;

    // --- Method-level capabilities (informational) ---
    static D_CONSTEXPR bool can_add_vertex   = has_add_vertex_method_v<clean_type>;
    static D_CONSTEXPR bool can_add_edge     = has_add_edge_method<clean_type>::value;
    static D_CONSTEXPR bool can_remove_edge  = has_remove_edge_method<clean_type>::value;
    static D_CONSTEXPR bool can_query_edge   = has_has_edge_method<clean_type>::value;
    static D_CONSTEXPR bool can_query_in_degree  = has_in_degree_method<clean_type>::value;
    static D_CONSTEXPR bool can_query_out_degree = has_out_degree_method<clean_type>::value;
    static D_CONSTEXPR bool has_graph_property   = has_graph_property_method_v<clean_type>;
};


// ===========================================================================
// XII.  VARIABLE TEMPLATES (_v ALIASES)
// ===========================================================================

#if D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES

    template<typename Type>
    inline D_CONSTEXPR bool is_directed_graph_v =
        is_directed_graph_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_undirected_graph_v =
        is_undirected_graph_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_mixed_graph_v =
        is_mixed_graph_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_oriented_graph_v =
        is_oriented_graph_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_tournament_graph_v =
        is_tournament_graph_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_bidirected_graph_v =
        is_bidirected_graph_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_simple_graph_v =
        is_simple_graph_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_multigraph_v =
        is_multigraph_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_pseudograph_v =
        is_pseudograph_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool allows_self_loops_v =
        allows_self_loops_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_dag_v =
        is_dag_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_tree_v =
        is_tree_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_forest_v =
        is_forest_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_weighted_graph_v =
        is_weighted_graph_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_vertex_propertied_v =
        is_vertex_propertied_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_propertied_graph_v =
        is_propertied_graph_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_hypergraph_v =
        is_hypergraph_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_adjacency_list_storage_v =
        is_adjacency_list_storage_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_adjacency_matrix_storage_v =
        is_adjacency_matrix_storage_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_edge_list_storage_v =
        is_edge_list_storage_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_csr_storage_v =
        is_csr_storage_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_csc_storage_v =
        is_csc_storage_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_incidence_matrix_storage_v =
        is_incidence_matrix_storage_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_node_based_storage_v =
        is_node_based_storage_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR bool is_implicit_graph_storage_v =
        is_implicit_graph_storage_t<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR graph_direction graph_direction_v =
        graph_direction_of<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR edge_multiplicity graph_multiplicity_v =
        graph_multiplicity_of<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR graph_storage graph_storage_v =
        graph_storage_of<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR graph_acyclicity graph_acyclicity_v =
        graph_acyclicity_of<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR graph_connectivity graph_connectivity_v =
        graph_connectivity_of<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR graph_density graph_density_v =
        graph_density_of<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR graph_planarity graph_planarity_v =
        graph_planarity_of<Type>::value;

    template<typename Type>
    inline D_CONSTEXPR loop_policy graph_loops_v =
        loops_of<Type>::value;

#endif  // D_ENV_CPP_FEATURE_LANG_VARIABLE_TEMPLATES


NS_END  // traits
NS_END  // container
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_GRAPH_GRAPH_TRAITS_HPP
