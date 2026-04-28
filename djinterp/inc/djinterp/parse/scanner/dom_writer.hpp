/******************************************************************************
* djinterp [dom]                                              dom_writer.hpp
*
* DOM writer:
*   This header defines `dom_writer`, a templated translator that takes
* the flat aggregate produced by cpp_scanner (cpp_scan_result) and
* populates an arena-backed DOM into a `parse_context`-shaped triple:
*
*     - a tree sink     (symbol_tree):   holds cpp_dom_node payloads
*     - a string sink   (string_table):  interns all string references
*     - a xref sink     (cross_ref):     holds dependency edges
*
*   The writer is templated on the sink types so it does not hard-couple
* to any specific arena implementation — anything that conforms to the
* structural traits in dom_writer_traits.hpp will work.  Deferred
* static_asserts on first use of `write` validate the contract.
*
*   The translation is a four-phase algorithm:
*
*     Phase 1 (strings):   every distinct string in the source
*                          dom_string_table is interned into the
*                          destination string sink, producing a
*                          remap vector (src_id -> dst_id).
*
*     Phase 2 (nodes):     each cpp_dom_node is copied; its nine
*                          dom_string_id fields are rewritten through
*                          the remap vector; the resulting node is
*                          allocated in the tree sink; the arena node
*                          id is recorded under its stable_id.
*
*     Phase 3 (topology):  for each (child_stable -> parent_stable)
*                          entry in parent_by_stable_id, both ends are
*                          looked up in the stable-to-node map and
*                          append_child is invoked on the tree sink.
*                          Orphans (parent not found) are counted.
*
*     Phase 4 (edges):     each dependency edge is forwarded to the
*                          xref sink's add_reference method.  Dangling
*                          endpoints are counted.
*
*   Writes are not transactional — a failure partway through leaves
* partial state in the sinks.  Callers who need atomicity should snapshot
* their arenas first.
*
*
* path:      /inc/cpp/dom/dom_writer.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.17
******************************************************************************/

#ifndef DJINTERP_DOM_WRITER_
#define DJINTERP_DOM_WRITER_ 1

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include "../core/djinterp.hpp"
#include "../scan/cpp_scanner.hpp"
#include "./cpp_dom_node.hpp"
#include "./dom_node.hpp"
#include "./dom_writer_traits.hpp"


NS_DJINTERP
NS_DOM


// ================================================================
//  dom_writer_status
// ================================================================

// dom_writer_status
//   typedef: classifies an issue encountered during writing.
typedef std::int32_t dom_writer_status;

constexpr dom_writer_status DDomWriterStatusSuccess           =  0;
constexpr dom_writer_status DDomWriterStatusInvalidStringRef  =  1;
constexpr dom_writer_status DDomWriterStatusMissingParent     =  2;
constexpr dom_writer_status DDomWriterStatusDanglingEdgeFrom  =  3;
constexpr dom_writer_status DDomWriterStatusDanglingEdgeTo    =  4;
constexpr dom_writer_status DDomWriterStatusAllocateFailed    =  5;
constexpr dom_writer_status DDomWriterStatusUserBase          = 64;


// ================================================================
//  dom_writer_diagnostic
// ================================================================

// dom_writer_diagnostic
//   struct: descriptor for a single non-fatal issue encountered
// during the write.  Emitted through the writer's on_diagnostic
// callback when set.
struct dom_writer_diagnostic
{
    dom_writer_status   status;
    std::uint64_t       stable_id;
    std::string         detail;

    dom_writer_diagnostic()
        : status   (DDomWriterStatusSuccess)
        , stable_id(0)
        , detail   ()
    {}

    dom_writer_diagnostic(dom_writer_status  _status,
                          std::uint64_t      _stable_id,
                          const std::string& _detail)
        : status   (_status)
        , stable_id(_stable_id)
        , detail   (_detail)
    {}
};


// ================================================================
//  dom_writer_report
// ================================================================

// dom_writer_report
//   struct: cumulative counters and diagnostic data summarizing
// a single write invocation.  Queryable after `write` returns.
struct dom_writer_report
{
    std::size_t     strings_interned;
    std::size_t     nodes_written;
    std::size_t     parents_wired;
    std::size_t     edges_written;
    std::size_t     orphan_parents;
    std::size_t     dangling_edges;
    std::size_t     invalid_string_refs;
    std::size_t     allocate_failures;

    dom_writer_report()
        : strings_interned   (0)
        , nodes_written      (0)
        , parents_wired      (0)
        , edges_written      (0)
        , orphan_parents     (0)
        , dangling_edges     (0)
        , invalid_string_refs(0)
        , allocate_failures  (0)
    {}

    // reset
    //   zeros all counters.
    void reset()
    {
        strings_interned    = 0;
        nodes_written       = 0;
        parents_wired       = 0;
        edges_written       = 0;
        orphan_parents      = 0;
        dangling_edges      = 0;
        invalid_string_refs = 0;
        allocate_failures   = 0;

        return;
    }

    // ok
    //   returns true when no diagnostics of any severity were
    // emitted — a fully clean write.
    bool ok() const
    {
        return ( (orphan_parents      == 0) &&
                 (dangling_edges      == 0) &&
                 (invalid_string_refs == 0) &&
                 (allocate_failures   == 0) );
    }
};


// ================================================================
//  dom_writer_callbacks
// ================================================================

// dom_writer_callbacks
//   struct: optional hooks for progress reporting and for
// observing individual diagnostics as they are emitted.
struct dom_writer_callbacks
{
    // on_phase_begin
    //   invoked at the start of each phase with its 1-based
    // index (1 = strings, 2 = nodes, 3 = topology, 4 = edges).
    std::function<void(int /*_phase*/,
                       const char* /*_name*/)>
        on_phase_begin;

    // on_phase_end
    //   invoked at the end of each phase with the number of
    // items processed in it.
    std::function<void(int /*_phase*/,
                       std::size_t /*_count*/)>
        on_phase_end;

    // on_diagnostic
    //   invoked for each non-fatal issue encountered.
    std::function<void(const dom_writer_diagnostic& /*_diag*/)>
        on_diagnostic;
};


// ================================================================
//  dom_writer
// ================================================================

// dom_writer
//   class: generic translator from cpp_scan_result into an
// arena-backed DOM.  Templated on the sink types so it adapts
// to any arena implementation conforming to the structural
// contract in dom_writer_traits.hpp.
//
//   _TreeType must support:
//     allocate(uint64_t, cpp_dom_node&&) -> _NodeId
//     append_child(_NodeId, _NodeId)
//
//   _StringSink must support:
//     intern(const std::string&)           (return type assignable
//                                           to dom_string_id)
//
//   _XrefSink must support:
//     add_reference(uint64_t, uint64_t)
template<typename _TreeType,
         typename _StringSink,
         typename _XrefSink>
class dom_writer
{
public:
    using tree_type        = _TreeType;
    using string_sink_type = _StringSink;
    using xref_sink_type   = _XrefSink;
    using report_type      = dom_writer_report;
    using callbacks_type   = dom_writer_callbacks;

    // node_id_type
    //   type: the arena node-id type returned by the tree
    // sink's allocate method.
    using node_id_type =
        traits::tree_node_id_t<_TreeType, cpp_dom_node>;

    // ========================================================
    //  construction
    // ========================================================

    // dom_writer (from three sinks)
    //   constructs a writer over the given tree, string, and
    // xref sinks.  The sinks must outlive the writer.
    dom_writer(_TreeType&    _tree,
               _StringSink&  _strings,
               _XrefSink&    _xref)
        : m_tree      (_tree)
        , m_strings   (_strings)
        , m_xref      (_xref)
        , m_callbacks ()
        , m_report    ()
    {}

    // disable copying — the writer holds non-owning references.
    dom_writer(const dom_writer&)            = delete;
    dom_writer& operator=(const dom_writer&) = delete;

    // ========================================================
    //  configuration
    // ========================================================

    // set_callbacks
    //   installs the progress / diagnostic callbacks.
    void set_callbacks(const callbacks_type& _cb)
    {
        m_callbacks = _cb;

        return;
    }

    // callbacks
    //   returns the currently installed callbacks.
    const callbacks_type& callbacks() const
    {
        return m_callbacks;
    }

    // ========================================================
    //  state
    // ========================================================

    // report
    //   returns the cumulative report block.
    const report_type& report() const
    {
        return m_report;
    }

    // reset_report
    //   zeroes the report counters without touching the sinks.
    void reset_report()
    {
        m_report.reset();

        return;
    }

    // ========================================================
    //  write
    // ========================================================

    // write
    //   executes the four-phase translation, feeding _result
    // into the writer's tree / string / xref sinks.  Accumulates
    // counts and diagnostics into the internal report.
    //
    //   Returns a const reference to the updated report.
    const report_type& write(const cpp_scanner::result_type& _result)
    {
        // Deferred structural checks.  Only on first use so
        // incomplete sink types at class-definition time do not
        // trigger false failures.

        static_assert(
            traits::has_allocate_method<_TreeType, cpp_dom_node>::value,
            "dom_writer: tree sink must provide "
            "`allocate(uint64_t, cpp_dom_node&&)`.");

        static_assert(
            traits::has_append_child_method<
                _TreeType,
                node_id_type
            >::value,
            "dom_writer: tree sink must provide "
            "`append_child(node_id, node_id)`.");

        static_assert(
            traits::has_intern_method<_StringSink>::value,
            "dom_writer: string sink must provide "
            "`intern(const std::string&)`.");

        static_assert(
            traits::has_add_reference_method<_XrefSink>::value,
            "dom_writer: xref sink must provide "
            "`add_reference(uint64_t, uint64_t)`.");

        // Phase 1 - intern every string from the source table
        // into the destination, building the remap vector.
        m_begin_phase(1, "strings");

        std::vector<dom_string_id> remap;
        m_build_string_remap(_result.strings, remap);

        m_end_phase(1, m_report.strings_interned);

        // Phase 2 - allocate every node, remapping its
        // dom_string_id fields through `remap`.  Record each
        // arena node id under its stable_id.
        m_begin_phase(2, "nodes");

        std::unordered_map<std::uint64_t, node_id_type>
            stable_to_node;

        m_allocate_nodes(_result, remap, stable_to_node);

        m_end_phase(2, m_report.nodes_written);

        // Phase 3 - wire parent/child relationships using the
        // stable_to_node map built in phase 2.
        m_begin_phase(3, "topology");

        m_wire_topology(_result, stable_to_node);

        m_end_phase(3, m_report.parents_wired);

        // Phase 4 - copy the dependency edges into the xref.
        m_begin_phase(4, "edges");

        m_copy_edges(_result, stable_to_node);

        m_end_phase(4, m_report.edges_written);

        return m_report;
    }


private:
    // ========================================================
    //  phase implementations
    // ========================================================

    // m_build_string_remap
    //   interns every string from _source into the destination
    // string sink and populates _remap so that
    // _remap[src_dom_string_id] is the destination id.  Slot 0
    // is reserved as the null sentinel in both tables.
    void m_build_string_remap(const dom_string_table&       _source,
                              std::vector<dom_string_id>&   _remap)
    {
        std::size_t n = _source.size();

        _remap.assign(n, D_DOM_NULL_STRING);

        // slot 0 is the shared null sentinel.
        for (dom_string_id src_id = 1;
             src_id < static_cast<dom_string_id>(n);
             ++src_id)
        {
            const std::string& s = _source.resolve(src_id);

            dom_string_id dst_id = static_cast<dom_string_id>(
                m_strings.intern(s)
            );

            _remap[src_id] = dst_id;

            m_report.strings_interned += 1;
        }

        return;
    }

    // m_remap_string_id
    //   resolves _src_id through _remap, reporting an invalid
    // reference and returning the null sentinel if out of
    // range.
    dom_string_id m_remap_string_id(
        dom_string_id                       _src_id,
        const std::vector<dom_string_id>&   _remap,
        std::uint64_t                       _stable_id)
    {
        if (_src_id >= _remap.size())
        {
            m_report.invalid_string_refs += 1;

            m_emit_diagnostic(dom_writer_diagnostic(
                DDomWriterStatusInvalidStringRef,
                _stable_id,
                "dom_string_id out of range of source table"
            ));

            return D_DOM_NULL_STRING;
        }

        return _remap[_src_id];
    }

    // m_allocate_nodes
    //   makes a copy of each source node with its string ids
    // rewritten through _remap, allocates it in the tree sink,
    // and records (stable_id -> arena_node_id) in _stable_to_node.
    void m_allocate_nodes(
        const cpp_scanner::result_type&                          _result,
        const std::vector<dom_string_id>&                        _remap,
        std::unordered_map<std::uint64_t, node_id_type>&         _stable_to_node)
    {
        _stable_to_node.reserve(_result.nodes.size());

        for (const cpp_dom_node& src : _result.nodes)
        {
            cpp_dom_node dst = src;

            // ---- dom_node (base) fields
            dst.name            = m_remap_string_id(
                src.name,            _remap, src.stable_id);
            dst.type_spelling   = m_remap_string_id(
                src.type_spelling,   _remap, src.stable_id);
            dst.comment         = m_remap_string_id(
                src.comment,         _remap, src.stable_id);
            dst.file            = m_remap_string_id(
                src.file,            _remap, src.stable_id);

            // ---- cpp_dom_node (derived) fields
            dst.qualified_name  = m_remap_string_id(
                src.qualified_name,  _remap, src.stable_id);
            dst.return_type     = m_remap_string_id(
                src.return_type,     _remap, src.stable_id);
            dst.signature       = m_remap_string_id(
                src.signature,       _remap, src.stable_id);
            dst.mangled_name    = m_remap_string_id(
                src.mangled_name,    _remap, src.stable_id);
            dst.underlying_type = m_remap_string_id(
                src.underlying_type, _remap, src.stable_id);

            node_id_type nid = m_tree.allocate(
                src.stable_id,
                static_cast<cpp_dom_node&&>(dst)
            );

            _stable_to_node.emplace(src.stable_id, nid);

            m_report.nodes_written += 1;
        }

        return;
    }

    // m_wire_topology
    //   walks parent_by_stable_id, looks up each child and
    // parent in _stable_to_node, and invokes append_child on
    // the tree sink.  Orphans (parent not found) are counted
    // and diagnosed.
    void m_wire_topology(
        const cpp_scanner::result_type&                        _result,
        const std::unordered_map<std::uint64_t,
                                 node_id_type>&                _stable_to_node)
    {
        for (const auto& entry : _result.parent_by_stable_id)
        {
            std::uint64_t child_sid  = entry.first;
            std::uint64_t parent_sid = entry.second;

            auto child_it  = _stable_to_node.find(child_sid);
            auto parent_it = _stable_to_node.find(parent_sid);

            if (child_it == _stable_to_node.end())
            {
                // child itself is absent; a missing-parent
                // diagnostic still makes more sense to the user.
                m_report.orphan_parents += 1;

                m_emit_diagnostic(dom_writer_diagnostic(
                    DDomWriterStatusMissingParent,
                    child_sid,
                    "child stable_id has no allocated node"
                ));

                continue;
            }

            if (parent_it == _stable_to_node.end())
            {
                m_report.orphan_parents += 1;

                m_emit_diagnostic(dom_writer_diagnostic(
                    DDomWriterStatusMissingParent,
                    child_sid,
                    "parent stable_id has no allocated node"
                ));

                continue;
            }

            m_tree.append_child(parent_it->second,
                                child_it->second);

            m_report.parents_wired += 1;
        }

        return;
    }

    // m_copy_edges
    //   forwards every (from, to) pair to the xref sink.
    // Endpoints absent from _stable_to_node are counted as
    // dangling but still copied — cross-TU references may point
    // outside the current result set.
    void m_copy_edges(
        const cpp_scanner::result_type&                        _result,
        const std::unordered_map<std::uint64_t,
                                 node_id_type>&                _stable_to_node)
    {
        for (const auto& edge : _result.dependency_edges)
        {
            std::uint64_t from = edge.first;
            std::uint64_t to   = edge.second;

            if (_stable_to_node.find(from) == _stable_to_node.end())
            {
                m_report.dangling_edges += 1;

                m_emit_diagnostic(dom_writer_diagnostic(
                    DDomWriterStatusDanglingEdgeFrom,
                    from,
                    "edge source stable_id has no allocated node"
                ));
            }

            if (_stable_to_node.find(to) == _stable_to_node.end())
            {
                m_report.dangling_edges += 1;

                m_emit_diagnostic(dom_writer_diagnostic(
                    DDomWriterStatusDanglingEdgeTo,
                    to,
                    "edge target stable_id has no allocated node"
                ));
            }

            m_xref.add_reference(from, to);

            m_report.edges_written += 1;
        }

        return;
    }


    // ========================================================
    //  callback dispatch
    // ========================================================

    // m_begin_phase
    //   dispatches on_phase_begin when installed.
    void m_begin_phase(int _phase,
                       const char* _name)
    {
        if (m_callbacks.on_phase_begin)
        {
            m_callbacks.on_phase_begin(_phase, _name);
        }

        return;
    }

    // m_end_phase
    //   dispatches on_phase_end when installed.
    void m_end_phase(int _phase,
                     std::size_t _count)
    {
        if (m_callbacks.on_phase_end)
        {
            m_callbacks.on_phase_end(_phase, _count);
        }

        return;
    }

    // m_emit_diagnostic
    //   dispatches on_diagnostic when installed.
    void m_emit_diagnostic(const dom_writer_diagnostic& _diag)
    {
        if (m_callbacks.on_diagnostic)
        {
            m_callbacks.on_diagnostic(_diag);
        }

        return;
    }


    // ========================================================
    //  state
    // ========================================================

    _TreeType&              m_tree;
    _StringSink&            m_strings;
    _XrefSink&              m_xref;
    callbacks_type          m_callbacks;
    report_type             m_report;
};


// ================================================================
//  write_cpp_scan_result
// ================================================================

// write_cpp_scan_result
//   free function: convenience one-shot.  Constructs a
// dom_writer over the provided sinks, invokes write, and
// returns its final report.  For callers that do not need
// callbacks or multiple writes into the same writer.
template<typename _TreeType,
         typename _StringSink,
         typename _XrefSink>
dom_writer_report
write_cpp_scan_result
(
    const cpp_scanner::result_type&     _result,
    _TreeType&                          _tree,
    _StringSink&                        _strings,
    _XrefSink&                          _xref
)
{
    dom_writer<_TreeType, _StringSink, _XrefSink> w(
        _tree, _strings, _xref
    );

    w.write(_result);

    return w.report();
}


NS_END  // dom
NS_END  // djinterp


#endif  // DJINTERP_DOM_WRITER_
