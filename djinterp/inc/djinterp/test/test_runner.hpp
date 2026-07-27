/******************************************************************************
* djinterp [test]                                              test_runner.hpp
*
*   The top-level RUN facade for DTest -- the single object that composes the
* pieces so running a suite is one call.  A test_runner owns a test_handler
* (the tree walker + event dispatcher), a test_option_set (the resolved
* configuration), and an option-driven reporter bundle bound to the event
* alphabet (test_event.hpp).  Hand it a tree, call run(), read the verdict.
*
*   WHAT IT COMPOSES:
*     - test_handler   : walks the tree, fires lifecycle events, tallies.
*     - test_options   : the knobs + per-node routes; the runner RESOLVES each
*                        node against them and routes its line accordingly.
*     - test_event     : the tags the reporter binds to (on_test_end,
*                        on_module_start, on_session_start / on_session_end).
*   The reporter is "a listener through and through": the runner installs a
* bundle on the handler's dispatcher whose bodies render and route report
* lines.  Because the reporter reads the option set at FIRE time (not bind
* time), options may be changed up to the moment run() is called.
*
*   WHERE OPTIONS MEET EVENTS:
*   For every leaf, the reporter builds a match_context from the node (name /
* type id / status, and tags parsed from a "tags" metadata key when present)
* and calls the free resolve(...).  The resolved verdict decides
* whether the line is shown (the show policy), how it is formatted (the
* possibly route-overridden format template), and which sink(s) it reaches
* (console and/or file, with per-node add/replace routing).  This is the same
* resolution the option subframework defines, applied one node at a time as
* the walk dispatches.
*
*   THE CINCH:
*     // runtime options
*     djinterp::test::run_tests(my_tree);                       // -> exit code
*     djinterp::test::run_tests(my_tree, my_options);
*
*     // or hold a runner for finer control
*     djinterp::test::test_runner runner;
*     runner.handler().on<on_test_failed>( ... );   // extra listeners
*     auto result = runner.run(my_tree);
*     return runner.exit_code();
*
*   WHAT IT REPORTS:
*   The event payloads carry the node only (no tree depth), so the built-in
* reporter numbers leaves ordinally and emits one line per leaf plus a closing
* summary.  Hierarchical numbering / depth indentation would need a
* depth-carrying event and are intentionally left to a richer reporter; the
* runner exposes handler() so such a reporter can be bound alongside.  Line
* width is clamped here; production layout (alignment / word-wrap under
* line_width) composes the cli_string algebra.
*
*   PORTABILITY:
*   The runner is C++11 throughout -- it drives the runtime test_option_set and
* the free resolve() the option subframework defines.
*
*
* TABLE OF CONTENTS
* =================
* I.    REPORTER HELPERS         (status / context / interpolation - internal)
* II.   TEST RUNNER              (the facade)
* III.  FREE-FUNCTION RUNNERS    (run_tests overloads + schema convenience)
*
*
* path:      /inc/djinterp/test/test_runner.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.20
******************************************************************************/

#ifndef DJINTERP_TEST_RUNNER_
#define DJINTERP_TEST_RUNNER_ 1

// std
#include <cstddef>
#include <fstream>
#include <ostream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"
#include "./test_common.hpp"     // test_status
#include "./test_event.hpp"      //  alphabet
#include "./test_object.hpp"     // basic_test
#include "./test_options.hpp"    // test_option_set, test_resolved, match_context, resolve
#include "./test_handler.hpp"    // test_handler, session_result, session_verdict, handler_id
#include "./test_pack.hpp"       // pack_report, pack_enabled (report packaging)


NS_DJINTERP
NS_TEST


// =========================================================================
// I.   REPORTER HELPERS
// =========================================================================

NS_INTERNAL

    // runner_status_of_helper
    //   helper: map a node's numeric status_type onto the framework
    // test_status enum (the same mapping the handler uses).
    D_INLINE test_status
    runner_status_of_helper(
        basic_test::status_type _raw
    )
    {
        switch (_raw)
        {
            case basic_test::status_passed:  return test_status::passed;
            case basic_test::status_failed:  return test_status::failed;
            case basic_test::status_skipped: return test_status::skipped;
            case basic_test::status_pending: return test_status::pending;
            case basic_test::status_error:   return test_status::error;
            default:                         return test_status::pending;
        }
    }

    // runner_status_str_helper
    //   helper: a fixed-width display token for a status.
    D_INLINE const char*
    runner_status_str_helper(
        test_status _status
    )
    {
        switch (_status)
        {
            case test_status::passed:  return "PASS";
            case test_status::failed:  return "FAIL";
            case test_status::skipped: return "SKIP";
            case test_status::pending: return "PEND";
            case test_status::error:   return "ERR ";
        }

        return "?";
    }

    // runner_shown_helper
    //   helper: whether a leaf of the given status is emitted under the show
    // policy (errors count as failures for display).
    D_INLINE bool
    runner_shown_helper(
        test_show   _show,
        test_status _status
    )
    {
        switch (_show)
        {
            case test_show::all:
            {
                return true;
            }

            case test_show::failures_only:
            {
                return ( (_status == test_status::failed) ||
                         (_status == test_status::error) );
            }

            case test_show::failures_and_skipped:
            {
                return ( (_status == test_status::failed) ||
                         (_status == test_status::error)  ||
                         (_status == test_status::skipped) );
            }

            case test_show::summary_only:
            case test_show::silent:
            {
                return false;
            }
        }

        return true;
    }

    // runner_subst_helper
    //   helper: replace every occurrence of _key in _text with _value.
    D_INLINE void
    runner_subst_helper(
        std::string&       _text,
        const char*        _key,
        const std::string& _value
    )
    {
        std::string            key(_key);
        std::string::size_type p = 0;

        if (key.empty())
        {
            return;
        }

        while ((p = _text.find(key, p)) != std::string::npos)
        {
            _text.replace(p, key.size(), _value);
            p += _value.size();
        }

        return;
    }

    // runner_split_tags_helper
    //   helper: split a comma-separated tag string into a vector, trimming
    // surrounding spaces.  Lets a node expose tags through a "tags" metadata
    // entry so tag-matching routes resolve through the event path.
    D_INLINE std::vector<std::string>
    runner_split_tags_helper(
        const std::string& _csv
    )
    {
        std::vector<std::string> out;
        std::string              cur;
        std::size_t              i = 0;

        for (i = 0; i <= _csv.size(); ++i)
        {
            if (i == _csv.size() || _csv[i] == ',')
            {
                // trim
                std::size_t b = 0;
                std::size_t e = cur.size();

                while (b < e && cur[b] == ' ')     { ++b; }
                while (e > b && cur[e - 1] == ' ') { --e; }

                if (e > b)
                {
                    out.push_back(cur.substr(b, e - b));
                }

                cur.clear();
            }
            else
            {
                cur.push_back(_csv[i]);
            }
        }

        return out;
    }

NS_END  // internal


// =========================================================================
// II.  TEST RUNNER
// =========================================================================

// test_runner
//   class: the top-level run facade.  Owns a test_handler and a
// test_option_set, installs an option-driven reporter on the handler's event
// dispatcher, and runs a tree to a session_result with one call.  Default
// construction yields the framework defaults writing to std::cout; supply a
// test_option_set to
// change behavior.
class test_runner
{
public:
    // test_runner
    //   constructor: framework-default options, console output, reporter
    // installed.
    test_runner()
        : m_handler(),
          m_options(),
          m_console(&std::cout),
          m_file(),
          m_file_open(false),
          m_file_buffer(),
          m_pack_buffering(false),
          m_index(0),
          m_reporter_ids(),
          m_installed(false)
    {
        install_reporters();
    }

    // test_runner
    //   constructor: explicit options.
    explicit test_runner(
        test_option_set _options
    )
        : m_handler(),
          m_options(static_cast<test_option_set&&>(_options)),
          m_console(&std::cout),
          m_file(),
          m_file_open(false),
          m_file_buffer(),
          m_pack_buffering(false),
          m_index(0),
          m_reporter_ids(),
          m_installed(false)
    {
        install_reporters();
    }

    ~test_runner()
    {
        uninstall_reporters();
    }

    // ---- configuration ----

    // options
    //   returns the runtime option set (mutable / const).
    test_option_set&       options()       D_NOEXCEPT { return m_options; }
    const test_option_set& options() const D_NOEXCEPT { return m_options; }

    // set_options
    //   replaces the option set.
    void
    set_options(
        test_option_set _options
    )
    {
        m_options = static_cast<test_option_set&&>(_options);

        return;
    }

    // set_console
    //   redirects console-sink output to _os (default std::cout).
    void
    set_console(
        std::ostream& _os
    ) D_NOEXCEPT
    {
        m_console = &_os;

        return;
    }

    // ---- run ----

    // run
    //   walks _nodes, dispatching lifecycle events (the reporter renders and
    // routes each one), and returns the session_result.  _nodes is any
    // iterable of test_object-protocol elements; the handler brackets the walk
    // with the session events.
    template<typename _Iterable>
    session_result
    run(
        _Iterable& _nodes
    )
    {
        open_file_if_needed();

        m_handler.run(_nodes);

        if (m_console != nullptr)
        {
            m_console->flush();
        }

        // flush the streamed file, or pack the buffered report and write it
        finalize_file();

        return m_handler.result();
    }

    // ---- results ----

    // result
    //   the current session_result snapshot.
    session_result
    result() const D_NOEXCEPT
    {
        return m_handler.result();
    }

    // verdict
    //   the three-way session classification (empty / passed / pending /
    // failed).
    session_verdict
    verdict() const D_NOEXCEPT
    {
        return m_handler.result().verdict();
    }

    // exit_code
    //   a process exit code: 0 only when at least one leaf was observed and
    // every leaf passed; 1 otherwise.  Mirrors session_result::all_passed,
    // which deliberately does NOT treat a pending (half-implemented) suite as
    // green.
    int
    exit_code() const D_NOEXCEPT
    {
        return m_handler.result().all_passed() ? 0 : 1;
    }

    // passed / failed / skipped / errors / pending / total
    //   running session counters.
    std::size_t passed()  const D_NOEXCEPT { return m_handler.passed();  }
    std::size_t failed()  const D_NOEXCEPT { return m_handler.failed();  }
    std::size_t skipped() const D_NOEXCEPT { return m_handler.skipped(); }
    std::size_t errors()  const D_NOEXCEPT { return m_handler.errors();  }
    std::size_t pending() const D_NOEXCEPT { return m_handler.pending(); }
    std::size_t total()   const D_NOEXCEPT { return m_handler.total();   }

    // ---- advanced access ----

    // handler
    //   the underlying test_handler, for binding additional listeners (custom
    // events, status-specific hooks, instrumentation) alongside the reporter.
    test_handler&       handler()       D_NOEXCEPT { return m_handler; }
    const test_handler& handler() const D_NOEXCEPT { return m_handler; }

private:
    // ---- reporter installation ----

    // install_reporters
    //   binds the option-driven reporter bundle on the handler's dispatcher:
    // a session-start reset, a per-module header, a per-leaf line, and a
    // closing summary.  Idempotent.
    void
    install_reporters()
    {
        if (m_installed)
        {
            return;
        }

        // session start: reset the ordinal counter
        m_reporter_ids.push_back(
            m_handler.template on<on_session_start>(
                [this]()
                {
                    this->m_index = 0;
                }));

        // interior node: a header line
        m_reporter_ids.push_back(
            m_handler.template on<on_module_start>(
                [this](const basic_test* _node)
                {
                    this->report_module(_node);
                }));

        // leaf node: one report line (fired once per leaf, after its status
        // event)
        m_reporter_ids.push_back(
            m_handler.template on<on_test_end>(
                [this](const basic_test* _node)
                {
                    this->report_test(_node);
                }));

        // session end: the summary line
        m_reporter_ids.push_back(
            m_handler.template on<on_session_end>(
                [this](std::size_t _passed,
                       std::size_t _failed)
                {
                    this->report_summary(_passed, _failed);
                }));

        m_installed = true;

        return;
    }

    // uninstall_reporters
    //   unbinds the reporter bundle.
    void
    uninstall_reporters() D_NOEXCEPT
    {
        std::size_t i = 0;

        for (i = 0; i < m_reporter_ids.size(); ++i)
        {
            m_handler.off(m_reporter_ids[i]);
        }

        m_reporter_ids.clear();
        m_installed = false;

        return;
    }

    // ---- reporter bodies ----

    // report_module
    //   renders an interior-node header using format_module, routed to the
    // base sink(s).  Suppressed under summary-only / silent.
    void
    report_module(
        const basic_test* _node
    )
    {
        if ( (m_options.show == test_show::summary_only) ||
             (m_options.show == test_show::silent) )
        {
            return;
        }

        std::string line = m_options.format_module;
        internal::runner_subst_helper(line, "{name}",
            _node->metadata().get("name"));

        route(line, m_options.sinks);

        return;
    }

    // report_test
    //   resolves the leaf against the option set and, if the show policy keeps
    // it, renders its line with the resolved format and routes it to the
    // resolved sink(s).
    void
    report_test(
        const basic_test* _node
    )
    {
        match_context ctx = context_of(_node);
        test_resolved r   = resolve(m_options, ctx);

        ++m_index;

        if (!internal::runner_shown_helper(r.show, ctx.status))
        {
            return;
        }

        std::string line = render_test(_node, ctx, r);

        route(line, r.sinks);

        return;
    }

    // report_summary
    //   renders the closing summary using format_summary, routed to the base
    // sink(s).  Suppressed under silent.
    void
    report_summary(
        std::size_t _passed,
        std::size_t _failed
    )
    {
        (void) _passed;
        (void) _failed;

        if (m_options.show == test_show::silent)
        {
            return;
        }

        session_result res = m_handler.result();
        std::string    line = m_options.format_summary;

        internal::runner_subst_helper(line, "{passed}",  to_string(res.passed));
        internal::runner_subst_helper(line, "{failed}",  to_string(res.failed));
        internal::runner_subst_helper(line, "{skipped}", to_string(res.skipped));
        internal::runner_subst_helper(line, "{errors}",  to_string(res.errors));
        internal::runner_subst_helper(line, "{pending}", to_string(res.pending));
        internal::runner_subst_helper(line, "{total}",   to_string(res.total));

        route(line, m_options.sinks);

        return;
    }

    // ---- rendering / routing helpers ----

    // context_of
    //   builds the match_context a route predicate resolves against from a
    // node: name / type id / status, plus tags parsed from a "tags" metadata
    // entry when present.
    match_context
    context_of(
        const basic_test* _node
    ) const
    {
        match_context c;
        c.name    = _node->metadata().get("name");
        c.type_id = _node->type_id();
        c.status  = internal::runner_status_of_helper(_node->status());
        c.tags    = internal::runner_split_tags_helper(
                        _node->metadata().get("tags"));

        return c;
    }

    // render_test
    //   interpolates a leaf's line from the resolved format, then clamps it to
    // the resolved width.  Supported tokens: {index} / {number} (ordinal),
    // {name}, {status}, {message}.  {duration} / {timing} resolve empty in the
    // pure-walk model (no per-node timing is carried).
    std::string
    render_test(
        const basic_test*    _node,
        const match_context& _ctx,
        const test_resolved& _r
    )
    {
        std::string line   = _r.format;
        std::string number = _r.number_tests ? to_string(m_index) : std::string();

        internal::runner_subst_helper(line, "{index}",    to_string(m_index));
        internal::runner_subst_helper(line, "{number}",   number);
        internal::runner_subst_helper(line, "{name}",     _ctx.name);
        internal::runner_subst_helper(line, "{status}",
            internal::runner_status_str_helper(_ctx.status));
        internal::runner_subst_helper(line, "{message}",
            _node->metadata().get("message"));
        internal::runner_subst_helper(line, "{duration}", std::string());
        internal::runner_subst_helper(line, "{timing}",   std::string());

        // bound to the resolved width (production composes the cli_string
        // layout algebra for real alignment / word-wrap)
        if ( (_r.line_width > 0) && (line.size() > _r.line_width) )
        {
            line = line.substr(0, _r.line_width);
        }

        return line;
    }

    // route
    //   writes _line to each destination present in _sinks.
    void
    route(
        const std::string& _line,
        test_sink          _sinks
    )
    {
        if ( test_sink_has(_sinks, test_sink::console) &&
             (m_console != nullptr) )
        {
            (*m_console) << _line << "\n";
        }

        if ( test_sink_has(_sinks, test_sink::file) )
        {
            // packing: accumulate for an end-of-run pack; otherwise stream
            if (m_pack_buffering)
            {
                m_file_buffer += _line;
                m_file_buffer += '\n';
            }
            else if (m_file_open)
            {
                m_file << _line << "\n";
            }
        }

        return;
    }

    // open_file_if_needed
    //   prepares the file sink for this run.  With packing on, the report is
    // collected in memory (m_file_buffer) and packed on finalize; otherwise the
    // file at output_path is opened for streaming.  Either path requires a
    // non-empty output_path.
    void
    open_file_if_needed()
    {
        m_pack_buffering = false;

        // no path: the file sink is inactive either way
        if (m_options.output_path.empty())
        {
            return;
        }

        // packing: collect the file-destined report in memory and pack it on
        // finalize (one codec / archive write), rather than streaming raw
        if (pack_enabled(m_options))
        {
            m_file_buffer.clear();
            m_pack_buffering = true;

            return;
        }

        // not packing: stream straight to the open file (idempotent open)
        if (m_file_open)
        {
            return;
        }

        m_file.open(m_options.output_path.c_str(),
                    std::ios::out | std::ios::trunc);
        m_file_open = m_file.is_open();

        return;
    }

    // finalize_file
    //   ends the run's file output: with packing on, pack the buffered report
    // and write the result to output_path; otherwise flush the streamed file.
    void
    finalize_file()
    {
        if (m_pack_buffering)
        {
            pack_and_write_file();

            return;
        }

        if (m_file_open)
        {
            m_file.flush();
        }

        return;
    }

    // pack_and_write_file
    //   packs m_file_buffer per the option set and writes the result to
    // output_path (binary).  If packing fails (e.g. the codec / format is not
    // built into this binary), the report is written UNPACKED so it is never
    // lost, with a one-line note on the console sink.
    void
    pack_and_write_file()
    {
        byte_blob packed;
        status      s = pack_report(m_options, m_file_buffer, packed);

        std::ofstream out(m_options.output_path.c_str(),
                          std::ios::out | std::ios::binary | std::ios::trunc);

        if (!out.is_open())
        {
            return;  // nowhere to write
        }

        if (s == status_ok)
        {
            out.write(packed.data(),
                      static_cast<std::streamsize>(packed.size()));
        }
        else
        {
            // fall back to the raw report rather than losing it
            out.write(m_file_buffer.data(),
                      static_cast<std::streamsize>(m_file_buffer.size()));

            if (m_console != nullptr)
            {
                (*m_console) << "[dtest] report packing failed ("
                             << status_message(s)
                             << "); wrote the report unpacked.\n";
            }
        }

        out.flush();

        return;
    }

    // to_string
    //   helper: std::size_t -> decimal string (C++11-clean).
    static std::string
    to_string(
        std::size_t _n
    )
    {
        if (_n == 0)
        {
            return std::string("0");
        }

        char        buf[24];
        std::size_t i = sizeof(buf);

        while (_n > 0)
        {
            buf[--i] = static_cast<char>('0' + (_n % 10));
            _n /= 10;
        }

        return std::string(buf + i, sizeof(buf) - i);
    }


    test_handler            m_handler;
    test_option_set         m_options;
    std::ostream*           m_console;
    std::ofstream           m_file;
    bool                    m_file_open;
    std::string             m_file_buffer;     // buffered report (packing)
    bool                    m_pack_buffering;  // packing active this run
    std::size_t             m_index;
    std::vector<handler_id> m_reporter_ids;
    bool                    m_installed;
};


// =========================================================================
// III. FREE-FUNCTION RUNNERS
// =========================================================================

// run_tests
//   function: construct a default runner, run _nodes, return the exit code.
// The one-call entry point.
template<typename _Iterable>
D_NODISCARD int
run_tests(
    _Iterable& _nodes
)
{
    test_runner runner;
    runner.run(_nodes);

    return runner.exit_code();
}

// run_tests
//   function: run _nodes under an explicit option set; return the exit code.
template<typename _Iterable>
D_NODISCARD int
run_tests(
    _Iterable&      _nodes,
    test_option_set _options
)
{
    test_runner runner(static_cast<test_option_set&&>(_options));
    runner.run(_nodes);

    return runner.exit_code();
}


NS_END  // test
NS_END  // djinterp


#endif  // DJINTERP_TEST_RUNNER_
