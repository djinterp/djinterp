/*******************************************************************************
* vparse [cli]                                                      commands.cpp
*
* Definitions for commands.hpp: what the subcommands over one grammar and
* over saved runs do; bench, corpus and compare are measures.cpp's, and the
* steps more than one subcommand takes are common.hpp's.
*   A subcommand reads every file it is given first, then runs, then makes
* its output whole and writes it in one piece: nothing is printed until
* nothing is left to fail but the writing itself. A grammar refused is said
* as an error, through the kit, and a file compile writes is written whole
* or not at all, by the kit.
*   A run is the model's execute(), the function the window's Run button
* calls, so the command line and the window cannot disagree about a run; the
* listing compile writes is the session's own disassembly, which the model's
* program is held to, instruction for instruction.
*   A RUN IS TRACED FOR WHAT IT EXPLAINS. A trace is what a failure's
* frontier and a saved run's heat are recovered from, and it costs memory in
* proportion to the steps. match runs each input untraced and runs a failed
* one again, traced, to say where it stopped -- the machine is
* deterministic, so the second run is the first one, watched -- unless it is
* given -q or --no-trace. A run match saves is traced from the start.
* check and compile never trace: where a grammar's reading stops, the model
* reads it again, traced, to say where.
*
* path:      /src/djinterp/tools/vparse/cli/commands.cpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.26
*******************************************************************************/
#include "./commands.hpp"  // corresponding header
#if D_ENV_LANG_IS_CPP17_OR_HIGHER  // the floor the header has

// std
#include <algorithm>     // std::find, std::max
#include <cstddef>       // std::size_t
#include <cstdlib>       // std::atoi
#include <filesystem>    // std::filesystem
#include <string>        // std::string, std::to_string
#include <system_error>  // std::error_code
#include <utility>       // std::move
#include <vector>        // std::vector
// djinterp
#include "djinterp/c/modules/parse/cli/arg_parser/arg_parser.h"  // option values
#include "djinterp/cpp/modules/parsegen/vparse/session.hpp"          // session, frontend
#include "djinterp/tools/cli/c/cli_io.h"                 // d_cli_write_file
#include "djinterp/tools/cli/c/cli_program.h"            // d_cli_file_error
// vparse
#include "./common.hpp"  // options, reading, endings, requests
#include "./report.hpp"  // json_object, json_array, json_listing,
                         // parts_text
#include "../../../../../inc/djinterp/tools/vparse/run.hpp"    // execute,
                                                               // headline,
                                                               // part_names
#include "../../../../../inc/djinterp/tools/vparse/store.hpp"  // save_run,
                                                               // list_saved,
                                                               // load_request,
                                                               // files by name


namespace vparse
{
namespace cli
{

namespace
{

namespace fs = std::filesystem;
namespace vp = ::djinterp::parsegen::vparse;
namespace wb = ::vparse::workbench;

// ALL_PARTS
//   constant: --part's value for every part a run has.
const char* const ALL_PARTS = "all";

// outcome
//   struct: one input's run, as match reports it: the input's name and the
// argument that named it, the run, and the folder it was saved to, if it
// was.
struct outcome
{
    std::string    name;
    std::string    argument;
    wb::run_record run;
    std::string    saved;
};

// match_report
//   struct: what match found: the grammar's name and the frontend that read
// it, how many inputs there were, each input's run -- ending at the first
// whose grammar was refused, if one was -- and the session folder the runs
// were saved in, in UTF-8; empty when they were not.
struct match_report
{
    std::string          grammar;
    std::string          frontend;
    std::size_t          total = 0u;
    std::vector<outcome> outcomes;
    std::string          session;
};

//------------------------------------------------------------------------------
// options
//------------------------------------------------------------------------------

/**
 * @brief Works out the parts a run prints.
 *
 * @param[in] _parts  the subcommand's --part option.
 * @return the parts named, in order, each once; "all" stands for every part
 *         a run has, in their display order.
 */
std::vector<std::string> parts_of(
    struct dj_arg_option* _parts
)
{
    std::vector<std::string> out;

    // the parts named, in the order named
    for (const std::string& part : values_of(_parts))
    {
        const std::vector<std::string> named =
            (part == ALL_PARTS) ? wb::part_names()
                                : std::vector<std::string>(1u, part);

        // each part once, where it was first asked for
        for (const std::string& each : named)
        {
            // a part not yet asked for
            if (std::find(out.begin(),
                          out.end(),
                          each) == out.end())
            {
                out.push_back(each);
            }
        }
    }

    return out;
}

//------------------------------------------------------------------------------
// saving
//------------------------------------------------------------------------------

/**
 * @brief Finds the id the next run saved in a session folder takes.
 *
 * @param[in] _session  the session folder, which need not exist yet.
 * @return one more than the highest `run-NNN` folder in it; 1 for none.
 *
 * Two command lines run in the same second share a session folder, since a
 * session's name is the time to the second, and each has to find the
 * numbers the other took.
 */
int next_run_id(
    const fs::path& _session
)
{
    int             highest = 0;
    std::error_code code;

    // every entry of the session folder, until one cannot be read
    for (fs::DDirectoryIterator each(_session, code), end;
         ( (!code) &&
           (each != end) );
         each.increment(code))
    {
        const std::string name = wb::utf8_from_path(each->path().filename());

        // run folders are "run-" and a number
        if (name.rfind("run-", 0u) == 0u)
        {
            highest = std::max(highest,
                               std::atoi(name.c_str() + 4));
        }
    }

    return highest + 1;
}

/**
 * @brief Saves a run into a session folder.
 *
 * @param[in]  _run      the run, its id the folder's number.
 * @param[in]  _session  the session folder.
 * @param[out] _folder   receives the run's folder, in UTF-8.
 * @param[out] _error    receives why it was not saved, on failure.
 * @return whether the run was saved.
 */
bool save(
    const wb::run_record& _run,
    const fs::path&       _session,
    std::string&          _folder,
    std::string&          _error
)
{
    std::string    reason;
    const fs::path folder = wb::save_run(_run,
                                         _session,
                                         &reason);

    // not saved: a run saved with a complaint about the index is saved
    if (folder.empty())
    {
        _error = "cannot save the run: " + reason;

        return false;
    }

    _folder = wb::utf8_from_path(folder);

    return true;
}

//------------------------------------------------------------------------------
// what is printed
//------------------------------------------------------------------------------

/**
 * @brief Says what check found of an accepted grammar, as text; a refused
 *        one's reason is said as an error (common.hpp's refuse()).
 *
 * @param[in] _run  the run check made over an empty input, its grammar
 *                  accepted.
 * @return the verdict, one or two lines.
 *
 * An accepted grammar whose run over nothing ran to the step limit is left
 * recursive in all but name -- PEG makes no progress there -- so check says
 * so, though the session accepted it and the status says it did.
 */
std::string check_text(
    const wb::run_record& _run
)
{
    const std::size_t rules = _run.rule_names.size();
    std::string       out   = "The grammar was accepted: " +
                              std::to_string(rules) +
                              ((rules == 1u) ? " rule (" : " rules (") +
                              joined(_run.rule_names) + "), " +
                              std::to_string(_run.program.size()) +
                              " instructions.\n";

    // a run over nothing that never ends
    if (_run.recognition.error == "step limit")
    {
        out += "Over an empty input it ran to the step limit, which is how "
               "left recursion loops.\n";
    }

    return out;
}

/**
 * @brief Says what check found, as JSON.
 *
 * @param[in] _name      the grammar's name.
 * @param[in] _frontend  the frontend that read it.
 * @param[in] _run       the run check made over an empty input.
 * @return the JSON document, and a newline.
 */
std::string check_json(
    const std::string&    _name,
    const std::string&    _frontend,
    const wb::run_record& _run
)
{
    const bool  ok    = accepted(_run);
    json_object out;

    out.add_string("grammar",
                   _name)
       .add_string("frontend",
                   _frontend)
       .add_bool("read",
                 _run.read_ok)
       .add_bool("accepted",
                 ok)
       .add_string("error",
                   ok ? std::string()
                      : wb::headline(_run))
       .add("rules",
            json_strings(_run.rule_names))
       .add_count("instructions",
                  _run.program.size())
       .add_bool("stops_at_step_limit",
                 (ok) &&
                 (_run.recognition.error == "step limit"));

    return out.text(0u) + "\n";
}

/**
 * @brief Describes one run's outcome as a JSON object on one line.
 *
 * @param[in] _outcome  the input's run.
 * @param[in] _as_text  whether the input was given as text.
 * @return the object: the input as it was given -- a file name, `-`, or
 *         the text itself -- and how its run went.
 */
std::string outcome_json(
    const outcome& _outcome,
    bool           _as_text
)
{
    const wb::parse_pass& pass = _outcome.run.recognition;
    json_object           out;

    out.add_string("input",
                   _outcome.argument)
       .add_bool("as_text",
                 _as_text)
       .add_bool("matched",
                 pass.matched)
       .add_count("consumed",
                  pass.consumed)
       .add_count("steps",
                  static_cast<std::size_t>(std::max(0L,
                                                    pass.steps)))
       .add_string("headline",
                   wb::headline(_outcome.run))
       .add_string("saved",
                   _outcome.saved);

    return out.line();
}

/**
 * @brief Says what match found, as text.
 *
 * @param[in] _outcomes  each input's run, in order.
 * @param[in] _session   the session folder the runs were saved in; empty
 *                       when they were not.
 * @return a line an input, then, for more than one, how many matched.
 */
std::string match_text(
    const std::vector<outcome>& _outcomes,
    const std::string&          _session
)
{
    std::string out;
    std::size_t matched = 0u;

    // a line an input, and the tally of those that matched
    for (const outcome& each : _outcomes)
    {
        out     += each.name + ": " + wb::headline(each.run) + "\n";
        matched += each.run.recognition.matched ? 1u : 0u;
    }

    // a tally is worth a line only where there is more than one to count
    if (_outcomes.size() > 1u)
    {
        out += std::to_string(matched) + " of " +
               std::to_string(_outcomes.size()) + " inputs matched.\n";
    }

    // where the runs went
    if (!_session.empty())
    {
        out += "Saved to " + _session + ".\n";
    }

    return out;
}

/**
 * @brief Says what match found, as JSON.
 *
 * @param[in] _grammar   the grammar's name.
 * @param[in] _frontend  the frontend that read it.
 * @param[in] _outcomes  each input's run, in order.
 * @param[in] _as_text   whether the inputs were given as text.
 * @return the JSON document, and a newline.
 */
std::string match_json(
    const std::string&          _grammar,
    const std::string&          _frontend,
    const std::vector<outcome>& _outcomes,
    bool                        _as_text
)
{
    json_array  inputs;
    std::size_t matched = 0u;

    // an element an input, and the tally of those that matched
    for (const outcome& each : _outcomes)
    {
        inputs.add(outcome_json(each,
                                _as_text));
        matched += each.run.recognition.matched ? 1u : 0u;
    }

    json_object out;

    out.add_string("grammar",
                   _grammar)
       .add_string("frontend",
                   _frontend)
       .add_bool("accepted",
                 true)
       .add_string("error",
                   std::string())
       .add("inputs",
            inputs.text(2u))
       .add_count("matched",
                  matched)
       .add_count("total",
                  _outcomes.size());

    return out.text(0u) + "\n";
}

/**
 * @brief Says why match ran nothing, as JSON: the grammar was refused.
 *
 * @param[in] _grammar   the grammar's name.
 * @param[in] _frontend  the frontend that read it.
 * @param[in] _run       the run that found the grammar refused.
 * @param[in] _total     how many inputs there were.
 * @return the JSON document, and a newline.
 */
std::string refusal_json(
    const std::string&    _grammar,
    const std::string&    _frontend,
    const wb::run_record& _run,
    std::size_t           _total
)
{
    json_object out;

    out.add_string("grammar",
                   _grammar)
       .add_string("frontend",
                   _frontend)
       .add_bool("accepted",
                 false)
       .add_string("error",
                   wb::headline(_run))
       .add("inputs",
            "[]")
       .add_count("matched",
                  0u)
       .add_count("total",
                  _total);

    return out.text(0u) + "\n";
}

/**
 * @brief Prints a run's headline and parts, as text.
 *
 * @param[in] _run    the run.
 * @param[in] _parts  the parts to print.
 * @param[in] _quiet  whether to leave out the headline.
 * @param[in] _saved  the folder it was saved to; empty when it was not.
 * @return the headline, a blank line and the parts, as the workbench's
 *         --print printed one part; the parts alone when quiet.
 */
std::string run_text(
    const wb::run_record&           _run,
    const std::vector<std::string>& _parts,
    bool                            _quiet,
    const std::string&              _saved
)
{
    std::string out = _quiet ? std::string()
                             : wb::headline(_run) + "\n\n";

    out += parts_text(_run,
                      _parts);

    // where the run went, unless the reader asked for the parts alone
    if ( (!_saved.empty()) &&
         (!_quiet) )
    {
        out += "\nSaved to " + _saved + ".\n";
    }

    return out;
}

/**
 * @brief The members run's JSON ends with: its input by its argument, as
 *        match says it, since `input` keeps the meaning it had, the input's
 *        name (R19).
 *
 * @param[in] _over     the input; empty for a run over nothing.
 * @param[in] _as_text  whether it was given as text, -t.
 * @return the members, `argument` and `as_text`.
 */
json_object run_tail(
    const source& _over,
    bool          _as_text
)
{
    json_object out;

    out.add_string("argument",
                   _over.argument)
       .add_bool("as_text",
                 _as_text);

    return out;
}

/**
 * @brief Prints a run and its parts, as JSON.
 *
 * @param[in] _head   the members naming where the run came from -- the
 *                    grammar and the input, or the saved folder -- which
 *                    come first.
 * @param[in] _run    the run.
 * @param[in] _parts  the parts to include.
 * @param[in] _saved  the folder it was saved to; empty when it was not.
 * @param[in] _tail   members that come last: keys a later version added,
 *                    after the ones that were there (R19).
 * @return the JSON document, and a newline.
 */
std::string run_json(
    const json_object&              _head,
    const wb::run_record&           _run,
    const std::vector<std::string>& _parts,
    const std::string&              _saved,
    const json_object&              _tail
)
{
    // where the run came from, first
    json_object           out  = _head;
    const wb::parse_pass& pass = _run.recognition;
    json_object           parts;

    // a member a part, in the order asked for
    for (const std::string& part : _parts)
    {
        parts.add(part,
                  json_listing(wb::part_listing(_run,
                                                part),
                               wb::part_title(part),
                               4u));
    }

    out.add_string("frontend",
                   _run.request.frontend)
       .add_string("headline",
                   wb::headline(_run))
       .add_bool("read",
                 _run.read_ok)
       .add_bool("built",
                 _run.built)
       .add_bool("matched",
                 pass.matched)
       .add_count("consumed",
                  pass.consumed)
       .add_count("steps",
                  static_cast<std::size_t>(std::max(0L,
                                                    pass.steps)))
       .add_string("saved",
                   _saved)
       .add("parts",
            parts.text(2u))
       .add_all(_tail);

    return out.text(0u) + "\n";
}

/**
 * @brief Prints the runs saved under a root, as JSON.
 *
 * @param[in] _root  the root, as given.
 * @param[in] _runs  the runs found, newest first.
 * @return the JSON document, and a newline.
 */
std::string saved_json(
    const std::string&                _root,
    const std::vector<wb::saved_run>& _runs
)
{
    json_array runs;

    // a record a run, newest first
    for (const wb::saved_run& each : _runs)
    {
        json_object run;

        run.add_string("folder",
                       wb::utf8_from_path(each.folder))
           .add_string("session",
                       wb::utf8_from_path(each.folder.parent_path()
                                              .filename()))
           .add_count("run",
                      static_cast<std::size_t>(std::max(0,
                                                        each.id)))
           .add_string("frontend",
                       each.frontend)
           .add_string("when",
                       each.when)
           .add_string("result",
                       each.result);
        runs.add(run.line());
    }

    json_object out;

    out.add_string("root",
                   _root)
       .add("runs",
            runs.text(2u));

    return out.text(0u) + "\n";
}

/**
 * @brief Lays out the runs saved under a root, as a listing.
 *
 * @param[in] _root  the root, as given.
 * @param[in] _runs  the runs found, newest first.
 * @return the listing: a run's folder, front end, time and result a row.
 */
wb::listing saved_listing(
    const std::string&                _root,
    const std::vector<wb::saved_run>& _runs
)
{
    wb::listing out;

    out.columns = { "folder", "front end", "when", "result" };
    out.note    = "No runs are saved under " + _root + ".";

    // a row a run, newest first
    for (const wb::saved_run& each : _runs)
    {
        out.rows.push_back({ wb::utf8_from_path(each.folder),
                             each.frontend,
                             each.when,
                             each.result });
        out.marks.push_back(0);
    }

    return out;
}

//------------------------------------------------------------------------------
// the steps match and run take
//------------------------------------------------------------------------------

/**
 * @brief Runs a match's inputs, one run each.
 *
 * @param[in]     _command  the subcommand.
 * @param[in]     _context  the run of the command line: -q leaves an input
 *                          that did not match unexplained, as --no-trace
 *                          does.
 * @param[in]     _read     the grammar and the inputs, in order.
 * @param[in]     _session  the session folder runs are saved in; empty to
 *                          save none.
 * @param[in,out] _report   gives the frontend; receives each input's run, in
 *                          order, ending at the first run whose grammar was
 *                          refused.
 * @return an empty string, or why a run could not be saved.
 */
std::string run_inputs(
    const match_command& _command,
    const context&       _context,
    const sources&       _read,
    const fs::path&      _session,
    match_report&        _report
)
{
    const bool        saving  = !_session.empty();
    const bool        explain = ( (!quiet(_context)) &&
                                  (!given(_command.run.no_trace)) );
    const long        limit   = step_limit_of(_command.run);
    const std::string memo    = memo_of(_command.run.memoize);
    int               id      = saving ? next_run_id(_session) : 1;

    // a run an input, in order
    for (const source& input : _read.inputs)
    {
        outcome         each;
        wb::run_request request = request_for(_report.frontend,
                                              _read.grammar,
                                              input,
                                              saving,
                                              limit);

        request.memoize = memo;
        each.name       = input.name;
        each.argument   = input.argument;
        each.run        = wb::execute(id,
                                      request);

        // a refused grammar refuses every input alike
        if (!accepted(each.run))
        {
            _report.outcomes.push_back(std::move(each));

            return std::string();
        }

        // watched again, to say where it stopped
        if ( (!each.run.recognition.matched) &&
             (!saving)                       &&
             (explain) )
        {
            request.trace = true;
            each.run      = wb::execute(id,
                                        request);
        }

        std::string error;

        // a run that cannot be saved ends the subcommand
        if ( (saving) &&
             (!save(each.run,
                    _session,
                    each.saved,
                    error)) )
        {
            return error;
        }

        _report.outcomes.push_back(std::move(each));
        id++;
    }

    return std::string();
}

/**
 * @brief Works out the session folder a subcommand saves into.
 *
 * @param[in] _save  the subcommand's save options.
 * @return a new session folder under --root, named for the time; an empty
 *         path when --save was not given.
 */
fs::path session_of(
    const save_options& _save
)
{
    // not saving
    if (!given(_save.save))
    {
        return fs::path();
    }

    return wb::path_from_utf8(value_of(_save.root)) / wb::session_name();
}

/**
 * @brief Prints a run, and ends the subcommand that made it.
 *
 * @tparam Command  the subcommand's record: run_command or replay_command.
 * @param[in] _command  the subcommand's record, whose --part and output
 *                      options say what is printed.
 * @param[in] _context  where the output goes, and -q.
 * @param[in] _name     what a refused grammar is said of: the grammar, or
 *                      the saved run's folder.
 * @param[in] _head     the members naming where the run came from, first
 *                      in its JSON.
 * @param[in] _run      the run.
 * @param[in] _saved    the folder the run was saved to; empty when it was
 *                      not.
 * @param[in] _tail     the members that come last in its JSON (R19).
 * @return the run's exit status; REFUSED, too, when the output could not be
 *         written.
 *
 * A run whose grammar was refused is printed all the same -- its parts say
 * how far it got -- and the refusal is said as an error.
 */
template<typename Command>
int print_run(
    const Command&        _command,
    const context&        _context,
    const std::string&    _name,
    const json_object&    _head,
    const wb::run_record& _run,
    const std::string&    _saved,
    const json_object&    _tail
)
{
    const std::vector<std::string> parts = parts_of(_command.parts);
    const int                      ended = accepted(_run)
                                               ? status_of(_run)
                                               : refuse(_context,
                                                        _name,
                                                        _run);

    // JSON is whole: the headline is one of its members
    if (wants_json(_command.output))
    {
        return emit(_context,
                    run_json(_head,
                             _run,
                             parts,
                             _saved,
                             _tail),
                    ended);
    }

    return emit(_context,
                run_text(_run,
                         parts,
                         quiet(_context),
                         _saved),
                ended);
}

/**
 * @brief Prints what match found, and ends the subcommand.
 *
 * @param[in] _command  the subcommand.
 * @param[in] _context  where the output goes, and -q.
 * @param[in] _report   what match found.
 * @return REFUSED when the grammar was refused, said as an error, PASSED
 *         when every input matched, NO_MATCH otherwise; REFUSED, too, when
 *         the output could not be written.
 */
int print_match(
    const match_command& _command,
    const context&       _context,
    const match_report&  _report
)
{
    const bool json   = wants_json(_command.output);
    const bool silent = quiet(_context);

    // a refused grammar: the one run that found it out says why
    if ( (!_report.outcomes.empty()) &&
         (!accepted(_report.outcomes.back().run)) )
    {
        const wb::run_record& refused = _report.outcomes.back().run;
        const int             said    = refuse(_context,
                                               _report.grammar,
                                               refused);

        // JSON says it too, as a member, unless -q left it out
        return ( (json) &&
                 (!silent) ) ? emit(_context,
                                    refusal_json(_report.grammar,
                                                 _report.frontend,
                                                 refused,
                                                 _report.total),
                                    said)
                             : said;
    }

    int status = exit_status::PASSED;

    // one input that did not match is enough
    for (const outcome& each : _report.outcomes)
    {
        status = (each.run.recognition.matched) ? status
                                                : exit_status::NO_MATCH;
    }

    // the status says it all
    if (silent)
    {
        return status;
    }

    return emit(_context,
                json ? match_json(_report.grammar,
                                  _report.frontend,
                                  _report.outcomes,
                                  given(_command.text))
                     : match_text(_report.outcomes,
                                  _report.session),
                status);
}

/**
 * @brief Writes compile's listing: to the file -o names, or to the standard
 *        output.
 *
 * @param[in] _command  the subcommand.
 * @param[in] _context  where the listing, or a failure, is written.
 * @param[in] _listing  the program's listing.
 * @return PASSED; REFUSED when the listing could not be written, said as
 *         the kit says it: `cannot write 'FILE': REASON`.
 *
 * A file is written as the kit writes one (GRAMMAR.md, R20): into a new
 * file beside it, then renamed over it, so a failure leaves the file that
 * was there as it was.
 */
int write_listing(
    const compile_command& _command,
    const context&         _context,
    const std::string&     _listing
)
{
    const std::string file = value_of(_command.output_file);

    // standard output, unless a file was named: `-` names it too
    if ( (file.empty()) ||
         (file == STANDARD_INPUT) )
    {
        return emit(_context,
                    _listing,
                    exit_status::PASSED);
    }

    const int failed = d_cli_write_file(file.c_str(),
                                        _listing.data(),
                                        _listing.size());

    // a file that cannot be written, and the system's reason
    if (failed != 0)
    {
        return d_cli_file_error(_context.program,
                                "write",
                                file.c_str(),
                                failed);
    }

    return exit_status::PASSED;
}

}  // anonymous namespace

namespace subcommand
{

/*
subcommand::check
  The grammar is judged the way the window judges one: a run over an empty
input, which reads it, asks the session, and builds it. The run over the
input is all but free, and it is how a grammar that loops gives itself away.
The run is not traced: nothing check says needs a trace, and where a reading
stops, the model reads the grammar again, traced, to say where.
*/
int check(
    const check_command& _command,
    const context&       _context
)
{
    source            grammar;
    const std::string unread = read_grammar(_command.source,
                                            _context,
                                            grammar);

    // a grammar that cannot be read
    if (!unread.empty())
    {
        return fail(_context,
                    unread);
    }

    const std::string    frontend = frontend_of(_command.source);
    const long           limit    = step_limit_of(_command.run);
    const wb::run_record run      = wb::execute(1,
                                                request_for(frontend,
                                                            grammar,
                                                            source(),
                                                            false,
                                                            limit));
    const bool           judged   = accepted(run);

    // refused: the answer is no, and the reason is said as an error
    if (!judged)
    {
        (void)refuse(_context,
                     grammar.name,
                     run);
    }

    const int verdict = judged ? exit_status::PASSED
                               : exit_status::NOT_ACCEPTED;

    // the status says it all
    if (quiet(_context))
    {
        return verdict;
    }

    // JSON gives either verdict; text, an accepted grammar's, the refusal
    // being said as an error
    if (wants_json(_command.output))
    {
        return emit(_context,
                    check_json(grammar.name,
                               frontend,
                               run),
                    verdict);
    }

    return emit(_context,
                judged ? check_text(run)
                       : std::string(),
                verdict);
}

/*
subcommand::compile
  The session builds the program, and its disassembly is what is written: the
listing the library's own suite holds programs to. When the session refuses,
the model reads the grammar again to say why, since only it can say where a
reading stopped.
*/
int compile(
    const compile_command& _command,
    const context&         _context
)
{
    source            grammar;
    const std::string unread = read_grammar(_command.source,
                                            _context,
                                            grammar);

    // a grammar that cannot be read
    if (!unread.empty())
    {
        return fail(_context,
                    unread);
    }

    const std::string        frontend = frontend_of(_command.source);
    vp::session              judge;
    const vp::compile_result built    =
        judge.load(grammar.bytes,
                   (frontend == "ebnf") ? vp::frontend::ebnf
                                        : vp::frontend::metacircular);

    // refused: the model's sentence says why, and where
    if (!built.ok)
    {
        const long           limit = wb::run_request().step_limit;
        const wb::run_record why   = wb::execute(1,
                                                 request_for(frontend,
                                                             grammar,
                                                             source(),
                                                             false,
                                                             limit));

        return refuse(_context,
                      grammar.name,
                      why);
    }

    return write_listing(_command,
                         _context,
                         judge.disassembly());
}

/*
subcommand::match
  Everything is read before anything runs (read_all). The grammar is judged
by the first run: a refused grammar refuses every input alike.
*/
int match(
    const match_command& _command,
    const context&       _context
)
{
    sources   read;
    const int status = read_all(_command,
                                _context,
                                read);

    // a file named wrongly, or '-' named twice
    if (status != exit_status::PASSED)
    {
        return status;
    }

    const fs::path session = session_of(_command.save);
    match_report   report;

    report.grammar  = read.grammar.name;
    report.frontend = frontend_of(_command.source);
    report.total    = read.inputs.size();
    report.session  = session.empty() ? std::string()
                                      : wb::utf8_from_path(session);

    const std::string trouble = run_inputs(_command,
                                           _context,
                                           read,
                                           session,
                                           report);

    // a run that could not be saved
    if (!trouble.empty())
    {
        return fail(_context,
                    trouble);
    }

    return print_match(_command,
                       _context,
                       report);
}

/*
subcommand::run
  One input, or none: a run over nothing still reads and builds the grammar,
and every part of it but the matches can be looked at.
*/
int run(
    const run_command& _command,
    const context&     _context
)
{
    sources   read;
    const int status = read_all(_command,
                                _context,
                                read);

    // a file named wrongly, or '-' named twice
    if (status != exit_status::PASSED)
    {
        return status;
    }

    const source      over     = read.inputs.empty() ? source()
                                                     : read.inputs[0];
    const std::string frontend = frontend_of(_command.source);
    const fs::path    session  = session_of(_command.save);
    wb::run_request   request  = request_for(frontend,
                                             read.grammar,
                                             over,
                                             !given(_command.run.no_trace),
                                             step_limit_of(_command.run));

    request.memoize = memo_of(_command.run.memoize);

    const wb::run_record record =
        wb::execute(session.empty() ? 1 : next_run_id(session),
                    request);
    std::string          folder;
    std::string          error;

    // saved before anything is printed, so a failure prints only itself
    if ( (!session.empty()) &&
         (!save(record,
                session,
                folder,
                error)) )
    {
        return fail(_context,
                    error);
    }

    json_object head;

    // the input by its name, as the run's parts say it; its argument last
    head.add_string("grammar",
                    read.grammar.name)
        .add_string("input",
                    over.name);

    return print_run(_command,
                     _context,
                     read.grammar.name,
                     head,
                     record,
                     folder,
                     run_tail(over,
                              given(_command.text)));
}

/*
subcommand::replay
  load_request() reads the three files a saved run is made from and nothing
else; the run made from them again is the run that was saved, part for part,
but for when it ran, how long it took and where it came from.
*/
int replay(
    const replay_command& _command,
    const context&        _context
)
{
    const std::string folder = value_of(_command.folder);
    wb::run_request   request;
    std::string       error;

    // not a saved run, or one that cannot be read
    if (!wb::load_request(wb::path_from_utf8(folder),
                          request,
                          &error))
    {
        return fail(_context,
                    error);
    }

    request.origin = folder;

    json_object head;

    head.add_string("folder",
                    folder);

    return print_run(_command,
                     _context,
                     folder,
                     head,
                     wb::execute(1,
                                 request),
                     std::string(),
                     json_object());
}

/*
subcommand::saved
  list_saved() finds every folder holding a run.txt, two levels down at most;
an empty root, or none at all, is no runs rather than a failure.
*/
int saved(
    const saved_command& _command,
    const context&       _context
)
{
    const std::string                root = value_of(_command.root);
    const std::vector<wb::saved_run> runs =
        wb::list_saved(wb::path_from_utf8(root));

    return emit(_context,
                wants_json(_command.output)
                    ? saved_json(root,
                                 runs)
                    : wb::listing_text(saved_listing(root,
                                                     runs)),
                exit_status::PASSED);
}

}  // namespace subcommand

}  // namespace cli
}  // namespace vparse

#endif  // floor, for now
