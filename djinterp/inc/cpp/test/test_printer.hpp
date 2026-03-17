/******************************************************************************
* djinterp [test]                                          test_printer.hpp
*
* Format-string-based printer for the test tree.
*   Walks a test_tree depth-first and renders each node through
* user-configurable format strings with `{key}` specifier
* substitution. Output is directed to a pluggable sink function,
* allowing writes to console (stdout), string buffer, file, or
* any custom destination.
*
* BUILT-IN SPECIFIERS:
*   {name}         node name
*   {id}           node test_id (decimal)
*   {rank}         node rank (decimal)
*   {status}       status string ("passed", "failed", etc.)
*   {symbol}       status symbol ([PASS], [FAIL], emoji, etc.)
*   {depth}        node depth in tree (0-based)
*   {indent}       repeated indent string for current depth
*   {duration}     elapsed time in ms (e.g. "12.34")
*   {message}      node message text
*   {children}     number of direct children
*   {subtree}      total nodes in subtree
*
* USAGE:
*   test_printer printer;
*   printer.set_node_format(
*       "{indent}{symbol} {name} ({duration}ms)\n");
*   printer.set_summary_format(
*       "\n{symbol} {passed}/{total} passed\n");
*   printer.print_tree(tree);
*
*   // or with custom sink:
*   std::string buf;
*   printer.set_sink([&](const char* s, std::size_t n)
*       { buf.append(s, n); });
*   printer.print_tree(tree);
*
* COMPONENTS:
*   djinterp::test::print_sink        - output callback type
*   djinterp::test::test_printer      - configurable tree printer
*
* PORTABLE ACROSS:
*   C++11, C++14, C++17, C++20, C++23, C++26
*
*
* path:      /inc/cpp/test/test_printer.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                          date: 2026.03.14
******************************************************************************/

#ifndef DJINTERP_TEST_PRINTER_
#define DJINTERP_TEST_PRINTER_ 1

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <string>

#include "test_tree.hpp"
#include "test_options.hpp"


NS_DJINTERP
NS_TEST


// =========================================================================
// I.   TYPES
// =========================================================================

// print_sink
//   type: callback that receives formatted output. The first
// parameter is a pointer to the text, the second is the byte
// count. Default sink writes to stdout.
using print_sink = std::function<void(const char*, std::size_t)>;


// =========================================================================
// II.  PRINT CONTEXT (internal)
// =========================================================================

NS_INTERNAL

    // print_context
    //   struct: aggregated counters accumulated during a tree
    // walk for use in summary format strings.
    struct print_context
    {
        std::size_t total;
        std::size_t passed;
        std::size_t failed;
        std::size_t skipped;
        std::size_t unknown;
        double      total_duration_ms;

        print_context()
            : total(0),
              passed(0),
              failed(0),
              skipped(0),
              unknown(0),
              total_duration_ms(0.0)
        {
        };
    };

    // to_string_int
    //   converts an integer to a decimal string.
    D_INLINE std::string
    to_string_int
    (
        std::int64_t _value
    )
    {
        char buf[32];

        std::snprintf(buf, 
                      sizeof(buf),
                      "%lld",
                      (long long)_value);

        return std::string(buf);
    }

    // to_string_uint
    //   converts an unsigned integer to a decimal string.
    D_INLINE std::string
    to_string_uint
    (
        std::uint64_t _value
    )
    {
        char buf[32];
        std::snprintf(buf, 
                      sizeof(buf),
                      "%llu",
                      (unsigned long long)_value);

        return std::string(buf);
    }

    // to_string_double
    //   converts a double to a string with 2 decimal places.
    D_INLINE std::string
    to_string_double
    (
        double _value
    )
    {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.2f", _value);

        return std::string(buf);
    }

    // format_expand
    //   performs {key} specifier substitution on _format. For
    // each `{key}` found, invokes _resolver(key) which returns
    // the replacement string. Unknown keys are left as-is.
    template<typename _Resolver>
    D_INLINE std::string
    format_expand
    (
        const std::string& _format,
        _Resolver&&        _resolver
    )
    {
        std::string out;
        out.reserve(_format.size());

        std::size_t i = 0;

        while (i < _format.size())
        {
            // look for '{'
            if (_format[i] == '{')
            {
                std::size_t close = _format.find('}', i + 1);

                if (close != std::string::npos)
                {
                    std::string key =
                        _format.substr(i + 1, close - i - 1);

                    std::string value = _resolver(key);

                    // if resolver returned empty and key is
                    // not a known key, preserve the original
                    // token
                    out += value;
                    i = close + 1;

                    continue;
                }
            }

            out += _format[i];
            ++i;
        }

        return out;
    }

    // build_indent
    //   repeats _str for _depth levels, clamped to _max_depth.
    D_INLINE std::string
    build_indent
    (
        const std::string& _str,
        std::size_t        _depth,
        std::size_t        _max_depth
    )
    {
        std::size_t levels =
            (_depth < _max_depth) ? _depth : _max_depth;

        std::string out;
        out.reserve(_str.size() * levels);

        for (std::size_t i = 0; i < levels; ++i)
        {
            out += _str;
        }

        return out;
    }

NS_END  // internal


// =========================================================================
// III. DEFAULT FORMAT STRINGS
// =========================================================================

// D_TEST_FORMAT_NODE_DEFAULT
//   default format for each node line.
static const char* D_TEST_FORMAT_NODE_DEFAULT =
    "{indent}{symbol} {name}\n";

// D_TEST_FORMAT_NODE_VERBOSE
//   verbose format including duration and id.
static const char* D_TEST_FORMAT_NODE_VERBOSE =
    "{indent}{symbol} {name} (id:{id} rank:{rank}"
    " {duration}ms)\n";

// D_TEST_FORMAT_NODE_MINIMAL
//   minimal format: failures only (used with rank filter).
static const char* D_TEST_FORMAT_NODE_MINIMAL =
    "{indent}{symbol} {name}\n";

// D_TEST_FORMAT_SUMMARY_DEFAULT
//   default summary format.
static const char* D_TEST_FORMAT_SUMMARY_DEFAULT =
    "\n{passed}/{total} passed, {failed} failed"
    ", {skipped} skipped ({total_duration}ms)\n";


// =========================================================================
// IV.  TEST PRINTER
// =========================================================================

// test_printer
//   class: configurable tree printer. Walks a test_tree
// depth-first, renders each node through a format string
// with `{key}` specifier substitution, and writes the
// result to a pluggable output sink.
class test_printer
{
public:
    test_printer()
        : m_node_format(D_TEST_FORMAT_NODE_DEFAULT)
        , m_summary_format(D_TEST_FORMAT_SUMMARY_DEFAULT)
        , m_indent_str("  ")
        , m_indent_max(10)
        , m_min_rank(-1)
        , m_print_passing(true)
        , m_print_skipped(true)
        , m_print_summary(true)
        , m_sink()
    {
        // default sink: stdout
        m_sink = [](const char* _data, std::size_t _len)
        {
            std::fwrite(_data, 1, _len, stdout);
        };
    };

    // ---- format configuration ----

    // set_node_format
    //   sets the format string used for each node.
    void set_node_format(const std::string& _fmt)
    {
        m_node_format = _fmt;
    };

    // set_summary_format
    //   sets the format string used for the summary line.
    void set_summary_format(const std::string& _fmt)
    {
        m_summary_format = _fmt;
    };

    // set_indent
    //   sets the indentation string and max depth.
    void set_indent(const std::string& _str,
                    std::size_t        _max_depth = 10)
    {
        m_indent_str = _str;
        m_indent_max = _max_depth;
    };

    // ---- filter configuration ----

    // set_min_rank
    //   sets the minimum rank for a node to be printed.
    // Nodes with rank < min_rank are skipped. Set to -1
    // (default) to print all nodes.
    void set_min_rank(std::int32_t _rank)
    {
        m_min_rank = _rank;
    };

    // set_print_passing
    //   controls whether passing nodes are printed.
    void set_print_passing(bool _print)
    {
        m_print_passing = _print;
    };

    // set_print_skipped
    //   controls whether skipped nodes are printed.
    void set_print_skipped(bool _print)
    {
        m_print_skipped = _print;
    };

    // set_print_summary
    //   controls whether the summary line is printed.
    void set_print_summary(bool _print)
    {
        m_print_summary = _print;
    };

    // ---- output sink ----

    // set_sink
    //   sets the output callback. The default writes to
    // stdout.
    void set_sink(print_sink _sink)
    {
        m_sink = std::move(_sink);
    };

    // set_sink_stdout
    //   resets the sink to stdout.
    void set_sink_stdout()
    {
        m_sink = [](const char* _data, std::size_t _len)
        {
            std::fwrite(_data, 1, _len, stdout);
        };
    };

    // set_sink_string
    //   sets the sink to append to the given string.
    void set_sink_string(std::string& _out)
    {
        m_sink = [&_out](const char* _data,
                         std::size_t _len)
        {
            _out.append(_data, _len);
        };
    };

    // set_sink_file
    //   sets the sink to write to the given FILE*.
    void set_sink_file(FILE* _file)
    {
        m_sink = [_file](const char* _data,
                         std::size_t _len)
        {
            std::fwrite(_data, 1, _len, _file);
        };
    };

    // ---- configure from test_options ----

    // configure
    //   reads output-related options from a test_options
    // instance and applies them to this printer.
    void configure(const test_options& _opts)
    {
        std::int64_t verbosity =
            _opts.get_int(D_TEST_OPT_VERBOSITY, 2);

        // select format based on verbosity
        if (verbosity <= 0)
        {
            m_node_format    = "";
            m_print_summary  = false;
            m_print_passing  = false;
            m_print_skipped  = false;
        }
        else if (verbosity == 1)
        {
            m_node_format    = D_TEST_FORMAT_NODE_MINIMAL;
            m_print_passing  = false;
            m_print_skipped  = false;
        }
        else if (verbosity >= 3)
        {
            m_node_format = D_TEST_FORMAT_NODE_VERBOSE;
        }

        // indent
        const std::string& indent_str =
            _opts.get_string(D_TEST_OPT_INDENT_STR);

        if (!indent_str.empty())
        {
            m_indent_str = indent_str;
        }

        m_indent_max = static_cast<std::size_t>(
            _opts.get_uint(D_TEST_OPT_INDENT_MAX_LEVEL, 10));

        // reporting flags
        m_print_passing =
            _opts.get_bool(D_TEST_OPT_REPORT_PASSED, true);
        m_print_skipped =
            _opts.get_bool(D_TEST_OPT_REPORT_SKIPPED, true);
        m_print_summary =
            _opts.get_bool(D_TEST_OPT_REPORT_SUMMARY, true);
    };

    // ---- printing ----

    // print_node
    //   renders a single node through the node format string
    // and writes it to the sink.
    void print_node(const test_node& _node,
                    std::size_t      _depth) const
    {
        // apply filters
        if ( (m_min_rank >= 0) &&
             (_node.rank < m_min_rank) )
        {
            return;
        }

        if ( (!m_print_passing) &&
             (is_passing(_node.status)) )
        {
            return;
        }

        if ( (!m_print_skipped) &&
             (_node.status == D_TEST_STATUS_SKIPPED) )
        {
            return;
        }

        if (m_node_format.empty())
        {
            return;
        }

        // build indent
        std::string indent = internal::build_indent(
            m_indent_str, _depth, m_indent_max);

        // resolve specifiers
        const test_node& node = _node;
        std::size_t depth     = _depth;

        std::string line = internal::format_expand(
            m_node_format,
            [&](const std::string& _key) -> std::string
            {
                if (_key == "name")
                {
                    return node.name;
                }

                if (_key == "id")
                {
                    return internal::to_string_uint(node.id);
                }

                if (_key == "rank")
                {
                    return internal::to_string_int(node.rank);
                }

                if (_key == "status")
                {
                    return status_to_string(node.status);
                }

                if (_key == "symbol")
                {
                    return result_symbol(node.status);
                }

                if (_key == "depth")
                {
                    return internal::to_string_uint(depth);
                }

                if (_key == "indent")
                {
                    return indent;
                }

                if (_key == "duration")
                {
                    return internal::to_string_double(
                        node.duration_ms);
                }

                if (_key == "message")
                {
                    return node.message;
                }

                if (_key == "children")
                {
                    return internal::to_string_uint(
                        node.child_count());
                }

                if (_key == "subtree")
                {
                    return internal::to_string_uint(
                        node.subtree_size());
                }

                // unknown key: preserve token
                return "{" + _key + "}";
            });

        write(line);
    };

    // print_tree
    //   walks the tree depth-first, printing each node, then
    // optionally prints a summary line.
    void print_tree(const test_tree& _tree) const
    {
        if (_tree.empty())
        {
            return;
        }

        // accumulate stats
        internal::print_context ctx;

        _tree.for_each(
            [&](const test_node& _node,
                std::size_t      _depth)
            {
                // print the node
                print_node(_node, _depth);

                // accumulate
                ++ctx.total;
                ctx.total_duration_ms += _node.duration_ms;

                if (is_passing(_node.status))
                {
                    ++ctx.passed;
                }
                else if (is_failing(_node.status))
                {
                    ++ctx.failed;
                }
                else if (_node.status == D_TEST_STATUS_SKIPPED)
                {
                    ++ctx.skipped;
                }
                else
                {
                    ++ctx.unknown;
                }
            });

        // print summary
        if (m_print_summary)
        {
            print_summary(ctx);
        }
    };

    // print_summary
    //   renders the summary format string with accumulated
    // counters and writes it to the sink.
    void print_summary(
        const internal::print_context& _ctx) const
    {
        if (m_summary_format.empty())
        {
            return;
        }

        // determine overall symbol
        const char* overall_symbol =
            (_ctx.failed > 0)
                ? test_symbols::fail
                : test_symbols::success;

        const internal::print_context& ctx = _ctx;

        std::string line = internal::format_expand(
            m_summary_format,
            [&](const std::string& _key) -> std::string
            {
                if (_key == "total")
                {
                    return internal::to_string_uint(ctx.total);
                }

                if (_key == "passed")
                {
                    return internal::to_string_uint(
                        ctx.passed);
                }

                if (_key == "failed")
                {
                    return internal::to_string_uint(
                        ctx.failed);
                }

                if (_key == "skipped")
                {
                    return internal::to_string_uint(
                        ctx.skipped);
                }

                if (_key == "unknown")
                {
                    return internal::to_string_uint(
                        ctx.unknown);
                }

                if (_key == "total_duration")
                {
                    return internal::to_string_double(
                        ctx.total_duration_ms);
                }

                if (_key == "symbol")
                {
                    return overall_symbol;
                }

                return "{" + _key + "}";
            });

        write(line);
    };

private:
    // write
    //   sends text to the sink.
    void write(const std::string& _text) const
    {
        if ( (m_sink) &&
             (!_text.empty()) )
        {
            m_sink(_text.data(), _text.size());
        }
    };

    std::string   m_node_format;
    std::string   m_summary_format;
    std::string   m_indent_str;
    std::size_t   m_indent_max;
    std::int32_t  m_min_rank;
    bool          m_print_passing;
    bool          m_print_skipped;
    bool          m_print_summary;
    print_sink    m_sink;
};


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_PRINTER_
