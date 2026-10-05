/*******************************************************************************
* djinterp [core]                                             graph_concepts.hpp
*
* Graph concepts:
*   C++20 concepts layered over graph_traits.hpp. These concepts provide
* readable constraints for graph-shaped containers without replacing the
* existing SFINAE trait surface.
*
*   The concepts mirror the verified public trait surface from
* graph_traits.hpp:
*   - graph identity and protocol detection
*   - direction / multiplicity / topology / storage classification
*   - weighted / propertied / hypergraph capability classification
*   - shorthand concepts over graph_class<T>
*
*
* path:      /inc/djinterp/core/container/graph/graph_concepts.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.29
*                                                            revised: 2026.10.01
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_GRAPH_GRAPH_CONCEPTS_HPP
#define DJINTERP_CONTAINER_GRAPH_GRAPH_CONCEPTS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

#ifndef __cplusplus
    #error "graph_concepts.hpp requires C++ compilation"
#endif

// djinterp
#include "graph_traits.hpp"


NS_DJINTERP
NS_CONTAINER

#if D_ENV_CPP_FEATURE_LANG_CONCEPTS

// ===========================================================================
// I.    Graph protocol concepts
// ===========================================================================

template<typename Type>
concept graph_type = traits::is_graph<Type>::value;

template<typename Type>
concept vertex_count_graph = traits::has_vertex_count_method_v<Type>;

template<typename Type>
concept edge_count_graph = traits::has_edge_count_method_v<Type>;

template<typename Type>
concept order_graph = traits::has_order_method_v<Type>;

template<typename Type>
concept magnitude_graph = traits::has_magnitude_method_v<Type>;

template<typename Type>
concept adjacency_graph = traits::has_adjacency_method<Type>::value;

template<typename Type>
concept mutable_vertex_graph = traits::has_add_vertex_method_v<Type>;

template<typename Type>
concept mutable_edge_graph = traits::has_add_edge_method<Type>::value;

template<typename Type>
concept removable_edge_graph = traits::has_remove_edge_method<Type>::value;

template<typename Type>
concept edge_query_graph = traits::has_has_edge_method<Type>::value;


// ===========================================================================
// II.   Direction concepts
// ===========================================================================

template<typename Type>
concept directed_graph_type = traits::is_directed_graph_t<Type>::value;

template<typename Type>
concept undirected_graph_type = traits::is_undirected_graph_t<Type>::value;

template<typename Type>
concept mixed_graph_type = traits::is_mixed_graph_t<Type>::value;

template<typename Type>
concept oriented_graph_type = traits::is_oriented_graph_t<Type>::value;

template<typename Type>
concept tournament_graph_type = traits::is_tournament_graph_t<Type>::value;

template<typename Type>
concept bidirected_graph_type = traits::is_bidirected_graph_t<Type>::value;


// ===========================================================================
// III.  Multiplicity / loops concepts
// ===========================================================================

template<typename Type>
concept simple_graph_type = traits::is_simple_graph_t<Type>::value;

template<typename Type>
concept multigraph_type = traits::is_multigraph_t<Type>::value;

template<typename Type>
concept pseudograph_type = traits::is_pseudograph_t<Type>::value;

template<typename Type>
concept self_loop_graph_type = traits::allows_self_loops_t<Type>::value;


// ===========================================================================
// IV.   Topology concepts
// ===========================================================================

template<typename Type>
concept dag_graph_type = traits::is_dag_t<Type>::value;

template<typename Type>
concept tree_graph_type = traits::is_tree_t<Type>::value;

template<typename Type>
concept forest_graph_type = traits::is_forest_t<Type>::value;


// ===========================================================================
// V.    Storage-strategy concepts
// ===========================================================================

template<typename Type>
concept adjacency_list_graph_type =
    traits::is_adjacency_list_storage_t<Type>::value;

template<typename Type>
concept adjacency_matrix_graph_type =
    traits::is_adjacency_matrix_storage_t<Type>::value;

template<typename Type>
concept edge_list_graph_type = traits::is_edge_list_storage_t<Type>::value;

template<typename Type>
concept csr_graph_type = traits::is_csr_storage_t<Type>::value;

template<typename Type>
concept csc_graph_type = traits::is_csc_storage_t<Type>::value;

template<typename Type>
concept incidence_matrix_graph_type =
    traits::is_incidence_matrix_storage_t<Type>::value;

template<typename Type>
concept node_based_graph_type = traits::is_node_based_storage_t<Type>::value;

template<typename Type>
concept implicit_graph_type = traits::is_implicit_graph_storage_t<Type>::value;

template<typename Type>
concept hyperedge_storage_graph_type =
    traits::is_hyperedge_storage_t<Type>::value;


// ===========================================================================
// VI.   Capability concepts
// ===========================================================================

template<typename Type>
concept weighted_graph_type = traits::is_weighted_graph_t<Type>::value;

template<typename Type>
concept vertex_propertied_graph_type =
    traits::is_vertex_propertied_t<Type>::value;

template<typename Type>
concept propertied_graph_type = traits::is_propertied_graph_t<Type>::value;

template<typename Type>
concept hypergraph_type = traits::is_hypergraph_t<Type>::value;

template<typename Type>
concept graph_property_graph = traits::has_graph_property_method_v<Type>;

template<typename Type>
concept in_degree_graph = traits::has_in_degree_method<Type>::value;

template<typename Type>
concept out_degree_graph = traits::has_out_degree_method<Type>::value;


// ===========================================================================
// VII.  Classification-based shorthand concepts
// ===========================================================================

template<typename Type>
concept classified_graph_type = traits::graph_class<Type>::is_graph_type;

template<typename Type>
concept classified_weighted_graph_type =
    traits::graph_class<Type>::is_weighted;

template<typename Type>
concept classified_propertied_graph_type =
    traits::graph_class<Type>::is_propertied;

template<typename Type>
concept classified_directed_graph_type =
    traits::graph_class<Type>::is_directed;

template<typename Type>
concept classified_tree_graph_type = traits::graph_class<Type>::is_tree;

template<typename Type>
concept classified_hypergraph_type = traits::graph_class<Type>::is_hypergraph;

#endif  // D_ENV_CPP_FEATURE_LANG_CONCEPTS


NS_END  // container
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_GRAPH_GRAPH_CONCEPTS_HPP
