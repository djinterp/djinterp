/******************************************************************************
* djinterp [scan]                                              cpp_scanner.hpp
*
* libclang-based C++ scanner:
*   This header defines `cpp_scanner`, a concrete scanner_base derivative
* that walks a C++ source tree, parses every translation unit with
* libclang, and extracts declarations as `cpp_dom_node` payloads.
*
*   The scanner threads its discovered nodes through an internal
* `cpp_scan_result` aggregate which collects:
*     - a vector of cpp_dom_node payloads
*     - a dom_string_table for string interning
*     - indexes by stable_id and by source file
*     - dependency edges (by stable_id pair)
*
*   Libclang is used internally via a pimpl so that consumers of this
* header do not need <clang-c/Index.h> on their include path; only
* cpp_scanner.cpp pulls it in.
*
*   Pipeline integration:
*     fs::file_tree ft;
*     ft.scan("/project");
*
*     cpp_scanner sc;
*     sc.add_include_path("/usr/include");
*     sc.scan_tree(ft, 0, cpp_source_predicate(ft));
*
*     auto& results = sc.results();
*     // feed results into arena::symbol_tree / sqlite persister
*
*
* path:      /inc/cpp/scan/cpp_scanner.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.17
******************************************************************************/

#ifndef DJINTERP_CPP_SCANNER_
#define DJINTERP_CPP_SCANNER_ 1

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>
#include "../core/djinterp.hpp"
#include "../dom/cpp_dom_node.hpp"
#include "../dom/dom_node.hpp"
#include "../fs/file_tree.hpp"
#include "./scanner.hpp"


NS_DJINTERP
NS_SCAN


// ================================================================
//  cpp_source_predicate
// ================================================================

// cpp_source_predicate
//   factory: produces a node predicate that matches regular
// files whose name ends in one of the canonical C / C++ source
// or header extensions.  Captures _tree by reference; the
// returned closure is valid only while _tree is alive.
//
//   Callers that want finer-grained matching (e.g. headers
// only, C++ only, glob patterns, exclusion lists) should use
// the combinators in file_tree_filter.hpp directly.
inline std::function<bool(fs::node_id)>
cpp_source_predicate
(
    const fs::file_tree& _tree
)
{
    return [&_tree](fs::node_id _id) -> bool
    {
        const fs::file_entry& entry = _tree[_id].data;

        if (entry.type != fs::file_type_regular)
        {
            return false;
        }

        std::size_t  len  = 0;
        const char*  name = _tree.name(_id, &len);

        if ((len == 0) || (name == nullptr))
        {
            return false;
        }

        // locate the last '.' in the name.
        std::size_t dot_pos = len;

        for (std::size_t i = len; i > 0; --i)
        {
            if (name[i - 1] == '.')
            {
                dot_pos = i - 1;
                break;
            }
        }

        // no dot, or name is just ".foo" — no real extension.
        if ((dot_pos == len) || (dot_pos == 0))
        {
            return false;
        }

        const char*  ext     = name + dot_pos;
        std::size_t  ext_len = len - dot_pos;

        // canonical C / C++ source and header extensions.
        static const char* const s_exts[] = {
            ".cpp", ".cxx", ".cc",  ".c++",
            ".hpp", ".hxx", ".hh",  ".h++",
            ".c",   ".h",
            ".C",   ".H",
            nullptr
        };

        for (const char* const* p = s_exts; *p != nullptr; ++p)
        {
            std::size_t pl = std::strlen(*p);

            if ((pl == ext_len) &&
                (std::strncmp(ext, *p, ext_len) == 0))
            {
                return true;
            }
        }

        return false;
    };
}


// ================================================================
//  cpp_define
// ================================================================

// cpp_define
//   struct: a single preprocessor -D flag in decomposed form.
// Empty `value` represents a value-less define (-DFOO).
struct cpp_define
{
    std::string     name;
    std::string     value;

    cpp_define()
        : name (),
          value()
    {}

    cpp_define(const std::string& _name,
               const std::string& _value = std::string())
        : name (_name),
          value(_value)
    {}
};


// ================================================================
//  cpp_scanner_config
// ================================================================

// cpp_scanner_config
//   struct: cpp_scanner configuration.  Extends scanner_config
// with the libclang-relevant knobs: include paths, compile
// flags, defines, language standard, target triple, and parse
// tuning options.
//
//   File-selection concerns (which extensions to accept, which
// directories to exclude) are not here — the caller supplies a
// predicate when invoking scan_tree.
struct cpp_scanner_config : public scanner_config
{
    // include_paths
    //   field: user include search paths.  Rendered as
    // `-I<path>` arguments for libclang.
    std::vector<std::string>    include_paths;

    // system_include_paths
    //   field: rendered as `-isystem <path>` so diagnostics in
    // these headers are downgraded.
    std::vector<std::string>    system_include_paths;

    // compile_flags
    //   field: raw libclang command-line flags, passed through
    // verbatim.
    std::vector<std::string>    compile_flags;

    // defines
    //   field: preprocessor defines.  Rendered as `-D<name>` or
    // `-D<name>=<value>` when a value is present.
    std::vector<cpp_define>     defines;

    // language_standard
    //   field: the language standard, e.g. "c++17", "c++20".
    // Rendered as `-std=<value>`.
    std::string                 language_standard;

    // target_triple
    //   field: LLVM target triple override, e.g.
    // "x86_64-pc-linux".  Rendered as `--target=<triple>` when
    // non-empty.
    std::string                 target_triple;

    // parse_comments
    //   field: request libclang to retain doc comments so that
    // they surface on extracted cpp_dom_node payloads.
    bool                        parse_comments;

    // skip_function_bodies
    //   field: enable CXTranslationUnit_SkipFunctionBodies.
    // Much faster when only declarations are needed.
    bool                        skip_function_bodies;

    // detail_preprocessing_record
    //   field: enable CXTranslationUnit_DetailedPreprocessingRecord
    // so that macro definitions and #include directives surface
    // as first-class cursors.
    bool                        detail_preprocessing_record;

    // skip_system_headers
    //   field: do not emit nodes for declarations located inside
    // system headers.
    bool                        skip_system_headers;

    cpp_scanner_config()
        : scanner_config             ()
        , include_paths              ()
        , system_include_paths       ()
        , compile_flags              ()
        , defines                    ()
        , language_standard          ("c++17")
        , target_triple              ()
        , parse_comments             (true)
        , skip_function_bodies       (false)
        , detail_preprocessing_record(false)
        , skip_system_headers        (true)
    {}
};


// ================================================================
//  cpp_scan_result
// ================================================================

// cpp_scan_result
//   struct: the aggregate result of a cpp_scanner run.  Owns
// the extracted DOM nodes, the interned string table, a set of
// lookup indexes, and the dependency edge list.
//
//   This struct is the direct feedstock for the arena symbol
// tree and for the SQLite persistence layer — its shape
// mirrors exactly what those consumers need.
struct cpp_scan_result
{
    // nodes
    //   field: every extracted declaration, in discovery order.
    std::vector<cpp_dom_node>   nodes;

    // strings
    //   field: the string-interning table backing every
    // dom_string_id in every node.
    dom_string_table            strings;

    // index_by_stable_id
    //   field: stable_id -> index into `nodes`.  Populated as
    // nodes are appended.
    std::map<std::uint64_t, std::size_t>
        index_by_stable_id;

    // indices_by_file
    //   field: interned file-id -> list of node indices declared
    // in that file.  Enables fast per-file queries without
    // re-scanning.
    std::map<dom_string_id, std::vector<std::size_t>>
        indices_by_file;

    // parent_by_stable_id
    //   field: child_stable_id -> parent_stable_id.  Reconstructs
    // the DOM tree topology from the flat node vector.
    std::map<std::uint64_t, std::uint64_t>
        parent_by_stable_id;

    // dependency_edges
    //   field: (from_stable_id, to_stable_id) reference edges
    // extracted from the AST.  Not deduplicated.
    std::vector<std::pair<std::uint64_t, std::uint64_t>>
        dependency_edges;

    cpp_scan_result()
        : nodes              ()
        , strings            ()
        , index_by_stable_id ()
        , indices_by_file    ()
        , parent_by_stable_id()
        , dependency_edges   ()
    {}

    // clear
    //   resets the aggregate to its initial empty state.
    void clear()
    {
        nodes.clear();
        strings.clear();
        index_by_stable_id.clear();
        indices_by_file.clear();
        parent_by_stable_id.clear();
        dependency_edges.clear();

        return;
    }

    // size
    //   returns the number of stored nodes.
    std::size_t size() const
    {
        return nodes.size();
    }
};


// ================================================================
//  cpp_scanner
// ================================================================

// cpp_scanner
//   class: libclang-based C++ scanner.  Derives from
// scanner_base via CRTP, supplies the required input_type /
// item_type / result_type typedefs, and implements the
// do_scan_file / do_reset hooks that the base calls.
//
//   Non-copyable (owns a libclang CXIndex); movable.
class cpp_scanner : public scanner_base<cpp_scanner>
{
public:
    using base_type      = scanner_base<cpp_scanner>;
    using input_type     = std::string;
    using item_type      = cpp_dom_node;
    using result_type    = cpp_scan_result;
    using callbacks_type = scanner_callbacks<cpp_dom_node>;

    // ========================================================
    //  construction
    // ========================================================

    cpp_scanner();

    explicit cpp_scanner(const cpp_scanner_config& _config);

    ~cpp_scanner();

    // disable copying (libclang handle is unique).
    cpp_scanner(const cpp_scanner&)            = delete;
    cpp_scanner& operator=(const cpp_scanner&) = delete;

    // enable moving.
    cpp_scanner(cpp_scanner&& _other) noexcept;
    cpp_scanner& operator=(cpp_scanner&& _other) noexcept;


    // ========================================================
    //  cpp-specific configuration
    // ========================================================

    // cpp_config
    //   returns the current C++ scanner configuration.
    const cpp_scanner_config& cpp_config() const;

    // set_cpp_config
    //   replaces the current C++ scanner configuration.  Also
    // copies the scanner_config portion into the base.
    void set_cpp_config(const cpp_scanner_config& _config);

    // add_include_path
    //   appends a user include search path (-I).
    void add_include_path(const std::string& _path);

    // add_system_include_path
    //   appends a system include search path (-isystem).
    void add_system_include_path(const std::string& _path);

    // add_compile_flag
    //   appends a raw libclang command-line flag.
    void add_compile_flag(const std::string& _flag);

    // add_define
    //   appends a preprocessor -D define.
    void add_define(const std::string& _name,
                    const std::string& _value = std::string());

    // set_language_standard
    //   sets -std=<value>.  Values such as "c++17", "c++20".
    void set_language_standard(const std::string& _std);

    // set_target_triple
    //   sets --target=<triple>.
    void set_target_triple(const std::string& _triple);


    // ========================================================
    //  item callback (templated on item_type)
    // ========================================================

    // set_callbacks
    //   installs the full (common + item-specific) callbacks.
    // The common slice is forwarded to scanner_base.
    void set_callbacks(const callbacks_type& _callbacks);

    // callbacks
    //   returns the full callbacks bundle (common + item).
    const callbacks_type& callbacks() const;


    // ========================================================
    //  result access
    // ========================================================

    // results
    //   returns the aggregate scan result.
    const result_type& results() const;

    // items_in_file
    //   returns pointers to every extracted node declared in
    // _file.  Pointers are stable for the lifetime of results().
    std::vector<const cpp_dom_node*>
        items_in_file(const std::string& _file) const;

    // find_by_stable_id
    //   returns a pointer to the node with the given stable_id,
    // or nullptr if not found.
    const cpp_dom_node*
        find_by_stable_id(std::uint64_t _id) const;

    // dependency_edges
    //   returns the dependency edge list (stable_id pairs).
    const std::vector<std::pair<std::uint64_t, std::uint64_t>>&
        dependency_edges() const;


    // ========================================================
    //  CRTP hooks — called by scanner_base
    // ========================================================

    // do_scan_file
    //   parses _path as a translation unit, walks its AST, and
    // appends every admitted declaration to the result.
    //
    //   Returns the number of items admitted (after the
    // on_item_discovered filter).
    std::size_t do_scan_file(const input_type& _path);

    // do_reset
    //   tears down and rebuilds the libclang index and clears
    // the result aggregate.
    void do_reset();


private:
    // ========================================================
    //  pimpl
    // ========================================================

    // m_impl
    //   struct: forward-declared opaque implementation.
    // Defined in cpp_scanner.cpp, which is the sole place that
    // includes <clang-c/Index.h>.
    struct m_impl;

    // ========================================================
    //  state
    // ========================================================

    m_impl*                 m_pimpl;
    cpp_scanner_config      m_cpp_config;
    callbacks_type          m_callbacks;
    result_type             m_results;


    // ========================================================
    //  internal helpers (defined in .cpp)
    // ========================================================

    // m_init_index
    //   creates the libclang index (held by pimpl).
    void m_init_index();

    // m_destroy_index
    //   releases the libclang index.
    void m_destroy_index();

    // m_build_argv
    //   renders cpp_config into a vector<const char*> suitable
    // for clang_parseTranslationUnit.  The returned pointers
    // reference strings owned by the returned std::vector<string>
    // out-parameter — keep it alive for the clang call.
    void m_build_argv(std::vector<std::string>&  _storage,
                      std::vector<const char*>&  _argv) const;

    // m_admit_node
    //   runs the on_item_discovered filter (if any) and, on
    // success, appends _node to the result, updates the
    // indexes, and bumps the base's item counter.  Returns true
    // if admitted.
    bool m_admit_node(const cpp_dom_node& _node,
                      const std::string&  _file_path);

    // m_parse_translation_unit
    //   drives libclang over a single TU: parse, walk cursors,
    // extract cpp_dom_nodes, admit them.  Returns the number
    // of admitted items.
    std::size_t m_parse_translation_unit(const std::string& _path);
};


NS_END  // scan
NS_END  // djinterp


#endif  // DJINTERP_CPP_SCANNER_
