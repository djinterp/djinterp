/*******************************************************************************
* vparse [ui]                                                           view.cpp
*
* The workbench's view and update.
*   The screen is one shell template with five holes -- toolbar, editors, runs,
* files and detail -- closed each frame with uxoxo's closing pass, so the layout
* is written once and each region is built by its own function. Everything is
* made of uxoxo elements; the two the workbench defines itself, listing and
* tape, come from workbench_elements.
*
* path:      /src/ui/vparse/view.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.09.22
*                                                            revised: 2026.10.26
*******************************************************************************/
#include "../../../inc/ui/vparse/view.hpp"  // corresponding header
#if D_ENV_LANG_IS_CPP17_OR_HIGHER  // the floor the header has
// std
#include <algorithm>   // std::max, std::min
#include <cstdlib>     // std::strtol
#include <filesystem>  // std::filesystem::path
#include <utility>     // std::forward, std::move
// uxoxo
#include "uxoxo/closing.hpp"             // closing_env, close, hole_at
#include "uxoxo/element_button.hpp"      // button
#include "uxoxo/element_column.hpp"      // column
#include "uxoxo/element_label.hpp"       // label
#include "uxoxo/element_panel.hpp"       // panel
#include "uxoxo/element_row.hpp"         // row
#include "uxoxo/element_selectable.hpp"  // selectable
#include "uxoxo/element_separator.hpp"   // separator
#include "uxoxo/element_tabs.hpp"        // tabs, tab
#include "uxoxo/element_text_field.hpp"  // text_field
#include "uxoxo/element_toggle.hpp"      // toggle
// vparse
#include "../../../inc/ui/vparse/elements.hpp"  // listing_view, tape_view
#include "../../../inc/djinterp/tools/vparse/store.hpp"  // save_run,
                                                               // list_saved,
                                                               // load_request,
                                                               // UTF-8 paths


namespace vparse
{
namespace workbench
{

namespace
{

using uxoxo::element_template;
using uxoxo::option_set;
using uxoxo::component::button;
using uxoxo::component::column;
using uxoxo::component::label;
using uxoxo::component::panel;
using uxoxo::component::row;
using uxoxo::component::selectable;
using uxoxo::component::separator;
using uxoxo::component::tab;
using uxoxo::component::tabs;
using uxoxo::component::text_field;
using uxoxo::component::toggle;

namespace fs = std::filesystem;

// with
//   builds an attribute record inline: with("tone", "muted")("wrap", true).
struct with
{
    option_set record;

    template<typename Value>
    with(
        const std::string& _key,
        Value&&            _value
    )
    {
        record.set(_key, std::forward<Value>(_value));
    }

    template<typename Value>
    with& operator()(
        const std::string& _key,
        Value&&            _value
    )
    {
        record.set(_key, std::forward<Value>(_value));

        return *this;
    }

    operator option_set() const
    {
        return record;
    }
};

// short_status
//   a run's outcome in a few words, for the runs list.
std::string short_status(
    const run_record& _run
)
{
    const parse_pass& rec = _run.recognition;

    if (!_run.read_ok)            { return "grammar not read"; }
    if (!_run.built)              { return "not built"; }
    if (!rec.ran)                 { return "did not run"; }
    if (rec.error == "step limit") { return "hit the step limit"; }

    if (rec.matched)
    {
        return "matched " + std::to_string(rec.consumed) + " chars";
    }

    if (rec.far.offset < 0)
    {
        return "no match";
    }

    const std::pair<int, int> at = line_col(_run.request.input,
                                            rec.far.offset);

    return "no match, stopped at " + std::to_string(at.first) + ":" +
           std::to_string(at.second);
}

bool succeeded(
    const run_record& _run
)
{
    return ( (_run.read_ok) &&
             (_run.built) &&
             (_run.recognition.matched) );
}

//------------------------------------------------------------------ regions

element_template toolbar(
    const model& _wb
)
{
    const bool notation = (_wb.frontend != "ebnf");

    return row({
        button(label("Run"), true,
               with("action", "run")("tone", "accent")
                   ("tooltip", "Run the grammar over the input "
                               "(Ctrl+Enter)")),
        label("front end", with("tone", "muted")),
        selectable("notation", notation, "frontend", 0,
                   with("width", 78.0)),
        selectable("EBNF", !notation, "frontend", 1, with("width", 44.0)),
        toggle("record trace", _wb.trace, "trace",
               with("tooltip", "The trace is what the tape, the matches "
                               "and the failure report are read from")),
        label("step limit", with("tone", "muted")),
        text_field(_wb.step_limit, "step_limit", with("width", 90.0)),
        label(_wb.status,
              with("tone", _wb.status_error ? "error" : "muted")) }, 2);
}

element_template editors(
    const model& _wb
)
{
    const bool        notation = (_wb.frontend != "ebnf");
    const std::string hint =
        notation ? "Name = alternatives ;  terms: 'c'  [a-z]  Ref, "
                   "with * and + after a term; | between alternatives"
                 : "name = expression ;  with ( ) | ? * +  'text'  [set]  "
                   ".  !x  and # comments";

    return column({
        label("Grammar", with("tone", "heading")),
        label(hint, with("tone", "muted")("wrap", true)),
        text_field(_wb.grammar, "edit_grammar",
                   with("multiline", true)("height", 0.38)),
        label("Input", with("tone", "heading")),
        text_field(_wb.input, "edit_input",
                   with("multiline", true)("height", 0.16)) });
}

element_template runs_list(
    const model& _wb
)
{
    std::vector<element_template> rows;

    // newest first
    for (std::size_t n = _wb.runs.size(); n-- > 0; )
    {
        const run_record& run = _wb.runs[n];
        const long        at  = static_cast<long>(n);
        const std::string line =
            "#" + std::to_string(run.id) + "  " +
            ((run.request.frontend == "ebnf") ? "EBNF    " : "notation") +
            "  " + short_status(run);

        rows.push_back(row({
            selectable("vs", _wb.compare == static_cast<int>(n),
                       "compare_run", at,
                       with("width", 24.0)
                           ("tooltip", "Compare the selected run with this "
                                       "one, part by part")),
            selectable(line, _wb.selected == static_cast<int>(n),
                       "select_run", at,
                       with("tone", succeeded(run) ? "ok" : "error")) }, 1));
    }

    if (rows.empty())
    {
        rows.push_back(label("No runs yet.", with("tone", "muted")));
    }

    return column({
        separator("Runs"),
        panel("runs", std::move(rows),
              with("height", 0.4)("border", true)) });
}

element_template files(
    const model& _wb
)
{
    std::vector<element_template> rows;

    for (std::size_t i = 0; i < _wb.saved.size(); ++i)
    {
        const saved_run&  s    = _wb.saved[i];
        const std::string line = s.session + "/" +
                                 utf8_from_path(s.folder.filename()) +
                                 "  " + s.frontend;

        rows.push_back(selectable(line, false, "load_saved",
                                  static_cast<long>(i),
                                  with("tooltip", s.result + "\n" +
                                                  utf8_from_path(s.folder))));
    }

    if (rows.empty())
    {
        rows.push_back(label("Nothing saved under " + _wb.root + " yet.",
                             with("tone", "muted")("wrap", true)));
    }

    const bool any = !_wb.runs.empty();

    return column({
        separator("Files"),
        text_field(_wb.root, "root",
                   with("placeholder", "folder for saved runs")
                       ("tooltip", "Runs are saved to <folder>/" +
                                   _wb.session + "/run-NNN")),
        row({ button(label("Save run"), any, with("action", "save_run")),
              button(label("Save all runs"), any,
                     with("action", "save_all")),
              toggle("save each run", _wb.save_each, "save_each",
                     with("tooltip", "Save every run as it finishes")) }),
        row({ label("Saved runs", with("tone", "muted")),
              button(label("Refresh"), true,
                     with("action", "refresh_saved")) }),
        panel("saved", std::move(rows), with("border", true)) });
}

element_template reading_tab(
    const run_record& _run,
    const model&      _wb,
    int               _other,
    bool              _tape_above
)
{
    if (_run.request.frontend == "ebnf")
    {
        return listing_view("reading", _run.id, _other);
    }

    std::vector<element_template> parts;

    // the grammar's tape, unless the detail view already shows it
    if (!_tape_above)
    {
        parts.push_back(panel("grammar_tape",
                              { tape_view(_run.id, "grammar",
                                          _wb.reading_step,
                                          "reading_step") },
                              with("height", 0.4)("border", true)
                                  ("resize", "y")));
    }

    parts.push_back(
        tabs("reading_parts",
             { tab("Captures", { listing_view("reading", _run.id, _other) }),
               tab("Trace",
                   { listing_view("reading_trace", _run.id, _other,
                                  _wb.reading_step,
                                  "reading_step") },
                   _wb.force_tab == part_title("reading_trace")) }));

    return column(std::move(parts));
}

// heat_tab
//   the Heat tab: which measure to map, then the map, its hot spots, and the
// profile, what the run counted rule by rule.
element_template heat_tab(
    const run_record& _run,
    const model&      _wb,
    int               _other
)
{
    return column({
        row({ label("measure", with("tone", "muted")),
              selectable("arrivals", _wb.heat_arrivals, "heat_measure", 0,
                         with("width", 70.0)),
              selectable("steps", !_wb.heat_arrivals, "heat_measure", 1,
                         with("width", 48.0)),
              label(_wb.heat_arrivals
                        ? "arrivals: separate times the parser came to "
                          "each place -- re-parsing"
                        : "steps: machine steps spent at each place",
                    with("tone", "muted")) }, 2),
        tabs("heat_parts",
             { tab("Map", { heat_view(_run.id, _other, _wb.step,
                                      _wb.heat_arrivals, "step") }),
               tab("Hot spots", { listing_view("heat", _run.id,
                                               _other) }),
               tab("Profile",
                   { listing_view("profile", _run.id,
                                  _other) },
                   _wb.force_tab == part_title("profile")) }) });
}

element_template detail(
    const model& _wb
)
{
    if ( (_wb.selected < 0) ||
         (static_cast<std::size_t>(_wb.selected) >= _wb.runs.size()) )
    {
        return column({
            label("No runs yet", with("tone", "heading")),
            label("Write a grammar and an input, then press Run. Every run "
                  "is kept: each stage can be inspected here, compared with "
                  "another run, and saved to files.",
                  with("tone", "muted")("wrap", true)) });
    }

    const run_record& run   = _wb.runs[static_cast<std::size_t>(_wb.selected)];
    const bool        other_ok =
        ( (_wb.compare >= 0) &&
          (static_cast<std::size_t>(_wb.compare) < _wb.runs.size()) &&
          (_wb.compare != _wb.selected) );
    const int         other = other_ok
        ? _wb.runs[static_cast<std::size_t>(_wb.compare)].id : -1;

    // the tape shows the grammar when the grammar is what went wrong
    const bool grammar_tape = ( (!run.read_ok) &&
                                (run.request.frontend != "ebnf") );

    const parse_pass& rec = run.recognition;
    const long        ip  =
        ( (_wb.step >= 0) &&
          (static_cast<std::size_t>(_wb.step) < rec.trace.size()) )
            ? rec.trace[static_cast<std::size_t>(_wb.step)].ip : -1;

    std::vector<element_template> actions = {
        button(label("Edit this run"), true,
               with("action", "edit_run")
                   ("tooltip", "Put this run's grammar and input back in "
                               "the editors")) };

    if (other_ok)
    {
        actions.push_back(label("Compared with run " + std::to_string(other) +
                                "; rows only one run has are washed red.",
                                with("tone", "muted")));
        actions.push_back(button(label("Stop comparing"), true,
                                 with("action", "stop_compare")));
    }
    else
    {
        actions.push_back(label("Pick vs on another run to compare them.",
                                with("tone", "muted")));
    }

    // the page to bring to the front: the one the model names, or the one
    // whose own tabs show the part the model names
    std::string front = _wb.force_tab;

    // a part shown on a tab of the Reading page, or of the Heat page
    if (front == part_title("reading_trace"))
    {
        front = part_title("reading");
    }
    else if (front == part_title("profile"))
    {
        front = part_title("heat");
    }

    const auto page = [&front](const std::string& _title,
                               std::vector<element_template> _content)
    {
        return tab(_title,
                   std::move(_content),
                   front == _title);
    };

    return column({
        row({ label("Run " + std::to_string(run.id),
                    with("tone", "heading")),
              label(((run.request.frontend == "ebnf") ? "EBNF" : "notation") +
                        std::string(", ") + run.when,
                    with("tone", "muted")) }),
        label(headline(run),
              with("tone", succeeded(run) ? "ok" : "error")("wrap", true)),
        row(std::move(actions)),
        panel("tape",
              { grammar_tape
                    ? tape_view(run.id, "grammar", _wb.reading_step,
                                "reading_step")
                    : tape_view(run.id, "input", _wb.step, "step") },
              with("height", 0.3)("border", true)("resize", "y")),
        tabs("parts", {
            page("Summary", { listing_view("summary", run.id, other) }),
            page("Grammar", { listing_view("grammar", run.id, other) }),
            page("Reading", { reading_tab(run, _wb, other,
                                          grammar_tape) }),
            page("Rules",   { listing_view("rules", run.id, other) }),
            page("Plan",    { listing_view("plan", run.id, other) }),
            page("Program", { listing_view("program", run.id, other, ip) }),
            page("Matches", { listing_view("matches", run.id, other) }),
            page("Heat",    { heat_tab(run, _wb, other) }),
            page("Trace",   { listing_view("trace", run.id, other, _wb.step,
                                           "step") }) }) });
}

//------------------------------------------------------------------ update

void set_status(
    model&             _wb,
    const std::string& _text,
    bool               _error
)
{
    _wb.status       = _text;
    _wb.status_error = _error;

    return;
}

// telling_step
//   the step worth opening a pass at: the last failure at the frontier when
// the pass failed, otherwise the final step.
long telling_step(
    const parse_pass& _pass
)
{
    if ( (!_pass.matched) &&
         (_pass.far.step >= 0) )
    {
        return _pass.far.step;
    }

    return _pass.trace.empty() ? 0
                               : static_cast<long>(_pass.trace.size()) - 1;
}

void select(
    model& _wb,
    long   _index
)
{
    if ( (_index < 0) ||
         (static_cast<std::size_t>(_index) >= _wb.runs.size()) )
    {
        return;
    }

    const run_record& run = _wb.runs[static_cast<std::size_t>(_index)];

    _wb.selected     = static_cast<int>(_index);
    _wb.step         = telling_step(run.recognition);
    _wb.reading_step = telling_step(run.reading);

    if (_wb.compare == _wb.selected)
    {
        _wb.compare = -1;
    }

    return;
}

fs::path session_folder(
    const model& _wb
)
{
    return path_from_utf8(_wb.root.empty() ? std::string("vparse_runs")
                                           : _wb.root) /
           _wb.session;
}

bool save_one(
    model&            _wb,
    const run_record& _run
)
{
    std::string    error;
    const fs::path folder = save_run(_run, session_folder(_wb), &error);

    if (folder.empty())
    {
        set_status(_wb, "Could not save run " + std::to_string(_run.id) +
                        ": " + error, true);

        return false;
    }

    set_status(_wb, error.empty()
                        ? "Saved run " + std::to_string(_run.id) + " to " +
                              utf8_from_path(folder)
                        : error,
               !error.empty());

    return true;
}

void add_run(
    model&             _wb,
    const run_request& _request
)
{
    _wb.runs.push_back(execute(_wb.next_id++, _request));
    select(_wb, static_cast<long>(_wb.runs.size()) - 1);

    const run_record& run = _wb.runs.back();

    // open the part that explains a failure
    if (!run.read_ok)
    {
        _wb.force_tab = "Reading";
    }
    else if (!run.built)
    {
        _wb.force_tab = "Rules";
    }

    set_status(_wb, "Run " + std::to_string(run.id) + ": " +
                    short_status(run), !succeeded(run));

    if (_wb.save_each)
    {
        save_one(_wb, run);
        _wb.saved = list_saved(path_from_utf8(_wb.root));
    }

    return;
}

void run_now(
    model& _wb
)
{
    run_request request;
    request.frontend = _wb.frontend;
    request.grammar  = _wb.grammar;
    request.input    = _wb.input;
    request.trace    = _wb.trace;

    const long limit = std::strtol(_wb.step_limit.c_str(), nullptr, 10);
    request.step_limit = std::max(1L, (limit > 0) ? limit : 100000L);

    add_run(_wb, request);

    return;
}

void load_folder(
    model&             _wb,
    const std::string& _folder
)
{
    run_request request;
    std::string error;

    // a folder whose run cannot be read back
    if (!load_request(path_from_utf8(_folder),
                      request,
                      &error))
    {
        set_status(_wb, "Could not load: " + error, true);

        return;
    }

    _wb.frontend = request.frontend;
    _wb.grammar  = request.grammar;
    _wb.input    = request.input;
    _wb.trace    = request.trace;

    add_run(_wb, request);

    const bool ok = succeeded(_wb.runs.back());
    set_status(_wb, "Loaded " + _folder + " as run " +
                    std::to_string(_wb.runs.back().id) + ": " +
                    short_status(_wb.runs.back()), !ok);

    return;
}

}  // anonymous namespace


/*
make_model
  The example is small enough to read at a glance and exercises choice, a
repeated rule and a class: an arithmetic sum.
*/
model make_model()
{
    model wb;

    wb.grammar = "Sum  = Term More* ;\n"
                 "More = '+' Term | '-' Term ;\n"
                 "Term = [0-9]+ ;\n";
    wb.input   = "12+3-45";
    wb.session = session_name();

    return wb;
}

/*
view
  One shell, five holes, closed with uxoxo's closing pass.
*/
uxoxo::element_template view(
    const model& _model
)
{
    using uxoxo::hole_at;

    const element_template shell = column({
        hole_at("toolbar"),
        row({ panel("left",
                    { hole_at("editors"), hole_at("runs"), hole_at("files") },
                    with("width", 0.3)("resize", "x")),
              panel("right", { hole_at("detail") }) }, 1) });

    uxoxo::closing_env env;
    env.set("toolbar", toolbar(_model));
    env.set("editors", editors(_model));
    env.set("runs", runs_list(_model));
    env.set("files", files(_model));
    env.set("detail", detail(_model));

    return uxoxo::close(shell, env);
}

/*
update
  Each kind maps to one change of state; running, saving and loading are the
only ones with effects beyond the model.
*/
void update(
    model&              _model,
    const uxoxo::event& _event
)
{
    model&         wb    = _model;
    const std::string& kind  = _event.kind;
    const option_set&  load  = _event.payload;
    const long         index = load.as_long("value", -1);

    if (kind == "run")
    {
        run_now(wb);
    }
    else if (kind == "edit_grammar")
    {
        wb.grammar = load.as_string("value", wb.grammar);
    }
    else if (kind == "edit_input")
    {
        wb.input = load.as_string("value", wb.input);
    }
    else if (kind == "frontend")
    {
        wb.frontend = (index == 1) ? "ebnf" : "notation";
    }
    else if (kind == "trace")
    {
        wb.trace = load.as_bool("value", wb.trace);
    }
    else if (kind == "step_limit")
    {
        std::string digits;

        for (const char c : load.as_string("value", ""))
        {
            if ( (c >= '0') && (c <= '9') && (digits.size() < 10) )
            {
                digits.push_back(c);
            }
        }

        wb.step_limit = digits;
    }
    else if (kind == "select_run")
    {
        select(wb, index);
    }
    else if (kind == "compare_run")
    {
        wb.compare = (wb.compare == static_cast<int>(index))
                         ? -1 : static_cast<int>(index);
    }
    else if (kind == "stop_compare")
    {
        wb.compare = -1;
    }
    else if (kind == "step")
    {
        wb.step = std::max(0L, index);
    }
    else if (kind == "reading_step")
    {
        wb.reading_step = std::max(0L, index);
    }
    else if (kind == "heat_measure")
    {
        wb.heat_arrivals = (index != 1);
    }
    else if ( (kind == "edit_run") &&
              (wb.selected >= 0) )
    {
        const run_request& request =
            wb.runs[static_cast<std::size_t>(wb.selected)].request;

        wb.frontend = request.frontend;
        wb.grammar  = request.grammar;
        wb.input    = request.input;
        wb.trace    = request.trace;
        set_status(wb, "The editors hold run " +
                       std::to_string(wb.runs[static_cast<std::size_t>(
                           wb.selected)].id) + " again.", false);
    }
    else if (kind == "root")
    {
        wb.root = load.as_string("value", wb.root);
    }
    else if ( (kind == "save_run") &&
              (wb.selected >= 0) )
    {
        save_one(wb, wb.runs[static_cast<std::size_t>(wb.selected)]);
        wb.saved = list_saved(path_from_utf8(wb.root));
    }
    else if (kind == "save_all")
    {
        std::size_t saved = 0;

        for (const run_record& run : wb.runs)
        {
            saved += save_one(wb, run) ? 1 : 0;
        }

        if (saved == wb.runs.size())
        {
            set_status(wb, "Saved " + std::to_string(saved) + " runs to " +
                           utf8_from_path(session_folder(wb)), false);
        }

        wb.saved = list_saved(path_from_utf8(wb.root));
    }
    else if (kind == "save_each")
    {
        wb.save_each = load.as_bool("value", wb.save_each);
    }
    else if (kind == "refresh_saved")
    {
        wb.saved = list_saved(path_from_utf8(wb.root));
        set_status(wb, "Found " + std::to_string(wb.saved.size()) +
                       " saved runs under " + wb.root, false);
    }
    else if ( (kind == "load_saved") &&
              (index >= 0) &&
              (static_cast<std::size_t>(index) < wb.saved.size()) )
    {
        load_folder(wb,
                    utf8_from_path(
                        wb.saved[static_cast<std::size_t>(index)].folder));
    }
    else if (kind == "load_folder")
    {
        load_folder(wb, load.as_string("value", ""));
    }

    return;
}

/*
find_run
  Runs are few; a scan is fine.
*/
const run_record* find_run(
    const model& _model,
    int          _id
)
{
    for (const run_record& run : _model.runs)
    {
        if (run.id == _id)
        {
            return &run;
        }
    }

    return nullptr;
}

}  // namespace workbench
}  // namespace vparse

#endif  // floor, for now
