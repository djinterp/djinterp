/******************************************************************************
* djinterp [test]                                                  test_emit.hpp
*
*   The FP + fluent OUTPUT surface for a finished run - two faces over one
* plan, both lowering into the single bundle/packaging engine (document_bundle
* + output_packaging) that emit_report drives.  Nothing here is a parallel
* config store: the verbs build the SAME output_config + document_bundle the
* procedural emit_report builds, and read the SAME option set through the free
* accessors.  This is the realization of optionator's pattern for emit - a
* run piped into output streams, configured by options - against the runtime
* option_set (the type-level register is the with_option_t option composition).
*
*   THE ALGEBRA (two monoids, no overlap):
*     - targets   as_pdf / as_xml / as_html / as_text  (and to_*_file shorthands)
*                 a render spec; '+' is document_bundle's own concat (emit BOTH).
*     - transforms per_module(t)        one document PER MODULE instead of one.
*                  t.with(tune) / | with(tune)   an OPTION transform, folded
*                  onto the running config: '|'-position = stream scope
*                  (re-environ downstream), .with(...) attached = target scope.
*     - terminals  to_dir / to_file / into_7z / into_zip / compress / to_console
*                  WHERE + HOW the assembled bundle is written.  A target with
*                  no destination resolves to the console.
*
*   WHAT A 'tune' IS:
*   tune:: builders return an option_tune = an option_set endomorphism (it sets
* specific slots and returns the set).  with(tune) composes one onto the
* running config; option_tunes compose by chaining.  LIVE today: packaging
* (tune::compression / tune::archiving feed output_config's compress_opts /
* archive_opts) and structure (per_module) and PDF style (.style(src)).  Layout
* tunes (tune::width / tune::color) are honored by the console reporter and
* become live for documents when the format skeletons consult the option set;
* the surface is already shaped for that.
*
*   BOTH FACES, ONE PLAN:
*       run.emit() | per_module(as_pdf) | into_7z("dtest");          // pipe
*       run.emit().to_pdf_file("r.pdf").to_html_file("r.html").into_7z("rep"); // fluent
*       run.emit().as_html();                                         // -> console
*       emit(report, opts) | as_pdf + as_xml | to_dir("out/");        // free origin
*
*   PORTABILITY:  C++17 (it composes test_output / document_bundle).
*
*
* path:      /inc/djinterp/test/output/test_emit.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_TEST_TEST_EMIT_
#define DJINTERP_TEST_TEST_EMIT_ 1

// std
#include <cstddef>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
// djinterp
#include "../core/djinterp.hpp"            // NS_*, D_NODISCARD, gates
#include "./test_output_config.hpp"        // to_output_config, doc_format, render_*_bytes,
                                           //   build_*, at_module,
                                           //   document_bundle, output_config, pack_mode,
                                           //   format_id_*, codec_id, write_to_disk,
                                           //   write_to_buffer, byte_blob, pdf_template_source


#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   OPTION TRANSFORMS  (mid-stream 'tune's)              ///
///////////////////////////////////////////////////////////////////////////////

// option_tune
//   type: an option_set endomorphism - the runtime "optionator".  with(tune)
// folds one onto the running config; they compose by chaining.
using option_tune = std::function<test_option_set(test_option_set)>;


namespace tune
{
    // width / color
    //   layout tunes (honored by the console reporter today; document-live once
    // the format skeletons consult the option set).
    D_NODISCARD inline option_tune
    width(std::size_t _n)
    {
        return [_n](test_option_set _o) -> test_option_set
        {
        #if D_ENV_LANG_IS_CPP20_OR_HIGHER
            _o.set<test_option::line_width>(_n);
        #else
            _o.line_width = _n;
        #endif
            return _o;
        };
    }

    D_NODISCARD inline option_tune
    color(bool _on)
    {
        return [_on](test_option_set _o) -> test_option_set
        {
        #if D_ENV_LANG_IS_CPP20_OR_HIGHER
            _o.set<test_option::color>(_on);
        #else
            _o.color = _on;
        #endif
            return _o;
        };
    }

    // compression / archiving
    //   packaging tunes - LIVE: they ride to_output_config into output_config's
    // compress_opts / archive_opts and are dispatched by the write engine.
    D_NODISCARD inline option_tune
    compression(compress_options _co)
    {
        return [_co](test_option_set _o) -> test_option_set
        {
        #if D_ENV_LANG_IS_CPP20_OR_HIGHER
            _o.set<test_option::compress_opts>(_co);
        #else
            _o.compress_opts = _co;
        #endif
            return _o;
        };
    }

    D_NODISCARD inline option_tune
    archiving(archive_options _ao)
    {
        return [_ao](test_option_set _o) -> test_option_set
        {
        #if D_ENV_LANG_IS_CPP20_OR_HIGHER
            _o.set<test_option::archive_opts>(_ao);
        #else
            _o.archive_opts = _ao;
        #endif
            return _o;
        };
    }

    // compose
    //   chain two tunes (left then right) - the monoid op behind successive
    // with(...)s.  Any caller-built option_tune (a lambda over the option set)
    // slots in here too, so exotic knobs need no new builder.
    D_NODISCARD inline option_tune
    compose(option_tune _a, option_tune _b)
    {
        return [_a, _b](test_option_set _o) -> test_option_set
        {
            test_option_set _x = _a ? _a(static_cast<test_option_set&&>(_o))
                                    : static_cast<test_option_set&&>(_o);
            return _b ? _b(static_cast<test_option_set&&>(_x))
                      : static_cast<test_option_set&&>(_x);
        };
    }
}  // namespace tune


///////////////////////////////////////////////////////////////////////////////
///                II.  TARGETS  (as_* / to_*_file / + / per_module)         ///
///////////////////////////////////////////////////////////////////////////////

// emit_target
//   struct: one render spec - a format, an optional destination filename
// (empty == inherit the terminal / console), and a target-local option_tune.
struct emit_target
{
    doc_format  fmt  = doc_format::text;
    std::string dest;          // "" -> terminal-provided / console
    option_tune local;         // target-scoped tune (folded for this render)

    // to
    //   pin this target's destination filename (the to_file half of to_*_file).
    D_NODISCARD emit_target
    to(std::string _name) const
    {
        emit_target _t = *this;
        _t.dest = static_cast<std::string&&>(_name);
        return _t;
    }

    // with
    //   fold a target-scoped option transform onto this target only.
    D_NODISCARD emit_target
    with(option_tune _t) const
    {
        emit_target _r = *this;
        _r.local = tune::compose(_r.local, static_cast<option_tune&&>(_t));
        return _r;
    }
};

// the bare format targets (no destination -> console unless a terminal places them)
inline const emit_target as_text     { doc_format::text,     std::string(), option_tune() };
inline const emit_target as_markdown { doc_format::markdown, std::string(), option_tune() };
inline const emit_target as_xml      { doc_format::xml,      std::string(), option_tune() };
inline const emit_target as_html     { doc_format::html,     std::string(), option_tune() };
inline const emit_target as_pdf      { doc_format::pdf,      std::string(), option_tune() };

// to_*_file  ==  as_<fmt>.to(name)  - the destination+format shorthand.
D_NODISCARD inline emit_target to_text_file(std::string _n) { return as_text.to(static_cast<std::string&&>(_n)); }
D_NODISCARD inline emit_target to_md_file  (std::string _n) { return as_markdown.to(static_cast<std::string&&>(_n)); }
D_NODISCARD inline emit_target to_xml_file (std::string _n) { return as_xml.to(static_cast<std::string&&>(_n)); }
D_NODISCARD inline emit_target to_html_file(std::string _n) { return as_html.to(static_cast<std::string&&>(_n)); }
D_NODISCARD inline emit_target to_pdf_file (std::string _n) { return as_pdf.to(static_cast<std::string&&>(_n)); }


// target_group
//   struct: an ordered set of targets plus the per-module flag.  '+' fans out
// (this IS document_bundle's concat lifted to specs); per_module flips one bit.
struct target_group
{
    std::vector<emit_target> targets;
    bool                     per_module = false;
};

// + : emit_target/target_group fan-out
D_NODISCARD inline target_group operator+(emit_target _a, emit_target _b) { target_group _g; _g.targets.push_back(_a); _g.targets.push_back(_b); return _g; }
D_NODISCARD inline target_group operator+(target_group _g, emit_target _b) { _g.targets.push_back(_b); return _g; }
D_NODISCARD inline target_group operator+(emit_target _a, target_group _g) { _g.targets.insert(_g.targets.begin(), _a); return _g; }

// per_module
//   transform: emit one document per module rather than one for the run.
D_NODISCARD inline target_group per_module(target_group _g) { _g.per_module = true; return _g; }
D_NODISCARD inline target_group per_module(emit_target _t)  { target_group _g; _g.targets.push_back(_t); _g.per_module = true; return _g; }


///////////////////////////////////////////////////////////////////////////////
///                III. TERMINALS  (where + how the bundle is written)       ///
///////////////////////////////////////////////////////////////////////////////

// emit_sink
//   struct: a terminal - the packaging mode + its name/dir, or console.  Built
// by the free factories below and consumed by the builder's pipe / methods.
struct emit_sink
{
    enum kind { loose, archive, deflate_each, console } mode = loose;

    format_id   fmt  = format_id_zip;   // archive container
    codec_id    cod  = codec_id_gzip;   // per-document codec
    std::string name;                   // archive base name / directory prefix
};

D_NODISCARD inline emit_sink into_7z (std::string _name) { emit_sink _s; _s.mode = emit_sink::archive; _s.fmt = format_id_sevenzip; _s.name = static_cast<std::string&&>(_name); return _s; }
D_NODISCARD inline emit_sink into_zip(std::string _name) { emit_sink _s; _s.mode = emit_sink::archive; _s.fmt = format_id_zip;      _s.name = static_cast<std::string&&>(_name); return _s; }
D_NODISCARD inline emit_sink compress(codec_id _c)       { emit_sink _s; _s.mode = emit_sink::deflate_each; _s.cod = _c; return _s; }
D_NODISCARD inline emit_sink to_dir  (std::string _dir)  { emit_sink _s; _s.mode = emit_sink::loose;   _s.name = static_cast<std::string&&>(_dir); return _s; }
D_NODISCARD inline emit_sink to_console()                { emit_sink _s; _s.mode = emit_sink::console; return _s; }


///////////////////////////////////////////////////////////////////////////////
///                IV.  THE BUILDER  (fluent + pipe + RAII default)          ///
///////////////////////////////////////////////////////////////////////////////

// emit_builder
//   class: the one plan both faces build.  Holds the report, the running option
// set (base + stream tunes), the target group, and a PDF-style source.  Fluent
// methods accumulate and return *this; terminals lower the plan into the
// bundle/packaging engine and return success.  If no terminal is reached, the
// destructor writes the accumulated targets (dest-ful -> loose files, otherwise
// the console) - so `run.emit().as_html();` lands on the console with no
// ceremony.  Move-only; a moved-from builder is inert.
class emit_builder
{
public:
    emit_builder(
        const test_report*  _report,
        test_option_set     _opts
    )
        : m_report(_report),
          m_opts(static_cast<test_option_set&&>(_opts)),
          m_group(),
          m_pdf(),
          m_done(false)
    {}

    emit_builder(const emit_builder&)            = delete;
    emit_builder& operator=(const emit_builder&) = delete;

    emit_builder(emit_builder&& _o) D_NOEXCEPT
        : m_report(_o.m_report),
          m_opts(static_cast<test_option_set&&>(_o.m_opts)),
          m_group(static_cast<target_group&&>(_o.m_group)),
          m_pdf(static_cast<pdf_template_source&&>(_o.m_pdf)),
          m_done(_o.m_done)
    {
        _o.m_done = true;   // the moved-from builder must not also write
    }

    ~emit_builder()
    {
        if (!m_done)
        {
            (void) flush_default();
        }
    }

    // ---- fluent accumulators (return *this) --------------------------------

    emit_builder& add(emit_target _t)            { m_group.targets.push_back(static_cast<emit_target&&>(_t)); return *this; }
    emit_builder& as(doc_format _f)              { return add(emit_target{ _f, std::string(), option_tune() }); }

    emit_builder& as_text_()                     { return as(doc_format::text); }
    emit_builder& as_markdown_()                 { return as(doc_format::markdown); }
    emit_builder& as_xml_()                      { return as(doc_format::xml); }
    emit_builder& as_html_()                     { return as(doc_format::html); }
    emit_builder& as_pdf_()                      { return as(doc_format::pdf); }

    emit_builder& to_text_file(std::string _n)   { return add(::djinterp::test::to_text_file(_n)); }
    emit_builder& to_md_file  (std::string _n)   { return add(::djinterp::test::to_md_file(_n)); }
    emit_builder& to_xml_file (std::string _n)   { return add(::djinterp::test::to_xml_file(_n)); }
    emit_builder& to_html_file(std::string _n)   { return add(::djinterp::test::to_html_file(_n)); }
    emit_builder& to_pdf_file (std::string _n)   { return add(::djinterp::test::to_pdf_file(_n)); }

    emit_builder& per_module()                   { m_group.per_module = true; return *this; }
    emit_builder& with(option_tune _t)           { m_opts = _t ? _t(static_cast<test_option_set&&>(m_opts)) : static_cast<test_option_set&&>(m_opts); return *this; }
    emit_builder& style(pdf_template_source _s)  { m_pdf = static_cast<pdf_template_source&&>(_s); return *this; }

    // ---- fluent terminals (return write success; discardable) --------------

    bool into_7z (std::string _name) { return run_sink(::djinterp::test::into_7z(_name)); }
    bool into_zip(std::string _name) { return run_sink(::djinterp::test::into_zip(_name)); }
    bool compress(codec_id _c)       { return run_sink(::djinterp::test::compress(_c)); }
    bool to_dir  (std::string _dir)  { return run_sink(::djinterp::test::to_dir(_dir)); }
    bool to_console()                { return run_sink(::djinterp::test::to_console()); }

    // ---- pipe hooks (used by operator| below) ------------------------------

    emit_builder& pipe_group(target_group _g)
    {
        for (std::size_t _i = 0; _i < _g.targets.size(); ++_i) { m_group.targets.push_back(_g.targets[_i]); }
        if (_g.per_module) { m_group.per_module = true; }
        return *this;
    }

    bool run_sink(emit_sink _s)
    {
        m_done = true;

        ::djinterp::document_bundle _bundle = build();     // producers borrow m_report by pointer

        ::djinterp::output_config _cfg = ::djinterp::test::to_output_config(m_opts);  // compress/archive opts
        _cfg.naming = [](const std::string& _n, const std::string& _e,
                         std::size_t, std::size_t) { return _n + _e; };               // names are already unique

        switch (_s.mode)
        {
            case emit_sink::archive:
            {
                _cfg.pack         = ::djinterp::pack_mode::archive;
                _cfg.format       = _s.fmt;
                _cfg.archive_name = _s.name;
                return ::djinterp::write_to_disk(_bundle, _cfg,
                    [](const std::string& _f) { return _f; });
            }
            case emit_sink::deflate_each:
            {
                _cfg.pack  = ::djinterp::pack_mode::compress;
                _cfg.codec = _s.cod;
                return ::djinterp::write_to_disk(_bundle, _cfg,
                    [](const std::string& _f) { return _f; });
            }
            case emit_sink::console:
            {
                _cfg.pack = ::djinterp::pack_mode::none;
                byte_blob _buf;
                const bool _ok = ::djinterp::write_to_buffer(_bundle, _cfg, _buf, std::string("\n"));
                std::cout.write(_buf.data(), static_cast<std::streamsize>(_buf.size()));
                std::cout.flush();
                return _ok;
            }
            case emit_sink::loose:
            default:
            {
                _cfg.pack = ::djinterp::pack_mode::none;
                const std::string _dir = _s.name;      // directory prefix (may be empty)
                return ::djinterp::write_to_disk(_bundle, _cfg,
                    [_dir](const std::string& _f) { return _dir + _f; });
            }
        }
    }

private:
    // build
    //   render every target into a deferred bundle item.  Per-module fans each
    // target across the run's modules (named by module); otherwise one item per
    // target (named by its pinned destination stem, else "report").  The run must
    // outlive write() - the producers borrow it by pointer.
    ::djinterp::document_bundle
    build()
    {
        ::djinterp::document_bundle  _bundle;
        const test_report*           _rp = m_report;
        const pdf_template_source    _src = m_pdf;

        std::size_t _t = 0;

        for (_t = 0; _t < m_group.targets.size(); ++_t)
        {
            const doc_format _fmt = m_group.targets[_t].fmt;

            if (m_group.per_module && (_rp != nullptr))
            {
                std::size_t _m = 0;
                for (_m = 0; _rp != nullptr && _m < _rp->modules.size(); ++_m)
                {
                    const report_module* _mod = &_rp->modules[_m];
                    const std::size_t    _idx = _m;

                    _bundle.add(
                        std::string(_mod->name),
                        std::string(format_extension(_fmt)),
                        [_rp, _mod, _idx, _fmt, _src]() -> byte_blob
                        {
                            return render_module_bytes(
                                at_module(_rp, _mod, _idx + 1), _fmt, _src);
                        });
                }
            }
            else
            {
                const std::string& _d   = m_group.targets[_t].dest;
                std::string        _name = _d.empty() ? std::string("report")
                                                      : stem_of(_d);

                _bundle.add(
                    static_cast<std::string&&>(_name),
                    std::string(format_extension(_fmt)),
                    [_rp, _fmt, _src]() -> byte_blob
                    {
                        return _rp ? render_report_bytes(*_rp, _fmt, _src)
                                   : byte_blob();
                    });
            }
        }

        return _bundle;
    }

    // flush_default
    //   the no-terminal fallback: if every target is destination-less, dump the
    // run to the console; otherwise write loose files to the pinned names.
    bool
    flush_default()
    {
        bool _any_dest = false;
        std::size_t _i = 0;
        for (_i = 0; _i < m_group.targets.size(); ++_i)
        {
            if (!m_group.targets[_i].dest.empty()) { _any_dest = true; break; }
        }

        return _any_dest ? run_sink(::djinterp::test::to_dir(std::string()))
                         : run_sink(::djinterp::test::to_console());
    }

    static std::string
    stem_of(const std::string& _path)
    {
        const std::string::size_type _slash = _path.find_last_of("/\\");
        const std::string _file = (_slash == std::string::npos) ? _path
                                                                 : _path.substr(_slash + 1);
        const std::string::size_type _dot = _file.find_last_of('.');
        const std::string _dir = (_slash == std::string::npos) ? std::string()
                                                               : _path.substr(0, _slash + 1);
        return _dir + ((_dot == std::string::npos) ? _file : _file.substr(0, _dot));
    }

    const test_report*  m_report;
    test_option_set     m_opts;
    target_group        m_group;
    pdf_template_source m_pdf;
    bool                m_done;
};


// ---- pipe wiring -----------------------------------------------------------

D_NODISCARD inline emit_builder operator|(emit_builder _b, emit_target  _t) { _b.add(static_cast<emit_target&&>(_t));  return _b; }
D_NODISCARD inline emit_builder operator|(emit_builder _b, target_group _g) { _b.pipe_group(static_cast<target_group&&>(_g)); return _b; }
D_NODISCARD inline emit_builder operator|(emit_builder _b, option_tune  _t) { _b.with(static_cast<option_tune&&>(_t)); return _b; }
inline bool                     operator|(emit_builder _b, emit_sink    _s) { return _b.run_sink(static_cast<emit_sink&&>(_s)); }


///////////////////////////////////////////////////////////////////////////////
///                V.   ORIGINS                                              ///
///////////////////////////////////////////////////////////////////////////////

// with
//   wrap an option transform for the stream (re-environs everything downstream).
// `run.emit() | with(tune::width(120)) | as_pdf + as_html | to_dir("out/")`.
D_NODISCARD inline option_tune with(option_tune _t) { return _t; }

// emit
//   the free origin: a plan over _report configured by _opts (default the
// framework defaults).  `emit(report) | per_module(as_pdf) | into_7z("dtest");`
D_NODISCARD inline emit_builder
emit(
    const test_report&  _report,
    test_option_set     _opts = default_test_options()
)
{
    return emit_builder(&_report, static_cast<test_option_set&&>(_opts));
}

//   The controller origin lives on the runner: report_builder (or test_runner)
// gains  emit_builder emit()  returning  ::djinterp::test::emit(this->report(),
// this->options());  so `run.emit() | ...` and `run.emit().to_pdf_file(...)`
// read off the same report + option set the run accumulated.  (Added there,
// not here, to keep this header free of the runner include.)


NS_END  // test
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_TEST_TEST_EMIT_
