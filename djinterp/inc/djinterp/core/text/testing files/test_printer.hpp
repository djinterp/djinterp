/******************************************************************************
* djinterp [test]                                             test_printer.hpp
*
*   Template-backed printer for the test framework.  Walks a test tree
* depth-first and renders each node through configurable text_template
* instances.  Output is directed to a pluggable sink function.
*
*   DESIGN PRINCIPLE:
*   Tree depth equates to indentation depth.  A node at depth 3 in the
* test tree is rendered with 3 repetitions of the indent unit string,
* exposed via the {indent} specifier.  This is the default behavior
* and requires no configuration.
*
*   FOR-EACH:
*   The printer walks any iterable container of test-protocol elements.
* For each element, it binds per-node specifiers to the node template,
* renders, and emits to the sink.  The walk accumulates counters for
* the summary.  Users supply extraction functions for name, message,
* depth, status, and optionally is_leaf / child_count / elapsed.
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
*   LINE NUMBERING:
*   The {line} specifier resolves to a global render-line counter
* that increments by one for every node emission.  Line numbers
* persist across header / section / footer boundaries within a
* single walk and reset when reset_context() is called (which
* every walk() invocation does as its first step).  When the node
* format produces a single output line per render, {line} matches
* the visible line number; when the format produces multiple lines
* per render, {line} marks the first line of the rendered node.
* 
* ELAPSED TIME:
*   Each rendered node may carry a wall-clock duration (in nanoseconds) 
* supplied by an optional extraction function:
*     _elapsed_fn(elem) -> std::int64_t   // nanoseconds
*   When no elapsed extractor is provided, every node renders with
* an elapsed value of 0.  The {elapsed} specifier is resolved by
* a configurable elapsed_format function (default: human-readable,
* auto-scaled unit).  Fixed-unit specifiers are also bound:
*     {elapsed_ns} - integer nanoseconds
*     {elapsed_us} - integer microseconds  (truncated)
*     {elapsed_ms} - integer milliseconds  (truncated)
*     {elapsed_s}  - decimal seconds       (two-place)
* TEMPLATE SECTIONS:
*   The printer owns six bound_template instances (a format-owning
* text_template plus its own binding set), each with its own format
* string.  Placeholders use "{key}" syntax.
*     header          - rendered once before the walk
*     section_header  - rendered at the start of each depth-0 group
*     node            - rendered per-node during the walk
*     section_footer  - rendered at the end of each depth-0 group
*     summary         - rendered once after the walk
*     footer          - rendered once at the very end
* BUILT-IN NODE SPECIFIERS:
*     {name}       - element name / description
*     {status}     - status string ("passed", "failed", ...)
*     {symbol}     - status symbol ("[PASS]", "[FAIL]", ...)
*     {depth}      - depth in tree (0-based decimal)
*     {indent}     - repeated indent unit for current depth
*     {number}     - sequential number per numbering mode
*     {line}       - global render-line counter (increments per node)
*     {message}    - element message text
*     {is_leaf}    - "true" or "false"
*     {children}   - direct child count (decimal)
*     {elapsed}    - elapsed time, formatted (auto-scaled units)
*     {elapsed_ns} - elapsed time, integer nanoseconds
*     {elapsed_us} - elapsed time, integer microseconds (truncated)
*     {elapsed_ms} - elapsed time, integer milliseconds (truncated)
*     {elapsed_s}  - elapsed time, two-place decimal seconds
* BUILT-IN SUMMARY SPECIFIERS:
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
* path:      /inc/djinterp/test/test_printer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.04.11
******************************************************************************/

/*
TABLE OF CONTENTS
=================
I.    NUMBERING MODE
II.   INDENT STATE
III.  PRINT CONTEXT
IV.   DEFAULT FORMATS
V.    TEST PRINTER
*/

#ifndef DJINTERP_TEST_PRINTER_
#define DJINTERP_TEST_PRINTER_ 1

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
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
    std::size_t  total;
    std::size_t  passed;
    std::size_t  failed;
    std::size_t  skipped;
    std::size_t  pending;
    std::size_t  errors;
    std::size_t  leaf_number;
    std::size_t  global_number;
    std::size_t  last_depth;
    std::size_t  line_counter;
    std::int64_t elapsed_total_ns;
    std::vector<std::size_t> depth_counters;

    print_context()
        : total(0), passed(0), failed(0),
          skipped(0), pending(0), errors(0),
          leaf_number(0), global_number(0),
          last_depth(0), line_counter(0),
          elapsed_total_ns(0), depth_counters()
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

    // int64_to_string
    //   helper: portable decimal stringification of a signed
    // 64-bit integer.  Casts through long long to satisfy the
    // %lld printf format on every conforming C++11 toolchain.
    static std::string
    int64_to_string(
        std::int64_t _v
    )
    {
        char buf[32];

        std::snprintf(buf, sizeof(buf), "%lld",
                      static_cast<long long>(_v));

        return std::string(buf);
    }

    // advance_line
    //   helper: returns the line number assigned to the
    // about-to-be-rendered node and advances the counter.
    // The first node rendered receives line number 1.
    std::size_t
    advance_line() D_NOEXCEPT
    {
        ++line_counter;

        return line_counter;
    }

    // accumulate_elapsed
    //   helper: adds the supplied nanosecond count to the
    // running elapsed total used by summary / footer
    // template binding.  Saturating clamp at int64 max so
    // pathological inputs cannot wrap.
    void
    accumulate_elapsed(
        std::int64_t _ns
    ) D_NOEXCEPT
    {
        // protect against overflow - clamp instead of wrapping
        if ( (_ns > 0) &&
             (elapsed_total_ns >
                  ( (std::numeric_limits<std::int64_t>::max)() - _ns )) )
        {
            elapsed_total_ns = (std::numeric_limits<std::int64_t>::max)();

            return;
        }

        if ( (_ns < 0) &&
             (elapsed_total_ns <
                  ( (std::numeric_limits<std::int64_t>::min)() - _ns )) )
        {
            elapsed_total_ns = (std::numeric_limits<std::int64_t>::min)();

            return;
        }

        elapsed_total_ns += _ns;

        return;
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

// D_TEST_FMT_NODE_LINE_NUMBERED
//   constant: line-numbered node format with depth-based indent.
static const char* const D_TEST_FMT_NODE_LINE_NUMBERED =
    "{line}: {indent}{symbol} {name}\n";

// D_TEST_FMT_NODE_TIMED
//   constant: depth-indented node format with elapsed time
// suffixed in human-readable form (auto-scaled units).
static const char* const D_TEST_FMT_NODE_TIMED =
    "{indent}{symbol} {name} ({elapsed})\n";

// D_TEST_FMT_NODE_LINE_TIMED
//   constant: combines line numbering, depth-based indent, and
// per-node elapsed time formatted via the configurable
// elapsed-format function.
static const char* const D_TEST_FMT_NODE_LINE_TIMED =
    "{line}: {indent}{symbol} {name} ({elapsed})\n";

// D_TEST_FMT_NODE_FULL
//   constant: maximally-decorated node format with line number,
// per-mode node number, depth indent, status, and elapsed time
// in milliseconds.
static const char* const D_TEST_FMT_NODE_FULL =
    "{line}: {indent}{number}. {symbol} {name}"
    " [{status}] ({elapsed_ms} ms)\n";

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

// D_TEST_FMT_SUMMARY_TIMED
//   constant: summary format that surfaces total elapsed time
// across all rendered nodes alongside the standard counters.
static const char* const D_TEST_FMT_SUMMARY_TIMED =
    "\n{symbol} {passed}/{total} passed"
    ", {failed} failed"
    ", {skipped} skipped"
    " ({pass_rate}) - total elapsed: {elapsed}\n";

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


// default_elapsed_format
//   function: human-readable formatter for a duration expressed
// in nanoseconds.  Auto-scales to the largest unit that yields a
// magnitude >= 1, choosing among ns, us, ms, and s.  Sub-microsecond
// values render as integer nanoseconds; all larger units render
// with two decimal places.
//
// Parameter(s):
//   _ns: signed nanosecond count.  Negative values are formatted
//        with a leading '-' and the same unit-selection logic.
inline std::string
default_elapsed_format(
    std::int64_t _ns
)
{
    char         buf[32];
    bool         negative;
    std::int64_t magnitude;

    negative  = (_ns < 0);
    magnitude = (negative) ? -_ns : _ns;

    // sub-microsecond: render as integer ns
    if (magnitude < 1000)
    {
        std::snprintf(buf,
                      sizeof(buf),
                      "%s%lld ns",
                      (negative) ? "-" : "",
                      static_cast<long long>(magnitude));

        return std::string(buf);
    }

    // sub-millisecond: render as us with two decimals
    if (magnitude < 1000000)
    {
        std::snprintf(buf,
                      sizeof(buf),
                      "%s%.2f us",
                      (negative) ? "-" : "",
                      (static_cast<double>(magnitude) / 1000.0));

        return std::string(buf);
    }

    // sub-second: render as ms with two decimals
    if (magnitude < 1000000000LL)
    {
        std::snprintf(buf,
                      sizeof(buf),
                      "%s%.2f ms",
                      (negative) ? "-" : "",
                      (static_cast<double>(magnitude) / 1000000.0));

        return std::string(buf);
    }

    // seconds with two decimals
    std::snprintf(buf,
                  sizeof(buf),
                  "%s%.2f s",
                  (negative) ? "-" : "",
                  (static_cast<double>(magnitude) / 1000000000.0));

    return std::string(buf);
}


///////////////////////////////////////////////////////////////////////////////
///                V.   TEST PRINTER                                          ///
///////////////////////////////////////////////////////////////////////////////

using print_sink = std::function<void(const char*, std::size_t)>;


// default extractor functors used by the simplified walk overloads
// when the caller declines to supply their own.  These are stateless
// types with templated call operators rather than C++14 generic
// lambdas, so the file remains buildable on C++11 toolchains where
// `auto` lambda parameters are unavailable.  Selection is controlled
// by env.h: on C++14 and later, D_ENV_LANG_IS_CPP14_OR_HIGHER is 1,
// but the functor path is used unconditionally because it is
// behaviourally identical and avoids divergent implementations.
NS_INTERNAL

    // const_zero_size_extractor
    //   trait: stateless extractor that ignores its argument and
    // always returns std::size_t{0}.  Bound as the default depth
    // extractor in the simplified walk overload.
    struct const_zero_size_extractor
    {
        template<typename _Elem>
        std::size_t operator()(const _Elem&) const D_NOEXCEPT
        {
            return 0;
        }
    };

    // const_zero_int64_extractor
    //   trait: stateless extractor that ignores its argument and
    // always returns std::int64_t{0}.  Bound as the default elapsed
    // extractor when the caller supplies no timing source.
    struct const_zero_int64_extractor
    {
        template<typename _Elem>
        std::int64_t operator()(const _Elem&) const D_NOEXCEPT
        {
            return 0;
        }
    };

    // const_true_leaf_extractor
    //   trait: stateless extractor that ignores its argument and
    // always returns true.  Bound as the default leaf-flag
    // extractor in the simplified walk overload (treats every
    // element as a leaf node).
    struct const_true_leaf_extractor
    {
        template<typename _Elem>
        bool operator()(const _Elem&) const D_NOEXCEPT
        {
            return true;
        }
    };

NS_END  // internal


// test_printer
//   class: template-backed tree printer with configurable
// indentation, numbering, line numbering, elapsed-time formatting,
// symbols, and six template sections.
class test_printer
{
public:
    // bound_template
    //   the stateful interpolation surface this printer is written
    // against - a format-owning text_template paired with an
    // accumulating key -> value binding set - re-expressed on top of
    // the lean, construct-once text_template (which parses a format
    // ONCE and takes its bindings per render).  set_format (re)parses;
    // bind / clear_bindings accumulate the binding set (bind overwrites
    // an existing key, else appends); render() interpolates that set
    // through the template.  Values are owned (std::string), so views
    // handed to the template stay valid for the whole render, and
    // temporaries passed to bind() are safe.  One binding set per
    // template preserves the old per-template binding behaviour: a
    // caller still binds e.g. {suite_name} on header_template() and
    // {section_name} on section_footer_template() before the matching
    // print_* call.
    class bound_template
    {
    public:
        bound_template() = default;

        explicit bound_template(
            std::string _format
        )
            : m_tmpl(static_cast<std::string&&>(_format)),
              m_bindings()
        {}

        // set_format -- (re)parse a new format string.
        void
        set_format(
            std::string _format
        )
        {
            m_tmpl = text_template<>(static_cast<std::string&&>(_format));

            return;
        }

        // format -- the current (owned) format string.
        D_NODISCARD const std::string&
        format() const D_NOEXCEPT
        {
            return m_tmpl.format();
        }

        // empty -- true iff the format string is empty.
        D_NODISCARD bool
        empty() const
        {
            return m_tmpl.empty();
        }

        // clear_bindings -- drop all bindings (call before a fresh set).
        void
        clear_bindings() D_NOEXCEPT
        {
            m_bindings.clear();

            return;
        }

        // bind -- record a key -> value pair.  Both are copied into
        // owned storage, so a temporary passed as either is safe.  An
        // existing key is overwritten, so re-binding across renders
        // (e.g. per-section counter refresh) yields the current value.
        void
        bind(
            std::string_view _key,
            std::string_view _value
        )
        {
            for (std::pair<std::string, std::string>& _entry : m_bindings)
            {
                if (std::string_view(_entry.first) == _key)
                {
                    _entry.second.assign(_value.data(), _value.size());

                    return;
                }
            }

            m_bindings.emplace_back(std::string(_key), std::string(_value));

            return;
        }

        // render -- interpolate the accumulated bindings through the
        // template.  Missing keys resolve to the empty view, exactly as
        // the template's built-in inline lookup does.
        D_NODISCARD std::string
        render() const
        {
            return m_tmpl.render(
                [this](std::string_view _key) -> std::string_view
                {
                    for (const std::pair<std::string, std::string>& _entry
                             : m_bindings)
                    {
                        if (std::string_view(_entry.first) == _key)
                        {
                            return std::string_view(_entry.second);
                        }
                    }

                    return std::string_view{};
                });
        }

        // tmpl -- the underlying parsed template (keys / inspection).
        D_NODISCARD text_template<>&
        tmpl() D_NOEXCEPT
        {
            return m_tmpl;
        }

        D_NODISCARD const text_template<>&
        tmpl() const D_NOEXCEPT
        {
            return m_tmpl;
        }

    private:
        text_template<>                                  m_tmpl;
        std::vector<std::pair<std::string, std::string>> m_bindings;
    };

    using symbol_fn_type         = std::function<std::string(test_status)>;
    using status_fn_type         = std::function<std::string(test_status)>;
    using filter_fn_type         = std::function<bool(test_status,
                                                      std::size_t)>;
    using binder_fn_type         = std::function<void(bound_template&,
                                                      std::size_t)>;
    using elapsed_format_fn_type = std::function<std::string(std::int64_t)>;

    // -----------------------------------------------------------------
    //  construction
    // -----------------------------------------------------------------

    test_printer()
        : m_node_tmpl(D_TEST_FMT_NODE_DEFAULT),
          m_summary_tmpl(D_TEST_FMT_SUMMARY_DEFAULT),
          m_header_tmpl(""),
          m_footer_tmpl(""),
          m_sec_hdr_tmpl(""),
          m_sec_ftr_tmpl(""),
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
          m_elapsed_format_fn(default_elapsed_format),
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

    void set_node_format(const std::string& _f)           { m_node_fmt = _f;    m_node_tmpl.set_format(_f);    return; }
    void set_summary_format(const std::string& _f)        { m_summary_fmt = _f; m_summary_tmpl.set_format(_f); return; }
    void set_header_format(const std::string& _f)         { m_header_fmt = _f;  m_header_tmpl.set_format(_f);  return; }
    void set_footer_format(const std::string& _f)         { m_footer_fmt = _f;  m_footer_tmpl.set_format(_f);  return; }
    void set_section_header_format(const std::string& _f) { m_sec_hdr_fmt = _f; m_sec_hdr_tmpl.set_format(_f); return; }
    void set_section_footer_format(const std::string& _f) { m_sec_ftr_fmt = _f; m_sec_ftr_tmpl.set_format(_f); return; }

    const std::string& node_format()           const D_NOEXCEPT { return m_node_fmt; }
    const std::string& summary_format()        const D_NOEXCEPT { return m_summary_fmt; }
    const std::string& header_format()         const D_NOEXCEPT { return m_header_fmt; }
    const std::string& footer_format()         const D_NOEXCEPT { return m_footer_fmt; }
    const std::string& section_header_format() const D_NOEXCEPT { return m_sec_hdr_fmt; }
    const std::string& section_footer_format() const D_NOEXCEPT { return m_sec_ftr_fmt; }


    // =================================================================
    //  template access
    // =================================================================

    bound_template&       node_template()                 D_NOEXCEPT { return m_node_tmpl; }
    const bound_template& node_template()           const D_NOEXCEPT { return m_node_tmpl; }
    bound_template&       summary_template()              D_NOEXCEPT { return m_summary_tmpl; }
    const bound_template& summary_template()        const D_NOEXCEPT { return m_summary_tmpl; }
    bound_template&       header_template()               D_NOEXCEPT { return m_header_tmpl; }
    const bound_template& header_template()         const D_NOEXCEPT { return m_header_tmpl; }
    bound_template&       footer_template()               D_NOEXCEPT { return m_footer_tmpl; }
    const bound_template& footer_template()         const D_NOEXCEPT { return m_footer_tmpl; }
    bound_template&       section_header_template()       D_NOEXCEPT { return m_sec_hdr_tmpl; }
    const bound_template& section_header_template() const D_NOEXCEPT { return m_sec_hdr_tmpl; }
    bound_template&       section_footer_template()       D_NOEXCEPT { return m_sec_ftr_tmpl; }
    const bound_template& section_footer_template() const D_NOEXCEPT { return m_sec_ftr_tmpl; }


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
    //  elapsed-time formatting
    // =================================================================

    // elapsed_format_function
    //   accessor: returns the active formatter used to resolve the
    // {elapsed} specifier.  Receives a signed nanosecond count and
    // returns a display string.
    const elapsed_format_fn_type&
    elapsed_format_function() const D_NOEXCEPT
    {
        return m_elapsed_format_fn;
    }

    // set_elapsed_format_function
    //   mutator: installs a custom formatter for the {elapsed}
    // specifier.  Pass an empty std::function to fall back to
    // default_elapsed_format on the next render.
    void
    set_elapsed_format_function(
        elapsed_format_fn_type _fn
    )
    {
        m_elapsed_format_fn =
            static_cast<elapsed_format_fn_type&&>(_fn);

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
            emit(m_header_tmpl.render());
        }

        return;
    }

    void
    print_footer() const
    {
        if (!m_footer_fmt.empty())
        {
            emit(m_footer_tmpl.render());
        }

        return;
    }

    void
    print_section_header() const
    {
        if (!m_sec_hdr_fmt.empty())
        {
            emit(m_sec_hdr_tmpl.render());
        }

        return;
    }

    void
    print_section_footer() const
    {
        if (!m_sec_ftr_fmt.empty())
        {
            bind_context_to(m_sec_ftr_tmpl, m_context);
            emit(m_sec_ftr_tmpl.render());
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
        std::size_t        _number,
        std::int64_t       _elapsed_ns = 0
    ) const
    {
        render_node(
            _status, _name, _message,
            _depth, true, 0,
            print_context::size_to_string(_number),
            _elapsed_ns);

        return;
    }


    // =================================================================
    //  rendering: for-each walk
    // =================================================================

    // walk
    //   iterates an iterable container, rendering each element
    // through the node template with depth-driven indentation,
    // line numbering, and per-node elapsed-time binding.
    //
    // Extraction functions:
    //   _name_fn(elem)    -> string
    //   _msg_fn(elem)     -> string
    //   _depth_fn(elem)   -> size_t
    //   _status_fn(elem)  -> test_status
    //   _leaf_fn(elem)    -> bool
    //   _elapsed_fn(elem) -> std::int64_t  (nanoseconds)
    template<typename _Container,
             typename _NameFn,
             typename _MsgFn,
             typename _DepthFn,
             typename _StatusFn,
             typename _LeafFn,
             typename _ElapsedFn>
    void
    walk(
        const _Container& _elements,
        _NameFn&&         _name_fn,
        _MsgFn&&          _msg_fn,
        _DepthFn&&        _depth_fn,
        _StatusFn&&       _status_fn,
        _LeafFn&&         _leaf_fn,
        _ElapsedFn&&      _elapsed_fn,
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

        // walk every element in arrival order, accumulating counters
        // and rendering through the node template
        for (const auto& elem : _elements)
        {
            test_status  s     = _status_fn(elem);
            std::string  name  = _name_fn(elem);
            std::string  msg   = _msg_fn(elem);
            std::size_t  depth = _depth_fn(elem);
            bool         leaf  = _leaf_fn(elem);
            std::int64_t ns    =
                static_cast<std::int64_t>(_elapsed_fn(elem));

            m_context.accumulate(s);

            std::string num = m_context.next_number(
                m_numbering, depth, leaf);

            if (!should_print(s, depth))
            {
                continue;
            }

            render_node(s, name, msg, depth, leaf, 0, num, ns);
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

    // walk (no elapsed extractor)
    //   delegates to the elapsed-aware walk with a zero-elapsed
    // extractor.  Preserves the pre-existing six-extractor API for
    // callers that have not yet wired in timing.
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
        walk(
            _elements,
            static_cast<_NameFn&&>(_name_fn),
            static_cast<_MsgFn&&>(_msg_fn),
            static_cast<_DepthFn&&>(_depth_fn),
            static_cast<_StatusFn&&>(_status_fn),
            static_cast<_LeafFn&&>(_leaf_fn),
            internal::const_zero_int64_extractor(),
            _with_header,
            _with_summary,
            _with_footer);

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
            internal::const_zero_size_extractor(),
            static_cast<_StatusFn&&>(_status_fn),
            internal::const_true_leaf_extractor());

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

        emit(m_summary_tmpl.render());

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

    // format_seconds
    //   helper: renders a nanosecond count as a two-place
    // decimal seconds string (e.g. "1.23").  Used to bind the
    // {elapsed_s} specifier without introducing dependency on
    // <chrono>.  Sign is preserved.
    static std::string
    format_seconds(
        std::int64_t _ns
    )
    {
        char buf[32];

        std::snprintf(buf,
                      sizeof(buf),
                      "%.2f",
                      (static_cast<double>(_ns) / 1000000000.0));

        return std::string(buf);
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
        const std::string& _num_str,
        std::int64_t       _elapsed_ns
    ) const
    {
        if (m_node_fmt.empty())
        {
            return;
        }

        // build the depth-driven indent string before binding so it
        // stays bound to the depth this caller asked us to render
        m_indent.set_depth(_depth);
        std::string indent_str = m_indent.build();

        // assign the rendering line number BEFORE emit so the format
        // string sees the correct value for this node's first line
        std::size_t line_no = m_context.advance_line();

        // accumulate elapsed for the section / summary aggregates
        m_context.accumulate_elapsed(_elapsed_ns);

        // resolve the human-readable elapsed string, falling back to
        // the built-in formatter if the user-supplied one is empty
        std::string elapsed_str =
            (m_elapsed_format_fn)
                ? m_elapsed_format_fn(_elapsed_ns)
                : default_elapsed_format(_elapsed_ns);

        m_node_tmpl.clear_bindings();

        m_node_tmpl.bind("name",       _name);
        m_node_tmpl.bind("status",     m_status_fn(_status));
        m_node_tmpl.bind("symbol",     m_symbol_fn(_status));
        m_node_tmpl.bind("depth",
                         print_context::size_to_string(_depth));
        m_node_tmpl.bind("indent",     indent_str);
        m_node_tmpl.bind("number",     _num_str);
        m_node_tmpl.bind("line",
                         print_context::size_to_string(line_no));
        m_node_tmpl.bind("message",    _message);
        m_node_tmpl.bind("is_leaf",    _is_leaf ? "true" : "false");
        m_node_tmpl.bind("children",
                         print_context::size_to_string(_child_count));
        m_node_tmpl.bind("elapsed",    elapsed_str);
        m_node_tmpl.bind("elapsed_ns",
                         print_context::int64_to_string(_elapsed_ns));
        m_node_tmpl.bind("elapsed_us",
                         print_context::int64_to_string(_elapsed_ns / 1000LL));
        m_node_tmpl.bind("elapsed_ms",
                         print_context::int64_to_string(
                             _elapsed_ns / 1000000LL));
        m_node_tmpl.bind("elapsed_s",  format_seconds(_elapsed_ns));

        if (m_node_binder)
        {
            m_node_binder(m_node_tmpl, _depth);
        }

        emit(m_node_tmpl.render());

        return;
    }


    // =================================================================
    //  internal: bind context
    // =================================================================

    // bind_context_to
    //   helper: populates the standard counter / aggregate
    // specifiers shared by header, footer, section, and summary
    // templates from a print_context snapshot.  This includes the
    // total-elapsed and line-count aggregates so summaries can
    // render the {elapsed} and {line} specifiers as suite-wide
    // values rather than per-node.
    void
    bind_context_to(
        bound_template&      _tmpl,
        const print_context& _ctx
    ) const
    {
        _tmpl.bind("total",
                   print_context::size_to_string(_ctx.total));
        _tmpl.bind("passed",
                   print_context::size_to_string(_ctx.passed));
        _tmpl.bind("failed",
                   print_context::size_to_string(_ctx.failed));
        _tmpl.bind("skipped",
                   print_context::size_to_string(_ctx.skipped));
        _tmpl.bind("pending",
                   print_context::size_to_string(_ctx.pending));
        _tmpl.bind("errors",
                   print_context::size_to_string(_ctx.errors));
        _tmpl.bind("pass_rate",  _ctx.pass_rate());
        _tmpl.bind("line",
                   print_context::size_to_string(_ctx.line_counter));

        // elapsed-time aggregate: {elapsed} resolves through the
        // configured formatter (or default), with fixed-unit
        // companions for callers that need raw integers
        std::string elapsed_str =
            (m_elapsed_format_fn)
                ? m_elapsed_format_fn(_ctx.elapsed_total_ns)
                : default_elapsed_format(_ctx.elapsed_total_ns);

        _tmpl.bind("elapsed",    elapsed_str);
        _tmpl.bind("elapsed_ns",
                   print_context::int64_to_string(
                       _ctx.elapsed_total_ns));
        _tmpl.bind("elapsed_us",
                   print_context::int64_to_string(
                       _ctx.elapsed_total_ns / 1000LL));
        _tmpl.bind("elapsed_ms",
                   print_context::int64_to_string(
                       _ctx.elapsed_total_ns / 1000000LL));
        _tmpl.bind("elapsed_s",
                   format_seconds(_ctx.elapsed_total_ns));

        std::string sym =
            ( ((_ctx.failed > 0) || (_ctx.errors > 0))
                ? default_symbol(test_status::failed)
                : default_symbol(test_status::passed) );

        _tmpl.bind("symbol",     sym);

        _tmpl.bind("status_word",
            ( ((_ctx.failed > 0) || (_ctx.errors > 0))
                ? "FAILED" : "PASSED" ));

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

    mutable bound_template m_node_tmpl;
    mutable bound_template m_summary_tmpl;
    mutable bound_template m_header_tmpl;
    mutable bound_template m_footer_tmpl;
    mutable bound_template m_sec_hdr_tmpl;
    mutable bound_template m_sec_ftr_tmpl;

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

    symbol_fn_type         m_symbol_fn;
    status_fn_type         m_status_fn;
    elapsed_format_fn_type m_elapsed_format_fn;
    filter_fn_type         m_filter;
    binder_fn_type         m_node_binder;

    mutable print_context m_context;
    print_sink            m_sink;
};

// configure_report_printer
//   function: presets a test_printer for the module-report console style -
// the banner header, the dashed module section header and results footer, the
// full assertion summary, and a two-space-indented "[SYM] name" node line with
// numbering off.  The per-section keys the templates reference (suite_name /
// suite_description for the header; section_name / section_description for the
// section header and footer) are still bound by the caller on the matching
// template before each print_* call - this helper only installs the formats.
//
//   This is the report-shaped counterpart to the printer's default (flat)
// presets, and the seam through which a caller can drive the report console
// through test_printer directly rather than through report_builder.
//
// Parameter(s):
//   _printer: the printer to configure (its sink and other settings are left
//             untouched).
// Return:
//   none.
D_INLINE void
configure_report_printer(
    test_printer& _printer
)
{
    _printer.set_header_format(D_TEST_FMT_HEADER_BANNER);
    _printer.set_section_header_format(D_TEST_FMT_SECTION_HEADER_DASHED);
    _printer.set_section_footer_format(D_TEST_FMT_SECTION_FOOTER_RESULTS);
    _printer.set_summary_format(D_TEST_FMT_SUMMARY_FULL);
    _printer.set_node_format("  {symbol} {name}\n");
    _printer.set_indent("  ", 16);
    _printer.set_numbering(numbering_mode::none);

    return;
}



NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_PRINTER_