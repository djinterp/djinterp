/******************************************************************************
* djinterp [test]                                                test_render.hpp
*
*   The test-side rendering module: the BINDING and the WALK in one place -
* the entire "how DTest state becomes a document" surface.  Everything here
* leans on the agnostic engines (interpolate, markup escape policies), so the
* per-format, per-level render family that used to live in test_document
* (render_run_flow / render_run_tree / render_module_* / render_unit_* /
* render_check_*) collapses to: a table of projections, a flat layout per
* format, and one traversal.
*
*   THE BINDING (sections I-II).  Every {token} a layout can name is a
* PROJECTION over the current focus - a (token, projection) row.  test_resolver
* is an interpolate resolver: (key) -> resolution<value>; it finds the key's
* projection, applies it to the bound focus, and yields an OWNING string (so it
* sits with lookup / recursive among the owning-value frames); an unknown key,
* or a token out of focus, is a MISS - the placeholder is left intact, which is
* what makes partial rendering and frame-chaining work.  Adding a token is
* adding a row; that is the whole cost of binding new state.
*     VALUE-LEVEL FALLBACK (`metadata.get(name) | or_else(test_number)`): each
*   projection returns std::optional<std::string> and does its own fallback -
*   curr_test_kind is the worked example (the test's name, else "test #N").
*   That or_else is at the VALUE level; interpolate's is at the RESOLUTION level
*   (chained frames).  Both, at the layer each belongs to.
*     COMPOSING MID-FLOW: chain(make_test_resolver(f), make_test_resolver(f,
*   const_table)) overlays constants (every frame in a chain shares the value
*   type, so the overlay is string-valued too - NOT the view-valued bindings);
*   when(pred, r) scopes a frame to a key family; recursive(make_test_resolver
*   (f)) lets a hit that is itself a template expand.  Compound STATE conditions
*   (the "3rd failure whose name has a 'q' on a gibbous-moon odd month") compose
*   with predicate.hpp inside a projection, or later in template selection.
*
*   THE WALK (sections III-VI).  render_report refocuses the binding per node
* and renders each level's flat LAYOUT through interpolate.  A "format" is no
* longer code - it is a report_layout (one literal per level) plus a SINK:
*       text / markdown : the literals + interpolate's plain interp_string_sink
*       xml  / html     : markup tags in the literals + escaping_sink<Policy>,
*                         which escapes resolved VALUES (not the tags), reusing
*                         markup_string_template's escape policies - the one
*                         reusable piece of that engine.  Its context/section
*                         model is NOT used: in-layout sections would force a
*                         parallel materialized context tree; per-level layouts
*                         over the resolver give the identical result with no new
*                         parser and no second data model.
*       pdf             : NOT here - pdf is a flow of ops, not a string; see
*                         test_render_pdf.hpp, kept separate for its pdf.hpp
*                         dependency weight.
*
*   FOCUS CONTRACT (reconcile with test_context.hpp): run / module_ / unit /
* check + 1-based *_index.  A unit's verdict word comes from report_verdict_word
* (test_report.hpp); a check's test_status has no library word, so the binding
* supplies check_status_word; and report_unit has no passed/failed count, so the
* binding rolls the unit's checks up with count_status.
*
*   PORTABILITY:  C++17 (interpolate.hpp, markup escape policies, std::optional).
*
*
* path:      /inc/djinterp/test/output/test_render.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                       created: 2026.06.29
******************************************************************************/

#ifndef DJINTERP_TEST_TEST_RENDER_
#define DJINTERP_TEST_TEST_RENDER_ 1

// std
#include <cstddef>
#include <functional>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
// djinterp
#include "../../core/djinterp.hpp"                      // NS_*, D_NODISCARD
#include "../../core/functional/interpolate.hpp"        // resolution, resolved/unresolved, interp_string_sink,
                                                        //   brace_scanner, interpolate_into (path: match your tree)
#include "../../core/text/markup_string_template.hpp"   // xml_escape_policy / html_escape_policy (path: match your tree)
#include "../test_common.hpp"                           // test_status (passed/failed/skipped/pending/error)
#include "../test_context.hpp"                          // test_context focus + at_* builders
#include "./test_report.hpp"                            // test_report, report_module/unit/check, report_verdict, report_verdict_word


#if D_ENV_LANG_IS_CPP17_OR_HIGHER


NS_DJINTERP
NS_TEST


///////////////////////////////////////////////////////////////////////////////
///                I.   PROJECTIONS  (the binding: focus -> fragment)        ///
///////////////////////////////////////////////////////////////////////////////

// test_projection
//   type: a binding's right-hand side - the current focus to a fragment, or
// nullopt to MISS (out of focus / not applicable).  This is the FP projection a
// {token} names; it closes over nothing but the focus it is handed.
using test_projection = std::function<std::optional<std::string>(const test_context&)>;

// projection_entry / projection_table
//   one (token, projection) row, and the ordered table of them.  Adding a token
// is adding a row; that is the entire cost of binding new state.
using projection_entry = std::pair<std::string_view, test_projection>;
using projection_table = std::vector<projection_entry>;


namespace internal
{
    // name_or_index
    //   the value-level fallback: the identifier if present, else "<noun> #N".
    // This is `metadata.get(name) | or_else(number)` spelled at the value level.
    inline std::string
    name_or_index(const std::string& _name, const char* _noun, std::size_t _index)
    {
        return _name.empty()
             ? (std::string(_noun) + " #" + std::to_string(_index))
             : _name;
    }

    // some / none
    //   tiny adapters so a projection body reads as a one-liner.
    inline std::optional<std::string> some(std::string _s) { return std::optional<std::string>(static_cast<std::string&&>(_s)); }
    inline std::optional<std::string> none()               { return std::optional<std::string>(); }

    // check_status_word
    //   a check's test_status as an uppercase word.  report_verdict_word covers
    // a unit's verdict, but there is no library word for a check's test_status,
    // so the binding supplies one - it is the layer whose job is state -> text.
    inline const char*
    check_status_word(test_status _s)
    {
        switch (_s)
        {
            case test_status::passed:  return "PASS";
            case test_status::failed:  return "FAIL";
            case test_status::skipped: return "SKIP";
            case test_status::pending: return "PEND";
            default:                   return "ERR";
        }
    }

    // count_status
    //   report_unit exposes no passed/failed COUNT accessor (only a derived
    // verdict), so unit_passed / unit_failed roll the unit's checks up here.
    inline std::size_t
    count_status(const report_unit& _u, test_status _s)
    {
        std::size_t _n = 0;
        std::size_t _i = 0;
        for (_i = 0; _i < _u.checks.size(); ++_i)
        {
            if (_u.checks[_i].status == _s) { ++_n; }
        }
        return _n;
    }
}  // namespace internal


// report_projections
//   the standard token table over the report model.  Each row is one {token}
// receiver; a token whose node is not in the current focus returns none() (a
// MISS), so the same table serves a whole-report layout and a single-check
// fragment without change.  THIS table is the test-specific binding in full.
D_NODISCARD inline const projection_table&
report_projections()
{
    using namespace internal;

    static const projection_table _table =
    {
        // ---- run (the whole report) ---------------------------------------
        { "report_title",    [](const test_context& _c){ return _c.run ? some(_c.run->title)    : none(); } },
        { "report_subtitle", [](const test_context& _c){ return _c.run ? some(_c.run->subtitle) : none(); } },
        { "report_author",   [](const test_context& _c){ return _c.run ? some(_c.run->author)   : none(); } },

        // ---- module -------------------------------------------------------
        { "module_name",     [](const test_context& _c){ return _c.module_ ? some(name_or_index(_c.module_->name, "module", _c.module_index)) : none(); } },
        { "module_desc",     [](const test_context& _c){ return _c.module_ ? some(_c.module_->description) : none(); } },
        { "module_index",    [](const test_context& _c){ return _c.module_ ? some(std::to_string(_c.module_index)) : none(); } },

        // ---- unit  ==  "the current test" ---------------------------------
        //   curr_test_kind is the worked example: the test's identifier, or its
        // number when unnamed.  Swap the body to read a test_kind / object
        // metadata field (via metadata_traits over the live node) and the rest
        // of the pipeline does not notice - it is still one row, one {token}.
        { "curr_test_kind",  [](const test_context& _c){ return _c.unit ? some(name_or_index(_c.unit->name, "test", _c.unit_index)) : none(); } },
        { "unit_name",       [](const test_context& _c){ return _c.unit ? some(name_or_index(_c.unit->name, "test", _c.unit_index)) : none(); } },
        { "unit_index",      [](const test_context& _c){ return _c.unit ? some(std::to_string(_c.unit_index)) : none(); } },
        { "unit_verdict",    [](const test_context& _c){ return _c.unit ? some(report_verdict_word(_c.unit->verdict)) : none(); } },
        { "unit_passed",     [](const test_context& _c){ return _c.unit ? some(std::to_string(count_status(*_c.unit, test_status::passed))) : none(); } },
        { "unit_failed",     [](const test_context& _c){ return _c.unit ? some(std::to_string(count_status(*_c.unit, test_status::failed))) : none(); } },

        // ---- check --------------------------------------------------------
        { "check_index",     [](const test_context& _c){ return _c.check ? some(std::to_string(_c.check_index)) : none(); } },
        { "check_desc",      [](const test_context& _c){ return _c.check ? some(_c.check->description) : none(); } },
        { "check_status",    [](const test_context& _c){ return _c.check ? some(check_status_word(_c.check->status)) : none(); } },
    };

    return _table;
}


///////////////////////////////////////////////////////////////////////////////
///                II.  THE RESOLVER  (an interpolate frame)                 ///
///////////////////////////////////////////////////////////////////////////////

// test_resolver
//   an interpolate resolver over a focus + a projection table.  operator()(key)
// finds the key's projection and applies it to the focus, yielding an owning
// string; an unknown key, or a projection that returns none(), is a MISS.  It is
// trivially copyable (two pointers) and so drops into chain / when / recursive
// and interpolate_into exactly like the built-in frames.
class test_resolver
{
public:
    using char_type  = char;
    using view_type  = std::string_view;
    using value_type = std::string;

    explicit test_resolver(
        const test_context&     _focus,
        const projection_table& _table = report_projections()
    )
        : m_focus(&_focus),
          m_table(&_table)
    {}

    D_NODISCARD ::djinterp::resolution<std::string>
    operator()(
        view_type _key
    ) const
    {
        std::size_t _i = 0;
        for (_i = 0; _i < m_table->size(); ++_i)
        {
            if ((*m_table)[_i].first == _key)
            {
                std::optional<std::string> _v = (*m_table)[_i].second(*m_focus);
                return _v ? ::djinterp::resolved(static_cast<std::string&&>(*_v))
                          : ::djinterp::unresolved<std::string>();
            }
        }

        return ::djinterp::unresolved<std::string>();
    }

private:
    const test_context*     m_focus;
    const projection_table* m_table;
};


// make_test_resolver
//   the factory: a resolver over the standard table, or a caller-supplied one.
//   Compose mid-flow: chain(make_test_resolver(f), make_test_resolver(f, t2))
// overlays a second (string-valued) table; when(pred, r) scopes by key;
// recursive(make_test_resolver(f)) expands a hit that is itself a template.
D_NODISCARD inline test_resolver
make_test_resolver(const test_context& _focus)
{
    return test_resolver(_focus);
}

D_NODISCARD inline test_resolver
make_test_resolver(const test_context& _focus, const projection_table& _table)
{
    return test_resolver(_focus, _table);
}


///////////////////////////////////////////////////////////////////////////////
///                III. THE LAYOUT  (data, one set per string format)        ///
///////////////////////////////////////////////////////////////////////////////

// report_layout
//   one flat {token} literal per tree level.  *_open render at that level's
// focus before descending; *_close after.  An empty member emits nothing, so a
// flow format leaves the *_close fields empty and a tree format puts the closing
// tags there.  THIS is a format - text/markdown/xml/html differ only here (plus
// the sink); none of it is code.
struct report_layout
{
    std::string run_open;
    std::string module_open;
    std::string unit_open;
    std::string check_line;     // the leaf; rendered once per check at check focus
    std::string unit_close;
    std::string module_close;
    std::string run_close;
};


///////////////////////////////////////////////////////////////////////////////
///                IV.  SINKS  (plain, and value-escaping)                   ///
///////////////////////////////////////////////////////////////////////////////
//   interpolate's own interp_string_sink is the plain (text / markdown) sink.  For
// markup formats the layout's literals are tags we must NOT escape, while the
// resolved values ARE escaped - so a sink that escapes only value() and passes
// literal() through, reusing markup_string_template's policy.

// escaping_sink
//   class: appends to a caller string; literal runs (the layout's tags) go in
// raw, resolved values are escaped through _Policy (markup's xml/html policy).
// Use as  escaping_sink<xml_escape_policy>  /  escaping_sink<html_escape_policy>.
template<typename _Policy>
class escaping_sink
{
public:
    using view_type   = std::string_view;
    using string_type = std::string;

    explicit escaping_sink(string_type& _out)
        : m_out(_out)
    {}

    void
    literal(view_type _run)
    {
        m_out.append(_run.data(), _run.size());
    }

    template<typename _Value>
    void
    value(const _Value& _value)
    {
        std::ostringstream _oss;                       // once-per-value; render path, off any hot path
        _Policy::escape(_oss, std::string(_value));
        const std::string _escaped = _oss.str();
        m_out.append(_escaped.data(), _escaped.size());
    }

private:
    string_type& m_out;
};


///////////////////////////////////////////////////////////////////////////////
///                V.   THE WALK  (one traversal, all string formats)        ///
///////////////////////////////////////////////////////////////////////////////

// emit
//   render one layout fragment against one focus into the sink: the flat
// interpolate fold, the test_resolver bound to this focus.  Scanner defaults to
// brace_scanner ({token}); swap it (e.g. a paired-% marker scanner) to change
// the layout dialect - the walk is scanner-agnostic.
template<typename _Sink,
         typename _Scanner = ::djinterp::brace_scanner<char>>
inline void
emit(
    const std::string&  _layout,
    const test_context& _focus,
    _Sink&              _sink
)
{
    if (_layout.empty())
    {
        return;
    }

    ::djinterp::interpolate_into(_sink, _Scanner(_layout), make_test_resolver(_focus));

    return;
}


// render_module
//   render ONE module subtree - its open, its units and their checks, its close -
// at a focus that already names the module.  render_report calls this per
// module; test_output's per-module-document path (build_per_module_bundle)
// calls it directly on an at_module focus, so a per-module bundle and the
// whole-run render share exactly one body.
template<typename _Sink>
inline void
render_module(
    const report_layout&    _layout,
    const test_context&     _fm,        // focus with run + module_ + module_index set
    _Sink&                  _sink
)
{
    emit(_layout.module_open, _fm, _sink);

    std::size_t _ui = 0;
    for (_ui = 0; _ui < _fm.module_->units.size(); ++_ui)
    {
        test_context _fu = _fm;
        _fu.unit       = &_fm.module_->units[_ui];
        _fu.unit_index = _ui + 1;

        emit(_layout.unit_open, _fu, _sink);

        std::size_t _ci = 0;
        for (_ci = 0; _ci < _fu.unit->checks.size(); ++_ci)
        {
            test_context _fc = _fu;
            _fc.check       = &_fu.unit->checks[_ci];
            _fc.check_index = _ci + 1;

            emit(_layout.check_line, _fc, _sink);
        }

        emit(_layout.unit_close, _fu, _sink);
    }

    emit(_layout.module_close, _fm, _sink);

    return;
}


// render_report
//   THE whole-run render: the run wrapper, then render_module per module.
// Generic over the sink, so a format is entirely (this layout) x (that sink).
// Borrows _run; nothing is materialized.
template<typename _Sink>
inline void
render_report(
    const report_layout&    _layout,
    const test_report&      _run,
    _Sink&                  _sink
)
{
    test_context _f;
    _f.run = &_run;

    emit(_layout.run_open, _f, _sink);

    std::size_t _mi = 0;
    for (_mi = 0; _mi < _run.modules.size(); ++_mi)
    {
        test_context _fm = _f;
        _fm.module_      = &_run.modules[_mi];
        _fm.module_index = _mi + 1;

        render_module(_layout, _fm, _sink);
    }

    emit(_layout.run_close, _f, _sink);

    return;
}


// render_report_string
//   convenience: render to a fresh string with the plain (text/markdown) sink.
// For xml/html, build an escaping_sink<Policy> over your string and call
// render_report directly.
D_NODISCARD inline std::string
render_report_string(
    const report_layout&    _layout,
    const test_report&      _run
)
{
    std::string                 _out;
    ::djinterp::interp_string_sink<char> _sink(_out);
    render_report(_layout, _run, _sink);
    return _out;
}


///////////////////////////////////////////////////////////////////////////////
///                VI.  DEFAULT LAYOUTS  (data; one per string format)       ///
///////////////////////////////////////////////////////////////////////////////
//   The house defaults - pure data.  Edit a literal to restyle; the walk and
// the binding never change.  text / markdown render through the plain
// interp_string_sink; xml / html through escaping_sink<...> (their literals are tags,
// their resolved {token} values are escaped).  Every {token} here is a row in
// report_projections(); an unbound token would simply pass through verbatim.
// PDF is deliberately absent - it is a flow of ops, not a string (test_render_pdf).

D_NODISCARD inline report_layout
layout_text()
{
    report_layout _l;
    _l.run_open    = "{report_title}\n========================================\n";
    _l.module_open = "\n[module {module_index}]  {module_name}\n";
    _l.unit_open   = "  {curr_test_kind}   ({unit_passed} passed, {unit_failed} failed)\n";
    _l.check_line  = "      {check_index}. {check_desc}   [{check_status}]\n";
    _l.run_close   = "\n";
    return _l;
}

D_NODISCARD inline report_layout
layout_markdown()
{
    report_layout _l;
    _l.run_open    = "# {report_title}\n\n";
    _l.module_open = "## {module_name}\n\n";
    _l.unit_open   = "### {curr_test_kind}\n\n";
    _l.check_line  = "- {check_index}. {check_desc} \xE2\x80\x94 **{check_status}**\n";
    _l.unit_close  = "\n";
    return _l;
}

D_NODISCARD inline report_layout
layout_xml()
{
    report_layout _l;
    _l.run_open     = "<report title=\"{report_title}\">\n";
    _l.module_open  = "  <module name=\"{module_name}\">\n";
    _l.unit_open    = "    <test name=\"{curr_test_kind}\">\n";
    _l.check_line   = "      <check index=\"{check_index}\" status=\"{check_status}\">{check_desc}</check>\n";
    _l.unit_close   = "    </test>\n";
    _l.module_close = "  </module>\n";
    _l.run_close    = "</report>\n";
    return _l;
}

D_NODISCARD inline report_layout
layout_html()
{
    report_layout _l;
    _l.run_open     = "<!doctype html>\n<html>\n<head><meta charset=\"utf-8\"><title>{report_title}</title></head>\n<body>\n<h1>{report_title}</h1>\n";
    _l.module_open  = "<section>\n  <h2>{module_name}</h2>\n";
    _l.unit_open    = "  <h3>{curr_test_kind}</h3>\n  <ul>\n";
    _l.check_line   = "    <li>{check_index}. {check_desc} <strong>{check_status}</strong></li>\n";
    _l.unit_close   = "  </ul>\n";
    _l.module_close = "</section>\n";
    _l.run_close    = "</body>\n</html>\n";
    return _l;
}


///////////////////////////////////////////////////////////////////////////////
//  WHERE THIS LANDS IN test_output
//  ----------------------------------------------------------------------------
//  render_report_bytes / render_module_bytes pick the layout + sink for a format
//  and run the walk: text/markdown -> render_report_string(layout_for(fmt), .);
//  xml/html -> escaping_sink<...> + render_report(layout_xml()/layout_html(),.);
//  pdf -> render_report_pdf_bytes (test_render_pdf.hpp).  No test_document, no
//  binding-env object is threaded - the resolver IS the binding.
///////////////////////////////////////////////////////////////////////////////


NS_END  // test
NS_END  // djinterp


#endif  // D_ENV_LANG_IS_CPP17_OR_HIGHER


#endif  // DJINTERP_TEST_TEST_RENDER_
