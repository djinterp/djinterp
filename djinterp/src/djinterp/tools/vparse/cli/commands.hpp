/*******************************************************************************
* vparse [cli]                                                      commands.hpp
*
* The command line's subcommands: the options each one reads, and the
* function that does what it asks. The handlers of check, compile, match,
* run, replay and saved are commands.cpp's; those of bench, corpus and
* compare, which measure a grammar over many inputs, are measures.cpp's.
*   cli.cpp makes the command tree on the kit and fills these records with
* the options it makes; once a parse succeeds, the kit runs the work of the
* subcommand that was used, which hands the handler its record, and the
* handler reads what was given through it. A record holds the options and
* not their values, so the parser stays the one place a value lives, and a
* default is the one the option shows in help.
*   Every handler returns an exit status (cli.hpp) and writes its output only
* once that output is whole, so a run that fails part-way prints the failure
* and nothing else. Every failure -- a file that cannot be read or written, a
* grammar refused, a want of memory -- is said as the kit says one,
* `vparse: error: MESSAGE` on the error stream, and is trouble, 2, so that
* every way the command line can refuse to go on reads the same.
*
* path:      /src/djinterp/tools/vparse/cli/commands.hpp
* link(s):   TBA
* author(s): Samuel 'teer' Neal-Blim                         created: 2026.10.16
*                                                            revised: 2026.10.26
*******************************************************************************/

/*
TABLE OF CONTENTS
=================
1.  OPTION RECORDS
    --------------
    1.  source_options
    2.  run_options
    3.  output_options
    4.  save_options
    5.  corpus_options
2.  SUBCOMMANDS
    -----------
    1.  The records
         1.  check_command
         2.  compile_command
         3.  match_command
         4.  run_command
         5.  replay_command
         6.  saved_command
         7.  bench_command
         8.  corpus_command
         9.  compare_command
    2.  The tree
         1.  command_line
3.  HANDLERS
    --------
    1.  subcommand::check
    2.  subcommand::compile
    3.  subcommand::match
    4.  subcommand::run
    5.  subcommand::replay
    6.  subcommand::saved
    7.  subcommand::bench
    8.  subcommand::corpus
    9.  subcommand::compare
*/

#ifndef VPARSE_CLI_COMMANDS_HPP
#define VPARSE_CLI_COMMANDS_HPP 1

// FLOOR, FOR NOW: below C++17 this file is empty, rather than an error, as
// the model it drives is.
#include "djinterp/env/env.h"  // D_ENV_LANG_*
#if D_ENV_LANG_IS_CPP17_OR_HIGHER

// djinterp
#include "djinterp/c/modules/parse/cli/arg_parser/arg_parser.h"  // dj_arg_command,
                                                         // dj_arg_option
// vparse
#include "./cli.hpp"  // context


namespace vparse
{
namespace cli
{


//==============================================================================
// 1.  OPTION RECORDS
//==============================================================================
//   Groups of options more than one subcommand takes. Each pointer is an
// option its subcommand owns, valid while the parser lives.


// 1.1    source_options
//------------------------------------------------------------------------------
// 1.1.1
// source_options
//   struct: the grammar a subcommand reads, and how: `grammar` is the
// operand GRAMMAR; `frontend` is -f/--frontend; `ebnf` is --ebnf, the same
// as --frontend ebnf.
struct source_options
{
    struct dj_arg_option* grammar  = nullptr;
    struct dj_arg_option* frontend = nullptr;
    struct dj_arg_option* ebnf     = nullptr;
};

// 1.2    run_options
//------------------------------------------------------------------------------
// 1.2.1
// run_options
//   struct: how the machine runs: `step_limit` is --step-limit, `no_trace`
// is --no-trace, and `memoize` is --memoize, the rules the program calls
// through the memo, which collects its values (common.hpp's memo_of());
// each is made only for a subcommand it means something to.
struct run_options
{
    struct dj_arg_option* step_limit = nullptr;
    struct dj_arg_option* no_trace   = nullptr;
    struct dj_arg_option* memoize    = nullptr;
};

// 1.3    output_options
//------------------------------------------------------------------------------
// 1.3.1
// output_options
//   struct: what is printed: `format` is --format, text or json. `-q` is the
// kit's, given anywhere on the command line (common.hpp's quiet()).
struct output_options
{
    struct dj_arg_option* format = nullptr;
};

// 1.4    save_options
//------------------------------------------------------------------------------
// 1.4.1
// save_options
//   struct: whether runs are saved, and where: `save` is --save, `root` is
// --root, the folder session folders are made in.
struct save_options
{
    struct dj_arg_option* save = nullptr;
    struct dj_arg_option* root = nullptr;
};

// 1.5    corpus_options
//------------------------------------------------------------------------------
// 1.5.1
// corpus_options
//   struct: how a corpus is made from a grammar (corpus.hpp's settings):
// `walks` is --walks, `depth` --depth, `length` --length, `repeats`
// --repeats, `misses` --misses and `seed` --seed.
struct corpus_options
{
    struct dj_arg_option* walks   = nullptr;
    struct dj_arg_option* depth   = nullptr;
    struct dj_arg_option* length  = nullptr;
    struct dj_arg_option* repeats = nullptr;
    struct dj_arg_option* misses  = nullptr;
    struct dj_arg_option* seed    = nullptr;
};


//==============================================================================
// 2.  SUBCOMMANDS
//==============================================================================


// 2.1    The records
//------------------------------------------------------------------------------
// 2.1.1
// check_command
//   struct: `vparse check GRAMMAR`. It takes --step-limit for its run over an
// empty input, which is how a grammar that loops gives itself away;
// `run.no_trace` is not made.
struct check_command
{
    struct dj_arg_command* command = nullptr;
    source_options        source;
    run_options           run;
    output_options        output;
};

// 2.1.2
// compile_command
//   struct: `vparse compile GRAMMAR [-o FILE]`; `output_file` is
// -o/--output. It runs nothing over an input, so it has no run options.
struct compile_command
{
    struct dj_arg_command* command     = nullptr;
    source_options        source;
    struct dj_arg_option*  output_file = nullptr;
};

// 2.1.3
// match_command
//   struct: `vparse match GRAMMAR INPUT...`; `inputs` is the operand INPUT,
// and `text` is -t/--text.
struct match_command
{
    struct dj_arg_command* command = nullptr;
    source_options        source;
    struct dj_arg_option*  inputs  = nullptr;
    struct dj_arg_option*  text    = nullptr;
    run_options           run;
    output_options        output;
    save_options          save;
};

// 2.1.4
// run_command
//   struct: `vparse run GRAMMAR [INPUT]`; `inputs` is the operand INPUT,
// which takes one at most -- named as match's is, so that the two are read
// alike -- `text` is -t/--text, and `parts` is -p/--part.
struct run_command
{
    struct dj_arg_command* command = nullptr;
    source_options        source;
    struct dj_arg_option*  inputs  = nullptr;
    struct dj_arg_option*  text    = nullptr;
    struct dj_arg_option*  parts   = nullptr;
    run_options           run;
    output_options        output;
    save_options          save;
};

// 2.1.5
// replay_command
//   struct: `vparse replay FOLDER`; `folder` is the operand FOLDER, and
// `parts` is -p/--part.
struct replay_command
{
    struct dj_arg_command* command = nullptr;
    struct dj_arg_option*  folder  = nullptr;
    struct dj_arg_option*  parts   = nullptr;
    output_options        output;
};

// 2.1.6
// saved_command
//   struct: `vparse saved [--root DIR]`.
struct saved_command
{
    struct dj_arg_command* command = nullptr;
    struct dj_arg_option*  root    = nullptr;
    output_options        output;
};

// 2.1.7
// bench_command
//   struct: `vparse bench GRAMMAR [INPUT...]`; `inputs` is the operand
// INPUT, `text` is -t/--text, and `times` is --times, how many runs of each
// input are timed. With no input, or with --walks, the
// grammar's own corpus is run too.
struct bench_command
{
    struct dj_arg_command* command = nullptr;
    source_options        source;
    struct dj_arg_option*  inputs  = nullptr;
    struct dj_arg_option*  text    = nullptr;
    corpus_options        corpus;
    run_options           run;
    struct dj_arg_option*  times   = nullptr;
    output_options        output;
};

// 2.1.8
// corpus_command
//   struct: `vparse corpus GRAMMAR [-o DIR]`; `folder` is -o/--output, the
// folder each input is written to as a file of its own.
struct corpus_command
{
    struct dj_arg_command* command = nullptr;
    source_options        source;
    corpus_options        corpus;
    struct dj_arg_option*  folder  = nullptr;
    output_options        output;
};

// 2.1.9
// compare_command
//   struct: `vparse compare FIRST SECOND [INPUT...]`; `source.grammar` is
// the operand FIRST, the program compared against, and `second` the
// operand SECOND; --frontend and --ebnf read both. `inputs` is the operand
// INPUT, and `text` is -t/--text; with no input, or with --walks, FIRST's
// own corpus is run too. --memoize is the second program's.
struct compare_command
{
    struct dj_arg_command* command = nullptr;
    source_options        source;
    struct dj_arg_option*  second  = nullptr;
    struct dj_arg_option*  inputs  = nullptr;
    struct dj_arg_option*  text    = nullptr;
    corpus_options        corpus;
    run_options           run;
    output_options        output;
};

// 2.2    The tree
//------------------------------------------------------------------------------
// 2.2.1
// command_line
//   struct: a record for each of the program's subcommands. The records point
// into the kit's program and its parser (cli.cpp), so they live no longer
// than the program, and the whole is never copied.
struct command_line
{
    check_command   check;
    compile_command compile;
    match_command   match;
    run_command     run;
    replay_command  replay;
    saved_command   saved;
    bench_command   bench;
    corpus_command  corpus;
    compare_command compare;
};


//==============================================================================
// 3.  HANDLERS
//==============================================================================
//   Each runs its subcommand once the parse that chose it has succeeded, and
// returns the exit status, an exit_status value: REFUSED for a grammar
// refused, and for a file it names that cannot be read or written, the
// failure said through `_context.program` (common.hpp, section 3). None
// throws, but for std::bad_alloc, which the kit's guard says as `out of
// memory`. They are named for their subcommands, in a namespace of their
// own, so that `subcommand::run` is not taken for cli::run, the program.

namespace subcommand
{

// 3.1    subcommand::check
//------------------------------------------------------------------------------
// 3.1.1
// subcommand::check
//   handler: reads GRAMMAR and judges it, as the session does; prints, when
// the grammar was accepted, its rules and the size of its program, and says
// why, as an error, when it was not. PASSED when accepted, NOT_ACCEPTED --
// its answer, no -- otherwise.
int check(
    const check_command& _command,
    const context&       _context
);

// 3.2    subcommand::compile
//------------------------------------------------------------------------------
// 3.2.1
// subcommand::compile
//   handler: builds GRAMMAR's program and writes the session's listing of it
// -- one instruction a line, after its address -- to standard output or to
// the file -o names, whole or not at all. PASSED when built, REFUSED
// otherwise.
int compile(
    const compile_command& _command,
    const context&         _context
);

// 3.3    subcommand::match
//------------------------------------------------------------------------------
// 3.3.1
// subcommand::match
//   handler: runs GRAMMAR over each INPUT in turn and prints, for each, how
// it went, and how many matched. PASSED when every input matched, NO_MATCH
// when one did not, REFUSED when the grammar was not read or not built.
int match(
    const match_command& _command,
    const context&       _context
);

// 3.4    subcommand::run
//------------------------------------------------------------------------------
// 3.4.1
// subcommand::run
//   handler: runs GRAMMAR over INPUT, or over nothing, and prints the run's
// headline and the parts --part names. PASSED when the input matched,
// NO_MATCH when it did not, REFUSED when the grammar was not read or not
// built.
int run(
    const run_command& _command,
    const context&     _context
);

// 3.5    subcommand::replay
//------------------------------------------------------------------------------
// 3.5.1
// subcommand::replay
//   handler: runs the run saved in FOLDER again, as the window's loading
// does, and prints it as subcommand::run prints a run; the same statuses,
// and REFUSED when FOLDER is not a saved run.
int replay(
    const replay_command& _command,
    const context&        _context
);

// 3.6    subcommand::saved
//------------------------------------------------------------------------------
// 3.6.1
// subcommand::saved
//   handler: lists the runs saved under --root, newest first. PASSED, also
// when there are none.
int saved(
    const saved_command& _command,
    const context&       _context
);

// 3.7    subcommand::bench
//------------------------------------------------------------------------------
// 3.7.1
// subcommand::bench
//   handler: runs GRAMMAR over each INPUT and over its corpus, counting each
// run and timing it apart, and prints the totals, the inputs and the rules.
// The steps repeat exactly; the time is the fastest of --times runs. PASSED
// when every run was made, matched or not; REFUSED when the grammar was not
// read or not built, or --memoize names a rule it does not have.
int bench(
    const bench_command& _command,
    const context&       _context
);

// 3.8    subcommand::corpus
//------------------------------------------------------------------------------
// 3.8.1
// subcommand::corpus
//   handler: makes GRAMMAR's corpus and prints it, or writes each input to a
// file of its own in the folder -o names. PASSED, also for a grammar whose
// start rule no walk can end, which has no corpus; REFUSED when the grammar
// was not read or not built.
int corpus(
    const corpus_command& _command,
    const context&        _context
);

// 3.9    subcommand::compare
//------------------------------------------------------------------------------
// 3.9.1
// subcommand::compare
//   handler: runs FIRST's program and SECOND's over the same inputs and
// names each input they say different things of. PASSED when they agree
// over every one, NO_MATCH when they differ, REFUSED when either grammar
// was not read or not built.
int compare(
    const compare_command& _command,
    const context&         _context
);

}  // namespace subcommand

}  // namespace cli
}  // namespace vparse

#endif  // floor, for now

#endif  // VPARSE_CLI_COMMANDS_HPP
