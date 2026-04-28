/******************************************************************************
* djinterp [scan]                                                 scanner.hpp
*
* Generic scanner framework:
*   This header defines the CRTP base from which all concrete scanners
* derive.  It establishes the minimum structural interface every scanner
* must expose and provides the largest common subset of functionality
* shared by all derived scanners:
*
*     - file_tree-driven walk with pluggable node-selection predicate
*     - duplicate-file protection via a scanned-files set
*     - cumulative statistics (files visited / scanned / failed, items
*       discovered, diagnostics emitted)
*     - cross-cutting callbacks for file lifecycle and diagnostics
*     - reset of all accumulated state
*
*   Input discovery is delegated to fs::file_tree.  Extension matching,
* glob matching, exclusion patterns, hidden-file skipping, and recursion
* control are predicate concerns handled by the file_tree_filter layer —
* the scanner itself is agnostic to selection policy and simply visits
* every node the caller hands it.
*
*   Item-type-specific concerns (callback for discovered items, storage
* of extracted items, aggregation into a result_type) belong to the
* derived scanner, which is templated-on-item_type and therefore cannot
* be stored generically in the base.
*
*   The base enforces its contract purely through SFINAE and static
* assertions — no virtual functions, no tag types.  A conforming
* derived scanner must provide:
*   - `input_type`    typedef  — the type of a single scan unit
*                                (typically `std::string` for a path)
*   - `item_type`     typedef  — the type of a discovered element
*   - `result_type`   typedef  — the aggregate result container
*   - `std::size_t    do_scan_file(const input_type&);`
*   - `void           do_reset();`
*
*
* path:      /inc/cpp/scan/scanner.hpp
* link(s):   TBA
* author(s): Sam 'teer' Neal-Blim                             date: 2026.04.17
******************************************************************************/

#ifndef DJINTERP_SCANNER_
#define DJINTERP_SCANNER_ 1

#include <cstddef>
#include <cstdint>
#include <functional>
#include <set>
#include <string>
#include <type_traits>
#include "../../djinterp/core/djinterp.hpp"
#include "../fs/file_tree.hpp"
#include "./scanner_traits.hpp"


NS_DJINTERP
NS_PARSE

// ================================================================
//  scan_status
// ================================================================

// scan_status
//   typedef: classifies the outcome of a scan operation.
typedef std::int32_t scan_status;

// DScanStatus*
//   constants: standard scan status codes.  Derived scanners
// may define additional codes above DScanStatusUserBase.
constexpr scan_status DScanStatusSuccess        =  0;
constexpr scan_status DScanStatusFileNotFound   =  1;
constexpr scan_status DScanStatusPermission     =  2;
constexpr scan_status DScanStatusReadFailure    =  3;
constexpr scan_status DScanStatusParseFailure   =  4;
constexpr scan_status DScanStatusSkipped        =  5;
constexpr scan_status DScanStatusAborted        =  6;
constexpr scan_status DScanStatusUserBase       = 64;


// ================================================================
//  scan_diagnostic
// ================================================================

// scan_diagnostic
//   struct: descriptor for a diagnostic (note, warning, error,
// or fatal) emitted during a scan.  Item-type agnostic, so it
// lives in the generic scanner base.
struct scan_diagnostic
{
    // severity_kind
    //   constants: severity levels for a diagnostic.  Values
    // increase with severity; code downstream may treat anything
    // at or above `severity_error` as fatal.
    enum severity_kind : std::uint8_t
    {
        severity_note    = 0,
        severity_warning = 1,
        severity_error   = 2,
        severity_fatal   = 3
    };

    std::string     file;
    std::uint32_t   line;
    std::uint32_t   column;
    std::uint8_t    severity;
    scan_status     status;
    std::string     message;

    scan_diagnostic()
        : file    ()
        , line    (0)
        , column  (0)
        , severity(severity_note)
        , status  (DScanStatusSuccess)
        , message ()
    {}

    scan_diagnostic(const std::string& _file,
                    std::uint32_t      _line,
                    std::uint32_t      _column,
                    std::uint8_t       _severity,
                    scan_status        _status,
                    const std::string& _message)
        : file    (_file)
        , line    (_line)
        , column  (_column)
        , severity(_severity)
        , status  (_status)
        , message (_message)
    {}
};


// ================================================================
//  scanner_config
// ================================================================

// scanner_config
//   struct: scan-semantics configuration shared by all
// scanners.  Filesystem-walk configuration (extensions,
// recursion, hidden-file handling) is no longer here — it
// lives in the predicate passed to scan_tree.
struct scanner_config
{
    // abort_on_error
    //   field: if true, a failed file aborts the entire scan.
    // Otherwise the scan continues and the failure is counted.
    bool                        abort_on_error;

    // max_file_size_bytes
    //   field: skip files larger than this.  0 = unlimited.
    // Checked at the scan_tree level against the file_entry's
    // recorded size before do_scan_file is invoked.
    std::size_t                 max_file_size_bytes;

    scanner_config()
        : abort_on_error     (false)
        , max_file_size_bytes(0)
    {}
};


// ================================================================
//  scanner_stats
// ================================================================

// scanner_stats
//   struct: cumulative counters tracked by the scanner base.
// Derived scanners may bump additional counters but these are
// always maintained.
struct scanner_stats
{
    std::size_t     files_visited;
    std::size_t     files_scanned;
    std::size_t     files_skipped;
    std::size_t     files_failed;
    std::size_t     items_discovered;
    std::size_t     diagnostics_emitted;
    std::size_t     bytes_read;

    scanner_stats()
        : files_visited      (0)
        , files_scanned      (0)
        , files_skipped      (0)
        , files_failed       (0)
        , items_discovered   (0)
        , diagnostics_emitted(0)
        , bytes_read         (0)
    {}

    // reset
    //   zeros all counters.
    void reset()
    {
        files_visited       = 0;
        files_scanned       = 0;
        files_skipped       = 0;
        files_failed        = 0;
        items_discovered    = 0;
        diagnostics_emitted = 0;
        bytes_read          = 0;

        return;
    }
};


// ================================================================
//  scanner_callbacks_common
// ================================================================

// scanner_callbacks_common
//   struct: the item-type-agnostic subset of scanner callbacks.
// These callbacks do not depend on the derived scanner's
// item_type and so can be stored in the CRTP base.
//
//   Each callback is optional; a default-constructed struct is
// a no-op across the board.
struct scanner_callbacks_common
{
    // on_file_begin
    //   predicate: invoked before a file is scanned.  Returning
    // false causes the file to be skipped (counted in
    // files_skipped).
    std::function<bool(const std::string& /*_path*/)>
        on_file_begin;

    // on_file_complete
    //   invoked after a file is scanned, with the number of
    // items discovered in that file.
    std::function<void(const std::string& /*_path*/,
                       std::size_t        /*_count*/)>
        on_file_complete;

    // on_file_failed
    //   invoked when a file's scan fails.  The status carries
    // the reason.
    std::function<void(const std::string& /*_path*/,
                       scan_status        /*_status*/,
                       const std::string& /*_message*/)>
        on_file_failed;

    // on_diagnostic
    //   invoked when any diagnostic is emitted.
    std::function<void(const scan_diagnostic& /*_diag*/)>
        on_diagnostic;
};


// ================================================================
//  scanner_callbacks
// ================================================================

// scanner_callbacks
//   struct: item-type-aware callback bundle.  Inherits the
// common callbacks and adds per-item reporting.  Used by
// derived scanners that want a single struct covering all
// hook points.
template<typename _ItemType>
struct scanner_callbacks : public scanner_callbacks_common
{
    using item_type = _ItemType;

    // on_item_discovered
    //   predicate: invoked per discovered item.  Returning false
    // causes the item to be excluded from the scanner's
    // aggregate result_type.
    std::function<bool(const _ItemType& /*_item*/)>
        on_item_discovered;
};


// ================================================================
//  scanner_base
// ================================================================

// scanner_base
//   class: CRTP base for file-tree-driven scanners.  Provides
// the common machinery — duplicate protection, callback
// dispatch, statistics tracking — and exposes the public scan
// entry points.  Input discovery is supplied by the caller in
// the form of an fs::file_tree plus an optional node-selection
// predicate.
//
//   Item-specific callbacks and storage live in the derived
// class (they cannot be held in the base because _Derived's
// nested types are incomplete during base instantiation).
//
//   _Derived must expose:
//     - `using input_type  = ...;`  (scanned unit)
//     - `using item_type   = ...;`  (discovered element type)
//     - `using result_type = ...;`  (aggregate result)
//     - `std::size_t do_scan_file(const input_type&);`
//     - `void        do_reset();`
template<typename _Derived>
class scanner_base
{
private:
    using derived_type = _Derived;

    // self
    //   returns a reference to the derived instance.
    derived_type& self()
    {
        return static_cast<derived_type&>(*this);
    }

    // self (const)
    //   returns a const reference to the derived instance.
    const derived_type& self() const
    {
        return static_cast<const derived_type&>(*this);
    }

protected:
    scanner_config              m_config;
    scanner_stats               m_stats;
    scanner_callbacks_common    m_callbacks;
    std::set<std::string>       m_scanned_files;
    bool                        m_aborted;

    scanner_base()
        : m_config       ()
        , m_stats        ()
        , m_callbacks    ()
        , m_scanned_files()
        , m_aborted      (false)
    {}


    ~scanner_base()
    {}


    // --------------------------------------------------------
    //  protected helpers (for derived classes)
    // --------------------------------------------------------

    // increment_items
    //   called by derived when it has admitted an item into its
    // result set.  Increments the cumulative item counter.
    void increment_items(std::size_t _count = 1)
    {
        m_stats.items_discovered += _count;

        return;
    }

    // emit_diagnostic
    //   dispatches a diagnostic through the common callback,
    // incrementing the diagnostics counter.  Derived scanners
    // call this when they want to report a non-fatal issue.
    void emit_diagnostic(const scan_diagnostic& _diag)
    {
        m_stats.diagnostics_emitted += 1;

        if (m_callbacks.on_diagnostic)
        {
            m_callbacks.on_diagnostic(_diag);
        }

        return;
    }

    // emit_file_failed
    //   dispatches a file-failure through the common callback
    // and increments files_failed.  Honors abort_on_error by
    // setting m_aborted which halts further scanning.
    void emit_file_failed(const std::string& _path,
                          scan_status        _status,
                          const std::string& _message)
    {
        m_stats.files_failed += 1;

        if (m_callbacks.on_file_failed)
        {
            m_callbacks.on_file_failed(_path, _status, _message);
        }

        if (m_config.abort_on_error)
        {
            m_aborted = true;
        }

        return;
    }

public:
    // --------------------------------------------------------
    //  configuration
    // --------------------------------------------------------

    // config
    //   returns the current generic scanner config.
    const scanner_config& config() const
    {
        return m_config;
    }

    // set_config
    //   replaces the current generic scanner config.
    void set_config(const scanner_config& _config)
    {
        m_config = _config;

        return;
    }

    // set_abort_on_error
    //   enables or disables the abort-on-error policy.
    void set_abort_on_error(bool _abort)
    {
        m_config.abort_on_error = _abort;

        return;
    }

    // set_max_file_size
    //   sets the maximum file size in bytes (0 = unlimited).
    void set_max_file_size(std::size_t _bytes)
    {
        m_config.max_file_size_bytes = _bytes;

        return;
    }

    // --------------------------------------------------------
    //  callbacks (common / non-item)
    // --------------------------------------------------------

    // set_callbacks_common
    //   installs the item-type-agnostic callbacks.
    void set_callbacks_common(const scanner_callbacks_common& _cb)
    {
        m_callbacks = _cb;

        return;
    }

    // callbacks_common
    //   returns the current common callbacks.
    const scanner_callbacks_common& callbacks_common() const
    {
        return m_callbacks;
    }

    // --------------------------------------------------------
    //  statistics and state query
    // --------------------------------------------------------

    // stats
    //   returns the cumulative statistics block.
    const scanner_stats& stats() const
    {
        return m_stats;
    }

    // total_files_scanned
    //   convenience accessor.
    std::size_t total_files_scanned() const
    {
        return m_stats.files_scanned;
    }

    // total_items_discovered
    //   convenience accessor.
    std::size_t total_items_discovered() const
    {
        return m_stats.items_discovered;
    }

    // scanned_files
    //   returns the set of file paths that have been scanned.
    const std::set<std::string>& scanned_files() const
    {
        return m_scanned_files;
    }

    // has_scanned
    //   returns true if _path has already been scanned.
    bool has_scanned(const std::string& _path) const
    {
        return (m_scanned_files.find(_path) !=
                m_scanned_files.end());
    }

    // aborted
    //   returns true if a scan has been halted by abort_on_error.
    bool aborted() const
    {
        return m_aborted;
    }

    // --------------------------------------------------------
    //  scan entry points
    // --------------------------------------------------------

    // scan_file
    //   invokes the derived scanner on a single file.  Tracks
    // statistics, dispatches lifecycle callbacks, and protects
    // against duplicate scans.
    //
    //   Returns the number of items discovered in _path, or 0
    // if the file was skipped or failed.
    std::size_t
    scan_file
    (
        const typename derived_type::input_type& _path
    )
    {
        // Deferred structural checks: validate the derived
        // scanner only on first use of scan_file.
        static_assert(
            traits::has_input_type<derived_type>::value,
            "Scanner must define a public `input_type` typedef.");

        static_assert(
            traits::has_item_type<derived_type>::value,
            "Scanner must define a public `item_type` typedef.");

        static_assert(
            traits::has_result_type<derived_type>::value,
            "Scanner must define a public `result_type` typedef.");

        static_assert(
            traits::has_do_scan_file_method<derived_type>::value,
            "Scanner must define a public `do_scan_file` member "
            "function accepting const input_type& and returning "
            "a size_t-convertible value.");

        static_assert(
            traits::has_do_reset_method<derived_type>::value,
            "Scanner must define a public `do_reset` member function.");

        m_stats.files_visited += 1;

        if (m_aborted)
        {
            m_stats.files_skipped += 1;
            return 0;
        }

        if (has_scanned(_path))
        {
            m_stats.files_skipped += 1;
            return 0;
        }

        if (m_callbacks.on_file_begin &&
            !m_callbacks.on_file_begin(_path))
        {
            m_stats.files_skipped += 1;
            return 0;
        }

        // Mark as scanned up-front so derived failures don't
        // cause re-entry on retry loops.
        m_scanned_files.insert(_path);

        std::size_t count = self().do_scan_file(_path);

        m_stats.files_scanned    += 1;
        m_stats.items_discovered += count;

        if (m_callbacks.on_file_complete)
        {
            m_callbacks.on_file_complete(_path, count);
        }

        return count;
    }

    // scan_files
    //   iterates a container of input_type paths, invoking
    // scan_file on each.  Honors the aborted flag.
    //
    //   Returns the total number of items discovered.
    template<typename _Container>
    std::size_t
    scan_files
    (
        const _Container& _paths
    )
    {
        std::size_t total = 0;

        for (const auto& path : _paths)
        {
            if (m_aborted)
            {
                break;
            }

            total += scan_file(path);
        }

        return total;
    }

    // scan_tree
    //   walks _tree from _root in depth-first order, invoking
    // scan_file on every regular-file node for which _pred
    // returns true.  _pred must be callable as
    // `bool(fs::node_id)`.
    //
    //   Only valid when the derived scanner's input_type is
    // std::string — the tree produces paths.  The max-file-size
    // policy is enforced here against each file_entry's
    // recorded size before do_scan_file is invoked.
    //
    //   Returns the total number of items discovered across
    // admitted files.
    template<typename _Predicate>
    std::size_t
    scan_tree
    (
        const fs::file_tree& _tree,
        fs::node_id          _root,
        const _Predicate&    _pred
    )
    {
        static_assert(
            std::is_same<
                typename derived_type::input_type,
                std::string
            >::value,
            "scan_tree requires the derived scanner's input_type "
            "to be std::string.");

        std::size_t total = 0;

        _tree.visit_depth_first(
            _root,
            [&](fs::node_id _id, std::size_t /*_depth*/) -> void
            {
                if (m_aborted)
                {
                    return;
                }

                const fs::file_entry& entry = _tree[_id].data;

                // scanners only consume regular files; directories,
                // symlinks, and unknown entries are passed over.
                if (entry.type != fs::file_type_regular)
                {
                    return;
                }

                // honor the max-file-size policy at the walk level
                // so the derived do_scan_file never sees oversized
                // files.
                if (m_config.max_file_size_bytes > 0 &&
                    entry.size > m_config.max_file_size_bytes)
                {
                    m_stats.files_skipped += 1;
                    return;
                }

                if (!_pred(_id))
                {
                    return;
                }

                total += scan_file(_tree.full_path(_id));
            }
        );

        return total;
    }

    // scan_tree (no-filter overload)
    //   scans every regular file reachable from _root.
    std::size_t
    scan_tree
    (
        const fs::file_tree& _tree,
        fs::node_id          _root = 0
    )
    {
        return scan_tree(
            _tree,
            _root,
            [](fs::node_id) -> bool { return true; }
        );
    }

    // scan_directory
    //   convenience: builds a temporary file_tree from _dir and
    // scans every regular file it contains.  For callers that
    // do not need a persistent file_tree artifact; those who
    // do should call fs::file_tree::scan + scan_tree directly.
    std::size_t
    scan_directory
    (
        const std::string& _dir
    )
    {
        fs::file_tree ft;

        fs::node_id root = ft.scan(_dir);

        if (root == fs::null_node)
        {
            return 0;
        }

        return scan_tree(ft, root);
    }

    // reset
    //   clears all accumulated state (scanned files set,
    // statistics, abort flag) and delegates to the derived
    // do_reset.  The configuration is preserved; use
    // `set_config(scanner_config())` to reset that too.
    void
    reset()
    {
        m_stats.reset();
        m_scanned_files.clear();
        m_aborted = false;

        self().do_reset();

        return;
    }
};


NS_END  // parse
NS_END  // djinterp


#endif  // DJINTERP_SCANNER_
