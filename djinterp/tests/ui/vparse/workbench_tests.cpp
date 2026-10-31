/*******************************************************************************
* vparse [tests]                                             workbench_tests.cpp
*
* The window's suite: the model an event leaves, and the face a model has.
*   Nothing here opens a window or draws: the model's behaviour is a function,
* and the face is a uxoxo template, so both are tested as values. The face is
* read through a platform of the suite's own, which lists the elements a
* template holds.
*   What a run holds, what the session decides for it, its heat and its files
* are the model's, which both programs stand on, and are tested without
* uxoxo in workbench_core_tests.cpp; the sections here are the ones that need
* the window's model and its face.
*   A suite is a table of sections, as djinterp's own are; main() is a loop
* over the table (tool_tests.hpp).
*   LEVELS. uxoxo's floor is C++17, so this file's is: below it the file is
* empty rather than an error.
*
*
* path:      /tests/ui/vparse/workbench_tests.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.06
*                                                            revised: 2026.10.26
*******************************************************************************/
#include "../../../inc/ui/vparse/view.hpp"  // the unit under test
#if D_ENV_LANG_IS_CPP17_OR_HIGHER  // the floor its header has

// std
#include <cstddef>    // std::size_t
#include <exception>  // std::exception
#include <string>     // std::string, std::to_string
#include <vector>     // std::vector
// uxoxo
#include "uxoxo/element.hpp"  // element_node, event, hole, option_set
#include "uxoxo/live.hpp"     // instantiate
#include "uxoxo/realize.hpp"  // realize
// vparse
#include "../../../inc/ui/vparse/elements.hpp"  // listing_type, tape_type, heat_type
#include "../../djinterp/tools/vparse/tool_tests.hpp"  // the suite's harness and grammars
#include "../../../inc/djinterp/tools/vparse/run.hpp"  // execute,
                                                          // headline,
                                                          // part_listing


namespace
{

namespace wb = ::vparse::workbench;

using ::uxoxo::event;
using ::uxoxo::option_set;
using ::vparse::tests::SUMS;
using ::vparse::tests::has;
using ::vparse::tests::request;
using ::vparse::tests::section;

// carrying
//   function: an event of a kind, whose payload's "value" is the one given.
template<typename Value>
event carrying(
    const std::string& _kind,
    const Value&       _value
)
{
    option_set payload;

    payload.set("value", _value);

    return event{ _kind, payload };
}

// bare
//   function: an event of a kind, with no payload.
event bare(
    const std::string& _kind
)
{
    return event{ _kind, option_set{} };
}

// inventory
//   platform: a face as the list of its elements, parents first, each as its
// type's name and the attributes these tests read. A hole is listed as its
// slot in angle brackets.
struct inventory
{
    using blueprint = std::vector<std::string>;

    struct on_pure
    {
        blueprint operator()(
            const ::uxoxo::hole& _hole
        ) const
        {
            return blueprint{ "<" + _hole.slot + ">" };
        }
    };

    struct on_archetype
    {
        blueprint operator()(
            const ::uxoxo::element_node<blueprint>& _node
        ) const
        {
            std::string line = _node.type->name;

            for (const char* key : { "text", "part", "action" })
            {
                const std::string value = _node.attrs.as_string(key, "");

                if (!value.empty())
                {
                    line += std::string(" ") + key + "=" + value;
                }
            }

            for (const char* key : { "run", "other", "step" })
            {
                const long value = _node.attrs.as_long(key, -2L);

                if (value != -2L)
                {
                    line += std::string(" ") + key + "=" +
                            std::to_string(value);
                }
            }

            blueprint out{ line };

            for (const blueprint& child : _node.children)
            {
                out.insert(out.end(), child.begin(), child.end());
            }

            return out;
        }
    };
};

// count
//   function: how many lines of an inventory begin with a prefix.
std::size_t count(
    const std::vector<std::string>& _lines,
    const std::string&              _prefix
)
{
    std::size_t n = 0u;

    for (const std::string& line : _lines)
    {
        n += (line.rfind(_prefix, 0) == 0u) ? 1u : 0u;
    }

    return n;
}

// listed
//   function: whether any line of an inventory begins with one prefix and
// contains another text.
bool listed(
    const std::vector<std::string>& _lines,
    const std::string&              _prefix,
    const std::string&              _part
)
{
    for (const std::string& line : _lines)
    {
        if ( (line.rfind(_prefix, 0) == 0u) &&
             (has(line, _part)) )
        {
            return true;
        }
    }

    return false;
}

/*
tests_workbench_example
  The model the workbench opens with runs its example, and every part of a
run has something to show.
  Tests the following:
  - the opening model's grammar and input match, as run 1, selected
  - the headline and the status say so
  - every part has rows, or a note saying why it has none, for a run that
    matched and for one that could not be read
  - the summary counts the revisits
*/
bool tests_workbench_example()
{
    wb::model bench = wb::make_model();

    wb::update(bench, bare("run"));

    if ( (bench.runs.size() != 1u)  ||
         (bench.selected != 0)      ||
         (bench.runs[0].id != 1) )
    {
        return false;
    }

    const wb::run_record& run = bench.runs[0];

    if ( (!run.recognition.matched)                          ||
         (run.recognition.consumed != bench.input.size())    ||
         (!has(wb::headline(run), "Matched all"))             ||
         (bench.status_error)                                 ||
         (!has(bench.status, "Run 1")) )
    {
        return false;
    }

    const wb::run_record unread = wb::execute(2, request("notation",
                                                         "Sum = Term More*",
                                                         "1"));

    for (const wb::run_record* each : { &run, &unread })
    {
        for (const std::string& part : wb::part_names())
        {
            const wb::listing shown = wb::part_listing(*each, part);

            if ( (shown.rows.empty()) &&
                 (shown.note.empty()) )
            {
                return false;
            }
        }
    }

    const wb::listing summary = wb::part_listing(run, "summary");

    for (const std::vector<std::string>& row : summary.rows)
    {
        if ( (!row.empty()) &&
             (row[0] == "revisits") )
        {
            return true;
        }
    }

    return false;
}

/*
tests_workbench_update
  The model after each event is the model the event describes.
  Tests the following:
  - editing, the front end, the trace switch and the step limit change the
    editors, and the step limit keeps only digits
  - run adds a run and selects it; a failed run opens at the step where it
    got furthest, and the tab that explains it is brought forward
  - selecting, comparing (and comparing again to stop), and the steps
  - the heat measure, and edit_run putting a run back in the editors
  - an event of a kind the workbench does not have changes nothing
*/
bool tests_workbench_update()
{
    wb::model bench = wb::make_model();

    wb::update(bench, carrying("edit_grammar", std::string(SUMS)));
    wb::update(bench, carrying("edit_input", std::string("12+3-*45")));
    wb::update(bench, carrying("step_limit", std::string("9a8 7")));
    wb::update(bench, carrying("trace", false));
    wb::update(bench, carrying("trace", true));

    if ( (bench.grammar != SUMS)       ||
         (bench.input != "12+3-*45")   ||
         (bench.step_limit != "987")   ||
         (!bench.trace) )
    {
        return false;
    }

    wb::update(bench, carrying("step_limit", std::string("100000")));
    wb::update(bench, bare("run"));

    const wb::run_record& failed = bench.runs.back();

    if ( (bench.runs.size() != 1u)                               ||
         (bench.step != failed.recognition.far.step)             ||
         (!bench.status_error) )
    {
        return false;
    }

    wb::update(bench, carrying("frontend", 1L));
    wb::update(bench, carrying("edit_grammar", std::string("S = ")));
    wb::update(bench, bare("run"));

    if ( (bench.frontend != "ebnf")            ||
         (bench.runs.size() != 2u)             ||
         (bench.selected != 1)                 ||
         (bench.force_tab != "Reading") )
    {
        return false;
    }

    wb::update(bench, carrying("select_run", 0L));
    wb::update(bench, carrying("compare_run", 1L));

    if ( (bench.selected != 0) ||
         (bench.compare != 1) )
    {
        return false;
    }

    wb::update(bench, carrying("compare_run", 1L));
    wb::update(bench, carrying("step", -5L));
    wb::update(bench, carrying("reading_step", 12L));
    wb::update(bench, carrying("heat_measure", 1L));

    if ( (bench.compare != -1)        ||
         (bench.step != 0)            ||
         (bench.reading_step != 12)   ||
         (bench.heat_arrivals) )
    {
        return false;
    }

    wb::update(bench, bare("edit_run"));

    if ( (bench.frontend != "notation")  ||
         (bench.grammar != SUMS)         ||
         (bench.input != "12+3-*45") )
    {
        return false;
    }

    const wb::model before = bench;

    wb::update(bench, carrying("no_such_event", 3L));
    wb::update(bench, carrying("select_run", 7L));

    return ( (bench.selected == before.selected)       &&
             (bench.runs.size() == before.runs.size())  &&
             (bench.grammar == before.grammar)          &&
             (bench.step == before.step) );
}

/*
tests_workbench_face
  The face is a whole template for every state the model can be in, and it
shows what the model holds.
  Tests the following:
  - with no runs, with a match, with a failure, with an unread grammar and
    with two runs compared, the face instantiates: it has no hole
  - with no runs it shows no listing, tape or heat element
  - with a notation run it has two tapes -- the input's, and the grammar's
    on the Reading tab -- and a heat element naming the run, and the tab
    list includes Heat
  - comparing names the other run in the heat element
*/
bool tests_workbench_face()
{
    wb::model empty = wb::make_model();
    wb::model matched = wb::make_model();
    wb::model failed  = wb::make_model();
    wb::model unread  = wb::make_model();

    wb::update(matched, bare("run"));
    wb::update(failed, carrying("edit_input", std::string("12+")));
    wb::update(failed, bare("run"));
    wb::update(unread, carrying("edit_grammar", std::string("Sum = ")));
    wb::update(unread, bare("run"));

    wb::model compared = matched;

    wb::update(compared, carrying("edit_input", std::string("1-2")));
    wb::update(compared, bare("run"));
    wb::update(compared, carrying("compare_run", 0L));

    try
    {
        for (const wb::model* each :
             { &empty, &matched, &failed, &unread, &compared })
        {
            (void)::uxoxo::instantiate(wb::view(*each));
        }
    }
    catch (const std::exception&)
    {
        return false;
    }

    const std::vector<std::string> none =
        ::uxoxo::realize<inventory>(wb::view(empty));
    const std::vector<std::string> one =
        ::uxoxo::realize<inventory>(wb::view(matched));
    const std::vector<std::string> two =
        ::uxoxo::realize<inventory>(wb::view(compared));

    if ( (count(none, wb::listing_type()->name) != 0u)  ||
         (count(none, wb::tape_type()->name) != 0u)     ||
         (count(none, wb::heat_type()->name) != 0u)     ||
         (count(none, "<") != 0u) )
    {
        return false;
    }

    if ( (count(one, wb::tape_type()->name) != 2u)              ||
         (!listed(one, wb::heat_type()->name, "run=1"))         ||
         (!listed(one, "tab ", "text=Heat"))                    ||
         (!listed(one, "tab ", "text=Trace")) )
    {
        return false;
    }

    // run 2 is selected, and compared with run 1
    return ( (listed(two, wb::heat_type()->name, "run=2"))  &&
             (listed(two, wb::heat_type()->name, "other=1")) );
}

// SECTIONS
//   constant: the suite, in the order it runs.
const section SECTIONS[] =
{
    { "tests_workbench_example",  &tests_workbench_example  },
    { "tests_workbench_update",   &tests_workbench_update   },
    { "tests_workbench_face",     &tests_workbench_face     }
};

}  // anonymous namespace

/*
main
  Runs every section, then the summary line the build's test matches.
*/
int main()
{
    return ::vparse::tests::run_sections("vparse workbench tests",
                                         SECTIONS);
}

#endif  // floor, for now
