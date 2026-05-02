/******************************************************************************
* djinterp [test]                                          test_printer.hpp
*
*   Template-backed printer for the test framework.  Walks a test tree
* depth-first and renders each node through configurable text_template
* instances.  Output is directed to a pluggable sink function.
*
*   DESIGN PRINCIPLE:
*   Tree depth equates to indentation depth.  A node at depth 3 in the
* test tree is rendered with 3 repetitions of the indent unit string.
* This is the default behavior and requires no configuration.
*
*   FOR-EACH:
*   The printer walks any iterable container of test-protocol elements.
* For each element, it binds per-node specifiers to the node template,
* renders, and emits to the sink.  The walk accumulates counters for
* the summary.  Users supply extraction functions for name, message,
* depth, status, and optionally is_leaf / child_count.
*
*   SYMBOLS:
*   The {symbol} specifier (e.g. "[PASS]", "[FAIL]") is resolved by a
* configurable symbol function: std::function<string(test_status)>.
* The default produces bracketed uppercase labels.  Users may replace
* it with emoji, colored ANSI, XML tags, or any other mapping.
* Similarly, {status} is resolved by a configurable status string
* function (default: lowercase word).
*
*   NUMBERING:
*   The {number} specifier is controlled by a numbering mode:
*     none       - always empty string
*     global     - monotonically increasing across all nodes
*     per_depth  - resets to 1 each time depth changes
*     leaves_only- numbers only leaf nodes; interior nodes get ""
*
*   TEMPLATE SECTIONS:
*   The printer owns six text_template instances, each with its own
* format string.  All use "{" / "}" markers by default.
*
*     header          - rendered once before the walk
*     section_header  - rendered at the start of each depth-0 group
*     node            - rendered per-node during the walk
*     section_footer  - rendered at the end of each depth-0 group
*     summary         - rendered once after the walk
*     footer          - rendered once at the very end
*
*   BUILT-IN NODE SPECIFIERS:
*     {name}       - element name / description
*     {status}     - status string ("passed", "failed", ...)
*     {symbol}     - status symbol ("[PASS]", "[FAIL]", ...)
*     {depth}      - depth in tree (0-based decimal)
*     {indent}     - repeated indent unit for current depth
*     {number}     - sequential number per numbering mode
*     {message}    - element message text
*     {is_leaf}    - "true" or "false"
*     {children}   - direct child count (decimal)
*
*   BUILT-IN SUMMARY SPECIFIERS:
*     {total}      - total node count
*     {passed}     - passed count
*     {failed}     - failed count
*     {skipped}    - skipped count
*     {pending}    - pending count
*     {errors}     - error count
*     {symbol}     - overall pass/fail symbol
*     {pass_rate}  - percentage string (e.g. "100.00%")
*
*   PORTABILITY:
*   C++11 minimum.
*
*
* TABLE OF CONTENTS
* =================
* I.    NUMBERING MODE
* II.   INDENT STATE
* III.  PRINT CONTEXT
* IV.   DEFAULT FORMATS
* V.    TEST PRINTER
*
*
* path:      /inc/djinterp/test/test_printer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.11
******************************************************************************/

#ifndef DJINTERP_TEST_PRINTER_
#define DJINTERP_TEST_PRINTER_ 1

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <string>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "../core/text/text_template.hpp"
#include "./test_common.hpp"


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   NUMBERING MODE                                       ///
///////////////////////////////////////////////////////////////////////////////

// numbering_mode
//   enum: controls how the {number} specifier is resolved.
enum class numbering_mode
{
    none        = 0,
    global      = 1,
    per_depth   = 2,
    leaves_only = 3
};


///////////////////////////////////////////////////////////////////////////////
///                II.  INDENT STATE                                         ///
///////////////////////////////////////////////////////////////////////////////

// indent_state
//   struct: tracks indentation configuration and current depth.
// Produces the indent string on demand by repeating the unit
// string for each depth level up to the configured maximum.
struct indent_state
{
    indent_state()
        : m_unit("  "),
          m_max(16),
          m_depth(0),
          m_transform(nullptr)
    {}

    indent_state(
            const std::string& _unit,
            std::size_t        _max
        )
            : m_unit(_unit),
              m_max(_max),
              m_depth(0),
              m_transform(nullptr)
    {}

    const std::string&
    indent_string() const D_NOEXCEPT
    {
        return m_unit;
    }

    void
    set_indent(
        const std::string& _unit,
        std::size_t        _max
    )
    {
        m_unit = _unit;
        m_max  = _max;

        return;
    }

    void
    set_unit(
        const std::string& _unit
    )
    {
        m_unit = _unit;

        return;
    }

    void
    set_max(
        std::size_t _max
    ) D_NOEXCEPT
    {
        m_max = _max;

        return;
    }

    std::size_t
    max() const D_NOEXCEPT
    {
        return m_max;
    }

    std::size_t
    depth() const D_NOEXCEPT
    {
        return m_depth;
    }

    std::size_t
    indent_depth() const D_NOEXCEPT
    {
        return m_depth;
    }

    void
    set_depth(
        std::size_t _depth
    ) D_NOEXCEPT
    {
        m_depth = _depth;

        return;
    }

    void
    set_transform(
        std::function<std::string(const std::string&)> _fn
    )
    {
        m_transform = static_cast<
            std::function<std::string(const std::string&)>&&>(
                _fn);

        return;
    }

    std::string
    build() const
    {
        std::size_t levels =
            (m_depth < m_max) ? m_depth : m_max;

        std::string result;
        result.reserve(m_unit.size() * levels);

        for (std::size_t i = 0; i < levels; ++i)
        {
            result += m_unit;
        }

        if (m_transform)
        {
            return m_transform(result);
        }

        return result;
    }

    std::string
    operator()() const
    {
        return build();
    }

private:
    std::string m_unit;
    std::size_t m_max;
    std::size_t m_depth;
    std::function<std::string(const std::string&)> m_transform;
};


///////////////////////////////////////////////////////////////////////////////
///                III. PRINT CONTEXT                                         ///
///////////////////////////////////////////////////////////////////////////////

// print_context
//   struct: counters accumulated during a tree walk.
struct print_context
{
    std::size_t total;
    std::size_t passed;
    std::size_t failed;
    std::size_t skipped;
    std::size_t pending;
    std::size_t errors;
    std::size_t leaf_number;
    std::size_t global_number;
    std::size_t last_depth;
    std::vector<std::size_t> depth_counters;

    print_context()
        : total(0), passed(0), failed(0),
          skipped(0), pending(0), errors(0),
          leaf_number(0), global_number(0),
          last_depth(0), depth_counters()
    {}

    void
    accumulate(
        test_status _status
    )
    {
        ++total;

        switch (static_cast<int>(_status))
        {
            case 0: { ++passed;  break; }
            case 1: { ++failed;  break; }
            case 2: { ++skipped; break; }
            case 3: { ++pending; break; }
            default: { ++errors; break; }
        }

        return;
    }

    std::string
    next_number(
        test::numbering_mode _mode,
        std::size_t          _depth,
        bool                 _is_leaf
    )
    {
        switch (_mode)
        {
            case test::numbering_mode::global:
            {
                ++global_number;

                return size_to_string(global_number);
            }

            case test::numbering_mode::per_depth:
            {
                while (depth_counters.size() <= _depth)
                {
                    depth_counters.push_back(0);
                }

                if (_depth != last_depth)
                {
                    for (std::size_t d = _depth;
                         d < depth_counters.size();
                         ++d)
                    {
                        depth_counters[d] = 0;
                    }
                }

                last_depth = _depth;
                ++depth_counters[_depth];

                return size_to_string(depth_counters[_depth]);
            }

            case test::numbering_mode::leaves_only:
            {
                if (!_is_leaf)
                {
                    return "";
                }

                ++leaf_number;

                return size_to_string(leaf_number);
            }

            default:
            {
                return "";
            }
        }
    }

    std::string
    pass_rate() const
    {
        if (total == 0)
        {
            return "0.00%";
        }

        char buf[16];

        std::snprintf(buf, sizeof(buf), "%.2f%%",
                      (static_cast<double>(passed) /
                       static_cast<double>(total)) * 100.0);

        return std::string(buf);
    }

    static std::string
    size_to_string(
        std::size_t _v
    )
    {
        char buf[32];

        std::snprintf(buf, sizeof(buf), "%zu", _v);

        return std::string(buf);
    }
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  DEFAULT FORMATS                                       ///
///////////////////////////////////////////////////////////////////////////////

static const char* const D_TEST_FMT_NODE_DEFAULT =
    "{indent}{symbol} {name}\n";

static const char* const D_TEST_FMT_NODE_VERBOSE =
    "{indent}{number}. {symbol} {name} [{status}]\n";

static const char* const D_TEST_FMT_NODE_NUMBERED =
    "{indent}{number}. {symbol} {name}\n";

static const char* const D_TEST_FMT_NODE_MINIMAL =
    "{symbol} {name}\n";

static const char* const D_TEST_FMT_NODE_MESSAGE =
    "{indent}{symbol} {name} - {message}\n";

static const char* const D_TEST_FMT_SUMMARY_DEFAULT =
    "\n{symbol} {passed}/{total} passed"
    ", {failed} failed"
    ", {skipped} skipped"
    " ({pass_rate})\n";

static const char* const D_TEST_FMT_SUMMARY_FULL =
    "\n  ASSERTION SUMMARY:\n"
    "    Total Assertions:     {total}\n"
    "    Assertions Passed:    {passed}\n"
    "    Assertions Failed:    {failed}\n"
    "    Assertion Pass Rate:  {pass_rate}\n";

static const char* const D_TEST_FMT_HEADER_BANNER =
    "========================================"
    "========================================\n"
    "  TESTING: {suite_name}\n"
    "========================================"
    "========================================\n"
    "  Description: {suite_description}\n"
    "========================================"
    "========================================\n\n";

static const char* const D_TEST_FMT_SECTION_HEADER_DASHED =
    "\n----------------------------------------"
    "----------------------------------------\n"
    "  MODULE: {section_name}\n"
    "  {section_description}\n"
    "----------------------------------------"
    "----------------------------------------\n\n";

static const char* const D_TEST_FMT_SECTION_FOOTER_RESULTS =
    "\n----------------------------------------"
    "----------------------------------------\n"
    "  MODULE RESULTS: {section_name}\n"
    "----------------------------------------"
    "----------------------------------------\n"
    "  Assertions: {passed}/{total} passed"
    " ({pass_rate})\n"
    "  Status:     {symbol} {section_name}"
    " MODULE {status_word}\n"
    "----------------------------------------"
    "----------------------------------------\n";


///////////////////////////////////////////////////////////////////////////////
///                V.   TEST PRINTER                                          ///
///////////////////////////////////////////////////////////////////////////////

using print_sink = std::function<void(const char*, std::size_t)>;

// test_printer
//   class: template-backed tree printer with configurable
// indentation, numbering, symbols, and six template sections.
class test_printer
{
public:
    using symbol_fn_type  = std::function<std::string(test_status)>;
    using status_fn_type  = std::function<std::string(test_status)>;
    using filter_fn_type  = std::function<bool(test_status, std::size_t)>;
    using binder_fn_type  = std::function<void(text_template&, std::size_t)>;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    test_printer()
        : m_node_tmpl("{", "}"),
          m_summary_tmpl("{", "}"),
          m_header_tmpl("{", "}"),
          m_footer_tmpl("{", "}"),
          m_sec_hdr_tmpl("{", "}"),
          m_sec_ftr_tmpl("{", "}"),
          m_node_fmt(D_TEST_FMT_NODE_DEFAULT),
          m_summary_fmt(D_TEST_FMT_SUMMARY_DEFAULT),
          m_header_fmt(""),
          m_footer_fmt(""),
          m_sec_hdr_fmt(""),
          m_sec_ftr_fmt(""),
          m_indent(),
          m_numbering(numbering_mode::none),
          m_print_passing(true),
          m_print_skipped(true),
          m_print_pending(true),
          m_symbol_fn(default_symbol),
          m_status_fn(default_status_string),
          m_filter(nullptr),
          m_node_binder(nullptr),
          m_context(),
          m_sink()
    {
        m_sink = [](const char* d, std::size_t n)
        {
            std::fwrite(d, 1, n, stdout);
        };
    }


    // =================================================================
    //  format setters
    // =================================================================

    void set_node_format(const std::string& _f)           { m_node_fmt = _f; return; }
    void set_summary_format(const std::string& _f)        { m_summary_fmt = _f; return; }
    void set_header_format(const std::string& _f)         { m_header_fmt = _f; return; }
    void set_footer_format(const std::string& _f)         { m_footer_fmt = _f; return; }
    void set_section_header_format(const std::string& _f) { m_sec_hdr_fmt = _f; return; }
    void set_section_footer_format(const std::string& _f) { m_sec_ftr_fmt = _f; return; }

    const std::string& node_format()           const D_NOEXCEPT { return m_node_fmt; }
    const std::string& summary_format()        const D_NOEXCEPT { return m_summary_fmt; }
    const std::string& header_format()         const D_NOEXCEPT { return m_header_fmt; }
    const std::string& footer_format()         const D_NOEXCEPT { return m_footer_fmt; }
    const std::string& section_header_format() const D_NOEXCEPT { return m_sec_hdr_fmt; }
    const std::string& section_footer_format() const D_NOEXCEPT { return m_sec_ftr_fmt; }


    // =================================================================
    //  template access
    // =================================================================

    text_template&       node_template()                 D_NOEXCEPT { return m_node_tmpl; }
    const text_template& node_template()           const D_NOEXCEPT { return m_node_tmpl; }
    text_template&       summary_template()              D_NOEXCEPT { return m_summary_tmpl; }
    const text_template& summary_template()        const D_NOEXCEPT { return m_summary_tmpl; }
    text_template&       header_template()               D_NOEXCEPT { return m_header_tmpl; }
    const text_template& header_template()         const D_NOEXCEPT { return m_header_tmpl; }
    text_template&       footer_template()               D_NOEXCEPT { return m_footer_tmpl; }
    const text_template& footer_template()         const D_NOEXCEPT { return m_footer_tmpl; }
    text_template&       section_header_template()       D_NOEXCEPT { return m_sec_hdr_tmpl; }
    const text_template& section_header_template() const D_NOEXCEPT { return m_sec_hdr_tmpl; }
    text_template&       section_footer_template()       D_NOEXCEPT { return m_sec_ftr_tmpl; }
    const text_template& section_footer_template() const D_NOEXCEPT { return m_sec_ftr_tmpl; }


    // =================================================================
    //  indent
    // =================================================================

    indent_state&       indent()       D_NOEXCEPT { return m_indent; }
    const indent_state& indent() const D_NOEXCEPT { return m_indent; }
    const std::string&  indent_string() const D_NOEXCEPT { return m_indent.indent_string(); }
    std::size_t         indent_depth()  const D_NOEXCEPT { return m_indent.indent_depth(); }

    void
    set_indent(
        const std::string& _unit,
        std::size_t        _max = 16
    )
    {
        m_indent.set_indent(_unit, _max);

        return;
    }


    // =================================================================
    //  numbering
    // =================================================================

    int
    numbering_mode() const D_NOEXCEPT
    {
        return static_cast<int>(m_numbering);
    }

    void
    set_numbering_mode(
        int _mode
    ) D_NOEXCEPT
    {
        m_numbering = static_cast<test::numbering_mode>(_mode);

        return;
    }

    void
    set_numbering_mode(
        test::numbering_mode _mode
    ) D_NOEXCEPT
    {
        m_numbering = _mode;

        return;
    }

    void
    set_numbering(
        test::numbering_mode _mode
    ) D_NOEXCEPT
    {
        m_numbering = _mode;

        return;
    }


    // =================================================================
    //  symbol / status functions
    // =================================================================

    const symbol_fn_type& symbol_function()        const D_NOEXCEPT { return m_symbol_fn; }
    const status_fn_type& status_string_function() const D_NOEXCEPT { return m_status_fn; }

    void
    set_symbol_function(
        symbol_fn_type _fn
    )
    {
        m_symbol_fn = static_cast<symbol_fn_type&&>(_fn);

        return;
    }

    void
    set_status_string_function(
        status_fn_type _fn
    )
    {
        m_status_fn = static_cast<status_fn_type&&>(_fn);

        return;
    }


    // =================================================================
    //  filters
    // =================================================================

    void set_print_passing(bool _p) D_NOEXCEPT { m_print_passing = _p; return; }
    void set_print_skipped(bool _p) D_NOEXCEPT { m_print_skipped = _p; return; }
    void set_print_pending(bool _p) D_NOEXCEPT { m_print_pending = _p; return; }

    void
    set_node_filter(
        filter_fn_type _f
    )
    {
        m_filter = static_cast<filter_fn_type&&>(_f);

        return;
    }

    void
    set_node_binder(
        binder_fn_type _b
    )
    {
        m_node_binder = static_cast<binder_fn_type&&>(_b);

        return;
    }


    // =================================================================
    //  sink
    // =================================================================

    void set_sink(print_sink _s)            { m_sink = static_cast<print_sink&&>(_s); return; }
    void set_sink_stdout()                  { m_sink = [](const char* d, std::size_t n) { std::fwrite(d, 1, n, stdout); }; return; }
    void set_sink_string(std::string& _o)   { m_sink = [&_o](const char* d, std::size_t n) { _o.append(d, n); }; return; }
    void set_sink_file(std::FILE* _f)       { m_sink = [_f](const char* d, std::size_t n) { std::fwrite(d, 1, n, _f); }; return; }


    // =================================================================
    //  context
    // =================================================================

    const print_context& context() const D_NOEXCEPT { return m_context; }
    void reset_context() { m_context = print_context(); return; }


    // =================================================================
    //  rendering: header / footer
    // =================================================================

    void
    print_header() const
    {
        if (!m_header_fmt.empty())
        {
            emit(m_header_tmpl.render(m_header_fmt));
        }

        return;
    }

    void
    print_footer() const
    {
        if (!m_footer_fmt.empty())
        {
            emit(m_footer_tmpl.render(m_footer_fmt));
        }

        return;
    }

    void
    print_section_header() const
    {
        if (!m_sec_hdr_fmt.empty())
        {
            emit(m_sec_hdr_tmpl.render(m_sec_hdr_fmt));
        }

        return;
    }

    void
    print_section_footer() const
    {
        if (!m_sec_ftr_fmt.empty())
        {
            bind_context_to(m_sec_ftr_tmpl, m_context);
            emit(m_sec_ftr_tmpl.render(m_sec_ftr_fmt));
        }

        return;
    }


    // =================================================================
    //  rendering: single node
    // =================================================================

    void
    print_node(
        test_status        _status,
        const std::string& _name,
        const std::string& _message,
        std::size_t        _depth,
        std::size_t        _number
    ) const
    {
        render_node(
            _status, _name, _message,
            _depth, true, 0,
            print_context::size_to_string(_number));

        return;
    }


    // =================================================================
    //  rendering: for-each walk
    // =================================================================

    // walk
    //   iterates an iterable container, rendering each element
    // through the node template with depth-driven indentation.
    //
    // Extraction functions:
    //   _name_fn(elem)   -> string
    //   _msg_fn(elem)    -> string
    //   _depth_fn(elem)  -> size_t
    //   _status_fn(elem) -> test_status
    //   _leaf_fn(elem)   -> bool
    template<typename _Container,
             typename _NameFn,
             typename _MsgFn,
             typename _DepthFn,
             typename _StatusFn,
             typename _LeafFn>
    void
    walk(
        const _Container& _elements,
        _NameFn&&         _name_fn,
        _MsgFn&&          _msg_fn,
        _DepthFn&&        _depth_fn,
        _StatusFn&&       _status_fn,
        _LeafFn&&         _leaf_fn,
        bool              _with_header  = false,
        bool              _with_summary = true,
        bool              _with_footer  = false
    )
    {
        reset_context();

        if (_with_header)
        {
            print_header();
        }

        for (const auto& elem : _elements)
        {
            test_status s     = _status_fn(elem);
            std::string name  = _name_fn(elem);
            std::string msg   = _msg_fn(elem);
            std::size_t depth = _depth_fn(elem);
            bool        leaf  = _leaf_fn(elem);

            m_context.accumulate(s);

            std::string num = m_context.next_number(
                m_numbering, depth, leaf);

            if (!should_print(s, depth))
            {
                continue;
            }

            render_node(s, name, msg, depth, leaf, 0, num);
        }

        if (_with_summary)
        {
            print_summary();
        }

        if (_with_footer)
        {
            print_footer();
        }

        return;
    }

    // walk (simplified - flat, all leaves)
    template<typename _Container,
             typename _NameFn,
             typename _MsgFn,
             typename _StatusFn>
    void
    walk(
        const _Container& _elements,
        _NameFn&&         _name_fn,
        _MsgFn&&          _msg_fn,
        _StatusFn&&       _status_fn
    )
    {
        walk(
            _elements,
            static_cast<_NameFn&&>(_name_fn),
            static_cast<_MsgFn&&>(_msg_fn),
            [](const auto&) -> std::size_t { return 0; },
            static_cast<_StatusFn&&>(_status_fn),
            [](const auto&) -> bool { return true; });

        return;
    }


    // =================================================================
    //  rendering: summary
    // =================================================================

    void
    print_summary() const
    {
        if (m_summary_fmt.empty())
        {
            return;
        }

        bind_context_to(m_summary_tmpl, m_context);

        emit(m_summary_tmpl.render(m_summary_fmt));

        return;
    }

private:
    // =================================================================
    //  internal: defaults
    // =================================================================

    static std::string
    default_symbol(
        test_status _s
    )
    {
        switch (static_cast<int>(_s))
        {
            case 0: { return "[PASS]"; }
            case 1: { return "[FAIL]"; }
            case 2: { return "[SKIP]"; }
            case 3: { return "[....]"; }
            case 4: { return "[ERR!]"; }
            default: { return "[????]"; }
        }
    }

    static std::string
    default_status_string(
        test_status _s
    )
    {
        switch (static_cast<int>(_s))
        {
            case 0: { return "passed"; }
            case 1: { return "failed"; }
            case 2: { return "skipped"; }
            case 3: { return "pending"; }
            case 4: { return "error"; }
            default: { return "unknown"; }
        }
    }


    // =================================================================
    //  internal: filter
    // =================================================================

    bool
    should_print(
        test_status _status,
        std::size_t _depth
    ) const
    {
        if ( (!m_print_passing) &&
             (_status == test_status::passed) )
        {
            return false;
        }

        if ( (!m_print_skipped) &&
             (_status == test_status::skipped) )
        {
            return false;
        }

        if ( (!m_print_pending) &&
             (_status == test_status::pending) )
        {
            return false;
        }

        if ( (m_filter) &&
             (!m_filter(_status, _depth)) )
        {
            return false;
        }

        return true;
    }


    // =================================================================
    //  internal: node render
    // =================================================================

    void
    render_node(
        test_status        _status,
        const std::string& _name,
        const std::string& _message,
        std::size_t        _depth,
        bool               _is_leaf,
        std::size_t        _child_count,
        const std::string& _num_str
    ) const
    {
        if (m_node_fmt.empty())
        {
            return;
        }

        m_indent.set_depth(_depth);
        std::string indent_str = m_indent.build();

        m_node_tmpl.clear_bindings();

        m_node_tmpl.bind("name",     _name);
        m_node_tmpl.bind("status",   m_status_fn(_status));
        m_node_tmpl.bind("symbol",   m_symbol_fn(_status));
        m_node_tmpl.bind("depth",    print_context::size_to_string(_depth));
        m_node_tmpl.bind("indent",   indent_str);
        m_node_tmpl.bind("number",   _num_str);
        m_node_tmpl.bind("message",  _message);
        m_node_tmpl.bind("is_leaf",  _is_leaf ? "true" : "false");
        m_node_tmpl.bind("children", print_context::size_to_string(_child_count));

        if (m_node_binder)
        {
            m_node_binder(m_node_tmpl, _depth);
        }

        emit(m_node_tmpl.render(m_node_fmt));

        return;
    }


    // =================================================================
    //  internal: bind context
    // =================================================================

    static void
    bind_context_to(
        text_template& _tmpl,
        const print_context& _ctx
    )
    {
        _tmpl.bind("total",     print_context::size_to_string(_ctx.total));
        _tmpl.bind("passed",    print_context::size_to_string(_ctx.passed));
        _tmpl.bind("failed",    print_context::size_to_string(_ctx.failed));
        _tmpl.bind("skipped",   print_context::size_to_string(_ctx.skipped));
        _tmpl.bind("pending",   print_context::size_to_string(_ctx.pending));
        _tmpl.bind("errors",    print_context::size_to_string(_ctx.errors));
        _tmpl.bind("pass_rate", _ctx.pass_rate());

        std::string sym =
            (_ctx.failed > 0 || _ctx.errors > 0)
                ? default_symbol(test_status::failed)
                : default_symbol(test_status::passed);

        _tmpl.bind("symbol",      sym);

        _tmpl.bind("status_word",
            (_ctx.failed > 0 || _ctx.errors > 0)
                ? "FAILED" : "PASSED");

        return;
    }


    // =================================================================
    //  internal: emit
    // =================================================================

    void
    emit(
        const std::string& _text
    ) const
    {
        if ( (m_sink) &&
             (!_text.empty()) )
        {
            m_sink(_text.data(), _text.size());
        }

        return;
    }


    // =================================================================
    //  storage
    // =================================================================

    mutable text_template m_node_tmpl;
    mutable text_template m_summary_tmpl;
    mutable text_template m_header_tmpl;
    mutable text_template m_footer_tmpl;
    mutable text_template m_sec_hdr_tmpl;
    mutable text_template m_sec_ftr_tmpl;

    std::string m_node_fmt;
    std::string m_summary_fmt;
    std::string m_header_fmt;
    std::string m_footer_fmt;
    std::string m_sec_hdr_fmt;
    std::string m_sec_ftr_fmt;

    mutable indent_state     m_indent;
    test::numbering_mode     m_numbering;

    bool m_print_passing;
    bool m_print_skipped;
    bool m_print_pending;

    symbol_fn_type m_symbol_fn;
    status_fn_type m_status_fn;
    filter_fn_type m_filter;
    binder_fn_type m_node_binder;

    mutable print_context m_context;
    print_sink            m_sink;
};


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_PRINTER_
