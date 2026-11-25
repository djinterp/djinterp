/*******************************************************************************
* djinterp [cpp][container]                                     graph_common.hpp
*
* Foundational graph types - enums, descriptors, edge primitives, storage
* policies, traversal tags, and sentinels.
*   Sits below `graph.hpp` and `graph_traits.hpp`; both files consume the
* vocabulary defined here.  Knows nothing about the storage layout of the
* graph itself; only describes properties.
*
*   Most of the property / classification enums refine concepts already
* declared in meta's `math_vocabulary.hpp` - e.g., `DGraphDirection` reuses
* `DDirectionality`, `DGraphAcyclicity` is a refinement of the
* acyclic-relation property.  Consumers that want the most general form
* should use those enums directly; `graph_*` enums add finer
* graph-only categories (e.g. tournament, DAG, tree, forest).
*
* DESIGN PRINCIPLES:
*   1. Open-ended: every storage strategy, every directionality, every
*      multiplicity is enumerable; the user chooses.
*   2. Zero overhead: every enum is a small integer, every descriptor
*      is a typed wrapper around an unsigned integer, every policy is
*      a tag struct of type aliases and `static constexpr` flags.
*   3. Containers as storage: the adjacency container per vertex is
*      a template parameter of `DGraph<>`, so any djinterp container --
*      sequential, sorted, hashed, threadsafe, even another graph --
*      may serve as the adjacency list.
*   4. C++11 baseline; feature-gated extensions for >= C++14/17/20.
*
*
* path:      /inc/djinterp/cpp/container/graph/graph_common.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.27
*                                                            revised: 2026.11.25
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    Vertex / Edge Identifier Types
      ------------------------------
      1.    invalid_id, is_invalid_id; depth_t, unknown_depth
      2.    Default ID type
      3.    vertex_descriptor / edge_descriptor (typed wrappers)
      4.    null_vertex / null_edge sentinels

II.   Graph Direction
      ---------------
      1.    graph_direction enum (refines DDirectionality)
      2.    helpers: is_directed_v, is_undirected_v

III.  Edge Multiplicity / Self-Loop Policy
      ------------------------------------
      1.    edge_multiplicity enum
      2.    loop_policy enum (re-export from math)

IV.   Storage Strategy
      ----------------
      1.    graph_storage enum (adjacency_list, matrix, edge_list,

      csr, csc, incidence, node_based, hyperedge_list)

V.    Density / Acyclicity / Connectivity
      -----------------------------------
      1.    graph_density
      2.    graph_acyclicity
      3.    graph_connectivity
      4.    graph_planarity

VI.   Edge Primitives
      ---------------
      1.    unweighted_edge<V>
      2.    weighted_edge<V, W>
      3.    labeled_edge<V, L>
      4.    property_edge<V, P>
      5.    hyperedge<V, N>
      6.    typed_edge<V, ...>

VII.  Property / Weight Policies
      --------------------------
      1.    no_property
      2.    weight_policy<W>
      3.    property_policy<P>

VIII. Traversal Order Tags
      --------------------
      1.    bfs_order_tag, dfs_order_tag,

      DPreOrderTag, DPostOrderTag,

      DLevelOrderTag, leaf_only_tag,

      topological_order_tag, dijkstra_order_tag,

      astar_order_tag, bellman_ford_order_tag

IX.   Adjacency Container Concept Helpers
      -----------------------------------
      1.    adjacency_value_type_of_t
      2.    default_adjacency_container_t
*/

#ifndef DJINTERP_CONTAINER_GRAPH_GRAPH_COMMON_HPP
#define DJINTERP_CONTAINER_GRAPH_GRAPH_COMMON_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <array>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"  // framework root
#include "../../meta/math_vocabulary.hpp"  // DDirectionality, DSelfLoopPolicy
// re_std
#include "../../../../re_std/cpp/cstdint/cstdint.hpp"  // re_std::uint8_t, uint32_t


NS_DJINTERP
NS_CONTAINER

    // =========================================================================
    // I.   VERTEX / EDGE IDENTIFIER TYPES
    // =========================================================================

    // invalid_id
    //   function (template): the canonical sentinel for an unsigned-integer
    // ID type: always (UInt)-1. (In djinterp::math, in math_common.hpp, until
    // 2026.11.11: the name is graph's alone.)
    template<typename UInt>
    D_CONSTEXPR_INLINE
    UInt invalid_id() noexcept
    {
        static_assert(std::is_unsigned<UInt>::value,
                      "invalid_id<T>() requires an unsigned integer type.");

        return static_cast<UInt>(-1);
    }

    // is_invalid_id
    //   function (template): true if `_id` equals the sentinel returned by
    // `invalid_id<T>()`.
    template<typename UInt>
    D_CONSTEXPR_INLINE
    bool is_invalid_id(
        UInt _id
    ) noexcept
    {
        return (_id == invalid_id<UInt>());
    }

    // depth_t
    //   type: the depth of a vertex below the root - the number of edges on
    // the path from root() to it.
    using depth_t = std::size_t;

    // unknown_depth
    //   constant: the depth reported for a vertex that root() does not reach.
    // (math::kUnknownDepth until 2026.11.11: the guide names constexpr values
    // in snake_case.)
    static D_CONSTEXPR depth_t unknown_depth =
        static_cast<depth_t>(-1);

    // default_graph_id_t
    //   type: default unsigned integer used to identify vertices and edges
    // when the user does not specify otherwise. re_std::uint32_t hits the sweet
    // spot of "big enough for almost any single-machine graph" while halving
    // the cost vs. std::size_t on 64-bit targets.
    using default_graph_id_t = re_std::uint32_t;


    // DVertexDescriptor
    //   class: a typed wrapper around an unsigned-integer vertex ID. The
    // wrapping prevents accidental cross-pollination of vertex IDs and edge
    // IDs at the type system level. Comparison and hashing are provided so the
    // descriptor can be used as a key in std::map / std::unordered_map.
    template<typename IdType = default_graph_id_t>
    class DVertexDescriptor
    {
    public:
        using id_type = IdType;

        static_assert(std::is_unsigned<id_type>::value,
                      "vertex_descriptor id_type must be unsigned.");

        D_CONSTEXPR
        DVertexDescriptor() noexcept
            : m_id(djinterp::container::invalid_id<id_type>())
        {}

        D_CONSTEXPR explicit
        DVertexDescriptor(id_type _id) noexcept
            : m_id(_id)
        {}

        D_CONSTEXPR id_type id() const noexcept
        {
            return m_id;
        }

        D_CONSTEXPR bool is_null() const noexcept
        {
            return djinterp::container::is_invalid_id(m_id);
        }

        D_CONSTEXPR explicit operator id_type() const noexcept
        {
            return m_id;
        }

        D_CONSTEXPR friend bool operator==(DVertexDescriptor _a,
                                           DVertexDescriptor _b) noexcept
        {
            return _a.m_id == _b.m_id;
        }

        D_CONSTEXPR friend bool operator!=(DVertexDescriptor _a,
                                           DVertexDescriptor _b) noexcept
        {
            return _a.m_id != _b.m_id;
        }

        D_CONSTEXPR friend bool operator<(DVertexDescriptor _a,
                                          DVertexDescriptor _b) noexcept
        {
            return _a.m_id < _b.m_id;
        }

        D_CONSTEXPR friend bool operator<=(DVertexDescriptor _a,
                                           DVertexDescriptor _b) noexcept
        {
            return _a.m_id <= _b.m_id;
        }

        D_CONSTEXPR friend bool operator>(DVertexDescriptor _a,
                                          DVertexDescriptor _b) noexcept
        {
            return _a.m_id > _b.m_id;
        }

        D_CONSTEXPR friend bool operator>=(DVertexDescriptor _a,
                                           DVertexDescriptor _b) noexcept
        {
            return _a.m_id >= _b.m_id;
        }

    private:
        id_type m_id;
    };

    // old name, until 0.9.0-rc.1
    template<typename IdType = default_graph_id_t>
    using vertex_descriptor = DVertexDescriptor<IdType>;


    // DEdgeDescriptor
    //   class: typed wrapper around an unsigned-integer edge ID, mirror of
    // vertex_descriptor.
    template<typename IdType = default_graph_id_t>
    class DEdgeDescriptor
    {
    public:
        using id_type = IdType;

        static_assert(std::is_unsigned<id_type>::value,
                      "edge_descriptor id_type must be unsigned.");

        D_CONSTEXPR
        DEdgeDescriptor() noexcept
            : m_id(djinterp::container::invalid_id<id_type>())
        {}

        D_CONSTEXPR explicit
        DEdgeDescriptor(id_type _id) noexcept
            : m_id(_id)
        {}

        D_CONSTEXPR id_type id() const noexcept
        {
            return m_id;
        }

        D_CONSTEXPR bool is_null() const noexcept
        {
            return djinterp::container::is_invalid_id(m_id);
        }

        D_CONSTEXPR explicit operator id_type() const noexcept
        {
            return m_id;
        }

        D_CONSTEXPR friend bool operator==(DEdgeDescriptor _a,
                                           DEdgeDescriptor _b) noexcept
        {
            return _a.m_id == _b.m_id;
        }

        D_CONSTEXPR friend bool operator!=(DEdgeDescriptor _a,
                                           DEdgeDescriptor _b) noexcept
        {
            return _a.m_id != _b.m_id;
        }

        D_CONSTEXPR friend bool operator<(DEdgeDescriptor _a,
                                          DEdgeDescriptor _b) noexcept
        {
            return _a.m_id < _b.m_id;
        }

    private:
        id_type m_id;
    };

    // old name, until 0.9.0-rc.1
    template<typename IdType = default_graph_id_t>
    using edge_descriptor = DEdgeDescriptor<IdType>;


    // null_vertex
    //   function: returns the sentinel vertex_descriptor for the given ID
    // type. Equivalent to a default-constructed descriptor.
    template<typename IdType = default_graph_id_t>
    D_CONSTEXPR_INLINE DVertexDescriptor<IdType> null_vertex() noexcept
    {
        return DVertexDescriptor<IdType>();
    }

    // null_edge
    //   function: returns the sentinel edge_descriptor for the given ID type.
    template<typename IdType = default_graph_id_t>
    D_CONSTEXPR_INLINE DEdgeDescriptor<IdType> null_edge() noexcept
    {
        return DEdgeDescriptor<IdType>();
    }


    // =========================================================================
    // II.  GRAPH DIRECTION
    // =========================================================================

    // DGraphDirection
    //   enum: graph-wide direction policy. This is a re-spelling of
    // DDirectionality with names tuned for graph theory; the values are
    // kept in sync via the conversion helpers below.
    enum class DGraphDirection : re_std::uint8_t
    {
        // unspecified / heterogeneous; treat per-edge
        unspecified = 0,

        // every edge is unordered (graph)
        undirected  = 1,

        // every edge is ordered (digraph)
        directed    = 2,

        // mix of directed and undirected edges (mixed graph)
        mixed       = 3,

        // directed AND no edge has its reverse counterpart (oriented graph)
        oriented    = 4,

        // every pair of distinct vertices has exactly one directed edge
        // between them (tournament)
        tournament  = 5,

        // for every (u, v) edge, (v, u) also exists; equivalent to
        // undirected for many algorithms but stored as digraph
        bidirected  = 6
    };

    // old name, until 0.9.0-rc.1
    using graph_direction = DGraphDirection;

    // to_math_directionality
    //   function: convert graph_direction into DDirectionality.
    D_CONSTEXPR_INLINE
    djinterp::DDirectionality to_math_directionality(
        DGraphDirection _d
    ) noexcept
    {
        return ( (_d == DGraphDirection::undirected) ? djinterp::DDirectionality::undirected :
                 (_d == DGraphDirection::directed)   ? djinterp::DDirectionality::directed   :
                 (_d == DGraphDirection::mixed)      ? djinterp::DDirectionality::mixed      :
                 (_d == DGraphDirection::oriented)   ? djinterp::DDirectionality::oriented   :
                 (_d == DGraphDirection::tournament) ? djinterp::DDirectionality::tournament :
                 (_d == DGraphDirection::bidirected) ? djinterp::DDirectionality::bidirected :
                                                       djinterp::DDirectionality::unknown );
    }


    // =========================================================================
    // III. EDGE MULTIPLICITY / SELF-LOOP POLICY
    // =========================================================================

    // DEdgeMultiplicity
    //   enum: graph-wide policy on duplicate edges between the same (u, v)
    // pair. Refines meta's DMultiplicityKind in graph terms.
    enum class DEdgeMultiplicity : re_std::uint8_t
    {
        unspecified = 0,

        // at most one edge per ordered/unordered pair (simple graph)
        simple      = 1,

        // duplicate edges allowed (multigraph)
        multi       = 2,

        // duplicates allowed up to a fixed bound (k-multigraph)
        bounded     = 3
    };

    // old name, until 0.9.0-rc.1
    using edge_multiplicity = DEdgeMultiplicity;

    // DLoopPolicy
    //   alias: re-export DSelfLoopPolicy under a graph-friendly name.
    using DLoopPolicy = djinterp::DSelfLoopPolicy;

    // old name, until 0.9.0-rc.1
    using loop_policy = DLoopPolicy;


    // =========================================================================
    // IV.  STORAGE STRATEGY
    // =========================================================================

    // DGraphStorage
    //   enum: how the graph lays its data out in memory. Selected by the user
    // as a template argument of `DGraph<>`; consumed by every method that has
    // multiple physical implementations.
    enum class DGraphStorage : re_std::uint8_t
    {
        unspecified         = 0,

        // each vertex carries a container of out-neighbors (the workhorse;
        // std::vector<std::vector<V>> style)
        adjacency_list      = 1,

        // |V| x |V| boolean / weight matrix
        // (best for dense graphs; O(1) edge query, O(V) neighbor scan)
        adjacency_matrix    = 2,

        // flat list of edges; no per-vertex structure (cheapest to build, most
        // expensive to query)
        edge_list           = 3,

        // Compressed Sparse Row: row_offset[V+1], col_index[E] (immutable /
        // static graphs; cache-friendly traversal)
        csr                 = 4,

        // Compressed Sparse Column: col_offset[V+1], row_index[E] (CSR
        // transposed; useful for in-neighbor queries)
        csc                 = 5,

        // |V| x |E| signed matrix indicating edge incidence
        // (uniform handling of directed and undirected edges)
        incidence_matrix    = 6,

        // each vertex is a heap node holding pointers to its neighbors
        // (intrusive; matches DLinkedNode / DDynamicNode)
        node_based          = 7,

        // hyperedges connecting an arbitrary number of vertices (each "edge"
        // is a set of vertex IDs)
        hyperedge_list      = 8,

        // half-edge / DCEL representation for planar / mesh graphs
        half_edge           = 9,

        // implicit graph: edges materialized on-demand by a function
        // (adjacency oracle)
        implicit            = 10
    };

    // old name, until 0.9.0-rc.1
    using graph_storage = DGraphStorage;


    // =========================================================================
    // V.   DENSITY / ACYCLICITY / CONNECTIVITY / PLANARITY
    // =========================================================================

    // DGraphDensity
    //   enum: known a priori density class. Used by algorithm dispatch to
    // choose between O(V^2) and O(V+E) implementations.
    enum class DGraphDensity : re_std::uint8_t
    {
        unknown      = 0,

        // |E| = O(V); typical adjacency-list workloads
        sparse       = 1,

        // |E| = O(V^2); adjacency-matrix often wins
        dense        = 2,

        // |E| = V*(V-1)/2 (undirected) or V*(V-1) (directed)
        complete     = 3,

        // |E| = 0
        empty        = 4
    };

    // old name, until 0.9.0-rc.1
    using graph_density = DGraphDensity;

    // DGraphAcyclicity
    //   enum: known cycle structure of the graph.
    enum class DGraphAcyclicity : re_std::uint8_t
    {
        unknown   = 0,

        // contains at least one cycle
        cyclic    = 1,

        // no cycles (general acyclic graph)
        acyclic   = 2,

        // directed and acyclic (DAG)
        dag       = 3,

        // acyclic + connected + |E| = |V| - 1 (tree)
        tree      = 4,

        // disjoint union of trees (forest)
        forest    = 5,

        // a tree rooted at a designated vertex
        rooted_tree = 6
    };

    // old name, until 0.9.0-rc.1
    using graph_acyclicity = DGraphAcyclicity;

    // DGraphConnectivity
    //   enum: connectivity class.
    enum class DGraphConnectivity : re_std::uint8_t
    {
        unknown            = 0,

        // there is a path between every pair of vertices
        connected          = 1,

        // disconnected (>= 2 components)
        disconnected       = 2,

        // directed: every vertex reachable from every other
        // following arrows
        strongly_connected = 3,

        // directed: connected when arrows are ignored
        weakly_connected   = 4,

        // single vertex, no edges
        trivial            = 5
    };

    // old name, until 0.9.0-rc.1
    using graph_connectivity = DGraphConnectivity;

    // DGraphPlanarity
    //   enum: planarity class.
    enum class DGraphPlanarity : re_std::uint8_t
    {
        unknown    = 0,
        planar     = 1,    // can be drawn without crossings
        non_planar = 2,
        outerplanar = 3,   // planar with all vertices on outer face
        toroidal   = 4     // embeddable on a torus
    };

    // old name, until 0.9.0-rc.1
    using graph_planarity = DGraphPlanarity;


    // =========================================================================
    // VI.  EDGE PRIMITIVES
    // =========================================================================

    // DUnweightedEdge
    //   struct: a (source, target) pair with no payload. The default edge type
    // for unweighted graphs.
    template<typename VertexId = default_graph_id_t>
    struct DUnweightedEdge
    {
        using vertex_id_type = VertexId;
        using weight_type    = void;

        vertex_id_type source;
        vertex_id_type target;

        D_CONSTEXPR
        DUnweightedEdge() noexcept
            : source(djinterp::container::invalid_id<vertex_id_type>()),
              target(djinterp::container::invalid_id<vertex_id_type>())
        {}

        D_CONSTEXPR
        DUnweightedEdge(vertex_id_type _src,
                        vertex_id_type _tgt) noexcept
            : source(_src),
              target(_tgt)
        {}

        D_CONSTEXPR friend bool
        operator==(const DUnweightedEdge& _a,
                   const DUnweightedEdge& _b) noexcept
        {
            return ( (_a.source == _b.source) &&
                     (_a.target == _b.target) );
        }

        D_CONSTEXPR friend bool
        operator!=(const DUnweightedEdge& _a,
                   const DUnweightedEdge& _b) noexcept
        {
            return !(_a == _b);
        }
    };

    // old name, until 0.9.0-rc.1
    template<typename VertexId = default_graph_id_t>
    using unweighted_edge = DUnweightedEdge<VertexId>;

    // DWeightedEdge
    //   struct: a (source, target, weight) triple. The weight type is
    // user-controlled; arithmetic types, ratios, and user weights all work.
    template<typename VertexId = default_graph_id_t,
             typename Weight    = double>
    struct DWeightedEdge
    {
        using vertex_id_type = VertexId;
        using weight_type    = Weight;

        vertex_id_type source;
        vertex_id_type target;
        weight_type    weight;

        D_CONSTEXPR
        DWeightedEdge() noexcept
            : source(djinterp::container::invalid_id<vertex_id_type>()),
              target(djinterp::container::invalid_id<vertex_id_type>()),
              weight()
        {}

        D_CONSTEXPR
        DWeightedEdge(vertex_id_type _src,
                      vertex_id_type _tgt,
                      weight_type    _w) noexcept
            : source(_src),
              target(_tgt),
              weight(_w)
        {}

        D_CONSTEXPR friend bool
        operator==(const DWeightedEdge& _a,
                   const DWeightedEdge& _b) noexcept
        {
            return ( (_a.source == _b.source) &&
                     (_a.target == _b.target) &&
                     (_a.weight == _b.weight) );
        }

        D_CONSTEXPR friend bool
        operator!=(const DWeightedEdge& _a,
                   const DWeightedEdge& _b) noexcept
        {
            return !(_a == _b);
        }
    };

    // old name, until 0.9.0-rc.1
    template<typename VertexId = default_graph_id_t,
             typename Weight    = double>
    using weighted_edge = DWeightedEdge<VertexId, Weight>;

    // DLabeledEdge
    //   struct: an edge with a string-or-otherwise label, plus an optional
    // weight.
    template<typename VertexId = default_graph_id_t,
             typename Label     = std::size_t>
    struct DLabeledEdge
    {
        using vertex_id_type = VertexId;
        using label_type     = Label;
        using weight_type    = void;

        vertex_id_type source;
        vertex_id_type target;
        label_type     label;

        D_CONSTEXPR
        DLabeledEdge() noexcept
            : source(djinterp::container::invalid_id<vertex_id_type>()),
              target(djinterp::container::invalid_id<vertex_id_type>()),
              label()
        {}

        D_CONSTEXPR
        DLabeledEdge(vertex_id_type _src,
                     vertex_id_type _tgt,
                     label_type     _lbl)
            : source(_src),
              target(_tgt),
              label(_lbl)
        {}
    };

    // old name, until 0.9.0-rc.1
    template<typename VertexId = default_graph_id_t,
             typename Label     = std::size_t>
    using labeled_edge = DLabeledEdge<VertexId, Label>;

    // DPropertyEdge
    //   struct: an edge carrying an arbitrary user property type. Generalises
    // weighted_edge / labeled_edge.
    template<typename VertexId = default_graph_id_t,
             typename Property = void>
    struct DPropertyEdge
    {
        using vertex_id_type = VertexId;
        using property_type  = Property;
        using weight_type    = Property;     // alias for trait detection

        vertex_id_type source;
        vertex_id_type target;
        property_type  property;

        D_CONSTEXPR
        DPropertyEdge() noexcept
            : source(djinterp::container::invalid_id<vertex_id_type>()),
              target(djinterp::container::invalid_id<vertex_id_type>()),
              property()
        {}

        D_CONSTEXPR
        DPropertyEdge(vertex_id_type _src,
                      vertex_id_type _tgt,
                      property_type  _prop)
            : source(_src),
              target(_tgt),
              property(static_cast<property_type&&>(_prop))
        {}
    };

    // old name, until 0.9.0-rc.1
    template<typename VertexId = default_graph_id_t,
             typename Property = void>
    using property_edge = DPropertyEdge<VertexId, Property>;

    // DHyperedge
    //   struct: a set of `Arity` vertex IDs related as a single edge. Used by
    // graphs whose storage strategy is `hyperedge_list`. Use Arity == 0 for
    // runtime-variable hyperedges (where the user should switch to the
    // dynamic_hyperedge type below).
    template<std::size_t Arity,
             typename    VertexId = default_graph_id_t>
    struct DHyperedge
    {
        using vertex_id_type             = VertexId;
        static D_CONSTEXPR std::size_t arity = Arity;

        std::array<vertex_id_type, Arity> vertices;

        D_CONSTEXPR
        DHyperedge() noexcept
            : vertices{}
        {}
    };

    // old name, until 0.9.0-rc.1
    template<std::size_t Arity,
             typename    VertexId = default_graph_id_t>
    using hyperedge = DHyperedge<Arity, VertexId>;

    // DDynamicHyperedge
    //   struct: a hyperedge with runtime-variable arity, backed by a
    // user-chosen container.
    template<typename VertexId   = default_graph_id_t,
             typename Container = std::vector<VertexId>>
    struct DDynamicHyperedge
    {
        using vertex_id_type = VertexId;
        using container_type = Container;

        container_type vertices;

        D_CONSTEXPR std::size_t arity() const
        {
            return vertices.size();
        }
    };

    // old name, until 0.9.0-rc.1
    template<typename VertexId   = default_graph_id_t,
             typename Container = std::vector<VertexId>>
    using dynamic_hyperedge = DDynamicHyperedge<VertexId, Container>;


    // =========================================================================
    // VII. PROPERTY / WEIGHT POLICIES
    // =========================================================================

    // no_property
    //   struct: empty placeholder type for "no property attached".
    // Specifically chosen over `void` so it can be a member of structs and a
    // template argument. Sized to one byte by C++ rules, but
    // [[no_unique_address]] (C++20) or empty-base optimisation will typically
    // erase it.
    struct no_property
    {};

    // DWeightPolicy
    //   struct: tag carrying a weight type. Use `DWeightPolicy<void>` (or the
    // default `DUnweightedPolicy` alias below) to declare an unweighted graph.
    template<typename Weight = double>
    struct DWeightPolicy
    {
        using weight_type = Weight;

        static D_CONSTEXPR bool is_weighted =
            !std::is_same<Weight, void>::value;
    };

    // old name, until 0.9.0-rc.1
    template<typename Weight = double>
    using weight_policy = DWeightPolicy<Weight>;

    // DUnweightedPolicy
    //   alias: explicit unweighted policy.
    using DUnweightedPolicy = DWeightPolicy<void>;

    // old name, until 0.9.0-rc.1
    using unweighted_policy = DUnweightedPolicy;

    // DPropertyPolicy
    //   struct: tag carrying a vertex / edge / graph property type.
    template<typename Property = no_property>
    struct DPropertyPolicy
    {
        using property_type = Property;

        static D_CONSTEXPR bool has_property =
            !std::is_same<Property, no_property>::value &&
            !std::is_same<Property, void>::value;
    };

    // old name, until 0.9.0-rc.1
    template<typename Property = no_property>
    using property_policy = DPropertyPolicy<Property>;


    // =========================================================================
    // VIII. TRAVERSAL ORDER TAGS
    // =========================================================================
    // Tag-dispatched traversal policies.  Used by graph<>::traverse<Tag>()
    // and by tree_iterator<Node, Tag>.  Each tag is empty.

    // bfs_order_tag
    //   tag: breadth-first search order.
    struct bfs_order_tag
    {};

    // dfs_order_tag
    //   tag: depth-first search order, pre-order by default.
    struct dfs_order_tag
    {};

    // DPreOrderTag
    //   tag: parent visited before children (DFS pre-order).
    struct DPreOrderTag
    {};

    // DPostOrderTag
    //   tag: children visited before parent (DFS post-order).
    struct DPostOrderTag
    {};

    // in_order_tag
    //   tag: in-order traversal (binary trees specifically).
    struct in_order_tag
    {};

    // DLevelOrderTag
    //   tag: level-by-level (BFS with explicit depth tracking).
    struct DLevelOrderTag
    {};

    // leaf_only_tag
    //   tag: leaves only (filter over pre-order).
    struct leaf_only_tag
    {};

    // topological_order_tag
    //   tag: topological order; valid only on DAGs.
    struct topological_order_tag
    {};

    // reverse_topological_order_tag
    //   tag: reverse topological order; valid only on DAGs.
    struct reverse_topological_order_tag
    {};

    // dijkstra_order_tag
    //   tag: shortest-path order (Dijkstra; non-negative weights).
    struct dijkstra_order_tag
    {};

    // bellman_ford_order_tag
    //   tag: shortest-path order (Bellman-Ford; supports negative weights).
    struct bellman_ford_order_tag
    {};

    // astar_order_tag
    //   tag: A* search order (heuristic-driven).
    struct astar_order_tag
    {};

    // best_first_order_tag
    //   tag: greedy best-first search order.
    struct best_first_order_tag
    {};

    // random_walk_order_tag
    //   tag: stochastic random-walk traversal.
    struct random_walk_order_tag
    {};


    // =========================================================================
    // IX.  ADJACENCY CONTAINER CONCEPT HELPERS
    // =========================================================================

    NS_INTERNAL

        // adjacency_value_type_helper
        //   helper: extracts `value_type` from the adjacency container, or
        // falls back to `Default` if the container has no value_type (e.g., a
        // fixed C array).
        template<typename Container,
                 typename Default,
                 typename = void>
        struct adjacency_value_type_helper
        {
            using type = Default;
        };

        // adjacency_value_type_helper<Container, Default,
        // std::void_t<typename Container::value_type>>
        //   trait: the `std::void_t<typename Container::value_type>` case; it
        // maps to `typename Container::value_type`.
        template<typename Container,
                 typename Default>
        struct adjacency_value_type_helper<Container,
                                           Default,
                                           std::void_t<typename Container::value_type>>
        {
            using type = typename Container::value_type;
        };

    NS_END  // internal

    // adjacency_value_type_of_t
    //   trait: pulls the `value_type` (i.e., the neighbor type) out of a
    // user-supplied adjacency container, falling back to `Default` if absent.
    template<typename Container,
             typename Default = default_graph_id_t>
    using adjacency_value_type_of_t =
        typename internal::adjacency_value_type_helper<
            Container, Default>::type;

    // default_adjacency_container_t
    //   trait: default adjacency container - a std::vector keyed by the
    // user's neighbor type. Users may override per-template-arg.
    template<typename NeighborType>
    using default_adjacency_container_t = std::vector<NeighborType>;

    // default_vertex_container_t
    //   trait: default top-level vertex container - a std::vector.
    template<typename VertexEntry>
    using default_vertex_container_t = std::vector<VertexEntry>;

    // default_edge_container_t
    //   trait: default edge container for edge-list / CSR storage.
    template<typename EdgeType>
    using default_edge_container_t = std::vector<EdgeType>;


NS_END  // container
NS_END  // djinterp


// =========================================================================
// std::hash specializations for vertex_descriptor and edge_descriptor.
// Outside any djinterp namespace by language requirement.
// =========================================================================

namespace std
{
    template<typename IdType>
    struct hash<djinterp::container::DVertexDescriptor<IdType>>
    {
        std::size_t operator()(
            const djinterp::container::DVertexDescriptor<IdType>& _v
        ) const noexcept
        {
            return std::hash<IdType>{}(_v.id());
        }
    };

    template<typename IdType>
    struct hash<djinterp::container::DEdgeDescriptor<IdType>>
    {
        std::size_t operator()(
            const djinterp::container::DEdgeDescriptor<IdType>& _e
        ) const noexcept
        {
            return std::hash<IdType>{}(_e.id());
        }
    };
}

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_GRAPH_GRAPH_COMMON_HPP
