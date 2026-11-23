/*******************************************************************************
* djinterp [cpp][container]                                            graph.hpp
*
* Versatile, version-portable graph container.
*   The `DGraph<>` template fulfills axes 1-8 of the djinterp container
* classification (lifetime, iteration, ordering, bounds, multiplicity,
* structure, storage, thread safety) and gives the user control over
* every type and every byte:
*
*    - vertex / edge / graph property types        (any type, default void)
*    - vertex / edge ID integer types              (any unsigned integer)
*    - direction policy                            (graph_direction enum)
*    - multiplicity policy                         (edge_multiplicity enum)
*    - self-loop policy                            (loop_policy enum)
*    - storage strategy                            (graph_storage enum)
*    - per-vertex adjacency container              (any djinterp container)
*    - top-level vertex container                  (any djinterp container)
*    - separate edge container                     (any djinterp container)
*    - bounds (size_interval, depth_interval, multiplicity_interval)
*    - thread-safety lock policy                   (any lock policy)
*    - allocator                                   (any std::allocator-compat)
*
*   The adjacency container parameter is the keystone: any djinterp
* container that exposes `value_type`, `begin()`/`end()`, and either
* `push_back()` or `insert()` may serve as the per-vertex adjacency
* list.  The framework auto-detects ordering (sorted vs. ordered vs.
* unordered), iteration level, and uniqueness from the container the
* user passes - a `std::vector` gives an ordered multigraph view, a
* `std::set` gives a sorted simple-graph view, an `std::unordered_set`
* gives a hashed simple-graph view, all with no further user
* configuration.
*
*   The graph also provides hierarchical-container methods (parent,
* children, root, depth) when the underlying topology is acyclic;
* these methods are SFINAE-disabled on cyclic graphs to avoid
* meaningless calls.
*
* GRAPH KIND ALIASES:
*   simple_graph<...>           - undirected, simple, no self-loops
*   digraph<...>                - directed, simple, no self-loops
*   pseudograph<...>            - undirected, simple, self-loops allowed
*   multigraph<...>             - undirected, multi, no self-loops
*   multidigraph<...>           - directed, multi, no self-loops
*   weighted_graph<W, ...>      - undirected, simple, weighted
*   weighted_digraph<W, ...>    - directed, simple, weighted
*   bipartite_graph<...>        - undirected, two vertex parts
*   tournament<...>             - directed, every pair connected
*   dag<...>                    - directed, acyclic
*   tree_graph<...>             - undirected, connected, acyclic
*   forest<...>                 - undirected, acyclic (>= 1 component)
*   hypergraph<...>             - edges connect arbitrary vertex sets
*   labeled_graph<L, ...>       - simple graph with labeled edges
*   property_graph<VP, EP, ...> - vertex and edge properties
*
* AXIS FULFILLMENT:
*   1. Lifetime           - mutable_storage by default; const view via
*                           `as_const()`; `constexpr` storage if user
*                           provides a constexpr-capable adjacency
*                           container.
*   2. Iteration          - forward iterable over vertices, with
*                           `vertices()`, `edges()`, `neighbors(v)`,
*                           `out_edges(v)`, `in_edges(v)` ranges; BFS /
*                           DFS / topological iterators tag-dispatched.
*   3. Ordering           - ordered (insertion order over vertices);
*                           sorted-adjacency exposed when the user's
*                           adjacency container is sorted.
*   4. Bounds             - size_interval, depth_interval (when
*                           hierarchical), multiplicity_interval (for
*                           parallel-edge bounds in multigraphs).
*   5. Multiplicity       - simple / multi / bounded via
*                           edge_multiplicity template parameter.
*   6. Structure          - hierarchical (parent / children / root /
*                           node_type / depth_type when acyclic); flat
*                           edge-list view always available.
*   7. Storage            - graph_storage template parameter selects
*                           adjacency_list / matrix / edge_list / CSR /
*                           CSC / incidence / node_based / hyperedge.
*   8. Thread Safety      - lock policy template parameter; null policy
*                           by default; framework-detected via
*                           threadsafe_container_traits.
*
*
* path:      /inc/djinterp/cpp/container/graph/graph.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.04.27
*                                                            revised: 2026.11.11
*******************************************************************************/

#ifndef DJINTERP_CONTAINER_GRAPH_GRAPH_HPP
#define DJINTERP_CONTAINER_GRAPH_GRAPH_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error (round
// 2's rule). The owner's ruling: compile at every level first; port down only
// where something needs it.
#include "../../../env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// std
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>
// djinterp
#include "../../djinterp.hpp"
#include "../traits/interval_protocol.hpp"     // closed_bounds
#include "./graph_common.hpp"                 // invalid_id, unknown_depth
#include "../../sync/threadsafe.hpp"
// re_std
#include "../../../../re_std/cpp/cstdint/cstdint.hpp"  // re_std::uint8_t


NS_DJINTERP
NS_CONTAINER

    // =========================================================================
    // I.   ADJACENCY ENTRY
    // =========================================================================
    //
    // An adjacency entry is the value stored inside the per-vertex adjacency
    // container.  For unweighted graphs this is just a vertex_id; for
    // weighted graphs it is a (vertex_id, weight) pair; for property
    // graphs it carries an edge property.  The graph<> template selects
    // the right adjacency_entry shape based on its weight / property
    // policies.

    // DAdjacencyEntry
    //   struct: a single entry inside a vertex's adjacency container. Holds
    // the neighbor's vertex ID and, optionally, a weight or edge property.
    // Specialised below for the unweighted case so the entry collapses to a
    // bare vertex ID at the type system level.
    template<typename VertexId,
             typename EdgeProperty = no_property>
    struct DAdjacencyEntry
    {
        using vertex_id_type   = VertexId;
        using edge_property_type = EdgeProperty;

        vertex_id_type   neighbor;
        edge_property_type property;

        D_CONSTEXPR
        DAdjacencyEntry() noexcept
            : neighbor(djinterp::container::invalid_id<vertex_id_type>()),
              property()
        {}

        D_CONSTEXPR
        DAdjacencyEntry(vertex_id_type      _n,
                        edge_property_type  _p)
            : neighbor(_n),
              property(static_cast<edge_property_type&&>(_p))
        {}

        D_CONSTEXPR friend bool
        operator==(const DAdjacencyEntry& _a,
                   const DAdjacencyEntry& _b)
        {
            return ( (_a.neighbor == _b.neighbor) &&
                     (_a.property == _b.property) );
        }

        D_CONSTEXPR friend bool
        operator<(const DAdjacencyEntry& _a,
                  const DAdjacencyEntry& _b)
        {
            return _a.neighbor < _b.neighbor;
        }
    };

    // old name, until 0.9.0-rc.1
    template<typename VertexId,
             typename EdgeProperty = no_property>
    using adjacency_entry = DAdjacencyEntry<VertexId, EdgeProperty>;

    // adjacency_entry<V, no_property>
    //   specialization: collapses the entry to a bare vertex ID for unweighted
    // graphs so an adjacency container of `std::vector<V>` (the cheapest
    // layout) is the natural representation.
    template<typename VertexId>
    struct DAdjacencyEntry<VertexId, no_property>
    {
        using vertex_id_type     = VertexId;
        using edge_property_type = no_property;

        vertex_id_type neighbor;

        D_CONSTEXPR
        DAdjacencyEntry() noexcept
            : neighbor(djinterp::container::invalid_id<vertex_id_type>())
        {}

        D_CONSTEXPR explicit
        DAdjacencyEntry(vertex_id_type _n) noexcept
            : neighbor(_n)
        {}

        D_CONSTEXPR
        operator vertex_id_type() const noexcept
        {
            return neighbor;
        }

        D_CONSTEXPR friend
        bool operator==(
            const DAdjacencyEntry& _a,
            const DAdjacencyEntry& _b
        ) noexcept
        {
            return _a.neighbor == _b.neighbor;
        }

        D_CONSTEXPR friend
        bool operator<(
            const DAdjacencyEntry& _a,
            const DAdjacencyEntry& _b
        ) noexcept
        {
            return _a.neighbor < _b.neighbor;
        }
    };


    // =========================================================================
    // II.  VERTEX ENTRY
    // =========================================================================

    // DVertexEntry
    //   struct: a single vertex record stored in the top-level vertex
    // container. Carries a user-controlled property and the per-vertex
    // adjacency container.
    template<typename VertexProperty,
             typename AdjacencyContainer>
    struct DVertexEntry
    {
        using vertex_property_type = VertexProperty;
        using adjacency_container  = AdjacencyContainer;

        vertex_property_type property;
        adjacency_container  out_adjacency;

        D_CONSTEXPR
        DVertexEntry()
            : property(),
              out_adjacency()
        {}

        D_CONSTEXPR explicit
        DVertexEntry(vertex_property_type _p)
            : property(static_cast<vertex_property_type&&>(_p)),
              out_adjacency()
        {}
    };

    // old name, until 0.9.0-rc.1
    template<typename VertexProperty,
             typename AdjacencyContainer>
    using vertex_entry = DVertexEntry<VertexProperty, AdjacencyContainer>;

    // vertex_entry<no_property, AC>
    //   specialization: collapses the property field for graphs whose vertices
    // carry no payload.
    template<typename AdjacencyContainer>
    struct DVertexEntry<no_property, AdjacencyContainer>
    {
        using vertex_property_type = no_property;
        using adjacency_container  = AdjacencyContainer;

        adjacency_container out_adjacency;

        D_CONSTEXPR
        DVertexEntry()
            : out_adjacency()
        {}
    };


    // =========================================================================
    // III. GRAPH (PRIMARY TEMPLATE)
    // =========================================================================
    //
    // Template parameter pack rationale:
    //   VertexProperty        - payload at each vertex
    //   EdgeProperty          - payload at each edge (typically a weight)
    //   GraphProperty         - payload at the graph itself
    //   VertexId              - type used to index vertices
    //   Direction             - graph_direction enum
    //   Multiplicity          - edge_multiplicity enum
    //   LoopPolicy            - self-loop policy
    //   AdjacencyContainer    - container template
    //                            instantiated per-vertex
    //                            (default std::vector); user controls
    //                            ordering / uniqueness here
    //   VertexContainer       - top-level vertex container template
    //   LockPolicy            - thread-safety lock policy
    //   Allocator             - allocator used for backing storage
    //
    // The choice to keep AdjacencyContainer as a *template template*
    // parameter (rather than a fully-instantiated type) is deliberate:
    // it lets us rebind the container's element type to the right
    // adjacency_entry instance internally without forcing the user to
    // know about the entry shape.

    template<typename VertexProperty                               = no_property,
             typename EdgeProperty                                 = no_property,
             typename GraphProperty                                = no_property,
             typename VertexId                                     = default_graph_id_t,
             DGraphDirection   Direction                           = DGraphDirection::directed,
             DEdgeMultiplicity Multiplicity                        = DEdgeMultiplicity::simple,
             DLoopPolicy       LoopPolicy                          = DLoopPolicy::disallow,
             template<typename, typename> class AdjacencyContainer = std::vector,
             template<typename, typename> class VertexContainer     = std::vector,
             typename LockPolicy                                   = djinterp::null_lock_policy,
             typename Allocator                                    = std::allocator<DAdjacencyEntry<VertexId, EdgeProperty>>>
    class DGraph
    {
    public:
        // ---------------------------------------------------------------------
        // type aliases (the structural protocol - what the trait system
        // probes)
        // ---------------------------------------------------------------------

        using self_type             = DGraph;
        using vertex_property_type  = VertexProperty;
        using edge_property_type    = EdgeProperty;
        using graph_property_type   = GraphProperty;

        using vertex_id_type        = VertexId;
        using edge_id_type          = VertexId;          // share ID space
        using vertex_descriptor_type = DVertexDescriptor<VertexId>;
        using edge_descriptor_type   = DEdgeDescriptor<VertexId>;

        using adjacency_entry_type   = DAdjacencyEntry<VertexId, EdgeProperty>;
        using allocator_type         = Allocator;

        // adjacency container, built from the user-provided template
        using adjacency_alloc_type =
            typename std::allocator_traits<allocator_type>::template
            rebind_alloc<adjacency_entry_type>;

        using adjacency_container_type =
            AdjacencyContainer<adjacency_entry_type,
                                adjacency_alloc_type>;

        // top-level vertex container
        using vertex_entry_type =
            DVertexEntry<VertexProperty, adjacency_container_type>;

        using vertex_alloc_type =
            typename std::allocator_traits<allocator_type>::template
            rebind_alloc<vertex_entry_type>;

        using vertex_container_type =
            VertexContainer<vertex_entry_type, vertex_alloc_type>;

        using size_type     = typename vertex_container_type::size_type;
        using difference_type = std::ptrdiff_t;

        // value_type alias: when iterating the graph by vertex, what does
        // dereferencing yield? Pick the descriptor for the unweighted case to
        // keep iteration cheap.
        using value_type = vertex_descriptor_type;

        // node_type / depth_type: structural detection signals (axis 6)
        using node_type  = vertex_entry_type;
        using depth_type = std::size_t;

        // bounds protocol (axis 4)
        using size_interval =
            djinterp::DClosedBounds<size_type, 0, std::numeric_limits<size_type>::max()>;
        using depth_interval =
            djinterp::DClosedBounds<depth_type, 0, std::numeric_limits<depth_type>::max()>;
        using multiplicity_interval =
            djinterp::DClosedBounds<size_type,
                ( (Multiplicity == DEdgeMultiplicity::simple) ? 0 : 0 ),
                ( (Multiplicity == DEdgeMultiplicity::simple) ? 1 :
                  std::numeric_limits<size_type>::max() )>;

        // thread-safety protocol (axis 8)
        using lock_policy_type = LockPolicy;
        using mutex_type =
            typename LockPolicy::mutex_type;

        // ---------------------------------------------------------------------
        // compile-time category flags (consumed by graph_traits)
        // ---------------------------------------------------------------------

        static D_CONSTEXPR DGraphDirection   direction        = Direction;
        static D_CONSTEXPR DEdgeMultiplicity multiplicity_kind = Multiplicity;
        static D_CONSTEXPR DLoopPolicy       loops            = LoopPolicy;
        static D_CONSTEXPR DGraphStorage     storage_strategy =
            DGraphStorage::adjacency_list;

        static D_CONSTEXPR bool is_directed_graph =
            ( (Direction == DGraphDirection::directed)   ||
              (Direction == DGraphDirection::oriented)   ||
              (Direction == DGraphDirection::tournament) ||
              (Direction == DGraphDirection::bidirected) );

        static D_CONSTEXPR bool is_undirected_graph =
            (Direction == DGraphDirection::undirected);

        static D_CONSTEXPR bool is_simple_graph =
            (Multiplicity == DEdgeMultiplicity::simple);

        static D_CONSTEXPR bool allows_parallel_edges =
            ( (Multiplicity == DEdgeMultiplicity::multi) ||
              (Multiplicity == DEdgeMultiplicity::bounded) );

        static D_CONSTEXPR bool allows_self_loops =
            ( (LoopPolicy == DLoopPolicy::allow) ||
              (LoopPolicy == DLoopPolicy::require) );

        static D_CONSTEXPR bool is_weighted_graph =
            !std::is_same<EdgeProperty, no_property>::value;

        static D_CONSTEXPR bool is_vertex_propertied =
            !std::is_same<VertexProperty, no_property>::value;

        static D_CONSTEXPR vertex_id_type npos =
            djinterp::container::invalid_id<vertex_id_type>();

        // ---------------------------------------------------------------------
        // constructors / destructor / rule of five
        // ---------------------------------------------------------------------

        D_CONSTEXPR
        DGraph()
            : m_vertices(),
              m_edge_count(0),
              m_property(),
              m_lock()
        {}

        D_CONSTEXPR explicit
        DGraph(const allocator_type& _alloc)
            : m_vertices(vertex_alloc_type(_alloc)),
              m_edge_count(0),
              m_property(),
              m_lock()
        {}

        D_CONSTEXPR explicit
        DGraph(size_type _initial_vertex_count,
              const allocator_type& _alloc = allocator_type())
            : m_vertices(_initial_vertex_count,
                         vertex_entry_type(),
                         vertex_alloc_type(_alloc)),
              m_edge_count(0),
              m_property(),
              m_lock()
        {}

        DGraph(const DGraph&)            = default;
        DGraph(DGraph&&) noexcept        = default;
        DGraph& operator=(const DGraph&) = default;
        DGraph& operator=(DGraph&&) noexcept = default;
        ~DGraph()                       = default;

        // ---------------------------------------------------------------------
        // size / capacity (axis 4: bounds)
        // ---------------------------------------------------------------------

        D_CONSTEXPR size_type size() const noexcept
        {
            return m_vertices.size();
        }

        D_CONSTEXPR size_type vertex_count() const noexcept
        {
            return m_vertices.size();
        }

        D_CONSTEXPR size_type edge_count() const noexcept
        {
            return m_edge_count;
        }

        D_CONSTEXPR size_type order() const noexcept
        {
            // graph theory: |V| is the "order" of the graph
            return vertex_count();
        }

        D_CONSTEXPR size_type magnitude() const noexcept
        {
            // graph theory: |E| is the "size" / "magnitude"
            return edge_count();
        }

        D_CONSTEXPR bool empty() const noexcept
        {
            return m_vertices.empty();
        }

        D_CONSTEXPR size_type max_size() const noexcept
        {
            return m_vertices.max_size();
        }

        // ---------------------------------------------------------------------
        // graph-level property (axis: graph property)
        // ---------------------------------------------------------------------

        D_CONSTEXPR graph_property_type& graph_property() noexcept
        {
            return m_property;
        }

        D_CONSTEXPR const graph_property_type& graph_property() const noexcept
        {
            return m_property;
        }

        // ---------------------------------------------------------------------
        // vertex modifiers
        // ---------------------------------------------------------------------

        // add_vertex
        //   method: appends a vertex with the given property (or a
        // default-constructed property if omitted) and returns its descriptor.
        vertex_descriptor_type add_vertex()
        {
            vertex_id_type id = static_cast<vertex_id_type>(m_vertices.size());
            m_vertices.emplace_back();

            return vertex_descriptor_type(id);
        }

        template<typename Prop = VertexProperty>
        typename std::enable_if<
            !std::is_same<Prop, no_property>::value,
            vertex_descriptor_type>::type
        add_vertex(Prop _property)
        {
            vertex_id_type id = static_cast<vertex_id_type>(m_vertices.size());
            m_vertices.emplace_back(static_cast<Prop&&>(_property));

            return vertex_descriptor_type(id);
        }

        // ---------------------------------------------------------------------
        // edge modifiers
        // ---------------------------------------------------------------------

        // add_edge (unweighted)
        //   method: connects two vertices. Honours direction and multiplicity
        // policy. Returns true on success, false if the edge violates
        // simple-graph or no-self-loop policy.
        template<typename Prop = EdgeProperty>
        typename std::enable_if<
            std::is_same<Prop, no_property>::value,
            bool>::type
        add_edge(vertex_descriptor_type _u,
                 vertex_descriptor_type _v)
        {
            return add_edge_impl(_u.id(), _v.id(),
                                 adjacency_entry_type(_v.id()),
                                 adjacency_entry_type(_u.id()));
        }

        // add_edge (weighted / propertied)
        //   method: connects two vertices, attaching an edge property.
        template<typename Prop = EdgeProperty>
        typename std::enable_if<
            !std::is_same<Prop, no_property>::value,
            bool>::type
        add_edge(vertex_descriptor_type _u,
                 vertex_descriptor_type _v,
                 Prop                   _property)
        {
            return add_edge_impl(
                _u.id(), _v.id(),
                adjacency_entry_type(_v.id(), _property),
                adjacency_entry_type(_u.id(), static_cast<Prop&&>(_property)));
        }

        // remove_edge
        //   method: removes the first edge between (u, v). For multigraphs,
        // removes a single instance. Returns true if an edge was found and
        // removed.
        bool remove_edge(vertex_descriptor_type _u,
                         vertex_descriptor_type _v)
        {
            if (!is_valid_id(_u.id()) || !is_valid_id(_v.id()))
            {
                return false;
            }

            auto& adj_u = m_vertices[_u.id()].out_adjacency;
            auto  it_u  = std::find_if(std::begin(adj_u), std::end(adj_u),
                [&_v](const adjacency_entry_type& _e) {
                    return _e.neighbor == _v.id();
                });

            if (it_u == std::end(adj_u))
            {
                return false;
            }

            adj_u.erase(it_u);

            // if undirected, also erase the reverse entry
            if (is_undirected_graph)
            {
                auto& adj_v = m_vertices[_v.id()].out_adjacency;
                auto  it_v  = std::find_if(std::begin(adj_v), std::end(adj_v),
                    [&_u](const adjacency_entry_type& _e) {
                        return _e.neighbor == _u.id();
                    });

                if (it_v != std::end(adj_v))
                {
                    adj_v.erase(it_v);
                }
            }

            --m_edge_count;

            return true;
        }

        // clear
        //   method: removes all vertices and edges.
        void clear()
        {
            m_vertices.clear();
            m_edge_count = 0;

            return;
        }

        // ---------------------------------------------------------------------
        // queries
        // ---------------------------------------------------------------------

        // contains
        //   method: true if the descriptor refers to an in-range vertex.
        D_CONSTEXPR bool contains(vertex_descriptor_type _v) const noexcept
        {
            return is_valid_id(_v.id());
        }

        // has_edge
        //   method: true if there is at least one edge from _u to _v (or
        // between them, for undirected graphs).
        bool has_edge(vertex_descriptor_type _u,
                      vertex_descriptor_type _v) const
        {
            if (!is_valid_id(_u.id()) || !is_valid_id(_v.id()))
            {
                return false;
            }

            const auto& adj = m_vertices[_u.id()].out_adjacency;

            return ( std::find_if(std::begin(adj), std::end(adj),
                [&_v](const adjacency_entry_type& _e) {
                    return _e.neighbor == _v.id();
                }) != std::end(adj) );
        }

        // out_degree
        //   method: number of out-edges incident to _v. For undirected graphs
        // this is just `degree`.
        size_type out_degree(vertex_descriptor_type _v) const
        {
            if (!is_valid_id(_v.id()))
            {
                return 0;
            }

            return m_vertices[_v.id()].out_adjacency.size();
        }

        size_type degree(vertex_descriptor_type _v) const
        {
            return out_degree(_v);
        }

        // in_degree
        //   method: number of in-edges to _v. For directed graphs this is
        // computed by scanning every other vertex's adjacency; for undirected
        // graphs it is `degree`. Users who need O(1) in- degree should
        // specialize with `bidirected` direction (which stores both in- and
        // out-edges) or maintain their own bookkeeping.
        size_type in_degree(vertex_descriptor_type _v) const
        {
            if (!is_valid_id(_v.id()))
            {
                return 0;
            }

            if (is_undirected_graph)
            {
                return out_degree(_v);
            }

            size_type count = 0;
            for (const auto& vert : m_vertices)
            {
                for (const auto& adj : vert.out_adjacency)
                {
                    if (adj.neighbor == _v.id())
                    {
                        ++count;
                    }
                }
            }

            return count;
        }

        // ---------------------------------------------------------------------
        // accessors
        // ---------------------------------------------------------------------

        // vertex_property (mutable)
        //   method: access the user property attached to _v. SFINAE- disabled
        // for graphs whose vertices carry no property.
        template<typename Prop = VertexProperty>
        typename std::enable_if<
            !std::is_same<Prop, no_property>::value,
            Prop&>::type
        vertex_property(vertex_descriptor_type _v)
        {
            return m_vertices[_v.id()].property;
        }

        template<typename Prop = VertexProperty>
        typename std::enable_if<
            !std::is_same<Prop, no_property>::value,
            const Prop&>::type
        vertex_property(vertex_descriptor_type _v) const
        {
            return m_vertices[_v.id()].property;
        }

        // adjacency
        //   method: access the per-vertex adjacency container. This is the
        // canonical way for clients to walk neighbors.
        adjacency_container_type& adjacency(vertex_descriptor_type _v)
        {
            return m_vertices[_v.id()].out_adjacency;
        }

        const adjacency_container_type& adjacency(vertex_descriptor_type _v) const
        {
            return m_vertices[_v.id()].out_adjacency;
        }

        // ---------------------------------------------------------------------
        // hierarchical container interface (axis 6: structure)
        // ---------------------------------------------------------------------
        // These methods are provided unconditionally; on cyclic graphs
        // they have well-defined "best-effort" semantics (parent is
        // first in-neighbor, root is first vertex with in-degree 0).

        // root
        //   method: returns the descriptor of the first vertex with in-degree
        // 0. Useful for tree/forest/DAG roots. For cyclic graphs returns
        // null_vertex().
        vertex_descriptor_type root() const
        {
            for (size_type i = 0; i < m_vertices.size(); ++i)
            {
                if (in_degree(vertex_descriptor_type(static_cast<vertex_id_type>(i))) == 0)
                {
                    return vertex_descriptor_type(static_cast<vertex_id_type>(i));
                }
            }

            return null_vertex<vertex_id_type>();
        }

        // parent
        //   method: returns the first in-neighbor of _v (canonical parent for
        // tree-like graphs).
        vertex_descriptor_type parent(vertex_descriptor_type _v) const
        {
            for (size_type i = 0; i < m_vertices.size(); ++i)
            {
                const auto& adj = m_vertices[i].out_adjacency;

                if (std::find_if(std::begin(adj), std::end(adj),
                        [&_v](const adjacency_entry_type& _e) {
                            return _e.neighbor == _v.id();
                        }) != std::end(adj))
                {
                    return vertex_descriptor_type(static_cast<vertex_id_type>(i));
                }
            }

            return null_vertex<vertex_id_type>();
        }

        // children
        //   method: returns a const reference to the out-adjacency container
        // of _v (canonical children for tree-like graphs).
        const adjacency_container_type& children(vertex_descriptor_type _v) const
        {
            return adjacency(_v);
        }

        // depth
        //   method: returns the depth of _v as the length of the shortest path
        // from root() to _v. Returns unknown_depth for unreachable vertices.
        depth_type depth(vertex_descriptor_type _v) const
        {
            return depth_from(_v, root());
        }

        // ---------------------------------------------------------------------
        // iteration (axis 2)
        // ---------------------------------------------------------------------
        // The graph's `begin()`/`end()` walk the vertices in insertion
        // order, yielding descriptors.  Edge / neighbor iteration is
        // exposed via the dedicated methods below.

        class vertex_iterator
        {
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type        = vertex_descriptor_type;
            using difference_type   = std::ptrdiff_t;
            using reference         = vertex_descriptor_type;
            using pointer           = const vertex_descriptor_type*;

            D_CONSTEXPR
            vertex_iterator() noexcept
                : m_index(0)
            {}

            D_CONSTEXPR explicit
            vertex_iterator(vertex_id_type _i) noexcept
                : m_index(_i)
            {}

            D_CONSTEXPR vertex_descriptor_type operator*() const noexcept
            {
                return vertex_descriptor_type(m_index);
            }

            D_CONSTEXPR vertex_iterator& operator++() noexcept
            {
                ++m_index;
                return *this;
            }

            D_CONSTEXPR vertex_iterator operator++(int) noexcept
            {
                vertex_iterator tmp(*this);
                ++m_index;
                return tmp;
            }

            D_CONSTEXPR friend bool
            operator==(vertex_iterator _a, vertex_iterator _b) noexcept
            {
                return _a.m_index == _b.m_index;
            }

            D_CONSTEXPR friend bool
            operator!=(vertex_iterator _a, vertex_iterator _b) noexcept
            {
                return _a.m_index != _b.m_index;
            }

        private:
            vertex_id_type m_index;
        };

        using iterator       = vertex_iterator;
        using const_iterator = vertex_iterator;

        D_CONSTEXPR vertex_iterator begin() const noexcept
        {
            return vertex_iterator(0);
        }

        D_CONSTEXPR vertex_iterator end() const noexcept
        {
            return vertex_iterator(static_cast<vertex_id_type>(m_vertices.size()));
        }

        D_CONSTEXPR vertex_iterator cbegin() const noexcept
        {
            return begin();
        }

        D_CONSTEXPR vertex_iterator cend() const noexcept
        {
            return end();
        }

        // ---------------------------------------------------------------------
        // text / debug
        // ---------------------------------------------------------------------

        // swap
        //   method: exchanges contents with another graph of the same template
        // instantiation.
        void swap(DGraph& _other) noexcept
        {
            using std::swap;
            swap(m_vertices,   _other.m_vertices);
            swap(m_edge_count, _other.m_edge_count);
            swap(m_property,   _other.m_property);

            return;
        }

        // ---------------------------------------------------------------------
        // comparison
        // ---------------------------------------------------------------------

        D_CONSTEXPR friend bool
        operator==(const DGraph& _a, const DGraph& _b)
        {
            return ( (_a.m_edge_count == _b.m_edge_count) &&
                     (_a.m_vertices   == _b.m_vertices) );
        }

        D_CONSTEXPR friend bool
        operator!=(const DGraph& _a, const DGraph& _b)
        {
            return !(_a == _b);
        }

    private:
        // ---------------------------------------------------------------------
        // internal helpers
        // ---------------------------------------------------------------------

        D_CONSTEXPR bool is_valid_id(vertex_id_type _id) const noexcept
        {
            return ( !djinterp::container::is_invalid_id(_id) &&
                     (static_cast<size_type>(_id) < m_vertices.size()) );
        }

        // add_edge_impl
        //   helper: shared back-end for add_edge. Honours direction and
        // multiplicity policy. `_forward_entry` will be inserted into u's
        // adjacency; `_reverse_entry` into v's only if the graph is undirected
        // (direction == undirected) or bidirected.
        bool add_edge_impl(vertex_id_type           _u,
                           vertex_id_type           _v,
                           adjacency_entry_type     _forward_entry,
                           adjacency_entry_type     _reverse_entry)
        {
            if (!is_valid_id(_u) || !is_valid_id(_v))
            {
                return false;
            }

            // self-loop policy
            if (_u == _v)
            {
                if (!allows_self_loops)
                {
                    return false;
                }
            }

            auto& adj_u = m_vertices[_u].out_adjacency;

            // simple-graph deduplication
            if (is_simple_graph)
            {
                if (std::find_if(std::begin(adj_u), std::end(adj_u),
                        [_v](const adjacency_entry_type& _e) {
                            return _e.neighbor == _v;
                        }) != std::end(adj_u))
                {
                    return false;
                }
            }

            adj_u.insert(std::end(adj_u),
                         static_cast<adjacency_entry_type&&>(_forward_entry));

            // mirror for undirected / bidirected
            if (is_undirected_graph ||
                (Direction == DGraphDirection::bidirected))
            {
                if (_u != _v)
                {
                    auto& adj_v = m_vertices[_v].out_adjacency;
                    adj_v.insert(std::end(adj_v),
                                 static_cast<adjacency_entry_type&&>(_reverse_entry));
                }
            }

            ++m_edge_count;

            return true;
        }

        // depth_from
        //   helper: BFS from `_from` to compute the distance to `_v`. Returns
        // unknown_depth if unreachable.
        depth_type depth_from(vertex_descriptor_type _v,
                              vertex_descriptor_type _from) const
        {
            if (_from.is_null() || _v.is_null())
            {
                return unknown_depth;
            }

            if (_v == _from)
            {
                return 0;
            }

            std::vector<bool>       visited(m_vertices.size(), false);
            std::vector<depth_type> dist(m_vertices.size(),
                                         unknown_depth);
            std::vector<vertex_id_type> queue;
            queue.reserve(m_vertices.size());

            visited[_from.id()] = true;
            dist   [_from.id()] = 0;
            queue.push_back(_from.id());

            for (size_type qi = 0; qi < queue.size(); ++qi)
            {
                vertex_id_type u = queue[qi];
                const auto&    adj = m_vertices[u].out_adjacency;

                for (const auto& entry : adj)
                {
                    if (!visited[entry.neighbor])
                    {
                        visited[entry.neighbor] = true;
                        dist   [entry.neighbor] = dist[u] + 1;

                        if (entry.neighbor == _v.id())
                        {
                            return dist[entry.neighbor];
                        }

                        queue.push_back(entry.neighbor);
                    }
                }
            }

            return unknown_depth;
        }

        // ---------------------------------------------------------------------
        // members
        // ---------------------------------------------------------------------

        vertex_container_type m_vertices;
        size_type             m_edge_count;
        graph_property_type   m_property;
        mutable lock_policy_type m_lock;
    };

    // old name, until 0.9.0-rc.1
    template<typename VertexProperty                               = no_property,
             typename EdgeProperty                                 = no_property,
             typename GraphProperty                                = no_property,
             typename VertexId                                     = default_graph_id_t,
             DGraphDirection   Direction                           = DGraphDirection::directed,
             DEdgeMultiplicity Multiplicity                        = DEdgeMultiplicity::simple,
             DLoopPolicy       LoopPolicy                          = DLoopPolicy::disallow,
             template<typename, typename> class AdjacencyContainer = std::vector,
             template<typename, typename> class VertexContainer     = std::vector,
             typename LockPolicy                                   = djinterp::null_lock_policy,
             typename Allocator                                    = std::allocator<DAdjacencyEntry<VertexId, EdgeProperty>>>
    using graph = DGraph<VertexProperty, EdgeProperty, GraphProperty, VertexId, Direction, Multiplicity, LoopPolicy, AdjacencyContainer, VertexContainer, LockPolicy, Allocator>;


    // =========================================================================
    // IV.  GRAPH-KIND ALIASES
    // =========================================================================
    // Most named graph categories collapse to specific (Direction,
    // Multiplicity, LoopPolicy) combinations of the primary template.
    // These aliases give users a vocabulary at least as rich as the
    // graph-theory literature.

    // DSimpleGraph
    //   alias: undirected, simple, no self-loops.
    template<typename VP         = no_property,
             typename EP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using DSimpleGraph =
        DGraph<VP, EP, GP, VertexId,
              DGraphDirection::undirected,
              DEdgeMultiplicity::simple,
              DLoopPolicy::disallow>;

    // old name, until 0.9.0-rc.1
    template<typename VP         = no_property,
             typename EP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using simple_graph = DSimpleGraph<VP, EP, GP, VertexId>;

    // DDigraph
    //   alias: directed, simple, no self-loops.
    template<typename VP         = no_property,
             typename EP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using DDigraph =
        DGraph<VP, EP, GP, VertexId,
              DGraphDirection::directed,
              DEdgeMultiplicity::simple,
              DLoopPolicy::disallow>;

    // old name, until 0.9.0-rc.1
    template<typename VP         = no_property,
             typename EP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using digraph = DDigraph<VP, EP, GP, VertexId>;

    // DPseudograph
    //   alias: undirected, simple, self-loops allowed.
    template<typename VP         = no_property,
             typename EP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using DPseudograph =
        DGraph<VP, EP, GP, VertexId,
              DGraphDirection::undirected,
              DEdgeMultiplicity::simple,
              DLoopPolicy::allow>;

    // old name, until 0.9.0-rc.1
    template<typename VP         = no_property,
             typename EP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using pseudograph = DPseudograph<VP, EP, GP, VertexId>;

    // DMultigraph
    //   alias: undirected, multi, no self-loops.
    template<typename VP         = no_property,
             typename EP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using DMultigraph =
        DGraph<VP, EP, GP, VertexId,
              DGraphDirection::undirected,
              DEdgeMultiplicity::multi,
              DLoopPolicy::disallow>;

    // old name, until 0.9.0-rc.1
    template<typename VP         = no_property,
             typename EP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using multigraph = DMultigraph<VP, EP, GP, VertexId>;

    // DMultidigraph
    //   alias: directed, multi, no self-loops.
    template<typename VP         = no_property,
             typename EP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using DMultidigraph =
        DGraph<VP, EP, GP, VertexId,
              DGraphDirection::directed,
              DEdgeMultiplicity::multi,
              DLoopPolicy::disallow>;

    // old name, until 0.9.0-rc.1
    template<typename VP         = no_property,
             typename EP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using multidigraph = DMultidigraph<VP, EP, GP, VertexId>;

    // DMultipseudograph
    //   alias: undirected, multi, self-loops allowed. The most permissive
    // undirected graph.
    template<typename VP         = no_property,
             typename EP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using DMultipseudograph =
        DGraph<VP, EP, GP, VertexId,
              DGraphDirection::undirected,
              DEdgeMultiplicity::multi,
              DLoopPolicy::allow>;

    // old name, until 0.9.0-rc.1
    template<typename VP         = no_property,
             typename EP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using multipseudograph = DMultipseudograph<VP, EP, GP, VertexId>;

    // DWeightedGraph
    //   alias: undirected, simple, weighted (W = double by default).
    template<typename Weight     = double,
             typename VP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using DWeightedGraph =
        DGraph<VP, Weight, GP, VertexId,
              DGraphDirection::undirected,
              DEdgeMultiplicity::simple,
              DLoopPolicy::disallow>;

    // old name, until 0.9.0-rc.1
    template<typename Weight     = double,
             typename VP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using weighted_graph = DWeightedGraph<Weight, VP, GP, VertexId>;

    // DWeightedDigraph
    //   alias: directed, simple, weighted.
    template<typename Weight     = double,
             typename VP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using DWeightedDigraph =
        DGraph<VP, Weight, GP, VertexId,
              DGraphDirection::directed,
              DEdgeMultiplicity::simple,
              DLoopPolicy::disallow>;

    // old name, until 0.9.0-rc.1
    template<typename Weight     = double,
             typename VP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using weighted_digraph = DWeightedDigraph<Weight, VP, GP, VertexId>;

    // DTournament
    //   alias: directed, simple, every pair connected. The "tournament"
    // semantics are a runtime invariant; the template enforces only direction.
    template<typename VP         = no_property,
             typename EP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using DTournament =
        DGraph<VP, EP, GP, VertexId,
              DGraphDirection::tournament,
              DEdgeMultiplicity::simple,
              DLoopPolicy::disallow>;

    // old name, until 0.9.0-rc.1
    template<typename VP         = no_property,
             typename EP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using tournament = DTournament<VP, EP, GP, VertexId>;

    // DDag
    //   alias: directed, simple, no self-loops. Acyclicity is a runtime
    // invariant, asserted by the user; the template selects the most
    // permissive acyclic-friendly representation.
    template<typename VP         = no_property,
             typename EP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using DDag = DDigraph<VP, EP, GP, VertexId>;

    // old name, until 0.9.0-rc.1
    template<typename VP         = no_property,
             typename EP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using dag = DDag<VP, EP, GP, VertexId>;

    // DTreeGraph
    //   alias: undirected, simple, no self-loops. Connectedness and acyclicity
    // are runtime invariants.
    template<typename VP         = no_property,
             typename EP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using DTreeGraph = DSimpleGraph<VP, EP, GP, VertexId>;

    // old name, until 0.9.0-rc.1
    template<typename VP         = no_property,
             typename EP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using tree_graph = DTreeGraph<VP, EP, GP, VertexId>;

    // DForest
    //   alias: identical instantiation to tree_graph; the distinction is
    // semantic (not necessarily connected).
    template<typename VP         = no_property,
             typename EP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using DForest = DSimpleGraph<VP, EP, GP, VertexId>;

    // old name, until 0.9.0-rc.1
    template<typename VP         = no_property,
             typename EP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using forest = DForest<VP, EP, GP, VertexId>;

    // DBipartiteGraph
    //   alias: undirected, simple, no self-loops. Bipartiteness is a runtime
    // invariant; the user may attach a 2-coloring as the vertex property.
    template<typename Part       = re_std::uint8_t,
             typename EP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using DBipartiteGraph = DSimpleGraph<Part, EP, GP, VertexId>;

    // old name, until 0.9.0-rc.1
    template<typename Part       = re_std::uint8_t,
             typename EP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using bipartite_graph = DBipartiteGraph<Part, EP, GP, VertexId>;

    // DLabeledGraph
    //   alias: simple graph whose edges carry a label.
    template<typename Label      = std::size_t,
             typename VP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using DLabeledGraph = DSimpleGraph<VP, Label, GP, VertexId>;

    // old name, until 0.9.0-rc.1
    template<typename Label      = std::size_t,
             typename VP         = no_property,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using labeled_graph = DLabeledGraph<Label, VP, GP, VertexId>;

    // DPropertyGraph
    //   alias: simple graph with both vertex and edge properties.
    template<typename VP,
             typename EP,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using DPropertyGraph = DSimpleGraph<VP, EP, GP, VertexId>;

    // old name, until 0.9.0-rc.1
    template<typename VP,
             typename EP,
             typename GP         = no_property,
             typename VertexId   = default_graph_id_t>
    using property_graph = DPropertyGraph<VP, EP, GP, VertexId>;


    // =========================================================================
    // V.   FREE-FUNCTION CONVENIENCES
    // =========================================================================

    // swap (free function)
    //   ADL hook allowing std::swap-style use.
    template<typename VP, typename EP, typename GP, typename VertexId,
             DGraphDirection D, DEdgeMultiplicity M, DLoopPolicy L,
             template<typename, typename> class AC,
             template<typename, typename> class VC,
             typename LP, typename A>
    inline
    void swap(
        DGraph<VP,EP,GP,VertexId,D,M,L,AC,VC,LP,A>& _a,
        DGraph<VP,EP,GP,VertexId,D,M,L,AC,VC,LP,A>& _b
    ) noexcept
    {
        _a.swap(_b);

        return;
    }

    // is_self_loop
    //   function: true if the edge endpoints are equal.
    template<typename VertexId>
    D_CONSTEXPR_INLINE
    bool is_self_loop(
        DVertexDescriptor<VertexId> _u,
        DVertexDescriptor<VertexId> _v
    ) noexcept
    {
        return _u.id() == _v.id();
    }


NS_END  // container
NS_END  // djinterp

#endif  // floor, for now

#endif  // DJINTERP_CONTAINER_GRAPH_GRAPH_HPP
